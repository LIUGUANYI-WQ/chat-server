#ifndef PROTOCOL_MESSAGE_H
#define PROTOCOL_MESSAGE_H

#include "message_types.h"
#include "json.hpp"
#include <memory>
#include <string>
#include <chrono>

namespace protocol {

using json = nlohmann::json;

// 完整消息结构体
struct Message {
    MessageHeader header;
    std::unique_ptr<MessageBody> body;
};

// MessageType 与字符串的转换辅助函数
std::string messageTypeToString(MessageType type);
MessageType stringToMessageType(const std::string& str);

// 序列化：Message -> JSON string
std::string encode(const Message& msg);

// 反序列化：JSON string -> Message
std::optional<Message> decode(const std::string& json_str);

// 辅助函数：创建各种类型的消息
Message createRegister(uint64_t seq, const std::string& username, const std::string& password);
Message createLogin(uint64_t seq, const std::string& username, const std::string& password);
Message createLoginResp(uint64_t seq, int code, const std::string& token, uint64_t uid);
Message createChat(uint64_t seq, const std::string& room_id, const std::string& content,
                    const std::string& token = "", std::optional<uint64_t> sender_uid = std::nullopt);
Message createJoinRoom(uint64_t seq, const std::string& room_id, const std::string& token = "");
Message createLeaveRoom(uint64_t seq, const std::string& room_id, const std::string& token = "");
Message createSystem(uint64_t seq, int code, const std::string& message);       
Message createError(uint64_t seq, int code, const std::string& message);        
Message createHeartbeat(uint64_t seq, const std::string& token = "");

// 获取当前时间戳（毫秒）
inline uint64_t getCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace protocol

#endif // PROTOCOL_MESSAGE_H