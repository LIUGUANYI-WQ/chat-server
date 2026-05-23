# 登录注册服务 - 高并发认证系统

基于 muduo 网络库的 C++ 高并发登录注册服务，支持异步数据库访问。

## 项目结构

```
taking/
├── include/              # 头文件
│   ├── MySQLConnectionPool.h
│   └── DBExecutor.h
├── src/                 # 源代码
│   ├── MySQLConnectionPool.cpp
│   ├── DBExecutor.cpp
│   ├── server.cpp
│   └── client.cpp
├── benchmark/           # 压测工具
│   └── benchmark.cpp
├── build/               # 构建输出目录
├── build.sh             # 一键编译脚本
├── start_server.sh      # 一键启动服务器
├── start_client.sh      # 一键启动客户端
├── CMakeLists.txt       # CMake 构建配置
├── Makefile             # Makefile 构建配置
└── README.md            # 项目文档
```

## 功能特性

- 异步数据库访问（muduo 线程池）
- MySQL 连接池（支持连接复用和自动重连）
- 线程安全的回调机制
- 支持高并发
- 一键编译和运行脚本

## 快速开始（推荐）

### 1. 一键编译
```bash
./build.sh
```

### 2. 启动服务器
```bash
./start_server.sh
```

### 3. 启动客户端（另开终端）
```bash
./start_client.sh
```

## 编译（手动方式）

### 使用 CMake
```bash
cd build
cmake ..
make
```

### 使用 Makefile
```bash
make
```

## 运行

### 启动服务器
```bash
./server
# 或使用一键脚本
./start_server.sh
```

### 启动客户端
```bash
./client
# 或使用一键脚本
./start_client.sh
```

## 压测

### 编译压测工具
```bash
make benchmark
```

### 运行压测
```bash
# 10 个线程，每个线程 1000 个请求
./benchmark 10 1000 127.0.0.1 8888
```

## 压测方案

### 1. 基础压测
使用提供的 `benchmark` 工具进行测试，逐步增加并发量：

```bash
# 小规模测试
./benchmark 4 500 127.0.0.1 8888

# 中等规模
./benchmark 10 1000 127.0.0.1 8888

# 大规模测试
./benchmark 20 2000 127.0.0.1 8888
```

### 2. 性能监控
在压测同时监控服务器资源使用：

```bash
# 监控 CPU 和内存
top

# 监控网络连接
netstat -an | grep 8888 | wc -l

# 监控 MySQL 连接
mysql -u root -p -e "SHOW PROCESSLIST;"
```

### 3. 记录关键指标

- **QPS (Queries Per Second)**：每秒处理请求数
- **Latency**：平均响应延迟
- **成功率**：成功请求占比
- **资源占用**：CPU、内存、网络带宽
- **MySQL 连接数**：数据库连接使用情况

### 4. 基准对比

记录阶段 1 优化前后的性能对比：

| 指标 | 同步版本 | 异步版本 | 提升 |
|------|---------|---------|------|
| QPS  |         |         |      |
| 延迟 |         |         |      |

## 压测注意事项

1. **预热**：先运行小规模测试，让 JIT 编译和连接池预热
2. **持续时间**：每次测试至少持续 30 秒以上
3. **多次测试**：每个场景运行 3-5 次取平均值
4. **环境稳定**：压测期间不要运行其他程序
5. **MySQL 配置**：确保 MySQL 的最大连接数足够

## 一键脚本说明

### build.sh
- 自动检测编译环境（CMake 或 Makefile）
- 可选清理旧的构建文件
- 并行编译加速

### start_server.sh
- 自动检测是否已编译
- 未编译时自动编译
- 检查 MySQL 服务状态
- 启动服务器

### start_client.sh
- 自动检测是否已编译
- 未编译时自动编译
- 启动客户端

