#ifndef FLAGS_HPP
#define FLAGS_HPP

#include <iostream>

enum class FlagType {
    INVALID,
    SYN,
    SYN_ACK,
    FIN,
    FIN_ACK,
    ACK,
    PSH_ACK,
    RST,
    DATA,
};

enum class FLAGS : uint8_t {
    URG = 1 << 5,
    ACK = 1 << 4,
    PSH = 1 << 3,
    RST = 1 << 2,
    SYN = 1 << 1,
    FIN = 1 << 0
}; 


inline uint8_t createFlag(FLAGS f1, FLAGS f2) {
    uint8_t flag1 = static_cast<uint8_t>(f1);
    uint8_t flag2 = static_cast<uint8_t>(f2);
    return flag1 | flag2;
}

constexpr inline FlagType decodeFlags(uint8_t value) {
    constexpr uint8_t VALID_FLAGS =
        static_cast<uint8_t>(FLAGS::URG) |
        static_cast<uint8_t>(FLAGS::ACK) |
        static_cast<uint8_t>(FLAGS::PSH) |
        static_cast<uint8_t>(FLAGS::RST) |
        static_cast<uint8_t>(FLAGS::SYN) |
        static_cast<uint8_t>(FLAGS::FIN);

    if ((value & ~VALID_FLAGS) != 0) return FlagType::INVALID;

    if (value & static_cast<uint8_t>(FLAGS::SYN)) {
        return (value & static_cast<uint8_t>(FLAGS::ACK)) ? FlagType::SYN_ACK : FlagType::SYN;
    }
    if (value & static_cast<uint8_t>(FLAGS::FIN)) {
        return (value & static_cast<uint8_t>(FLAGS::ACK)) ? FlagType::FIN_ACK : FlagType::FIN;
    }
    if (value & static_cast<uint8_t>(FLAGS::RST)) return FlagType::RST;
    if (value & static_cast<uint8_t>(FLAGS::ACK)) {
        return (value & static_cast<uint8_t>(FLAGS::PSH)) ? FlagType::PSH_ACK : FlagType::ACK;
    }
    return FlagType::DATA;
}

inline std::string flagsToStr(uint8_t flag) {
    FlagType flag_type = decodeFlags(flag);
    switch(flag_type) {
        case FlagType::INVALID: return "INVALID"; 
        case FlagType::SYN: return "SYN";
        case FlagType::SYN_ACK: return "SYN_ACK";
        case FlagType::FIN: return "FIN";
        case FlagType::FIN_ACK: return "FIN_ACK";
        case FlagType::ACK: return "ACK";
        case FlagType::PSH_ACK: return "PSH_ACK";
        case FlagType::RST: return "RST";
        case FlagType::DATA: return "DATA";
        default: return "ERROR";
    }
}


#endif 