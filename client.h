#ifndef CLIENT_H
#define CLIENT_H

#include <libwebsockets.h>
#include <string>
#include <thread>
#include "student.pb.h"

class WebSocketClient {
public:
    WebSocketClient(const std::string& address, int port);
    ~WebSocketClient();
    bool connect();
    void stop();
    std::string getAddress() const;
    int getPort() const;

private:
    std::string address;
    int port;
    bool interrupted;
    static struct lws *wsi;
    lws_context_creation_info info;
    lws_context *context;

    void handleUserInput();
    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len);

    static const struct lws_protocols protocols[];
};

#endif // CLIENT_H
