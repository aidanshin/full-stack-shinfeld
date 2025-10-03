#ifndef CONNECTION_HPP
#define CONNECTION_HPP 

#include "SocketHandler.hpp"
#include "Client.hpp"
#include "Flags.hpp"
#include <map>
#include <ctime>

//TODO: DEFINE DEFAULT START, SEND, 
class Connection {
    

    private:
        static constexpr uint16_t MAX_DATA_SIZE = 1000;
        static constexpr time_t MAX_SEGMENT_LIFE = 20; // typical value is 2 minutes 
        uint16_t source_port;
        uint16_t destination_port;
        std::string source_ip_str;
        std::string destination_ip_str;    
        uint32_t sourceIP;
        uint32_t destinationIP;
        uint16_t window_size{1000};
        uint16_t urgent_pointer{0};
        uint32_t default_sequence_number{1000};
        uint32_t default_ack_number{0};
        
        ThreadSafeQueue<std::unique_ptr<Segment>> receiverQueue;
        ThreadSafeQueue<std::pair<std::unique_ptr<Segment>, std::function<void()>>> senderQueue;
        ThreadSafeQueue<std::string>& inputQueue;
        
        std::unique_ptr<SocketHandler> socketHandler;
        
        std::map<uint16_t, Client> clients;
        std::vector<uint8_t> sendBuffer; 
        

        std::atomic<bool> running{false};
        std::atomic<bool> timeToClose{false};
        mutable std::mutex mtx;
        std::thread communicationThread;
        std::condition_variable safeToClose;
        bool canCloseDown = false;

        void communicate();
        
        void createMessage(uint16_t srcPort, uint16_t dstPrt, uint32_t seqNum, uint32_t ackNum, uint8_t flag, uint16_t window, uint16_t urgentPtr, uint32_t dstIP, uint8_t state, uint32_t start, uint32_t end);
        void resendMessages(uint16_t port);
        void sendMessages(uint16_t port, size_t dataWritten=0);

        void messageHandler(std::unique_ptr<Segment> seg, size_t dataWritten=0);
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

};

#endif