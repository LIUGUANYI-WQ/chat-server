#include "RedisChatStore.h"
#include "RedisManager.h"
#include "json.hpp"
#include <muduo/base/Logging.h>
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace store {

using json = nlohmann::json;

RedisChatStore::RedisChatStore(RedisManager* redis) : redis_(redis) {
}

void RedisChatStore::setOnline(uint64_t uid) {
    if (!redis_) return;
    redis_->sadd("online_users", std::to_string(uid));
}

void RedisChatStore::setOffline(uint64_t uid) {
    if (!redis_) return;
    redis_->srem("online_users", std::to_string(uid));
}

bool RedisChatStore::isOnline(uint64_t uid) {
    if (!redis_) return false;
    return redis_->sismember("online_users", std::to_string(uid));
}

std::vector<uint64_t> RedisChatStore::getOnlineUsers(const std::string& room) {
    std::vector<uint64_t> result;
    if (!redis_) return result;

    auto members = redis_->smembers("online_users");
    for (const auto& m : members) {
        try {
            result.push_back(std::stoull(m));
        } catch (...) {
            // 忽略无效的数字
        }
    }

    return result;
}

void RedisChatStore::cacheMessage(const std::string& room, const ChatMsg& msg) {
    if (!redis_) return;

    // 序列化为JSON
    json j;
    j["id"] = msg.id;
    j["room_id"] = msg.room_id;
    j["sender_uid"] = msg.sender_uid;
    j["content"] = msg.content;

    std::time_t t = std::chrono::system_clock::to_time_t(msg.created_at);
    std::tm tm = *std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    j["created_at"] = ss.str();

    std::string key = "room:" + room + ":msgs";
    redis_->lpush(key, j.dump());
    redis_->ltrim(key, 0, 99);  // 保留最近100条
}

std::vector<ChatMsg> RedisChatStore::getRecentMessages(const std::string& room, int count) {
    std::vector<ChatMsg> result;
    if (!redis_) return result;

    std::string key = "room:" + room + ":msgs";
    auto messages = redis_->lrange(key, 0, count - 1);

    for (const auto& jsonStr : messages) {
        try {
            json j = json::parse(jsonStr);

            ChatMsg msg;
            msg.id = j["id"].get<uint64_t>();
            msg.room_id = j["room_id"].get<std::string>();
            msg.sender_uid = j["sender_uid"].get<uint64_t>();
            msg.content = j["content"].get<std::string>();

            std::tm tm = {};
            std::istringstream ss(j["created_at"].get<std::string>());
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            msg.created_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));

            result.push_back(msg);
        } catch (...) {
            // 忽略解析错误的消息
        }
    }

    return result;
}

} // namespace store