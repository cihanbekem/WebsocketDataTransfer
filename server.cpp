#include "server.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

using namespace std;

vector<unsigned char> WebSocketServer::fileBuffer;

WebSocketServer::WebSocketServer(int port)
    : port(port), interrupted(false) {
    memset(&info, 0, sizeof(info));
    info.port = port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
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

    cout << "Server is running on port " << port << ". Press Enter to stop..." << endl;

    while (!interrupted) {
        lws_service(context, 1000);
    }

    return true;
}

void WebSocketServer::stop() {
    interrupted = true;
}

int WebSocketServer::callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                         void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_RECEIVE: {
            StudentList studentList;
            if (studentList.ParseFromArray(in, len)) {
                cout << "Received Protobuf data:" << endl;
                for (const auto& student : studentList.students()) {
                    cout << "Student ID: " << student.student_id() << endl;
                    cout << "Name: " << student.name() << endl;
                    cout << "Age: " << student.age() << endl;
                    cout << "Email: " << student.email() << endl;
                    cout << "Major: " << student.major() << endl;
                    cout << "GPA: " << student.gpa() << endl;
                }
            } else {
                cout << "Received unknown format data." << endl;
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
    {NULL, NULL, 0, 0} /* terminator */
};
