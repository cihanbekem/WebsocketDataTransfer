#include <libwebsockets.h>
#include <string.h>
#include <iostream>

class WebSocketServer {
public:
    WebSocketServer(int port) : port(port), interrupted(false) {
        memset(&info, 0, sizeof(info));
        info.port = port;
        info.protocols = protocols;
    }

    ~WebSocketServer() {
        lws_context_destroy(context);
    }

    bool start() {
        context = lws_create_context(&info);
        if (!context) {
            std::cerr << "lws_create_context failed\n";
            return false;
        }

        std::cout << "Server started on port " << port << std::endl;

        while (!interrupted) {
            lws_service(context, 1000);
        }
        return true;
    }

    void stop() {
        interrupted = true;
    }

private:
    static int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                             void *user, void *in, size_t len) {
        if (reason == LWS_CALLBACK_HTTP) {
            lws_return_http_status(wsi, HTTP_STATUS_OK, "WebSocket Server is Running");
            return -1;
        }
        return 0;
    }

    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len) {
        switch (reason) {
            case LWS_CALLBACK_ESTABLISHED:
                std::cout << "Client connected" << std::endl;
                break;
            case LWS_CALLBACK_RECEIVE:
                std::cout << "Received: " << (const char *)in << std::endl;
                lws_write(wsi, (unsigned char *)in, len, LWS_WRITE_TEXT);
                break;
            default:
                break;
        }
        return 0;
    }

    static const struct lws_protocols protocols[];

    struct lws_context_creation_info info;
    struct lws_context *context;
    int port;
    bool interrupted;

    
};

const struct lws_protocols WebSocketServer::protocols[] = {
    {"http-only", WebSocketServer::callback_http, 0, 0},
    {"websocket-protocol", WebSocketServer::callback_websockets, 0, 0},
    {NULL, NULL, 0, 0} /* terminator */
};



int main() {
    WebSocketServer server(8080);
    if (server.start()) {
        std::cout << "WebSocket Server started on port 8080\n";
    }
    return 0;
}
