#include "WebSocketServer.hpp" 
#include "HttpHandler.hpp"

WebSocketServer::WebSocketServer(
    uint16_t port,
    const std::string& ip
) : 
    port(port),
    IP(ip)
{
    INFO_SRC("WebSocketServer Object Created - %u %s", port, ip.c_str());
}

bool WebSocketServer::start() {
    if(!socketInit()) {
        //CRITICAL_SRC() FAILURE
        CRITICAL_SRC("WebSocketServer[start] - Socket Init Failure");
        return false;
    } 

    if(!acceptClient()) {
        CRITICAL_SRC("WebSocketServer[start] - Failure to Accept Client");
        return false;
    }

    if(!establishHttpConnection()) {
        CRITICAL_SRC("WebSocketServer[Start] - Http Establishment Failure");
        //CRITICAL_SRC() FAILURE IN HTTP 
        //CLOSE SOCKET 
        return false;
    }
   
    return true;
}

bool WebSocketServer::end() {
    //close threads
    //close socket 
    if(clientSocket >= 0) close(clientSocket);
    if(serverSocket >= 0) close(serverSocket);
    INFO_SRC("WebSocketServer[end] - Closed sockets");
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
            // std::cout << buffer << std::endl;
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

void WebSocketServer::tempFunc() {
    std::cout << "Server Waiting for websocket data" << std::endl;

    while (true) {
        uint8_t buffer[1024];
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            std::cout << "Connection closed or error" << std::endl;
            break;
        }
        
        std::cout << bytesReceived << std::endl;

        std::cout << "Hex dump: ";
        for (ssize_t i = 0; i < bytesReceived; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << (int)buffer[i] << " ";
        }
        std::cout << std::dec << std::endl;

        // Print printable characters
        std::cout << "Printable: ";
        for (ssize_t i = 0; i < bytesReceived; ++i) {
            if (std::isprint(buffer[i]))
                std::cout << (char)buffer[i];
            else
                std::cout << ".";
        }
        std::cout << std::endl;
        std::string input;
        std::cin >> input;
        if(input == "quit") break;
    }
}