#ifndef WEBSOCKETFRAME_HPP
#define WEBSOCKETFRAME_HPP

#include <vector>
#include <iostream>
#include <iomanip>

class WebSocketFrame {
    private:
        struct FIN_OPCODE {
            uint8_t byte;
            bool fin() const {return byte & 0x80;}
            void toggle_fin() {byte ^= 0x80;}
            bool rsv1() const {return byte & 0x40;}
            bool rsv2() const {return byte & 0x20;}
            bool rsv3() const {return byte & 0x10;}
            uint8_t opcode() const {return byte & 0x0F;}
        };

        struct MASK_LENGTH {
            uint8_t byte;
            void toggle_mask() {byte ^= 0x80;}
            void no_mask() {byte &= 0x7F;}
            bool mask() const {return byte & 0x80;}
            uint8_t payload_length() const {return byte & 0x7F;}
        };

        FIN_OPCODE fin_opcode_byte;
        MASK_LENGTH mask_length_byte;

        uint64_t payload_length;
        uint32_t mask_key;
        std::vector<uint8_t> payload;

        static bool validateOpcode(uint8_t opcode);

    public:
        
        enum class OPCODE : uint8_t {
            MSG_FRAG = 0x0,
            TEXT = 0x1,
            BINARY = 0x2,
            CLOSED = 0x8,
            PING = 0x9,
            PONG = 0xA
        };

        WebSocketFrame(
            bool FIN,
            bool RSV1,
            bool RSV2,
            bool RSV3,
            uint8_t OPCODE,
            bool MASK,
            uint64_t payloadLength,
            uint32_t maskKey,
            std::vector<uint8_t> payload
        );

        std::vector<uint8_t> getPayload() const;
        void setPayload(std::vector<uint8_t> data);
        std::vector<uint8_t>&& extractPayload();
        
        void setMaskKey(uint32_t key);
        uint32_t getMaskKey() const;
        void removeMask();

        std::vector<uint8_t> encodeFrame();
        static std::unique_ptr<WebSocketFrame> decodeFrame(const std::vector<uint8_t>& frame);  
        
        void printFrame();
};
#endif