#include "ChatServer.h"
#include "../protocol/message.h"
#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <arpa/inet.h>
#include <cstring>

namespace net {

ChatServer::ChatServer(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr,
                       service::AuthService* authService, service::ChatService* chatService,
                       store::IUserStore* userStore)
    : server_(loop, listenAddr, "ChatServer"),
      loop_(loop),
      authService_(authService),
      chatService_(chatService),
      userStore_(userStore),
      nextUid_(1) {

    server_.setConnectionCallback([this](const muduo::net::TcpConnectionPtr& conn) {
        this->onConnection(conn);
    });

    server_.setMessageCallback([this](const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp time) {
        this->onMessage(conn, buf, time);
    });
}

void ChatServer::start() {
    server_.start();
}

void ChatServer::onConnection(const muduo::net::TcpConnectionPtr& conn) {       
    if (conn->connected()) {
        LOG_INFO << "Client connected: " << conn->peerAddress().toIpPort();     

        uint64_t uid = nextUid_++;
        {
            muduo::MutexLockGuard lock(mutex_);
            connections_[uid] = conn;
            connToUid_[conn.get()] = uid;
        }
    } else {
        LOG_INFO << "Client disconnected: " << conn->peerAddress().toIpPort();  

        uint64_t uid = 0;
        {
            muduo::MutexLockGuard lock(mutex_);
            auto it = connToUid_.find(conn.get());
            if (it != connToUid_.end()) {
                uid = it->second;
                connections_.erase(uid);
                connToUid_.erase(it);
            }
        }

        if (uid > 0 && chatService_) {
            chatService_->userOffline(uid);
        }
    }
}

void ChatServer::onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp time) {
    // 长度头拆包：4字节大端长度 + payload，循环读出所有完整消息
    while (buf->readableBytes() >= 4) {
        uint32_t len;
        memcpy(&len, buf->peek(), 4);
        len = ntohl(len);
        if (buf->readableBytes() < 4 + len) break;  // 消息还没到齐，等下次
        buf->retrieve(4);
        std::string msgStr = buf->retrieveAsString(len);

        auto optMsg = protocol::decode(msgStr);
        if (!optMsg) {
            sendError(conn, "Invalid message format");
            continue;
        }

        protocol::Message& msg = *optMsg;

        switch (msg.header.type) {
            case protocol::MessageType::REGISTER:
                handleRegister(conn, msg);
                break;
            case protocol::MessageType::LOGIN:
                handleLogin(conn, msg);
                break;
            case protocol::MessageType::CHAT:
                handleChat(conn, msg);
                break;
            case protocol::MessageType::JOIN_ROOM:
                handleJoinRoom(conn, msg);
                break;
            case protocol::MessageType::LEAVE_ROOM:
                handleLeaveRoom(conn, msg);
                break;
            case protocol::MessageType::PRIVATE_CHAT:
                handlePrivateChat(conn, msg);
                break;
            case protocol::MessageType::ADD_FRIEND:
                handleAddFriend(conn, msg);
                break;
            case protocol::MessageType::HEARTBEAT:
                handleHeartbeat(conn, msg);
                break;
            default:
                sendError(conn, "Unknown message type");
                break;
        }
    }
}

void ChatServer::handleRegister(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    auto* body = static_cast<protocol::RegisterBody*>(msg.body.get());
    std::string username = body->username;
    std::string password = body->password;
    uint64_t seq = msg.header.seq;

    service::RegisterResult result = authService_->registerUser(username, password);

    if (result.success) {
        sendMessage(conn, protocol::createSystem(seq, 0, "Registration successful"));
    } else {
        sendError(conn, seq, result.code, result.message);
    }
}

