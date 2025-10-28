#include "VIMMessage.hpp"
#include <csignal>

std::atomic<bool> running(true);

void signalHandler(int signum) {
    std::cout << "\n Caught Crtl-C (SIGINT). Stopping..." <<std::endl;
    running = false;
}

int main(int argc, const char* argv[]) {
    
    if (argc != 5) {
        std::cout << "Valid Input: ./main tcpIP tcpPort wsIP wsPort" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string tcpIP, wsIP;
    uint16_t tcpPort, wsPort;

    tcpIP = argv[1];
    wsIP = argv[3];
    tcpPort = static_cast<uint16_t>(std::stoi(argv[2]));
    wsPort = static_cast<uint16_t>(std::stoi(argv[4]));

    Logger::setPriority(LogLevel::TRACE);
    Logger::enableFileOutput();

    INFO_SRC("main[main] - Initialized logger and enabled file output");

    std::signal(SIGINT, signalHandler);

    VIMMessage vim_message(tcpIP, tcpPort, wsIP, wsPort);

    vim_message.start();

    while(running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::cout << "Calling vim_message.stop()" << std::endl;
    vim_message.stop();
    std::cout<< "VIMMessage stopped." << std::endl;
    
    return 0;
}