#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <fstream>
#include <streambuf>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

class WebSocketClient {
public:
    WebSocketClient(const string& address, int port)
        : address(address), port(port), interrupted(false) {
        memset(&info, 0, sizeof(info));
        info.protocols = protocols;
    }

    ~WebSocketClient() {
        lws_context_destroy(context);
    }

    bool connect() {
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

    void stop() {
        interrupted = true;
    }

    string getAddress() const {
        return address;
    }

    int getPort() const {
        return port;
    }

private:
    void handleUserInput() {
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

                    string line;
                    while (getline(file, line)) {
                        unsigned char buffer[LWS_PRE + line.size()];
                        memcpy(&buffer[LWS_PRE], line.c_str(), line.size());
                        lws_write(wsi, &buffer[LWS_PRE], line.size(), LWS_WRITE_TEXT);
                        lws_callback_on_writable(wsi);
                    }

                    file.close();
                    cout << "CSV file sent successfully." << endl;

                } else {
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
                    lws_callback_on_writable(wsi);
                    cout << "File sent successfully." << endl;
                }

                string endSignal = "END";
                unsigned char endBuffer[LWS_PRE + endSignal.size()];
                memcpy(&endBuffer[LWS_PRE], endSignal.c_str(), endSignal.size());
                lws_write(wsi, &endBuffer[LWS_PRE], endSignal.size(), LWS_WRITE_TEXT);
                lws_callback_on_writable(wsi);
            } else if (command == "message") {
                string message;
                cout << "Enter a message to send to the server: ";
                getline(cin, message);

                if (!message.empty()) {
                    unsigned char buffer[LWS_PRE + message.size()];
                    memcpy(&buffer[LWS_PRE], message.c_str(), message.size());
                    lws_write(wsi, &buffer[LWS_PRE], message.size(), LWS_WRITE_TEXT);
                    lws_callback_on_writable(wsi);
                }
            } else if (command == "exit") {
                stop();
            }
        }
    }

    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len) {
        switch (reason) {
            case LWS_CALLBACK_CLIENT_ESTABLISHED:
                cout << "WebSocket Client connected\n";
                lws_callback_on_writable(wsi);
                break;

            case LWS_CALLBACK_CLIENT_RECEIVE: {
                string receivedMessage((const char *)in, len);
                cout << "Received from server: " << receivedMessage << endl;

                if (receivedMessage == "Successfully opened") {
                    cout << "Server successfully opened the file." << endl;
                } else {
                    ofstream outFile("received_data_from_server.json");
                    json j;
                    j["received_message"] = receivedMessage;
                    outFile << j.dump(4);
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

    static const struct lws_protocols protocols[];

    struct lws_context_creation_info info;
    struct lws_context *context;
    struct lws *wsi;
    std::string address;
    int port;
    bool interrupted;
};

const struct lws_protocols WebSocketClient::protocols[] = {
    {"websocket-protocol", WebSocketClient::callback_websockets, 0, 0},
    {NULL, NULL, 0, 0} /* terminator */
};

int main() {
    WebSocketClient client("localhost", 8080);
    if (client.connect()) {
        cout << "WebSocket Client connected to server\n";
    }

    json client_json;
    client_json["address"] = client.getAddress();
    client_json["port"] = client.getPort();
    client_json["status"] = "connected";

    ofstream jsonFile("client_info.json");
    jsonFile << client_json.dump(4);
    jsonFile.close();

    return 0;
}
