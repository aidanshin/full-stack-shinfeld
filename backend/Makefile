CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./include

SRC = src/Client.cpp src/Segment.cpp src/SocketHandler.cpp src/Connection.cpp
SRC_SOCKET = src/Segment.cpp src/SocketHandler.cpp
SRC_SEGMENT = src/Segment.cpp

SEGMENT_TEST_SRC = tests/SegmentTest.cpp
SOCKET_TEST_SRC = tests/SocketTest.cpp
LOGGER_TEST_SRC = tests/LoggerTest.cpp
SIMPLE_TEST_SRC = tests/SimpleTest.cpp

MAIN = main.cpp
MAIN_BIN = tcp_program

SEGMENT_TEST_BIN = tests/segment_test
SOCKET_TEST_BIN = tests/socket_test
LOGGER_TEST_BIN = tests/logger_test
SIMPLE_TEST_BIN = tests/simple_test

$(MAIN_BIN): $(MAIN) $(SRC)
	$(CXX) $(CXXFLAGS) $(MAIN) $(SRC) -o $(MAIN_BIN)

$(SEGMENT_TEST_BIN): $(SEGMENT_TEST_SRC) $(SRC_SEGMENT)
	$(CXX) $(CXXFLAGS) $(SEGMENT_TEST_SRC) $(SRC_SEGMENT) -o $(SEGMENT_TEST_BIN)

$(SOCKET_TEST_BIN): $(SOCKET_TEST_SRC) $(SRC_SOCKET)
	$(CXX) $(CXXFLAGS) $(SOCKET_TEST_SRC) $(SRC_SOCKET) -o $(SOCKET_TEST_BIN)

$(LOGGER_TEST_BIN): $(LOGGER_TEST_SRC)
	$(CXX) $(CXXFLAGS) $(LOGGER_TEST_SRC) -o $(LOGGER_TEST_BIN)

$(SIMPLE_TEST_BIN): $(SIMPLE_TEST_SRC)
	$(CXX) $(CXXFLAGS) $(SIMPLE_TEST_SRC) -o $(SIMPLE_TEST_BIN)

clean:
	rm -f $(SEGMENT_TEST_BIN) $(SOCKET_TEST_BIN) $(LOGGER_TEST_BIN) $(MAIN_BIN) app_*.log 

run: $(MAIN_BIN)
	./$(MAIN_BIN) $(ARGS)

run_segment_test: $(SEGMENT_TEST_BIN)
	./$(SEGMENT_TEST_BIN) $(ARGS)

run_socket_test: $(SOCKET_TEST_BIN)
	./$(SOCKET_TEST_BIN) $(ARGS)

run_logger_test: $(LOGGER_TEST_BIN)
	./$(LOGGER_TEST_BIN) $(ARGS)
