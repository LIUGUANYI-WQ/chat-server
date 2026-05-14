#ifndef SERVICE_CHATSERVICE_H
#define SERVICE_CHATSERVICE_H

#include "../store/IChatStore.h"
#include "../store/IUserStore.h"
#include <string>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <muduo/base/Mutex.h>

namespace service {

// 消息回调类型
using MessageCallback = std::function<void(uint64_t uid, const std::string& roomId, const std::string& content)>;

class ChatService {
public:
    ChatService(store::IChatStore* chatStore, store::IUserStore* userStore);
    ~ChatService() = default;

    // 用户上线/下线
    void userOnline(uint64_t uid);
    void userOffline(uint64_t uid);

    // 房间操作
    bool joinRoom(uint64_t uid, const std::string& roomId);
    bool leaveRoom(uint64_t uid, const std::string& roomId);

    // 发送消息
    bool sendChatMessage(uint64_t senderUid, const std::string& roomId, const std::string& content);

    // 获取在线用户
    std::vector<uint64_t> getOnlineUsersInRoom(const std::string& roomId);

    // 获取历史消息
    std::vector<store::ChatMsg> getRecentMessages(const std::string& roomId, int count);

    // 设置消息回调（用于网络层发送）
    void setMessageCallback(MessageCallback callback);

private:
    store::IChatStore* chatStore_;
    store::IUserStore* userStore_;

    muduo::MutexLock mutex_;
    std::unordered_map<std::string, std::unordered_set<uint64_t>> roomMembers_;  // roomId -> uids
    MessageCallback messageCallback_;

    uint64_t nextMessageId_;
};

} // namespace service

#endif // SERVICE_CHATSERVICE_H