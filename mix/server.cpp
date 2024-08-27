#include "server.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sstream>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

// Statik değişkenlerin tanımı
struct lws *WebSocketServer::wsi = nullptr;
vector<unsigned char> WebSocketServer::fileBuffer;

WebSocketServer::WebSocketServer(int port) : port(port), interrupted(false) {
    memset(&info, 0, sizeof(info));
    info.port = port;
    info.protocols = protocols;
}

WebSocketServer::~WebSocketServer() {
    if (context) lws_context_destroy(context);
}

bool WebSocketServer::start() {
    context = lws_create_context(&info);
    if (!context) {
        cerr << "lws_create_context failed\n";
        return false;
    }

    cout << "Server started on port " << port << endl;
    thread inputThread(&WebSocketServer::handleUserInput, this);
    while (!interrupted) lws_service(context, 1000);
    inputThread.join();
    return true;
}

void WebSocketServer::stop() {
    interrupted = true;
}

void WebSocketServer::handleUserInput() {
    while (!interrupted) {
        if (wsi) {
            string command;
            cout << "Enter 'send' to send a file, 'message' to send a message, or 'exit' to quit: ";
            getline(cin, command);

            if (command == "send") {
                string filePath;
                cout << "Enter the path of the file to send: ";
                getline(cin, filePath);

                ifstream file(filePath, ios::binary | ios::ate);
                if (!file.is_open()) {
                    cerr << "Failed to open file." << endl;
                    continue;
                }

                auto start = chrono::steady_clock::now();
                streamsize size = file.tellg();
                file.seekg(0, ios::beg);
                vector<char> buffer(size);
                if (file.read(buffer.data(), size)) {
                    lws_write(wsi, (unsigned char*)buffer.data(), size, LWS_WRITE_BINARY);
                }
                auto end = chrono::steady_clock::now();
                chrono::duration<double> elapsedSend = end - start;
                cout << "File sent in " << elapsedSend.count() << " seconds." << endl;
            } else if (command == "message") {
                string message;
                cout << "Enter a message to send to the client: ";
                getline(cin, message);

                if (!message.empty()) {
                    auto start = chrono::steady_clock::now();
                    lws_write(wsi, (unsigned char*)message.c_str(), message.size(), LWS_WRITE_TEXT);
                    auto end = chrono::steady_clock::now();
                    chrono::duration<double> elapsedMessage = end - start;
                    cout << "Message sent in " << elapsedMessage.count() << " seconds." << endl;
                }
            } else if (command == "exit") {
                stop();
            }
        }
    }
}

int WebSocketServer::callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                         void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            cout << "Client connected" << endl;
            WebSocketServer::wsi = wsi;
            break;

        case LWS_CALLBACK_RECEIVE: {
            string receivedMessage((const char *)in, len);
            cout << "Received: " << receivedMessage << endl;

            auto start = chrono::steady_clock::now();
            if (receivedMessage.find("{") == 0) { // Eğer JSON formatında veri geldiyse
                ofstream outFile("received_data_from_client.json", ios::app);
                if (outFile.is_open()) {
                    outFile << receivedMessage << endl;
                    outFile.close();
                } else {
                    cerr << "Failed to open JSON file for writing." << endl;
                }
            } else {
                // Normal veri ise
                json j;
                vector<string> lines;
                stringstream ss(receivedMessage);
                string line;
                while (getline(ss, line, '\n')) lines.push_back(line);

                json newEntry;
                newEntry["received_message"] = lines;
                j.push_back(newEntry);

                ofstream outFile("received_data_from_client.json");
                if (outFile.is_open()) {
                    outFile << j.dump(4) << endl;
                    outFile.close();
                } else {
                    cerr << "Failed to open JSON file for writing." << endl;
                }
            }
            auto end = chrono::steady_clock::now();
            chrono::duration<double> elapsed = end - start;
            cout << "Data received and written to file in " << elapsed.count() << " seconds." << endl;
            break;
        }

        default:
            break;
    }
    return 0;
}

const struct lws_protocols WebSocketServer::protocols[] = {
    {"websocket-protocol", WebSocketServer::callback_websockets, 0, 0},
    {NULL, NULL, 0, 0} /* terminator */
};
