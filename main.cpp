#include "server.h"
#include "client.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <client|server> [address] [port]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "server") {
        int port = 8080; // Varsayılan port
        if (argc >= 3) {
            port = std::stoi(argv[2]);
        }
        WebSocketServer server(port);
        if (server.start()) {
            std::cout << "Server is running on port " << port << ". Press Enter to stop..." << std::endl;
            std::cin.get();
            server.stop();
        } else {
            std::cerr << "Failed to start the server." << std::endl;
            return 1;
        }
    } else if (mode == "client") {
        std::string address = "localhost"; // Varsayılan adres
        int port = 8080; // Varsayılan port
        if (argc >= 4) {
            address = argv[2];
            port = std::stoi(argv[3]);
        }
        WebSocketClient client(address, port);
        if (client.connect()) {
            std::cout << "Client is connected to the server." << std::endl;
        } else {
            std::cerr << "Failed to connect the client to the server." << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Invalid mode. Use 'client' or 'server'." << std::endl;
        return 1;
    }

    return 0;
}
