#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <fstream>
#include <vector>

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

        thread inputThread(&WebSocketServer::handleUserInput, this);

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
            if (wsi) {
                string command;
                cout << "Enter 'send' to send a file, 'message' to send a message, or 'exit' to quit: ";
                getline(cin, command);

                if (command == "send") {
                    string filePath;
                    cout << "Enter the path of the file to send: ";
                    getline(cin, filePath);

                    ifstream file(filePath, ios::binary | ios::ate);
                    if (!file.is_open()) {
                        cerr << "Failed to open file." << endl;
                        continue;
                    }

                    streambuf* sbuf = file.rdbuf();
                    size_t size = sbuf->pubseekoff(0, ios::end, ios::in);
                    sbuf->pubseekpos(0, ios::in);

                    string fileContent(size, '\0');
                    sbuf->sgetn(&fileContent[0], size);
                    file.close();

                    unsigned char buffer[LWS_PRE + fileContent.size()];
                    memcpy(&buffer[LWS_PRE], fileContent.c_str(), fileContent.size());
                    lws_write(wsi, &buffer[LWS_PRE], fileContent.size(), LWS_WRITE_TEXT);
                } else if (command == "message") {
                    string message;
                    cout << "Enter a message to send to the client: ";
                    getline(cin, message);

                    if (!message.empty()) {
                        unsigned char buffer[LWS_PRE + message.size()];
                        memcpy(&buffer[LWS_PRE], message.c_str(), message.size());
                        lws_write(wsi, &buffer[LWS_PRE], message.size(), LWS_WRITE_TEXT);
                    }
                } else if (command == "exit") {
                    stop();
                }
            }
        }
    }

    static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len) {
        static vector<unsigned char> fileBuffer;

        switch (reason) {
            case LWS_CALLBACK_ESTABLISHED:
                cout << "Client connected" << endl;
                WebSocketServer::wsi = wsi;  // Bağlantıyı kaydet
                break;

            case LWS_CALLBACK_RECEIVE: {
                // Veriyi dosyaya yazma
                fileBuffer.insert(fileBuffer.end(), (unsigned char*)in, (unsigned char*)in + len);

                // Dosyayı yazma
                ofstream outFile("received_file_from_client.txt", ios::binary | ios::app);
                if (outFile.is_open()) {
                    outFile.write(reinterpret_cast<const char*>(fileBuffer.data()), fileBuffer.size());
                    fileBuffer.clear();
                    outFile.close();
                    cout << "File successfully written to 'received_file_from_client.txt'" << endl;
                } else {
                    cerr << "Failed to open file for writing." << endl;
                }

                // Başarıyla dosyanın açıldığını istemciye bildirme
                string successMessage = "Successfully opened";
                unsigned char buffer[LWS_PRE + successMessage.size()];
                memcpy(&buffer[LWS_PRE], successMessage.c_str(), successMessage.size());
                lws_write(wsi, &buffer[LWS_PRE], successMessage.size(), LWS_WRITE_TEXT);

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
    static vector<unsigned char> fileBuffer;  // Alınan dosya verilerini saklar
};

struct lws *WebSocketServer::wsi = nullptr;
vector<unsigned char> WebSocketServer::fileBuffer;

const struct lws_protocols WebSocketServer::protocols[] = {
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
