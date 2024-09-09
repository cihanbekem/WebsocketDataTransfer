#include "client.hpp"
#include "server.hpp"

#include <iostream>
#include <string>
#include <thread>

void runClient(const std::string& address, int port)
{
    WebSocketClient client(address, port);
    if (client.connect())
    {
        std::cout << "Client connected to " << address << ":" << port << std::endl;

        std::thread inputThread(
            [&client]()
            {
                client.handleUserInput(); // Start user input handling in this thread
            }
        );

        // Check client.interrupted instead of client.getContext()
        while (!client.interrupted)
        {
            lws_service(client.getContext(), 1000);
        }

        inputThread.join();
    }
    else
    {
        std::cerr << "Failed to connect the client." << std::endl;
    }
}

void runServer(int port)
{
    WebSocketServer server(port);

    if (server.start())
    {
        std::cout << "Server running on port " << port << std::endl;
    }
    else
    {
        std::cerr << "Failed to start the server." << std::endl;
    }
}

int main()
{
    std::string mode;
    std::cout << "Enter mode (client or server): ";

    std::getline(std::cin, mode);
    if (mode == "server")
    {
        int port = 8080; // Default port for server
        runServer(port);
    }
    else if (mode == "client")
    {
        std::string address = "localhost"; // Default address for client
        int port = 8080; // Default port for client
        runClient(address, port);
    }
    else
    {
        std::cerr << "Invalid mode specified. Use 'client' or 'server'." << std::endl;
        return 1;
    }

    return 0;
}