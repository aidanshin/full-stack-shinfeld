#include "Connection.hpp"
#include "Logger.hpp"

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
    ThreadSafeQueue<std::string>& q
) 
:
    source_port(srcPort),
    destination_port(destPort),
    source_ip_str(srcIPstr),
    destination_ip_str(destIPstr),
    inputQueue(q)
{
    sourceIP = convertIpAddress(source_ip_str);
    destinationIP = convertIpAddress(destination_ip_str);
    INFO_SRC("Connection Initialized with Target- [SRCPRT=%u DSTPRT=%u SRCIP=%s DSTIP=%s]", srcPort, destPort, srcIPstr.c_str(), destIPstr.c_str());
}

Connection::Connection (
    uint16_t srcPort,
    std::string srcIPstr,
    ThreadSafeQueue<std::string>& q
)
:
    source_port(srcPort),
    source_ip_str(srcIPstr),
    inputQueue(q)
{
    sourceIP = convertIpAddress(source_ip_str);
    
    INFO_SRC("Connection Initialized without Target- [SRCPRT=%u SRCIP=%s]", srcPort, srcIPstr.c_str());
}


void Connection::connect() {
    running = true;

    INFO_SRC("Connection[connect] - Attempting to initialize SocketHandler [IP:%u PORT:%u]", sourceIP, source_port);
    socketHandler = std::make_unique<SocketHandler>(source_port, sourceIP, receiverQueue, senderQueue);
    socketHandler->start();

    if (destination_port) {
        INFO_SRC("Connection[connect] - Attempting to create target client [IP:%u PORT:%u]", destinationIP, destination_port);
        clients.try_emplace(destination_port, destination_port, destinationIP, 0, 0, 0, static_cast<uint8_t>(STATE::NONE));
    }

    communicationThread = std::thread(&Connection::communicate, this);
    
    INFO_SRC("Connection[connect] - Communication Thread Called");
}

void Connection::createMessage(uint16_t srcPort, uint16_t dstPrt, uint32_t seqNum, uint32_t ackNum, uint8_t flag, uint16_t window, uint16_t urgentPtr, uint32_t dstIP, std::vector<uint8_t> data, uint8_t state) {
    if (auto client = clients.find(dstPrt); client != clients.end()) {
        DEBUG_SRC("Connection[createMessage] - New Packet Created To Send");
        size_t dataSize = data.size();
        std::unique_ptr<Segment> seg = std::make_unique<Segment>(srcPort, dstPrt, seqNum, ackNum, flag, window, urgentPtr, dstIP, std::move(data));
        Segment* rawSeg = seg.get();

        client->second.pushMessage(std::move(seg));
        senderQueue.push(std::pair<Segment*, std::function<void()>>(rawSeg, client->second.LastTimeMessageSent()));
        DEBUG_SRC("Connection[createMessage] - Pushed packet to clients.messagesSent & senderQueue");
        uint32_t newSeq = seqNum+dataSize+1;
        if (state != client->second.getState()) client->second.setState(state);
        if (newSeq != client->second.getExpectedSequence()) client->second.setExpectedSequence(newSeq);
        if (ackNum != client->second.getExpectedAck()) client->second.setExpectedAck(ackNum);
         
    } else {
        WARNING_SRC("Connection[createMessage] - Function called but destination port (%u) is not within Clients", dstPrt);
    }
}

