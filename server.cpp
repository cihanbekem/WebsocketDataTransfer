#include <libwebsockets.h>
#include <string.h>
#include <iostream>
using namespace std;

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
            cerr << "lws_create_context failed\n";
            return false;
        }

        cout << "Server started on port " << port << endl;

        while (!interrupted) {
            lws_service(context, 1000);

            // Eğer client bağlanmışsa mesaj gönder
            if (wsi) {
                string message;
                cout << "Enter a message to send to the client: ";
                getline(cin, message);

                if (!message.empty()) {
                    unsigned char buffer[LWS_PRE + message.size()];
                    memset(buffer, 0, sizeof(buffer));  // Buffer'ı sıfırlama
                    memcpy(&buffer[LWS_PRE], message.c_str(), message.size());
                    lws_write(wsi, &buffer[LWS_PRE], message.size(), LWS_WRITE_TEXT);
                    lws_callback_on_writable(wsi);  // Serverın tekrar yazılabilir hale gelmesini sağlar
                }
            }
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
                cout << "Client connected" << endl;
                WebSocketServer::wsi = wsi;  // Client bağlantısını kaydet
                break;

            case LWS_CALLBACK_RECEIVE: {
                cout << "Received: " << string((const char *)in, len) << endl;

                // Mesajı buffer'a yaz
                unsigned char buffer[LWS_PRE + len];
                memset(buffer, 0, sizeof(buffer));  // Buffer'ı sıfırlama

                memcpy(&buffer[LWS_PRE], in, len);

                // Mesajı geri gönder
                lws_write(wsi, &buffer[LWS_PRE], len, LWS_WRITE_TEXT);

                lws_callback_on_writable(wsi);  // Serverın tekrar yazılabilir hale gelmesini sağlar    
                break;
            }
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

    static struct lws *wsi;  // Bağlı olan istemcinin bağlantısı
};

struct lws *WebSocketServer::wsi = nullptr;  // Static üye değişkeni tanımlaması

const struct lws_protocols WebSocketServer::protocols[] = {
    {"http-only", WebSocketServer::callback_http, 0, 0},
    {"websocket-protocol", WebSocketServer::callback_websockets, 0, 0},
    {NULL, NULL, 0, 0} /* terminator */
};

int main() {
    WebSocketServer server(8080);
    if (server.start()) {
        cout << "WebSocket Server started on port 8080\n";
    }
    return 0;
}
