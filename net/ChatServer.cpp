#include "ChatServer.h"
#include <muduo/base/Logging.h>

namespace net {

ChatServer::ChatServer(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr,
                       service::AuthService* authService, service::ChatService* chatService)
    : server_(loop, listenAddr, "ChatServer"), loop_(loop), authService_(authService),
      chatService_(chatService), nextUid_(1) {
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    chatService_->setMessageCallback([this](uint64_t uid, const std::string& roomId, const std::string& content) {
        muduo::net::TcpConnectionPtr conn;
        {
            muduo::MutexLockGuard lock(mutex_);
            auto it = connections_.find(uid);
            if (it != connections_.end()) {
                conn = it->second;
            }
        }

        if (conn && conn->connected()) {
            protocol::Message msg;
            msg.header.type = protocol::MessageType::CHAT;
            msg.body["room_id"] = roomId;
            msg.body["content"] = content;
            sendMessage(conn, msg);
        }
    });
}

void ChatServer::start() {
    server_.start();
}

void ChatServer::onConnection(const muduo::net::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        LOG_INFO << "New connection: " << conn->peerAddress().toIpPort();
    } else {
        LOG_INFO << "Connection closed: " << conn->peerAddress().toIpPort();

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

        if (uid > 0) {
            chatService_->userOffline(uid);
        }
    }
}

void ChatServer::onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp time) {
    while (buf->readableBytes() >= 4) {
        const void* data = buf->peek();
        uint32_t len = muduo::net::sockets::networkToHost32(*static_cast<const uint32_t*>(data));

        if (len > 65536) {
            LOG_ERROR << "Message too long: " << len;
            conn->shutdown();
            return;
        }

        if (buf->readableBytes() < len + 4) {
            break;
        }

        std::string msgStr(buf->peek() + 4, len);
        buf->retrieve(len + 4);

        protocol::Message msg;
        if (!protocol::decode(msgStr, msg)) {
            LOG_ERROR << "Failed to decode message";
            sendError(conn, "Invalid message format");
            continue;
        }

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
            case protocol::MessageType::HEARTBEAT:
                handleHeartbeat(conn, msg);
                break;
            default:
                LOG_WARN << "Unknown message type: " << static_cast<int>(msg.header.type);
                sendError(conn, "Unknown message type");
                break;
        }
    }
}

void ChatServer::handleRegister(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    std::string username = msg.body["username"].get<std::string>();
    std::string password = msg.body["password"].get<std::string>();

    authService_->asyncRegister(username, password, loop_, [this, conn, msg](bool success, const std::string& message) {
        protocol::Message reply;
        reply.header.type = protocol::MessageType::SYSTEM;
        reply.body["success"] = success;
        reply.body["message"] = message;
        sendMessage(conn, reply);
    });
}

void ChatServer::handleLogin(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    std::string username = msg.body["username"].get<std::string>();
    std::string password = msg.body["password"].get<std::string>();

    authService_->asyncLogin(username, password, loop_, [this, conn, msg](bool success, const std::string& message) {
        protocol::Message reply;
        if (success) {
            reply.header.type = protocol::MessageType::LOGIN_RESP;

            uint64_t uid;
            {
                muduo::MutexLockGuard lock(mutex_);
                uid = nextUid_++;
                connections_[uid] = conn;
                connToUid_[conn.get()] = uid;
            }

            chatService_->userOnline(uid);
            reply.body["code"] = 0;
            reply.body["uid"] = uid;
        } else {
            reply.header.type = protocol::MessageType::ERROR;
            reply.body["code"] = -1;
            reply.body["message"] = message;
        }
        sendMessage(conn, reply);
    });
}

void ChatServer::handleChat(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    uint64_t uid = 0;
    {
        muduo::MutexLockGuard lock(mutex_);
        auto it = connToUid_.find(conn.get());
        if (it != connToUid_.end()) {
            uid = it->second;
        }
    }

    if (uid == 0) {
        sendError(conn, "Please login first");
        return;
    }

    std::string roomId = msg.body["room_id"].get<std::string>();
    std::string content = msg.body["content"].get<std::string>();

    chatService_->sendChatMessage(uid, roomId, content);
}

void ChatServer::handleJoinRoom(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    uint64_t uid = 0;
    {
        muduo::MutexLockGuard lock(mutex_);
        auto it = connToUid_.find(conn.get());
        if (it != connToUid_.end()) {
            uid = it->second;
        }
    }

    if (uid == 0) {
        sendError(conn, "Please login first");
        return;
    }

    std::string roomId = msg.body["room_id"].get<std::string>();
    chatService_->joinRoom(uid, roomId);

    protocol::Message reply;
    reply.header.type = protocol::MessageType::SYSTEM;
    reply.body["message"] = "Joined room " + roomId;
    sendMessage(conn, reply);
}

void ChatServer::handleLeaveRoom(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    uint64_t uid = 0;
    {
        muduo::MutexLockGuard lock(mutex_);
        auto it = connToUid_.find(conn.get());
        if (it != connToUid_.end()) {
            uid = it->second;
        }
    }

    if (uid == 0) {
        sendError(conn, "Please login first");
        return;
    }

    std::string roomId = msg.body["room_id"].get<std::string>();
    chatService_->leaveRoom(uid, roomId);

    protocol::Message reply;
    reply.header.type = protocol::MessageType::SYSTEM;
    reply.body["message"] = "Left room " + roomId;
    sendMessage(conn, reply);
}

void ChatServer::handleHeartbeat(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    protocol::Message reply;
    reply.header.type = protocol::MessageType::HEARTBEAT;
    sendMessage(conn, reply);
}

void ChatServer::sendMessage(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg) {
    std::string encoded = protocol::encode(msg);
    muduo::net::Buffer buf;
    uint32_t len = static_cast<uint32_t>(encoded.size());
    buf.appendInt32(muduo::net::sockets::hostToNetwork32(len));
    buf.append(encoded);
    conn->send(&buf);
}

void ChatServer::sendError(const muduo::net::TcpConnectionPtr& conn, const std::string& errorMsg) {
    protocol::Message msg;
    msg.header.type = protocol::MessageType::ERROR;
    msg.body["message"] = errorMsg;
    sendMessage(conn, msg);
}

}