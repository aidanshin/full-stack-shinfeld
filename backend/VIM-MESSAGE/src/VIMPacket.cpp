// Encoding for the VIM messages 
// Sending data in byte format 
/*
ISSUES IF FRAMING OCCURS FROM FRONT END WE NEED A WAY DEFINING THE END OF A MESSAGE 
like \r\n for HTTP just incase we get multiple frames for one message 

Packet: 

Types:
- 1: SEND MSG 
- 2: Receive MSG
- 3: New User Creation Request
- 4: Confirmation of User Connection 

Packet: 
Type = 1 || 2
Type = 1 bytes
UserID = 4 bytes
MsgID = 2 bytes
MSG = Payload sizes from websocket - 7 bytes for type and userID 
Total Bytes = 7 + MSG size 

TYPE = 3
type = 1 byte 
IP = 4 bytes
PORT = 2 bytes 
Total bytes = 7

Type = 4 
type = 1 byte
IP = 4 bytes 
PORT = 2 bytes 
USERID = 4 bytes 
Total Bytes = 11 

*/

//TODO: Create Message -> returns encoded bytes of data (unique_ptr)
//TODO: Decode Message -> takes bytes return bool if successful, pass in a message object that wil be written to

#include "VIMPacket.hpp"

VIMPacket::VIMPacket(
    uint8_t type,
    uint32_t userId,
    uint16_t msgId,
    std::vector<uint8_t>&& msgData
) : 
    type(type),
    user_id(userId),
    msg_id(msgId),
    msg_data(std::move(msgData))
{}

VIMPacket::VIMPacket(
    uint8_t type,
    uint32_t IP,
    uint16_t port
) : 
    type(type),
    ip(IP),
    port(port)
{}

VIMPacket::VIMPacket(
    uint8_t type,
    uint32_t IP,
    uint16_t port,
    uint32_t userId
) : 
    type(type),
    ip(IP),
    port(port),
    user_id(userId)
{}

uint8_t VIMPacket::getType() const {return type;}
uint32_t VIMPacket::getUserId() const {return user_id;}
uint16_t VIMPacket::getMsgId() const {return msg_id;}
const std::vector<uint8_t>& VIMPacket::getMsgData() const {return msg_data;}
uint32_t VIMPacket::getIP() const {return ip;}
uint16_t VIMPacket::getPort() const {return port;}

template<typename T>
static void appendBits (std::vector<uint8_t>& packet, const T& value) {
    size_t size = sizeof(value);
    
    for(size_t i = size; i > 0; --i) {
        packet.push_back((value >> (8 * (i - 1))) & 0xFF);
    }
}


std::vector<uint8_t> VIMPacket::createPacket(uint8_t type, uint32_t userId, uint16_t msgId, const std::vector<uint8_t>& data) {
    size_t packet_size = HEADER_SIZE + data.size();
    std::vector<uint8_t> packet;
    packet.reserve(packet_size);

    packet.push_back(type);

    appendBits(packet, userId);
    appendBits(packet, msgId);

    packet.insert(packet.end(), data.begin(), data.end());
    return packet;
}

std::vector<uint8_t> VIMPacket::createPacket(uint8_t type, uint32_t IP, uint16_t port) {
    std::vector<uint8_t> packet;
    packet.reserve(HEADER_SIZE);

    packet.push_back(type);
    appendBits(packet, IP);
    appendBits(packet, port);  
    return packet;
}

std::vector<uint8_t> VIMPacket::createPacket(uint8_t type, uint32_t IP, uint16_t port, uint32_t userId) {
    std::vector<uint8_t> packet;
    packet.reserve(VIMPacket::USER_CONFIRMATION_SIZE);

    packet.push_back(type);
    appendBits(packet, IP);
    appendBits(packet, port);
    appendBits(packet, userId);
    return packet;
}

std::optional<VIMPacket> VIMPacket::decodePacket(std::vector<uint8_t>& packet) {
    if (packet.size() <= 0) return std::nullopt;
    uint8_t type = packet[0];
    switch(type) {
        case 1: case 2:
            return decodeMsgPacket(packet);
        case 3: 
            return decodeUserCreation(packet);
        case 4: 
            return decodeUserConfirmation(packet);
        default:
            return std::nullopt;
    }
}

std::optional<VIMPacket> VIMPacket::decodeMsgPacket(std::vector<uint8_t>& packet) {
    uint8_t msg_type = packet[0];
    uint32_t msg_userId = (packet[1] << 24) | (packet[2] << 16) | (packet[3] << 8) | packet[4];
    uint16_t msg_msgId = (packet[5] << 8) | packet[6];
    std::vector<uint8_t> payload(std::make_move_iterator(packet.begin()+7), std::make_move_iterator(packet.end()));
    return std::make_optional(VIMPacket(msg_type, msg_userId, msg_msgId, std::move(payload)));
}


std::optional<VIMPacket> VIMPacket::decodeUserCreation(std::vector<uint8_t>& packet) {
    if (packet.size() < HEADER_SIZE) return std::nullopt;
    uint8_t msg_type = packet[0];
    uint32_t msg_IP = (packet[1] << 24) | (packet[2] << 16) | (packet[3] << 8) | packet[4];
    uint16_t msg_port = (packet[5] << 8) | packet[6];
    return std::make_optional(VIMPacket(msg_type, msg_IP, msg_port));
}

std::optional<VIMPacket> VIMPacket::decodeUserConfirmation(std::vector<uint8_t>& packet) {
    if(packet.size() < USER_CONFIRMATION_SIZE) return std::nullopt;
    uint8_t msg_type = packet[0];
    uint32_t msg_IP = (packet[1] << 24) | (packet[2] << 16) | (packet[3] << 8) | packet[4];
    uint16_t msg_port = (packet[5] << 8) | packet[6];
    uint32_t msg_userId = (packet[7] << 24) | (packet[8] << 16) | (packet[9] << 8) | packet[10];
    return std::make_optional(VIMPacket(msg_type, msg_IP, msg_port, msg_userId));
}