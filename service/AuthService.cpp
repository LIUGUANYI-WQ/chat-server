#include "AuthService.h"
#include "../include/RedisManager.h"
#include <random>
#include <sstream>
#include <muduo/base/Logging.h>

namespace service {

AuthService::AuthService(store::IUserStore* userStore)
    : userStore_(userStore) {
}

RegisterResult AuthService::registerUser(const std::string& username, const std::string& password) {
    RegisterResult result;

    if (username.empty() || password.empty()) {
        result.success = false;
        result.code = 400;
        result.message = "Username and password cannot be empty";
        return result;
    }

    auto existingUser = userStore_->findByUsername(username);
    if (existingUser.has_value()) {
        result.success = false;
        result.code = 409;
        result.message = "Username already exists";
        return result;
    }

    store::User user;
    user.username = username;
    user.password = hashPassword(password);
    user.created_at = std::chrono::system_clock::now();

    if (userStore_->insertUser(user)) {
        result.success = true;
        result.code = 200;
        result.message = "Registration successful";
    } else {
        result.success = false;
        result.code = 500;
        result.message = "Registration failed";
    }

    return result;
}

LoginResult AuthService::loginUser(const std::string& username, const std::string& password) {
    LoginResult result;

    if (username.empty() || password.empty()) {
        result.success = false;
        result.code = 400;
        result.message = "Username and password cannot be empty";
        result.token = "";
        result.uid = 0;
        return result;
    }

    auto userOpt = userStore_->findByUsername(username);
    if (!userOpt.has_value()) {
        result.success = false;
        result.code = 401;
        result.message = "User not found";
        result.token = "";
        result.uid = 0;
        return result;
    }

    auto& user = userOpt.value();
    std::string hashedInputPwd = hashPassword(password);
    
    bool passwordMatch = (user.password == hashedInputPwd) || (user.password == password);
    
    if (!passwordMatch) {
        result.success = false;
        result.code = 401;
        result.message = "Invalid password";
        result.token = "";
        result.uid = 0;
        return result;
    }

    std::string token = generateToken(username);
    userStore_->saveToken(token, user.id);

    result.success = true;
    result.code = 200;
    result.message = "Login successful";
    result.token = token;
    result.uid = user.id;

    return result;
}

std::string AuthService::hashPassword(const std::string& password) {
    std::string hashed = "hashed_" + password;
    return hashed;
}

std::string AuthService::generateToken(const std::string& username) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream token;
    token << username << "_";
    for (int i = 0; i < 32; ++i) {
        token << std::hex << dis(gen);
    }

    return token.str();
}

} // namespace service