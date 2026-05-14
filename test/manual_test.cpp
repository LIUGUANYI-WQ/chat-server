#include "../protocol/message.h"
#include "../protocol/message_types.h"
#include <iostream>
#include <string>

using namespace protocol;

int main() {
    // 创建一条聊天消息
    auto msg = createChat(1, "room1", "Hello world!", "testtoken123", 1001);
    std::cout << "Original Message:" << std::endl;
    std::cout << "  Type: CHAT" << std::endl;
    std::cout << "  Seq: " << msg.header.seq << std::endl;
    std::cout << "  Token: " << msg.header.token << std::endl;

    // 编码
    std::string json_str = encode(msg);
    std::cout << "\nEncoded JSON:" << std::endl;
    std::cout << json_str << std::endl;

    // 解码
    auto decoded_opt = decode(json_str);
    if (!decoded_opt) {
        std::cerr << "Decode failed!" << std::endl;
        return 1;
    }

    auto& decoded = *decoded_opt;
    std::cout << "\nDecoded Message:" << std::endl;
    std::cout << "  Type: " << messageTypeToString(decoded.header.type) << std::endl;
    std::cout << "  Seq: " << decoded.header.seq << std::endl;
    std::cout << "  Token: " << decoded.header.token << std::endl;

    auto* chat_body = static_cast<ChatBody*>(decoded.body.get());
    std::cout << "  Room: " << chat_body->room_id << std::endl;
    std::cout << "  Content: " << chat_body->content << std::endl;
    std::cout << "  Sender UID: " << (chat_body->sender_uid ? std::to_string(*chat_body->sender_uid) : "none") << std::endl;

    return 0;
}