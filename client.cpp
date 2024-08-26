#include "client.h"
#include "student.pb.h"
#include <iostream>
#include <cstring>
#include <thread>

using namespace std;

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
        cerr << "lws_create_context failed\n";
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
        cerr << "lws_client_connect_via_info failed\n";
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
        cout << "Enter 'send_pb' to send protobuf data or 'exit' to quit: ";
        getline(cin, command);

        if (command == "send_pb") {
            StudentList studentList;

            // Örnek veriler ekleyelim
            Student* student1 = studentList.add_students();
            student1->set_student_id(1);
            student1->set_name("Alice Johnson");
            student1->set_age(21);
            student1->set_email("alice.johnson@example.com");
            student1->set_major("Physics");
            student1->set_gpa(3.9);

            Student* student2 = studentList.add_students();
            student2->set_student_id(2);
            student2->set_name("Bob Smith");
            student2->set_age(22);
            student2->set_email("bob.smith@example.com");
            student2->set_major("Chemistry");
            student2->set_gpa(3.6);

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

int WebSocketClient::callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                         void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            string receivedMessage((const char *)in, len);

            StudentList studentList;
            if (studentList.ParseFromString(receivedMessage)) {
                cout << "Received Protobuf data:" << endl;
                for (const Student& student : studentList.students()) {
                    cout << "ID: " << student.student_id() << ", Name: " << student.name()
                         << ", Age: " << student.age() << ", Email: " << student.email()
                         << ", Major: " << student.major() << ", GPA: " << student.gpa() << endl;
                }
            } else {
                cout << "Received Text data: " << receivedMessage << endl;
            }

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
