#include "tokenizer/Tokenizer.hpp"
#include "parser/Parser.hpp"
#include "parser/Statements.hpp"
#include "compiler/Compiler.hpp"


#include <string>
#include <vector>
#include <iostream>
#include <fstream>

int main(int argc, char **argv) {

    // create arg object
    std::vector<std::string> args(argv, argv + argc);
    
    // open file
    std::ifstream file;
    file.open(args[1]);

    std::string line, allCode="";
    while (std::getline(file, line)) {
        allCode += line + '\n';
    }

    std::vector<tokenizer::Token> tokens = tokenizer::tokenize(allCode);

    for (auto t : tokens) {
        t.debug_print();
    }

    std::vector<parser::Statement*> AST = parser::Parser::parse(tokens);

    for (auto s : AST) {
        s->debug_print(0);
    }

    llvm::Module* mm = compiler::compileModule(AST, compiler::base_name(args[1]));

    parser::Parser::saveCompilation(mm, compiler::base_name(args[1]) + ".o");

}