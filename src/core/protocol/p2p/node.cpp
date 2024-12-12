#include "node.hpp"

#include "dht_rescode.hpp"
#include "../../crypto/sha3.hpp"
#include "../../crypto/rsa.hpp"
#include "routing_table.hpp"
#include "../transport/ip_address.hpp"
#include "../messages/msgpack.hpp"
#include "../../version.hpp"
#include "../../tools/base64.hpp"
#include "key_mapper.hpp"
#include "../../tools/string.hpp"
#include "../../tools/timestamp.hpp"
#include "../../database/database.hpp"
#include "../../network/i2p.hpp"

#include <nlohmann/json.hpp>

#include <utils/monero_utils.h>
#include <wallet/monero_wallet_full.h>

#include <cstring> // memset
#include <future>
#include <iomanip> // std::set*
#include <cassert>
#include <thread>
#include <unordered_set>
#include <set>

namespace laiin_crypto = laiin::crypto;
namespace laiin_timestamp = laiin::timestamp;
namespace laiin_string = laiin::string;

namespace laiin {

Node::Node(const std::string& address, int port, bool local) : sockfd(-1), bootstrap(false), check_counter(0) { 
    // Convert URL to IP (in case it happens to be a url)
    std::string ip_address = laiin::ip::resolve(address);
    // Generate a random node ID - use public ip address for uniqueness
    public_ip_address = (local) ? get_public_ip_address() : ip_address;
    id = generate_node_id(public_ip_address, port);
    ----
    memset(&storage, 0, sizeof(storage));
    if(is_ipv4(ip_address)) storage.ss_family = AF_INET;
    if(is_ipv6(ip_address)) storage.ss_family = AF_INET6;
    ----
    // If this node is a local node that you own
    if(local == true) {
        // Create a UDP socket
        sockfd = socket(storage.ss_family, SOCK_DGRAM, 0);
        if(sockfd < 0) {
            perror("socket");
            throw std::runtime_error("::socket failed");
        }

        // set a timeout of TIMEOUT_VALUE seconds for recvfrom
        struct timeval tv;
        tv.tv_sec = laiin_DHT_RECV_TIMEOUT;  // timeout in seconds
        tv.tv_usec = 0; // timeout in microseconds
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
            std::cerr << "Error setting socket options" << std::endl;
            close(sockfd);
            exit(0);
        }
        
        // Make sockfd non-blocking
        int flags = fcntl(sockfd, F_GETFL, 0);
        if(flags == -1) {
            perror("fcntl");
        }
        if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl");
        }

        // To avoid "bind" error and allow multiple nodes to run on the same IP address, we can use different port numbers for each local node we create
        // Bind to port
        int port_dynamic = port; // initial port number
        const int MAX_PORT_NUM = 65535; // port cannot go past this number
        bool use_ephemeral_port = false;

        while(true) {        
            if(storage.ss_family == AF_INET) {
                sockin = {0};//memset(&sockin, 0, sizeof(sockin)); // both approaches are valid, but memset is more reliable and portable
                sockin.sin_family = storage.ss_family;
                sockin.sin_port = htons(port_dynamic);//htons(std::stoi(std::to_string(port_dynamic))); // the second approach may be useful in cases where the port number is being manipulated as a string or integer elsewhere in the code
                if(inet_pton(storage.ss_family, ip_address.c_str(), &sockin.sin_addr) <= 0) { //sockin.sin_addr.s_addr = htonl(INADDR_ANY) - binds to any available network interface // inet_addr(ip_address.c_str()) - binds to a specific ip // recommended to use inet_pton() over inet_addr() when working with networking in modern systems.
                    perror("inet_pton");
                }
            }
            if(storage.ss_family == AF_INET6) {
                memset(&sockin6, 0, sizeof(sockin6));
                sockin6.sin6_family = storage.ss_family;
                sockin6.sin6_port = htons(port_dynamic);
                if(inet_pton(storage.ss_family, ip_address.c_str(), &sockin6.sin6_addr) <= 0) { 
                    perror("inet_pton");
                }
            }
            
            if(bind(sockfd, (storage.ss_family == AF_INET6) ? (struct sockaddr *)&sockin6 : (struct sockaddr *)&sockin, (storage.ss_family == AF_INET6) ? sizeof(sockin6) : sizeof(sockin)) == 0) {
                // Update node ID in case the port ever changes
                id = generate_node_id(public_ip_address, port_dynamic);
                std::cout << "DHT node bound to port " << port_dynamic << std::endl;
                break;
            }
            std::cout << "Port " << port_dynamic << " already in use." << std::endl;

            use_ephemeral_port = true;
            if(use_ephemeral_port) {
                // Bind to the ephemeral port number
                sockin.sin_port = htons(0);
                if(bind(sockfd, (storage.ss_family == AF_INET6) ? (struct sockaddr *)&sockin6 : (struct sockaddr *)&sockin, (storage.ss_family == AF_INET6) ? sizeof(sockin6) : sizeof(sockin)) == 0) {
                    // Get the actual port number used by the socket
                    struct sockaddr_in local_addr;
                    socklen_t local_addr_len = sizeof(local_addr);
                    getsockname(sockfd, (struct sockaddr*)&local_addr, &local_addr_len);
                    port_dynamic = ntohs(local_addr.sin_port);
                    // Generate new node ID with the ephemeral port number
                    id = generate_node_id(public_ip_address, port_dynamic);
                    std::cout << "DHT node bound to ephemeral port " << port_dynamic << std::endl;
                    break;
                }
            }
            // Use the next available port number
            port_dynamic++;
            if (port_dynamic > MAX_PORT_NUM) {
                throw std::runtime_error("Unable to bind to any available port.");
            }
        }

        // Node is now bound to a unique port number
    }
    
    // If this is an external node that you do not own
    if(local == false) {
        sockfd = socket(storage.ss_family, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("socket");
        }
        // This socket is not meant to be bound to any port. It will only be used to retrieve the IP address and port number
    
        if(storage.ss_family == AF_INET) {
            memset(&sockin, 0, sizeof(sockin));
            sockin.sin_family = storage.ss_family;
            sockin.sin_port = htons(port);
            if(inet_pton(storage.ss_family, ip_address.c_str(), &sockin.sin_addr) <= 0) { 
                perror("inet_pton");
            }
        }
        if(storage.ss_family == AF_INET6) {
            memset(&sockin6, 0, sizeof(sockin6));
            sockin6.sin6_family = storage.ss_family;
            sockin6.sin6_port = htons(port);
            if(inet_pton(storage.ss_family, ip_address.c_str(), &sockin6.sin6_addr) <= 0) { 
                perror("inet_pton");
            }
        }
        // Set socket options, such as timeout or buffer size, if needed
    }
    
    // Create the routing table with an empty vector of nodes
    if(!routing_table.get()) {
        routing_table = std::make_unique<RoutingTable>(std::vector<Node*>{});
        routing_table->my_node_id = this->id;
    } 
       
    // Initialize key mapper
    if(!key_mapper.get()) {
        key_mapper = std::make_unique<KeyMapper>();
    }
}

Node::Node(Node&& other) noexcept
    : id(std::move(other.id)),
      version(std::move(other.version)),
      data(std::move(other.data)),
      providers(std::move(other.providers)),
      //server(std::move(other.server)),
      sockfd(other.sockfd),
      sockin(std::move(other.sockin)),
      sockin6(std::move(other.sockin6)),
      storage(std::move(other.storage)),
      routing_table(std::move(other.routing_table)),
      public_ip_address(std::move(other.public_ip_address)),
      bootstrap(other.bootstrap),
      check_counter(other.check_counter)
{
    // Reset the moved-from object's members to a valid state
    other.sockfd = -1;
    // ... reset other members ...
}

Node::~Node() {
    if(sockfd > 0) {
        close(sockfd);
        sockfd = -1;
    }
}

std::string Node::generate_node_id(const std::string& address, int port) {
    std::string node_info = address + ":" + std::to_string(port);
    std::string hash = laiin_crypto::sha3_256(node_info);
    return hash.substr(0, NUM_BITS / 4);
}

void Node::join() {
    if(sockfd < 0) throw std::runtime_error("socket is dead");

    // Bootstrap the DHT node with a set of known nodes
    for (const auto& bootstrap_node : BOOTSTRAP_NODES) {
        std::cout << "\033[35;1mJoining bootstrap node - " << bootstrap_node.first << ":" << bootstrap_node.second << "\033[0m\n";

        // Ping each known node to confirm that it is online - the main bootstrapping primitive. If a node replies, and if there is space in the routing table, it will be inserted.
        if(!ping(bootstrap_node.first, bootstrap_node.second)) {
            std::cerr << "ping: failed to ping bootstrap node\n"; continue;
        }
        
        // Add the bootstrap node to routing table (optional)
        auto seed_node = std::make_unique<Node>((bootstrap_node.first == "127.0.0.1") ? this->public_ip_address : bootstrap_node.first, bootstrap_node.second, false);
        seed_node->set_bootstrap(true);
        Node& seed_node_ref = *seed_node; // take a reference to the Node object (to avoid segfault)
        //routing_table->add_node(std::move(seed_node)); // seed_node becomes invalid after we move ownership to routing table so it cannot be used
        
        // Send a "find_node" message to the bootstrap node and wait for a response message
        auto nodes = send_find_node(this->id, (bootstrap_node.first == "127.0.0.1") ? "127.0.0.1" : seed_node_ref.get_ip_address(), seed_node_ref.get_port());//std::cout << "Sending find_node message to " << seed_node_ref.get_ip_address() << ":" << seed_node_ref.get_port() << "\n";
        if(nodes.empty()) {
            std::cerr << "find_node: No nodes found\n"; continue;
        }
        
        // Then add nodes to the routing table
        for (auto& node : nodes) {
            // Ping the received nodes first
            std::string node_ip = (node->get_ip_address() == this->public_ip_address) ? "127.0.0.1" : node->get_ip_address();
            if(!ping(node_ip, node->get_port())) {
                continue; // Skip the node and continue with the next iteration
            }
            // Process the response and update the routing table if necessary
            routing_table->add_node(std::move(node));
        }
    }
    
    // Print the contents of the routing table
    routing_table->print_table();
}

