#include "Client.hpp"
#include "Flags.hpp"
#include "Segment.hpp"
#include "Logger.hpp"

Client::Client(
    uint16_t port,
    uint32_t IP,
    uint32_t expectedSeq,
    uint32_t expectedAck,
    uint32_t lastAck,
    uint8_t state,  
    const std::string& filePath          
) : port(port), 
    IP(IP), 
    expected_sequence(expectedSeq), 
    expected_ack(expectedAck),
    last_ack(lastAck),
    state(state),
    filename(filePath)
{
    if (filename.empty()) {
        filename = std::to_string(IP) + "_" + std::to_string(port) + ".dat";
    }
    INFO_SRC("Client [IP=%u PORT=%u] - Created [expectedSeq=%u, expectedAck=%u, lastAck=%u, state=%s, file=%s]",
         IP, port, expected_sequence, expected_ack, last_ack, stateToStr(state).c_str(), filename.c_str());

}

Client::~Client() {
    closeFile();
    INFO_SRC("Client [IP=%u PORT=%u] - Deleted", IP, port);
}

void Client::info() {
    DEBUG_SRC("Client[IP=%u PORT=%u EXPSEQ=%u EXPACK=%u LASTACK=%u STATE=%s lastByteSent=%u totalSizeOfMessagesSent=%u isFineSent=%d]", 
            IP, port, expected_sequence, expected_ack, last_ack, stateToStr(state).c_str(), lastByteSent, totalSizeOfMessagesSent,  static_cast<int>(isFinSent));
}

uint16_t Client::getPort() const {return port;}
uint32_t Client::getIP() const {return IP;}
uint32_t Client::getExpectedSequence() const {return expected_sequence;}
uint32_t Client::getExpectedAck() const {return expected_ack;}
uint32_t Client::getLastAck() const {return last_ack;}
uint8_t Client::getState() const {return state;}
uint32_t Client::getLastByteSent() const {return lastByteSent;}
bool Client::getIsFinSent() const {return isFinSent;}
uint16_t Client::getWindowSize() const {return windowSize;}
std::string Client::getFileName() const {return filename;}
std::shared_ptr<SegmentInfo> Client::getTrackerSeg() {return tracker_segment;}

void Client::setPort(uint16_t p) {port = p;}
void Client::setIP(uint32_t ip) {IP = ip;}
void Client::setExpectedSequence(uint32_t seq) {expected_sequence = seq;}
void Client::setExpectedAck(uint32_t ack) {expected_ack = ack;}
void Client::setLastAck(uint32_t ack) {last_ack = ack;}
void Client::setState(uint8_t s) {
    DEBUG_SRC("Client [IP=%u PORT=%u] - State change: %s -> %s", IP, port, stateToStr(state).c_str(), stateToStr(s).c_str());
    state = s;
}
void Client::setLastByteSent(uint32_t size) {lastByteSent = size;}
void Client::setIsFinSent(bool fin) {isFinSent = fin;}
void Client::setWindowSize(uint16_t size) {windowSize = size;}
void Client::setTrackerSeg(std::shared_ptr<SegmentInfo> seg) {
    tracker_segment = std::move(seg);
    TRACE_SRC("Client[IP=%u PORT=%u] - New Tracker Seg -> SEQ=%u", IP, port, tracker_segment->getSeqNum());
}

Client::TransmissionInfo& Client::getTransmissionInfo() {
    return transmission_info;
}

//BUG: IF values are too big ommit them as they will skew the calculations and caused larger delays. 
void Client::updateTransmissionInfo(double sampleRTT) {
    if(transmission_info.estimatedRTT == 0.0) {
        transmission_info.estimatedRTT = sampleRTT;
        transmission_info.deviationRTT = sampleRTT / 2.0;
    } 
    else {
        transmission_info.estimatedRTT = (1-ALPHA) * transmission_info.estimatedRTT + ALPHA * sampleRTT;
        transmission_info.deviationRTT = (1-BETA) * transmission_info.deviationRTT + BETA * std::abs(transmission_info.estimatedRTT - sampleRTT);
    }

    transmission_info.timeout_interval = transmission_info.estimatedRTT + 4 * transmission_info.deviationRTT;
    TRACE_SRC("Client[updateTransmissionInfo] - Client[%u:%u] timeout_interval=%.2f ms | estimatedRTT=%.2f ms | devRTT=%.2f ms", IP, port, transmission_info.timeout_interval, transmission_info.estimatedRTT, transmission_info.deviationRTT);
}

