CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -fsanitize=address -I./include -I../WEBSOCKET/include -I../TCP/include
LDFLAGS_CRYP = -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto
CXXFLAGS += -I/opt/homebrew/opt/openssl@3/include

WEBSOCKET_SRC = ../WEBSOCKET/src/HttpHandler.cpp ../WEBSOCKET/src/WebSocketFrame.cpp ../WEBSOCKET/src/WebSocketServer.cpp
TCP_SRC = ../TCP/src/*.cpp

VIMPACKET_TEST_SRC = tests/VIMPacketTest.cpp src/VIMPacket.cpp
VIMPACKET_TEST_BIN = tests/vimpacket_test 

SOCKET_TEST_SRC = tests/SocketTesting.cpp src/VIMPacket.cpp 
SOCKET_TEST_BIN = tests/socket_test

.PHONY: clean run packet_test socket_test

$(VIMPACKET_TEST_BIN) : $(VIMPACKET_TEST_SRC)
	$(CXX) $(CXXFLAGS) $(VIMPACKET_TEST_SRC) -o $(VIMPACKET_TEST_BIN)

$(SOCKET_TEST_BIN) : $(SOCKET_TEST_SRC) $(WEBSOCKET_SRC)
	$(CXX) $(CXXFLAGS) $(SOCKET_TEST_SRC) $(WEBSOCKET_SRC) $(LDFLAGS_CRYP) -o $(SOCKET_TEST_BIN)

clean:
	rm -rf logs/*.log *.log *.dSYM tests/*.dSYM $(VIMPACKET_TEST_BIN) $(SOCKET_TEST_BIN)

run: $(MAIN_BIN)
	./$(MAIN_BIN) $(ARGS)

packet_test: $(VIMPACKET_TEST_BIN)
	./$(VIMPACKET_TEST_BIN)

socket_test: $(SOCKET_TEST_BIN)
	./$(SOCKET_TEST_BIN)