bool Node::ping(const std::string& address, int port) {
    return send_ping(address, port);
}

std::vector<Node*> Node::find_node(const std::string& target, int count) const { 
    if(!routing_table.get()) {
        return {};
    }
    // Get the nodes from the routing table that are closest to the target (node id or key)
    std::vector<Node*> closest_nodes = routing_table->find_closest_nodes(target, count);
    return closest_nodes;
}

int Node::put(const std::string& key, const std::string& value) {
    // If data is a duplicate, skip it and return success (true)
    if (has_key(key) && get(key) == value) {
        std::cout << "Data already exists. Skipping ...\n";
        return true;
    }
    
    if(!validate(key, value)) {
        return false;
    }
    
    // If node has the key but the value has been altered, compare both old and new values before updating the value
    if (has_key(key) && get(key) != value) {
        std::cout << "Updating value for key (" << key << ") ...\n";
        return set(key, value);
    }
    
    //std::unique_lock<std::shared_mutex> lock(data_mutex);
    data[key] = value;
    return has_key(key); // boolean
}

int Node::store(const std::string& key, const std::string& value) {    
    return put(key, value);
}

std::string Node::get(const std::string& key) const { 
    //std::shared_lock<std::shared_mutex> lock(data_mutex);
    auto it = data.find(key);
    if (it != data.end()) {
        return it->second;
    }
    return "";
}

std::string Node::find_value(const std::string& key) const {
    return get(key);
}

int Node::remove(const std::string& key) {
    //std::unique_lock<std::shared_mutex> lock(data_mutex);
    data.erase(key); // causes Segfault when called in thread :(
    return (data.count(key) == 0); // boolean
}

int Node::remove_all() {
    data.clear();
    return data.empty();
}

void Node::map(const std::string& key, const std::string& value) {
    key_mapper->add(key, value);
}

int Node::set(const std::string& key, const std::string& value) {
    nlohmann::json json = nlohmann::json::parse(value); // Already validated in put() so we just need to parse it without checking for errors
    
    std::string current_value = get(key);
    nlohmann::json current_json = nlohmann::json::parse(current_value);
    
    // Verify that no immutable fields have been altered
    std::string metadata = json["metadata"].get<std::string>();
    if(metadata != current_json["metadata"].get<std::string>()) { std::cerr << "\033[91mMetadata mismatch\033[0m\n"; return false; } // metadata is immutable
    if(metadata == "user") {
        if(!json["monero_address"].is_string()) { return false; }
        std::string user_id = json["monero_address"].get<std::string>();
        if(user_id != current_json["monero_address"].get<std::string>()) { std::cerr << "\033[91mUser ID (Monero Primary Address) mismatch\033[0m\n"; return false; } // monero_address is immutable
    }
    if(metadata == "listing") {
        if(!json["id"].is_string()) { return false; }
        if(!json["seller_id"].is_string()) { return false; }
        std::string listing_uuid = json["id"].get<std::string>();
        std::string seller_id = json["seller_id"].get<std::string>(); // seller_id (monero primary address)
        if(listing_uuid != current_json["id"].get<std::string>()) { std::cerr << "\033[91mListing UUID mismatch\033[0m\n"; return false; } // id is immutable
        if(seller_id != current_json["seller_id"].get<std::string>()) { std::cerr << "\033[91mSeller ID mismatch\033[0m\n"; return false; } // seller_id is immutable
    }
    if(metadata == "product_rating" || metadata == "seller_rating") {
        if(!json["rater_id"].is_string()) { return false; }
        std::string rater_id = json["rater_id"].get<std::string>(); // rater_id (monero primary address)
        if(rater_id != current_json["rater_id"].get<std::string>()) { std::cerr << "\033[91mRater ID mismatch\033[0m\n"; return false; } // rater_id is immutable
    }
    
    // Make sure the signature has been updated
    if (json.contains("signature") && json["signature"].is_string()) {
        std::string signature = json["signature"].get<std::string>();
        if(signature == current_json["signature"].get<std::string>()) { std::cerr << "\033[91mSignature is outdated\033[0m\n"; return false; }
    }
    
    // Note: All messages are unique and cannot be modified once created, so they should not ever be able to pass through this function
    // No "last_updated" field found in the modified value, only the current value, discard the new value (its likely outdated) - untested
    if(!json.contains("last_updated") && current_json.contains("last_updated")) {
        std::cout << "Value for key (" << key << ") is already up to date" << std::endl;
        return true;
    }
    // Compare "last_updated" field of modified value and current value - untested
    if(json.contains("last_updated") && json["last_updated"].is_string()) {
        std::string last_updated = json["last_updated"].get<std::string>();
                
        // Check if current value has a last_updated field too
        if(current_json.contains("last_updated") && current_json["last_updated"].is_string()) {
            std::string current_last_updated = current_json["last_updated"].get<std::string>();
            // Compare the new json's last_updated timestamp with the current json's own
            // And choose whichever has the most recent timestamp then exit the function
            std::string most_recent_timestamp = laiin_timestamp::get_most_recent_timestamp(last_updated, current_last_updated);
            // If this node has the up-to-date value, return true as there is no need to update
            if(most_recent_timestamp == current_last_updated) {
                std::cout << "Value for key (" << key << ") is already up to date" << std::endl;
                return true;
            }
        }
        // If current value does not have a last_updated field 
        // then it means it's probably outdated, so do nothing.
        // It will be replaced with the new value at the end of the scope
    }
    
    data[key] = value;
    return has_key(key); // boolean
}

void Node::add_provider(const std::string& data_hash, const Peer& peer) {
    // Check if the data_hash is already in the providers map
    auto it = providers.find(data_hash);
    if (it != providers.end()) {
        // If the data_hash is already in the map, check for duplicates
        for (const auto& existing_peer : it->second) {
            if (existing_peer.address == peer.address && existing_peer.port == peer.port) {
                // Peer with the same address and port already exists, so don't add it again
                std::cout << "Provider (\033[36m" << peer.address + ":" + std::to_string(peer.port) << "\033[0m) for hash (" << data_hash << ") already exists" << std::endl;
                return;
            }
        }
        // If the data_hash is already in the map, add the peer to the vector of peers
        it->second.push_back(peer);
        //std::cout << "Provider (\033[36m" << peer.address + ":" + std::to_string(peer.port) << "\033[0m) for hash (" << data_hash << ") has been added" << std::endl;
    } else {
        // If the data_hash is not in the map, create a new vector of peers and add the peer
        providers.emplace(data_hash, std::deque<Peer>{peer});
        //std::cout << "Provider (\033[36m" << peer.address + ":" + std::to_string(peer.port) << "\033[0m) for hash (" << data_hash << ") has been added (0)" << std::endl;
    }
}

void Node::remove_providers(const std::string& data_hash) {
    // Find the data_hash entry in the providers map
    auto it = providers.find(data_hash);
    if (it != providers.end()) {
        // If the data_hash exists, remove the entry from the map
        providers.erase(it);
    }
}

void Node::remove_provider(const std::string& data_hash, const std::string& address, int port) {
    // Find the data_hash entry in the providers map
    auto it = providers.find(data_hash);
    if (it != providers.end()) {
        // Iterate through the vector of providers for the data_hash
        auto& peers = it->second;
        for (auto peer_it = peers.begin(); peer_it != peers.end(); ) {
            if (peer_it->address == address && peer_it->port == port) {
                // If the address and port match, remove the provider from the vector
                peer_it = peers.erase(peer_it);
            } else {
                ++peer_it;
            }
        }

        // If the vector becomes empty after removing the provider, remove the entry from the map
        if (peers.empty()) {
            providers.erase(it);
        }
    }
}

std::deque<Peer> Node::get_providers(const std::string& data_hash) const {
    std::deque<Peer> peers = {};

    // Check if data_hash is in providers
    auto data_hash_it = providers.find(data_hash);
    if (data_hash_it != providers.end()) {
        // If data_hash is in providers, get the vector of peers
        peers = data_hash_it->second;
    }
    
    return peers;
}

void Node::persist_routing_table(const std::string& address, int port) {
    if(!is_hardcoded()) return; // Regular nodes cannot run this function
    
    db::Sqlite3 * database = laiin::get_database();
    if(!database) throw std::runtime_error("database is not opened");
    
    if(!database->table_exists("routing_table")) { 
        database->execute("CREATE TABLE routing_table("
        "ip_address TEXT, port INTEGER, UNIQUE(ip_address, port));");
    }
    
    database->execute_params("INSERT INTO routing_table (ip_address, port) VALUES (?1, ?2);", { address, std::to_string(port) });
}

