#include "server.h"
#include "student.pb.h"
#include <libwebsockets.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <fstream>
#include <sstream>
#include <chrono>  // For timing

struct lws* WebSocketServer::wsi = nullptr;  // Static üye tanımı

WebSocketServer::WebSocketServer(int port) : port(port), interrupted(false) {
    std::memset(&info, 0, sizeof(info));
    info.port = port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;  // Optional: validate UTF-8
}

WebSocketServer::~WebSocketServer() {
    lws_context_destroy(context);
}

bool WebSocketServer::start() {
    context = lws_create_context(&info);
    if (!context) {
        std::cerr << "lws_create_context failed\n";
        return false;
    }

    std::cout << "Server started on port " << port << std::endl;

    std::thread inputThread(&WebSocketServer::handleUserInput, this);

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
    std::string command;
    while (!interrupted) {
        std::cout << "Enter 'send_csv <filename>' to send a CSV file or 'stop' to quit: ";
        std::getline(std::cin, command);
        processCommand(command);
    }
}

void WebSocketServer::processCommand(const std::string& command) {
    if (command.rfind("send_csv", 0) == 0) {
        std::string filename = command.substr(9);

        // Measure the time it takes to send the data
        auto start = std::chrono::high_resolution_clock::now();

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

        sendData(serializedData);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        std::cout << "CSV data from '" << filename << "' sent as Protobuf." << std::endl;
        std::cout << "Send time: " << elapsed.count() << " seconds" << std::endl;

    } else if (command.rfind("send_message", 0) == 0) {
        std::string message = command.substr(13);
        sendData(message);

        std::cout << "Message sent: " << message << std::endl;

    } else if (command == "stop") {
        stop();
    } else {
        std::cout << "Unknown command." << std::endl;
    }
}

void WebSocketServer::sendData(const std::string& data) {
    if (wsi) {
        unsigned char buffer[LWS_PRE + data.size()];
        std::memcpy(&buffer[LWS_PRE], data.c_str(), data.size());
        lws_write(wsi, &buffer[LWS_PRE], data.size(), LWS_WRITE_BINARY);
    } else {
        std::cerr << "WebSocket connection is not established.\n";
    }
}

int WebSocketServer::callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                         void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            std::cout << "WebSocket connection established." << std::endl;
            WebSocketServer::wsi = wsi;  // Set the global wsi to the current connection
            break;

        case LWS_CALLBACK_RECEIVE: {
            std::string receivedData((const char *)in, len);
            StudentList studentList;
            if (studentList.ParseFromString(receivedData)) {
                std::cout << "Received Protobuf data:" << std::endl;
                for (const Student& student : studentList.students()) {
                    std::cout << "ID: " << student.student_id() << ", Name: " << student.name()
                             << ", Age: " << student.age() << ", Email: " << student.email()
                             << ", Major: " << student.major() << ", GPA: " << student.gpa() << std::endl;
                }
            } else {
                std::cout << "Received message: " << receivedData << std::endl;
            }
            break;
        }

        case LWS_CALLBACK_CLOSED:
            std::cout << "WebSocket connection closed." << std::endl;
            WebSocketServer::wsi = nullptr;  // Clear the global wsi when connection closes
            break;

        default:
            break;
    }
    return 0;
}

const struct lws_protocols WebSocketServer::protocols[] = {
    {"websocket-protocol", WebSocketServer::callback_websockets, 0, 0},
    {NULL, NULL, 0, 0}
};
