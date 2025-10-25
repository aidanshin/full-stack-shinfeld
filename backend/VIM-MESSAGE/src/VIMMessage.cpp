#include "VIMMessage.hpp"

VIMMessage::VIMMessage(
    std::string IP,
    uint16_t port
) : 
    IP(IP),
    port(port)
{
    connection = std::make_unique<Connection>(port, IP, tcp_input_queue, clients);
}

void VIMMessage::start() {
    connection->connect();

    startWebSocketServer();

    running = true;
    vim_message_handler = std::thread(&VIMMessage::startMessageHandler, this);
}

void VIMMessage::startWebSocketServer() {
    websocketserver = std::make_unique<WebSocketServer>(ws_receiver_queue, ws_sender_queue);
    if(!websocketserver->start()) CRITICAL_SRC("startWebSocketServer - start return faile");
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
                    if(data.getType() == 3) {
                        connection->addClient(data.getPort(), data.getIP());
                    }
                    else {
                        tcp_input_queue.push(std::move(payloadCopy));
                    }
                }
                if(!payload.empty()) tcp_input_queue.push(std::move(payload));
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
            auto decode_packet = VIMPacket::decodePacket(new_packet);
            if(!decode_packet.has_value()) continue;
            hasData = true;
            switch(decode_packet->getType()) {
                case 1: 
                    ws_sender_queue.push(std::move(VIMPacket::createPacket(2, decode_packet->getUserId(), decode_packet->getMsgId(), decode_packet->getMsgData())));
                    break;
                case 2: 
                    TRACE_SRC("tcpHandler - Received type 2 packet");
                    decode_packet->printPacket();
                    break;
                case 3: { 
                    auto confirm_packet = VIMPacket::createPacket(4, decode_packet->getIP(), decode_packet->getPort(), 1000);
                    auto confirm_packet_tcp = confirm_packet;
                    ws_sender_queue.push(std::move(confirm_packet));
                    tcp_input_queue.push(std::move(confirm_packet_tcp));

                    break;
                }
                case 4: {
                    //confirming connection notify UI and messages can now be sent 
                    auto confirm_packet = VIMPacket::createPacket(4, decode_packet->getIP(), decode_packet->getPort(), 1000);
                    ws_sender_queue.push(std::move(confirm_packet));
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
    while (running) {
        
        if(websocketHandler()){

        } 
        else if (tcpHandler()) {

        }
        else {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    INFO_SRC("startMessageHandler - Closing");
}