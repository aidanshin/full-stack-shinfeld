#include "Connection.hpp"
#include "Logger.hpp"


// TODO: FUNCTION TO CREATE NEW CLIENT CONNECTIONS 
// TODO: ONLY ACCEPTS BUT CANNOT NOT CREATE NEW WHILE RUNNING


//TODO: Change all client reference to auto& client = client->second so do not have to call client->second ...
static uint32_t convertIpAddress(const std::string& ip) {
    struct in_addr addr;
    if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) throw std::runtime_error("Invalid IP");
    return ntohl(addr.s_addr);
}

Connection::Connection (
    uint16_t srcPort,
    uint16_t destPort,
    std::string srcIPstr,
    std::string destIPstr,
    ThreadSafeQueue<std::vector<uint8_t>>& q,
    std::map<uint16_t, Client>& clientsDict
) 
:
    source_port(srcPort),
    destination_port(destPort),
    source_ip_str(srcIPstr),
    destination_ip_str(destIPstr),
    inputQueue(q),
    clients(clientsDict)
{
    sourceIP = convertIpAddress(source_ip_str);
    destinationIP = convertIpAddress(destination_ip_str);
    INFO_SRC("Connection Initialized with Target- [SRCPRT=%u DSTPRT=%u SRCIP=%s DSTIP=%s]", srcPort, destPort, srcIPstr.c_str(), destIPstr.c_str());
}

Connection::Connection (
    uint16_t srcPort,
    std::string srcIPstr,
    ThreadSafeQueue<std::vector<uint8_t>>& q,
    std::map<uint16_t, Client>& clientsDict
)
:
    source_port(srcPort),
    source_ip_str(srcIPstr),
    inputQueue(q),
    clients(clientsDict)
{
    sourceIP = convertIpAddress(source_ip_str);
    
    INFO_SRC("Connection Initialized without Target- [SRCPRT=%u SRCIP=%s]", srcPort, srcIPstr.c_str());
}


void Connection::connect() {
    running = true;

    INFO_SRC("Connection[connect] - Attempting to initialize SocketHandler [IP:%u PORT:%u]", sourceIP, source_port);
    //TODO: CHANGE THE IP TYPE TO BE STR CONST CHAR* AND CONVERT IN SOCKET HANDLER
    socketHandler = std::make_unique<SocketHandler>(source_port, sourceIP, receiverQueue, senderQueue, sendBuffer);
    socketHandler->start();

    if (destination_port != 0 && !destination_ip_str.empty()) {
        INFO_SRC("Connection[connect] - Attempting to create target client [IP:%u PORT:%u]", destinationIP, destination_port);
        clients.try_emplace(destination_port, destination_port, destinationIP, 0, 0, 0, static_cast<uint8_t>(STATE::NONE));
    }

    communicationThread = std::thread(&Connection::communicate, this);
    
    INFO_SRC("Connection[connect] - Communication Thread Called");
}

void Connection::createMessage(uint16_t srcPort, uint16_t dstPrt, uint32_t seqNum, uint32_t ackNum, uint8_t flag, uint16_t window, uint16_t urgentPtr, uint32_t dstIP, uint8_t state, uint32_t start, uint32_t end) {
    if (auto clientIt = clients.find(dstPrt); clientIt != clients.end()) {
        Client& client = clientIt->second;
        DEBUG_SRC("Connection[createMessage] - New Packet Created To Send");

        std::function<void()> func;
        if (flag == static_cast<uint8_t>(FLAGS::SYN) || flag == createFlag(FLAGS::SYN, FLAGS::ACK) || flag == static_cast<uint8_t>(FLAGS::FIN) || flag == createFlag(FLAGS::FIN, FLAGS::ACK) || (flag == static_cast<uint8_t>(FLAGS::ACK) && start >= 0 && end > 0)) {
            std::shared_ptr trackerSeg = std::make_shared<SegmentInfo>(seqNum, flag,  start, end-start, false);
            if(!client.getTrackerSeg()) {
                trackerSeg->setTracking(true);
                client.setTrackerSeg(trackerSeg);
            }
            if(client.getFrontSeqNum() != seqNum) {
                client.pushMessage(trackerSeg);
            }
            func = trackerSeg->LastTimeMessageSent(trackerSeg);
        }

        std::unique_ptr<Segment> seg = std::make_unique<Segment>(srcPort, dstPrt, seqNum, ackNum, flag, window, urgentPtr, dstIP, start, end);
        senderQueue.push(std::pair<std::unique_ptr<Segment>, std::function<void()>>(std::move(seg), func));
        if (client.getState() != state) client.setState(state);
        if (client.getExpectedAck() != ackNum) client.setExpectedAck(ackNum);
    } else {
        WARNING_SRC("Connection[createMessage] - Function called but destination port (%u) is not within Clients", dstPrt);
    }
}

