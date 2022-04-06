#include "Tokenizer.hpp"

namespace tokenizer {

    std::vector<Token> tokenize(std::string& data) {
        std::vector<Token> tokens;
        Token currentToken;
        currentToken.value = "";

        for (char cChar : data) {
            ++currentToken.charNo;
            
            if (cChar == ' ' || cChar == '\n' || cChar == '\t') { // whitespace, new line etc
                    if (currentToken.type != TokenType::UNDEFINED) {
                        tokens.emplace_back(currentToken);
                        currentToken.type = TokenType::UNDEFINED;
                        currentToken.value = "";
                    }
                    if (cChar == '\n') { currentToken.charNo = 0; currentToken.lineNo++; }

            } else if (isdigit(cChar)) { // number
                if (currentToken.type == TokenType::INTEGER || currentToken.type == TokenType::DOUBLE) {
                    currentToken.value.append(1, cChar);
                } else if (currentToken.type == TokenType::UNDEFINED) {
                    currentToken.type = TokenType::INTEGER;
                    currentToken.value.append(1, cChar);
                }


            } else if (std::find(std::begin(operator_list), std::end(operator_list), cChar) != std::end(operator_list)) { // operator
                if (currentToken.type != TokenType::UNDEFINED) {
                    tokens.emplace_back(currentToken);
                    currentToken.type = TokenType::UNDEFINED;
                    currentToken.value = "";
                }
                    
                currentToken.type = TokenType::OPERATOR;
                currentToken.value.append(1, cChar);

                tokens.emplace_back(currentToken);
                currentToken.type = TokenType::UNDEFINED;
                currentToken.value = "";

            } else { // identifier
                if (currentToken.type == TokenType::UNDEFINED) {
                    currentToken.type = TokenType::IDENTIFIER;
                    currentToken.value.append(1, cChar);
                } else if (currentToken.type == TokenType::IDENTIFIER) {
                    currentToken.value.append(1, cChar);
                } else {
                    if (currentToken.type != TokenType::UNDEFINED) {
                        tokens.emplace_back(currentToken);
                        currentToken.type = TokenType::UNDEFINED;
                        currentToken.value = "";
                    }
                }
            }

        }

        if (currentToken.type != TokenType::UNDEFINED)
            tokens.emplace_back(currentToken);

        return tokens;
    }

    void Token::debug_print() {
        std::cout << "\u001B[33mToken \u001B[0m(\u001B[36m" << type << "\u001B[0m,\u001B[36m \"" << value << "\"\u001B[0m,\u001B[36m " << lineNo << ":" << charNo << "\u001B[0m)\n";
    }

}