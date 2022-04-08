#pragma once

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>

#include "Statements.hpp"
#include "../tokenizer/Tokenizer.hpp"

namespace parser {

    const std::string data_types[] = {"void", "int", "float", "double", "long", "bool", "char"};
    const std::string math_ops[] = {"+", "-", "*", "/"};
    const std::string logic_ops[] = {"<", ">", "!", "="};

    class Parser {

        public:

            static tokenizer::Token* get_next();
            static bool is_next();
            static std::vector<Statement*> parse(std::vector<tokenizer::Token> tokens);

            static std::optional<Statement*> expect_function();
            static std::optional<Statement*> expect_expression(bool skip_semicolon = false);
            static std::optional<Statement*> expect_variable_call();
            static std::optional<Statement*> expect_value_expression(bool skipBin, bool skipLog);
            static std::optional<Statement*> expect_function_call();
            static std::optional<Statement*> expect_type_cast();

            static std::optional<Statement*> expect_logic_expression();
            static std::optional<Statement*> expect_logic_RHS(Statement* LHS);

            static std::optional<Statement*> expect_binary_expression();
            static std::optional<Statement*> expect_binary_RHS(int prec, Statement* LHS);

            static std::optional<Statement*> expect_variable_definition();
            static std::optional<Statement*> expect_variable_assignment();
            static std::optional<Statement*> expect_if();
            static std::optional<Statement*> expect_for();

            static std::optional<tokenizer::Token*> expect_identifier(const std::string& name);
            static std::optional<tokenizer::Token*> expect_operator(const std::string& name);
            static std::optional<tokenizer::Token*> expect_type(const std::string& name);
            static std::optional<tokenizer::Token*> expect_integer();
            static std::optional<tokenizer::Token*> expect_long_int();
            static std::optional<tokenizer::Token*> expect_char();
            static std::optional<tokenizer::Token*> expect_boolean();

            static int get_precedence(tokenizer::Token* token);

            static void error(tokenizer::Token* token, const std::string& message);

    };
    
}