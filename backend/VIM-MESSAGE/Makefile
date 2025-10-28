CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -fsanitize=address -I./include  -I../TCP/include -I../WEBSOCKET/include
LDFLAGS_CRYP = -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto
CXXFLAGS += -I/opt/homebrew/opt/openssl@3/include

WEBSOCKET_SRC = ../WEBSOCKET/src/HttpHandler.cpp ../WEBSOCKET/src/WebSocketFrame.cpp ../WEBSOCKET/src/WebSocketServer.cpp
TCP_SRC = ../TCP/src/Client.cpp ../TCP/src/Segment.cpp ../TCP/src/SocketHandler.cpp ../TCP/src/Connection.cpp ../TCP/src/SegmentInfo.cpp
VIMMESSAGE_SRC = src/VIMMessage.cpp src/VIMPacket.cpp

VIMPACKET_TEST_SRC = tests/VIMPacketTest.cpp src/VIMPacket.cpp
VIMPACKET_TEST_BIN = tests/vimpacket_test 

SOCKET_TEST_SRC = tests/SocketTesting.cpp src/VIMPacket.cpp 
SOCKET_TEST_BIN = tests/socket_test
MAIN_BIN = main

MAIN = main.cpp

.PHONY: clean run packet_test socket_test

$(VIMPACKET_TEST_BIN) : $(VIMPACKET_TEST_SRC)
	$(CXX) $(CXXFLAGS) $(VIMPACKET_TEST_SRC) -o $(VIMPACKET_TEST_BIN)

$(SOCKET_TEST_BIN) : $(SOCKET_TEST_SRC) $(WEBSOCKET_SRC)
	$(CXX) $(CXXFLAGS) $(SOCKET_TEST_SRC) $(WEBSOCKET_SRC) $(LDFLAGS_CRYP) -o $(SOCKET_TEST_BIN)

$(MAIN_BIN): $(MAIN) $(WEBSOCKET_SRC) $(TCP_SRC) $(VIMMESSAGE_SRC)
	$(CXX) $(CXXFLAGS) $(MAIN) $(VIMMESSAGE_SRC) $(WEBSOCKET_SRC) $(TCP_SRC) $(LDFLAGS_CRYP) -o $(MAIN_BIN)

clean:
	rm -rf logs/*.log *.log *.dSYM tests/*.dSYM $(VIMPACKET_TEST_BIN) $(SOCKET_TEST_BIN) $(MAIN_BIN) *.dat 

run: $(MAIN_BIN)
	./$(MAIN_BIN) $(ARGS)

packet_test: $(VIMPACKET_TEST_BIN)
	./$(VIMPACKET_TEST_BIN)

socket_test: $(SOCKET_TEST_BIN)
	./$(SOCKET_TEST_BIN)

