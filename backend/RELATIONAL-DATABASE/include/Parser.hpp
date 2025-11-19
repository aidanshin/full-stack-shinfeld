#ifndef PARSER_HPP
#define PARSER_HPP

#include "CommandTypes.hpp"

#include <memory>

class Parser {
    private:
        std::unique_ptr<Command> parseCreateTable(const std::string& input);
        std::unique_ptr<Command> parseInsert(const std::string& input);
        std::unique_ptr<Command> parseSelect(const std::string& input);
        std::unique_ptr<Command> parseUpdate(const std::string& input);
        std::unique_ptr<Command> parseDelete(const std::string& input);

    public:
        std::unique_ptr<Command> parse(const std::string& input);

};

#endif