void ChatServer::handleLogin(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    auto* body = static_cast<protocol::LoginBody*>(msg.body.get());
    std::string username = body->username;
    std::string password = body->password;
    uint64_t seq = msg.header.seq;

    uint64_t connUid = 0;
    {
        muduo::MutexLockGuard lock(mutex_);
        auto it = connToUid_.find(conn.get());
        if (it != connToUid_.end()) {
            connUid = it->second;
        }
    }

    if (connUid == 0) {
        sendError(conn, "Connection not registered");
        return;
    }

    service::LoginResult result = authService_->loginUser(username, password);  

    if (result.success) {
        LOG_INFO << "LOGIN OK: username=" << username << " realUid=" << result.uid
                 << " tempUid=" << connUid << " connections_.size=" << connections_.size();
        chatService_->userOnline(result.uid);
        {
            muduo::MutexLockGuard lock(mutex_);
            connToUid_[conn.get()] = result.uid;
            auto it = connections_.find(connUid);
            if (it != connections_.end()) {
                connections_.erase(it);
            }
            connections_[result.uid] = conn;
        }
        LOG_INFO << "After mapping: connections_.size=" << connections_.size();
        sendMessage(conn, protocol::createLoginResp(seq, 0, result.token, result.uid));
    } else {
        sendError(conn, seq, result.code, result.message);
    }
}

void ChatServer::handleChat(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    uint64_t uid = authenticate(conn, msg);
    if (uid == 0) {
        sendError(conn, "Please login first");
        return;
    }

    auto* body = static_cast<protocol::ChatBody*>(msg.body.get());
    std::string roomId = body->room_id;
    std::string content = body->content;

    // 业务层返回房间所有成员 uid，网络层负责找到对应连接并发出
    auto members = chatService_->sendChatMessage(uid, roomId, content);

    LOG_INFO << "handleChat: room=" << roomId << " has " << members.size()
             << " members, broadcasting...";

    broadcastToRoom(uid, roomId, members, content);

    sendMessage(conn, protocol::createSystem(msg.header.seq, 0, "Message sent"));
}

void ChatServer::handleJoinRoom(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    uint64_t uid = authenticate(conn, msg);
    if (uid == 0) {
        sendError(conn, "Please login first");
        return;
    }

    auto* body = static_cast<protocol::JoinRoomBody*>(msg.body.get());
    std::string roomId = body->room_id;

    LOG_INFO << "JOIN_ROOM: uid=" << uid << " room=" << roomId;
    if (chatService_->joinRoom(uid, roomId)) {
        LOG_INFO << "User " << uid << " joined room " << roomId;
        sendMessage(conn, protocol::createSystem(msg.header.seq, 0, "Joined room " + roomId));
    } else {
        sendError(conn, "Failed to join room");
    }
}

void ChatServer::handleLeaveRoom(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    uint64_t uid = authenticate(conn, msg);
    if (uid == 0) {
        sendError(conn, "Please login first");
        return;
    }

    auto* body = static_cast<protocol::LeaveRoomBody*>(msg.body.get());
    std::string roomId = body->room_id;

    if (chatService_->leaveRoom(uid, roomId)) {
        LOG_INFO << "User " << uid << " left room " << roomId;
        sendMessage(conn, protocol::createSystem(msg.header.seq, 0, "Left room " + roomId));
    } else {
        sendError(conn, "Failed to leave room");
    }
}

void ChatServer::handlePrivateChat(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    uint64_t uid = authenticate(conn, msg);
    if (uid == 0) {
        sendError(conn, "Please login first");
        return;
    }

    auto* body = static_cast<protocol::PrivateChatBody*>(msg.body.get());
    uint64_t targetUid = chatService_->sendPrivateMessage(uid, body->to_uid, body->content);

    if (targetUid > 0) {
        // 对方在线，找到连接直接发送
        muduo::net::TcpConnectionPtr targetConn;
        {
            muduo::MutexLockGuard lock(mutex_);
            auto it = connections_.find(targetUid);
            if (it != connections_.end()) targetConn = it->second;
        }

        if (targetConn && targetConn->connected()) {
            auto chatMsg = protocol::createPrivateChat(0, body->to_uid, body->content, "", uid);
            sendMessage(targetConn, chatMsg);
            sendMessage(conn, protocol::createSystem(msg.header.seq, 0, "Private message sent"));
            LOG_INFO << "Private chat delivered: " << uid << " → " << targetUid;
        } else {
            sendError(conn, "Target user disconnected");
        }
    } else {
        sendMessage(conn, protocol::createSystem(msg.header.seq, 0, "User offline, message stored"));
    }
}

