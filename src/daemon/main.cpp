#include <iostream>
#include <string>
#include <future>
#include <thread>
#include <shared_mutex>
// laiin
#include "../core/crypto/sha3.hpp"
#include "../core/protocol/p2p/node.hpp"
#include "../core/protocol/p2p/routing_table.hpp"
#include "../core/protocol/transport/ip_address.hpp"
#include "../core/protocol/transport/server.hpp"
#include "../core/protocol/rpc/json_rpc.hpp"
#include "../core/protocol/messages/msgpack.hpp"
#include "../core/database/database.hpp"
#include "../core/tools/logger.hpp"
#include "../core/version.hpp"
#include "../core/tools/filesystem.hpp"
#include "../core/network/i2p.hpp"

#include <cxxopts.hpp>

#include <Daemon.h>

#define NEROMON_TAG "\033[1;95m[neromon]:\033[0m "

using namespace laiin;

std::mutex server_mutex;
std::shared_mutex node_mutex; // Define a shared mutex to protect concurrent access to the Node object

std::atomic<bool> running(true);
----------------------

std::string extract_json_payload(const std::string& request) {
    // Find the start of the JSON payload
    const std::string json_start = "\r\n\r\n";
    const auto json_pos = request.find(json_start);
    if (json_pos == std::string::npos) {
        // JSON payload not found
        return "";
    }
    // Extract the JSON payload
    const auto json_payload = request.substr(json_pos + json_start.size());
    return json_payload;
}

----------------------

void rpc_server(const std::string& address) {
    Server server(address, laiin_RPC_DEFAULT_PORT);
    
    while (running) {
        // Accept incoming connections and handle clients concurrently
        if(server.accept() != -1) {            
            std::thread request_thread([&]() {
                // Lock the server mutex
                std::lock_guard<std::mutex> lock(server_mutex);

                // Read json-rpc request object from client
                std::string request_object = server.read();

                // Extract JSON payload from request
                const std::string json_payload = extract_json_payload(request_object);

                std::stringstream http_response;

                // Process JSON-RPC request
                std::string response_object = "";
                if(laiin::rpc::is_json_rpc(json_payload)) {
                    response_object = laiin::rpc::json::process(json_payload);
                    http_response << "HTTP/1.1 200 OK\r\n";
                } else {
                    std::stringstream error_msg;
                    nlohmann::json error_obj;
                    error_obj["jsonrpc"] = "2.0";
                    error_obj["error"] = {};
                    error_obj["error"]["code"] = -32600;
                    error_obj["error"]["message"] = "Invalid Request";//error_obj["error"]["data"] = "Additional error information"; // may be ommited
                    error_obj["id"] = nullptr;
                    error_msg << error_obj.dump();
                    response_object = error_msg.str();
                    http_response << "HTTP/1.1 400 Bad Request\r\n";
                }

                // Build HTTP response string
                http_response << "Content-Type: application/json\r\n";
                http_response << "Content-Length: " << response_object.length() << "\r\n";
                http_response << "\r\n";
                http_response << response_object;

                // Send HTTP response to client
                server.write(http_response.str());
            });

            request_thread.detach();
        }
        
        if (!running) {
            // Stop accepting new connections and exit the loop
            break;
        }        
    }
    // Close the server socket before exiting
    std::cout << "RPC server closed\n";
    server.shutdown();
    server.close();
}

----------------------

void ipc_server(Node& node) {
    // Prevent seed node from being accepted by IPC server 
    // since its only meant to act as an initial contact point for new nodes joining the network
    if (node.is_hardcoded()) {
        std::cout << "Seed node is not allowed to use the local IPC server. Please start another daemon instance to use the GUI or CLI\n";
        return;
    }
    
    Server server("127.0.0.1", laiin_IPC_DEFAULT_PORT);
    
    while (running) {        
        if(server.accept() != -1) {  // ONLY accepts a single client            
            while (true) {
                std::vector<uint8_t> request;
                // wait for incoming message from client
                int recv_size = server.receive(request);
                if (recv_size == 0) {
                    // Republish data before exiting (or in case of client app crashes)
                    node.republish();
                    // Set running to false to close the server (TODO: find a way to gracefully close all threads)
                    ////running = false;
                    // Connection closed by client, break out of loop
                    break;
                }
                std::vector<uint8_t> response;
                {
                    
                    //std::shared_lock<std::shared_mutex> read_lock(node_mutex); // Locking the node_mutex may cause the IPC server to not respond to the client requests for some reason
                    // Perform both read and write operations on the node object
                    // process JSON request and generate response
                    response = laiin::msgpack::process(request, node, true);
                }
                // The shared_lock is destroyed and the lock is released here          
                // send response to client
                server.send(response);
            }
        } 
    }  
    // Close the server socket before exiting // TODO: implement SIGINT (Ctrl+C)
    std::cout << "IPC server closed\n";
    server.close();
}

