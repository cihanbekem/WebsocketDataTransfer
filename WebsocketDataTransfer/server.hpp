#ifndef SERVER_HPP
#define SERVER_HPP

#include <libwebsockets.h>
#include "student.pb.h"
#include <string>
#include <atomic>

/**
 * @class WebSocketServer
 * @brief WebSocket sunucusunu yöneten sınıf.
 *
 * Bu sınıf, bir WebSocket sunucusu başlatmak, durdurmak ve veri iletimi sağlamak için gerekli işlevleri sağlar.
 */
class WebSocketServer {
public:
    /**
     * @brief WebSocketServer sınıfının yapıcı fonksiyonu.
     *
     * @param port WebSocket sunucusunun dinleyeceği port numarası.
     */
    WebSocketServer(int port);

    /**
     * @brief WebSocketServer sınıfının yıkıcı fonksiyonu.
     *
     * WebSocketServer nesnesinin kaynaklarını serbest bırakır.
     */
    ~WebSocketServer();

    /**
     * @brief WebSocket sunucusunu başlatır.
     *
     * Sunucunun başlatılmasını ve bağlantıların kabul edilmesini sağlar.
     *
     * @return Başarıyla başlatıldıysa true, aksi halde false döner.
     */
    bool start();

    /**
     * @brief WebSocket sunucusunu durdurur.
     *
     * Sunucunun çalışmasını durdurur ve kaynakları temizler.
     */
    void stop();

    /**
     * @brief WebSocket bağlantı nesnesini döner.
     *
     * WebSocket sunucusunun WebSocket bağlantı nesnesini döner.
     *
     * @return WebSocket bağlantı nesnesi.
     */
    static struct lws* wsi; ///< WebSocket bağlantı nesnesi.

private:
    int port; ///< WebSocket sunucusunun dinleyeceği port numarası.
    std::atomic<bool> interrupted; ///< Kullanıcı tarafından bağlantının kesilip kesilmediğini belirten bayrak.
    lws_context* context; ///< WebSocket bağlamı.
    struct lws_context_creation_info info; ///< WebSocket bağlamı oluşturma bilgileri.

    /**
     * @brief Kullanıcıdan gelen girdileri işler.
     *
     * Kullanıcıdan gelen komutları alır ve uygun işlemleri gerçekleştirir.
     */
    void handleUserInput();

    /**
     * @brief Gelen komutu işler.
     * Verilen komutu işler ve uygun yanıtı üretir.
     * @param command İşlenecek komut.
     */
    void processCommand(const std::string& command);

    /**
     * @brief WebSocket üzerinden veri gönderir.
     *
     * Belirtilen veriyi WebSocket bağlantısı üzerinden gönderir.
     *
     * @param data Gönderilecek veri.
     */
    void sendData(const std::string& data);

    /**
     * @brief WebSocket geri çağırma işlevi.
     *
     * WebSocket olaylarını yönetir ve işleme alır.
     *
     * @param wsi WebSocket bağlantı nesnesi.
     * @param reason Olayın nedeni.
     * @param user Kullanıcıya özgü veri (bu işlevde kullanılmaz).
     * @param in Gelen veri.
     * @param len Gelen verinin uzunluğu.
     * @return 0 başarılıysa, diğer değerler hata kodlarıdır.
     */
    static int callback_websockets(struct lws* wsi, enum lws_callback_reasons reason,
                                   void* user, void* in, size_t len);

    /**
     * @brief WebSocket protokollerinin tanımlı olduğu dizi.
     *
     * WebSocket protokollerinin tanımlandığı ve geri çağırma işlevleri ile ilişkilendirildiği dizidir.
     */
    static const struct lws_protocols protocols[];
};

#endif // SERVER_HPP
