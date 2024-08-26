#include "student.pb.h"
#include <libwebsockets.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <thread>

using namespace std;

class WebSocketClient {
public:
    WebSocketClient(const string& address, int port);
    ~WebSocketClient();
    bool connect();
    void stop();
    void handleUserInput();

private:
    static struct lws *wsi;
    struct lws_context *context;
    struct lws_context_creation_info info;
    string address;
    int port;
    bool interrupted;

    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len);

    static const struct lws_protocols protocols[];
};

struct lws *WebSocketClient::wsi = nullptr;

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

void WebSocketClient::handleUserInput() {
    while (!interrupted) {
        string command;
        cout << "Enter 'send_csv <filename>' to send a CSV file or 'exit' to quit: ";
        getline(cin, command);

        if (command.rfind("send_csv", 0) == 0) {
            string filename = command.substr(9);
            StudentList studentList;
            ifstream file(filename);
            string line;

            getline(file, line); // Skip header

            while (getline(file, line)) {
                stringstream ss(line);
                string item;
                Student* student = studentList.add_students();

                getline(ss, item, ',');
                student->set_student_id(stoi(item));

                getline(ss, item, ',');
                student->set_name(item);

                getline(ss, item, ',');
                student->set_age(stoi(item));

                getline(ss, item, ',');
                student->set_email(item);

                getline(ss, item, ',');
                student->set_major(item);

                getline(ss, item, ',');
                student->set_gpa(stof(item));
            }

            string serializedData;
            studentList.SerializeToString(&serializedData);

            unsigned char buffer[LWS_PRE + serializedData.size()];
            memcpy(&buffer[LWS_PRE], serializedData.c_str(), serializedData.size());
            lws_write(wsi, &buffer[LWS_PRE], serializedData.size(), LWS_WRITE_BINARY);

            cout << "CSV data from '" << filename << "' sent as Protobuf." << endl;

        } else if (command == "exit") {
            stop();
        }
    }
}

int WebSocketClient::callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                         void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_RECEIVE:
            cout << "Received message from server." << endl;
            break;

        default:
            break;
    }
    return 0;
}

const struct lws_protocols WebSocketClient::protocols[] = {
    {"websocket-protocol", WebSocketClient::callback_websockets, 0, 0},
    {NULL, NULL, 0, 0}
};

WebSocketClient* createClient(const std::string& address, int port) {
    return new WebSocketClient(address, port);
}

