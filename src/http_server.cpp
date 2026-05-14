#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <sstream>

using namespace muduo;
using namespace muduo::net;

void sendResponse(const TcpConnectionPtr& conn) {
    std::ostringstream resp;
    resp << " HTTP/1.1 200 OK\r\n;
