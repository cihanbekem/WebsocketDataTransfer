#include "client.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
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
        cout << "Enter 'send' to send a CSV file, 'message' to send a message, or 'exit' to quit: ";
        getline(cin, command);

        if (command == "send") {
            string filePath;
            cout << "Enter the path of the CSV file to send: ";
            getline(cin, filePath);

            if (filePath.size() >= 4 && filePath.substr(filePath.size() - 4) == ".csv") {
                ifstream file(filePath);
                if (!file.is_open()) {
                    cerr << "Could not open file." << endl;
                    continue;
                }

                StudentList studentList;
                string line;
                getline(file, line);  // Skip header line

                while (getline(file, line)) {
                    stringstream ss(line);
                    string token;
                    Student* student = studentList.add_students();

                    getline(ss, token, ','); student->set_student_id(stoi(token));
                    getline(ss, token, ','); student->set_name(token);
                    getline(ss, token, ','); student->set_age(stoi(token));
                    getline(ss, token, ','); student->set_email(token);
                    getline(ss, token, ','); student->set_major(token);
                    getline(ss, token, ','); student->set_gpa(stof(token));
                }

                file.close();

                // Send Protobuf data
                string serializedData;
                studentList.SerializeToString(&serializedData);

                unsigned char buffer[LWS_PRE + serializedData.size()];
                memcpy(&buffer[LWS_PRE], serializedData.c_str(), serializedData.size());
                lws_write(wsi, &buffer[LWS_PRE], serializedData.size(), LWS_WRITE_BINARY);
            }
        } else if (command == "message") {
            string message;
            cout << "Enter the message to send to the server: ";
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

            StudentList studentList;
            if (studentList.ParseFromString(receivedMessage)) {
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
