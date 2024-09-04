#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <libwebsockets.h>
#include "student.pb.h"
#include <string>
#include <atomic>

/**
 * @class WebSocketClient
 * @brief WebSocket istemci sınıfı.
 *
 * Bu sınıf, WebSocket sunucusuna bağlanmak, veri göndermek ve kullanıcı girdilerini işlemek için kullanılan işlevleri sağlar.
 */
class WebSocketClient {
public:
    /**
     * @brief Constructor fonksiyon.
     *
     * WebSocketClient nesnesini verilen adres ve port ile başlatır.
     *
     * @param address Bağlanılacak WebSocket sunucusunun adresi.
     * @param port Bağlanılacak WebSocket sunucusunun portu.
     */
    WebSocketClient(const std::string& address, int port);

    /**
     * @brief Deconstructor fonksiyon. 
     */
    ~WebSocketClient();

    /**
     * @brief WebSocket sunucusuna bağlanır.
     *
     * WebSocket bağlantısını başlatır.
     *
     * @return Bağlantı başarılıysa true, değilse false döner.
     */
    bool connect();

    /**
     * @brief WebSocket bağlantısını durdurur.
     *
     * WebSocket bağlantısını sonlandırır ve temizler.
     */
    void stop();

    /**
     * @brief Kullanıcı girdilerini işler.
     *
     * Kullanıcıdan komutları alır ve uygun işlemi gerçekleştirir.
     */
    void handleUserInput();

    /**
     * @brief Protobuf verisinin tamamlanıp tamamlanmadığını kontrol eder.
     *
     * @param data Kontrol edilecek veri.
     * @return Veri tamamlanmışsa true, tamamlanmamışsa false döner.
     */
    static bool isCompleteProtobuf(const std::string& data);

    /**
     * @brief JSON verisinin tamamlanıp tamamlanmadığını kontrol eder.
     *
     * @param data Kontrol edilecek veri.
     * @return Veri tamamlanmışsa true, aksi halde false döner.
     */
    static bool isCompleteJson(const std::string& data);

    std::atomic<bool> interrupted; 

    /**
     * @brief WebSocket bağlamını alır.
     * @return WebSocket bağlamı.
     */
    lws_context* getContext() const;

private:
    /**
     * @brief WebSocket geri çağırma fonksiyonu.
     *
     * @param wsi WebSocket bağlantı nesnesi.
     * @param reason Olayın nedeni.
     * @param user Kullanıcıya özgü veri (bu işlev için kullanılmaz).
     * @param in Gelen veri.
     * @param len Gelen verinin uzunluğu.
     * @return 0 her şey yolundaysa, diğer değerler hata kodlarıdır.
     */
    static int callback_websockets(struct lws* wsi, enum lws_callback_reasons reason,
                                   void* user, void* in, size_t len);

    static const struct lws_protocols protocols[]; ///< WebSocket protokollerinin listesi.

    struct lws_context_creation_info info; ///< WebSocket bağlamı oluşturma bilgileri.
    struct lws_context *context; ///< WebSocket bağlamı.
    std::string address; ///< WebSocket sunucusunun adresi.
    int m_port; ///< WebSocket sunucusunun portu.

    static struct lws *wsi; ///< WebSocket bağlantı nesnesi.
    static bool is_json; ///< JSON formatı bayrağı.
    static std::string accumulatedData; ///< Biriken veri.
};

#endif // CLIENT_HPP
