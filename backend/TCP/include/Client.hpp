#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <fstream>
#include <stdexcept>
#include <map>
#include <cstring> 
#include <queue>
#include <ctime>

#include "Segment.hpp"
#include "SegmentInfo.hpp"

class Client {
    private:    
        inline static constexpr double ALPHA = 0.125;
        inline static constexpr double BETA = 0.25;

        uint16_t port{0};                                               // PORT
        uint32_t IP{0};                                                 // IP
        uint32_t expected_sequence{0};                                  // Expected Sequence from Sender (Next Sequence to Send to Client)
        uint32_t expected_ack{0};                                       // Expected Ack from Sender (Acknowledge data received from Client - last seq num received + 1)
        uint32_t last_ack{0};                                           // Last Segment ACK from Client
        uint8_t state{0};                                               // CURRENT STATE

        std::map<uint32_t, std::unique_ptr<Segment>> messageBuffer{};   // Message Buffer holding out of order packets
        std::queue<std::shared_ptr<SegmentInfo>> messagesSent;          // QUEUE holding un-ACK messages (can retransmit)
        std::shared_ptr<SegmentInfo> tracker_segment{};                 // Last Time Sent a message

        uint16_t totalSizeOfMessagesSent{0};                            // total size of QUEUE in bytes
        uint32_t lastByteSent{0};                                       // Last Byte Sent from Data
        uint16_t windowSize{0};
        bool isFinSent{false};                                          // True when we have created a FIN and appended to the queue

        // ms
        struct TransmissionInfo {
            double estimatedRTT=0.0;
            double deviationRTT=0.0;
            double timeout_interval=10000.0;
            uint8_t number_of_timeouts=0;
        };

        TransmissionInfo transmission_info;

        std::string filename;                                           // FILENAME FOR DATA RECEIVED
        std::ofstream file;                                             // ofstream of file
    public:
        Client() = default;
        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        // Client(Client&&) noexcept = default;
        // Client& operator = (Client&&) noexcept = default;

        explicit Client(
            uint16_t port,
            uint32_t IP,
            uint32_t expectedSeq,
            uint32_t expectedAck,
            uint32_t lastAck,
            uint8_t state,  
            const std::string& filePath = ""          
        );

        ~Client();

        uint16_t getPort() const;
        uint32_t getIP() const;
        uint32_t getExpectedSequence() const;
        uint32_t getExpectedAck() const;
        uint32_t getLastAck() const;
        uint8_t getState() const;
        uint32_t getLastByteSent() const;
        bool getIsFinSent() const;
        uint16_t getWindowSize() const;
        std::shared_ptr<SegmentInfo> getTrackerSeg();
        TransmissionInfo& getTransmissionInfo();
        
        void setPort(uint16_t p);
        void setIP(uint32_t ip);
        void setExpectedSequence(uint32_t seq);
        void setExpectedAck(uint32_t ack);
        void setLastAck(uint32_t ack);
        void setState(uint8_t s);
        void setLastByteSent(uint32_t size);
        void setTrackerSeg(std::shared_ptr<SegmentInfo> seg);
        
        void updateTransmissionInfo(double sampleRTT);
        void doubleTimeoutInterval();

        void checkTrackerSegment(uint32_t seqNum);

        void setIsFinSent(bool fin);
        void setWindowSize(uint16_t size);

        bool checkItemMessageBuffer(uint32_t seq);
        bool popItemMessageBuffer(uint32_t seq, std::unique_ptr<Segment>& val);
        void setItemMessageBuffer(uint32_t seq, std::unique_ptr<Segment> val);

        void pushMessage(std::shared_ptr<SegmentInfo> seg);
        std::chrono::steady_clock::time_point getMessageTimeSent();
        uint32_t getFrontSeqNum();
        bool popMessage(std::shared_ptr<SegmentInfo>& seg);
        bool checkFront(uint32_t ackNum);
        void copyFront(std::shared_ptr<SegmentInfo>& seg);

        bool hasMessages() const;
        uint16_t sizeMessageSent() const;
        size_t numMessageSentAvailable() const;

        void openFile();
        size_t writeFile(const std::vector<uint8_t>& data);
        void closeFile();
        void setFileName(const std::string& filePath);
        std::string getFileName() const;
        
        void info();
};

#endif