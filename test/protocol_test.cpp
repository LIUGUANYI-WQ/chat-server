#include "../protocol/message.h"
#include "../protocol/message_types.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace protocol;

// 测试辅助：比较两个消息是否相等
bool messagesEqual(const Message& msg1, const Message& msg2) {
    if (msg1.header.type != msg2.header.type) return false;
    if (msg1.header.seq != msg2.header.seq) return false;
    if (msg1.header.token != msg2.header.token) return false;

    switch (msg1.header.type) {
        case MessageType::REGISTER: {
            auto* b1 = static_cast<RegisterBody*>(msg1.body.get());
            auto* b2 = static_cast<RegisterBody*>(msg2.body.get());
            return b1->username == b2->username && b1->password == b2->password;
        }
        case MessageType::LOGIN: {
            auto* b1 = static_cast<LoginBody*>(msg1.body.get());
            auto* b2 = static_cast<LoginBody*>(msg2.body.get());
            return b1->username == b2->username && b1->password == b2->password;
        }
        case MessageType::LOGIN_RESP: {
            auto* b1 = static_cast<LoginRespBody*>(msg1.body.get());
            auto* b2 = static_cast<LoginRespBody*>(msg2.body.get());
            return b1->code == b2->code && b1->token == b2->token && b1->uid == b2->uid;
        }
        case MessageType::CHAT: {
            auto* b1 = static_cast<ChatBody*>(msg1.body.get());
            auto* b2 = static_cast<ChatBody*>(msg2.body.get());
            return b1->room_id == b2->room_id && b1->content == b2->content && b1->sender_uid == b2->sender_uid;
        }
        case MessageType::JOIN_ROOM:
        case MessageType::LEAVE_ROOM: {
            auto* b1 = static_cast<JoinRoomBody*>(msg1.body.get());
            auto* b2 = static_cast<JoinRoomBody*>(msg2.body.get());
            return b1->room_id == b2->room_id;
        }
        case MessageType::SYSTEM:
        case MessageType::ERROR: {
            auto* b1 = static_cast<SystemBody*>(msg1.body.get());
            auto* b2 = static_cast<SystemBody*>(msg2.body.get());
            return b1->code == b2->code && b1->message == b2->message;
        }
        case MessageType::HEARTBEAT:
            return true;
    }
    return false;
}

// 测试1: 注册消息往返
void testRegisterRoundtrip() {
    std::cout << "Testing REGISTER roundtrip..." << std::endl;
    auto original = createRegister(1, "testuser", "testpass123");
    std::string json_str = encode(original);
    auto decoded_opt = decode(json_str);
    assert(decoded_opt.has_value() && "Decode failed");
    assert(messagesEqual(original, *decoded_opt) && "Fields mismatch");
    std::cout << "  ✓ PASS" << std::endl;
}

// 测试2: 登录消息往返
void testLoginRoundtrip() {
    std::cout << "Testing LOGIN roundtrip..." << std::endl;
    auto original = createLogin(2, "testuser", "testpass123");
    std::string json_str = encode(original);
    auto decoded_opt = decode(json_str);
    assert(decoded_opt.has_value());
    assert(messagesEqual(original, *decoded_opt));
    std::cout << "  ✓ PASS" << std::endl;
}

// 测试3: 登录响应消息往返
void testLoginRespRoundtrip() {
    std::cout << "Testing LOGIN_RESP roundtrip..." << std::endl;
    auto original = createLoginResp(3, 0, "testtoken123", 1001);
    std::string json_str = encode(original);
    auto decoded_opt = decode(json_str);
    assert(decoded_opt.has_value());
    assert(messagesEqual(original, *decoded_opt));
    std::cout << "  ✓ PASS" << std::endl;
}

// 测试4: 聊天消息往返
void testChatRoundtrip() {
    std::cout << "Testing CHAT roundtrip..." << std::endl;
    auto original = createChat(4, "room1", "Hello world!", "usertoken", 1001);
    std::string json_str = encode(original);
    auto decoded_opt = decode(json_str);
    assert(decoded_opt.has_value());
    assert(messagesEqual(original, *decoded_opt));
    std::cout << "  ✓ PASS" << std::endl;
}

// 测试5: 加入房间消息往返
void testJoinRoomRoundtrip() {
    std::cout << "Testing JOIN_ROOM roundtrip..." << std::endl;
    auto original = createJoinRoom(5, "room1", "usertoken");
    std::string json_str = encode(original);
    auto decoded_opt = decode(json_str);
    assert(decoded_opt.has_value());
    assert(messagesEqual(original, *decoded_opt));
    std::cout << "  ✓ PASS" << std::endl;
}

// 测试6: 离开房间消息往返
void testLeaveRoomRoundtrip() {
    std::cout << "Testing LEAVE_ROOM roundtrip..." << std::endl;
    auto original = createLeaveRoom(6, "room1", "usertoken");
    std::string json_str = encode(original);
    auto decoded_opt = decode(json_str);
    assert(decoded_opt.has_value());
    assert(messagesEqual(original, *decoded_opt));
    std::cout << "  ✓ PASS" << std::endl;
}

// 测试7: 系统消息往返
void testSystemRoundtrip() {
    std::cout << "Testing SYSTEM roundtrip..." << std::endl;
    auto original = createSystem(7, 0, "System message test");
    std::string json_str = encode(original);
    auto decoded_opt = decode(json_str);
    assert(decoded_opt.has_value());
    assert(messagesEqual(original, *decoded_opt));
    std::cout << "  ✓ PASS" << std::endl;
}

// 测试8: 错误消息往返
void testErrorRoundtrip() {
    std::cout << "Testing ERROR roundtrip..." << std::endl;
    auto original = createError(8, 404, "Not found");
    std::string json_str = encode(original);
    auto decoded_opt = decode(json_str);
    assert(decoded_opt.has_value());
    assert(messagesEqual(original, *decoded_opt));
    std::cout << "  ✓ PASS" << std::endl;
}

// 测试9: 心跳消息往返
void testHeartbeatRoundtrip() {
    std::cout << "Testing HEARTBEAT roundtrip..." << std::endl;
    auto original = createHeartbeat(9, "usertoken");
    std::string json_str = encode(original);
    auto decoded_opt = decode(json_str);
    assert(decoded_opt.has_value());
    assert(messagesEqual(original, *decoded_opt));
    std::cout << "  ✓ PASS" << std::endl;
}

// 测试10: 无效JSON解码
void testInvalidJsonDecode() {
    std::cout << "Testing invalid JSON decode..." << std::endl;
    auto result = decode("invalid json {{{");
    assert(!result.has_value());
    std::cout << "  ✓ PASS" << std::endl;
}

int main() {
    std::cout << "=== Protocol Unit Tests ===" << std::endl << std::endl;
    testRegisterRoundtrip();
    testLoginRoundtrip();
    testLoginRespRoundtrip();
    testChatRoundtrip();
    testJoinRoomRoundtrip();
    testLeaveRoomRoundtrip();
    testSystemRoundtrip();
    testErrorRoundtrip();
    testHeartbeatRoundtrip();
    testInvalidJsonDecode();
    std::cout << std::endl << "✅ All tests passed!" << std::endl;
    return 0;
}