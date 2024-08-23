#ifndef SERVER_H
#define SERVER_H

#include <libwebsockets.h>
#include <vector>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

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
    static std::vector<unsigned char> fileBuffer;
};

#endif // SERVER_H