void Node::rebuild_routing_table() {
    if(!is_hardcoded()) return; // Regular nodes cannot run this function
    
    db::Sqlite3 * database = laiin::get_database();
    if(!database) throw std::runtime_error("database is not opened");
    if(!database->table_exists("routing_table")) {
        return; // Table does not exist, exit function
    }
    // Prepare statement
    std::string command = "SELECT ip_address, port FROM routing_table;";
    sqlite3_stmt * stmt = nullptr;
    if(sqlite3_prepare_v2(database->get_handle(), command.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "\033[91msqlite3_prepare_v2: " + std::string(sqlite3_errmsg(database->get_handle())) << "\033[0m" << std::endl;
        return;
    }
    // Check if there is any data returned by the statement
    if(sqlite3_column_count(stmt) > 0) {
        std::cout << "\033[35;1mRebuilding routing table from backup ...\033[0m" << std::endl;
    }
    // Get all table values row by row
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        std::string ip_address; int port = 0;
        for(int i = 0; i < sqlite3_column_count(stmt); i++) {
            std::string column_value = (sqlite3_column_text(stmt, i) == nullptr) ? "NULL" : reinterpret_cast<const char *>(sqlite3_column_text(stmt, i));
            if(column_value == "NULL") continue;
            if(i == 0) ip_address = column_value;
            if(i == 1) port = sqlite3_column_int(stmt, i);//std::stoi(column_value);
            if(port == 0) continue;
            
            if(!ping((ip_address == this->public_ip_address) ? "127.0.0.1" : ip_address, port)) {
                std::cerr << "ping: failed to ping node\n"; 
                // Remove unresponsive nodes from database
                database->execute_params("DELETE FROM routing_table WHERE ip_address = ?1 AND port = ?2", { ip_address, std::to_string(port) });
                continue;
            }
            
            auto node = std::make_unique<Node>(ip_address, port, false);
            if(!node->is_hardcoded()) {
                routing_table->add_node(std::move(node));
            }
        }
    }
    // Finalize statement
    sqlite3_finalize(stmt);
}

std::vector<uint8_t> Node::send_query(const std::string& address, uint16_t port, const std::vector<uint8_t>& message, int recv_timeout) {
    // Step 2: Resolve the hostname and construct a destination address
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // use IPv4
    hints.ai_socktype = SOCK_DGRAM; // use UDP //SOCK_STREAM; // use TCP
    if (getaddrinfo(address.c_str(), std::to_string(port ? port : laiin_P2P_DEFAULT_PORT).c_str(), &hints, &res) != 0) {
        std::cerr << "Error resolving hostname" << std::endl; // probably the wrong family
        return {};
    }

    if (res == NULL) {
        std::cerr << "Error resolving hostname" << std::endl;
        return {};
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = hints.ai_family;
    memcpy(&dest_addr, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    //
    // Step 3: Create a new socket descriptor and send the ping message to the server
    // Note: This is a separate socket used for actively sending/receiving queries while the main socket, which is non-blocking is used for listening/responding to messages
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        return {};
    }

    if (sendto(socket_fd/*sockfd*/, message.data(), message.size(), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("sendto");
        close(socket_fd);
        return {};
    }
    // Set a timeout for the receive operation
    // Note: Setting a timeout is better than setting the socket to non-blocking because if the socket is non-blocking then it will never receive the pong because it does not wait for the pong message so it fails immediately (returns immediately, regardless of whether data is available or not).
    struct timeval timeout;
    timeout.tv_sec = recv_timeout; // Timeout in seconds
    timeout.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        perror("setsockopt");
        close(socket_fd);
        return {};
    }
    //
    // Step 4: Receive the pong message from the server
    std::vector<uint8_t> receive_buffer(laiin_RECV_BUFFER_SIZE);
    socklen_t fromlen = sizeof(struct sockaddr_in);
    int bytes_received = recvfrom(socket_fd/*sockfd*/, receive_buffer.data(), receive_buffer.size(), 0,
                                  (struct sockaddr*)&dest_addr, &fromlen);
    if (bytes_received < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("recvfrom");
            close(socket_fd);
            return {};
        }
        
        // No data available at the moment, you can handle it based on your needs
        // For example, you can retry receiving or return an empty vector
        
        close(socket_fd);
        return {};
    }
    receive_buffer.resize(bytes_received);
    close(socket_fd);

    return receive_buffer;
}

bool Node::send_ping(const std::string& address, int port) {
    // Create the ping message
    std::string transaction_id = msgpack::generate_transaction_id();
    nlohmann::json query_object;
    query_object["tid"] = transaction_id;
    query_object["query"] = "ping";
    query_object["args"]["id"] = this->id;
    int our_port = get_port();
    if(our_port != laiin_P2P_DEFAULT_PORT) {
        query_object["args"]["port"] = our_port; // for testing on local network. This cannot be removed since the two primary sockets used in the protocol have different ports with the "port" being the actual port
    }
    query_object["version"] = std::string(laiin_DHT_VERSION);
    
    auto ping_message = nlohmann::json::to_msgpack(query_object);
    //
    auto receive_buffer = send_query(address, port, ping_message, laiin_DHT_PING_TIMEOUT);
    //
    // Parse the pong message and extract the transaction ID and response fields
    nlohmann::json pong_message;
    try {
        pong_message = nlohmann::json::from_msgpack(receive_buffer);
    } catch (const std::exception& e) {
        std::cerr << "\033[91mNode " << address << ":" << port << " did not respond\033[0m" << std::endl;
        return false;
    }
    if (!pong_message.contains("response") || pong_message.contains("error")) {
        std::cerr << "Received invalid pong message" << std::endl;
        return false;
    }
    std::cout << "\033[32m" << pong_message.dump() << "\033[0m\n";
    std::string received_transaction_id = pong_message["tid"].get<std::string>();
    auto response_object = pong_message["response"];
    std::string node_id = response_object["id"].get<std::string>();
    
    // Check that the pong message corresponds to the ping message
    if (received_transaction_id != transaction_id) {
        std::cerr << "Received pong message with incorrect transaction ID" << std::endl;
        return false;
    }

    return true;
}

std::vector<std::unique_ptr<Node>> Node::send_find_node(const std::string& target, const std::string& address, uint16_t port) {
    std::string transaction_id = msgpack::generate_transaction_id();

    nlohmann::json query_object;
    query_object["tid"] = transaction_id;
    query_object["query"] = "find_node";
    query_object["args"]["id"] = this->id;
    query_object["args"]["target"] = target;
    query_object["version"] = std::string(laiin_DHT_VERSION);
    
    auto find_node_message = nlohmann::json::to_msgpack(query_object);
    --
    auto receive_buffer = send_query(address, port, find_node_message);
    --
    // Parse nodes message
    nlohmann::json nodes_message;
    try {
        nodes_message = nlohmann::json::from_msgpack(receive_buffer);
    } catch (const std::exception& e) {
        std::cerr << "Node \033[91m" << address << ":" << port << "\033[0m did not respond" << std::endl;
        return {};
    }
    std::cout << "\033[32m" << nodes_message.dump() << "\033[0m\n";
    // Create node vector and store nodes from the message inside the vector
    std::vector<std::unique_ptr<Node>> nodes;
    if (nodes_message.contains("response") && nodes_message["response"].contains("nodes")) {
        for (auto& node_json : nodes_message["response"]["nodes"]) {
            if (node_json.contains("ip_address") && node_json.contains("port")) {
                std::string ip_address = node_json["ip_address"];
                uint16_t port = node_json["port"];
                auto node = std::make_unique<Node>(ip_address, port, false);
                if (node->id != this->id && !routing_table->has_node(node->id)) { // add node to vector only if it's not the current node
                    nodes.push_back(std::move(node));
                }
            }
        }
    }
    return nodes;
}

