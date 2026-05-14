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

## 扩展优化方向

  我有一个基于 muduo 网络库的 C++ 注册登录系统项目，运行在 WSL2 环境，
  已跑通基础的 TCP 客户端-服务端通信、注册和登录功能，数据存储在 MySQL。

  项目目标是做成一个高并发、可水平扩展的认证服务，最终在简历上有竞争力。

  请你按照以下迭代路线帮我优化，每一步给出具体代码方案：

  阶段 1：数据库访问异步化
  - 使用 muduo 自带的线程池，将 MySQL 查询操作从 IO 线程剥离
  - 配合数据库连接池（复用连接，减少建连开销）
  - 回调通过 EventLoop::runInLoop 安全返回 IO 线程
  - 处理线程安全：数据拷贝、shared_ptr 保活、连接断开兜底

  阶段 2：多 Reactor 模型 + 密码安全
  - TcpServer 开启主从 Reactor（one loop per thread），充分利用多核
  - 密码使用 bcrypt 哈希存储，注册时加盐哈希，登录时校验密文
  - username 字段建唯一索引，查询只取必要列

  阶段 3：引入 Redis 缓存层
  - 登录成功生成 token，存 Redis（带过期时间）
  - 后续请求优先查 Redis 校验 token，命中则跳过 DB
  - 热点用户信息也缓存到 Redis，降低数据库压力

  阶段 4：容器化集群部署
  - 使用 Docker Compose 编排多实例（3个服务节点 + Nginx + Redis + MySQL）
  - Nginx 四层负载均衡（least_conn 策略）
  - 多实例共享 Redis 做 session 存储，实现无状态水平扩展
  - 健康检查与故障自动摘除

  阶段 5：性能压测与量化
  - 使用 wrk 或自研压测工具
  - 记录每个阶段的 QPS、P99 延迟、最大并发连接数
  - 输出优化前后对比数据

  技术栈约束：C++17、muduo、MySQL、Redis、Nginx、Docker Compose、bcrypt
  项目路径：各阶段代码分目录存放，CMake 构建
  每次只做一个阶段，做完等我确认再继续下一阶段。