#ifndef WEBSOCKETSERVER_HPP
#define WEBSOCKETSERVER_HPP

#include "Logger.hpp"
#include "NetCommon.hpp"
/*
TODO: Create Connection via TCP Socket
TODO: Establish HTPP Connection
TODO: Start Receiving and Sending Threads for communication
TODO: Close Connection
*/
class WebSocketServer {
    private:
        static constexpr uint16_t DEFAULT_PORT = 8080;
        static inline const char* DEFAULT_IP = "127.0.0.1";
        static constexpr int DEFAULT_LISTEN = 5;
        uint16_t port;
        std::string IP;
        int serverSocket;
        int clientSocket;

        bool socketInit(int listenQueue=DEFAULT_LISTEN); 
        bool acceptClient();
        // bool handleClient();
        bool establishHttpConnection();
    public:

        WebSocketServer(uint16_t port=DEFAULT_PORT, const std::string& ip=DEFAULT_IP);

        bool start();
        bool end();
        void tempFunc();
};

#endif