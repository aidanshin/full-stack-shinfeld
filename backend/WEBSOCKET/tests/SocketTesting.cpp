#include "WebSocketServer.hpp"

int main(int argc, const char* arvg[]) {
    Logger::setPriority(LogLevel::TRACE);
    Logger::enableFileOutput("logs/SocketTesting.log");

    WebSocketServer server;
    if(!server.start()) CRITICAL_SRC("SocketTesting[main] - start return false");
    server.tempFunc();
    if(!server.end()) CRITICAL_SRC("SocketTesting[main] - end return false");    

    return 0;
}