//FIXME: REDETERMINE THIS LOGIC ON NUMBER OF TIMEOUTS 
void Client::doubleTimeoutInterval() {
    if(transmission_info.timeout_interval < 5000) {
        transmission_info.timeout_interval = transmission_info.timeout_interval * 2.0;
    }
    transmission_info.number_of_timeouts += 1;
}

void Client::checkTrackerSegment(uint32_t seqNum) {
    TRACE_SRC("Client[checkTrackerSegment] - Checking tracker segement for seqNum=%u", seqNum);
    if(tracker_segment) {
        TRACE_SRC("Client[checkTrackerSegment] - Checking tracker segement for seqNum=%u isTracking=%d", seqNum, tracker_segment->isTracking());
        if(tracker_segment->getSeqNum() < seqNum && tracker_segment->isTracking()) {
            auto now = std::chrono::steady_clock::now();
            double sampleRTT = std::chrono::duration_cast<std::chrono::milliseconds>(now - tracker_segment->getTimeSent()).count();
            if(sampleRTT < 1.0) {
                sampleRTT = 1000.0;
                TRACE_SRC("Client[checkTrackerSegment] - RTT too small, using default of 1 second");
            }
            else if (sampleRTT > 5000) {
                sampleRTT = 5000;
                TRACE_SRC("Client[checkTrackerSegment] - RTT too large, using default of 5 seconds to account for delay");
            }
            TRACE_SRC("Client[checkTrackerSegment] - Calculated sampleRTT=%.2f ms", sampleRTT);
            updateTransmissionInfo(sampleRTT);
            tracker_segment = nullptr;
        }
    } else {
        TRACE_SRC("Client[checkTrackerSegment] - No tracker segment present");
    }
}

// MESSAGE BUFFER FUNCTIONS
bool Client::checkItemMessageBuffer(uint32_t seq) {
    bool exists = (messageBuffer.count(seq) > 0);
    if (!exists) {
        TRACE_SRC("Client [IP=%u PORT=%u] - Packet[SEQ=%u] not found in Message Buffer", IP, port, seq);
    }
    return exists;
}

bool Client::popItemMessageBuffer(uint32_t seq, std::unique_ptr<Segment>& val) {
    auto it = messageBuffer.find(seq);
    if (it != messageBuffer.end()) {
        TRACE_SRC("Client [IP=%u PORT=%u] - Removed Packet[SEQ=%u]", it->second->getDestinationIP(), it->second->getDestPrt(), it->second->getSeqNum());
        val = std::move(it->second);   
        messageBuffer.erase(it);       
        return true;
    }
    WARNING_SRC("Client [IP=%u PORT=%u] - Packet[SEQ=%u] not found in MessageBuffer", IP, port, seq);
    return false;
}

void Client::setItemMessageBuffer(uint32_t seq, std::unique_ptr<Segment> val) {
    messageBuffer.insert_or_assign(seq, std::move(val));
    DEBUG_SRC("Client [IP=%u PORT=%u] - Received out of order Packet[SEQ=%u EXPSEQ=%u]", IP, port, seq, expected_ack);
}

// MESSAGES SENT FUNCTIONS
void Client::pushMessage(std::shared_ptr<SegmentInfo> seg) {
    uint16_t segSize = Segment::HEADER_SIZE + static_cast<uint16_t>(seg->getDataSize());
    TRACE_SRC("Client [IP=%u PORT=%u] - Packet[SEQ=%u SIZE=%u] Sent and appended", IP, port, seg->getSeqNum(), segSize);
    messagesSent.push(std::move(seg));
    totalSizeOfMessagesSent += segSize;
    DEBUG_SRC("Client [IP=%u PORT=%u] - Increased Size of totalSizeOfMessagesSent\tSIZE: %u", IP, port, totalSizeOfMessagesSent);
}

