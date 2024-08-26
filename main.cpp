#include <iostream>
#include <string>
#include "server.h"
#include "client.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [server|client]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    if (mode == "server") {
        WebSocketServer server(8080); // Port 8080
        if (server.start()) {
            std::cout << "Server is running on port 8080. Press Enter to stop..." << std::endl;
            std::cin.get();  // Wait for user input to stop the server
            server.stop();
        } else {
            std::cerr << "Failed to start the server." << std::endl;
            return 1;
        }
    } else if (mode == "client") {
        WebSocketClient client("localhost", 8080); // Server address and port
        if (client.connect()) {
            std::cout << "Client is connected to the server." << std::endl;
        } else {
            std::cerr << "Failed to connect the client to the server." << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Invalid mode. Use 'server' or 'client'." << std::endl;
        return 1;
    }

    return 0;
}