---
  你是一个精通 muduo 网络库、Redis、MySQL 的 C++ 后端工程师。

  ## 项目背景

  我有一个基于 muduo 的 C++ 认证服务，运行在 WSL2 环境。已完成：
  - muduo TcpServer（多 Reactor 模式，one loop per thread）
  - 用户注册（bcrypt 哈希密码）和登录（token 机制）
  - MySQL 存储用户数据，Redis 做 token 缓存
  - 数据库操作通过 muduo 线程池异步化，runInLoop 回调返回 IO 线程
  - 自定义二进制协议（length + type + payload）用于 TCP 拆包

  ## 本次任务

  在现有认证服务的基础上，引入聊天功能，并完成三次迭代重构。每次只做一个迭代，等我确认再继续下一个。

  ## 约束条件

  - C++17，muduo 网络库
  - JSON 库用 nlohmann/json（单头文件，MIT 协议）
  - CMake 构建，每个迭代的代码放在独立子目录
  - 线程安全：数据拷贝进 DB 线程池，结果通过 runInLoop 弹回 IO 线程
  - 不在 IO 线程做阻塞操作
  - 不写前端代码，只做后端

  ---

  ## 迭代一：JSON 协议设计

  将现有的自定义二进制协议替换为 JSON 格式的私有通信协议。

  ### 协议结构

  每条消息统一包含消息头和消息体：

  {
    "header": {
      "type": "消息类型",
      "seq": 自增序号,
      "timestamp": 时间戳,
      "token": "认证令牌"
    },
    "body": {
      // 根据 type 不同，body 字段不同
    }
  }

  ### 消息类型定义

  | type | 方向 | body 字段 | 说明 |
  |------|------|----------|------|
  | REGISTER | C→S | username, password | 注册 |
  | LOGIN | C→S | username, password | 登录 |
  | LOGIN_RESP | S→C | code, token, uid | 登录响应 |
  | CHAT | C→S / S→C | room_id, content | 聊天消息 |
  | JOIN_ROOM | C→S | room_id | 加入房间 |
  | LEAVE_ROOM | C→S | room_id | 离开房间 |
  | SYSTEM | S→C | code, message | 系统通知 |
  | ERROR | S→C | code, message | 错误信息 |
  | HEARTBEAT | C→S | 无 | 心跳保活 |

  ### 要求

  - 定义 MessageHeader 和 MessageBody 结构体
  - body 用 std::variant 或继承体系处理不同消息类型
  - 封装 encode(Message) → std::string 和 decode(std::string) → Message
  - 替换现有 codec 中的解析逻辑，保持与 muduo 拆包层的兼容
  - 加 seq 字段用于请求-响应匹配（客户端发请求带 seq，服务端回复带相同 seq）

  ### 产出物

  - `protocol/` 目录：message.h / message.cpp / message_types.h
  - 单元测试：编解码往返验证（encode → decode → 字段不变）

  ---

  ## 迭代二：分层架构 + 接口抽象

  将现有代码重构成三层架构，用接口解耦各层依赖。

  ### 分层结构

  网络层（net/）      → 只负责收发包、编解码，调业务层接口
  业务层（service/）  → 聊天逻辑、鉴权、好友管理，只依赖数据层接口
  数据层（store/）    → 定义接口 + 提供 MySQL 和 Redis 两种实现

  ### 数据层接口

  ```cpp
  // store/IUserStore.h
  class IUserStore {
  public:
      virtual std::optional<User> findByToken(const std::string& token) = 0;
      virtual std::optional<User> findByUsername(const std::string& username) = 0;
      virtual bool insertUser(const User& user) = 0;
  };

  // store/IChatStore.h
  class IChatStore {
  public:
      virtual void setOnline(uint64_t uid) = 0;
      virtual void setOffline(uint64_t uid) = 0;
      virtual bool isOnline(uint64_t uid) = 0;
      virtual std::vector<uint64_t> getOnlineUsers(const std::string& room) = 0;
      virtual void cacheMessage(const std::string& room, const ChatMsg& msg) = 0;
      virtual std::vector<ChatMsg> getRecentMessages(const std::string& room, int count) = 0;
  };

  要求

  - 业务层只依赖接口（纯虚类），不 include MySQL 或 Redis 的头文件
  - 具体实现在构造函数注入（依赖注入），main 函数里组装
  - 网络回调只做转调：收包 → 解析 JSON → 调 service 方法 → 编码结果 → 发包
  - 每个业务函数不超过 50 行
  - CMakeLists 按层分 library，上层 link 下层的接口库，不 link 实现

  目录结构

  project/
  ├── net/           # 网络层：TcpServer 包装、codec、回调注册
  ├── service/       # 业务层：AuthService、ChatService
  ├── store/         # 数据层接口 + 实现
  │   ├── IUserStore.h
  │   ├── IChatStore.h
  │   ├── MysqlUserStore.h / .cpp
  │   ├── RedisUserStore.h / .cpp
  │   └── RedisChatStore.h / .cpp
  ├── protocol/      # 迭代一的协议层
  └── main.cpp

  产出物

  - 完整的三层目录结构
  - 每一层可独立编译
  - 业务层不包含任何 SQL 语句或 Redis 命令字符串

  ---
  迭代三：Redis 聊天缓存 + MySQL 连接池

  Redis 缓存策略

  ┌──────────┬────────────────────────┬────────────────────────────────┐
  │ 数据类型 │       Redis 结构       │              说明              │
  ├──────────┼────────────────────────┼────────────────────────────────┤
  │ 在线状态 │ SET online_users       │ uid 集合，上线 SADD，下线 SREM │
  ├──────────┼────────────────────────┼────────────────────────────────┤
  │ 好友列表 │ SET user:{uid}:friends │ 好友 uid 集合                  │
  ├──────────┼────────────────────────┼────────────────────────────────┤
  │ 最近消息 │ LIST room:{id}:msgs    │ 最新 100 条，LPUSH + LTRIM     │
  ├──────────┼────────────────────────┼────────────────────────────────┤
  │ 历史消息 │ MySQL                  │ 冷数据，分页查询               │
  └──────────┴────────────────────────┴────────────────────────────────┘

  聊天消息流程

  发消息：
    ├─ Redis LPUSH room:{id}:msgs 消息JSON    ← 写缓存
    ├─ Redis LTRIM room:{id}:msgs 0 99        ← 只保留最近100条
    └─ 投递到线程池，异步写 MySQL 消息表       ← 批量持久化

  拉消息：
    ├─ 进房间 → Redis LRANGE room:{id}:msgs 0 49   ← 缓存命中
    └─ 翻历史 → MySQL SELECT ... WHERE id < ? LIMIT 50  ← 冷数据

  MySQL 连接池

  - 封装 MysqlPool 类
  - 用 std::queue 管理空闲连接，mutex + condition_variable 做线程安全借还
  - 支持设置最大连接数（默认 8）
  - 连接复用：借出 → 使用 → 归还，不每次新建
  - 配合已有的 DB 线程池使用（每个 DB 线程从连接池 borrow，用完 Return）

  要求

  - RedisChatStore 实现 IChatStore 接口
  - 消息持久化到 MySQL 是异步的，不阻塞聊天消息的实时转发
  - 连接池要处理超时归还和异常连接的自动回收
  - 在线状态在用户连接建立/断开时更新，心跳超时 30 秒自动踢下线

  产出物

  - store/RedisChatStore.h/.cpp
  - store/MysqlPool.h/.cpp
  - service/ChatService.h/.cpp（完整的聊天业务逻辑）
  - 单节点聊天可跑通：多客户端登录 → 加入房间 → 互发消息

  ---
  请从迭代一开始。每次只做一个迭代，代码写完后等我确认再继续下一个。