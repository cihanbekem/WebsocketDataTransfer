
#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <fstream>
#include <streambuf>
#include <nlohmann/json.hpp>
#include <vector>
#include <sstream>

using namespace std;
using json = nlohmann::json;

class WebSocketClient {
public:
    WebSocketClient(const string& address, int port);
    ~WebSocketClient();
    bool connect();
    void stop();
    string getAddress() const;
    int getPort() const;

private:
    void handleUserInput();
    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len);

    static const struct lws_protocols protocols[];
    struct lws_context_creation_info info;
    struct lws_context *context;
    struct lws *wsi;
    std::string address;
    int port;
    bool interrupted;
};

#endif // WEBSOCKETCLIENT_H