void Connection::sendMessages(uint16_t port, size_t dataWritten) {
    if(auto clientIt = clients.find(port); clientIt != clients.end()) {
        Client& client = clientIt->second;
        if(client.getLastByteSent() == sendBuffer.size()) {
            if(dataWritten) createMessage(source_port, client.getPort(), client.getExpectedSequence(), client.getExpectedAck(), static_cast<uint8_t>(FLAGS::ACK), window_size, urgent_pointer, client.getIP(), client.getState(), 0, 0);
            return;
        }
        INFO_SRC("Connection[sendMessages] - Checking the buffer size and created new packets if possible");
        
        uint16_t sizeMessagesSent = client.sizeMessageSent();
        if (sizeMessagesSent >= client.getWindowSize()) {
            WARNING_SRC("Connection[sendMessages] - Client[IP:PORT %u:%u] messageSentSize=%u > windowSize=%u", client.getIP(), port, sizeMessagesSent, client.getWindowSize());
            if(dataWritten) createMessage(source_port, client.getPort(), client.getExpectedSequence(), client.getExpectedAck(), static_cast<uint8_t>(FLAGS::ACK), window_size, urgent_pointer, client.getIP(), client.getState(), 0, 0);
            return;
        }

        uint16_t bufferAvailable = client.getWindowSize() - sizeMessagesSent;
        bool sentData = false;

        while(bufferAvailable > Segment::HEADER_SIZE && client.getLastByteSent() < sendBuffer.size()) {

            uint32_t start = client.getLastByteSent();
            uint16_t maxData = std::min<uint16_t>(MAX_DATA_SIZE, bufferAvailable-Segment::HEADER_SIZE);
            uint32_t end = std::min<uint32_t>(start + maxData, sendBuffer.size());

            createMessage(
                source_port, 
                client.getPort(), 
                client.getExpectedSequence(), 
                client.getExpectedAck(), 
                static_cast<uint8_t>(FLAGS::ACK), 
                window_size, urgent_pointer, 
                client.getIP(), 
                client.getState(), 
                start, 
                end
            );

            
            client.setLastByteSent(end);

            uint16_t payload = static_cast<uint16_t>(end-start);
            client.setExpectedSequence(client.getExpectedSequence()+payload);
            bufferAvailable -= static_cast<uint16_t>(payload + Segment::HEADER_SIZE);

            sentData = true;
            
            TRACE_SRC("Connection[messageHandler] - bytesSent=%u bufferSizeAvailable=%u", end, bufferAvailable); 

        }

        if(client.getLastByteSent() == sendBuffer.size()) {
            DEBUG_SRC("Connection[sendMessages] - Packets containing all the sendBuffer generated lastByteSent=%u sendBuffer.size=%u", client.getLastByteSent(), sendBuffer.size());
        }

        if(!sentData && dataWritten) {
            createMessage(source_port, client.getPort(), client.getExpectedSequence(), client.getExpectedAck(), static_cast<uint8_t>(FLAGS::ACK), window_size, urgent_pointer, client.getIP(), client.getState(), 0, 0);
        }
        
    } 
}

