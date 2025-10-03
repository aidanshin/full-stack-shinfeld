#include "SocketHandler.hpp"
#include "Flags.hpp"
#include "Logger.hpp"
#include "ErrorTest.hpp"

SocketHandler::SocketHandler (
    uint16_t port,
    uint32_t selfIP,
    ThreadSafeQueue<std::unique_ptr<Segment>>& receiverQueue,
    ThreadSafeQueue<std::pair<std::unique_ptr<Segment>, std::function<void()>>>& senderQueue,
    std::vector<uint8_t>& sendBuffer
)
:
    receiverQueue(receiverQueue),
    senderQueue(senderQueue),
    sendBuffer(sendBuffer),
    socketfd(-1),
    port(port),
    selfIP(selfIP)
{
    INFO_SRC("SocketHandler Initialized - [PORT=%u IP=%u]", port, selfIP);
}

void SocketHandler::receive() {
    static constexpr size_t BUFFER_SIZE = 1024;
    std::vector<uint8_t> packet(BUFFER_SIZE);
    socklen_t len;
    struct sockaddr_in senaddr{};
    int n;
    memset(&senaddr, 0, sizeof(senaddr));
    INFO_SRC("SockerHandler[Receiver] - Thread Started");
    ErrorTest error_test;
    while (running){
        len = sizeof(senaddr);

        n = recvfrom(socketfd, reinterpret_cast<char *>(packet.data()), BUFFER_SIZE, 0, (struct sockaddr *) &senaddr, &len);

        if (n > 0) {
            TRACE_SRC("SocketHandler[Receiver] - New Packet Arrived SIZE=%i", n);
            std::vector<uint8_t> payload(packet.begin(), packet.begin()+n);
            uint32_t sourceIP = ntohl(senaddr.sin_addr.s_addr);
            uint8_t protocol = 6;
            std::unique_ptr<Segment> segment = Segment::decode(sourceIP, selfIP, protocol, payload);
            if (segment && decodeFlags(segment->getFlags()) != FlagType::INVALID) {
                segment->setDestinationIP(sourceIP);
                if(error_test.dropPacket(0.9)){
                    TRACE_SRC("SocketHandler[Receiver] - New Packet Valid [IP=%u PORT=%u SEQ=%u ACK=%u FLAG=%s]", sourceIP, segment->getSrcPrt(), segment->getSeqNum(), segment->getAckNum(), flagsToStr(segment->getFlags()).c_str());
                    receiverQueue.push(std::move(segment));
                } else {
                    WARNING_SRC("SocketHandler[Receiver] - Dropped the packet [IP=%u PORT=%u SEQ=%u ACK=%u FLAG=%s]", sourceIP, segment->getSrcPrt(), segment->getSeqNum(), segment->getAckNum(), flagsToStr(segment->getFlags()).c_str());
                }
            } else {
                WARNING_SRC("SocketHandler[Receiver] - Dropped Invalid Segment[IP=%u PORT=%u SIZE=%d]", sourceIP, ntohs(senaddr.sin_port), n);
            }
        }
        else if (n < 0) {
            if(!running) break;
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                ERROR_SRC("SocketHandler[Receiver] - recvfrom() failure");
            }
        }
    }
}

void SocketHandler::send() {
    INFO_SRC("SocketHandler[Sender] - Thread Started");

    while (running) {
        
        auto opt_item = senderQueue.pop();
        
        if (!opt_item) {
            WARNING_SRC("SocketHandler[Sender] - Sender queue return nullptr, ignoring response");
        }
        else {
            std::pair<std::unique_ptr<Segment>, std::function<void()>> item = std::move(*opt_item);
            std::unique_ptr<Segment> segment = std::move(item.first);
            struct sockaddr_in senaddr{};
            memset(&senaddr, 0, sizeof(senaddr));

            senaddr.sin_family = AF_INET;
            senaddr.sin_port = htons(segment->getDestPrt()); // target port
            senaddr.sin_addr.s_addr = htonl(segment->getDestinationIP()); // target IP
            
            uint8_t protocol = 6;

            if(segment->getEnd() && segment->getStart() >= 0){
                segment->setData(std::vector<uint8_t>(sendBuffer.begin() + segment->getStart(), sendBuffer.begin() + segment->getEnd()));
            }

            std::vector<uint8_t> msg = segment->encode(selfIP, segment->getDestinationIP(), protocol);
            
            DEBUG_SRC("SocketHandler[Sender] - Preparing packet to send[IP=%u DSTPRT=%u SEQ=%u ACK=%u FLAGS=%s]", segment->getDestinationIP(), segment->getDestPrt(), segment->getSeqNum(), segment->getAckNum(), flagsToStr(segment->getFlags()).c_str()); 

            int n;
            if ((n = sendto(socketfd, reinterpret_cast<const char *>(msg.data()), msg.size(), 0, (sockaddr*)&senaddr, sizeof(senaddr))) < 0) {
                ERROR_SRC("SocketHandler[Sender] - sendto() failure");
            } else {
                TRACE_SRC("SocketHandler[Sender] - Successfully sent %i bytes", n);
                if(item.second) {
                    item.second();
                    TRACE_SRC("SocketHandler[Sender] - set time seent");
                }
            }
            
        }
    }
}


void SocketHandler::start() {
    INFO_SRC("SocketHandler[Start] - Attempting Initialization");
    if ((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {  
        ERROR_SRC("SocketHandler[Start] - Socket Creation Failure");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in recaddr;
    memset(&recaddr, 0, sizeof(recaddr));

    recaddr.sin_family = AF_INET;
    recaddr.sin_addr.s_addr = INADDR_ANY;
    recaddr.sin_port = htons(port);

    INFO_SRC("SocketHandler[Start] - Binding to port %u", port);
    if (bind(socketfd, (const struct sockaddr *)& recaddr, sizeof(recaddr)) < 0) {
        ERROR_SRC("SocketHandler[Start] - Bind Failure");
        exit(EXIT_FAILURE);
    }

    struct timeval tv = { .tv_sec = 5, .tv_usec=0};
    
    INFO_SRC("SocketHandler[Start] - Setting timeout for socket [SEC=%ld USEC=%ld]", (long)tv.tv_sec, (long)tv.tv_usec);
    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        ERROR_SRC("SocketHandler[Start] - Set Timeout Failure");
        exit(EXIT_FAILURE);
    }
    
    running = true;
    receiverThread = std::thread(&SocketHandler::receive, this);
    senderThread = std::thread(&SocketHandler::send, this);
    INFO_SRC("SocketHandler[Start] - Initialized\tReceiver & Sender Thread Called");
}

void SocketHandler::stop() {
    INFO_SRC("SocketHandler[Stop] - Called to now safely close sockets.");
    running = false;
    senderQueue.close();
    receiverQueue.close();
    shutdown(socketfd, SHUT_RDWR);
    if(receiverThread.joinable()) receiverThread.join();
    if(senderThread.joinable()) senderThread.join();
    close(socketfd);
    INFO_SRC("SocketHandler[Stop] - Successfully closed Receiver & Sender Thread and Socket");
}