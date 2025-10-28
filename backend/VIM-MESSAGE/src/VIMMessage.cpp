#include "VIMMessage.hpp"

VIMMessage::VIMMessage(
    std::string tcpIP,
    uint16_t tcpPort,
    std::string wsIP,
    uint16_t wsPort
) : 
    tcp_IP(tcpIP),
    tcp_port(tcpPort),
    ws_IP(wsIP),
    ws_port(wsPort)
{
    connection = std::make_unique<Connection>(tcp_port, tcp_IP, tcp_input_queue, clients);
    TRACE_SRC("VIMMessage[constructor] - TCP[IP:PORT] %s:%u | WS[IP:PORT] %s:%u", tcpIP.c_str(), tcpPort, wsIP.c_str(), wsPort);
}

void VIMMessage::start() {
    INFO_SRC("VIMMessage[start] - Starting Process");
    connection->connect();

    startWebSocketServer();

    running = true;
    vim_message_handler = std::thread(&VIMMessage::startMessageHandler, this);
}

void VIMMessage::startWebSocketServer() {
    websocketserver = std::make_unique<WebSocketServer>(ws_receiver_queue, ws_sender_queue, ws_port, ws_IP);
    if(!websocketserver->start()) {CRITICAL_SRC("startWebSocketServer - start return failed"); exit(EXIT_FAILURE);}
}

void VIMMessage::stop() {
    running = false;
    if(vim_message_handler.joinable()) vim_message_handler.join();
    connection->disconnect();
    websocketserver->stop();
}

bool VIMMessage::websocketHandler() {
    std::vector<uint8_t> new_packet;
    if(ws_receiver_queue.tryPop(new_packet)) {
            auto decode_packet = WebSocketFrame::decodeFrame(new_packet);
            if(decode_packet) {
                auto payload = decode_packet->extractPayload();
                auto payloadCopy = payload;
                auto decoded_payload = VIMPacket::decodePacket(payload);
                if(decoded_payload.has_value()) {
                    auto data = decoded_payload.value();
                    if(data.getType() == 3 && clients.find(data.getPort()) == clients.end()) {
                        connection->addClient(data.getPort(), data.getIP());
                    }
                    tcp_input_queue.push(std::move(payloadCopy));
                    TRACE_SRC("VIMMessage[websocketHandler] - Forward data to tcp input queue");
                }
                return true;
            }
        }
    return false;
}

bool VIMMessage::tcpHandler() {
    std::vector<uint8_t> new_packet;
    bool hasData = false;
    for(auto& [port, client] : clients) {
        if(client.receivedData.tryPop(new_packet)) {
            //FIXME
            auto decode_packet = VIMPacket::decodePacket(new_packet);
            if(!decode_packet.has_value()) continue;
            hasData = true;
            switch(decode_packet->getType()) {
                case 1: {
                        auto confirm_packet = VIMPacket::createPacket(2, decode_packet->getUserId(), decode_packet->getMsgId(), decode_packet->getMsgData());
                        auto frame = WebSocketFrame(true, false, false, false, static_cast<uint8_t>(WebSocketFrame::OPCODE::BINARY), false, static_cast<uint64_t>(confirm_packet.size()), 0, std::move(confirm_packet));
                        ws_sender_queue.push(frame.encodeFrame());
                        TRACE_SRC("VIMMessage[tcpHandler] - 1: Forwarded to WS sender queue");
                        break;
                    }
                case 2: 
                    TRACE_SRC("tcpHandler - Received type 2 packet");
                    decode_packet->printPacket();
                    break;
                case 3: { 
                    auto confirm_packet = VIMPacket::createPacket(4, decode_packet->getIP(), decode_packet->getPort(), 1000);
                    auto confirm_packet_tcp = confirm_packet;
                    auto frame = WebSocketFrame(true, false, false, false, static_cast<uint8_t>(WebSocketFrame::OPCODE::BINARY), false, static_cast<uint64_t>(confirm_packet.size()), 0, std::move(confirm_packet));
                    ws_sender_queue.push(frame.encodeFrame());
                    tcp_input_queue.push(std::move(confirm_packet_tcp));
                    TRACE_SRC("VIMMessage[tcpHandler] - 3: Send confirm packet to WS + TCP");
                    break;
                }
                case 4: {
                    auto confirm_packet = VIMPacket::createPacket(4, decode_packet->getIP(), decode_packet->getPort(), 1000);
                    auto frame = WebSocketFrame(true, false, false, false, static_cast<uint8_t>(WebSocketFrame::OPCODE::BINARY), false, static_cast<uint64_t>(confirm_packet.size()), 0, std::move(confirm_packet));
                    ws_sender_queue.push(frame.encodeFrame());
                    TRACE_SRC("VIMMessage[tcpHandler] - 4: Send confirm to WS Sender");
                    break;
                }
                default:
                    hasData = false;
                    continue;
            }
        }
    }
    return hasData;
}


void VIMMessage::startMessageHandler() {
    INFO_SRC("VIMMessage[startMessageHandler] - Started");
    while (running) {
        
        if(websocketHandler()){

        } 
        else if (tcpHandler()) {

        }
        else {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    INFO_SRC("VIMMessage[startMessageHandler] - Closing");
}