#pragma once

#include <iostream>
#include <vector>

#include "../parser/Parser.hpp"
#include "../parser/Statements.hpp"

// llvm imports
#include "llvm/IR/Value.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"

namespace compiler {

    llvm::Module* compileModule(std::vector<parser::Statement*> module);

    llvm::Function* compileFunction(parser::Statement* statement, llvm::Module* mod);
    llvm::Value* compileExpression(parser::Statement* statement, llvm::Module* mod);
    llvm::Value* compileValueExpression(parser::Statement* statement, llvm::Module* mod);
    llvm::ReturnInst* compileReturn(parser::Statement* statement, llvm::Module* mod);

}