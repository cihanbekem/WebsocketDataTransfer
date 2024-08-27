#include "server.h"
#include "client.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <sstream>
#include <vector>

using namespace std;

int main() {
    // İstemci ve sunucu seçimi
    string mode;
    cout << "Sunucu başlatmak için 'server', istemci başlatmak için 'client' yazın: ";
    getline(cin, mode);

    if (mode == "server") {
        // Sunucu başlatma
        WebSocketServer server(8080); // Sunucu portu 8080
        if (server.start()) {
            cout << "Server is running. Press Enter to stop..." << endl;
            cin.get(); // Kullanıcının Enter tuşuna basmasını bekle
            server.stop();  // Sunucuyu durdur
        } else {
            cerr << "Failed to start the server." << endl;
            return 1;
        }
    } else if (mode == "client") {
        // İstemci başlatma
        WebSocketClient client("localhost", 8080); // localhost ve port 8080
        if (client.connect()) {
            cout << "Client connected to server." << endl;
        } else {
            cerr << "Failed to connect to the server." << endl;
            return 1;
        }
    } else {
        cerr << "Geçersiz seçim! 'server' ya da 'client' yazmalısınız." << endl;
        return 1;
    }

    return 0;
}