void Connection::resendMessages(uint16_t port, uint32_t newAck) {
    if(auto client = clients.find(port); client != clients.end()) {
        if(client->second.hasMessages()) {
            size_t loop = client->second.numMessageSentAvailable();
            INFO_SRC("Connection[resendMessages] - Client[PORT=%u] has %zu messages to send", port, loop);
            while (loop-- > 0) {
                std::unique_ptr<Segment> seg;
                if(client->second.popMessage(seg)) {
                    if(!seg) continue;
                    if (newAck && seg->getAckNum() != newAck) {seg->setAckNum(newAck);}

                    Segment* rawSeg = seg.get();
                    
                    DEBUG_SRC("Connection[resendMessages] - Resending Packet [DSTIP=%u DSTPRT=%u SEQ=%u ACK=%U FLAG=%s]", seg->getDestinationIP(), seg->getDestPrt(), seg->getSeqNum(), seg->getAckNum(), flagsToStr(seg->getFlags()).c_str());
                    client->second.pushMessage(std::move(seg));
                    senderQueue.push(std::pair<Segment*, std::function<void()>>(rawSeg, client->second.LastTimeMessageSent()));
                }
            }
            if (newAck) {
                client->second.setExpectedSequence(newAck);
                TRACE_SRC("Connection[resendMessages] - New Expected Sequence %u", newAck);
            }
        }
    } else {
        WARNING_SRC("Connection[resendMessages] - Could not find PORT=%u in clients", port);
    }
}

void Connection::sendMessages(uint16_t port) {
    if(auto client = clients.find(port); client != clients.end()) {
        uint16_t messagesSentSize = client->second.sizeMessageSent();
        uint16_t bufferSizeAvailable = client->second.getWindowSize() - messagesSentSize;
        
        INFO_SRC("Connection[sendMessages] - Checking the buffer size and created new packets if possible");
        int iterations = round(bufferSizeAvailable/MAX_DATA_SIZE)+1;
        for (int i = 0; i < iterations; i++) {
            if (client->second.getLastByteSent() == dataToSend.size()) {
                break;
            }

            auto start = dataToSend.begin() + client->second.getLastByteSent();
            auto end = (start + MAX_DATA_SIZE > dataToSend.end()) ? dataToSend.end() : start + MAX_DATA_SIZE;
            std::vector<uint8_t> msg(start, end);

            createMessage(
                source_port, 
                client->second.getPort(), 
                client->second.getExpectedSequence(), 
                client->second.getExpectedAck(), 
                static_cast<uint8_t>(FLAGS::ACK), 
                window_size, 
                urgent_pointer, 
                client->second.getIP(), 
                msg, 
                client->second.getState()
            );
            
            size_t bytesSent = std::distance(dataToSend.begin(), end);
            client->second.setLastByteSent(bytesSent);
            bufferSizeAvailable -= bytesSent + Segment::HEADER_SIZE;
            TRACE_SRC("Connection[messageHandler] - bytesSent=%zu bufferSizeAvailable=%u", bytesSent, bufferSizeAvailable);                 
        }
    }
}


void Connection::messageHandler(std::unique_ptr<Segment> seg) {
    if (auto client = clients.find(seg->getSrcPrt()); client != clients.end()) {
        INFO_SRC("Connection[messageHandler] - Checking if packets sent are ACK'd");


        if (client->second.hasMessages()) {
            while(client->second.checkFront(seg->getAckNum()));  
        }
        
        client->second.setLastAck(seg->getAckNum());
        client->second.setWindowSize(seg->getWindowSize());

        if (seg->getSeqNum() > client->second.getExpectedAck()) {
            client->second.setItemMessageBuffer(seg->getSeqNum(), std::move(seg));
        }
        else {
            sendMessages(client->second.getPort());

            if (!client->second.getIsFinSent() && client->second.getLastByteSent() == dataToSend.size()) {
                if(timeToClose || client->second.getState() == static_cast<uint8_t>(STATE::CLOSING)) {
                    DEBUG_SRC("Connection[messageHandler] - Sent all data to client[IP:%u PORT:%u], creating FIN", client->second.getIP(), client->second.getPort());
                    createMessage(source_port, 
                                client->second.getPort(), 
                                client->second.getExpectedSequence(), 
                                client->second.getExpectedAck(), 
                                static_cast<uint8_t>(FLAGS::FIN), 
                                window_size, urgent_pointer, 
                                client->second.getIP(), 
                                std::vector<uint8_t>{}, 
                                (client->second.getState() == static_cast<uint8_t>(STATE::CLOSING) ? static_cast<uint8_t>(STATE::LAST_ACK) : static_cast<uint8_t>(STATE::FIN_WAIT_1))
                    );
                    client->second.setIsFinSent(true);
                }
            }
        }
    } else {
        WARNING_SRC("Connection[messageHandler] - Could not find PORT=%u in clients", seg->getSrcPrt());
    }
}

