#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/TcpConnection.h>

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <arpa/inet.h>
#include <cstring>

#include "../protocol/message.h"
#include "../protocol/message_types.h"

using namespace muduo;
using namespace muduo::net;

class Client {
public:
    Client(EventLoop* loop, const InetAddress& serverAddr)
        : loop_(loop), client_(loop, serverAddr, "Client"),
          loggedIn_(false), currentUid_(0), running_(true) {
        client_.setConnectionCallback(
            std::bind(&Client::onConnection, this, std::placeholders::_1));
        client_.setMessageCallback(
            std::bind(&Client::onMessage, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3));
        client_.enableRetry();
    }

    void connect() { client_.connect(); }
    void stop() { running_ = false; }

    void startInputLoop() {
        inputThread_ = std::thread([this]() {
            while (running_) {
                int choice;
                std::cin >> choice;
                if (!running_) break;

                if (loggedIn_) {
                    switch (choice) {
                        case 1: handleJoinRoom(); break;
                        case 2: handleLeaveRoom(); break;
                        case 3: handleSendChat(); break;
                        case 4: handlePrivateChat(); break;
                        case 5: handleAddFriend(); break;
                        case 6: loggedIn_ = false; loop_->runInLoop([this]{ printMainMenu(); }); break;
                        default: loop_->runInLoop([this]{ printChatMenu(); }); break;
                    }
                } else {
                    switch (choice) {
                        case 1: handleRegister(); break;
                        case 2: handleLogin(); break;
                        case 3:
                            if (!savedToken_.empty()) { handleTokenLogin(); break; }
                            running_ = false; return;
                        case 4:
                            if (savedToken_.empty()) { running_ = false; return; }
                            break;
                        default:
                            loop_->runInLoop([this]{ printMainMenu(); });
                            break;
                    }
                }
            }
        });
    }

    void join() {
        if (inputThread_.joinable()) inputThread_.join();
    }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        conn_ = conn;
        if (conn->connected()) {
            LOG_INFO << "连接到服务器成功";
            printMainMenu();
        } else {
            LOG_INFO << "与服务器断开连接";
            conn_.reset();
            running_ = false;
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
        while (buf->readableBytes() >= 4) {
            uint32_t len;
            memcpy(&len, buf->peek(), 4);
            len = ntohl(len);
            if (buf->readableBytes() < 4 + len) break;
            buf->retrieve(4);
            std::string msgStr = buf->retrieveAsString(len);

            auto optMsg = protocol::decode(msgStr);
            if (!optMsg) continue;
            processMessage(*optMsg);
        }

        // 消息处理完后刷新菜单提示
        if (loggedIn_) printChatMenu();
        else printMainMenu();
    }

    void processMessage(const protocol::Message& msg) {
        switch (msg.header.type) {
            case protocol::MessageType::LOGIN_RESP: {
                auto* body = static_cast<protocol::LoginRespBody*>(msg.body.get());
                if (body->code == 0) {
                    std::cout << "\n=== 登录成功 ===" << std::endl;
                    std::cout << "Token: " << body->token << std::endl;
                    savedToken_ = body->token;
                    currentUid_ = body->uid;
                    std::cout << "用户ID: " << currentUid_ << std::endl;
                    loggedIn_ = true;
                } else {
                    std::cout << "\n=== 登录失败 === 错误码: " << body->code << std::endl;
                }
                break;
            }
            case protocol::MessageType::SYSTEM: {
                auto* body = static_cast<protocol::SystemBody*>(msg.body.get());
                std::cout << "\n[系统] " << body->message << std::endl;
                break;
            }
            case protocol::MessageType::ERROR: {
                auto* body = static_cast<protocol::ErrorBody*>(msg.body.get());
                std::cout << "\n[错误] " << body->message << std::endl;
                break;
            }
            case protocol::MessageType::CHAT: {
                auto* body = static_cast<protocol::ChatBody*>(msg.body.get());
                std::cout << "\n[" << body->room_id << "]";
                if (body->sender_uid.has_value())
                    std::cout << " 用户" << body->sender_uid.value();
                std::cout << ": " << body->content << std::endl;
                break;
            }
            case protocol::MessageType::PRIVATE_CHAT: {
                auto* body = static_cast<protocol::PrivateChatBody*>(msg.body.get());
                std::cout << "\n[私聊]";
                if (body->sender_uid.has_value())
                    std::cout << " 用户" << body->sender_uid.value();
                std::cout << ": " << body->content << std::endl;
                break;
            }
            case protocol::MessageType::HEARTBEAT:
                std::cout << "[心跳]" << std::endl;
                break;
            default: break;
        }
    }

