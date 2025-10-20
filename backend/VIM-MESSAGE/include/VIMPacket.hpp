#ifndef VIMPACKET_HPP 
#define VIMPACKET_HPP

#include <vector>
#include <optional> 


class VIMPacket {
    private:
        uint8_t type{0};
        uint32_t user_id{0};
        uint16_t msg_id{0};
        
        uint32_t ip{0};
        uint16_t port{0};

        std::vector<uint8_t> msg_data{};

        static std::optional<VIMPacket> decodeMsgPacket(std::vector<uint8_t>& packet);
        static std::optional<VIMPacket> decodeUserCreation(std::vector<uint8_t>& packet);
        static std::optional<VIMPacket> decodeUserConfirmation(std::vector<uint8_t>& packet);

    public:
        static constexpr size_t HEADER_SIZE = 7;
        static constexpr size_t USER_CONFIRMATION_SIZE = 11;
        VIMPacket(
            uint8_t type,
            uint32_t userId,
            uint16_t msgId,
            std::vector<uint8_t>&& msgData
        );
        
        VIMPacket(
            uint8_t type,
            uint32_t ip,
            uint16_t port
        );
        
        VIMPacket(
            uint8_t type,
            uint32_t ip,
            uint16_t port,
            uint32_t userId
        );

        uint8_t getType() const;
        uint32_t getUserId() const;
        uint16_t getMsgId() const;
        const std::vector<uint8_t>& getMsgData() const;
        uint32_t getIP() const;
        uint16_t getPort() const;

        static std::vector<uint8_t> createPacket(uint8_t type, uint32_t userId, uint16_t msgId, const std::vector<uint8_t>& data);
        static std::vector<uint8_t> createPacket(uint8_t type, uint32_t IP, uint16_t port); 
        static std::vector<uint8_t> createPacket(uint8_t type, uint32_t IP, uint16_t port, uint32_t userId); 

        static std::optional<VIMPacket> decodePacket(std::vector<uint8_t>& packet);

};


#endif 