void Connection::messageHandler(std::unique_ptr<Segment> seg, size_t dataWritten) {
    if (auto clientIt = clients.find(seg->getSrcPrt()); clientIt != clients.end()) {
        Client& client = clientIt->second;
        INFO_SRC("Connection[messageHandler] - Checking if packets sent are ACK'd");

        if(client.hasMessages()) while(client.checkFront(seg->getAckNum()));

        client.setLastAck(seg->getAckNum());
        client.setWindowSize(seg->getWindowSize());

        if (seg->getSeqNum() > client.getExpectedAck()) {
            client.setItemMessageBuffer(seg->getSeqNum(), std::move(seg));
        }
        else {

            sendMessages(client.getPort(), dataWritten);
            if (seg->getFlags() == static_cast<uint8_t>(FLAGS::FIN)) receiverQueue.push(std::move(seg)); // out of order FIN and we push it back to receive queue so then we can run logic as if it just arrived
            if (!client.getIsFinSent() && client.getLastByteSent() == sendBuffer.size()) {
                if(timeToClose || client.getState() == static_cast<uint8_t>(STATE::CLOSING)) {
                    DEBUG_SRC("Connection[messageHandler] - Sent all data to client[IP:%u PORT:%u], creating FIN", client.getIP(), client.getPort());
                    createMessage(source_port, 
                                client.getPort(), 
                                client.getExpectedSequence(), 
                                client.getExpectedAck(), 
                                static_cast<uint8_t>(FLAGS::FIN), 
                                window_size, urgent_pointer, 
                                client.getIP(), 
                                (client.getState() == static_cast<uint8_t>(STATE::CLOSING) ? static_cast<uint8_t>(STATE::LAST_ACK) : static_cast<uint8_t>(STATE::FIN_WAIT_1)),
                                0, 0
                    );
                    client.setIsFinSent(true);
                    client.setExpectedSequence(client.getExpectedSequence()+1);
                }
            }
        }

    } else {
        WARNING_SRC("Connection[messageHandler] - Could not find PORT=%u in clients", seg->getSrcPrt());
    }
}

//BUG: New Message will be added to send messages buffer we need a way to create the message again but not append to send messages. 
//BUG: Make function that checks if client buffer has the seq number already it will not push it in. just check the front 
void Connection::resendMessages(uint16_t port) {
    if(auto clientIt = clients.find(port); clientIt != clients.end()) {
        Client& client = clientIt->second;
        if(client.hasMessages()) {
            std::shared_ptr<SegmentInfo> segToResend;
            client.copyFront(segToResend);
            if(segToResend) {
                createMessage(
                    source_port,
                    client.getPort(),
                    segToResend->getSeqNum(),
                    client.getExpectedAck(),
                    segToResend->getFlag(),
                    client.getWindowSize(),
                    urgent_pointer,
                    client.getIP(),
                    client.getState(),
                    segToResend->getStart(),
                    (segToResend->getStart()+segToResend->getDataSize())
                );
            }
        }
    } else {
        WARNING_SRC("Connection[resendMessages] - Could not find PORT=%u in clients", port);
    }
}

void Connection::messageResendCheck() {
    std::vector<uint16_t> clientsToRemove;

    for(auto& [port, client] : clients) {
        if(client.getState() == static_cast<uint8_t>(STATE::CLOSED)) clientsToRemove.push_back(port);
        else {
            if(client.hasMessages() && client.getMessageTimeSent() != std::chrono::steady_clock::time_point{}) {
                auto now = std::chrono::steady_clock::now();
                double timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - client.getMessageTimeSent()).count();
                if (timeDiff > client.getTransmissionInfo().timeout_interval) { 
                    // std::cout << "TimeDiff: " << timeDiff << std::endl;
                    // std::cout << "Transmission Timeout inverval 1000x: " << client.getTransmissionInfo().timeout_interval << std::endl;
                    resendMessages(client.getPort());
                    client.doubleTimeoutInterval();
                    //TODO: IMPLEMENT A FEATURE THAT WILL STOP RESEND ATTEMPTS AFTER n amount of attempts
                    TRACE_SRC("Connection[messageResendCheck] - Timeout Reach -> resending small SEQ=%u and 2x TIMEOUT=%d", client.getLastAck(), client.getTransmissionInfo().timeout_interval);
                    //TODO: Implement a breakdown feature if the timeout has been doubled n amount of time that this connection should be destroyed.
                    if(client.getTransmissionInfo().number_of_timeouts > 10) {
                        clientsToRemove.push_back(port);
                        INFO_SRC("Connection[messageResendCheck] - Number of Timeouts > 5 -> deleting CLIENT");
                    }
                }
            }
            //TODO: check if we should go into an IDLE STATE
        }
    }

    if(!clientsToRemove.empty()) {
        for(uint16_t port : clientsToRemove) {
            clients.erase(port);
        }
    }
    
    sleep(3); //FIXME: REMOVE THIS BAD BOY
}

