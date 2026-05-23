#ifndef STORE_ICHATSTORE_H
#define STORE_ICHATSTORE_H

#include "types.h"
#include <string>
#include <vector>
#include <cstdint>

namespace store {

// 聊天存储接口
class IChatStore {
public:
    virtual ~IChatStore() = default;

    // 设置用户在线
    virtual void setOnline(uint64_t uid) = 0;

    // 设置用户离线
    virtual void setOffline(uint64_t uid) = 0;

    // 检查用户是否在线
    virtual bool isOnline(uint64_t uid) = 0;

    // 获取房间内的在线用户
    virtual std::vector<uint64_t> getOnlineUsers(const std::string& room) = 0;

    // 缓存聊天消息
    virtual void cacheMessage(const std::string& room, const ChatMsg& msg) = 0;

    // 获取最近的消息
    virtual std::vector<ChatMsg> getRecentMessages(const std::string& room, int count) = 0;
};

} // namespace store

#endif // STORE_ICHATSTORE_H