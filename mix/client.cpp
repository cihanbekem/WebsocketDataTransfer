#include "client.h"
#include "student.pb.h"
#include <libwebsockets.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

struct lws* WebSocketClient::wsi = nullptr;

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
        std::cout << "Enter 'send_csv <filename> <format>' to send a CSV file in Protobuf or JSON format or 'exit' to quit: ";
        std::getline(std::cin, command);

        if (command.rfind("send_csv", 0) == 0) {
            std::istringstream stream(command.substr(9));
            std::string filename, format;
            stream >> filename >> format;

            bool isProtobuf = (format == "protobuf");

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

            auto start = std::chrono::high_resolution_clock::now();
            std::string serializedData;
            if (isProtobuf) {
                studentList.SerializeToString(&serializedData);
            } else {
                nlohmann::json jsonData;
                for (const auto& student : studentList.students()) {
                    nlohmann::json jsonStudent;
                    jsonStudent["student_id"] = student.student_id();
                    jsonStudent["name"] = student.name();
                    jsonStudent["age"] = student.age();
                    jsonStudent["email"] = student.email();
                    jsonStudent["major"] = student.major();
                    jsonStudent["gpa"] = student.gpa();
                    jsonData.push_back(jsonStudent);
                }
                serializedData = jsonData.dump();
            }
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;

            unsigned char buffer[LWS_PRE + serializedData.size()];
            std::memcpy(&buffer[LWS_PRE], serializedData.c_str(), serializedData.size());
            lws_write(wsi, &buffer[LWS_PRE], serializedData.size(), isProtobuf ? LWS_WRITE_BINARY : LWS_WRITE_TEXT);

            std::cout << "CSV data from '" << filename << "' sent as " << (isProtobuf ? "Protobuf." : "JSON.") << std::endl;
            std::cout << "Serialization time: " << elapsed.count() << " seconds" << std::endl;

        } else if (command == "exit") {
            stop();
        }
    }
}

int WebSocketClient::callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                         void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_RECEIVE: {
            std::string receivedData((const char *)in, len);
            if (receivedData[0] == '{' || receivedData[0] == '[') {  // JSON
                nlohmann::json jsonData = nlohmann::json::parse(receivedData);

                std::cout << "Received JSON data:" << std::endl;
                for (const auto& jsonStudent : jsonData) {
                    std::cout << "ID: " << jsonStudent["student_id"].get<int>()
                              << ", Name: " << jsonStudent["name"].get<std::string>()
                              << ", Age: " << jsonStudent["age"].get<int>()
                              << ", Email: " << jsonStudent["email"].get<std::string>()
                              << ", Major: " << jsonStudent["major"].get<std::string>()
                              << ", GPA: " << jsonStudent["gpa"].get<float>() << std::endl;
                }
            } else {  // Protobuf
                StudentList studentList;
                if (studentList.ParseFromString(receivedData)) {
                    std::cout << "Received Protobuf data:" << std::endl;
                    for (const Student& student : studentList.students()) {
                        std::cout << "ID: " << student.student_id()
                                  << ", Name: " << student.name()
                                  << ", Age: " << student.age()
                                  << ", Email: " << student.email()
                                  << ", Major: " << student.major()
                                  << ", GPA: " << student.gpa() << std::endl;
                    }
                }
            }
            break;
        }

        case LWS_CALLBACK_CLOSED:
            std::cout << "WebSocket connection closed." << std::endl;
            wsi = nullptr;
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
