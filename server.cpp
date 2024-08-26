#include "server.h"
#include "student.pb.h"
#include <iostream>
#include <thread>
#include <cstring>

using namespace std;

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
            cout << "Enter 'send_pb' to send protobuf data or 'exit' to quit: ";
            getline(cin, command);

            if (command == "send_pb") {
                StudentList studentList;

                // Örnek veriler ekleyelim
                Student* student1 = studentList.add_students();
                student1->set_student_id(1);
                student1->set_name("John Doe");
                student1->set_age(20);
                student1->set_email("john.doe@example.com");
                student1->set_major("Computer Science");
                student1->set_gpa(3.5);

                Student* student2 = studentList.add_students();
                student2->set_student_id(2);
                student2->set_name("Jane Smith");
                student2->set_age(22);
                student2->set_email("jane.smith@example.com");
                student2->set_major("Mathematics");
                student2->set_gpa(3.8);

                string serializedData;
                studentList.SerializeToString(&serializedData);

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

            StudentList studentList;
            if (studentList.ParseFromString(receivedData)) {
                cout << "Received Protobuf data:" << endl;
                for (const Student& student : studentList.students()) {
                    cout << "ID: " << student.student_id() << ", Name: " << student.name()
                         << ", Age: " << student.age() << ", Email: " << student.email()
                         << ", Major: " << student.major() << ", GPA: " << student.gpa() << endl;
                }
            } else {
                cout << "Received Text data: " << receivedData << endl;
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