int Node::send_put(const std::string& key, const std::string& value) {
    if(!is_value_publishable(value)) { return 0; } // Prevent listings from being published
    
    nlohmann::json query_object;
    query_object["query"] = "put";
    query_object["args"]["id"] = this->id;
    query_object["args"]["key"] = key;
    query_object["args"]["value"] = value;
    query_object["version"] = std::string(laiin_DHT_VERSION);
    // Determine which nodes get to put the key-value data in their hash table
    std::vector<Node *> closest_nodes = find_node(key, laiin_DHT_REPLICATION_FACTOR); // 5=replication factor
    
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(closest_nodes.begin(), closest_nodes.end(), rng);
    
    // Keep track of the number of nodes to which put messages have been sent
    size_t nodes_sent_count = 0;
    std::unordered_set<Node*> sent_nodes;
    std::unordered_set<Node*> failed_nodes;
    // Send put message to the closest nodes
    for(auto const& node : closest_nodes) {
        if (node == nullptr) continue;
        
        std::string transaction_id = msgpack::generate_transaction_id();
        query_object["tid"] = transaction_id; // tid should be unique for each put message
        std::vector<uint8_t> query_put = nlohmann::json::to_msgpack(query_object);
    
        std::string node_ip = (node->get_ip_address() == this->public_ip_address) ? "127.0.0.1" : node->get_ip_address();
        uint16_t node_port = node->get_port();
        std::cout << "Sending put request to \033[36m" << node_ip << ":" << node_port << "\033[0m\n";
        auto receive_buffer = send_query(node_ip, node_port, query_put);
        // Process the response here
        nlohmann::json response_put;
        try {
            response_put = nlohmann::json::from_msgpack(receive_buffer);
        } catch (const std::exception& e) {
            std::cerr << "Node \033[91m" << node_ip << ":" << node_port << "\033[0m did not respond" << std::endl;
            node->check_counter += 1;
            failed_nodes.insert(node);
            continue; // Continue with the next closest node if this one fails
        }   
        // Add the node to the sent_nodes set
        sent_nodes.insert(node);
        // Show response and increase count
        std::cout << ((response_put.contains("error")) ? ("\033[91m") : ("\033[32m")) << response_put.dump() << "\033[0m\n";
        if(response_put.contains("response")) {
            add_provider(key, { node->get_ip_address(), node_port });
        }
        nodes_sent_count++;
    }
    // Handle the case when there are fewer closest nodes than laiin_DHT_REPLICATION_FACTOR - this most likely means that there are not enough nodes in the network
    if (closest_nodes.size() < laiin_DHT_REPLICATION_FACTOR) return nodes_sent_count;
    
    // If the desired number of nodes is not reached due to non-responses, replace failed nodes with new nodes and continue sending put messages
    if (nodes_sent_count < laiin_DHT_REPLICATION_FACTOR) {
        size_t remaining_nodes = laiin_DHT_REPLICATION_FACTOR - nodes_sent_count;
        std::cout << "Nodes remaining: " << remaining_nodes << " out of " << laiin_DHT_REPLICATION_FACTOR << "\n";
        std::cout << "Routing table total node count: " << routing_table->get_node_count() << "\n";
        
        std::vector<Node*> all_nodes = find_node(key, routing_table->get_node_count());
        std::vector<Node*> replacement_nodes;
        
        // Iterate over all the nodes in the routing table
        for (const auto& node : all_nodes) {
            if (std::find(closest_nodes.begin(), closest_nodes.end(), node) == closest_nodes.end() &&
                std::find(failed_nodes.begin(), failed_nodes.end(), node) == failed_nodes.end() &&
                sent_nodes.find(node) == sent_nodes.end()) {
                    replacement_nodes.push_back(node);
            }
        }
        
        if (replacement_nodes.size() < remaining_nodes) {
            // Handle the case where there are not enough replacement nodes available
            std::cerr << "Not enough replacement nodes available.\n";
        } else {
            // Select the required number of replacement nodes
            replacement_nodes.resize(remaining_nodes);

            // Send put messages to the replacement nodes
            for (const auto& replacement_node : replacement_nodes) {
                std::string transaction_id = msgpack::generate_transaction_id();
                query_object["tid"] = transaction_id;
                std::vector<uint8_t> query_put = nlohmann::json::to_msgpack(query_object);

                std::string node_ip = (replacement_node->get_ip_address() == this->public_ip_address) ? "127.0.0.1" : replacement_node->get_ip_address();
                uint16_t node_port = replacement_node->get_port();
                std::cout << "Sending put request to \033[36m" << node_ip << ":" << node_port << "\033[0m\n";
                auto receive_buffer = send_query(node_ip, node_port, query_put);
                // Process the response and update the nodes_sent_count and sent_nodes accordingly
                nlohmann::json response_put;
                try {
                    response_put = nlohmann::json::from_msgpack(receive_buffer);
                } catch (const std::exception& e) {
                    std::cerr << "Node \033[91m" << node_ip << ":" << node_port << "\033[0m did not respond" << std::endl;
                    replacement_node->check_counter += 1;
                    continue; // Continue with the next replacement node if this one fails
                }   
                // Show response and increase count
                std::cout << ((response_put.contains("error")) ? ("\033[91m") : ("\033[32m")) << response_put.dump() << "\033[0m\n";
                if(response_put.contains("response")) {
                    add_provider(key, { replacement_node->get_ip_address(), node_port });
                }
                nodes_sent_count++;
            }
        }
    }
    return nodes_sent_count;
}

int Node::send_store(const std::string& key, const std::string& value) {
    return send_put(key, value);
}

std::string Node::send_get(const std::string& key) {
    nlohmann::json query_object;
    query_object["query"] = "get";
    query_object["args"]["id"] = this->id;
    query_object["args"]["key"] = key;
    query_object["version"] = std::string(laiin_DHT_VERSION);
    
    // First, check to see if we have the key before performing any other operations
    if(has_key(key)) { return get(key); }
    
    if(has_key_cached(key)) {
        return get_cached(key); // Validate is slow so don't validate our cached hash table (for now)
    }
    
    // Second option is to check our providers to see if any holds the key we are looking for
    auto our_providers = get_providers(key);
    if(!our_providers.empty()) {
        std::cout << "Found " << our_providers.size() << " providers for key (" << key << ")\n";
        // Now contact each provider for the value to the key
        for(auto const& peer : our_providers) {
            // Construct the get query (request)
            std::string transaction_id = msgpack::generate_transaction_id();
            query_object["tid"] = transaction_id;
            std::vector<uint8_t> query_get = nlohmann::json::to_msgpack(query_object);
            
            // Send a get request to provider
            std::string peer_addr = (peer.address == this->public_ip_address) ? "127.0.0.1" : peer.address;
            std::cout << "Sending get request to \033[36m" << peer_addr << ":" << peer.port << "\033[0m\n";
            auto receive_buffer = send_query(peer_addr, peer.port, query_get, 2);
            
            // Process the response
            nlohmann::json response_get;
            try {
                response_get = nlohmann::json::from_msgpack(receive_buffer);
            } catch (const std::exception& e) {
                std::cerr << "Provider \033[91m" << peer_addr << ":" << peer.port << "\033[0m did not respond" << std::endl;
                remove_provider(key, peer.address, peer.port); // Remove this peer from providers
                continue; // Skip to next provider if this one is unresponsive
            }   
            
            // Handle the response
            std::cout << ((response_get.contains("error")) ? ("\033[91m") : ("\033[32m")) << response_get.dump() << "\033[0m\n";
            if(response_get.contains("error")) { // "Key not found"
                remove_provider(key, peer.address, peer.port); // Data is lost, remove peer from providers
                continue; 
            }
            if(response_get.contains("response") && response_get["response"].contains("value")) {
                auto value = response_get["response"]["value"].get<std::string>();
                if (validate(key, value)) { 
                    return value;
                }
            }
        }
    }
    
    // Third option is to send a get_providers request to the nodes in our routing table that are closest to the key
    // Then get the value for the given key from the received providers
    auto providers = send_get_providers(key);
    if(!providers.empty()) {
        std::cout << "Found " << providers.size() << " providers for key (" << key << ")\n";
        for(auto const& peer : providers) {
            // Construct the get query (request)
            std::string transaction_id = msgpack::generate_transaction_id();
            query_object["tid"] = transaction_id;
            std::vector<uint8_t> query_get = nlohmann::json::to_msgpack(query_object);
            
            // Send a get request to provider
            std::string peer_addr = (peer.address == this->public_ip_address) ? "127.0.0.1" : peer.address;
            std::cout << "Sending get request to \033[36m" << peer_addr << ":" << peer.port << "\033[0m\n";
            auto receive_buffer = send_query(peer_addr, peer.port, query_get, 2);
            
            // Process the response
            nlohmann::json response_get;
            try {
                response_get = nlohmann::json::from_msgpack(receive_buffer);
            } catch (const std::exception& e) {
                std::cerr << "Provider \033[91m" << peer_addr << ":" << peer.port << "\033[0m did not respond" << std::endl;
                remove_provider(key, peer.address, peer.port); // Remove this peer from providers
                continue; // Skip to next provider if this one is unresponsive
            }   
            
            // Handle the response
            std::cout << ((response_get.contains("error")) ? ("\033[91m") : ("\033[32m")) << response_get.dump() << "\033[0m\n";
            if(response_get.contains("error")) { // "Key not found"
                remove_provider(key, peer.address, peer.port); // Data is lost, remove peer from providers
                continue; 
            }
            if(response_get.contains("response") && response_get["response"].contains("value")) {
                auto value = response_get["response"]["value"].get<std::string>();
                if (validate(key, value)) { 
                    return value;
                }
            }
        }
    }
    
    return "";
}

std::string Node::send_find_value(const std::string& key) {
    return send_get(key);
}

void Node::send_remove(const std::string& key) {
    nlohmann::json query_object;
    query_object["query"] = "remove";
    query_object["args"]["key"] = key;
    query_object["version"] = std::string(laiin_DHT_VERSION);
    
    std::vector<Node *> closest_nodes = find_node(key, laiin_DHT_MAX_CLOSEST_NODES);
    
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(closest_nodes.begin(), closest_nodes.end(), rng);
    
    // Send remove query message to the closest nodes
    for(auto const& node : closest_nodes) {
        if (node == nullptr) continue;
        
        std::string transaction_id = msgpack::generate_transaction_id();
        query_object["tid"] = transaction_id; // tid should be unique for each query
        std::vector<uint8_t> remove_query = nlohmann::json::to_msgpack(query_object);
        // Send remove query message
        std::string node_ip = (node->get_ip_address() == this->public_ip_address) ? "127.0.0.1" : node->get_ip_address();
        int node_port = node->get_port();
        std::cout << "Sending remove request to \033[36m" << node_ip << ":" << node_port << "\033[0m\n";
        auto receive_buffer = send_query(node_ip, node_port, remove_query);
        // Process the response here
        nlohmann::json remove_response;
        try {
            remove_response = nlohmann::json::from_msgpack(receive_buffer);
        } catch (const std::exception& e) {
            std::cerr << "Node \033[91m" << node_ip << ":" << node_port << "\033[0m did not respond" << std::endl;
            node->check_counter += 1;
            continue; // Continue with the next closest node if this one fails
        }   
        // Show response
        std::cout << ((remove_response.contains("error")) ? ("\033[91m") : ("\033[32m")) << remove_response.dump() << "\033[0m\n";
    }
    
}

