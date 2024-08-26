#ifndef SERVER_H
#define SERVER_H

#include <libwebsockets.h>
#include <vector>
#include "student.pb.h"

class WebSocketServer {
public:
    WebSocketServer(int port);
    ~WebSocketServer();
    bool start();
    void stop();

private:
    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len);
    lws_context_creation_info info;
    lws_context *context;
    int port;
    bool interrupted;

    static std::vector<unsigned char> fileBuffer;
    static const struct lws_protocols protocols[];
};

#endif // SERVER_H
