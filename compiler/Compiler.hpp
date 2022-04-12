#pragma once

#include <iostream>
#include <vector>
#include <fstream>

#include "../parser/Parser.hpp"
#include "../tokenizer/Tokenizer.hpp"
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

    llvm::Module* compileModule(std::vector<parser::Statement*> module, const std::string& name);

    std::string base_name(std::string const & path);

    llvm::Module* compileImport(parser::Statement* statement, llvm::Module* mod);
    llvm::Function* compileFunction(parser::Statement* statement, llvm::Module* mod);
    llvm::Value* compileExpression(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileValueExpression(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::ReturnInst* compileReturn(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileVariableDefinition(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileVariableCall(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileString(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileArrayDef(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileArrayCall(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileVariableAssignment(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileFunctionCall(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileMath(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileIfStatement(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileForStatement(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileTypeCast(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileLogicExpr(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Value* compileGetAlloca(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);

    llvm::AllocaInst* allocateEntry(llvm::Function* func, llvm::Type* t, const std::string& name);

    llvm::Function* compileIntrinsic(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope);
    llvm::Function* createFDeclaration(llvm::Module* mod, const std::string& name, llvm::Type* rt, std::vector<llvm::Type*> at, bool varargs);

    llvm::Type* compileType(const std::string& type);

}