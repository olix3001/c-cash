#include "Statements.hpp"

namespace parser {
    
    void Statement::debug_print(int indent) {
        std::cout << std::string(indent*2, ' ') << "\u001B[36m" << dataType << " ";
        for (auto arg : args) {
            std::cout << "[" << arg.first << " " << arg.second << "] ";
        }
        std::cout << "\u001B[33m" << type << " " << "\u001B[32m" << value << "\u001B[0m" << " (\n";
        for(Statement* statement : statements) {
            statement->debug_print(indent + 2);
        }
        std::cout << std::string(indent*2, ' ') << ")" << std::endl;
    }

}