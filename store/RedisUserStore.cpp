#include "RedisUserStore.h"
#include "RedisManager.h"
#include <muduo/base/Logging.h>
#include <cstdint>

namespace store {

RedisUserStore::RedisUserStore(RedisManager* redis, IUserStore* mysqlStore)
    : redis_(redis), mysqlStore_(mysqlStore) {
}

std::optional<User> RedisUserStore::findByToken(const std::string& token) {
    if (!redis_) {
        return std::nullopt;
    }

    std::string uidStr = redis_->get("token:" + token);
    if (uidStr.empty()) {
        return std::nullopt;
    }

    uint64_t uid = std::stoull(uidStr);
    // 这里我们简化，只返回带uid的User，或者需要从MySQL获取完整信息
    User user;
    user.id = uid;
    // 真实项目中可能还需要从MySQL获取username等，但这里我们简化
    return user;
}

std::optional<User> RedisUserStore::findByUsername(const std::string& username) {
    // 委托给MySQL实现
    if (mysqlStore_) {
        return mysqlStore_->findByUsername(username);
    }
    return std::nullopt;
}

bool RedisUserStore::insertUser(const User& user) {
    // 委托给MySQL实现
    if (mysqlStore_) {
        return mysqlStore_->insertUser(user);
    }
    return false;
}

void RedisUserStore::saveToken(const std::string& token, uint64_t uid) {
    if (!redis_) {
        return;
    }

    std::string key = "token:" + token;
    redis_->set(key, std::to_string(uid), 3600);  // 1小时过期
}

} // namespace store