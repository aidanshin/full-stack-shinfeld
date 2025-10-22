#include "VIMPacket.hpp"
#include <iostream>

#include <arpa/inet.h>
#include <stdio.h>
#include <iomanip>  // for std::hex, std::setw, std::setfill

void printPacket(std::vector<uint8_t>& packet) {
    for (uint8_t byte : packet) {
        std::cout << "0x"
                  << std::hex << std::uppercase
                  << std::setw(2) << std::setfill('0')
                  << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << std::endl;  // reset to decimal
}

int main() {
    std::vector<uint8_t> msg = {'a', 'b', 'c', 'd'};
    const char *ip = "127.0.0.1";
    struct in_addr addr;
    inet_pton(AF_INET, ip, &addr);
    uint32_t ip_val = htonl(addr.s_addr);
    uint16_t port = 50000;

    std::vector<uint8_t> msgPacket = VIMPacket::createPacket(1, 1, 1, msg);
    
    printPacket(msgPacket);

    std::vector<uint8_t> userRequest = VIMPacket::createPacket(3, ip_val, port);

    printPacket(userRequest);

    std::vector<uint8_t> userConfirm = VIMPacket::createPacket(4, ip_val, port, 1);

    printPacket(userConfirm);

    auto p1 = VIMPacket::decodePacket(msgPacket);
    auto p2 = VIMPacket::decodePacket(userRequest);
    auto p3 = VIMPacket::decodePacket(userConfirm);

    if (p1.has_value()) p1.value().printPacket();
    if (p2.has_value()) p2.value().printPacket();
    if (p3.has_value()) p3.value().printPacket();

    return 0;
}