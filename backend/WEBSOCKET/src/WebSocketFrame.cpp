#include "WebSocketFrame.hpp"
#include "Logger.hpp"

WebSocketFrame::WebSocketFrame(
    bool FIN,
    bool RSV1,
    bool RSV2,
    bool RSV3,
    uint8_t OPCODE,
    bool MASK,
    uint64_t payloadLength,
    uint32_t maskKey,
    std::vector<uint8_t> payload
) :
    payload_length(payloadLength),
    mask_key(maskKey),
    payload(payload)
{
    uint8_t byte = 0;
    byte |= FIN << 7;
    byte |= RSV1 << 6;
    byte |= RSV2 << 5;
    byte |= RSV3 << 4;
    byte |= OPCODE;  
    fin_opcode_byte.byte = byte;


    byte = 0;
    byte |= MASK << 7;

    uint8_t payload_len;
    if (payloadLength <= 125) payload_len = payloadLength;
    else if(payloadLength <= UINT16_MAX) payload_len = 126;
    else payload_len = 127;
    byte |= payload_len;  

    mask_length_byte.byte = byte; 
}

std::vector<uint8_t> WebSocketFrame::getPayload() const {
    return payload;
}

void WebSocketFrame::setPayload(std::vector<uint8_t> data) {
    payload = std::move(data);
}

void WebSocketFrame::setMaskKey(uint32_t key) {
    mask_key = key;
}

uint32_t WebSocketFrame::getMaskKey() const {
    return mask_key;
}

void WebSocketFrame::removeMask() {
    mask_key = 0;
    mask_length_byte.no_mask();
}

std::vector<uint8_t> WebSocketFrame::encodeFrame() {
    std::vector<uint8_t> msg;
    msg.reserve(2 + (mask_length_byte.payload_length() >= 126 ? (mask_length_byte.payload_length() == 127 ? 8 : 2 ) : 0) + (mask_key ? 4 : 0) + payload.size());
    msg.push_back(fin_opcode_byte.byte);
    msg.push_back(mask_length_byte.byte);
    if(mask_length_byte.payload_length() == 126) {
            msg.push_back(static_cast<uint8_t>((payload_length >> 8) & 0xFF));
            msg.push_back(static_cast<uint8_t>(payload_length& 0xFF));
    } 
    else if (mask_length_byte.payload_length() == 127) {
        for(int i = 7; i >= 0; --i) {
            msg.push_back(static_cast<uint8_t>((payload_length >> (i*8)) & 0xFF));
        }
    }

    if (mask_key) {
        for(int i = 3; i >= 0; --i) {
            msg.push_back(static_cast<uint8_t>((mask_key >> (i*8)) & 0xFF));
        }
        size_t maskStart = msg.size()-4;
        for(size_t i = 0; i < payload.size(); ++i) {
            uint8_t mask_byte = msg[maskStart + (i % 4)];
            msg.push_back(payload[i] ^ mask_byte);
        }
    }
    else { msg.insert(msg.end(), payload.begin(), payload.end()); }

    return msg;
}

bool WebSocketFrame::validateOpcode(uint8_t opcode) {
    switch(opcode) {
        case static_cast<uint8_t>(OPCODE::MSG_FRAG): 
        case static_cast<uint8_t>(OPCODE::TEXT): 
        case static_cast<uint8_t>(OPCODE::BINARY):  
        case static_cast<uint8_t>(OPCODE::CLOSED): 
        case static_cast<uint8_t>(OPCODE::PING): 
        case static_cast<uint8_t>(OPCODE::PONG): 
            return true; 
        default: 
            return false;
    }
}

