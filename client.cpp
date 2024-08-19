#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <thread>

using namespace std;

class WebSocketClient {
public:
    WebSocketClient(const string& address, int port)
        : address(address), port(port), interrupted(false) {
        memset(&info, 0, sizeof(info));
        info.protocols = protocols;
    }

    ~WebSocketClient() {
        lws_context_destroy(context);
    }

    bool connect() {
        context = lws_create_context(&info);
        if (!context) {
            cout << "lws_create_context failed\n";
            return false;
        }

        struct lws_client_connect_info ccinfo = {0};
        ccinfo.context = context;
        ccinfo.address = address.c_str();
        ccinfo.port = port;
        ccinfo.path = "/";
        ccinfo.protocol = protocols[0].name;
        ccinfo.host = lws_canonical_hostname(context);
        ccinfo.origin = "origin";
        ccinfo.ietf_version_or_minus_one = -1;

        wsi = lws_client_connect_via_info(&ccinfo);
        if (!wsi) {
            cout << "lws_client_connect_via_info failed\n";
            return false;
        }

        thread inputThread(&WebSocketClient::handleUserInput, this);

        while (!interrupted) {
            lws_service(context, 1000);
        }

        inputThread.join();
        return true;
    }

    void stop() {
        interrupted = true;
    }

private:
    void handleUserInput() {
        while (!interrupted) {
            string message;
            cout << "Enter a message to send to the server: ";
            cout << endl;
            getline(cin, message);

            if (!message.empty()) {
                unsigned char buffer[LWS_PRE + message.size()];
                memcpy(&buffer[LWS_PRE], message.c_str(), message.size());
                lws_write(wsi, &buffer[LWS_PRE], message.size(), LWS_WRITE_TEXT);
                lws_callback_on_writable(wsi);
            }
        }
    }

    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len) {
        switch (reason) {
            case LWS_CALLBACK_CLIENT_ESTABLISHED:
                cout << "WebSocket Client connected\n";
                lws_callback_on_writable(wsi);  // Client'ın tekrar yazılabilir hale gelmesini sağlar
                break;

            case LWS_CALLBACK_CLIENT_RECEIVE:
                string receivedMessage((const char *)in, len);  // Mesajı doğru şekilde işle
                cout << "Received from server: " << receivedMessage << endl;
                lws_callback_on_writable(wsi);  // Client'ın tekrar yazılabilir hale gelmesini sağlar
                break;

            /*default:
                break;*/
        }
        return 0;
    }

    static const struct lws_protocols protocols[];

    struct lws_context_creation_info info;
    struct lws_context *context;
    struct lws *wsi;
    std::string address;
    int port;
    bool interrupted;
};

const struct lws_protocols WebSocketClient::protocols[] = {
    {"websocket-protocol", WebSocketClient::callback_websockets, 0, 0},
    {NULL, NULL, 0, 0} /* terminator */
};

int main() {
    WebSocketClient client("localhost", 8080);
    if (client.connect()) {
        cout << "WebSocket Client connected to server\n";
    }
    return 0;
}