----------------------

void dht_server(Node& node) {
    std::cout << "******************************************************\n";
    std::cout << "Node ID: " << node.get_id() << "\n";
    std::cout << "IP address: " << node.get_ip_address() << /*" (" << node.get_public_ip_address() << ")*/"\n";
    std::cout << "Port number: " << node.get_port() << "\n\n";
    std::cout << "******************************************************\n";
    // Start the DHT node's main loop in a separate thread
    std::thread run_thread([&]() {
        //std::shared_lock<std::shared_mutex> read_lock(node_mutex);
        node.run();
    });

    // Join the DHT network
    if (!node.is_hardcoded()) {
        //std::shared_lock<std::shared_mutex> read_lock(node_mutex);
        node.join(); // A seed node cannot join the network
    }
    
    if(node.is_hardcoded()) {
        node.rebuild_routing_table();
    }

    run_thread.join(); // Wait for the run thread to finish
}

----------------------

int main(int argc, char** argv)
{
    std::string daemon { "neromon" };
    std::string daemon_version { daemon + " v" + std::string(laiin_DAEMON_VERSION) };
    cxxopts::Options options(daemon, std::string(daemon_version));

    options.add_options()
        ("h,help", "Print usage")
        ("v,version", "Show version")
        ("b,bootstrap", "Run this node as a bootstrap node")//("bl,bootstrap-lazy", "Run this node as a bootstrap node without specifying multiaddress")//("c,config", "Path to configuration file", cxxopts::value<std::string>())
        ("rpc,enable-rpc", "Enables the RPC daemon server")
        ("public,public-node", "Make your daemon into a public node")
    ;
    
    auto result = options.parse(argc, argv);
    
    if(result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }
    
    if(result.count("version")) {
        std::cout << daemon << " version " << std::string(laiin_DAEMON_VERSION) << std::endl;
        exit(0);
    }       

    if(result.count("config")) {
        std::string config_path = result["config"].as<std::string>();
        if(!config_path.empty()) {}
    }
    
    std::string ip_address = laiin_LOOPBACK_ADDRESS;
    if(result.count("public")) {
        ip_address = laiin_ANY_ADDRESS;
    }
    
    // create "datastore" folder within "~/.config/laiin/" path (to prevent sqlite3_open: out of memory error)
    std::string data_dir = laiin_DEFAULT_DATABASE_PATH;
    if(!laiin::filesystem::is_directory(data_dir)) {
        if(!laiin::filesystem::make_directory(data_dir)) {
            throw std::runtime_error("Failed to create laiin data dir");
        }
    }
    
    db::Sqlite3 * database = laiin::get_database();
    if(!database->table_exists("mappings")) { 
        database->execute("CREATE VIRTUAL TABLE mappings USING fts5(search_term, key, content, tokenize='porter unicode61');");
    }
    
    std::thread i2pd_thread([&]() { 
        if (Daemon.init(argc, argv))
	    {
		    if (Daemon.start())
			    Daemon.run ();
		    else {
			    if(laiin::is_i2p_running()) {
		            std::cout << "\033[90mi2pd was already running in the background\033[0m\n";
		            return;
		        } else {
		            throw std::runtime_error("i2pd failed to start");
		        }
		    }
		    Daemon.stop();
	    }
	});
    
    laiin::Node node("0.0.0.0"/*ip_address*/, laiin_P2P_DEFAULT_PORT, true);
    
    if(result.count("bootstrap")) {   
        std::cout << "Switching to bootstrap mode ...\n";
        node.set_bootstrap(true);
        assert(node.get_ip_address() == laiin_ANY_ADDRESS && "Bootstrap node is not public");
        // ALWAYS use address "0.0.0.0" for bootstrap nodes so that it is reachable by all nodes in the network, regardless of their location.
    }
    
    std::thread ipc_thread([&node]() { ipc_server(node); }); // For IPC communication between the local GUI client and the local daemon server
    std::thread dht_thread([&node]() { dht_server(node); }); // DHT communication for peer discovery and data storage
    std::thread rpc_thread;  // Declare the thread object // RPC communication for processing requests from outside clients (disabled by default)
    
    if(result.count("rpc")) {
        std::cout << "RPC enabled\n";
        rpc_thread = std::thread(rpc_server, std::cref(ip_address));  // Initialize the thread object 
    }
    
    // Wait for all threads to finish
    if (rpc_thread.joinable()) {
        rpc_thread.join();
    }
    ipc_thread.join();
    dht_thread.join();
    i2pd_thread.join();
    
    return 0;
}
