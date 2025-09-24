#include <iostream>
#include <iomanip>
#include "Segment.hpp"
#include "Flags.hpp"
#include "Logger.hpp"


Segment::Segment(
    uint16_t srcPort,
    uint16_t destPort,
    uint32_t seqNum,
    uint32_t ackNum,
    uint8_t flags,
    uint16_t window,
    uint16_t urgentPtr,
    uint32_t destinationIP,
    std::vector<uint8_t> payload
) 
:   source_port_address(srcPort), 
    destination_port_address(destPort),
    sequence_number(seqNum),
    acknowledgement_number(ackNum),
    header_length(5),
    reserved(0),
    flags(flags),
    window_size(window),
    checksum(0),
    urgent_pointer(urgentPtr),
    destinationIP(destinationIP),
    data(std::move(payload)) 
{
    INFO_SRC("Segment Created - [srcPrt=%u dstPrt=%u seqNum=%u ackNum=%u flags=%s window=%u urgent=%u destIP=%u dataSize=%zu]",
        source_port_address, destination_port_address, sequence_number, acknowledgement_number, flagsToStr(flags).c_str(), window_size, urgent_pointer, destinationIP, data.size());
}

uint16_t Segment::getSrcPrt() const {
    return source_port_address;
}

uint16_t Segment::getDestPrt() const {
    return destination_port_address;
}

uint32_t Segment::getSeqNum() const {
    return sequence_number;
}

uint32_t Segment::getAckNum() const {
    return acknowledgement_number;
}

uint8_t Segment::getFlags() const {
    return flags;
}

uint16_t Segment::getWindowSize() const {
    return window_size;
}

uint16_t Segment::getUrgentPointer() const {
    return urgent_pointer;
}

const std::vector<uint8_t>& Segment::getData() const {
    return data;
}

uint32_t Segment::getDestinationIP() const {
    return destinationIP;
}


// Setters

void Segment::setSeqNum(uint32_t val) {
    sequence_number = val;
}

void Segment::setAckNum(uint32_t val) {
    acknowledgement_number = val;
}

void Segment::setFlag(uint8_t val) {
    flags = val;
}   

void Segment::setWindowSize(uint16_t val) {
    window_size = val;
}

void Segment::setUrgentPointer(uint16_t val) {
    urgent_pointer = val;
}

void Segment::setData(const std::vector<uint8_t>& payload) {
    data = payload;
}

void Segment::setDestinationIP(uint32_t ip) {
    destinationIP = ip;
}


template<typename T>
static void appendBits (std::vector<uint8_t>& packet, const T& value) {
    size_t size = sizeof(value);
    
    for(int i = size; i > 0; --i) {
        packet.push_back((value >> (8 * (i - 1))) & 0xFF);
    }
}

std::vector<uint8_t> Segment::encode(uint32_t sourceIP, uint32_t destinationIP, uint8_t protocol) {
   
    std::vector<uint8_t> packet;

    
    appendBits(packet, source_port_address); // Source port 
    appendBits(packet, destination_port_address); // Destination port
    appendBits(packet, sequence_number); // sequence number
    appendBits(packet, acknowledgement_number); // acknowledgement number 
    packet.push_back((header_length << 4) & 0xFF);  // 4 bits of header_length 4 bits of reserved and flags 
    packet.push_back(flags); // 2 bits reserved & 6 bits of flags
    appendBits(packet, window_size);// window_size
    packet.push_back(0x00); // check_sum
    packet.push_back(0x00); // check_sum
    appendBits(packet, urgent_pointer); //urgent_point
    packet.insert(packet.end(), data.begin(), data.end()); // data

    checksum = create_checksum(sourceIP, destinationIP, protocol, packet);
    
    packet[16] = (checksum >> 8) & 0xFF; // checksum MSB
    packet[17] = checksum & 0xFF; // checksum LSB
    
    if (packet.size() > 1024) {
        WARNING_SRC("Segment - Packet Size[%zu] is larger than MAX_SIZE", packet.size());
    }

    TRACE_SRC("Segment[SRCPRT=%u DSTPRT=%u SEQ=%u ACK=%u FLAG=%s] - Encoded", source_port_address, destination_port_address, sequence_number, acknowledgement_number, flagsToStr(flags).c_str());

    return packet;
}

template<typename T>
T combineBytes(const std::vector<uint8_t>& bytes, int start) {
    size_t size = sizeof(T); 
    T temp = 0 ;

    for (int i = 0; i < size; i++) {
        temp |= T(bytes[i+start]) << (8 * (size-i-1));
    }
    return temp;
}

