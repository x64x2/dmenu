
#include <iostream>
// laiin
#include "../src/laiin.hpp"
using namespace laiin;


Process * server_process;

void launch_neromon() {
    // on launching laiin, start the neromon process, if it has not yet been started    
    int neromon = Process::get_process_by_name("neromon");
    if(neromon != -1) {
        laiin::print("neromon is already running in the background", 4);
        return;
    }
    server_process = new Process(); // don't forget to delete this!
    server_process->create("./neromon", "");
    // show all processes
    Process::show_processes();
}

void wait_neromon() {
    ::sleep(2);
}

int main() {
    // laiinmon
    launch_neromon();
    // wait for neromon to finish launching
    wait_neromon();
    // connect client (laiin) to neromon
    Client * client = Client::get_main_client();
	int client_port = 1234;
	std::string client_ip = "0.0.0.0";//"localhost";//0.0.0.0 means anyone can connect to your server
	if(!client->connect(client_port, client_ip)) {
	    // free process
	    delete server_process; // kills process
	    server_process = nullptr;
	    // exit application
	    exit(0);
	} else std::cout << client->read() << std::endl; // read from server once
	// on browser, go to: https://127.0.0.1:1234/  
	while(1) {}
    return 0;
}
