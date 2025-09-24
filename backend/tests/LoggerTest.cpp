#include "Logger.hpp" 

int main() {

    Logger::enableFileOutput();

    // uint32_t ack = 1042;
    // uint32_t seq = 1000;
    // uint16_t port = 5000;
    // std::string ip = "127.0.0.1"; 

    // std::string msg = "IP: %s PORT: %u\n\tACK: %u\tSEQ: %u";

    
    TRACE_SRC("%s", "TEST THIS IS A TEST");
    TRACE_SRC("TEST");

    // TRACE("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);
    // INFO("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);
    // DEBUG("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);
    // WARNING("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);
    // ERROR("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);
    // CRITICAL("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);

    // TRACE_SRC("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);
    // INFO_SRC("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);
    // DEBUG_SRC("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);
    // WARNING_SRC("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);
    // ERROR_SRC("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);
    // CRITICAL_SRC("IP: %s PORT: %u\tACK: %u\tSEQ: %u", ip.c_str(), port, ack, seq);

    return 0;
}