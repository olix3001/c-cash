#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

namespace tokenizer {

    const char operator_list[] = {'(', ')', '{', '}', ';', ':', ',', '[', ']', '=', '+', '-', '/', '*', '#', '<', '>', '\''};

    // Different types of tokens
    enum TokenType {
        UNDEFINED = -1,
        IDENTIFIER = 0,
        OPERATOR = 1,
        INTEGER = 2,
        DOUBLE = 3,
        STRING = 4,
        CHAR = 5,
    };

    struct Token {
        enum TokenType type{TokenType::UNDEFINED};
        std::string value;

        int lineNo{0};
        int charNo{0};

        void debug_print();
    };

    // Main tokenizer method
    std::vector<Token> tokenize(std::string& data);

}