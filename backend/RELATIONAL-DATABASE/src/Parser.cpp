#include "Parser.hpp"

#include <iostream>

static const std::unordered_map<std::string, CommandType> cmd_map {
    {"CREATE", CREATE_TABLE},
    {"INSERT", INSERT},
    {"SELECT", SELECT},
    {"UPDATE", UPDATE},
    {"DELETE", DELETE}
};

static const std::unordered_map<std::string, Type> type_map {
    {"INT", INT},
    {"BIGINT", BIGINT},
    {"STRING", STRING},
    {"BOOL", BOOL},
    {"FLOAT", FLOAT},
    {"NULLTYPE", NULLTYPE}
};

//TODO: Implement Create Database

std::unique_ptr<Command> Parser::parse(const std::string& input) {
    std::string trimmedInput = input;
    trimmedInput.erase(0, trimmedInput.find_first_not_of(" \t\n\r\f\v"));
    std::string firstWord = trimmedInput.substr(0, trimmedInput.find(' '));
    if (firstWord.length() != 6) return nullptr;
    std::transform(firstWord.begin(), firstWord.end(), firstWord.begin(), [](unsigned char c){return std::toupper(c);});

    auto it = cmd_map.find(firstWord);
    if (it != cmd_map.end()) {
        switch (it->second) {
            case CREATE_TABLE:
                return parseCreateTable(trimmedInput);
            case INSERT:
                return parseInsert(trimmedInput);
            case SELECT:
                return parseSelect(trimmedInput);
            case UPDATE:
                return parseUpdate(trimmedInput);
            case DELETE:
                return parseDelete(trimmedInput);
            default:
                return nullptr;
        }
    } else {
        return nullptr;
    }
}


//TODO refractor into separate functions
std::unique_ptr<Command> Parser::parseCreateTable(const std::string& input) {
    std::string::size_type tableNameStart, tableNameEnd;

    tableNameStart = input.find("TABLE ", 7);
    if(tableNameStart == std::string::npos) {
        std::cout << "TABLE not after CREATE" << std::endl;
        return nullptr;
    }
    tableNameStart += 6;

    tableNameEnd = input.find('(', tableNameStart);
    if(tableNameEnd == std::string::npos) {
        std::cout << "CREATE TABLE command missing (" << std::endl;
        return nullptr;
    }

    std::string nameInput = input.substr(tableNameStart, tableNameEnd-tableNameStart-1);

    std::string::size_type start = tableNameEnd+1;

    auto table = std::make_unique<CreateTableCommand>(); 
    
    table->tableName = nameInput;

    bool reachLastLine = false;
    while (!reachLastLine) {
        std::string::size_type endLine = input.find(',', start);
        if (endLine == std::string::npos) {
            endLine = input.find(");", start);
            reachLastLine = true;
            if(endLine == std::string::npos) {
                std::cout << "CREATE TABLE command missing );" << input.substr(start) << std::endl;
            }
        }
        std::string line = input.substr(start, endLine-start);
        
        
        std::vector<std::string> info;
        std::string::size_type startPoint = 0, endPoint = endLine - start;
        while (startPoint < (endLine-start)) {    
            startPoint = line.find_first_not_of(" \t\n\v\f\r", startPoint);
            if (startPoint == std::string::npos) {
                std::cout << "Error decoding column info: " << line << std::endl;
                return nullptr;
            }
            endPoint = line.find_first_of(" \n\t\r\v\f", startPoint);
            if (endPoint == std::string::npos) {
                endPoint = endLine-start;
            }            
            // std::cout << line.substr(startPoint, endPoint-startPoint) << " " << startPoint << " " << endPoint << std::endl;
            info.push_back(line.substr(startPoint, endPoint-startPoint));
            startPoint = endPoint+1;

        }
        
        if (info.size() != 2 && info.size() != 3 && info.size() != 5) {
            std::cout << "Invalid Column Input SIZE=" << info.size() << " - needs to be 2, 3, or 5: " << line << std::endl;
            return nullptr;
        }
        
        ColumnInfo col;
        
        //TODO: Refractor this into separate functions
        for (size_t i = 0; i < info.size(); ++i) {
            std::string& str = info[i];
            switch (i) {
                case 0: { 
                    col.name = str;
                    break;
                }

                case 1: {
                    auto it = type_map.find(str); 
                    if (it != type_map.end()) {
                        col.type = it->second;
                        break;
                    } else {
                        std::cout << "Invalid type: " << line << std::endl;
                        return nullptr;
                    }
                }
                case 2: {

                    if(str == "NOT_NULL") {
                        col.isNullable = false;
                        break;
                    } 
                    else if (str == "PRIMARY_KEY") {
                        col.isPrimaryKey = true;
                        break;
                    } else if (str == "FOREIGN_KEY") {
                        col.isForeignKey = true;
                        break;
                    }
                    else {
                        std::cout << "Invalid attribute: " << line << std::endl;
                        return nullptr;
                    }
                }
                case 3: {
                    if (str == "References") {
                        break;
                    } else {
                        std::cout << "Missing references: " << line << std::endl;
                        return nullptr;
                    }
                }
                case 4: {
                    std::string::size_type colStart = str.find('(');
                    if (colStart == std::string::npos) {
                        std::cout << "Missing ( for Foreign Key Reference: " << line << std::endl;
                        return nullptr;
                    }
                    
                    std::string::size_type colEnd = str.find(')');
                    if(colEnd == std::string::npos) {
                        std::cout << "Missing ) for Foreign Key Reference: " << line << std::endl;
                        return nullptr;
                    }
                    
                    col.refTable = str.substr(0, colStart);
                    
                    col.refColumn = str.substr(colStart+1, colEnd-colStart-1);
                    
                    break;
                }
            }
        }

        table->columns.push_back(col);
        
        start = endLine+1;  
    } 

    return table;
}

std::unique_ptr<Command> Parser::parseInsert(const std::string& input){

}

std::unique_ptr<Command> Parser::parseSelect(const std::string& input) {

}

std::unique_ptr<Command> Parser::parseUpdate(const std::string& input) {

}

std::unique_ptr<Command> Parser::parseDelete(const std::string& input) {

}
