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
    static struct lws* wsi;

private:
    int m_port;
    std::atomic<bool> interrupted;
    lws_context* context;
    struct lws_context_creation_info info;

    

    void handleUserInput();
    void processCommand(const std::string& command);
    void sendData(const std::string& data);
    static int callback_websockets(struct lws* wsi, enum lws_callback_reasons reason,
                                   void* user, void* in, size_t len);

    static const struct lws_protocols protocols[];
};

#endif // SERVER_H