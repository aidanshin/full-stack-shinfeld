// Main brain logic
// call functions to starts threads for all connections 
// logs all data in and out 
// link to react so they both start at the same time ? 



// Functions 
// estbalihses web socket connection wtih REACT + starts TCP server 
// - give two threads for sending and receiving to react 
// new message from React, if it is new client
// create a new TCP connection for client - need to implement logic on TCP library 
// infinite loop of checking for new message 
// decode 
// process data 

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
    // start TCP COnnection
    connection->connect();

    //start webscoket server 
    startWebSocketServer();

    running = true;
    startMessageHandler();
}

void VIMMessage::startWebSocketServer() {
    websocketserver = std::make_unique<WebSocketServer>(ws_receiver_queue, ws_sender_queue);
    if(!websocketserver->start()) CRITICAL_SRC("startWebSocketServer - start return faile");
}

void VIMMessage::stop() {
    running = false;
    connection->disconnect();
    websocketserver->stop();
}

bool VIMMessage::websocketHandler() {
    std::vector<uint8_t> new_packet;
    if(ws_receiver_queue.tryPop(new_packet)) {
            auto decode_packet = WebSocketFrame::decodeFrame(new_packet);
            if(decode_packet) {
                auto payload = decode_packet->getPayload();
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
                    //received message from connection change type to a 2 and forward to UI
                    ws_sender_queue.push(std::move(VIMPacket::createPacket(2, decode_packet->getUserId(), decode_packet->getMsgId(), decode_packet->getMsgData())));
                    // break;
                case 2: 
                    //error should have not reeived this 
                    // ws_sender_queue.push(std::move(new_packet));
                    TRACE_SRC("tcpHandler - Received type 2 packet");
                    decode_packet->printPacket();
                    break;
                case 3: 
                    //requesting connnection to you -> add IP and Port to clients 
                    auto ip = decode_packet->getIP();
                    auto port = decode_packet->getPort();
                    // TODO
                    // FIXME
                    // BUG
                    // clients.try_emplace(port, client) TODO FIXME BUG
                    break;
                case 4: 
                    //confirming connection notify UI and messages can now be sent 
                    break;
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
    //check WebSocketQueue Receiver Queue for command 
        // determine how to change the packet and pipeline to inputQueue of TCP 
        // 
    //check TCP Client receiveQueues 
        // determine logic direction of data from the tcp data 

}