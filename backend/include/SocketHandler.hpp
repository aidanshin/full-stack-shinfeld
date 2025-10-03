#ifndef SOCKETHANDLER_HPP
#define SOCKETHANDLER_HPP

#include "NetCommon.hpp"
#include "Segment.hpp"
#include "ThreadSafeQueue.hpp"

class SocketHandler {
    private:
        ThreadSafeQueue<std::unique_ptr<Segment>>& receiverQueue;
        ThreadSafeQueue<std::pair<std::unique_ptr<Segment>, std::function<void()>>>& senderQueue;
        std::vector<uint8_t>& sendBuffer; 

        int socketfd;
        uint16_t port;
        uint32_t selfIP;
        std::atomic<bool> running{false};
        std::thread receiverThread;
        std::thread senderThread;
        
        
        std::function<void()> updateLastTimeSent;

        void receive();

        void send();

    public:

        SocketHandler(
            uint16_t port,
            uint32_t selfIP,
            ThreadSafeQueue<std::unique_ptr<Segment>>& receiverQueue,
            ThreadSafeQueue<std::pair<std::unique_ptr<Segment>, std::function<void()>>>& senderQueue,
            std::vector<uint8_t>& sendBuffer 
        );

        void start();

        void stop();
};

#endif 