#include "server.h"
#include "client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

using namespace std;

int main() {
    string mode;
    cout << "Sunucu başlatmak için 'server', istemci başlatmak için 'client' yazın: ";
    getline(cin, mode);

    if (mode == "server") {
        WebSocketServer server(8080); // Sunucu portu 8080
        if (server.start()) {
            cout << "Server is running. Press Enter to stop..." << endl;
            cin.get(); // Kullanıcının Enter tuşuna basmasını bekle
            server.stop(); // Sunucuyu durdur
        } else {
            cerr << "Failed to start the server." << endl;
            return 1;
        }
    } else if (mode == "client") {
        WebSocketClient client("localhost", 8080); // localhost ve port 8080
        auto start = chrono::steady_clock::now(); // Zaman ölçümüne başla
        if (client.connect()) {
            auto end = chrono::steady_clock::now(); // Zaman ölçümünü bitir
            chrono::duration<double> elapsed = end - start;
            cout << "Client connected to server in " << elapsed.count() << " seconds." << endl;
            cout << "Enter 'send' to send a file or 'message' to send a message: ";
            
            string command;
            while (getline(cin, command)) {
                if (command == "send") {
                    // Dosya gönderimi işlemi
                    string filePath;
                    cout << "Enter the path of the file to send: ";
                    getline(cin, filePath);
                    
                    // Zaman ölçümünü başlat
                    auto startSend = chrono::steady_clock::now();
                    // Dosyayı gönder (implementasyon eksik)
                    auto endSend = chrono::steady_clock::now();
                    chrono::duration<double> elapsedSend = endSend - startSend;
                    cout << "File sent in " << elapsedSend.count() << " seconds." << endl;
                } else if (command == "message") {
                    string message;
                    cout << "Enter a message to send to the server: ";
                    getline(cin, message);
                    
                    // Zaman ölçümünü başlat
                    auto startMessage = chrono::steady_clock::now();
                    // Mesajı gönder (implementasyon eksik)
                    auto endMessage = chrono::steady_clock::now();
                    chrono::duration<double> elapsedMessage = endMessage - startMessage;
                    cout << "Message sent in " << elapsedMessage.count() << " seconds." << endl;
                } else if (command == "exit") {
                    client.stop();
                    break;
                }
            }
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
