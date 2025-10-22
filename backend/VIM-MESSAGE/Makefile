CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -fsanitize=address -I./include

VIMPACKET_TEST_SRC = tests/VIMPacketTest.cpp src/VIMPacket.cpp
VIMPACKET_TEST_BIN = tests/vimpacket_test 

$(VIMPACKET_TEST_BIN) : $(VIMPACKET_TEST_SRC)
	$(CXX) $(CXXFLAGS) $(VIMPACKET_TEST_SRC) -o $(VIMPACKET_TEST_BIN)

clean:
	rm -rf logs/app_*.log *.log *.dSYM tests/*.dSYM $(VIMPACKET_TEST_BIN)

run: $(MAIN_BIN)
	./$(MAIN_BIN) $(ARGS)

packet_test: $(VIMPACKET_TEST_BIN)
	./$(VIMPACKET_TEST_BIN)