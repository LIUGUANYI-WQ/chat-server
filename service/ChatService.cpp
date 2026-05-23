#include "ChatService.h"
#include "../include/DBExecutor.h"
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

std::vector<uint64_t> ChatService::sendChatMessage(uint64_t senderUid, const std::string& roomId, const std::string& content) {
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

    std::vector<uint64_t> members;
    {
        muduo::MutexLockGuard lock(mutex_);
        auto it = roomMembers_.find(roomId);
        if (it != roomMembers_.end()) {
            members.assign(it->second.begin(), it->second.end());
        }
    }

    LOG_INFO << "User " << senderUid << " sent message in room " << roomId
             << " → " << members.size() << " recipients";
    return members;
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

    LOG_INFO << "getOnlineUsersInRoom(" << roomId << ") found " << result.size() << " users";
    return result;
}

uint64_t ChatService::sendPrivateMessage(uint64_t fromUid, uint64_t toUid, const std::string& content) {
    store::ChatMsg msg;
    msg.id = nextMessageId_++;
    msg.room_id = "";  // 私聊无房间
    msg.sender_uid = fromUid;
    msg.content = content;
    msg.created_at = std::chrono::system_clock::now();
    chatStore_->cacheMessage("private:" + std::to_string(fromUid) + ":" + std::to_string(toUid), msg);

    DBExecutor::instance().asyncSaveChatMessage(msg, loop_, [](bool success, const std::string&) {
        if (!success) LOG_ERROR << "Failed to save private message to MySQL";
    });

    // 检查对方是否在线
    if (chatStore_->isOnline(toUid)) {
        LOG_INFO << "Private chat: " << fromUid << " → " << toUid << " (online)";
        return toUid;
    }

    LOG_INFO << "Private chat: " << fromUid << " → " << toUid << " (offline)";
    return 0;
}

bool ChatService::addFriend(uint64_t uid, uint64_t friendUid) {
    DBExecutor::instance().asyncAddFriend(uid, friendUid, loop_, [](bool success, const std::string& msg) {
        if (!success) LOG_ERROR << "Failed to add friend: " << msg;
    });

    LOG_INFO << "Friend added: " << uid << " ↔ " << friendUid;
    return true;
}

std::vector<store::ChatMsg> ChatService::getRecentMessages(const std::string& roomId, int count) {
    return chatStore_->getRecentMessages(roomId, count);
}

} // namespace service