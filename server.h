
#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

class WebSocketServer {
public:
    WebSocketServer(int port);
    ~WebSocketServer();
    bool start();
    void stop();

private:
    void handleUserInput();
    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len);

    static const struct lws_protocols protocols[];

    struct lws_context_creation_info info;
    struct lws_context *context;
    int port;
    bool interrupted;

    static struct lws *wsi;  
    static vector<unsigned char> fileBuffer;
};

#endif // WEBSOCKETSERVER_H