void Node::send_map(const std::string& address, int port) {
    nlohmann::json query_object;
    query_object["query"] = "map";
    query_object["args"]["id"] = this->id;
    query_object["args"]["port"] = get_port(); // the port of the peer that is announcing itself (map will also be used to "announce" the peer or provider)
    query_object["version"] = std::string(laiin_DHT_VERSION);
    
    bool map_sent = false;
    for (const auto& pair : data) {
        const std::string& key = pair.first;
        const std::string& value = pair.second;
        
        query_object["args"]["key"] = key;
        query_object["args"]["value"] = value;
        std::string transaction_id = msgpack::generate_transaction_id();
        query_object["tid"] = transaction_id; // tid should be unique for each map message
        std::vector<uint8_t> map_message = nlohmann::json::to_msgpack(query_object);

        auto receive_buffer = send_query(address, port, map_message);
        // Process the response here
        nlohmann::json map_response_message;
        try {
            map_response_message = nlohmann::json::from_msgpack(receive_buffer);
            map_sent = true;
        } catch (const std::exception& e) {
            std::cerr << "Node \033[91m" << address << ":" << port << "\033[0m did not respond to send_map" << std::endl;
        }
        // Show response
        std::cout << ((map_response_message.contains("error")) ? ("\033[91m") : ("\033[92m")) << map_response_message.dump() << "\033[0m\n";
    }
    
    if(map_sent && !data.empty()) { std::cout << "Sent map request to \033[36m" << address << ":" << port << "\033[0m\n"; }
}

void Node::send_map_v2(const std::string& address, int port) {
    if(is_hardcoded()) return; // Hardcoded nodes cannot run this function
    
    db::Sqlite3 * database = laiin::get_database();
    if(!database) throw std::runtime_error("database is not opened");
    if(!database->table_exists("hash_table")) {
        return; // Table does not exist, exit function
    }
    // Prepare statement
    std::string command = "SELECT key, value FROM hash_table;";
    sqlite3_stmt * stmt = nullptr;
    if(sqlite3_prepare_v2(database->get_handle(), command.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "\033[91msqlite3_prepare_v2: " + std::string(sqlite3_errmsg(database->get_handle())) << "\033[0m" << std::endl;
        return;
    }
    // Check if there is any data returned by the statement
    if(sqlite3_column_count(stmt) > 0) {
        std::cout << "\033[35;1mSending hash table map from cache ...\033[0m" << std::endl;
    }
    // Get all table values row by row
    std::unordered_map<std::string, std::string> hash_table;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        std::string key, value;
        for(int i = 0; i < sqlite3_column_count(stmt); i++) {
            if(i == 0) {
                key = (sqlite3_column_text(stmt, i) == nullptr) ? "" : reinterpret_cast<const char *>(sqlite3_column_text(stmt, i));
            }
            if(i == 1) {
                value = (sqlite3_column_text(stmt, i) == nullptr) ? "" : reinterpret_cast<const char *>(sqlite3_column_text(stmt, i));
            }
            
            if(key.empty() || value.empty()) { continue; }
            
            hash_table[key] = value;
        }
    }
    // Finalize statement
    sqlite3_finalize(stmt);
    -
    nlohmann::json query_object;
    query_object["query"] = "map";
    query_object["args"]["id"] = this->id;
    query_object["args"]["port"] = get_port();
    query_object["version"] = std::string(laiin_DHT_VERSION);
    
    for (const auto& [key, value] : hash_table) {
        query_object["args"]["key"] = key;
        query_object["args"]["value"] = value;
        std::string transaction_id = msgpack::generate_transaction_id();
        query_object["tid"] = transaction_id;
        std::vector<uint8_t> map_request = nlohmann::json::to_msgpack(query_object);

        auto receive_buffer = send_query(address, port, map_request);
        
        nlohmann::json map_response;
        try {
            map_response = nlohmann::json::from_msgpack(receive_buffer);
            std::cout << ((map_response.contains("error")) ? ("\033[91m") : ("\033[92m")) << map_response.dump() << "\033[0m\n";
        } catch (const std::exception& e) {
            std::cerr << "\033[91mNode " << address << ":" << port << " did not respond to send_map\033[0m" << std::endl;
        }
    }
}

std::deque<Peer> Node::send_get_providers(const std::string& key) {
    std::deque<Peer> peers = {};
    std::set<std::pair<std::string, uint16_t>> unique_peers; // Set to store unique IP-port pairs
    
    nlohmann::json query_object;
    query_object["query"] = "get_providers";
    query_object["args"]["id"] = this->id;
    query_object["args"]["key"] = key;
    query_object["version"] = std::string(laiin_DHT_VERSION);
    
    std::vector<Node *> closest_nodes = find_node(key, laiin_DHT_MAX_CLOSEST_NODES);
    
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(closest_nodes.begin(), closest_nodes.end(), rng);
    
    for (auto const& node : closest_nodes) {
        if (node == nullptr) continue;
        if (node->get_status() != NodeStatus::Active) continue;
        // Construct message with a unique tid
        std::string transaction_id = msgpack::generate_transaction_id();
        query_object["tid"] = transaction_id;
        auto get_providers_query = nlohmann::json::to_msgpack(query_object);
        // Send get_providers query message to each node
        std::string node_ip = (node->get_ip_address() == this->public_ip_address) ? "127.0.0.1" : node->get_ip_address();
        int node_port = node->get_port();
        std::cout << "Sending get_providers request to \033[36m" << node_ip << ":" << node_port << "\033[0m\n";
        auto receive_buffer = send_query(node_ip, node_port, get_providers_query);
        // Process the response here
        nlohmann::json get_providers_response;
        try {
            get_providers_response = nlohmann::json::from_msgpack(receive_buffer);
        } catch (const std::exception& e) {
            std::cerr << "Node \033[91m" << node_ip << ":" << node_port << "\033[0m did not respond" << std::endl;
            node->check_counter += 1;
            continue; // Continue with the next node if this one fails
        }
        // Show response and handle the retrieved value
        std::cout << ((get_providers_response.contains("error")) ? ("\033[91m") : ("\033[32m")) << get_providers_response.dump() << "\033[0m\n";
        if(get_providers_response.contains("error")) {
            continue; // Skip if error
        }
        if (get_providers_response.contains("response") && get_providers_response["response"].contains("values")) {
            for (auto& values_json : get_providers_response["response"]["values"]) {
                if (values_json.contains("ip_address") && values_json.contains("port")) {
                    std::string ip_address = values_json["ip_address"];
                    uint16_t port = values_json["port"];
                    // Check if the IP-port pair is already added
                    if (unique_peers.insert({ip_address, port}).second) {
                        Peer provider = Peer{ip_address, port};
                        peers.push_back(provider);
                    }
                }
            }
        }
    }    
    
    return peers;
}



void Node::refresh() {
    std::vector<Node *> closest_nodes = find_node(this->id, laiin_DHT_MAX_CLOSEST_NODES);
    
    for(const auto& neighbor : closest_nodes) {
        std::cout << "Sending find_node request to " << neighbor->get_ip_address() << ":" << neighbor->get_port() << "\n";
        auto nodes = send_find_node(this->id, (neighbor->get_ip_address() == this->public_ip_address) ? "127.0.0.1" : neighbor->get_ip_address(), neighbor->get_port());
        if(nodes.empty()) {
            std::cerr << "find_node: No unique nodes found\n"; continue;
        }
        
        // Then add received nodes to the routing table
        for (auto& node : nodes) {
            // Ping the received nodes first
            std::string node_addr = (node->get_ip_address() == this->public_ip_address) ? "127.0.0.1" : node->get_ip_address();
            if(!ping(node_addr, node->get_port())) {
                continue; // Skip the node and continue with the next iteration
            }
            // Update the routing table with the received node
            routing_table->add_node(std::move(node));
        }
    }
    
    routing_table->print_table();
}



void Node::republish() {
    for (const auto& [key, value] : data) {
        send_put(key, value);
    }
    
    if(!data.empty()) { std::cout << "\033[93mData republished\033[0m\n"; }
}



