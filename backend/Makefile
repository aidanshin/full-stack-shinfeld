CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -fsanitize=address -I./include

SRC = src/Client.cpp src/Segment.cpp src/SocketHandler.cpp src/Connection.cpp src/SegmentInfo.cpp
SRC_SOCKET = src/Segment.cpp src/SocketHandler.cpp
SRC_SEGMENT = src/Segment.cpp
SRC_CLIENT = src/Client.cpp src/Segment.cpp

SEGMENT_TEST_SRC = tests/SegmentTest.cpp
SOCKET_TEST_SRC = tests/SocketTest.cpp
LOGGER_TEST_SRC = tests/LoggerTest.cpp
SIMPLE_TEST_SRC = tests/SimpleTesting.cpp
CLIENT_TEST_SRC = tests/ClientTest.cpp
ERROR_TEST_SRC = tests/ErrorTest.cpp

MAIN = main.cpp
MAIN_BIN = tcp_program
MAIN_ERROR_BIN = tcp_program_error

SEGMENT_TEST_BIN = tests/segment_test
SOCKET_TEST_BIN = tests/socket_test
LOGGER_TEST_BIN = tests/logger_test
SIMPLE_TEST_BIN = tests/simple_test
CLIENT_TEST_BIN = tests/client_test
ERROR_TEST_BIN = tests/error_test

$(MAIN_BIN): $(MAIN) $(SRC)
	$(CXX) $(CXXFLAGS) $(MAIN) $(SRC) -o $(MAIN_BIN)

$(MAIN_ERROR_BIN): $(MAIN) $(SRC) $(ERROR_TEST_SRC)
	$(CXX) $(CXXFLAGS) $(MAIN) $(SRC) $(ERROR_TEST_SRC) -o $(MAIN_ERROR_BIN)	

$(SEGMENT_TEST_BIN): $(SEGMENT_TEST_SRC) $(SRC_SEGMENT)
	$(CXX) $(CXXFLAGS) $(SEGMENT_TEST_SRC) $(SRC_SEGMENT) -o $(SEGMENT_TEST_BIN)

$(SOCKET_TEST_BIN): $(SOCKET_TEST_SRC) $(SRC_SOCKET)
	$(CXX) $(CXXFLAGS) $(SOCKET_TEST_SRC) $(SRC_SOCKET) -o $(SOCKET_TEST_BIN)

$(LOGGER_TEST_BIN): $(LOGGER_TEST_SRC)
	$(CXX) $(CXXFLAGS) $(LOGGER_TEST_SRC) -o $(LOGGER_TEST_BIN)

$(SIMPLE_TEST_BIN): $(SIMPLE_TEST_SRC)
	$(CXX) $(CXXFLAGS) $(SIMPLE_TEST_SRC) -o $(SIMPLE_TEST_BIN)

$(CLIENT_TEST_BIN) : $(CLIENT_TEST_SRC) $(SRC_CLIENT)
	$(CXX) $(CXXFLAGS) $(CLIENT_TEST_SRC) $(SRC_CLIENT) -o $(CLIENT_TEST_BIN)

$(ERROR_TEST_BIN) : $(ERROR_TEST_SRC)
	$(CXX) $(CXXFLAGS) $(ERROR_TEST_SRC) -o $(ERROR_TEST_BIN)

clean:
	rm -f $(SEGMENT_TEST_BIN) $(SOCKET_TEST_BIN) $(LOGGER_TEST_BIN) $(CLIENT_TEST_BIN) $(MAIN_BIN) $(MAIN_ERROR_BIN) $(SIMPLE_TEST_BIN) $(ERROR_TEST_BIN) logs/app_*.log *.dat *.log

run: $(MAIN_BIN)
	./$(MAIN_BIN) $(ARGS)

run_error: $(MAIN_ERROR_BIN)
	./$(MAIN_ERROR_BIN) $(ARGS)

run_segment_test: $(SEGMENT_TEST_BIN)
	./$(SEGMENT_TEST_BIN) $(ARGS)

run_socket_test: $(SOCKET_TEST_BIN)
	./$(SOCKET_TEST_BIN) $(ARGS)

run_logger_test: $(LOGGER_TEST_BIN)
	./$(LOGGER_TEST_BIN) $(ARGS)

run_simple_test: $(SIMPLE_TEST_BIN)
	./$(SIMPLE_TEST_BIN) $(ARGS)

run_client_test: $(CLIENT_TEST_BIN)
	./$(CLIENT_TEST_BIN)

run_error_test: $(ERROR_TEST_BIN)
	./$(ERROR_TEST_BIN) $(ARGS)
