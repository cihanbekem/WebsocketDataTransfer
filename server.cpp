#include "student.pb.h"
#include <libwebsockets.h>
#include <iostream>
#include <cstring>
#include <thread>
//#include "server.h"

using namespace std;

class WebSocketServer {
public:
    WebSocketServer(int port);
    ~WebSocketServer();
    bool start();
    void stop();
    void handleUserInput();

private:
    static struct lws *wsi;
    struct lws_context *context;
    struct lws_context_creation_info info;
    int port;
    bool interrupted;

    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len);

    static const struct lws_protocols protocols[];
};

struct lws *WebSocketServer::wsi = nullptr;

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
            cout << "Enter 'exit' to quit: ";
            getline(cin, command);

            if (command == "exit") {
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
            StudentList studentList;
            if (studentList.ParseFromString(receivedData)) {
                cout << "Received Protobuf data:" << endl;
                for (const Student& student : studentList.students()) {
                    cout << "ID: " << student.student_id() << ", Name: " << student.name()
                         << ", Age: " << student.age() << ", Email: " << student.email()
                         << ", Major: " << student.major() << ", GPA: " << student.gpa() << endl;
                }
            } else {
                cout << "Received unknown data: " << receivedData << endl;
            }
            break;
        }

        default:
            break;
    }
    return 0;
}

const struct lws_protocols WebSocketServer::protocols[] = {
    {"websocket-protocol", WebSocketServer::callback_websockets, 0, 0},
    {NULL, NULL, 0, 0}
};

WebSocketServer* createServer(int port) {
    return new WebSocketServer(port);
}