    // 只打印菜单，不读输入（输入由 inputThread_ 负责）
    void printMainMenu() {
        std::cout << "\n=== 主菜单 ===" << std::endl;
        std::cout << "1. 注册  2. 登录";
        if (!savedToken_.empty()) std::cout << "  3. Token登录  4. 退出";
        else std::cout << "  3. 退出";
        std::cout << std::endl << "> " << std::flush;
    }

    void printChatMenu() {
        std::cout << "\n=== 聊天菜单 ===" << std::endl;
        std::cout << "1.加入房间 2.离开房间 3.群聊 4.私聊 5.加好友 6.退出";
        std::cout << std::endl << "> " << std::flush;
    }

    // ---- 以下 handler 在 inputThread_ 中执行，读 cin 不会阻塞事件循环 ----

    void handleRegister() {
        std::string username, password;
        std::cout << "\n=== 用户注册 ===" << std::endl;
        std::cout << "用户名: " << std::flush;
        std::cin >> username;
        std::cout << "密码: " << std::flush;
        std::cin >> password;
        sendMessage(protocol::createRegister(1, username, password));
    }

    void handleLogin() {
        std::string username, password;
        std::cout << "\n=== 用户登录 ===" << std::endl;
        std::cout << "用户名: " << std::flush;
        std::cin >> username;
        std::cout << "密码: " << std::flush;
        std::cin >> password;
        sendMessage(protocol::createLogin(2, username, password));
    }

    void handleTokenLogin() {
        std::cout << "[Token登录暂不支持，请使用用户名密码]" << std::endl;
        handleLogin();
    }

    void handleJoinRoom() {
        std::string roomId;
        std::cout << "\n房间ID: " << std::flush;
        std::cin >> roomId;
        sendMessage(protocol::createJoinRoom(4, roomId, savedToken_));
    }

    void handleLeaveRoom() {
        std::string roomId;
        std::cout << "\n房间ID: " << std::flush;
        std::cin >> roomId;
        sendMessage(protocol::createLeaveRoom(5, roomId, savedToken_));
    }

    void handleSendChat() {
        std::string roomId, content;
        std::cout << "\n房间ID: " << std::flush;
        std::cin >> roomId;
        std::cin.ignore();
        std::cout << "消息: " << std::flush;
        std::getline(std::cin, content);
        sendMessage(protocol::createChat(6, roomId, content, savedToken_));
    }

    void handlePrivateChat() {
        uint64_t toUid;
        std::string content;
        std::cout << "\n对方UID: " << std::flush;
        std::cin >> toUid;
        std::cin.ignore();
        std::cout << "消息: " << std::flush;
        std::getline(std::cin, content);
        sendMessage(protocol::createPrivateChat(7, toUid, content, savedToken_));
    }

    void handleAddFriend() {
        uint64_t friendUid;
        std::cout << "\n好友UID: " << std::flush;
        std::cin >> friendUid;
        sendMessage(protocol::createAddFriend(8, friendUid, savedToken_));
    }

    void sendMessage(const protocol::Message& msg) {
        if (conn_) {
            std::string encoded = protocol::encode(msg);
            uint32_t len = htonl(static_cast<uint32_t>(encoded.size()));
            conn_->send(std::string(reinterpret_cast<const char*>(&len), 4) + encoded);
        }
    }

    EventLoop* loop_;
    TcpClient client_;
    TcpConnectionPtr conn_;
    std::string savedToken_;
    std::atomic<bool> loggedIn_{false};
    uint64_t currentUid_ = 0;
    std::atomic<bool> running_{true};
    std::thread inputThread_;
};

int main() {
    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 8888);
    Client client(&loop, serverAddr);
    client.connect();
    client.startInputLoop();
    loop.loop();
    client.stop();
    client.join();
    return 0;
}
