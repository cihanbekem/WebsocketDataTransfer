#include "client.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <fstream>
#include <vector>
#include <sstream>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

struct lws *WebSocketClient::wsi = nullptr; // Eksik tanım

WebSocketClient::WebSocketClient(const string& address, int port)
    : address(address), port(port), interrupted(false) {
    memset(&info, 0, sizeof(info));
    info.protocols = protocols;
}

WebSocketClient::~WebSocketClient() {
    lws_context_destroy(context);
}

bool WebSocketClient::connect() {
    context = lws_create_context(&info);
    if (!context) {
        cout << "lws_create_context failed\n";
        return false;
    }

    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = context;
    ccinfo.address = address.c_str();
    ccinfo.port = port;
    ccinfo.path = "/";
    ccinfo.protocol = protocols[0].name;
    ccinfo.host = lws_canonical_hostname(context);
    ccinfo.origin = "origin";
    ccinfo.ietf_version_or_minus_one = -1;

    wsi = lws_client_connect_via_info(&ccinfo);
    if (!wsi) {
        cout << "lws_client_connect_via_info failed\n";
        return false;
    }

    thread inputThread(&WebSocketClient::handleUserInput, this);

    while (!interrupted) {
        lws_service(context, 1000);
    }

    inputThread.join();
    return true;
}

void WebSocketClient::stop() {
    interrupted = true;
}

string WebSocketClient::getAddress() const {
    return address;
}

int WebSocketClient::getPort() const {
    return port;
}

void WebSocketClient::handleUserInput() {
    while (!interrupted) {
        string command;
        cout << "Enter 'send' to send a file, 'message' to send a message, or 'exit' to quit: ";
        getline(cin, command);

        if (command == "send") {
            string filePath;
            cout << "Enter the path of the file to send: ";
            getline(cin, filePath);

            if (filePath.size() >= 4 && filePath.substr(filePath.size() - 4) == ".csv") {
                ifstream file(filePath);
                if (!file.is_open()) {
                    cerr << "Failed to open file." << endl;
                    continue;
                }

                vector<vector<string>> csvData;
                string line;

                while (getline(file, line)) {
                    stringstream ss(line);
                    string item;
                    vector<string> row;

                    while (getline(ss, item, ',')) {
                        row.push_back(item);
                    }

                    csvData.push_back(row);
                }

                file.close();

                json j;
                for (size_t i = 0; i < csvData.size(); ++i) {
                    j["row" + to_string(i)] = csvData[i];
                }

                ofstream jsonFile("file_content.json");
                jsonFile << j.dump(4);
                jsonFile.close();

                // Veriyi WebSocket üzerinden gönder
                string fileContent = j.dump();
                unsigned char buffer[LWS_PRE + fileContent.size()];
                memcpy(&buffer[LWS_PRE], fileContent.c_str(), fileContent.size());
                lws_write(wsi, &buffer[LWS_PRE], fileContent.size(), LWS_WRITE_TEXT);
            }
        } else if (command == "message") {
            string message;
            cout << "Enter a message to send to the server: ";
            getline(cin, message);

            if (!message.empty()) {
                unsigned char buffer[LWS_PRE + message.size()];
                memcpy(&buffer[LWS_PRE], message.c_str(), message.size());
                lws_write(wsi, &buffer[LWS_PRE], message.size(), LWS_WRITE_TEXT);
            }
        } else if (command == "exit") {
            stop();
        }
    }
}

int WebSocketClient::callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                         void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            string receivedMessage((const char *)in, len);
            cout << "Received from server: " << receivedMessage << endl;

            // Mevcut JSON dosyasını aç ve oku
            ifstream inFile("received_data_from_server.json");
            json j;

            if (inFile) {
                inFile >> j;  // Eğer dosya zaten varsa ve içerik varsa onu oku
                inFile.close();
            }

            // Yeni mesajı satırlara böl
            vector<string> lines;
            stringstream ss(receivedMessage);
            string line;
            while (getline(ss, line, '\n')) {
                lines.push_back(line);
            }

            // Yeni satırları JSON nesnesine ekle
            json newEntry;
            newEntry["received_message"] = lines;
            j.push_back(newEntry);

            // Güncellenmiş JSON'u dosyaya tekrar yaz
            ofstream outFile("received_data_from_server.json");
            outFile << j.dump(4) << endl;  // Güzel formatlanmış şekilde JSON'u yaz
            outFile.close();

            cout << "Text content written to file." << endl;

            lws_callback_on_writable(wsi);
            break;
        }

        default:
            break;
    }
    return 0;
}

const struct lws_protocols WebSocketClient::protocols[] = {
    {"websocket-protocol", WebSocketClient::callback_websockets, 0, 0},
    {NULL, NULL, 0, 0} /* terminator */
};
