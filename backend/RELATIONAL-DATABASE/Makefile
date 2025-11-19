CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wshadow -Wpedantic -g -I./include
CXXFLAGS += -fsanitize=address -fsanitize=undefined -fstack-protector-strong

SRC_PARSER = src/Parser.cpp

PARSER_TEST_SRC = tests/ParserTest.cpp 

PARSER_TEST_BIN = tests/parser_test

$(PARSER_TEST_BIN): $(PARSER_TEST_SRC) $(SRC_PARSER)
	$(CXX) $(CXXFLAGS) $(PARSER_TEST_SRC) $(SRC_PARSER) -o $(PARSER_TEST_BIN)

clean: 
	rm -rf $(PARSER_TEST_BIN)  *.dSYM tests/*.dSYM

run_parser_test: $(PARSER_TEST_BIN)
	./$(PARSER_TEST_BIN)