void Connection::messageResendCheck() {
    std::vector<uint16_t> clientsToRemove;
    for(auto& [port, client] : clients) {
        if(client.getLastTimeMessageSent()) {
            double diff = std::difftime(std::time(NULL), client.getLastTimeMessageSent());
            if(diff > MAX_SEGMENT_LIFE) {
                std::cout << diff << std::endl;

                if (client.getState() == static_cast<uint8_t>(STATE::CLOSED)) {
                    clientsToRemove.push_back(port);
                } else {
                    if(client.hasMessages()) {
                        
                        resendMessages(port, client.getExpectedAck());
                    }
                }
            }
        }
        
    }

    if (!clientsToRemove.empty()) {
        for (uint16_t port : clientsToRemove) {
            clients.erase(port);
        }
    }
}

void Connection::communicate() {

    INFO_SRC("Connection[communicate] - Function started");

    if (!clients.empty()) {
        INFO_SRC("Connection[communicate] - Targets provided creating and sending SYN");
        Client& client = clients.at(destination_port);
        createMessage(source_port, client.getPort(), default_sequence_number, default_ack_number, static_cast<uint8_t>(FLAGS::SYN), window_size,  urgent_pointer, destinationIP, std::vector<uint8_t>{}, static_cast<uint8_t>(STATE::SYN_SENT));
    }
    std::cout<< "QUITING" << std::endl;
    // exit(1);
    std::unique_ptr<Segment> seg;
    std::string input;
    while (running) {
        if (receiverQueue.tryPop(seg)) {
            switch(decodeFlags(seg->getFlags())) {
                case FlagType::SYN:{
                    // Client client(seg->getSrcPrt(), seg->getDestinationIP(), 0, seg->getSeqNum()+1, 0, static_cast<uint8_t>(STATE::SYN_RECEIVED));
                    clients.try_emplace(seg->getSrcPrt(), seg->getSrcPrt(), seg->getDestinationIP(), 0, seg->getSeqNum()+1, 0, static_cast<uint8_t>(STATE::SYN_RECEIVED));
                    DEBUG_SRC("Connection[communicate] - SYN RECEIVED Client[IP:%u SRC:%u SEQ:%u]", seg->getSrcPrt(), seg->getDestinationIP(), seg->getSeqNum());
                    createMessage(source_port, seg->getSrcPrt(), default_sequence_number, seg->getSeqNum()+1, createFlag(FLAGS::SYN, FLAGS::ACK), window_size, urgent_pointer, seg->getDestinationIP(), std::vector<uint8_t>{}, static_cast<uint8_t>(STATE::SYN_SENT));
                    break;
                }
                case FlagType::SYN_ACK:
                    
                    if (auto client = clients.find(seg->getSrcPrt()); client != clients.end()) {
                        if (seg->getAckNum() == client->second.getExpectedSequence()) {
                            DEBUG_SRC("Connection[communicate] - SYN_ACK RECEIVED Client[IP:%u SEQ:%u]", seg->getDestinationIP(), seg->getSrcPrt());
                            
                            createMessage(source_port, client->second.getPort(), client->second.getExpectedSequence(), seg->getSeqNum()+1, static_cast<uint8_t>(FLAGS::ACK), window_size, urgent_pointer, client->second.getIP(), std::vector<uint8_t>{}, static_cast<uint8_t>(STATE::ESTABLISHED));
                            messageHandler(std::move(seg));
                        } 
                        else {
                            WARNING_SRC("Connection[communicate] - Client[IP:%u PORT:%u] Sent SYN_ACK with Incorrect ACK [EXPACK=%u ACK=%u]", seg->getDestinationIP(), seg->getSrcPrt(), client->second.getExpectedAck(), seg->getAckNum());
                        }
                    } 
                    else {
                        WARNING_SRC("Connection[communicate] - Received SYN_ACK from unknown client [IP:%u PORT:%u]", seg->getDestinationIP(), seg->getSrcPrt());
                    }
		            break;
                case FlagType::FIN:
                    if (auto client = clients.find(seg->getSrcPrt()); client != clients.end()) {
                        if (seg->getSeqNum() > client->second.getExpectedAck()) {
                            if ((seg->getAckNum() > client->second.getLastAck() && seg->getAckNum() <= client->second.getExpectedSequence())) {
                                messageHandler(std::move(seg));
                            } else {
                                client->second.setWindowSize(seg->getWindowSize());
                                client->second.setItemMessageBuffer(seg->getSeqNum(), std::move(seg));
                            }
                            DEBUG_SRC("Connection[communicate] - Received out of order packet [TYPE=FIN SEQ=%u EXPSEQ=%u]", seg->getSeqNum(), client->second.getExpectedAck());
                        }
                        else if ((seg->getAckNum() > client->second.getLastAck() && seg->getAckNum() <= client->second.getExpectedSequence()) && seg->getSeqNum() == client->second.getExpectedAck()) {
                            DEBUG_SRC("Connection[communicate] - Received in order packet[FIN] with valid ACK");

                            uint8_t newState = static_cast<uint8_t>(STATE::NONE);
                            if (client->second.getState() == static_cast<uint8_t>(STATE::ESTABLISHED)) {
                                newState = static_cast<uint8_t>(STATE::CLOSING);
                                DEBUG_SRC("Connection[communicate] - Received FIN transitioning from ESTABLISHED to CLOSING");
                            }
                            else if (client->second.getState() == static_cast<uint8_t>(STATE::FIN_WAIT_2)) {
                                newState = static_cast<uint8_t>(STATE::TIME_WAIT);
                                DEBUG_SRC("Connection[communicate] - Received FIN transitioning from FIN_WAIT_2 to TIME_WAIT");
                            }

                            if (newState != static_cast<uint8_t>(STATE::NONE)) {
                                DEBUG_SRC("Connection[communicate] - Received FIN and transitioning state, calling createMessage with flag=FIN_ACK");
                                createMessage(source_port, client->second.getPort(), client->second.getExpectedSequence(), client->second.getExpectedAck()+1, createFlag(FLAGS::FIN, FLAGS::ACK), window_size, urgent_pointer, client->second.getIP(), std::vector<uint8_t>{}, newState);
                                client->second.setWindowSize(seg->getWindowSize());
                                if (newState == static_cast<uint8_t>(STATE::CLOSING)) messageHandler(std::move(seg));
                            } else {
                                WARNING_SRC("Connection[communicate] - Received FIN but transition state failed currentState=%s", stateToStr(client->second.getState()).c_str());
                            }
                            
                        } 
                        else {
                            WARNING_SRC("Connection[communicate] - Client[IP:%u PORT:%u] Sent FIN with Incorrect ACK [EXPACK=%u ACK=%u]", seg->getDestinationIP(), seg->getSrcPrt(), client->second.getExpectedAck(), seg->getAckNum());
                        }
                    } 
                    else {
                        WARNING_SRC("Connection[communicate] - Received FIN from unknown client [IP:%u PORT:%u]", seg->getDestinationIP(), seg->getSrcPrt());
                    }

                    break;
                case FlagType::FIN_ACK:

                    if (auto client = clients.find(seg->getSrcPrt()); client != clients.end()) {
                        if(seg->getSeqNum() > client->second.getExpectedAck()) {
                            if ((seg->getAckNum() > client->second.getLastAck() && seg->getAckNum() <= client->second.getExpectedSequence())) {
                                messageHandler(std::move(seg));
                            } 
                            else {
                                client->second.setWindowSize(seg->getWindowSize());
                                client->second.setItemMessageBuffer(seg->getSeqNum(), std::move(seg));
                            }
                            DEBUG_SRC("Connection[communicate] - Received out of order packet [TYPE=FIN_ACK SEQ=%u EXPSEQ=%u]", seg->getSeqNum(), client->second.getExpectedAck());

                        }
                        else if (seg->getAckNum() == client->second.getExpectedSequence() && seg->getSeqNum() == client->second.getExpectedAck()) {
                            DEBUG_SRC("Connection[communicate] - Received in order packet[FIN_ACK] with valid ACK");

                            while(client->second.checkFront(seg->getAckNum()));
                            client->second.setExpectedSequence(seg->getAckNum()+1);
                            client->second.setExpectedAck(seg->getSeqNum()+1);
                            client->second.setLastAck(seg->getAckNum());
                            client->second.setWindowSize(seg->getWindowSize());

                            STATE newState = STATE::NONE;
                            if (client->second.getState() == static_cast<uint8_t>(STATE::FIN_WAIT_1)) {
                                newState = STATE::FIN_WAIT_2;
                                DEBUG_SRC("Connection[communicate] - Received FIN transitioning from FIN_WAIT_1 to FIN_WAIT_2");
                            }
                            else if (client->second.getState() == static_cast<uint8_t>(STATE::LAST_ACK)) {
                                newState = STATE::CLOSED;
                                DEBUG_SRC("Connection[communicate] - Received FIN transitioning from LAST_ACK to CLOSED");
                            }

                            client->second.setState(static_cast<uint8_t>(newState));
                            
                        } 
                        else {
                            WARNING_SRC("Connection[communicate] - Client[IP:%u PORT:%u] Sent FIN_ACK with Incorrect SEQ & ACK [EXP_SEQ=%u EXP_ACK=%u SEQ=%u ACK=%u]", seg->getDestinationIP(), seg->getSrcPrt(), client->second.getExpectedAck(), client->second.getExpectedSequence(), seg->getSeqNum(),  seg->getAckNum());
                        }
                    } 
                    else {
                        WARNING_SRC("Connection[communicate] - Received FIN_ACK from unknown client [IP:%u PORT:%u]", seg->getDestinationIP(), seg->getSrcPrt());
                    }
                    
                    break;

                case FlagType::ACK:
                    
                    if (auto client = clients.find(seg->getSrcPrt()); client != clients.end()) {
                        if (seg->getSeqNum() > client->second.getExpectedAck()) {
                            if(seg->getAckNum() > client->second.getLastAck() && seg->getAckNum() <= client->second.getExpectedSequence()) {
                                messageHandler(std::move(seg));
                            }
                            else {
                                client->second.setWindowSize(seg->getWindowSize());
                                client->second.setItemMessageBuffer(seg->getSeqNum(), std::move(seg));
                            }
                            DEBUG_SRC("Connection[communicate] - Received out of order packet [TYPE=ACK SEQ=%u EXPSEQ=%u]", seg->getSeqNum(), client->second.getExpectedAck());
                        }
                        else if(seg->getAckNum() > client->second.getLastAck() && seg->getAckNum() <= client->second.getExpectedSequence() && seg->getSeqNum() == client->second.getExpectedAck()) {
                            DEBUG_SRC("Connection[communicate] - Received in order packet[ACK] with valid ACK");
                            
                            size_t data_written = 0;
                            
                            if (!seg->getData().empty()) {
                                INFO_SRC("Connection[communicate] - Received packet[IP=%u PORT=%u SEQ=%u ACK=%u SIZE=%zu] with data -> attempting to write to file", client->second.getIP(), client->second.getPort(), seg->getSeqNum(), seg->getAckNum(), seg->getData().size());
                                
                                data_written = client->second.writeFile(seg->getData());
                                
                                uint32_t new_seq_num = seg->getSeqNum() + data_written + 1;
                                
                                if (client->second.checkItemMessageBuffer(new_seq_num)){
                                    std::unique_ptr<Segment> outOfOrderSegment;
                                    while (client->second.popItemMessageBuffer(new_seq_num, outOfOrderSegment)) {
                                        TRACE_SRC("Connection[communicate] - Attempting to write out of order packet[IP=%u PORT=%u SEQ=%u ACK=%u SIZE=%zu] to file", client->second.getIP(), client->second.getPort(), outOfOrderSegment->getSeqNum(), outOfOrderSegment->getAckNum(), outOfOrderSegment->getData().size());
                                        data_written = client->second.writeFile(outOfOrderSegment->getData());
                                        new_seq_num += data_written + 1;
                                    }
                                }
                                
                                client->second.closeFile();
                                
                                seg->setSeqNum(new_seq_num); 
                                client->second.setExpectedAck(new_seq_num);
                            } 

                            DEBUG_SRC("Connection[communicate] - Calling messageHandler to determine additional responses for packet received");
                            messageHandler(std::move(seg));
                            
                        } else {
                            WARNING_SRC("Connection[communicate] - Client[IP:%u PORT:%u] Sent ACK with Incorrect ACK [EXPECTEDACK=%u ACK=%u]", seg->getDestinationIP(), seg->getSrcPrt(), client->second.getExpectedAck(), seg->getAckNum());
                        }
                    } else {
                        WARNING_SRC("Connection[communicate] - Received ACK from unknown client [IP:%u PORT:%u]", seg->getDestinationIP(), seg->getSrcPrt());
                    }
                    
                    continue;
                case FlagType::PSH_ACK:
                    continue;
                case FlagType::RST:
                    continue;
                case FlagType::DATA:
                    continue;
                default: continue;
            }
        }
        
        else if (inputQueue.tryPop(input)) {
            dataToSend.insert(dataToSend.end(), input.begin(), input.end());
            
            INFO_SRC("Connection[communicate] - received input data and sucessfully inserted into dataToSend");
            
            for(auto& [port, client] : clients) {
                // if(!client.hasMessages()) {
                    if(client.getState() == static_cast<uint8_t>(STATE::ESTABLISHED) || client.getState() == static_cast<uint8_t>(STATE::CLOSING)) {
                        sendMessages(client.getPort());
                    }
                // }
            }
        } 
        else if (timeToClose) {
            INFO_SRC("Connection[communicate] - timeToClose=TRUE -> creating for FINs for clients");
            
            std::vector<uint16_t> portsToRemove;
            
            for(auto& [port, client] : clients) {
                if(!client.getIsFinSent()) {
                    TRACE_SRC("Connection[communicate] - Client[IP=%u PORT=%u] Has not Sent/Received FIN -> checking if sent all data", client.getIP(), port);
                    
                    if(client.getLastByteSent() == dataToSend.size()) {
                        DEBUG_SRC("Connection[communicate] - Sending FIN to Client[IP=%u PORT=%u]", client.getIP(), port);

                        createMessage(source_port,
                                      port,
                                      client.getExpectedSequence(),
                                      client.getExpectedAck(),
                                      static_cast<uint8_t>(FLAGS::FIN),
                                      window_size,
                                      urgent_pointer, 
                                      client.getIP(),
                                      std::vector<uint8_t>{},
                                      (client.getState() == static_cast<uint8_t>(STATE::ESTABLISHED) || client.getState() == static_cast<uint8_t>(STATE::CLOSING) ? static_cast<uint8_t>(STATE::FIN_WAIT_1) : static_cast<uint8_t>(STATE::LAST_ACK))
                        );

                        client.setIsFinSent(true);
                    }
                }
                else if (client.getState() == static_cast<uint8_t>(STATE::TIME_WAIT)) {
                    if(std::difftime(std::time(nullptr), client.getLastTimeMessageSent()) > (2*MAX_SEGMENT_LIFE)) {
                        portsToRemove.push_back(port);
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
            messageResendCheck();
        }
        else {
            sleep(3);
        }
    
    }

}

void Connection::disconnect() {
    running = false;
    timeToClose = true;
    {
        std::unique_lock<std::mutex> lock(mtx);
        safeToClose.wait(lock, [this] {return canCloseDown;});
    }
    socketHandler->stop();
    if(communicationThread.joinable()) communicationThread.join();
    INFO_SRC("Connection[disconnect] - Closed all threads and connection");
}