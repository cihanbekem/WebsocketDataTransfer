
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
#include <vector> // std::vector kullanımına dikkat

#define LOG_INFO_CLIENT(message) \
    std::cout << "CLIENT_INFO: " << message << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl;

struct lws* WebSocketClient::wsi = nullptr;

bool WebSocketClient::is_json = false;
std::string WebSocketClient::accumulatedData;

WebSocketClient::WebSocketClient(const std::string& address, int port)
    : address(address), port(port), interrupted(false)
{
    std::memset(&info, 0, sizeof(info));
    info.protocols = protocols;
}

WebSocketClient::~WebSocketClient()
{
    lws_context_destroy(context);
}

bool WebSocketClient::connect()
{
    context = lws_create_context(&info);
    if (!context)
    {
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
    if (!wsi)
    {
        std::cerr << "lws_client_connect_via_info failed\n";
        return false;
    }

    return true;
}

void WebSocketClient::stop()
{
    interrupted = true;
}

void WebSocketClient::handleUserInput()
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

            try
            {
                if (isProtobuf)
                {
                    std::cout << "CSV data from '" << filename <<
                        "' sent as " << (isProtobuf ? "Protobuf." : "JSON.") << std::endl;
                    studentList.SerializeToString(&serializedData);

                    std::vector<unsigned char> buffer(LWS_PRE + serializedData.size());
                    std::memcpy(&buffer[LWS_PRE], serializedData.c_str(), serializedData.size());
                    LOG_INFO_CLIENT("print");

                    lws_write(wsi, &buffer[LWS_PRE], serializedData.size(), LWS_WRITE_BINARY);
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
                    serializedData = jsonData.dump(); // JSON verisini oluştur

                    // Save JSON to file
                    std::ofstream jsonFile("file_content_client.json");
                    jsonFile << jsonData.dump(4);
                    jsonFile.close();

                    std::vector<unsigned char> buffer(LWS_PRE + serializedData.size());
                    std::memcpy(&buffer[LWS_PRE], serializedData.data(), serializedData.size());

                    //std::cout << "clientSendData: " << serializedData.c_str() << std::endl;
                    //std::cout << "client data len buffer: " << buffer.size() << std::endl;
                    //std::cout << "client data len serializedData: " << serializedData.size() << std::endl;

                    lws_write(wsi, &buffer[LWS_PRE], serializedData.size(), LWS_WRITE_TEXT);
                }

            }
            catch (const std::exception& e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;

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

lws_context* WebSocketClient::getContext() const
{
    return context;  // Burada context dönüyoruz
}

bool WebSocketClient::isCompleteJson(const std::string& data)
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

bool WebSocketClient::isCompleteProtobuf(const std::string& data)
{
    StudentList studentList;
    return studentList.ParseFromString(data);
}


int WebSocketClient::callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                         void *user, void *in, size_t len)
{
    std::cout<<"client reason: "<<reason<<std::endl;
    switch (reason)
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            std::cout << "WebSocket connection established." << std::endl;
            WebSocketClient::wsi = wsi;  // Set the global wsi to the current connection
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
        {
            LOG_INFO_CLIENT("print");
            std::cout << "server data len: " << len <<
                " server in len: " << strlen((const char *)in) << std::endl;

            std::string receivedData((const char *)in, len);

            if (accumulatedData.length() == 0)
            {
                LOG_INFO_CLIENT("print");

                if (receivedData[0] == '{' || receivedData[0] == '[')
                {
                    LOG_INFO_CLIENT("print");

                    is_json = true;
                }
                else
                {
                    LOG_INFO_CLIENT("print");

                    is_json = false;
                }
            }

            accumulatedData.append((const char*)in, len);
            std::cout << "accumulatedData len" << accumulatedData.length() << std::endl;



            if (is_json)
            {
                LOG_INFO_CLIENT("print");

                if (isCompleteJson(accumulatedData)) //if (accumulatedData.length() > 12281)
                {
                    LOG_INFO_CLIENT("print");

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
                LOG_INFO_CLIENT("print");

                if (isCompleteProtobuf(accumulatedData))
                {
                    LOG_INFO_CLIENT("print");

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
        
        case LWS_CALLBACK_CLIENT_CLOSED:
            std::cout << "WebSocket connection closed." << std::endl;
            WebSocketClient::wsi = nullptr;  // Clear the global wsi when connection closes
            break;

        default:
                    std::cout << "WebSocket default." << std::endl;

        
            break;
    }
    return 0;
}

const struct lws_protocols WebSocketClient::protocols[] =
{
    {"websocket-protocol", WebSocketClient::callback_websockets, 0, 65000},
    {NULL, NULL, 0, 0}
};
