#ifndef SERVICE_CHATSERVICE_H
#define SERVICE_CHATSERVICE_H

#include ../store/IChatStore.h
#include ../store/IUserStore.h
#include ../include/DBExecutor.h
#include <string>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <muduo/base/Mutex.h>

namespace service {

using MessageCallback = std::function<void(uint64_t uid, const std::string& roomId, const std::string& content)>;

class ChatService {
public:
    ChatService(store::IChatStore* chatStore, store::IUserStore* userStore, muduo::net::EventLoop* loop);
    ~ChatService() = default;

    void userOnline(uint64_t uid);
    void userOffline(uint64_t uid);

    bool joinRoom(uint64_t uid, const std::string& roomId);
    bool leaveRoom(uint64_t uid, const std::string& roomId);

    bool sendChatMessage(uint64_t senderUid, const std::string& roomId, const std::string& content);

    std::vector<uint64_t> getOnlineUsersInRoom(const std::string& roomId);

    std::vector<store::ChatMsg> getRecentMessages(const std::string& roomId, int count);

    void setMessageCallback(MessageCallback callback);

private:
    store::IChatStore* chatStore_;
    store::IUserStore* userStore_;
    muduo::net::EventLoop* loop_;

    muduo::MutexLock mutex_;
    std::unordered_map<std::string, std::unordered_set<uint64_t>> roomMembers_;
    MessageCallback messageCallback_;

    uint64_t nextMessageId_;
};

}

#endif