void Connection::addClient(uint16_t port, uint32_t ip) {
    INFO_SRC("Connection[addClient] - Adding Client by sending SYN [IP=%u PORT=%u]", ip, port);
    clients.try_emplace(port, port, ip, 0, 0, 0, static_cast<uint8_t>(STATE::NONE));
    createMessage(source_port, port, default_sequence_number, default_ack_number, static_cast<uint8_t>(FLAGS::SYN), window_size, urgent_pointer, destinationIP, static_cast<uint8_t>(STATE::SYN_SENT), 0, 0);
    clients[port].setExpectedSequence(default_sequence_number+1);
}


//TODO: REFRACTOR CODE TO MAKE THIS FUNCTION SMALLER AND IMPLEMENT FUNCTIONAL PROGRAMMING 
// FUNCTION FOR EACH STATE SO WE CAN SPLIT IT UP AND CAN FIND ERRORS EASIER
void Connection::communicate() {

    INFO_SRC("Connection[communicate] - Function started");

    if (!clients.empty()) {
        INFO_SRC("Connection[communicate] - Targets provided creating and sending SYN");
        Client& client = clients.at(destination_port);
        createMessage(source_port, client.getPort(), default_sequence_number, default_ack_number, static_cast<uint8_t>(FLAGS::SYN), window_size,  urgent_pointer, destinationIP, static_cast<uint8_t>(STATE::SYN_SENT), 0, 0);
        client.setExpectedSequence(default_sequence_number+1);
    }
    
    std::unique_ptr<Segment> seg;
    std::vector<uint8_t> input;
    while (running) {
        if (receiverQueue.tryPop(seg)) {
            if(seg->getDestPrt() != source_port) {
                WARNING_SRC("Connection[communicate] - Received Valid SEG - INVALID PORT -> DSTPRT=%u | SRCPRT=%u", seg->getDestPrt(), source_port);
            }
            else {
                switch(decodeFlags(seg->getFlags())) {
                    case FlagType::SYN:{
                        clients.try_emplace(seg->getSrcPrt(), seg->getSrcPrt(), seg->getDestinationIP(), 0, seg->getSeqNum()+1, 0, static_cast<uint8_t>(STATE::SYN_RECEIVED));
                        DEBUG_SRC("Connection[communicate] - SYN RECEIVED Client[IP:%u SRC:%u SEQ:%u]", seg->getSrcPrt(), seg->getDestinationIP(), seg->getSeqNum());
                        createMessage(source_port, seg->getSrcPrt(), default_sequence_number, seg->getSeqNum()+1, createFlag(FLAGS::SYN, FLAGS::ACK), window_size, urgent_pointer, seg->getDestinationIP(), static_cast<uint8_t>(STATE::SYN_SENT), 0, 0);
                        clients[seg->getSrcPrt()].setExpectedSequence(default_sequence_number+1);
                        break;
                    }
                    case FlagType::SYN_ACK:
                        
                        if (auto clientIt = clients.find(seg->getSrcPrt()); clientIt != clients.end()) {
                            Client& client = clientIt->second;
                            if (seg->getAckNum() == client.getExpectedSequence()) {
                                client.checkTrackerSegment(seg->getAckNum());

                                DEBUG_SRC("Connection[communicate] - SYN_ACK RECEIVED Client[IP:%u SEQ:%u]", seg->getDestinationIP(), seg->getSrcPrt());
                                
                                createMessage(source_port, client.getPort(), client.getExpectedSequence(), seg->getSeqNum()+1, static_cast<uint8_t>(FLAGS::ACK), window_size, urgent_pointer, client.getIP(), static_cast<uint8_t>(STATE::ESTABLISHED), 0, 0);
                                if(client.hasMessages()) while(client.checkFront(seg->getAckNum()));
                                client.setLastAck(seg->getAckNum());
                                client.setWindowSize(seg->getWindowSize());
                            } 
                            else {
                                WARNING_SRC("Connection[communicate] - Client[IP:%u PORT:%u] Sent SYN_ACK with Incorrect ACK [EXPACK=%u ACK=%u]", seg->getDestinationIP(), seg->getSrcPrt(), client.getExpectedAck(), seg->getAckNum());
                            }
                        } 
                        else {
                            WARNING_SRC("Connection[communicate] - Received SYN_ACK from unknown client [IP:%u PORT:%u]", seg->getDestinationIP(), seg->getSrcPrt());
                        }
                        break;
                    case FlagType::FIN:
                        if (auto clientIt = clients.find(seg->getSrcPrt()); clientIt != clients.end()) {
                            Client& client = clientIt->second;
                            client.checkTrackerSegment(seg->getAckNum());

                            if (seg->getSeqNum() > client.getExpectedAck()) {
                                uint32_t copySeqNum = seg->getSeqNum();
                                if ((seg->getAckNum() >= client.getLastAck() && seg->getAckNum() <= client.getExpectedSequence())) {
                                    messageHandler(std::move(seg));
                                } else {
                                    client.setWindowSize(seg->getWindowSize());
                                    client.setItemMessageBuffer(seg->getSeqNum(), std::move(seg));
                                }
                                DEBUG_SRC("Connection[communicate] - Received out of order packet [TYPE=FIN SEQ=%u EXPSEQ=%u]", copySeqNum, client.getExpectedAck());
                            }
                            else if ((seg->getAckNum() >= client.getLastAck() && seg->getAckNum() <= client.getExpectedSequence()) && seg->getSeqNum() == client.getExpectedAck()) {
                                DEBUG_SRC("Connection[communicate] - Received in order packet[FIN] with valid ACK");

                                uint8_t newState = static_cast<uint8_t>(STATE::NONE);
                                if (client.getState() == static_cast<uint8_t>(STATE::ESTABLISHED)) {
                                    newState = static_cast<uint8_t>(STATE::CLOSING);
                                    DEBUG_SRC("Connection[communicate] - Received FIN transitioning from ESTABLISHED to CLOSING");
                                }
                                else if (client.getState() == static_cast<uint8_t>(STATE::FIN_WAIT_2)) {
                                    newState = static_cast<uint8_t>(STATE::TIME_WAIT);
                                    DEBUG_SRC("Connection[communicate] - Received FIN transitioning from FIN_WAIT_2 to TIME_WAIT");
                                }

                                if (newState != static_cast<uint8_t>(STATE::NONE)) {
                                    DEBUG_SRC("Connection[communicate] - Received FIN and transitioning state, calling createMessage with flag=FIN_ACK");
                                    createMessage(source_port, client.getPort(), client.getExpectedSequence(), seg->getSeqNum()+1, createFlag(FLAGS::FIN, FLAGS::ACK), window_size, urgent_pointer, client.getIP(), newState, 0,0);
                                    client.setWindowSize(seg->getWindowSize());
                                    client.setLastAck(seg->getAckNum());
                                    client.setExpectedAck(seg->getSeqNum()+1);
                                    client.setExpectedSequence(client.getExpectedSequence()+1);
                                    if (newState == static_cast<uint8_t>(STATE::CLOSING)) messageHandler(std::move(seg));
                                } else {
                                    WARNING_SRC("Connection[communicate] - Received FIN but transition state failed currentState=%s", stateToStr(client.getState()).c_str());
                                }
                                
                            } 
                            else {
                                WARNING_SRC("Connection[communicate] - Client[IP:%u PORT:%u] Sent FIN with Incorrect SEQ & ACK [EXP_SEQ=%u EXP_ACK=%u SEQ=%u ACK=%u]", seg->getDestinationIP(), seg->getSrcPrt(), client.getExpectedAck(), client.getExpectedSequence(), seg->getSeqNum(),  seg->getAckNum());
                            }
                        } 
                        else {
                            WARNING_SRC("Connection[communicate] - Received FIN from unknown client [IP:%u PORT:%u]", seg->getDestinationIP(), seg->getSrcPrt());
                        }

                        break;
                    case FlagType::FIN_ACK:

                        if (auto clientIt = clients.find(seg->getSrcPrt()); clientIt != clients.end()) {
                            Client& client = clientIt->second;
                            client.checkTrackerSegment(seg->getAckNum());

                            if(seg->getSeqNum() > client.getExpectedAck()) {
                                uint32_t copySeqNum = seg->getSeqNum();
                                if ((seg->getAckNum() > client.getLastAck() && seg->getAckNum() <= client.getExpectedSequence())) {
                                    messageHandler(std::move(seg));
                                } 
                                DEBUG_SRC("Connection[communicate] - Received out of order packet [TYPE=FIN_ACK SEQ=%u EXPSEQ=%u]", copySeqNum, client.getExpectedAck());
                            }
                            else if (seg->getAckNum() == client.getExpectedSequence() && seg->getSeqNum() == client.getExpectedAck()) {
                                DEBUG_SRC("Connection[communicate] - Received in order packet[FIN_ACK] with valid ACK");
                                
                                while(client.checkFront(seg->getAckNum()));
                                client.setExpectedSequence(seg->getAckNum());
                                client.setExpectedAck(seg->getSeqNum()+1);
                                client.setLastAck(seg->getAckNum());
                                client.setWindowSize(seg->getWindowSize());

                                STATE newState = STATE::NONE;
                                if (client.getState() == static_cast<uint8_t>(STATE::FIN_WAIT_1)) {
                                    newState = STATE::FIN_WAIT_2;
                                    DEBUG_SRC("Connection[communicate] - Received FIN transitioning from FIN_WAIT_1 to FIN_WAIT_2");
                                }
                                else if (client.getState() == static_cast<uint8_t>(STATE::LAST_ACK)) {
                                    newState = STATE::CLOSED;
                                    DEBUG_SRC("Connection[communicate] - Received FIN transitioning from LAST_ACK to CLOSED");
                                }

                                client.setState(static_cast<uint8_t>(newState));
                                client.info();
                            } 
                            else {
                                WARNING_SRC("Connection[communicate] - Client[IP:%u PORT:%u] Sent FIN_ACK with Incorrect SEQ & ACK [EXP_SEQ=%u EXP_ACK=%u SEQ=%u ACK=%u]", seg->getDestinationIP(), seg->getSrcPrt(), client.getExpectedAck(), client.getExpectedSequence(), seg->getSeqNum(),  seg->getAckNum());
                            }
                        } 
                        else {
                            WARNING_SRC("Connection[communicate] - Received FIN_ACK from unknown client [IP:%u PORT:%u]", seg->getDestinationIP(), seg->getSrcPrt());
                        }
                        
                        break;

                    case FlagType::ACK:
                        if (auto clientIt = clients.find(seg->getSrcPrt()); clientIt != clients.end()) {
                            Client& client = clientIt->second;
                            client.checkTrackerSegment(seg->getAckNum());
                            if (seg->getSeqNum() > client.getExpectedAck()) {
                                uint32_t copySeqNum = seg->getSeqNum();
                                if(seg->getAckNum() > client.getLastAck() && seg->getAckNum() <= client.getExpectedSequence()) {
                                    messageHandler(std::move(seg));
                                }
                                else {
                                    client.setWindowSize(seg->getWindowSize());
                                    client.setItemMessageBuffer(seg->getSeqNum(), std::move(seg));
                                }
                                DEBUG_SRC("Connection[communicate] - Received out of order packet [TYPE=ACK SEQ=%u EXPSEQ=%u]", copySeqNum, client.getExpectedAck());
                            }
                            else if(seg->getAckNum() >= client.getLastAck() && seg->getAckNum() <= client.getExpectedSequence() && seg->getSeqNum() == client.getExpectedAck()) {
                                DEBUG_SRC("Connection[communicate] - Received in order packet[ACK] with valid ACK");
                                
                                size_t data_written = 0;
                                
                                if (!seg->getData().empty()) {
                                    INFO_SRC("Connection[communicate] - Received packet[IP=%u PORT=%u SEQ=%u ACK=%u SIZE=%zu] with data -> attempting to write to file", client.getIP(), client.getPort(), seg->getSeqNum(), seg->getAckNum(), seg->getData().size());
                                    
                                    // for(uint8_t byte : seg->getData()) {
                                    //     std::cout << byte;
                                    // }
                                    // std::cout << std::endl;
                                    
                                    std::vector<uint8_t> packetData = seg->getData();
                                    client.receivedData.push(std::move(packetData));
                                    data_written = client.writeFile(std::move(seg->getData()));
                                    
                                    uint32_t new_seq_num = seg->getSeqNum() + data_written;
                                    //FIXME: Out of order packet could be a FIN/FIN_ACK what do we do then
                                    if (client.checkItemMessageBuffer(new_seq_num)){
                                        std::unique_ptr<Segment> outOfOrderSegment;
                                        while (client.popItemMessageBuffer(new_seq_num, outOfOrderSegment)) {
                                            if(outOfOrderSegment->getFlags() == static_cast<uint8_t>(FLAGS::ACK) && outOfOrderSegment->getData().size() > 0){
                                                TRACE_SRC("Connection[communicate] - Attempting to write out of order packet[IP=%u PORT=%u SEQ=%u ACK=%u SIZE=%zu] to file", client.getIP(), client.getPort(), outOfOrderSegment->getSeqNum(), outOfOrderSegment->getAckNum(), outOfOrderSegment->getData().size());
                                                
                                                // for(uint8_t byte : outOfOrderSegment->getData()) {
                                                //     std::cout << byte;
                                                // }
                                                // std::cout << std::endl;

                                                packetData = outOfOrderSegment->getData();
                                                client.receivedData.push(std::move(packetData));
                                                data_written = client.writeFile(std::move(outOfOrderSegment->getData()));
                                                new_seq_num += data_written;
                                            }
                                            else break;
                                        }
                                        seg = std::move(outOfOrderSegment);
                                    }
                                    
                                    client.closeFile();
                                    
                                    seg->setSeqNum(new_seq_num); 
                                    client.setExpectedAck(new_seq_num);
                                
                                } 

                                if(client.getState() == static_cast<uint8_t>(STATE::SYN_SENT)) {
                                    client.setState(static_cast<uint8_t>(STATE::ESTABLISHED));
                                    
                                } 
                                DEBUG_SRC("Connection[communicate] - Calling messageHandler to determine additional responses for packet received");
                                messageHandler(std::move(seg), data_written);
                                
                                
                            } else {
                                WARNING_SRC("Connection[communicate] - Client[IP:%u PORT:%u] Sent ACK with Incorrect ACK [SEGSEQ=%u SEGACK=%u CLISEQ=%u CLIACK=%u]", seg->getDestinationIP(), seg->getSrcPrt(), seg->getSeqNum(), seg->getAckNum(), client.getExpectedSequence(), client.getExpectedAck());
                            }
                        } else {
                            WARNING_SRC("Connection[communicate] - Received ACK from unknown client [IP:%u PORT:%u]", seg->getDestinationIP(), seg->getSrcPrt());
                        }
                        
                        continue;
                    case FlagType::PSH_ACK:
                        //TODO: Implement PSH_ACK logic 
                        continue;
                    case FlagType::RST:
                        //TODO: Implement RST logic 
                        continue;
                    case FlagType::DATA:
                        //TODO: Implement DATA logic 
                        continue;
                    default: continue;
                }
            }
        }
        
        else if (inputQueue.tryPop(input)) {
            sendBuffer.insert(sendBuffer.end(), input.begin(), input.end());
            
            INFO_SRC("Connection[communicate] - received input data and sucessfully inserted into sendBuffer");
            
            for(auto& [port, client] : clients) {
                if(client.getState() == static_cast<uint8_t>(STATE::ESTABLISHED) || client.getState() == static_cast<uint8_t>(STATE::CLOSING)) {
                    sendMessages(client.getPort());
                }
            }
        } 
        else if (timeToClose) {
            if(clients.empty()) {
                // std::cout << "Clients empty shutting down loop and calling safeToClose" << std::endl;
                running = false;
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    canCloseDown = true;
                    safeToClose.notify_one();
                }
                break;
            }
            
            std::vector<uint16_t> portsToRemove;
            
            for(auto& [port, client] : clients) {
                if(!client.getIsFinSent()) {
                    TRACE_SRC("Connection[communicate] - Client[IP=%u PORT=%u] Has not Sent/Received FIN -> checking if sent all data", client.getIP(), port);
                    
                    if(client.getLastByteSent() == sendBuffer.size()) {
                        DEBUG_SRC("Connection[communicate] - Sending FIN to Client[IP=%u PORT=%u]", client.getIP(), port);

                        createMessage(source_port,
                                      port,
                                      client.getExpectedSequence(),
                                      client.getExpectedAck(),
                                      static_cast<uint8_t>(FLAGS::FIN),
                                      window_size,
                                      urgent_pointer, 
                                      client.getIP(),
                                      (client.getState() == static_cast<uint8_t>(STATE::ESTABLISHED) || client.getState() == static_cast<uint8_t>(STATE::CLOSING) ? static_cast<uint8_t>(STATE::FIN_WAIT_1) : static_cast<uint8_t>(STATE::LAST_ACK)),
                                      0, 0
                        );
                        client.setExpectedSequence(client.getExpectedSequence()+1);
                        client.setIsFinSent(true);
                    }
                }
                else if (client.getState() == static_cast<uint8_t>(STATE::TIME_WAIT)) {
                    if(client.hasMessages() && client.getMessageTimeSent() != std::chrono::steady_clock::time_point{}) {
                        auto now = std::chrono::steady_clock::now();
                        double timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - client.getMessageTimeSent()).count();
                        if (timeDiff > 2*client.getTransmissionInfo().timeout_interval) { 
                            portsToRemove.push_back(port);
                            INFO_SRC("Connection[communicate] - Client[%u:%u] has officied finished the 2x timeout_interval - deleted port", client.getIP(), client.getPort());
                        } 
                    }
                }
                else if (client.getState() == static_cast<uint8_t>(STATE::CLOSED)) {
                    portsToRemove.push_back(port);
                }
            }

            if (!portsToRemove.empty()) {
                for (uint16_t port : portsToRemove) {
                    clients.erase(port);
                }
            }

        }
        
        else if(!clients.empty()) {
            //FIXME: ERROR WITH RESEND RESULTING IN OUT OF PACKETS AS WELL 
            messageResendCheck();
        }
        else {
            //TODO: Implement proper logic regarding sleeping when nothing is avaibable. 
            sleep(3);
        }
    
    }
}

void Connection::disconnect() {
    timeToClose = true;
    {
        std::unique_lock<std::mutex> lock(mtx);
        safeToClose.wait(lock, [this] {return canCloseDown;});
    }
    // std::cout << "Safe to Close trigger" << std::endl;
    socketHandler->stop();
    if(communicationThread.joinable()) communicationThread.join();
    INFO_SRC("Connection[disconnect] - Closed all threads and connection");
}
