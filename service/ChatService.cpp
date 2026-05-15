#include "ChatService.h"
#include <muduo/base/Logging.h>

namespace service {

ChatService::ChatService(store::IChatStore* chatStore, store::IUserStore* userStore, muduo::net::EventLoop* loop)
    : chatStore_(chatStore), userStore_(userStore), loop_(loop), nextMessageId_(1) {}

void ChatService::userOnline(uint64_t uid) {
    chatStore_->setOnline(uid);
    LOG_INFO << "User " << uid << " is online";
}

void ChatService::userOffline(uint64_t uid) {
    chatStore_->setOffline(uid);

    muduo::MutexLockGuard lock(mutex_);
    for (auto& pair : roomMembers_) {
        pair.second.erase(uid);
    }

    LOG_INFO << "User " << uid << " is offline";
}

bool ChatService::joinRoom(uint64_t uid, const std::string& roomId) {
    muduo::MutexLockGuard lock(mutex_);
    roomMembers_[roomId].insert(uid);
    LOG_INFO << "User " << uid << " joined room " << roomId;
    return true;
}

bool ChatService::leaveRoom(uint64_t uid, const std::string& roomId) {
    muduo::MutexLockGuard lock(mutex_);
    auto it = roomMembers_.find(roomId);
    if (it != roomMembers_.end()) {
        it->second.erase(uid);
        if (it->second.empty()) {
            roomMembers_.erase(it);
        }
    }
    LOG_INFO << "User " << uid << " left room " << roomId;
    return true;
}

bool ChatService::sendChatMessage(uint64_t senderUid, const std::string& roomId, const std::string& content) {
    store::ChatMsg msg;
    msg.id = nextMessageId_++;
    msg.room_id = roomId;
    msg.sender_uid = senderUid;
    msg.content = content;
    msg.created_at = std::chrono::system_clock::now();
    chatStore_->cacheMessage(roomId, msg);

    DBExecutor::instance().asyncSaveChatMessage(msg, loop_, [](bool success, const std::string&) {
        if (!success) {
            LOG_ERROR << "Failed to save chat message to MySQL";
        }
    });

    std::unordered_set<uint64_t> members;
    {
        muduo::MutexLockGuard lock(mutex_);
        auto it = roomMembers_.find(roomId);
        if (it != roomMembers_.end()) {
            members = it->second;
        }
    }

    for (uint64_t uid : members) {
        if (messageCallback_) {
            messageCallback_(uid, roomId, content);
        }
    }

    LOG_INFO << "User " << senderUid << " sent message in room " << roomId;
    return true;
}

std::vector<uint64_t> ChatService::getOnlineUsersInRoom(const std::string& roomId) {
    std::vector<uint64_t> result;
    muduo::MutexLockGuard lock(mutex_);

    auto it = roomMembers_.find(roomId);
    if (it != roomMembers_.end()) {
        for (uint64_t uid : it->second) {
            if (chatStore_->isOnline(uid)) {
                result.push_back(uid);
            }
        }
    }

    return result;
}

std::vector<store::ChatMsg> ChatService::getRecentMessages(const std::string& roomId, int count) {
    return chatStore_->getRecentMessages(roomId, count);
}

void ChatService::setMessageCallback(MessageCallback callback) {
    messageCallback_ = std::move(callback);
}

}