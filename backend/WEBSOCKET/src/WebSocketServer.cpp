#include "WebSocketServer.hpp" 
#include "HttpHandler.hpp"

WebSocketServer::WebSocketServer(
    ThreadSafeQueue<std::vector<uint8_t>>& recQ,
    ThreadSafeQueue<std::vector<uint8_t>>& senQ,
    uint16_t port,
    const std::string& ip
) : 
    port(port),
    IP(ip),
    receiverQueue(recQ),
    senderQueue(senQ)
{
    INFO_SRC("WebSocketServer Object Created - %s:%u", ip.c_str(), port);
}

bool WebSocketServer::start() {
    if(!socketInit()) {
        CRITICAL_SRC("WebSocketServer[start] - Socket Init Failure");
        return false;
    } 

    if(!acceptClient()) {
        CRITICAL_SRC("WebSocketServer[start] - Failure to Accept Client");
        return false;
    }

    if(!establishHttpConnection()) {
        CRITICAL_SRC("WebSocketServer[Start] - Http Establishment Failure");
        return false;
    }
    
    running = true;
    senderThread = std::thread(&WebSocketServer::sendData, this);
    receiverThread = std::thread(&WebSocketServer::receiveData, this);
    INFO_SRC("WebSocketServer[Start] - Connection Established - Sender and Receiver Threads have started");

    return true;
}

bool WebSocketServer::stop() {
    INFO_SRC("WebSocketServer[stop] - Closing down sockets and threads");
    running = false;
    senderQueue.close();
    receiverQueue.close();

    shutdown(clientSocket, SHUT_RDWR);
    shutdown(serverSocket, SHUT_RDWR);

    if(receiverThread.joinable()) receiverThread.join();
    if(senderThread.joinable()) senderThread.join();

    if(clientSocket >= 0) close(clientSocket);
    if(serverSocket >= 0) close(serverSocket);

    INFO_SRC("WebSocketServer[end] - Closed sockets and threads");

    return true;
}

bool WebSocketServer::socketInit(int listenQueue) {
   
    INFO_SRC("WebSocketServer[socketInit] - Creating WebSocketServer Socket");
    if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ERROR_SRC("WebSocketServer[socketInit] - Socket creation failure");
        return false;
    }

    struct sockaddr_in self_addr;
    memset(&self_addr, 0, sizeof(self_addr));

    self_addr.sin_family = AF_INET;
    self_addr.sin_addr.s_addr = INADDR_ANY; // TODO: FUTURE MIGHT WANT TO SET THIS AS THE IP OF THE WEB BROWSER - PASS THIS AS ARG
    self_addr.sin_port = htons(port);

    INFO_SRC("WebSocketServer[socketInit] - Binding to port %u", port);
    if(bind(serverSocket, (const struct sockaddr *)& self_addr, sizeof(self_addr)) < 0) {
        ERROR_SRC("WebSocketServer[socketInit] - Bind Failure");
        return false;
    }

    struct timeval tv = {.tv_sec = 5, .tv_usec=0};

    INFO_SRC("WebSocketServer[socketInit] - Setting timeout for socket [SEC=%ld USEC=%ld]", (long)tv.tv_sec, (long)tv.tv_usec);
    if (setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        ERROR_SRC("WebSocketServer[socketInit] - Set Timeout Failure");
        return false;
    }

    if(listen(serverSocket, listenQueue) < 0) {
        ERROR_SRC("WebSocketServer[socketInit] - Listen Failure");
        return false;
    }
    
    return true;
}

bool WebSocketServer::acceptClient() {
    INFO_SRC("WebSocketServer[acceptClient] - Waiting for TCP connection request");

    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if ((clientSocket = accept(serverSocket, (struct sockaddr*)& client_addr, &client_addr_len)) < 0) {
        ERROR_SRC("WebSocketServer[acceptClient] - Failure creating clientSocket");
        return false;
    }
    INFO_SRC("WebSocketServer[acceptClient] - New Connection from IP=%s PORT=%u", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    return true;
}

bool WebSocketServer::establishHttpConnection() {
    bool established = false;
    
    while(!established) {
        char buffer[1024];
        ssize_t n;
        if((n = recv(clientSocket, buffer, sizeof(buffer)-1, 0)) > 0) {
            buffer[n] = '\0';

            HttpHandler handler;
            if(!handler.validateRequest(std::string(buffer))) {
                WARNING_SRC("WebSocketServer[establihsHttpConnection] - Invalid request");
                continue;
            };
            std::string response = handler.generateResponse();
            if((n = send(clientSocket, response.data(), response.length(), 0)) < 0 ) {
                RETURN_ERROR("WebSocketServer[establishHttpConnection] - Failed sending response)");
            }  
            
            established = true;
        }
    }

    return true;

}

bool WebSocketServer::sendData() {
    INFO_SRC("WebSocketServer[sendData] - Function Called");

    while (running) {
        
        auto messageToSend = senderQueue.pop();

        if(!messageToSend) {
             WARNING_SRC("WebSocketServer[sendData] - senderQueue return nullptr, ignoring response");
             continue;
        }
       
        std::vector<uint8_t> msg = std::move(*messageToSend);

        ssize_t bytesSent = send(clientSocket, msg.data(), msg.size(), 0);
        if(bytesSent < 0 ) {
            ERROR_SRC("WebSocketServer[sendData] - Failure sending data (errno: %d)", errno);
        } else {
            TRACE_SRC("WebSocketServer[sendData] - Successfully sent %zd bytes", bytesSent);
        }
    }

    return true;
}

bool WebSocketServer::receiveData() {
    INFO_SRC("WebSocketServer[receiveData] - Function Called");
    uint8_t buffer[1024];

    while (running) {
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        
        if(bytesReceived <=0) {
            WARNING_SRC("WebSocketServer[receivedData] - Failure during recv()");
            break;
        }
        receiverQueue.push(std::vector<uint8_t>(buffer, buffer+bytesReceived));
        TRACE_SRC("WebSocketServer[receiveData] - Received %zd bytes, pushed to queue", bytesReceived);
    }

    return true;
}
