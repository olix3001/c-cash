#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "llvm/IR/Value.h"

namespace parser {

    enum StatementType {
        FUNCTION_DEFINITION = 0,
        CODE_BLOCK = 1,
        RETURN = 2,
        INTEGER_LITERAL = 3,
        VARIABLE_DEFINITON = 4,
        VARIABLE_CALL = 5,
        FUNCTION_CALL = 6,
        MATH = 7,
        BOOLEAN_LITERAL = 8,
        TYPE_CAST = 9,
        LONG_LITERAL = 10,
        CHAR_LITERAL = 11,
        LOGIC_EXPRESSION = 12,
        IF = 13,
        IFELSE = 14,
        VARIABLE_ASSIGNMENT = 15,
        FOR_LOOP = 16,
        IMPORT = 17,
        STRING = 18,
        GET_ALLOCA = 19,
    };
        
    struct Scope {
        Scope* parent;
        std::map<std::string, llvm::Value*> namedValues;

        Scope() {}
        Scope(Scope* parent) : parent(parent) {
            namedValues.insert(parent->namedValues.begin(), parent->namedValues.end());
        };
    };

    class Statement {
        public:
            StatementType type;
            std::string value;
            std::vector<Statement*> statements;

            std::vector<std::pair<std::string, std::string>> args; // used only for functions
            std::string dataType; // used for some things only
            Scope* scope;

            Statement( StatementType type, std::string value ) : type( type ), value( value ) {};
            virtual ~Statement() = default;

            void debug_print(int indent);
    };

}