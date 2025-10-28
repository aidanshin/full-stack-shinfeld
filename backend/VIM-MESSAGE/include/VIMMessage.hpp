#ifndef VIMMESSAGE_HPP
#define VIMMESSAGE_HPP

#include "Connection.hpp"
#include "ThreadSafeQueue.hpp"
#include "WebSocketServer.hpp"
#include "WebSocketFrame.hpp"
#include "VIMPacket.hpp"

class VIMMessage {
    private:
        std::atomic<bool> running{false};

        std::string tcp_IP;
        uint16_t tcp_port;

        std::string ws_IP;
        uint16_t ws_port;

        ThreadSafeQueue<std::vector<uint8_t>> tcp_input_queue;
        std::map<uint16_t, Client> clients;
        std::unique_ptr<Connection> connection;

        ThreadSafeQueue<std::vector<uint8_t>> ws_receiver_queue;
        ThreadSafeQueue<std::vector<uint8_t>> ws_sender_queue;

        std::unique_ptr<WebSocketServer> websocketserver;
        
        std::thread vim_message_handler;

        void startWebSocketServer();                

        void startMessageHandler();
        bool websocketHandler();
        bool tcpHandler();
    public:
        VIMMessage(
            std::string tcpIP,
            uint16_t tcpPort,
            std::string wsIP,
            uint16_t wsPort
        );
        void start();
        void stop();
};

#endif