#ifndef SERVER_H
#define SERVER_H

#include <libwebsockets.h>
#include "student.pb.h"
#include <string>
#include <atomic>

class WebSocketServer {
public:
    WebSocketServer(int port);
    ~WebSocketServer();

    bool start();
    void stop();

private:
    int port;
    std::atomic<bool> interrupted;
    lws_context* context;
    struct lws_context_creation_info info;

    static struct lws* wsi;

    void handleUserInput();
    void processCommand(const std::string& command);
    static int callback_websockets(struct lws* wsi, enum lws_callback_reasons reason,
                                   void* user, void* in, size_t len);

    static const struct lws_protocols protocols[];
};

#endif // SERVER_H
