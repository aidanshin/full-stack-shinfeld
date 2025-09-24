// g++ -std=c++17 -I../include SocketTest.cpp ../src/Segment.cpp ../src/SocketHandler.cpp -o socket_test

#include <iostream>
#include "Segment.hpp"
#include "SocketHandler.hpp"

uint32_t convertIpAddress(const std::string& ip) {
    struct in_addr addr;
    if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) throw std::runtime_error("Invalid IP");
    return ntohl(addr.s_addr);
}

int main(int argc, char*argv[]) {

    if (argc != 6) {
        std::cout << "Invalid Input." << std::endl;
        std::cout << "./tcp_program source_port destination_port source_ip destination_ip payload" << std::endl;
        exit(1);
    }
    
    uint16_t srcPort = static_cast<uint16_t>(std::stoi(argv[1]));
    uint16_t destPort = static_cast<uint16_t>(std::stoi(argv[2]));
    uint32_t seqNum = 1000;
    uint32_t ackNum = 100;
    uint8_t flags = 32;
    uint16_t window = 100;
    uint16_t urgentPtr = 0;
    std::string destinationStr = argv[3];
    std::string ipStr = argv[4];
    std::vector<uint8_t> payload(reinterpret_cast<uint8_t*>(argv[5]), reinterpret_cast<uint8_t*>(argv[5]) + strlen(argv[5]));

    uint32_t sourceIP = convertIpAddress(ipStr);
    uint32_t destinationIP = convertIpAddress(destinationStr);
    
    ThreadSafeQueue<std::unique_ptr<Segment>> receiverQueue;
    ThreadSafeQueue<std::unique_ptr<Segment>> senderQueue;

    SocketHandler sockfd(srcPort, ntohl(sourceIP), receiverQueue, senderQueue);
    sockfd.start();
    
    auto segPtr = std::make_unique<Segment>(srcPort, destPort, seqNum, ackNum, flags, window, urgentPtr, destinationIP, payload);
    segPtr->printSegment();

    sleep(5);
    senderQueue.push(std::move(segPtr));

    std::unique_ptr<Segment> receivedSeg = receiverQueue.pop();

    if (receivedSeg) {
        receivedSeg->printSegment();
    }

    receiverQueue.close();
    senderQueue.close();

    sockfd.stop();

    return 0;
}