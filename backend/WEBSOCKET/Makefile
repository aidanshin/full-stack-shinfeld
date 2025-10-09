CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -fsanitize=address -I./include
LDFLAGS_CRYP = -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto
CXXFLAGS += -I/opt/homebrew/opt/openssl@3/include

SRC = src/WebSocketServer.cpp 

SRC_SOCKET_TEST = src/WebSocketServer.cpp src/HttpHandler.cpp 
SRC_HTTP_HANDLER_TEST = src/HttpHandler.cpp

MAIN = main.cpp
MAIN_BIN = web_socket

SOCKET_TEST = tests/SocketTesting.cpp 
HTTP_HANDLER_TEST = tests/HttpHandlerTesting.cpp

SOCKET_TEST_BIN = tests/socket_testing
HTTP_HANDLER_TEST_BIN = tests/http_handler_testing

$(MAIN_BIN): $(MAIN) $(SRC)
	$(CXX) $(CXXFLAGS) $(MAIN) $(SRC) -o $(MAIN_BIN)

$(SOCKET_TEST_BIN) : $(SOCKET_TEST) $(SRC_SOCKET_TEST)
	$(CXX) $(CXXFLAGS) $(SOCKET_TEST) $(SRC_SOCKET_TEST) $(LDFLAGS_CRYP) -o $(SOCKET_TEST_BIN)

$(HTTP_HANDLER_TEST_BIN) : $(HTTP_HANDLER_TEST) $(SRC_HTTP_HANDLER_TEST)
	$(CXX) $(CXXFLAGS) $(HTTP_HANDLER_TEST) $(SRC_HTTP_HANDLER_TEST) $(LDFLAGS_CRYP) -o $(HTTP_HANDLER_TEST_BIN)

clean:
	rm -rf $(MAIN_BIN) $(SOCKET_TEST_BIN) $(HTTP_HANDLER_TEST_BIN) *.dSYM *.log logs/*.log tests/*.dSYM *.o 

run: $(MAIN_BIN)
	./$(MAIN_BIN) $(ARGS)

run_socket_test: $(SOCKET_TEST_BIN)
	./$(SOCKET_TEST_BIN) $(ARGS)

run_http_handler_test: $(HTTP_HANDLER_TEST_BIN)
	./$(HTTP_HANDLER_TEST_BIN)