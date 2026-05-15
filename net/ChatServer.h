#ifndef NET_CHATSERVER_H
#define NET_CHATSERVER_H

#include "../service/AuthService.h"
#include "../service/ChatService.h"
#include "../protocol/message.h"
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <string>
#include <unordered_map>
#include <muduo/base/Mutex.h>

namespace net {

class ChatServer {
public:
    ChatServer(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr,
               service::AuthService* authService, service::ChatService* chatService);

    void start();

private:
    void onConnection(const muduo::net::TcpConnectionPtr& conn);
    void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp time);

    void handleRegister(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg);
    void handleLogin(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg);
    void handleChat(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg);
    void handleJoinRoom(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg);
    void handleLeaveRoom(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg);
    void handleHeartbeat(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg);

    void sendMessage(const muduo::net::TcpConnectionPtr& conn, const protocol::Message& msg);
    void sendError(const muduo::net::TcpConnectionPtr& conn, const std::string& errorMsg);

    muduo::net::TcpServer server_;
    muduo::net::EventLoop* loop_;
    service::AuthService* authService_;
    service::ChatService* chatService_;

    muduo::MutexLock mutex_;
    std::unordered_map<uint64_t, muduo::net::TcpConnectionPtr> connections_;
    std::unordered_map<muduo::net::TcpConnection*, uint64_t> connToUid_;
    uint64_t nextUid_;
};

}

#endif