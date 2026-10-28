#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <unordered_set>

class UserSkinDatabase final
{
public:
    // proxyMode: "", "rus" или "cf".
    explicit UserSkinDatabase(std::string proxyMode = {});
    ~UserSkinDatabase();

    UserSkinDatabase(const UserSkinDatabase&) = delete;
    UserSkinDatabase& operator=(const UserSkinDatabase&) = delete;

    // true, когда первоначальная загрузка завершена.
    bool isLoaded() const noexcept
    {
        return m_loaded.load(std::memory_order_acquire);
    }

    // Ищет персональный скин, затем клановый.
    // Возвращает nullopt, если скин не найден.
    std::optional<uint32_t> findSkinId(
        std::string_view nickname
    ) const;

    // Повторяет проверку UserDatabase.isNickWithPassword().
    bool requiresPassword(std::string_view nickname) const;

    // Проверяет, должен ли ник быть скрыт.
    bool isInvisibleNick(
        std::string_view nickname
    ) const;

    std::size_t personalSkinCount() const;
    std::size_t clanSkinCount() const;

private:
    void loadWorker();

    bool downloadTable(
        const std::string& path,
        std::string& contents
    );

    bool httpGet(
        const std::wstring& host,
        const std::wstring& path,
        std::string& contents
    );

    std::wstring buildDataHost(int number) const;

    static std::string normalizeNick(
        std::string_view nickname
    );

    static std::unordered_map<std::string, uint32_t>
        parsePersonalTable(const std::string& contents);

    static std::vector<std::pair<std::string, uint32_t>>
        parseClanTable(const std::string& contents);

    static std::vector<std::string> parseNamesFromIdTable(
        const std::string& contents
    );

    static std::vector<std::string> parseInvisibleNickList(
        const std::string& contents
    );

    std::string m_proxyMode;

    std::thread m_worker;

    std::atomic<bool> m_stopping{ false };
    std::atomic<bool> m_loaded{ false };

    mutable std::shared_mutex m_mutex;

    // Ник -> skinId.
    std::unordered_map<std::string, uint32_t> m_personalSkins;

    // Префикс клана -> skinId.
    std::vector<std::pair<std::string, uint32_t>> m_clanSkins;

    // Ники и клановые префиксы, которые в Java помечены hasPassword.
    std::unordered_set<std::string> m_passwordProtectedNicks;
    std::vector<std::string> m_passwordProtectedClanPrefixes;

    // Ники, для которых Java устанавливает invisibleNick = true.
    std::unordered_set<std::string> m_invisibleNicks;
};