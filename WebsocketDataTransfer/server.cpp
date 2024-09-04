#include "server.hpp"
#include "student.pb.h"
#include <libwebsockets.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

#define LOG_INFO_SERVER(message) \
    std::cout << "SERVER_INFO: " << message << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl;

struct lws* WebSocketServer::wsi = nullptr;

bool is_json = false;
std::string accumulatedData;

WebSocketServer::WebSocketServer(int port) : m_port(port), interrupted(false)
{
    std::memset(&info, 0, sizeof(info));
    info.port = port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;
}

WebSocketServer::~WebSocketServer()
{
    lws_context_destroy(context);
}

bool WebSocketServer::start()
{
    context = lws_create_context(&info);
    if (!context)
    {
        std::cerr << "lws_create_context failed\n";
        return false;
    }

    std::cout << "Server started on port " << m_port << std::endl;

    std::thread inputThread(&WebSocketServer::handleUserInput, this);

    while (!interrupted)
    {
        lws_service(context, 0);
    }

    inputThread.join();
    return true;
}

void WebSocketServer::stop()
{
    interrupted = true;
}

void WebSocketServer::handleUserInput()
{
    while (!interrupted)
    {
        std::string command;
        std::cout << "Enter 'send_csv <filename> <format>' to send a CSV file"
            " in Protobuf or JSON format or 'exit' to quit: ";
        std::getline(std::cin, command);

        if (command.rfind("send_csv", 0) == 0)
        {
            std::istringstream stream(command.substr(9));
            std::string filename, format;
            stream >> filename >> format;

            bool isProtobuf = (format == "protobuf");

            StudentList studentList;
            std::ifstream file(filename);
            std::string line;

            std::getline(file, line); // Skip header

            while (std::getline(file, line))
            {
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
            if (isProtobuf)
            {
                studentList.SerializeToString(&serializedData);

                // Save Protobuf to file
                std::ofstream protobufFile("file_content_server.pb", std::ios::binary);
                protobufFile.write(serializedData.data(), serializedData.size());
                protobufFile.close();
            }
            else
            {
                nlohmann::json jsonData;
                for (const auto& student : studentList.students())
                {
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

                // Save JSON to file
                std::ofstream jsonFile("file_content_server.json");
                jsonFile << jsonData.dump(4);
                jsonFile.close();
            }
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;

            // Allocate buffer size and copy data
            unsigned char* buffer = new unsigned char[LWS_PRE + serializedData.size()];
            std::memcpy(&buffer[LWS_PRE], serializedData.c_str(), serializedData.size());

            std::cout << "server data len buffer: " << (LWS_PRE + serializedData.size()) << std::endl;
            std::cout << "server data len serializedData: " << serializedData.size() << std::endl;

            // Write data using libwebsockets
            lws_write(wsi, &buffer[LWS_PRE], serializedData.size(),
                isProtobuf ? LWS_WRITE_BINARY : LWS_WRITE_TEXT);

            // Free allocated buffer
            delete[] buffer;

            std::cout << "CSV data from '" << filename <<
                "' sent as " << (isProtobuf ? "Protobuf." : "JSON.") << std::endl;
            std::cout << "Serialization time: " << elapsed.count() << " seconds" << std::endl;

        }
        else if (command == "exit")
        {
            stop();
        }
    }
}

// Check if the accumulated data is a complete JSON message
bool isCompleteJson(const std::string& data)
{
    // Simple check for JSON completeness
    // This is an example; you might need a more sophisticated check
    size_t openBrackets =
        std::count(data.begin(), data.end(), '{') + std::count(data.begin(), data.end(), '[');
    size_t closeBrackets =
        std::count(data.begin(), data.end(), '}') + std::count(data.begin(), data.end(), ']');

    std::cout << "acc data openBrackets: " << openBrackets <<" acc data closeBrackets: " << closeBrackets << std::endl;

    return openBrackets == closeBrackets;
}

// Check if the accumulated data is a complete Protobuf message
bool isCompleteProtobuf(const std::string& data)
{
    StudentList studentList;
    return studentList.ParseFromString(data);
}

int WebSocketServer::callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                         void *user, void *in, size_t len)
{
    switch (reason)
    {
        case LWS_CALLBACK_ESTABLISHED:
            std::cout << "WebSocket connection established." << std::endl;
            WebSocketServer::wsi = wsi;  // Set the global wsi to the current connection
            break;

        case LWS_CALLBACK_RECEIVE:
        {
            LOG_INFO_SERVER("print");
            std::cout << "server data len: " << len <<
                " server in len: " << strlen((const char *)in) << std::endl;

            std::string receivedData((const char *)in, len);

            if (accumulatedData.length() == 0)
            {
                LOG_INFO_SERVER("print");

                if (receivedData[0] == '{' || receivedData[0] == '[')
                {
                    LOG_INFO_SERVER("print");

                    is_json = true;
                }
                else
                {
                    LOG_INFO_SERVER("print");

                    is_json = false;
                }
            }

            accumulatedData.append((const char*)in, len);
            std::cout << "accumulatedData len" << accumulatedData.length() << std::endl;



            if (is_json)
            {
                LOG_INFO_SERVER("print");

                if (isCompleteJson(accumulatedData)) //if (accumulatedData.length() > 12281)
                {
                    LOG_INFO_SERVER("print");

                    nlohmann::json jsonData = nlohmann::json::parse(accumulatedData);

                    std::cout << "Received JSON data:" << std::endl;
                    for (const auto& jsonStudent : jsonData)
                    {
                        std::cout << "ID: " << jsonStudent["student_id"].get<int>()
                                << ", Name: " << jsonStudent["name"].get<std::string>()
                                << ", Age: " << jsonStudent["age"].get<int>()
                                << ", Email: " << jsonStudent["email"].get<std::string>()
                                << ", Major: " << jsonStudent["major"].get<std::string>()
                                << ", GPA: " << jsonStudent["gpa"].get<float>() << std::endl;
                    }

                    accumulatedData.clear();
                }
            }
            else
            {
                LOG_INFO_SERVER("print");

                if (isCompleteProtobuf(accumulatedData))
                {
                    LOG_INFO_SERVER("print");

                    StudentList studentList;
                    if (studentList.ParseFromString(accumulatedData))
                    {
                        std::cout << "Received Protobuf data:" << std::endl;
                        for (const Student& student : studentList.students())
                        {
                            std::cout << "ID: " << student.student_id()
                                    << ", Name: " << student.name()
                                    << ", Age: " << student.age()
                                    << ", Email: " << student.email()
                                    << ", Major: " << student.major()
                                    << ", GPA: " << student.gpa() << std::endl;
                        }
                    }

                    accumulatedData.clear();
                }
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

const struct lws_protocols WebSocketServer::protocols[] =
{
    {"websocket-protocol", WebSocketServer::callback_websockets, 0, 65000},
    {NULL, NULL, 0, 0}
};