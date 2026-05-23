#ifndef STORE_REDISUSERSTORE_H
#define STORE_REDISUSERSTORE_H

#include "IUserStore.h"
#include <string>
#include <memory>

class RedisManager;

namespace store {

// Redis用户存储实现（主要用于token缓存）
class RedisUserStore : public IUserStore {
public:
    explicit RedisUserStore(RedisManager* redis, IUserStore* mysqlStore);
    ~RedisUserStore() override = default;

    std::optional<User> findByToken(const std::string& token) override;
    std::optional<User> findByUsername(const std::string& username) override;
    bool insertUser(const User& user) override;
    void saveToken(const std::string& token, uint64_t uid) override;

private:
    RedisManager* redis_;
    IUserStore* mysqlStore_;  // 委托MySQL实现来做持久化
};

} // namespace store

#endif // STORE_REDISUSERSTORE_H