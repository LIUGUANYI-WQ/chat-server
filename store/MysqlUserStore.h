#ifndef STORE_MYSQLUSERSTORE_H
#define STORE_MYSQLUSERSTORE_H

#include "IUserStore.h"
#include <string>
#include <memory>

// 前置声明，避免引入mysql头文件到store接口层
class MySQLConnectionPool;

namespace store {

// MySQL用户存储实现
class MysqlUserStore : public IUserStore {
public:
    explicit MysqlUserStore(MySQLConnectionPool* pool);
    ~MysqlUserStore() override = default;

    std::optional<User> findByToken(const std::string& token) override;
    std::optional<User> findByUsername(const std::string& username) override;  
    bool insertUser(const User& user) override;
    void saveToken(const std::string& token, uint64_t uid) override;

private:
    MySQLConnectionPool* pool_;
};

} // namespace store

#endif // STORE_MYSQLUSERSTORE_H