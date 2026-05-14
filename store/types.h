#ifndef STORE_TYPES_H
#define STORE_TYPES_H

#include <string>
#include <cstdint>
#include <vector>
#include <optional>
#include <chrono>

namespace store {

// 用户信息
struct User {
    uint64_t id;
    std::string username;
    std::string password;  // 哈希后的密码
    std::chrono::system_clock::time_point created_at;
};

// 聊天消息
struct ChatMsg {
    uint64_t id;
    std::string room_id;
    uint64_t sender_uid;
    std::string content;
    std::chrono::system_clock::time_point created_at;
};

} // namespace store

#endif // STORE_TYPES_H