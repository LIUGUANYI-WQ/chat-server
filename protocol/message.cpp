#include "message.h"
#include <stdexcept>
#include <cstring>

namespace protocol {

using json = nlohmann::json;

// MessageType 与字符串的转换
std::string messageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::REGISTER: return "REGISTER";
        case MessageType::LOGIN: return "LOGIN";
        case MessageType::LOGIN_RESP: return "LOGIN_RESP";
        case MessageType::CHAT: return "CHAT";
        case MessageType::JOIN_ROOM: return "JOIN_ROOM";
        case MessageType::LEAVE_ROOM: return "LEAVE_ROOM";
        case MessageType::PRIVATE_CHAT: return "PRIVATE_CHAT";
        case MessageType::ADD_FRIEND: return "ADD_FRIEND";
        case MessageType::SYSTEM: return "SYSTEM";
        case MessageType::ERROR: return "ERROR";
        case MessageType::HEARTBEAT: return "HEARTBEAT";
        default: throw std::invalid_argument("Unknown message type");
    }
}

MessageType stringToMessageType(const std::string& str) {
    if (str == "REGISTER") return MessageType::REGISTER;
    if (str == "LOGIN") return MessageType::LOGIN;
    if (str == "LOGIN_RESP") return MessageType::LOGIN_RESP;
    if (str == "CHAT") return MessageType::CHAT;
    if (str == "JOIN_ROOM") return MessageType::JOIN_ROOM;
    if (str == "LEAVE_ROOM") return MessageType::LEAVE_ROOM;
    if (str == "PRIVATE_CHAT") return MessageType::PRIVATE_CHAT;
    if (str == "ADD_FRIEND") return MessageType::ADD_FRIEND;
    if (str == "SYSTEM") return MessageType::SYSTEM;
    if (str == "ERROR") return MessageType::ERROR;
    if (str == "HEARTBEAT") return MessageType::HEARTBEAT;
    throw std::invalid_argument("Unknown message type string: " + str);
}

// encode 实现
std::string encode(const Message& msg) {
    json j;
    j["header"]["type"] = messageTypeToString(msg.header.type);
    j["header"]["seq"] = msg.header.seq;
    j["header"]["timestamp"] = msg.header.timestamp;
    j["header"]["token"] = msg.header.token;

    switch (msg.header.type) {
        case MessageType::REGISTER: {
            auto* body = static_cast<RegisterBody*>(msg.body.get());
            j["body"]["username"] = body->username;
            j["body"]["password"] = body->password;
            break;
        }
        case MessageType::LOGIN: {
            auto* body = static_cast<LoginBody*>(msg.body.get());
            j["body"]["username"] = body->username;
            j["body"]["password"] = body->password;
            break;
        }
        case MessageType::LOGIN_RESP: {
            auto* body = static_cast<LoginRespBody*>(msg.body.get());
            j["body"]["code"] = body->code;
            j["body"]["token"] = body->token;
            j["body"]["uid"] = body->uid;
            break;
        }
        case MessageType::CHAT: {
            auto* body = static_cast<ChatBody*>(msg.body.get());
            j["body"]["room_id"] = body->room_id;
            j["body"]["content"] = body->content;
            if (body->sender_uid) {
                j["body"]["sender_uid"] = *body->sender_uid;
            }
            break;
        }
        case MessageType::JOIN_ROOM: {
            auto* body = static_cast<JoinRoomBody*>(msg.body.get());
            j["body"]["room_id"] = body->room_id;
            break;
        }
        case MessageType::LEAVE_ROOM: {
            auto* body = static_cast<LeaveRoomBody*>(msg.body.get());
            j["body"]["room_id"] = body->room_id;
            break;
        }
        case MessageType::PRIVATE_CHAT: {
            auto* body = static_cast<PrivateChatBody*>(msg.body.get());
            j["body"]["to_uid"] = body->to_uid;
            j["body"]["content"] = body->content;
            if (body->sender_uid) j["body"]["sender_uid"] = *body->sender_uid;
            break;
        }
        case MessageType::ADD_FRIEND: {
            auto* body = static_cast<AddFriendBody*>(msg.body.get());
            j["body"]["friend_uid"] = body->friend_uid;
            break;
        }
        case MessageType::SYSTEM: {
            auto* body = static_cast<SystemBody*>(msg.body.get());
            j["body"]["code"] = body->code;
            j["body"]["message"] = body->message;
            break;
        }
        case MessageType::ERROR: {
            auto* body = static_cast<ErrorBody*>(msg.body.get());
            j["body"]["code"] = body->code;
            j["body"]["message"] = body->message;
            break;
        }
        case MessageType::HEARTBEAT: {
            j["body"] = json::object();
            break;
        }
    }

    return j.dump();
}

