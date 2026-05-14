#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/TcpConnection.h>

#include <iostream>
#include <string>
#include <functional>

using namespace muduo;
using namespace muduo::net;

class Client {
public:
    Client(EventLoop* loop, const InetAddress& serverAddr)
        : client_(loop, serverAddr, "Client") {
        client_.setConnectionCallback(
            std::bind(&Client::onConnection, this, std::placeholders::_1));
        client_.setMessageCallback(
            std::bind(&Client::onMessage, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3));
        client_.enableRetry();
    }

    void connect() {
        client_.connect();
    }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        conn_ = conn;
        if (conn->connected()) {
            LOG_INFO << "连接到服务器成功";
            showMenu();
        } else {
            LOG_INFO << "与服务器断开连接";
            conn_.reset();
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
        std::string response = buf->retrieveAllAsString();
        std::cout << "\n服务器响应：\n" << response << std::endl;

        size_t tokenPos = response.find("token: ");
        if (tokenPos != std::string::npos) {
            size_t tokenStart = tokenPos + 7;
            size_t tokenEnd = response.find("\n", tokenStart);
            if (tokenEnd == std::string::npos) {
                tokenEnd = response.find("\r\n", tokenStart);
            }
            if (tokenEnd == std::string::npos) {
                tokenEnd = response.length();
            }
            savedToken_ = response.substr(tokenStart, tokenEnd - tokenStart);
            std::cout << "Token已保存: " << savedToken_ << std::endl;
        }

        showMenu();
    }

    void showMenu() {
        std::cout << "\n=== 用户菜单 ===" << std::endl;
        std::cout << "1. 注册" << std::endl;
        std::cout << "2. 登录" << std::endl;
        if (!savedToken_.empty()) {
            std::cout << "3. Token登录 (已保存token)" << std::endl;
            std::cout << "4. 退出" << std::endl;
        } else {
            std::cout << "3. 退出" << std::endl;
        }
        std::cout << "请输入选择: ";

        int choice;
        std::cin >> choice;

        switch (choice) {
            case 1:
                handleRegister();
                break;
            case 2:
                handleLogin();
                break;
            case 3:
                if (!savedToken_.empty()) {
                    handleTokenLogin();
                } else {
                    if (conn_) {
                        conn_->shutdown();
                    }
                    exit(0);
                }
                break;
            case 4:
                if (!savedToken_.empty()) {
                    if (conn_) {
                        conn_->shutdown();
                    }
                    exit(0);
                }
            default:
                std::cout << "无效选择，请重新输入" << std::endl;
                showMenu();
                break;
        }
    }

    void handleRegister() {
        std::string username, password;
        std::cout << "=== 用户注册 ===" << std::endl;
        std::cout << "用户名: ";
        std::cin >> username;
        std::cout << "密码: ";
        std::cin >> password;

        std::string request = "REGISTER " + username + " " + password + "\n";
        if (conn_) {
            conn_->send(request);
        }
    }

    void handleLogin() {
        std::string username, password;
        std::cout << "=== 用户登录 ===" << std::endl;
        std::cout << "用户名: ";
        std::cin >> username;
        std::cout << "密码: ";
        std::cin >> password;

        std::string request = "LOGIN " + username + " " + password + "\n";
        if (conn_) {
            conn_->send(request);
        }
    }

    void handleTokenLogin() {
        if (savedToken_.empty()) {
            std::cout << "没有保存的token，请先登录" << std::endl;
            showMenu();
            return;
        }

        std::cout << "=== Token登录 ===" << std::endl;
        std::cout << "使用token登录: " << savedToken_ << std::endl;

        std::string request = "TOKEN_LOGIN " + savedToken_ + "\n";
        if (conn_) {
            conn_->send(request);
        }
    }

    TcpClient client_;
    TcpConnectionPtr conn_;
    std::string savedToken_;
};

int main() {
    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 8888);
    Client client(&loop, serverAddr);
    client.connect();
    loop.loop();
    return 0;
}