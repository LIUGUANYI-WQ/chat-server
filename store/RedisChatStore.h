#ifndef STORE_REDISCHATSTORE_H
#define STORE_REDISCHATSTORE_H

#include "IChatStore.h"
#include <string>
#include <memory>

class RedisManager;

namespace store {

class RedisChatStore : public IChatStore {
public:
    explicit RedisChatStore(RedisManager* redis);
    ~RedisChatStore() override = default;

    void setOnline(uint64_t uid) override;
    void setOffline(uint64_t uid) override;
    bool isOnline(uint64_t uid) override;
    std::vector<uint64_t> getOnlineUsers(const std::string& room) override;
    void cacheMessage(const std::string& room, const ChatMsg& msg) override;
    std::vector<ChatMsg> getRecentMessages(const std::string& room, int count) override;

private:
    RedisManager* redis_;
};

} // namespace store

#endif // STORE_REDISCHATSTORE_H