// decode 实现
std::optional<Message> decode(const std::string& json_str) {
    try {
        json j = json::parse(json_str);
        Message msg;

        // 解析header
        msg.header.type = stringToMessageType(j["header"]["type"].get<std::string>());
        msg.header.seq = j["header"]["seq"].get<uint64_t>();
        msg.header.timestamp = j["header"]["timestamp"].get<uint64_t>();
        msg.header.token = j["header"]["token"].get<std::string>();

        // 解析body
        switch (msg.header.type) {
            case MessageType::REGISTER: {
                auto body = std::make_unique<RegisterBody>();
                body->username = j["body"]["username"].get<std::string>();
                body->password = j["body"]["password"].get<std::string>();
                msg.body = std::move(body);
                break;
            }
            case MessageType::LOGIN: {
                auto body = std::make_unique<LoginBody>();
                body->username = j["body"]["username"].get<std::string>();
                body->password = j["body"]["password"].get<std::string>();
                msg.body = std::move(body);
                break;
            }
            case MessageType::LOGIN_RESP: {
                auto body = std::make_unique<LoginRespBody>();
                body->code = j["body"]["code"].get<int>();
                body->token = j["body"]["token"].get<std::string>();
                body->uid = j["body"]["uid"].get<uint64_t>();
                msg.body = std::move(body);
                break;
            }
            case MessageType::CHAT: {
                auto body = std::make_unique<ChatBody>();
                body->room_id = j["body"]["room_id"].get<std::string>();
                body->content = j["body"]["content"].get<std::string>();
                if (j["body"].contains("sender_uid")) {
                    body->sender_uid = j["body"]["sender_uid"].get<uint64_t>();
                }
                msg.body = std::move(body);
                break;
            }
            case MessageType::JOIN_ROOM: {
                auto body = std::make_unique<JoinRoomBody>();
                body->room_id = j["body"]["room_id"].get<std::string>();
                msg.body = std::move(body);
                break;
            }
            case MessageType::LEAVE_ROOM: {
                auto body = std::make_unique<LeaveRoomBody>();
                body->room_id = j["body"]["room_id"].get<std::string>();
                msg.body = std::move(body);
                break;
            }
            case MessageType::PRIVATE_CHAT: {
                auto body = std::make_unique<PrivateChatBody>();
                body->to_uid = j["body"]["to_uid"].get<uint64_t>();
                body->content = j["body"]["content"].get<std::string>();
                if (j["body"].contains("sender_uid"))
                    body->sender_uid = j["body"]["sender_uid"].get<uint64_t>();
                msg.body = std::move(body);
                break;
            }
            case MessageType::ADD_FRIEND: {
                auto body = std::make_unique<AddFriendBody>();
                body->friend_uid = j["body"]["friend_uid"].get<uint64_t>();
                msg.body = std::move(body);
                break;
            }
            case MessageType::SYSTEM: {
                auto body = std::make_unique<SystemBody>();
                body->code = j["body"]["code"].get<int>();
                body->message = j["body"]["message"].get<std::string>();
                msg.body = std::move(body);
                break;
            }
            case MessageType::ERROR: {
                auto body = std::make_unique<ErrorBody>();
                body->code = j["body"]["code"].get<int>();
                body->message = j["body"]["message"].get<std::string>();
                msg.body = std::move(body);
                break;
            }
            case MessageType::HEARTBEAT: {
                msg.body = std::make_unique<HeartbeatBody>();
                break;
            }
        }

        return msg;
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

// 创建消息的辅助函数
Message createRegister(uint64_t seq, const std::string& username, const std::string& password) {
    Message msg;
    msg.header.type = MessageType::REGISTER;
    msg.header.seq = seq;
    msg.header.timestamp = getCurrentTimestamp();
    msg.header.token = "";
    auto body = std::make_unique<RegisterBody>();
    body->username = username;
    body->password = password;
    msg.body = std::move(body);
    return msg;
}

Message createLogin(uint64_t seq, const std::string& username, const std::string& password) {
    Message msg;
    msg.header.type = MessageType::LOGIN;
    msg.header.seq = seq;
    msg.header.timestamp = getCurrentTimestamp();
    msg.header.token = "";
    auto body = std::make_unique<LoginBody>();
    body->username = username;
    body->password = password;
    msg.body = std::move(body);
    return msg;
}

Message createLoginResp(uint64_t seq, int code, const std::string& token, uint64_t uid) {
    Message msg;
    msg.header.type = MessageType::LOGIN_RESP;
    msg.header.seq = seq;
    msg.header.timestamp = getCurrentTimestamp();
    msg.header.token = "";
    auto body = std::make_unique<LoginRespBody>();
    body->code = code;
    body->token = token;
    body->uid = uid;
    msg.body = std::move(body);
    return msg;
}

Message createChat(uint64_t seq, const std::string& room_id, const std::string& content, 
                    const std::string& token, std::optional<uint64_t> sender_uid) {
    Message msg;
    msg.header.type = MessageType::CHAT;
    msg.header.seq = seq;
    msg.header.timestamp = getCurrentTimestamp();
    msg.header.token = token;
    auto body = std::make_unique<ChatBody>();
    body->room_id = room_id;
    body->content = content;
    body->sender_uid = sender_uid;
    msg.body = std::move(body);
    return msg;
}

Message createJoinRoom(uint64_t seq, const std::string& room_id, const std::string& token) {
    Message msg;
    msg.header.type = MessageType::JOIN_ROOM;
    msg.header.seq = seq;
    msg.header.timestamp = getCurrentTimestamp();
    msg.header.token = token;
    auto body = std::make_unique<JoinRoomBody>();
    body->room_id = room_id;
    msg.body = std::move(body);
    return msg;
}

Message createLeaveRoom(uint64_t seq, const std::string& room_id, const std::string& token) {
    Message msg;
    msg.header.type = MessageType::LEAVE_ROOM;
    msg.header.seq = seq;
    msg.header.timestamp = getCurrentTimestamp();
    msg.header.token = token;
    auto body = std::make_unique<LeaveRoomBody>();
    body->room_id = room_id;
    msg.body = std::move(body);
    return msg;
}

Message createSystem(uint64_t seq, int code, const std::string& message) {
    Message msg;
    msg.header.type = MessageType::SYSTEM;
    msg.header.seq = seq;
    msg.header.timestamp = getCurrentTimestamp();
    msg.header.token = "";
    auto body = std::make_unique<SystemBody>();
    body->code = code;
    body->message = message;
    msg.body = std::move(body);
    return msg;
}

Message createError(uint64_t seq, int code, const std::string& message) {
    Message msg;
    msg.header.type = MessageType::ERROR;
    msg.header.seq = seq;
    msg.header.timestamp = getCurrentTimestamp();
    msg.header.token = "";
    auto body = std::make_unique<ErrorBody>();
    body->code = code;
    body->message = message;
    msg.body = std::move(body);
    return msg;
}

Message createPrivateChat(uint64_t seq, uint64_t to_uid, const std::string& content,
                           const std::string& token, std::optional<uint64_t> sender_uid) {
    Message msg;
    msg.header.type = MessageType::PRIVATE_CHAT;
    msg.header.seq = seq;
    msg.header.timestamp = getCurrentTimestamp();
    msg.header.token = token;
    auto body = std::make_unique<PrivateChatBody>();
    body->to_uid = to_uid;
    body->content = content;
    body->sender_uid = sender_uid;
    msg.body = std::move(body);
    return msg;
}

Message createAddFriend(uint64_t seq, uint64_t friend_uid, const std::string& token) {
    Message msg;
    msg.header.type = MessageType::ADD_FRIEND;
    msg.header.seq = seq;
    msg.header.timestamp = getCurrentTimestamp();
    msg.header.token = token;
    auto body = std::make_unique<AddFriendBody>();
    body->friend_uid = friend_uid;
    msg.body = std::move(body);
    return msg;
}

Message createHeartbeat(uint64_t seq, const std::string& token) {
    Message msg;
    msg.header.type = MessageType::HEARTBEAT;
    msg.header.seq = seq;
    msg.header.timestamp = getCurrentTimestamp();
    msg.header.token = token;
    msg.body = std::make_unique<HeartbeatBody>();
    return msg;
}

} // namespace protocol