bool Node::validate(const std::string& key, const std::string& value) {
    if(key.length() != 64) {
        std::cerr << "\033[91mInvalid key length\033[0m\n";
        return false;
    }
    
    // Ensure that the value is valid JSON
    if(value.empty()) { return false; }
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(value);
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "\033[91mJSON parsing error: " << e.what() << "\033[0m" << std::endl;
        return false;
    }
    
    // Make sure value contains a valid metadata field
    if(!json.is_object()) { return false; }
    if(!json.contains("metadata")) { return false; }
    if(!json["metadata"].is_string()) { return false; }
    std::string metadata = json["metadata"].get<std::string>();
    std::vector<std::string> valid_metadatas = { "user","listing","product_rating","seller_rating","order","message", };
    if (std::find(valid_metadatas.begin(), valid_metadatas.end(), metadata) == valid_metadatas.end()) {
        std::cerr << "\033[91mInvalid metadata field: " << metadata << "\033[0m\n";
        return false;
    }
    
    if(!validate_fields(value)) {
        return false;
    }
    
    // Verify the value using the signature field
    if(!verify(value)) {
        return false;
    }
    
    // Reject expired data and remove if previously stored
    db::Sqlite3 * database = laiin::get_database();
    if(json.contains("expiration_date")) {
        if(!json["expiration_date"].is_string()) { return false; }
        std::string expiration_date = json["expiration_date"].get<std::string>();
        if(laiin::timestamp::is_expired(expiration_date)) {
            std::cerr << "\033[91mData has expired\033[0m\n";
            if(has_key(key)) {
                remove(key);
            }
            if(database->get_integer_params("SELECT EXISTS(SELECT key FROM hash_table WHERE key = ?1)", { key }) == 1) {
                database->execute_params("DELETE FROM hash_table WHERE key = ?1", { key });
            }
            if(database->get_integer_params("SELECT EXISTS(SELECT key FROM mappings WHERE key = ?1)", { key }) == 1) {
                database->execute_params("DELETE FROM mappings WHERE key = ?1", { key });
            }
            return false;
        }
    }
    
    return true;
}

bool Node::validate_fields(const std::string& value) {
    nlohmann::json json = nlohmann::json::parse(value);
    std::string metadata = json["metadata"].get<std::string>();
    
    if(metadata == "user") {
        if (!json.contains("created_at") && !json["created_at"].is_string()) { return false; }
        if (!json.contains("monero_address") && !json["monero_address"].is_string()) { return false; }
        if (!json.contains("public_key") && !json["public_key"].is_string()) { return false; }
        if (!json.contains("signature") && !json["signature"].is_string()) { return false; }
        // Optional fields
        if (json.contains("avatar")) { 
            if(!json["avatar"].is_object()) { return false; }
            const auto& avatar = json["avatar"];
            if(!avatar.contains("name") && !avatar["name"].is_string()) { return false; }
            if(!avatar.contains("size") && !avatar["size"].is_number_integer()) { return false; }
            if(!avatar.contains("pieces") && !avatar["pieces"].is_array()) { return false; }
            if(!avatar.contains("piece_size") && !avatar["piece_size"].is_number_integer()) { return false; }
        }
        if (json.contains("display_name")) { 
            if(!json["display_name"].is_string()) { return false; }
            std::string display_name = json["display_name"].get<std::string>();
            if(!laiin::string_tools::is_valid_username(display_name)) {
                return false;
            }
            if((display_name.length() < laiin_MIN_USERNAME_LENGTH) ||
                (display_name.length() > laiin_MAX_USERNAME_LENGTH)) {
                return false;
            }
        }
    }
    
    return true;
}

bool Node::verify(const std::string& value) const {
    nlohmann::json json = nlohmann::json::parse(value);

    // Get required fields from the value
    std::string metadata = json["metadata"].get<std::string>();
    std::string signed_message, signing_address, signature;
    // Messages and orders don't need to be verified by peers since they will be encrypted then decrypted and verified by the intended recipient instead
    if(metadata == "order") { return true; }
    if(metadata == "message") { return true; } // Signing address is encrypted
    if(metadata == "user") {
        if(!json["monero_address"].is_string()) { return false; }
        signed_message = json["monero_address"].get<std::string>(); // user_id
        signing_address = signed_message; // the monero_address (monero primary address) is both the signed message and the signing address
    }
    if(metadata == "listing") {
        if(!json["id"].is_string()) { return false; }
        signed_message = json["id"].get<std::string>(); // the id (uuid) is the signed message
        if(!json["seller_id"].is_string()) { return false; }
        signing_address = json["seller_id"].get<std::string>(); // the seller_id (monero primary address) is the signing address      
    }
    if(metadata == "product_rating" || metadata == "seller_rating") {
        if(!json["comments"].is_string()) { return false; }
        signed_message = json["comments"].get<std::string>(); // the comments is the signed message
        if(!json["rater_id"].is_string()) { return false; }
        signing_address = json["rater_id"].get<std::string>(); // the rater_id (monero primary address) is the signing address
    }
    
    // Get signature field
    if (json.contains("signature")) {
        if(!json["signature"].is_string()) { return false; }
        signature = json["signature"].get<std::string>(); // the signature may have been updated
    }
    
    // Validate signing address and signature
    auto network_type = monero_network_type::STAGENET;
    if(!monero_utils::is_valid_address(signing_address, network_type)) {
        std::cerr << "\033[91mInvalid signing address\033[0m\n";
        return false;
    }
    if(signature.length() != 93 || !laiin::string::contains_first_of(signature, "Sig")) { 
        std::cerr << "\033[91mInvalid signature\033[0m\n";
        return false;
    }
    
    // Verify the signed message
    monero::monero_wallet_config wallet_config_obj;
    wallet_config_obj.m_path = "";
    wallet_config_obj.m_password = "";
    wallet_config_obj.m_network_type = network_type;
    std::unique_ptr<monero::monero_wallet_full> monero_wallet_obj = std::unique_ptr<monero_wallet_full>(monero_wallet_full::create_wallet (wallet_config_obj, nullptr));
    bool verified = monero_wallet_obj->verify_message(signed_message, signing_address, signature).m_is_good;
    if(!verified) {
        std::cerr << "\033[91mData verification failed\033[0m" << std::endl;
        monero_wallet_obj->close(false);
        monero_wallet_obj.reset();
        return false;
    }
    monero_wallet_obj->close(false);
    monero_wallet_obj.reset();
    
    return true;
}

void Node::expire(const std::string& key, const std::string& value) {
    db::Sqlite3 * database = laiin::get_database();
    
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(value);
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return; // Invalid value, exit function
    }
    
    if(json.contains("expiration_date")) {
        if(!json["expiration_date"].is_string()) { 
            if(remove(key) == true) {
                int error = database->execute_params("DELETE FROM mappings WHERE key = ?1", { key });
                error = database->execute_params("DELETE FROM hash_table WHERE key = ?1", { key });
            }
            return; // Invalid expiration_date, exit function
        }
        std::string expiration_date = json["expiration_date"].get<std::string>();
        if(laiin_timestamp::is_expired(expiration_date)) {
            // Remove the data from hash table if it was previously stored
            if(has_key(key)) {
                std::cout << "Data with key (" << key << ") has expired. Removing from hash table ...\n";
                if(remove(key) == true) {
                    int error = database->execute_params("DELETE FROM mappings WHERE key = ?1", { key });
                    error = database->execute_params("DELETE FROM hash_table WHERE key = ?1", { key });
                }
            }
        }
    }
}

int Node::cache(const std::string& key, const std::string& value) {
    db::Sqlite3 * database = laiin::get_database();
    int rescode = SQLITE_OK;
    
    if(!database->table_exists("hash_table")) { 
        rescode = database->execute("CREATE TABLE hash_table("
        "key TEXT, value TEXT, UNIQUE(key));");
        if(rescode != SQLITE_OK) { throw std::runtime_error("Error creating on-disk hash table"); }
        
        rescode = database->execute("CREATE INDEX idx_hash_table_keys ON hash_table(key)");
        if(rescode != SQLITE_OK) { std::cerr << "\033[0;91mError creating index for on-disk hash table\033[0m" << std::endl; }
    }
    
    bool key_found = database->get_integer_params("SELECT EXISTS(SELECT key FROM hash_table WHERE key = ?1)", { key });
    if(key_found) {
        rescode = database->execute_params("UPDATE hash_table SET value = ?1 WHERE key = ?2", { value, key });
        return (rescode == SQLITE_OK);
    }
    
    rescode = database->execute_params("INSERT INTO hash_table (key, value) VALUES (?1, ?2);", { key, value });
    return (rescode == SQLITE_OK);
}


void Node::periodic_purge() {
    while (true) {
        {
            // Acquire the lock before accessing the data
            std::shared_lock<std::shared_mutex> read_lock(node_read_mutex);
    
            if(!data.empty()) { std::cout << "\033[34;1mPerforming periodic data removal\033[0m\n"; }
            
            for (const auto& [key, value] : data) {
                ////expire(key, value);
            }
            
            // read_lock is released here
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(laiin_DHT_DATA_REMOVAL_INTERVAL));
    }
}

void Node::periodic_refresh() {
    while (true) {
        {
            // Acquire the lock before accessing the data
            std::shared_lock<std::shared_mutex> read_lock(node_read_mutex);
            
            
            if(routing_table->get_node_count() > 0) { std::cout << "\033[34;1mPerforming periodic bucket refresh\033[0m\n"; }
            
            refresh();
            
            // read_lock is released here
        }
        
        // Sleep for a specified interval
        std::this_thread::sleep_for(std::chrono::seconds(laiin_DHT_BUCKET_REFRESH_INTERVAL));
    }
}

