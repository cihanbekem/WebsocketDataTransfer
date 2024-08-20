#include <libwebsockets.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

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

                    // JSON dosyasına kaydet
                   json j;
j["file_content"] = fileContent;  // Tüm içeriği tek bir string olarak saklamak yerine:

// İçeriği satır satır ayırın ve her bir satırı ayrı bir dizi öğesi olarak saklayın.
vector<string> lines;
stringstream ss(fileContent);
string line;

while (getline(ss, line)) {
    lines.push_back(line);
}

j["file_content"] = lines;  // JSON'a dizi olarak ekleyin.

ofstream jsonFile("file_content.json");
jsonFile << j.dump(4);
jsonFile.close();

                    
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

            case LWS_CALLBACK_CLIENT_RECEIVE: {
    string receivedMessage((const char *)in, len);
    cout << "Received from server: " << receivedMessage << endl;

    // Mevcut JSON dosyasını aç ve oku
    ifstream inFile("received_data_from_server.json");
    json j;

    if (inFile) {
        inFile >> j;  // Eğer dosya zaten varsa ve içerik varsa onu oku
        inFile.close();
    }

    // Yeni mesajı satırlara böl
    vector<string> lines;
    stringstream ss(receivedMessage);
    string line;
    while (getline(ss, line)) {
        // Her satırı ayrı bir eleman olarak ekle
        lines.push_back(line);
    }

    // Yeni satırları JSON nesnesine ekle
    json newEntry;
    newEntry["received_message"] = lines;
    j.push_back(newEntry);

    // Güncellenmiş JSON'u dosyaya tekrar yaz
    ofstream outFile("received_data_from_server.json");
    outFile << j.dump(4) << endl;  // Güzel formatlanmış şekilde JSON'u yaz
    outFile.close();

    cout << "Text content written to file." << endl;

    lws_callback_on_writable(wsi);
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
