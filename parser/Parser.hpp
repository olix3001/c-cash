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

    const std::string data_types[] = {"void", "int", "float", "double", "long", "bool"};

    class Parser {

        public:

            static tokenizer::Token* get_next();
            static bool is_next();
            static std::vector<Statement*> parse(std::vector<tokenizer::Token> tokens);

            static std::optional<Statement*> expect_function();
            static std::optional<Statement*> expect_expression();
            static std::optional<Statement*> expect_value_expression();

            static std::optional<Statement*> expect_variable_definition();

            static std::optional<tokenizer::Token*> expect_identifier(const std::string& name);
            static std::optional<tokenizer::Token*> expect_operator(const std::string& name);
            static std::optional<tokenizer::Token*> expect_type(const std::string& name);
            static std::optional<tokenizer::Token*> expect_integer();

            static void error(tokenizer::Token* token, const std::string& message);

    };
    
}