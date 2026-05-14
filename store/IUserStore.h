#ifndef STORE_IUSERSTORE_H
#define STORE_IUSERSTORE_H

#include "types.h"
#include <string>
#include <optional>

namespace store {

// 用户存储接口
class IUserStore {
public:
    virtual ~IUserStore() = default;

    // 通过token查找用户
    virtual std::optional<User> findByToken(const std::string& token) = 0;

    // 通过用户名查找用户
    virtual std::optional<User> findByUsername(const std::string& username) = 0;

    // 插入用户（返回是否成功）
    virtual bool insertUser(const User& user) = 0;

    // 保存token对应用户的映射（用于缓存）
    virtual void saveToken(const std::string& token, uint64_t uid) = 0;
};

} // namespace store

#endif // STORE_IUSERSTORE_H