void ChatServer::handleAddFriend(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    uint64_t uid = authenticate(conn, msg);
    if (uid == 0) {
        sendError(conn, "Please login first");
        return;
    }

    auto* body = static_cast<protocol::AddFriendBody*>(msg.body.get());
    if (chatService_->addFriend(uid, body->friend_uid)) {
        sendMessage(conn, protocol::createSystem(msg.header.seq, 0, "Friend added"));
        LOG_INFO << "Friend added: " << uid << " ↔ " << body->friend_uid;
    } else {
        sendError(conn, "Failed to add friend");
    }
}

void ChatServer::handleHeartbeat(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    sendMessage(conn, protocol::createHeartbeat(msg.header.seq, ""));
}

void ChatServer::sendMessage(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    std::string encoded = protocol::encode(msg);
    uint32_t len = htonl(static_cast<uint32_t>(encoded.size()));
    conn->send(std::string(reinterpret_cast<const char*>(&len), 4) + encoded);
}

void ChatServer::sendError(const muduo::net::TcpConnectionPtr& conn, const std::string& errorMsg) {
    sendMessage(conn, protocol::createError(0, 1, errorMsg));
}

void ChatServer::sendError(const muduo::net::TcpConnectionPtr& conn, uint64_t seq, int code, const std::string& errorMsg) {
    sendMessage(conn, protocol::createError(seq, code, errorMsg));
}

uint64_t ChatServer::authenticate(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    // 快速路径：当前连接已经做过认证
    {
        muduo::MutexLockGuard lock(mutex_);
        auto it = connToUid_.find(conn.get());
        if (it != connToUid_.end()) {
            return it->second;
        }
    }

    // 慢路径：通过 token 从 Redis 恢复会话（短连接 / nc 场景）
    if (msg.header.token.empty()) {
        return 0;
    }

    auto userOpt = userStore_->findByToken(msg.header.token);
    if (!userOpt.has_value()) {
        LOG_WARN << "Invalid token: " << msg.header.token.substr(0, 16) << "...";
        return 0;
    }

    uint64_t uid = userOpt->id;
    {
        muduo::MutexLockGuard lock(mutex_);
        connToUid_[conn.get()] = uid;
        connections_[uid] = conn;
    }

    // 标记在线（可能在之前的连接上已经标记过，重复 SADD 无影响）
    chatService_->userOnline(uid);

    LOG_INFO << "Authenticated user " << uid << " via token on connection "
             << conn->peerAddress().toIpPort();
    return uid;
}

void ChatServer::broadcastToRoom(uint64_t senderUid, const std::string& roomId,
                                  const std::vector<uint64_t>& members, const std::string& content) {
    LOG_INFO << "broadcastToRoom: room=" << roomId << " members=" << members.size()
             << " connections_.size=" << connections_.size();

    for (uint64_t memberUid : members) {
        muduo::net::TcpConnectionPtr memberConn;
        {
            muduo::MutexLockGuard lock(mutex_);
            auto it = connections_.find(memberUid);
            if (it != connections_.end()) {
                memberConn = it->second;
            }
        }

        if (memberConn && memberConn->connected()) {
            LOG_INFO << "  -> Sending to user " << memberUid;
            protocol::Message chatMsg = protocol::createChat(0, roomId, content, "", senderUid);
            sendMessage(memberConn, chatMsg);
        } else {
            LOG_WARN << "  -> User " << memberUid << " NOT in connections_ (found="
                     << (memberConn ? "yes" : "no")
                     << " connected=" << (memberConn && memberConn->connected() ? "yes" : "no")
                     << "), skipping";
        }
    }
}

} // namespace net