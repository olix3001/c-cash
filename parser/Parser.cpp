#include "Parser.hpp"

namespace parser {

    int cTokenI = 0;
    tokenizer::Token* cToken = nullptr;
    std::vector<tokenizer::Token> Tokens;

    tokenizer::Token* Parser::get_next() {
        if (cTokenI >= Tokens.size()) return nullptr;
        cToken = &Tokens[cTokenI++];
        return cToken;
    }
    bool Parser::is_next() {
        return cTokenI < Tokens.size()-1;
    }

    std::vector<Statement*> Parser::parse(std::vector<tokenizer::Token> tokens) {
        cTokenI = 0;
        Tokens = tokens;
        std::vector<Statement*> result;

        while(is_next()) {
            get_next();
            // parse function declaration
            std::optional<Statement*> fDefinition = expect_function();
            if (fDefinition.has_value()) {
                result.emplace_back(fDefinition.value());
            }
        }

        return result;
    }

    std::optional<tokenizer::Token*> Parser::expect_identifier(const std::string& name = std::string()) {
        if (cToken->type != tokenizer::TokenType::IDENTIFIER) { return std::nullopt; }
        if (!name.empty() && cToken->value != name) { return std::nullopt; }

        std::regex pattern("[A-Za-z_]\\w*");
        std::smatch result;
        if(!regex_match(cToken->value, result, pattern)) error(cToken, "You can only use letters, digits and _ in identifiers"); 

        tokenizer::Token* returnToken = cToken;
        get_next();
        return returnToken;
    }

    std::optional<tokenizer::Token*> Parser::expect_operator(const std::string &name) {
        if(cToken->type != tokenizer::TokenType::OPERATOR ) { return std::nullopt; }
        if(!name.empty() && cToken->value != name) { return std::nullopt; }

        tokenizer::Token* returnToken = cToken;
        get_next();
        return returnToken;
    }

    std::optional<tokenizer::Token*> Parser::expect_type(const std::string& name = std::string()) {
        if(cToken->type != tokenizer::TokenType::IDENTIFIER ) { return std::nullopt; }
        if (std::find(std::begin(data_types), std::end(data_types), cToken->value) == std::end(data_types)) { return std::nullopt; }
        if(!name.empty() && cToken->value != name) { return std::nullopt; }

        tokenizer::Token* returnToken = cToken;
        get_next();
        return returnToken;
    }


    std::optional<Statement*> Parser::expect_function() {
        // expect "def" keyword
        if (!expect_identifier("def").has_value()) { return std::nullopt; }

        // expect function type
        std::optional<tokenizer::Token*> typeToken = expect_type();
        if (!typeToken.has_value()) { --cTokenI; return std::nullopt; }

        // expect function name
        std::optional<tokenizer::Token*> nameToken = expect_identifier();
        if (!nameToken.has_value()) { cTokenI-=2; return std::nullopt; }

        Statement* fd = new Statement(StatementType::FUNCTION_DEFINITION, nameToken.value()->value);
        fd->dataType = typeToken.value()->value;

        return fd;
        // todo: implement rest
    }


    void Parser::error(tokenizer::Token* token, const std::string& message) {
        std::cout << token->lineNo << ":" << token->charNo << std::endl;
        throw std::runtime_error(message);
    }

}