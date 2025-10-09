#include "ErrorTest.hpp"

void ErrorTest::bitManipulator(std::vector<uint8_t>& msg) {
    if(msg.empty()) return;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> byteDistrib(
        (msg.size() > 20) ? 20 : 4,
        (msg.size() > 20) ? msg.size() - 1 : 19
    );
    int byteIndex = byteDistrib(gen);
    
    std::uniform_int_distribution<> bitDistrib(0,7);
    int bitIndex = bitDistrib(gen);
    
    msg[byteIndex] ^= (1<<bitIndex);
}

bool ErrorTest::bernoulliFunc(double probability) {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::bernoulli_distribution dist(probability);

    return dist(gen);
}

bool ErrorTest::toOutofOrder(double probability) {
    return bernoulliFunc(probability);
}

bool ErrorTest::dropPacket(double probability) {
    return bernoulliFunc(probability);
}