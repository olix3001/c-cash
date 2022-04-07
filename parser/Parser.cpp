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

        std::cout << "Compilation debug:\n";

        get_next();
        while(is_next()) {

            // parse function declaration
            std::optional<Statement*> fDefinition = expect_function();
            if (fDefinition.has_value()) {
                result.emplace_back(fDefinition.value());
            } else {
                error(cToken, "Expected global definition like function, variable or class");
            }
        }

        std::cout << result.size() << '\n';
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

    std::optional<tokenizer::Token*> Parser::expect_integer() {
        if(cToken->type != tokenizer::TokenType::INTEGER ) { return std::nullopt; }

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

    std::optional<Statement*> Parser::expect_variable_call() {
        std::optional<tokenizer::Token*> nameToken = expect_identifier();
        if (!nameToken.has_value()) return std::nullopt;

        return new Statement(StatementType::VARIABLE_CALL, nameToken.value()->value);
    }


    std::optional<Statement*> Parser::expect_variable_definition() {
        if (!expect_identifier("var").has_value()) { return std::nullopt; }

        // expect variable type
        std::optional<tokenizer::Token*> typeToken = expect_type();
        if (!typeToken.has_value()) { --cTokenI; return std::nullopt; }

        // expect variable name
        std::optional<tokenizer::Token*> nameToken = expect_identifier();
        if (!nameToken.has_value()) { cTokenI-=2; return std::nullopt; }

        // expect initialization
        if (!expect_operator("=").has_value()) { error(cToken, "Expected '=' in variable initialization"); }

        // expect default value
        std::optional<Statement*> defVal = expect_value_expression();
        if (!defVal.has_value()) { error(cToken, "Expected variable initial value"); }

        // expect semicolon to end the command
        if (!expect_operator(";").has_value()) { error(cToken, "Expected ';'"); }

        Statement* stmt = new Statement(StatementType::VARIABLE_DEFINITON, nameToken.value()->value);
        stmt->dataType = typeToken.value()->value;
        stmt->statements.emplace_back(defVal.value());
        
        return stmt;
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

        // arguments
        if (!expect_operator("(").has_value()) { error(cToken, "Expected '('"); }
        // TODO: support arguments
        if (!expect_operator(")").has_value()) { error(cToken, "Expected ')'"); }

        // function
        std::optional<Statement*> expr = expect_expression();
        if (!expr.has_value()) { error(cToken, "Expected function body"); }
        fd->statements.emplace_back(expr.value());

        return fd;
    }

    std::optional<Statement*> Parser::expect_value_expression() {
        // primitives
        std::optional<tokenizer::Token*> ct;
        std::optional<Statement*> cs;

        if ((ct = expect_integer()).has_value()) {
            return new Statement(StatementType::INTEGER_LITERAL, ct.value()->value);
        }

        // variable
        if ((cs = expect_variable_call()).has_value()) {
            return cs;
        }
    }

    std::optional<Statement*> Parser::expect_expression() {
        // block
        if (expect_operator("{").has_value()) {
            Statement* stmt = new Statement(StatementType::CODE_BLOCK, "");
            while (true) {
                if (expect_operator("}").has_value()) break;

                std::optional<Statement*> expr = expect_expression();
                stmt->statements.emplace_back(expr.value());

                if (!expr.has_value()) { error(cToken, "Expected expression or '}'"); }
            }
            return stmt;
        }

        // return
        if (expect_identifier("return").has_value()) {
            std::optional<Statement*> retVal = expect_value_expression();
            if (!retVal.has_value()) { error(cToken, "Expected return value"); }

            Statement* retExpr = new Statement(StatementType::RETURN, "");
            retExpr->statements.emplace_back(retVal.value());
            if (!expect_operator(";").has_value()) { error(cToken, "Expected ';'"); }
            return retExpr;
        }

        std::optional<Statement*> temp;

        // variable definition
        temp = expect_variable_definition();
        if (temp.has_value()) {
            return temp;
        }
    }


    void Parser::error(tokenizer::Token* token, const std::string& message) {
        std::cout << token->lineNo << ":" << token->charNo << std::endl;
        throw std::runtime_error(message);
    }

}