std::unique_ptr<WebSocketFrame> WebSocketFrame::decodeFrame(const std::vector<uint8_t>& frame) {
    if (frame.size() < 2) {
        ERROR_SRC("WebSocketFrame[decodeFrame] - Invalid frame size < 2");
        return nullptr;
    }
    FIN_OPCODE firstByte{frame[0]};
    MASK_LENGTH secondByte{frame[1]};

    if(!validateOpcode(firstByte.opcode())) {
        ERROR_SRC("WebSocketFrame[decodeFrame] - Invalid opcode=%u", firstByte.opcode());
        return nullptr;
    }

    size_t pos = 2;
    uint64_t payloadLength = secondByte.payload_length();
    if(payloadLength == 126) {
        if (frame.size() < pos + 2) {
            ERROR_SRC("WebSocketFrame[decodeFrame] - Invalid frame size (%zu) | payloadLength=126", frame.size());
            return nullptr;
        }
        payloadLength = (static_cast<uint64_t>(frame[pos]) << 8) | frame[pos+1];
        pos += 2;
    } 
    else if (payloadLength == 127) {
        if(frame.size() < pos + 8) {
            ERROR_SRC("WebSocketFrame[decodeFrame] - Invalid frame size (%zu) | payloadLength=127", frame.size());
            return nullptr;
        }   
        payloadLength = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLength = (payloadLength << 8) | frame[pos+i];
        }
        pos += 8;
    }

    uint32_t maskKey = 0;
    if(secondByte.mask()) {
        if (frame.size() < pos + 4) {
            ERROR_SRC("WebSocketFrame[decodeFrame] - Invalid frame size (%zu) | mask key issue", frame.size());
            return nullptr;
        }
        for(int i = 0; i < 4; ++i) {
            maskKey = (maskKey << 8) | frame[pos+i];
        }
        pos += 4;
    }

    if (frame.size() < pos + payloadLength) {
        ERROR_SRC("WebSocketFrame[decodeFrame] - Invalid frame size (%zu) | Expected size=%zu", frame.size(), pos+payloadLength);
        return nullptr;
    }
    std::vector<uint8_t> payload(frame.begin() + pos, frame.begin() + pos + payloadLength);

    if (secondByte.mask()) {
        uint8_t maskBytes[4];
        maskBytes[0] = (maskKey >> 24) & 0xFF;
        maskBytes[1] = (maskKey >> 16) & 0xFF;
        maskBytes[2] = (maskKey >> 8) & 0xFF;
        maskBytes[3] = maskKey & 0xFF;
        for (size_t i = 0; i < payload.size(); ++i) {
            payload[i] ^= maskBytes[i % 4];
        }
    }

    return std::make_unique<WebSocketFrame>(
        firstByte.fin(),
        firstByte.rsv1(),
        firstByte.rsv2(),
        firstByte.rsv3(),
        firstByte.opcode(),
        secondByte.mask(),
        payloadLength,
        maskKey,
        std::move(payload)
    );
}

void WebSocketFrame::printFrame() {
    using std::setw;
    using std::setfill;
    using std::left;
    using std::right;
    using std::hex;
    using std::dec;

    std::cout << std::left; // left-align text labels
    std::cout << dec << setfill(' ');
    // ─────────────── Header Row 1 ───────────────
    std::cout << setw(6)  << "FIN" 
              << "|" << setw(6)  << "RSV1"
              << "|" << setw(6)  << "RSV2"
              << "|" << setw(6)  << "RSV3"
              << "|" << setw(8)  << "OPCODE"
              << "\n";

    // Values row 1
    std::cout << setw(6)  << (int)fin_opcode_byte.fin()
              << "|" << setw(6)  << (int)fin_opcode_byte.rsv1()
              << "|" << setw(6)  << (int)fin_opcode_byte.rsv2()
              << "|" << setw(6)  << (int)fin_opcode_byte.rsv3()
              << "|" << setw(8)  << (int)fin_opcode_byte.opcode()
              << "\n\n";

    // ─────────────── Header Row 2 ───────────────
    std::cout << setw(6)  << "MASK"
              << "|" << setw(18) << "PAYLOAD LEN(7bit)"
              << "|" << setw(18) << "ACTUAL LEN"
              << "|" << setw(10) << "MASK KEY"
              << "\n";

    // Values row 2
    std::cout << setw(6)  << (int)mask_length_byte.mask()
              << "|" << setw(18) << (int)mask_length_byte.payload_length()
              << "|" << setw(18) << payload_length
              << "| ";

    if (mask_key) {
        uint8_t maskBytes[4];
        maskBytes[0] = (mask_key >> 24) & 0xFF;
        maskBytes[1] = (mask_key >> 16) & 0xFF;
        maskBytes[2] = (mask_key >> 8) & 0xFF;
        maskBytes[3] = mask_key & 0xFF;

        std::cout << std::hex << std::setfill('0');
        for (int i = 0; i < 4; ++i)
            std::cout << std::setw(2) << (int)maskBytes[i] << " ";
        std::cout << std::dec;
        std::cout << std::setfill(' ');
    } else {
        std::cout << "None";
    }
    std::cout << "\n\n";

    // ─────────────── Payload ───────────────
    std::cout << "PAYLOAD (" << payload.size() << " bytes): ";
    for (uint8_t c : payload) {
        std::cout << "0x"
                  << std::hex << std::uppercase
                  << std::setw(2) << std::setfill('0')
                  << static_cast<int>(c) << " ";
    }

    std::cout << std::dec << "\n";
}
