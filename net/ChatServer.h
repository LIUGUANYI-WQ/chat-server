#ifndef SERVICE_AUTHSERVICE_H
#define SERVICE_AUTHSERVICE_H

#include "../store/IUserStore.h"
#include <string>
#include <cstdint>

namespace service {

// 注册结果
struct RegisterResult {
    bool success;
    int code;
    std::string message;
};

// 登录结果
struct LoginResult {
    bool success;
    int code;
    std::string token;
    uint64_t uid;
};

class AuthService {
public:
    explicit AuthService(store::IUserStore* userStore);
    ~AuthService() = default;

    RegisterResult registerUser(const std::string& username, const std::string& password);
    LoginResult loginUser(const std::string& username, const std::string& password);

private:
    store::IUserStore* userStore_;

    std::string hashPassword(const std::string& password);
    std::string generateToken(const std::string& username);
};

} // namespace service

#endif // SERVICE_AUTHSERVICE_H