bool Client::popMessage(std::shared_ptr<SegmentInfo>& seg) {
    if (messagesSent.empty()) return false;
    uint16_t segSize = Segment::HEADER_SIZE + static_cast<uint16_t>(messagesSent.front()->getDataSize());
    seg = std::move(messagesSent.front());
    messagesSent.pop();
    totalSizeOfMessagesSent -= segSize;
    DEBUG_SRC("Client [IP=%u PORT=%u] - Packet[SEQ=%u, SIZE=%u] popped. TotalSentSize=%u", 
        IP, port, seg->getSeqNum(), segSize, totalSizeOfMessagesSent);
    return true;
}

void Client::copyFront(std::shared_ptr<SegmentInfo>& seg) {
    if(messagesSent.empty()) return;
    uint16_t segSize = Segment::HEADER_SIZE + static_cast<uint16_t>(messagesSent.front()->getDataSize());
    seg = messagesSent.front();
    totalSizeOfMessagesSent += segSize;
    DEBUG_SRC("Client [IP=%u PORT=%u] - Packet[SEQ=%u, SIZE=%u] to be retransmitted.", 
        IP, port, seg->getSeqNum(), segSize);
}

std::chrono::steady_clock::time_point Client::getMessageTimeSent() {
    if(messagesSent.empty()) return {};
    return messagesSent.front()->getTimeSent();
}

uint32_t Client::getFrontSeqNum() {
    if(messagesSent.empty()) return 0;
    return messagesSent.front()->getSeqNum();
}

bool Client::checkFront(uint32_t ackNum) {
    if (messagesSent.empty()) return false;
    if (messagesSent.front()->getSeqNum() < ackNum) {
        uint16_t segSize = Segment::HEADER_SIZE + static_cast<uint16_t>(messagesSent.front()->getDataSize());
        totalSizeOfMessagesSent -= segSize;
        DEBUG_SRC("Client [IP=%u PORT=%u] - Deleting a Packet that has been sent and Ack'd\tSEQ: %u\tAckNumReceived: %u", IP, port, messagesSent.front()->getSeqNum(), ackNum);
        messagesSent.pop();
        return true;
    } 
    else return false;
}

bool Client::hasMessages() const { return !messagesSent.empty();}
uint16_t Client::sizeMessageSent() const {return totalSizeOfMessagesSent;}
size_t Client::numMessageSentAvailable() const {return messagesSent.size();}


// FILE FUNCTIONS 
void Client::openFile() {
    if(!file.is_open()) {
        file.open(filename, std::ios::binary | std::ios::app);
        if (!file) {
            CRITICAL_SRC("Client [IP=%u PORT=%u] - Failed to open file\tfilename: %s\tIP:port: %u:%u", IP, port, filename.c_str(), IP, port);
            throw std::runtime_error("Failed to open file for client");
        }
        INFO_SRC("Client [IP=%u PORT=%u] - File Opened %s", IP, port, filename.c_str());
    }
}

size_t Client::writeFile(const std::vector<uint8_t>& data) {
    if(!file.is_open()) openFile();
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!file) {
        CRITICAL_SRC("Client [IP=%u PORT=%u] - Failed to write data\tfilename: %s\tIP:port: %u:%u", IP, port, filename.c_str(), IP, port);
        throw std::runtime_error("Failed to write full data to file");
    }
    file.flush();
    DEBUG_SRC("Client[IP:PORT %u:%u] - Successfully Wrote %zu bytes", IP, port, data.size()); 
    return data.size();
}

void Client::closeFile() {
    if(file.is_open()) {
        file.close();
        INFO_SRC("Client [IP=%u PORT=%u] - File Closed", IP, port);
    }
}

void Client::setFileName(const std::string& filePath) {
    if(file.is_open()) file.close();
    filename = filePath;
}