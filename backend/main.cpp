#include <iostream>
#include "Connection.hpp"
#include "Logger.hpp"

int main(int argc, char*argv[]) {

    Logger::setPriority(LogLevel::TRACE);
    Logger::enableFileOutput();

    INFO_SRC("main[main] - Initialized logger and enabled file output");

    uint16_t srcPort, dstPort = 0;
    std::string srcIP, dstIP;

    if (argc == 3) {
        //SERVER
        srcPort = static_cast<uint16_t>(std::stoi(argv[1]));
        srcIP = argv[2];
        
        INFO_SRC("main[main] - Source = %s:%s", argv[2], argv[1]);

    } 
    else if (argc == 5) {
        //CLIENT WITH A SERVER IN MIND 
        srcPort = static_cast<uint16_t>(std::stoi(argv[1]));
        srcIP = argv[2];
        dstPort = static_cast<uint16_t>(std::stoi(argv[3]));
        dstIP = argv[4];

        INFO_SRC("main[main] - Source = %s:%s", argv[2], argv[1]);
        INFO_SRC("main[main] - Target = %s:%s", argv[4], argv[3]);


    } else {
        CRITICAL("main[main] - Invalid number of arguments provided (Allowed 2 or 4) - %i", argc);
        perror("./main source_port source_ip destination_port destination_ip");
        exit(1);
    }

    INFO_SRC("main[main] - InputQueue initialized");
    ThreadSafeQueue<std::string> inputQueue;
    std::unique_ptr<Connection> connection;

    if (!dstPort && dstIP.empty()) {
        connection = std::make_unique<Connection>(srcPort, srcIP, inputQueue);
    } else {
        connection = std::make_unique<Connection>(srcPort, dstPort, srcIP, dstIP, inputQueue);
    }


    connection->connect();

    bool loop = true;
    std::string input;
    while (loop) {
        std::cin >> input;
        if(input == "quit") {
            INFO_SRC("main[main] - quit inputted -> SHUTDOWN CALLED");
            loop = false;
            connection->disconnect();
        }
        else {
            inputQueue.push(input);
        }
    }

    return 0;
}