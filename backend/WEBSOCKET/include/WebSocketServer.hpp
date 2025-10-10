#ifndef WEBSOCKETSERVER_HPP
#define WEBSOCKETSERVER_HPP

#include "Logger.hpp"
#include "NetCommon.hpp"
#include "ThreadSafeQueue.hpp"

class WebSocketServer {
    private:
        static constexpr uint16_t DEFAULT_PORT = 8080;
        static inline const char* DEFAULT_IP = "127.0.0.1";
        static constexpr int DEFAULT_LISTEN = 5;
        uint16_t port;
        std::string IP;
        int serverSocket;
        int clientSocket;

        ThreadSafeQueue<std::vector<uint8_t>>& receiverQueue;
        ThreadSafeQueue<std::vector<uint8_t>>& senderQueue;

        std::thread receiverThread;
        std::thread senderThread;

        std::atomic<bool> running{false};


        bool socketInit(int listenQueue=DEFAULT_LISTEN); 
        bool acceptClient();
        bool establishHttpConnection();

        bool sendData();
        bool receiveData();
    public:

        WebSocketServer(
            ThreadSafeQueue<std::vector<uint8_t>>& recQ,
            ThreadSafeQueue<std::vector<uint8_t>>& senQ,
            uint16_t port=DEFAULT_PORT, 
            const std::string& ip=DEFAULT_IP
        );
        

        bool start();
        bool stop();
};

#endif