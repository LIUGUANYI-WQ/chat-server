#ifndef PROTOCOL_MESSAGE_TYPES_H
#define PROTOCOL_MESSAGE_TYPES_H

#include <string>
#include <cstdint>
#include <optional>
#include <vector>

namespace protocol {

// 消息类型枚举
enum class MessageType {
    REGISTER,
    LOGIN,
    LOGIN_RESP,
    CHAT,
    JOIN_ROOM,
    LEAVE_ROOM,
    PRIVATE_CHAT,
    ADD_FRIEND,
    SYSTEM,
    ERROR,
    HEARTBEAT
};

// 消息头
struct MessageHeader {
    MessageType type;
    uint64_t seq;
    uint64_t timestamp;
    std::string token;
};

// 消息体基类
struct MessageBody {
    virtual ~MessageBody() = default;
};

// 注册消息
struct RegisterBody : public MessageBody {
    std::string username;
    std::string password;
};

// 登录消息
struct LoginBody : public MessageBody {
    std::string username;
    std::string password;
};

// 登录响应消息
struct LoginRespBody : public MessageBody {
    int code;
    std::string token;
    uint64_t uid;
};

// 聊天消息
struct ChatBody : public MessageBody {
    std::string room_id;
    std::string content;
    std::optional<uint64_t> sender_uid;
};

// 加入房间消息
struct JoinRoomBody : public MessageBody {
    std::string room_id;
};

// 离开房间消息
struct LeaveRoomBody : public MessageBody {
    std::string room_id;
};

// 系统通知消息
struct SystemBody : public MessageBody {
    int code;
    std::string message;
};

// 错误消息
struct ErrorBody : public MessageBody {
    int code;
    std::string message;
};

// 私聊消息
struct PrivateChatBody : public MessageBody {
    uint64_t to_uid;
    std::string content;
    std::optional<uint64_t> sender_uid;
};

// 添加好友消息
struct AddFriendBody : public MessageBody {
    uint64_t friend_uid;
};

// 心跳消息
struct HeartbeatBody : public MessageBody {
};

} // namespace protocol

#endif // PROTOCOL_MESSAGE_TYPES_H