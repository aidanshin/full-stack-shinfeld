#include "WebSocketServer.hpp"
#include "WebSocketFrame.hpp"
#include "VIMPacket.hpp"

#include <chrono>
std::atomic<bool> running{false};
    
void mainLogic(ThreadSafeQueue<std::vector<uint8_t>>& receiverQueue, ThreadSafeQueue<std::vector<uint8_t>>& senderQueue,  ThreadSafeQueue<std::string>& inputQueue) {
   while (running) {
        std::vector<uint8_t> newFrame;
        std::string input;
        if(receiverQueue.tryPop(newFrame)) {
            std::unique_ptr<WebSocketFrame> new_frame_decoded = WebSocketFrame::decodeFrame(newFrame);
            if(new_frame_decoded) {
                auto payload = new_frame_decoded->getPayload();
                auto decoded_payload = VIMPacket::decodePacket(payload);
                new_frame_decoded->printFrame(); 
                if(decoded_payload.has_value()) {
                    decoded_payload.value().printPacket();
                }
            }
            else WARNING_SRC("SocketTesting[main] - Invalid or incomplete frame received");
        } 
        else if (inputQueue.tryPop(input)) {
            if(input == "quit") { break; }
            std::vector<uint8_t> payload;
            if(input == "userReq") {
                const char *ip = "127.0.0.1";
                struct in_addr addr;
                inet_pton(AF_INET, ip, &addr);
                uint32_t ip_val = htonl(addr.s_addr);
                payload = VIMPacket::createPacket(3, ip_val, 50000);
            } 
            else if (input == "userCon") {
                const char *ip = "127.0.0.1";
                struct in_addr addr;
                inet_pton(AF_INET, ip, &addr);
                uint32_t ip_val = htonl(addr.s_addr);
                payload = VIMPacket::createPacket(4, ip_val, 50000, 1); 
            }
            else {
                std::vector<uint8_t> payloadToSend(input.begin(), input.end());
                payload = VIMPacket::createPacket(1, 2, 2, payloadToSend);
            }
            WebSocketFrame frame(
                true, false, false, false, static_cast<uint8_t>(WebSocketFrame::OPCODE::BINARY), false, static_cast<uint64_t>(payload.size()), 0, std::move(payload)
            ); 
            std::vector<uint8_t> frameToSend = frame.encodeFrame();
            senderQueue.push(frameToSend);
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }  
}

int main(int argc, const char* arvg[]) {
    Logger::setPriority(LogLevel::TRACE);
    Logger::enableFileOutput("logs/SocketTesting.log");

    ThreadSafeQueue<std::vector<uint8_t>> receiverQueue;
    ThreadSafeQueue<std::vector<uint8_t>> senderQueue;

    WebSocketServer server(receiverQueue, senderQueue);

    if(!server.start()) CRITICAL_SRC("SocketTesting[main] - start return false");

    ThreadSafeQueue<std::string> inputQueue;
    running = true;
    std::thread communicate = std::thread(&mainLogic, std::ref(receiverQueue), std::ref(senderQueue), std::ref(inputQueue));
    
    std::string input;
    while (std::getline(std::cin, input)) {
            if(input == "quit") { 
                running=false;
                inputQueue.push(input);
                break; 
            }
            inputQueue.push(input);
    }

    if(communicate.joinable()) communicate.join();

    if(!server.stop()) CRITICAL_SRC("SocketTesting[main] - end return false");    

    return 0;
}