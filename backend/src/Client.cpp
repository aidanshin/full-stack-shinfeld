#include "Client.hpp"
#include "Connection.hpp"
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

uint16_t Client::getPort() const {return port;}
uint32_t Client::getIP() const {return IP;}
uint32_t Client::getExpectedSequence() const {return expected_sequence;}
uint32_t Client::getExpectedAck() const {return expected_ack;}
uint32_t Client::getLastAck() const {return last_ack;}
uint8_t Client::getState() const {return state;}
uint16_t Client::getLastByteSent() const {return lastByteSent;}
bool Client::getIsFinSent() const {return isFinSent;}
uint16_t Client::getWindowSize() const {return windowSize;}

void Client::setPort(uint16_t p) {port = p;}
void Client::setIP(uint32_t ip) {IP = ip;}
void Client::setExpectedSequence(uint32_t seq) {expected_sequence = seq;}
void Client::setExpectedAck(uint32_t ack) {expected_ack = ack;}
void Client::setLastAck(uint32_t ack) {last_ack = ack;}
void Client::setState(uint8_t s) {
    DEBUG_SRC("Client [IP=%u PORT=%u] - State change: %s -> %s", IP, port, stateToStr(state).c_str(), stateToStr(s).c_str());
    state = s;
}
void Client::setLastByteSent(uint16_t size) {lastByteSent = size;}
void Client::setIsFinSent(bool fin) {isFinSent = fin;}
void Client::setWindowSize(uint16_t size) {windowSize = size;}


time_t Client::getLastTimeMessageSent() const { 
    std::lock_guard<std::mutex> lock(mtx); 
    return lastTimeMessageSent; 
} 

void Client::setLastTimeMessageSent() { 
    std::lock_guard<std::mutex> lock(mtx); 
    lastTimeMessageSent = time(NULL); 
} 
std::function<void()> Client::LastTimeMessageSent() { 
    return [this] {this->setLastTimeMessageSent();}; 
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
    DEBUG_SRC("Client [IP=%u PORT=%u] - Received out of order Packet[SEQ=%u]", IP, port, seq);
}

// MESSAGES SENT FUNCTIONS
void Client::pushMessage(std::unique_ptr<Segment> seg) {
    uint16_t segSize = Segment::HEADER_SIZE + seg->getData().size();
    TRACE_SRC("Client [IP=%u PORT=%u] - Packet[SEQ=%u SIZE=%u] Sent and appended", IP, port, seg->getSeqNum(), segSize);
    messagesSent.push(std::move(seg));
    totalSizeOfMessagesSent += segSize;
    DEBUG_SRC("Client [IP=%u PORT=%u] - Increased Size of totalSizeOfMessagesSent\tSIZE: %u", IP, port, totalSizeOfMessagesSent);
}

bool Client::popMessage(std::unique_ptr<Segment>& seg) {
    if (messagesSent.empty()) return false;
    uint16_t segSize = Segment::HEADER_SIZE + messagesSent.front()->getData().size();
    seg = std::move(messagesSent.front());
    messagesSent.pop();
    totalSizeOfMessagesSent -= segSize;
    DEBUG_SRC("Client [IP=%u PORT=%u] - Packet[SEQ=%u, SIZE=%u] popped. TotalSentSize=%u", 
        IP, port, seg->getSeqNum(), segSize, totalSizeOfMessagesSent);
    return true;
}

bool Client::checkFront(uint32_t ackNum) {
    if (messagesSent.empty()) return false;
    if (messagesSent.front()->getSeqNum() <= ackNum) {
        uint16_t segSize = Segment::HEADER_SIZE + messagesSent.front()->getData().size();
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