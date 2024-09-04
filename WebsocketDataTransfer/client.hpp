#ifndef CLIENT_H
#define CLIENT_H

#include <libwebsockets.h>
#include "student.pb.h"
#include <string>
#include <atomic>

class WebSocketClient {
public:
    WebSocketClient(const std::string& address, int port);
    ~WebSocketClient();

    bool connect();
    void stop();
    void handleUserInput();
    static bool isCompleteProtobuf(const std::string& data);
    static bool isCompleteJson(const std::string& data);

    std::atomic<bool> interrupted;
    lws_context* getContext() const; // Bu fonksiyonun bildirimi burada olmalı

private:
    static int callback_websockets(struct lws* wsi, enum lws_callback_reasons reason,
                                   void* user, void* in, size_t len);

    static const struct lws_protocols protocols[];

    struct lws_context_creation_info info;
    struct lws_context *context;
    std::string address;
    int m_port;

    static struct lws *wsi;
    static bool is_json;
    static std::string accumulatedData;

};

#endif // CLIENT_H