void Node::periodic_republish() {
    while (true) {
        {
            // Acquire the lock before accessing the data
            std::shared_lock<std::shared_mutex> read_lock(node_read_mutex);
            
            // Perform periodic republishing here
            // This code will run concurrently with the listen/receive loop
            if(!data.empty()) { std::cout << "\033[34;1mPerforming periodic data propagation\033[0m\n"; }
            
            republish();
            
            // read_lock is released here
        }
        
        // Sleep for a specified interval
        std::this_thread::sleep_for(std::chrono::seconds(laiin_DHT_DATA_REPUBLISH_INTERVAL));
    }
}

void Node::periodic_check() {
    ////std::vector<std::string> dead_node_ids {};
    while(true) {
        {
        // Acquire the lock before accessing the routing table
        std::shared_lock<std::shared_mutex> read_lock(node_read_mutex);
        // Perform periodic checks here
        // This code will run concurrently with the listen/receive loop
        for (auto& bucket : routing_table->buckets) {
            for (auto& node : bucket.second) {
                if (node.get() == nullptr) continue; // It's possible that the invalid node object is being accessed or modified by another thread concurrently, even though its already been removed from the routing_table
                std::string node_ip = (node->get_ip_address() == this->public_ip_address) ? "127.0.0.1" : node->get_ip_address();
                uint16_t node_port = node->get_port();
                
                // Skip the bootstrap nodes from the periodic checks
                if (node->is_hardcoded()) continue;
                
                std::cout << "Performing periodic node health check on \033[34m" << node_ip << ":" << node_port << "\033[0m\n";
                
                // Perform the liveness check on the current node
                bool pinged = ping(node_ip, node_port);
                
                // Update the liveness status of the node in the routing table
                node->check_counter = pinged ? 0 : (node->check_counter + 1);
                std::cout << "Health check failures: " << node->check_counter << (" (" + node->get_status_as_string() + ")") << "\n";
                
                // If node is dead, remove it from the routing table
                if(node->is_dead()) {
                    std::cout << "\033[0;91m" << node->public_ip_address << ":" << node_port << "\033[0m marked as dead\n";
                    if(routing_table->has_node(node->public_ip_address, node_port)) {
                        ////dead_node_ids.push_back(node->get_id());
                        routing_table->remove_node(node->public_ip_address, node_port); // Already has internal write_lock
                    }
                }
            }
        }
            // read_lock is released here
        }
        
        on_dead_node(dead_node_ids);
        // Clear the vector for the next iteration
        dead_node_ids.clear();
        // Sleep for a specified interval
        std::this_thread::sleep_for(std::chrono::seconds(laiin_DHT_NODE_HEALTH_CHECK_INTERVAL));
    }
}



bool Node::on_keyword_blocked(const nlohmann::json& value) {
    // Note: This code is in the testing stage
    // Block certain keywords/search terms from listings
    if(!json.contains("metadata")) { return false; }
    if(json["metadata"] == "listing") {
        // Block categories marked as "Illegal"
        if(json["product"]["category"] == "Illegal" ||
            json["product"]["subcategory"] == "Illegal") {
            std::cout << "Illegal product blocked\n";
            return true; // The category is blocked, do not insert the data
        }
        // Block certain tags
        std::vector<std::string> blocked_tags = { "heroin", "meth", "cp", "child porn" };
        
        if(json["product"].contains("tags")) {
            if(!json["product"]["tags"].is_array()) { return false; }
            std::vector<std::string> product_tags = json["product"]["tags"].get<std::vector<std::string>>();
            // Check if any of the product tags match the blocked tags
            bool has_blocked_tag = std::any_of(product_tags.begin(), product_tags.end(), [&](const std::string& tag) {
                return std::find(blocked_tags.begin(), blocked_tags.end(), tag) != blocked_tags.end();
            });
            // Print the result
            if (has_blocked_tag) {
                std::cout << "Product contains a blocked tag." << std::endl;
                return true;
            } else {
                std::cout << "Product does not contain any blocked tags." << std::endl;
            }
        }
        // Block other search terms
    }
    return false;
}



void Node::on_ping(const std::vector<uint8_t>& buffer, const struct sockaddr_in& client_addr) {
    if (buffer.size() > 0) {
        nlohmann::json message = nlohmann::json::from_msgpack(buffer);
        if (message.contains("query") && message["query"] == "ping") {
            std::string sender_id = message["args"]["id"].get<std::string>();
            std::string sender_ip = inet_ntoa(client_addr.sin_addr);
            uint16_t sender_port = (message["args"].contains("port")) ? (uint16_t)message["args"]["port"] : laiin_P2P_DEFAULT_PORT;
            
            // Validate node id
            std::string calculated_node_id = generate_node_id((sender_ip == "127.0.0.1") ? this->public_ip_address : sender_ip, sender_port);
            if(sender_id == calculated_node_id) {
                bool has_node = routing_table->has_node((sender_ip == "127.0.0.1") ? this->public_ip_address : sender_ip, sender_port);
                if (!has_node) {
                    auto node_that_pinged = std::make_unique<Node>((sender_ip == "127.0.0.1") ? this->public_ip_address : sender_ip, sender_port, false);
                    if(!node_that_pinged->is_hardcoded()) { // To prevent the seed node from being stored in the routing table
                        routing_table->add_node(std::move(node_that_pinged)); // Already has internal write_lock
                        persist_routing_table((sender_ip == "127.0.0.1") ? this->public_ip_address : sender_ip, sender_port);
                        routing_table->print_table();
                    
                        // Announce your node as a data provider to the new node that recently joined the network to make product/service listings more easily discoverable by the new node
                        send_map_v2((sender_ip == this->public_ip_address) ? "127.0.0.1" : sender_ip, sender_port);
                    }
                }
            }
        }
    }
}

void Node::on_map(const std::vector<uint8_t>& buffer, const struct sockaddr_in& client_addr) {
    if (buffer.size() > 0) {
        nlohmann::json message = nlohmann::json::from_msgpack(buffer);
        if (message.contains("query") && message["query"] == "map") {
            std::string sender_id = message["args"]["id"].get<std::string>();
            std::string sender_ip = inet_ntoa(client_addr.sin_addr);
            uint16_t sender_port = (message["args"].contains("port")) ? (uint16_t)message["args"]["port"] : laiin_P2P_DEFAULT_PORT;
            std::string key = message["args"]["key"].get<std::string>();
            
            // Validate node id
            std::string calculated_node_id = generate_node_id((sender_ip == "127.0.0.1") ? this->public_ip_address : sender_ip, sender_port);
            if(sender_id == calculated_node_id) {
                // Save this peer as the provider of this key
                add_provider(key, Peer{ (sender_ip == "127.0.0.1") ? this->public_ip_address : sender_ip, sender_port });
            }
        }
    }
}

void Node::run() {
    
    run_optimized();
    return;
    
    // Start a separate thread for periodic checks and republishing
    std::thread periodic_check_thread([this]() { periodic_check(); });
    std::thread periodic_republish_thread([this]() { periodic_republish(); });
    std::thread periodic_purge_thread([this]() { periodic_purge(); });
    std::thread periodic_refresh_thread([this]() { periodic_refresh(); });
    
    while (true) {
        std::vector<uint8_t> buffer(laiin_RECV_BUFFER_SIZE);
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int bytes_received = recvfrom(sockfd, buffer.data(), buffer.size(), 0,
                                      (struct sockaddr*)&client_addr, &client_addr_len);
        if (bytes_received == -1 && errno == EAGAIN) {
            // No data available, continue loop
            continue;
        }
        else if (bytes_received < 0) {
            perror("recvfrom");
        }

        // Resize the buffer to the actual number of received bytes
        buffer.resize(bytes_received);

        if (buffer.size() > 0) std::cout << "Received request from \033[0;36m" << inet_ntoa(client_addr.sin_addr) << "\033[0m\n";
        
        // Create a lambda function to handle the request
        auto handle_request_fn = [=]() {
            // Acquire the lock before accessing the routing table
            std::shared_lock<std::shared_mutex> read_lock(node_read_mutex);
            // Process the message
            std::vector<uint8_t> response = laiin::msgpack::process(buffer, *this, false);

            // Send the response
            int bytes_sent = sendto(sockfd, response.data(), response.size(), 0,
                                (struct sockaddr*)&client_addr, client_addr_len);
            if (bytes_sent < 0) {
                perror("sendto");
            }
        
            // Add the node that pinged this node to the routing table
            on_ping(buffer, client_addr);
            
            // Add provider
            on_map(buffer, client_addr);
        };
        
        // Create a detached thread to handle the request
        std::thread request_thread(handle_request_fn);
        request_thread.detach();
    }
    // Wait for the periodic threads to finish
    periodic_check_thread.join();
    periodic_republish_thread.join();
    periodic_purge_thread.join();
    periodic_refresh_thread.join();
}

