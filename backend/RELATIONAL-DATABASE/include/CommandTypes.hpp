#ifndef COMMANDTYPES_HPP
#define COMMANDTYPES_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

enum Type {INT, BIGINT, STRING, BOOL, FLOAT, NULLTYPE};

struct ColumnInfo {
    std::string name;
    Type type;
    bool isPrimaryKey = false;
    bool isForeignKey = false;
    bool isNullable = true;
    std::string refTable;
    std::string refColumn;

    void print() const  {
        std::cout << "Column: " << name << std::endl;
        std::cout << "\tType: " << type << std::endl;
        std::cout << "\tPrimary Key: " << isPrimaryKey << std::endl;
        std::cout << "\tForeign Key: " << isForeignKey << std::endl;
        if (isForeignKey) {
            std::cout << "\trefTable: " << refTable << std::endl;
            std::cout << "\trefColumn: " << refColumn << std::endl;
        }
        std::cout << "\tNullable: " << isNullable << std::endl;
    }
};

enum CommandType {CREATE_TABLE, INSERT, SELECT, UPDATE, DELETE};


struct Command {
    CommandType type;
    virtual ~Command() = default;

    virtual void print() const = 0;
};

struct CreateTableCommand : public Command {
    std::string tableName;
    std::vector<ColumnInfo> columns;
    CreateTableCommand() {type = CREATE_TABLE; }

    void print() const override {
        std::cout << "Table: " << tableName << std::endl;
        for (const auto& col: columns) col.print();
    }

};

struct InsertCommand : public Command {

};

struct SelectCommand : public Command {

};

struct UpdateCommand : public Command {

};

struct DeleteCommand : public Command {

};




#endif