std::unique_ptr<Segment> Segment::decode(uint32_t sourceIP, uint32_t destinationIP, uint8_t protocol, const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 20 || !check_checksum(sourceIP, destinationIP, protocol, bytes)) {
        DEBUG_SRC("Segment - Invalid Size || Invalid CheckSum");
        return nullptr;
    }
    uint16_t srcPrt = combineBytes<uint16_t>(bytes, 0);
    uint16_t destPrt = combineBytes<uint16_t>(bytes, 2);
    uint32_t seqNum = combineBytes<uint32_t>(bytes, 4);
    uint32_t ackNum = combineBytes<uint32_t>(bytes, 8);
    // uint8_t headerLength = bytes[12];
    uint8_t flags = bytes[13];
    uint16_t window = combineBytes<uint16_t>(bytes, 14);
    uint16_t checksum = combineBytes<uint16_t>(bytes, 16);
    uint16_t urgentPtr = combineBytes<uint16_t>(bytes, 18);

    std::vector<uint8_t> payload;
    if (bytes.size() > 20) {
        copy(bytes.begin() + 20, bytes.end(), back_inserter(payload));
    }
    // std::cout << "Length of payload: " << payload.size() << std::endl;

    std::unique_ptr<Segment> segment = std::make_unique<Segment>(srcPrt, destPrt, seqNum, ackNum, flags, window, urgentPtr, sourceIP, std::move(payload));
    segment->checksum = checksum;

    TRACE_SRC("Segment Decoded - [SRCPRT=%u DSTPRT=%u SEQ=%u ACK=%u FLAG=%s WINDOW=%u CHKSUM=%u URGPTR=%u SRCIP=%u DATASIZE=%zu]", 
        srcPrt, destPrt, seqNum, ackNum, flagsToStr(flags).c_str(), window, checksum, urgentPtr, segment->getData().size());

    return segment;
}

static uint32_t calculateCheckSum (uint32_t sourceIP, uint32_t destinationIP, uint8_t protocol, const std::vector<uint8_t>& bytes) {
    size_t segmentSize = bytes.size();
    uint32_t sum = 0;

    sum += ((sourceIP >> 16) & 0xFFFF) + (sourceIP & 0xFFFF);
    sum += ((destinationIP >> 16) & 0xFFFF) + (destinationIP & 0xFFFF);
    sum += static_cast<uint16_t>(protocol);
    sum += static_cast<uint16_t>(segmentSize);

    for (int i = 0; i < segmentSize; i+=2) {
        if (i+1 == segmentSize) {
            sum += uint16_t((bytes[i] << 8) & 0xFFFF);
        } else {
            sum += uint16_t(((bytes[i] << 8) & 0xFFFF) | bytes[i+1]);
        }
    }

    while (sum > 0xFFFF) {
        sum = (sum >> 16) + (sum & 0xFFFF);
    }
    
    return sum;
}

uint16_t Segment::create_checksum(uint32_t sourceIP, uint32_t destinationIP, uint8_t protocol, const std::vector<uint8_t>& bytes) {
    uint32_t sum = calculateCheckSum(sourceIP, destinationIP, protocol, bytes);

    return ~sum;
}

bool Segment::check_checksum(uint32_t sourceIP, uint32_t destinationIP, uint8_t protocol, const std::vector<uint8_t>& bytes) {
    uint32_t sum = calculateCheckSum(sourceIP, destinationIP, protocol, bytes);

    return sum == 0xFFFF;
}

void Segment::printSegment() {

    std::vector<std::pair<std::string, uint64_t>> fields = {
        {"Source Port", source_port_address},
        {"Destination Port", destination_port_address},
        {"Sequence Number", sequence_number},
        {"Acknowledgement Number", acknowledgement_number},
        {"Header Length", header_length},
        {"Reserved", reserved},
        {"Flags", flags},
        {"Window Size", window_size},
        {"Checksum", checksum},
        {"Urgent Pointer", urgent_pointer}
    };

    for (auto& [name, value] : fields) {
        std::cout << std::setw(20) << std::left << name << ": "
             << std::dec << value 
             << " | 0x"
             << std::setw(sizeof(value)*2) 
             << std::hex << std::uppercase << value 
             << "\n";
    }

    std::cout << std::setw(20) << std::left << "Data" << ": ";
    for (uint8_t x : data) std::cout << char(x);
    std::cout << "\n";

    std::cout << std::setw(20) << std::left << "Data" << ": ";
    for (uint8_t x : data) {
        std::cout << std::hex << std::setw(2)  
             << std::uppercase << static_cast<int>(x) << " ";
    }
    std::cout << "\n\n";

    std::cout << std::dec;
}