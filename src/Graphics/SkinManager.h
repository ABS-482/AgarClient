#pragma once

#include <glad/glad.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class SkinManager
{
public:
    explicit SkinManager(int workerThreads = 2);
    ~SkinManager();

    SkinManager(const SkinManager&) = delete;
    SkinManager& operator=(const SkinManager&) = delete;

    // Не блокирует. Возвращает 0, пока текстура ещё не готова —
    // в этом случае запрос уже поставлен в очередь фоновой загрузки.
    GLuint getTexture(uint32_t skinId);

    // Вызывать РОВНО ОДИН РАЗ за кадр, из главного потока —
    // подгружает в GPU то, что успело скачаться/декодироваться в фоне.
    void processCompleted();

private:
    struct DecodedImage
    {
        uint32_t skinId = 0;
        int width = 0;
        int height = 0;
        std::vector<unsigned char> pixels; // RGBA8; пусто — значит ошибка
    };

    void workerLoop();
    std::string buildSkinUrl(uint32_t skinId) const;
    bool downloadSkin(uint32_t skinId, std::vector<unsigned char>& data);
    GLuint loadLocalTexture(const char* path);

    std::unordered_map<uint32_t, GLuint> m_textures; // готовые GL-текстуры
    std::unordered_set<uint32_t> m_requested;         // уже в очереди/скачивается

    std::mutex m_requestMutex;
    std::condition_variable m_requestCv;
    std::queue<uint32_t> m_requestQueue;

    std::mutex m_resultMutex;
    std::queue<DecodedImage> m_resultQueue;

    std::vector<std::thread> m_workers;
    std::atomic<bool> m_stopping{ false };
};