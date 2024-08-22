#include "server.h"
#include "client.h"
#include <libwebsockets.h>
#include <vector>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>


int main() {
    WebSocketServer server(8080); // Sunucu portu 8080
    if (server.start()) {
        std::cout << "Server is running. Press Enter to stop..." << std::endl;
        std::cin.get(); // Kullanıcının Enter tuşuna basmasını bekle
        server.stop();  // Sunucuyu durdur
    } else {
        std::cerr << "Failed to start the server." << std::endl;
        return 1;
    }
    return 0;
}