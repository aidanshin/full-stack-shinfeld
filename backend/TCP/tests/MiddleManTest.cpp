
#include "Logger.hpp"
#include "NetCommon.hpp"

#include <random>


std::atomic<bool> running = true;

// static uint32_t convertIpAddress(const std::string& ip) {
// 	struct in_addr addr;
// 	if(inet_pton(AF_INET, ip.c_str(), &addr) != 1) throw std::runtime_error("INVALID IP");
// 	return ntohl(addr.s_addr);
// }

void bitManipulator(std::vector<uint8_t>& msg) {
    if(msg.empty()) return;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> byteDistrib(
        (msg.size() > 20) ? 20 : 4,
        (msg.size() > 20) ? msg.size() - 1 : 19
    );
    int byteIndex = byteDistrib(gen);
    
    std::uniform_int_distribution<> bitDistrib(0,7);
    int bitIndex = bitDistrib(gen);
    
    msg[byteIndex] ^= (1<<bitIndex);
}

void start(uint16_t srcPort, uint16_t clientPort, const char* clientIP, uint16_t serverPort, const char* serverIP) {

	INFO_SRC("MiddleManTest[main] - Creating middle man socket");
	
	int socketfd;
		
	if ((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		ERROR_SRC("MiddleManTest[main] - Socket creation failure");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in middle_addr;
	memset(&middle_addr, 0, sizeof(middle_addr));
	
	middle_addr.sin_family = AF_INET;
	middle_addr.sin_addr.s_addr = INADDR_ANY;
	middle_addr.sin_port = htons(srcPort);

	INFO_SRC("MiddleManTest[main] - Binding to port %u", srcPort);
	if(bind(socketfd, (const struct sockaddr *)& middle_addr, sizeof(middle_addr)) < 0) {
		ERROR_SRC("MiddleManTest[main] - Bind Failure");
		exit(EXIT_FAILURE);
	}

	struct timeval tv = { .tv_sec = 3, .tv_usec=0};
    
    INFO_SRC("SocketHandler[Start] - Setting timeout for socket [SEC=%ld USEC=%ld]", (long)tv.tv_sec, (long)tv.tv_usec);
    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        ERROR_SRC("SocketHandler[Start] - Set Timeout Failure");
        exit(EXIT_FAILURE);
    }

	static constexpr size_t BUFFER_SIZE = 1024;
	std::vector<uint8_t> packet(BUFFER_SIZE);

	struct sockaddr_in client_addr{};
	struct sockaddr_in server_addr{};
	socklen_t len = sizeof(sockaddr_in);

	memset(&client_addr, 0, sizeof(client_addr));
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverIP, &server_addr.sin_addr);

	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(clientPort);
	inet_pton(AF_INET, clientIP, &client_addr.sin_addr);

	while (running) {
		sockaddr_in sender_addr{};
		ssize_t n = recvfrom(socketfd, reinterpret_cast<char *>(packet.data()), BUFFER_SIZE, 0, (struct sockaddr *) &sender_addr, &len);
		
		if (n > 0) {
			TRACE_SRC("MiddleManTest[main] - New Packet Arrived Size=%i", n);
			
			if(ntohs(sender_addr.sin_port) == serverPort && sender_addr.sin_addr.s_addr == server_addr.sin_addr.s_addr) {
				sendto(socketfd, packet.data(), n, 0, (sockaddr*)&client_addr, sizeof(client_addr));
				TRACE_SRC("MiddleManTest[main] - Forwarded %zd bytes from server to client", n);
			} 
			else if(ntohs(sender_addr.sin_port) == clientPort && sender_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
				sendto(socketfd, packet.data(), n, 0, (sockaddr*)&server_addr, sizeof(server_addr));
				TRACE_SRC("MiddleManTest[main] - Forwarded %zd bytes from client to server", n);
			} 
			else {
				WARNING_SRC("MiddleManTest[main] - Ignored received packet.");
			}

		} else {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			} else {
				ERROR_SRC("MiddleManTest[main] - recvfrom() failures");
			}
		}
	}

	close(socketfd);
}

int main(int argc, char* argv[]) {
	
	if (argc != 6) {
		std::cout << "./error_test srcprt clientPort clientIP serverPort serverIP" << std::endl;
		exit(EXIT_FAILURE);
	}

	Logger::setPriority(LogLevel::TRACE);
    Logger::enableFileOutput("logs/MiddleManTest.log");
	
	uint16_t srcPort = static_cast<uint16_t>(std::stoi(argv[1]));	
	uint16_t clientPort = static_cast<uint16_t>(std::stoi(argv[2]));	
	uint16_t serverPort = static_cast<uint16_t>(std::stoi(argv[4]));	
	const char* clientIP = argv[3];
	const char* serverIP = argv[5];
	
	INFO_SRC("MiddleManTest[test] - Starting socket thread");
	std::thread startThread = std::thread(start, srcPort, clientPort, clientIP, serverPort, serverIP);

	std::string input;
	while (true) {
		std::cin >> input;
		if (input == "quit") {
			running = false;
			break;
		}
	}

	if(startThread.joinable()) startThread.join();

	return 0;
}
