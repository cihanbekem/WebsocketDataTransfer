#ifndef SERVER_H
#define SERVER_H

#include <libwebsockets.h>
#include "student.pb.h"
#include <string>

class WebSocketServer {
public:
    WebSocketServer(int port);
    ~WebSocketServer();

    bool start();
    void stop();
    static WebSocketServer* createServer(int port); // Burada eklediğiniz fonksiyon bildirimi
    // Diğer metodlar

private:
    // Üye değişkenler
    int port;
    bool interrupted;
    lws_context* context;
    struct lws_context_creation_info info;

    static struct lws* wsi;

    void handleUserInput();
    static int callback_websockets(struct lws* wsi, enum lws_callback_reasons reason,
                                   void* user, void* in, size_t len);

    static const struct lws_protocols protocols[];
};

#endif // SERVER_H
