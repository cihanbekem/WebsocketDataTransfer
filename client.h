#ifndef CLIENT_H
#define CLIENT_H

#include <libwebsockets.h>
#include <string>
#include "student.pb.h"
//#include "client.h"

class WebSocketClient {
public:
    WebSocketClient(const std::string& address, int port);
    ~WebSocketClient();

    bool connect();
    void stop();
    std::string getAddress() const;
    int getPort() const;

private:
    void handleUserInput();
    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len);

    static const struct lws_protocols protocols[];

    struct lws_context_creation_info info;
    struct lws_context *context;
    std::string address;
    int port;
    bool interrupted;

    static struct lws *wsi;
};

#endif // CLIENT_H