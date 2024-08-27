#include "client.h"
#include "student.pb.h"
#include <libwebsockets.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <thread>

struct lws* WebSocketClient::wsi = nullptr;  // Static üye tanımı

WebSocketClient::WebSocketClient(const std::string& address, int port)
    : address(address), port(port), interrupted(false) {
    std::memset(&info, 0, sizeof(info));
    info.protocols = protocols;
}

WebSocketClient::~WebSocketClient() {
    lws_context_destroy(context);
}

bool WebSocketClient::connect() {
    context = lws_create_context(&info);
    if (!context) {
        std::cerr << "lws_create_context failed\n";
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
        std::cerr << "lws_client_connect_via_info failed\n";
        return false;
    }

    return true;
}

void WebSocketClient::stop() {
    interrupted = true;
}

void WebSocketClient::handleUserInput() {
    while (!interrupted) {
        std::string command;
        std::cout << "Enter 'send_csv <filename>' to send a CSV file or 'exit' to quit: ";
        std::getline(std::cin, command);

        if (command.rfind("send_csv", 0) == 0) {
            std::string filename = command.substr(9);
            StudentList studentList;
            std::ifstream file(filename);
            std::string line;

            std::getline(file, line); // Skip header

            while (std::getline(file, line)) {
                std::stringstream ss(line);
                std::string item;
                Student* student = studentList.add_students();

                std::getline(ss, item, ',');
                student->set_student_id(std::stoi(item));

                std::getline(ss, item, ',');
                student->set_name(item);

                std::getline(ss, item, ',');
                student->set_age(std::stoi(item));

                std::getline(ss, item, ',');
                student->set_email(item);

                std::getline(ss, item, ',');
                student->set_major(item);

                std::getline(ss, item, ',');
                student->set_gpa(std::stof(item));
            }

            std::string serializedData;
            studentList.SerializeToString(&serializedData);

            unsigned char buffer[LWS_PRE + serializedData.size()];
            std::memcpy(&buffer[LWS_PRE], serializedData.c_str(), serializedData.size());
            lws_write(wsi, &buffer[LWS_PRE], serializedData.size(), LWS_WRITE_BINARY);

            std::cout << "CSV data from '" << filename << "' sent as Protobuf." << std::endl;

        } else if (command == "exit") {
            stop();
        }
    }
}

int WebSocketClient::callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                         void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_RECEIVE:
            std::cout << "Received message from server." << std::endl;
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

lws_context* WebSocketClient::getContext() const {
    return context;
}