#include "Parser.hpp"

int main(int argc, char* argv[]) {
    // std::cout << "A\tB\tC" << std::endl;
    std::string create_table_str = "\t\n\t\nCREATE TABLE NAME (UserID INT PRIMARY_KEY, Username STRING NOT_NULL, Email STRING, ProfileID INT FOREIGN_KEY References Profiles(ID));";
    Parser p;

    auto cmd = p.parse(create_table_str);

    cmd->print();
    
    return 0;
}