// This uses less CPU
void Node::run_optimized() {
    // Start a separate thread for periodic checks and republishing
    std::thread periodic_check_thread([this]() { periodic_check(); });
    std::thread periodic_republish_thread([this]() { periodic_republish(); });
    std::thread periodic_purge_thread([this]() { periodic_purge(); });
    std::thread periodic_refresh_thread([this]() { periodic_refresh(); });

    while (true) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(sockfd, &read_set);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  // Timeout of 100ms

        int ready = select(sockfd + 1, &read_set, nullptr, nullptr, &timeout);
        if (ready == -1) {
            perror("select");
            // Handle the error
            // ...
            break;
        } else if (ready == 0) {
            // No data available within the timeout period, continue the loop
            continue;
        }

        if (FD_ISSET(sockfd, &read_set)) {
            std::vector<uint8_t> buffer(laiin_RECV_BUFFER_SIZE);
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int bytes_received = recvfrom(sockfd, buffer.data(), buffer.size(), MSG_DONTWAIT,
                                          (struct sockaddr*)&client_addr, &client_addr_len);
            if (bytes_received == -1 && errno == EAGAIN) {
                // No data available, continue loop
                continue;
            }
            else if (bytes_received < 0) {
                perror("recvfrom");
            }
            
            if (bytes_received > 0) {
                // Resize the buffer to the actual number of received bytes
                buffer.resize(bytes_received);

                if (buffer.size() > 0) std::cout << "Received request from \033[0;36m" << inet_ntoa(client_addr.sin_addr) << "\033[0m\n";
                
                // Create a lambda function to handle the request
                auto handle_request_fn = [=]() {
                    // Acquire the lock before accessing the routing table
                    std::shared_lock<std::shared_mutex> read_lock(node_read_mutex);
                    // Process the message
                    std::vector<uint8_t> response = laiin::msgpack::process(buffer, *this, false);

                    // Send the response
                    int bytes_sent = sendto(sockfd, response.data(), response.size(), 0,
                                    (struct sockaddr*)&client_addr, client_addr_len);
                    if (bytes_sent < 0) {
                        perror("sendto");
                    }
                
                    // Add the node that pinged this node to the routing table
                    on_ping(buffer, client_addr);
                    
                    // Add provider
                    on_map(buffer, client_addr);
                };
                
                // Create a detached thread to handle the request
                std::thread request_thread(handle_request_fn);
                request_thread.detach();
            }
        }
    }         
    // Wait for the periodic threads to finish
    periodic_check_thread.join();    
    periodic_republish_thread.join();
    periodic_purge_thread.join();
    periodic_refresh_thread.join();
}

std::string Node::get_id() const {
    return id;
}

std::string Node::get_ip_address() const {
    const int ADDRSTRLEN = (storage.ss_family == AF_INET6) ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;
    char ip_address[ADDRSTRLEN] = {0};

    if(storage.ss_family == AF_INET) {
        strcpy(ip_address, inet_ntoa(sockin.sin_addr));//std::string ip_address = inet_ntoa(sockin.sin_addr);
    } 
    if(storage.ss_family == AF_INET6) {
        inet_ntop(storage.ss_family, &(sockin6.sin6_addr), ip_address, ADDRSTRLEN);
    }

    return std::string(ip_address);
}

std::string Node::get_local_ip_address() const {
    return get_ip_address();
}

std::string Node::get_device_ip_address() const {
    std::future<std::string> result = std::async(std::launch::async, laiin::get_device_ip_address);
    return result.get();
}

std::string Node::get_public_ip_address() const {
    std::future<std::string> result = std::async(std::launch::async, laiin::get_public_ip_address);
    return result.get();
}

uint16_t Node::get_port() const {
    uint16_t port = 0;
    
    if(storage.ss_family == AF_INET) {
        port = ntohs(sockin.sin_port);
    } 
    if(storage.ss_family == AF_INET6) {
        port = ntohs(sockin6.sin6_port);
    }
    if(port == 0) {
        // Get the actual port number used by the socket
        struct sockaddr_in local_addr;
        socklen_t local_addr_len = sizeof(local_addr);
        getsockname(sockfd, (struct sockaddr*)&local_addr, &local_addr_len);
        port = ntohs(local_addr.sin_port);
    }    
    return port;
}

Server * Node::get_server() const {
    return server.get();
}

RoutingTable * Node::get_routing_table() const {
    return routing_table.get();
}

std::vector<Peer> Node::get_peers() const {
    std::vector<Peer> peers_list;
    for (auto& bucket : routing_table->buckets) {
        for (auto& node : bucket.second) {
            if (node.get() == nullptr) continue;
            Peer peer;
            peer.address = node->get_ip_address();
            peer.port = node->get_port();
            peer.id = node->get_id();
            peer.status = node->get_status();
            //peer.distance = node->get_distance(this->id);
            peers_list.push_back(peer);
        }
    }
    return peers_list;
}

int Node::get_peer_count() const {
    return routing_table->get_node_count();
}

int Node::get_active_peer_count() const {
    int active_count = 0;
    for (auto& bucket : routing_table->buckets) {
        for (auto& node : bucket.second) {
            if (node.get() == nullptr) continue;
            if (node->get_status() == NodeStatus::Active) {
                active_count++;
            }
        }
    }
    return active_count;
}

int Node::get_idle_peer_count() const {
    int idle_count = 0;
    for (auto& bucket : routing_table->buckets) {
        for (auto& node : bucket.second) {
            if (node.get() == nullptr) continue;
            if (node->get_status() == NodeStatus::Inactive) {
                idle_count++;
            }
        }
    }
    return idle_count;
}

NodeStatus Node::get_status() const {
    if(check_counter == 0) return NodeStatus::Active;
    if(check_counter <= (laiin_DHT_MAX_HEALTH_CHECKS - 1)) return NodeStatus::Inactive;
    if(check_counter >= laiin_DHT_MAX_HEALTH_CHECKS) return NodeStatus::Dead;
    return NodeStatus::Inactive;
}

std::string Node::get_status_as_string() const {
    if(check_counter == 0) return "Active";
    if(check_counter <= (laiin_DHT_MAX_HEALTH_CHECKS - 1)) return "Inactive";
    if(check_counter >= laiin_DHT_MAX_HEALTH_CHECKS) return "Dead";
    return "Unknown";
}

std::vector<std::string> Node::get_keys() const {
    std::vector<std::string> keys;

    for (const auto& pair : data) {
        keys.push_back(pair.first);
    }

    return keys;
}

std::vector<std::pair<std::string, std::string>> Node::get_data() const {
    std::vector<std::pair<std::string, std::string>> data_vector;

    for (const auto& pair : data) {
        data_vector.push_back(pair);
    }

    return data_vector;
}

const std::unordered_map<std::string, std::string>& Node::get_data() const {
    return data;
}

int Node::get_data_count() const {
    return data.size();
}

int Node::get_data_ram_usage() const {
    size_t total_size = sizeof(data);
    
    for(const auto& [key, value] : data) {
        total_size += sizeof(std::pair<const std::string, std::string>);
        total_size += key.capacity();
        total_size += value.capacity();
    }

    // Note: this does not account for internal hash table overhead, which can vary by implementation
    return total_size;
}

std::string Node::get_cached(const std::string& key) {
    db::Sqlite3 * database = laiin::get_database();
    if(!database) { throw std::runtime_error("database is not opened"); }
    if(!database->table_exists("hash_table")) { return ""; }
    
    return database->get_text_params("SELECT value FROM hash_table WHERE key = ?1 LIMIT 1", { key });
}

bool Node::has_key(const std::string& key) const {
    return (data.count(key) > 0);
}

bool Node::has_key_cached(const std::string& key) const {
    db::Sqlite3 * database = laiin::get_database();
    if(!database) { throw std::runtime_error("database is not opened"); }
    if(!database->table_exists("hash_table")) { return false; }
    
    return database->get_integer_params("SELECT EXISTS(SELECT key FROM hash_table WHERE key = ?1)", { key });
}

bool Node::has_value(const std::string& value) const {
    for (const auto& pair : data) {
        if (pair.second == value) {
            return true;
        }
    }
    return false;
}

bool Node::is_hardcoded(const std::string& address, uint16_t port) {
    for (const auto& bootstrap_node : BOOTSTRAP_NODES) {
        if (laiin::ip::resolve(bootstrap_node.first) == address && bootstrap_node.second == port) {
            return true;
        }
    }
    return false;
}

bool Node::is_hardcoded() const {
    for (const auto& bootstrap_node : BOOTSTRAP_NODES) {
        auto bootstrap_node_ip = laiin::ip::resolve(bootstrap_node.first);
        if (bootstrap_node_ip == this->public_ip_address
        && bootstrap_node.second == this->get_port()) {
            return true;
        }
        
        if((bootstrap_node_ip == "127.0.0.1" || bootstrap_node_ip == "0.0.0.0")
        && bootstrap_node.second == laiin_P2P_DEFAULT_PORT
        && bootstrap_node.second == this->get_port()) { // For testing on localhost. Remove this soon!!
            return true;
        }
    }
    return false;
}

bool Node::is_bootstrap_node() const {
    return (bootstrap == true) || is_hardcoded();
}

bool Node::is_dead() const {
    return (check_counter >= laiin_DHT_MAX_HEALTH_CHECKS);
}

bool Node::is_value_publishable(const std::string& value) {
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(value);
    } catch (const nlohmann::json::parse_error& e) {
        return false; // Invalid value, return false
    }

    if(!json.is_object()) { return false; }
    if(!json.contains("metadata")) { return false; }
    if(!json["metadata"].is_string()) { return false; }
    std::string metadata = json["metadata"].get<std::string>();
    
    std::vector<std::string> non_publishable_metadatas = { "listing" };
    return (std::find(non_publishable_metadatas.begin(), non_publishable_metadatas.end(), metadata) == non_publishable_metadatas.end());
}

void Node::set_bootstrap(bool bootstrap) {
    this->bootstrap = bootstrap;
}

}
