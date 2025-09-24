#ifndef CONNECTION_HPP
#define CONNECTION_HPP 

#include "SocketHandler.hpp"
#include "Client.hpp"
#include "Flags.hpp"
#include <map>
#include <ctime>

enum class STATE : uint8_t{
            NONE,
            SYN_SENT,
            SYN_RECEIVED,
            ESTABLISHED,
            FIN_WAIT_1,
            FIN_WAIT_2,
            TIME_WAIT,
            CLOSING,
            CLOSED,
            LAST_ACK
        };

inline std::string stateToStr(uint8_t state) {
    switch(state) {
        case static_cast<uint8_t>(STATE::NONE): return "NONE"; 
        case static_cast<uint8_t>(STATE::SYN_SENT): return "SYN_SENT";
        case static_cast<uint8_t>(STATE::SYN_RECEIVED): return "SYN_RECEIVED";
        case static_cast<uint8_t>(STATE::ESTABLISHED): return "ESTABLISHED";
        case static_cast<uint8_t>(STATE::FIN_WAIT_1): return "FIN_WAIT_1";
        case static_cast<uint8_t>(STATE::FIN_WAIT_2): return "FIN_WAIT_2";
        case static_cast<uint8_t>(STATE::TIME_WAIT): return "TIME_WAIT";
        case static_cast<uint8_t>(STATE::CLOSING): return "CLOSING";
        case static_cast<uint8_t>(STATE::CLOSED): return "CLOSED";
        case static_cast<uint8_t>(STATE::LAST_ACK): return "LAST_ACK";
        default: return "ERROR";
    }
}

class Connection {
    

    private:
        static constexpr uint16_t MAX_DATA_SIZE = 1000;
        static constexpr time_t MAX_SEGMENT_LIFE = 5; // typical value is 2 minutes 
        uint16_t source_port;
        uint16_t destination_port;
        std::string source_ip_str;
        std::string destination_ip_str;    
        uint32_t sourceIP;
        uint32_t destinationIP;
        // uint32_t window_slider{0};
        uint16_t window_size{100};
        uint16_t urgent_pointer{0};
        uint32_t default_sequence_number{1000};
        uint32_t default_ack_number{0};
        
        ThreadSafeQueue<std::unique_ptr<Segment>> receiverQueue;
        ThreadSafeQueue<std::pair<Segment*, std::function<void()>>> senderQueue;
        ThreadSafeQueue<std::string>& inputQueue;
        
        std::unique_ptr<SocketHandler> socketHandler;
        
        std::map<uint16_t, Client> clients;
        std::vector<uint8_t> dataToSend; 
        uint16_t lastByteSent{0}; // data number of byte - subtract 1 to get index of dataToSend


        std::atomic<bool> running{false};
        std::atomic<bool> timeToClose{false};
        mutable std::mutex mtx;
        std::thread communicationThread;
        std::condition_variable safeToClose;
        bool canCloseDown = false;

        void communicate();
        
        void createMessage(uint16_t srcPort, uint16_t dstPrt, uint32_t seqNum, uint32_t ackNum, uint8_t flag, uint16_t window, uint16_t urgentPtr, uint32_t dstIP, std::vector<uint8_t> data, uint8_t state);
        void resendMessages(uint16_t port, uint32_t newAck);

        void messageHandler(std::unique_ptr<Segment> seg);
        void messageResendCheck();

    public:

        Connection (
            uint16_t srcPort,
            uint16_t destPort,
            std::string srcIPstr,
            std::string destIPstr,
            ThreadSafeQueue<std::string>& q
        );

        Connection (
            uint16_t srcPort,
            std::string srcIPstr,
            ThreadSafeQueue<std::string>& q
        );

        Connection() = delete;
        
        void connect();
        void disconnect();

        uint16_t getWindowSize() const {return window_size;}
        void setWindowSize(uint16_t val) {window_size = val;}
        
        uint16_t getUrgentPointer() const {return urgent_pointer;}
        void setUrgentPointer(uint16_t val) {urgent_pointer = val;}

        uint32_t getDefaultSequenceNumber() const {return default_sequence_number;}
        void setDefaultSequenceNumber(uint32_t val) {default_sequence_number = val;}

        uint32_t getDefaultAckNumber() const {return default_ack_number;}
        void setDefaultAckNumber(uint32_t val) {default_ack_number = val;}

        uint16_t getLastByteSent() const {return lastByteSent;}
        void setLastByteSent(uint16_t val) {lastByteSent = val;}

};

#endif