#include "server.h"
#include "client.h"
#include <iostream>

int main() {
    std::string mode;
    std::cout << "Choose mode (client/server): ";
    std::cin >> mode;

    if (mode == "client") {
        WebSocketClient client("localhost", 8080);
        if (client.connect()) {
            std::cout << "WebSocket Client connected to server\n";
        }

        json client_json;
        client_json["address"] = client.getAddress();
        client_json["port"] = client.getPort();
        client_json["status"] = "connected";

        std::ofstream jsonFile("client_info.json");
        jsonFile << client_json.dump(4);
        jsonFile.close();

    } else if (mode == "server") {
        WebSocketServer server(8080);
        if (server.start()) {
            std::cout << "WebSocket Server started on port 8080\n";
        }
    } else {
        std::cout << "Invalid mode selected.\n";
    }

    return 0;
}
