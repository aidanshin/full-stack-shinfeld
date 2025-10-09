#ifndef ERRORTEST_HPP
#define ERRORTEST_HPP

#include "Logger.hpp"

#include <random>
#include <queue>
class ErrorTest {
    private:
        bool bernoulliFunc(double probability);
    public:
        ErrorTest() = default;
        
        void bitManipulator(std::vector<uint8_t>& msg);
        bool toOutofOrder(double probability);
        bool dropPacket(double probability);
};

#endif 