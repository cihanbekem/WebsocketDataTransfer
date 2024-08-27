#ifndef CLIENT_H
#define CLIENT_H

#include <libwebsockets.h>
#include <string>
#include <atomic>
#include "student.pb.h"

class WebSocketClient {
public:
    WebSocketClient(const std::string& address, int port);
    ~WebSocketClient();

    bool connect();
    void stop();
    std::string getAddress() const;
    int getPort() const;

    // Publicly expose the interrupted flag for the thread to check
    std::atomic<bool> interrupted;

    // Make handleUserInput public to call it from main
    void handleUserInput();

    // Accessor for context (not typically recommended, consider alternatives)
    lws_context* getContext() const;

private:
    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len);

    static const struct lws_protocols protocols[];

    struct lws_context_creation_info info;
    struct lws_context *context;
    std::string address;
    int port;

    static struct lws *wsi;  // Static üye tanımı
};

#endif // CLIENT_H

// g++ main.cpp server.cpp client.cpp student.pb.cc -o my_program -lwebsockets -lprotobuf
// ./my_program