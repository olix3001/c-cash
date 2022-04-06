#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace parser {

    enum StatementType {
        FUNCTION_DEFINITION = 0,
        CODE_BLOCK = 1,
        RETURN = 2,
        INTEGER_LITERAL = 3,
    };
        
    class Statement {
        public:
            StatementType type;
            std::string value;
            std::vector<Statement*> statements;

            std::vector<std::pair<std::string, std::string>> args; // used only for functions
            std::string dataType; // used for some things only

            Statement( StatementType type, std::string value ) : type( type ), value( value ) {};
            virtual ~Statement() = default;

            void debug_print(int indent);
    };

}