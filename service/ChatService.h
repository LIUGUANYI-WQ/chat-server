#ifndef SERVICE_CHATSERVICE_H
#define SERVICE_CHATSERVICE_H

#include "../store/IChatStore.h"
#include "../store/IUserStore.h"
#include "../include/DBExecutor.h"
#include <string>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoop.h>

namespace service {

class ChatService {
public:
    ChatService(store::IChatStore* chatStore, store::IUserStore* userStore, muduo::net::EventLoop* loop);
    ~ChatService() = default;

    void userOnline(uint64_t uid);
    void userOffline(uint64_t uid);

    bool joinRoom(uint64_t uid, const std::string& roomId);
    bool leaveRoom(uint64_t uid, const std::string& roomId);

    // 发送群聊消息，返回房间内所有成员 uid 列表供网络层广播
    std::vector<uint64_t> sendChatMessage(uint64_t senderUid, const std::string& roomId, const std::string& content);

    // 发送私聊消息，返回目标 uid（0 表示对方不在线）
    uint64_t sendPrivateMessage(uint64_t fromUid, uint64_t toUid, const std::string& content);

    bool addFriend(uint64_t uid, uint64_t friendUid);

    std::vector<uint64_t> getOnlineUsersInRoom(const std::string& roomId);

    std::vector<store::ChatMsg> getRecentMessages(const std::string& roomId, int count);

private:
    store::IChatStore* chatStore_;
    store::IUserStore* userStore_;
    muduo::net::EventLoop* loop_;

    muduo::MutexLock mutex_;
    std::unordered_map<std::string, std::unordered_set<uint64_t>> roomMembers_;

    uint64_t nextMessageId_;
};

}

#endif