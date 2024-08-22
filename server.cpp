#include "server.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <sstream>
#include <vector>

using namespace std;
using json = nlohmann::json;

struct lws *WebSocketServer::wsi = nullptr;
vector<unsigned char> WebSocketServer::fileBuffer;

WebSocketServer::WebSocketServer(int port) : port(port), interrupted(false) {
    memset(&info, 0, sizeof(info));
    info.port = port;
    info.protocols = protocols;
}

WebSocketServer::~WebSocketServer() {
    lws_context_destroy(context);
}

bool WebSocketServer::start() {
    context = lws_create_context(&info);
    if (!context) {
        cerr << "lws_create_context failed\n";
        return false;
    }

    cout << "Server started on port " << port << endl;

    thread inputThread(&WebSocketServer::handleUserInput, this);

    while (!interrupted) {
        lws_service(context, 1000);
    }

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
            cout << "Enter 'send' to send a file, 'message' to send a message, 'send_pb' to send protobuf data, or 'exit' to quit: ";
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

                streambuf* sbuf = file.rdbuf();
                size_t size = sbuf->pubseekoff(0, ios::end, ios::in);
                sbuf->pubseekpos(0, ios::in);

                string fileContent(size, '\0');
                sbuf->sgetn(&fileContent[0], size);
                file.close();

                unsigned char buffer[LWS_PRE + fileContent.size()];
                memcpy(&buffer[LWS_PRE], fileContent.c_str(), fileContent.size());
                lws_write(wsi, &buffer[LWS_PRE], fileContent.size(), LWS_WRITE_TEXT);

                json j;
                vector<string> lines;
                stringstream ss(fileContent);
                string line;
                while (getline(ss, line)) {
                    lines.push_back(line);
                }
                j["file_content"] = lines;

                ofstream jsonFile("file_content.json");
                jsonFile << j.dump(4);
                jsonFile.close();

            } else if (command == "message") {
                string message;
                cout << "Enter a message to send to the client: ";
                getline(cin, message);

                if (!message.empty()) {
                    unsigned char buffer[LWS_PRE + message.size()];
                    memcpy(&buffer[LWS_PRE], message.c_str(), message.size());
                    lws_write(wsi, &buffer[LWS_PRE], message.size(), LWS_WRITE_TEXT);
                }

            } else if (command == "send_pb") {
                FileContent fileContent;
                fileContent.add_lines("This is a line in protobuf format");
                fileContent.add_lines("This is another line in protobuf format");

                string serializedData;
                fileContent.SerializeToString(&serializedData);

                unsigned char buffer[LWS_PRE + serializedData.size()];
                memcpy(&buffer[LWS_PRE], serializedData.c_str(), serializedData.size());
                lws_write(wsi, &buffer[LWS_PRE], serializedData.size(), LWS_WRITE_BINARY);

                cout << "Protobuf data sent." << endl;

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
            string receivedData((const char *)in, len);

            // Protobuf verisi olup olmadığını kontrol et
            FileContent fileContent;
            if (fileContent.ParseFromString(receivedData)) {
                cout << "Received Protobuf data:" << endl;
                for (int i = 0; i < fileContent.lines_size(); i++) {
                    cout << fileContent.lines(i) << endl;
                }
            } else {
                // JSON verisi olarak kabul edip işleme devam edelim
                cout << "Received JSON/Text data: " << receivedData << endl;

                // Burada JSON verisini işlemeye devam edebiliriz
                // Örneğin, JSON verisini dosyaya yazmak:
                json receivedJson = json::parse(receivedData);
                ofstream outFile("received_data_from_server.json");
                outFile << receivedJson.dump(4);
                outFile.close();

                cout << "Text content written to file." << endl;
            }
            
            lws_callback_on_writable(wsi);
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
