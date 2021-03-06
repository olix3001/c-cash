#include "Tokenizer.hpp"

namespace tokenizer {

    std::vector<Token> tokenize(std::string& data) {
        std::vector<Token> tokens;
        Token currentToken;
        currentToken.value = "";

        for (int k = 0; k<data.size(); ++k) {
            char cChar = data[k];
            ++currentToken.charNo;
            
            if(cChar == '"') {
                currentToken.type = TokenType::OPERATOR;
                currentToken.value = "\"";                
                tokens.emplace_back(currentToken);
                currentToken.type = TokenType::IDENTIFIER;
                currentToken.value = "";
                ++k;
                cChar = data[k];

                while (cChar != '"') {
                    if (cChar == '\\') {
                        ++k;
                        cChar = data[k];
                        if (cChar == 'n') {
                            currentToken.value.append(1, '\n');
                        } else if (cChar == 't') {
                            currentToken.value.append(1, '\t');
                        }
                    } else {
                        currentToken.value.append(1, cChar);
                    }
                    ++k;
                    cChar = data[k];
                }    

                tokens.emplace_back(currentToken);
                currentToken.type = TokenType::OPERATOR;
                currentToken.value = "\"";                
                tokens.emplace_back(currentToken);
                currentToken.type = TokenType::UNDEFINED;
                currentToken.value = "";
            } else if (cChar == ' ' || cChar == '\n' || cChar == '\t') { // whitespace, new line etc
                    if (currentToken.type != TokenType::UNDEFINED) {
                        tokens.emplace_back(currentToken);
                        currentToken.type = TokenType::UNDEFINED;
                        currentToken.value = "";
                    }
                    if (cChar == '\n') { currentToken.charNo = 0; currentToken.lineNo++; }

            } else if (isdigit(cChar)) { // number
                if (tokens.size() > 1 && tokens[tokens.size() - 1].value == ".") {
                    Token* t = &tokens[tokens.size() - 1];
                    t->type = TokenType::DOUBLE;
                    t->value.append(1, cChar);
                } else if (currentToken.type == TokenType::INTEGER || currentToken.type == TokenType::DOUBLE) {
                    currentToken.value.append(1, cChar);
                } else if (currentToken.type == TokenType::UNDEFINED) {
                    currentToken.type = TokenType::INTEGER;
                    currentToken.value.append(1, cChar);
                } else {
                    if (currentToken.type != TokenType::UNDEFINED) {
                        tokens.emplace_back(currentToken);
                        currentToken.type = TokenType::UNDEFINED;
                        currentToken.value = "";
                    }
                }

            } else if (std::find(std::begin(operator_list), std::end(operator_list), cChar) != std::end(operator_list)) { // operator

                if (cChar == '.' && currentToken.type == TokenType::INTEGER) {
                    currentToken.type = TokenType::DOUBLE;
                    currentToken.value.append(1, cChar);
                    continue;
                } else if (currentToken.type != TokenType::UNDEFINED) {
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
                if (tokens.size() > 1 && tokens[tokens.size() - 1].value == "\\") {
                    Token* t = &tokens[tokens.size() - 1];
                    t->type = TokenType::IDENTIFIER;
                    if (cChar == 'n') {
                        t->value = "\n";
                    } else if (cChar == 't') {
                        t->value = "\t";
                    } else if (cChar == 's') {
                        t->value = " ";
                    }

                    currentToken.type = TokenType::UNDEFINED;
                    currentToken.value = "";
                    
                } else if (currentToken.type == TokenType::UNDEFINED) {
                    currentToken.type = TokenType::IDENTIFIER;
                    currentToken.value.append(1, cChar);
                } else if (currentToken.type == TokenType::IDENTIFIER) {
                    currentToken.value.append(1, cChar);
                } else {
                    if (currentToken.type != TokenType::UNDEFINED) {
                        tokens.emplace_back(currentToken);
                        currentToken.type = TokenType::IDENTIFIER;
                        currentToken.value = cChar;
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