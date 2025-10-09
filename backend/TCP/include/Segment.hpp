#ifndef SEGMENT_HPP
#define SEGMENT_HPP

#include <cstdint>
#include <vector>
#include <memory>


class Segment { 
    private: 
        uint16_t source_port_address;
        uint16_t destination_port_address;
        uint32_t sequence_number;
        uint32_t acknowledgement_number;
        uint8_t header_length;
        uint8_t reserved;
        uint8_t flags;
        uint16_t window_size;
        uint16_t checksum;
        uint16_t urgent_pointer;
        uint32_t destinationIP;

        uint32_t start;
        uint32_t end;
        std::vector<uint8_t> data;
 
        uint16_t create_checksum(uint32_t sourceIP, uint32_t destinationIP, uint8_t protocol, const std::vector<uint8_t>& bytes);
        static bool check_checksum(uint32_t sourceIP, uint32_t destinationIP, uint8_t protocol, const std::vector<uint8_t>& bytes);
        
    public:
        static constexpr uint8_t HEADER_SIZE = 20;

        Segment(
            uint16_t srcPort,
            uint16_t destPort,
            uint32_t seqNum,
            uint32_t ackNum,
            uint8_t flags,
            uint16_t window,
            uint16_t urgentPtr,
            uint32_t destinationIP,
            uint32_t start,
            uint32_t end
        );

        Segment(
            uint16_t srcPort,
            uint16_t destPort,
            uint32_t seqNum,
            uint32_t ackNum,
            uint8_t flags,
            uint16_t window,
            uint16_t urgentPtr,
            uint32_t destinationIP,
            std::vector<uint8_t> payload
        );
        
        std::vector<uint8_t> encode(uint32_t sourceIP, uint32_t destinationIP, uint8_t protocol);
        
        static std::unique_ptr<Segment> decode(uint32_t sourceIP, uint32_t destinationIP, uint8_t protocol, const std::vector<uint8_t>& bytes);
        
        const std::vector<uint8_t>& getData() const;
        void setData(const std::vector<uint8_t>& payload);

        uint16_t getSrcPrt() const;
        uint16_t getDestPrt() const;
        uint32_t getSeqNum() const;
        uint32_t getAckNum() const;
        uint8_t getFlags() const;
        uint16_t getWindowSize() const;
        uint16_t getUrgentPointer() const;
        uint32_t getDestinationIP() const;
        uint32_t getStart() const;
        uint32_t getEnd() const;

        void setSeqNum(uint32_t val);
        void setAckNum(uint32_t val);
        void setFlag(uint8_t flag);
        void setWindowSize(uint16_t val);
        void setUrgentPointer(uint16_t val);
        void setDestinationIP(uint32_t ip);
        void setStart(uint32_t start);
        void setEnd(uint32_t end);


        void printSegment();

};

#endif