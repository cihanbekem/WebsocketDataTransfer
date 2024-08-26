/*

#include "client.h"
#include "server.h"
#include <iostream>
#include <string>
#include <cstdlib>

void compileAndRun(const std::string& mode) {
    std::string command;

    if (mode == "server") {
        command = "g++ -o server server.cpp student.pb.cc -lprotobuf -lwebsockets -lpthread -std=c++17";
    } else if (mode == "client") {
        command = "g++ -o client client.cpp student.pb.cc -lprotobuf -lwebsockets -lpthread -std=c++17";
    } else {
        std::cerr << "Invalid mode specified." << std::endl;
        exit(1);
    }

    // Compile the program
    if (system(command.c_str()) == 0) {
        // Run the compiled program
        std::string runCommand = "./" + mode;
        system(runCommand.c_str());
    } else {
        std::cerr << "Compilation failed." << std::endl;
        exit(1);
    }
}

int main() {
    std::string mode;
    std::cout << "Enter mode (client or server): ";
    std::getline(std::cin, mode);

    compileAndRun(mode);

    return 0;
}
*/

#include "client.h"
#include "server.h"
#include <iostream>
#include <string>

void runClient(const std::string& address, int port) {
    WebSocketClient client(address, port);
    if (client.connect()) {
        std::cout << "Client connected to " << address << ":" << port << std::endl;
    } else {
        std::cerr << "Failed to connect the client." << std::endl;
    }
}

void runServer(int port) {
    WebSocketServer server(port);
    if (server.start()) {
        std::cout << "Server running on port " << port << std::endl;
        std::cout << "Press Enter to stop the server..." << std::endl;
        std::cin.get();  // Wait for Enter key to stop the server
        server.stop();
    } else {
        std::cerr << "Failed to start the server." << std::endl;
    }
}

int main() {
    std::string mode;
    std::cout << "Enter mode (client or server): ";
    std::getline(std::cin, mode);

    if (mode == "server") {
        int port = 8080; // Default port for server
        runServer(port);
    } else if (mode == "client") {
        std::string address = "localhost"; // Default address for client
        int port = 8080; // Default port for client
        runClient(address, port);
    } else {
        std::cerr << "Invalid mode specified. Use 'client' or 'server'." << std::endl;
        return 1;
    }

    return 0;
}
