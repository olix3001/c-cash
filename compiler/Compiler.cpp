#include "Compiler.hpp"

namespace compiler {

    llvm::LLVMContext llvmContext;
    llvm::IRBuilder<> Builder(llvmContext);
    
    llvm::Module* compileModule(std::vector<parser::Statement*> module) {
        // create module
        llvm::Module* mod = new llvm::Module("test", llvmContext);

        for (parser::Statement* s : module) {
            if (s->type == parser::StatementType::FUNCTION_DEFINITION)
                compileFunction(s, mod);
        }
        

        mod->print(llvm::errs(), nullptr);

        // llvm::FunctionPassManager* pm = new llvm::FunctionPassManager(mod);

        // pm->add(llvm::createPromoteMemoryToRegisterPass());
        // pm->add(createInstructionCombiningPass());

        // pm->doInitialization();

        return mod;
    }

    llvm::Function* compileFunction(parser::Statement* statement, llvm::Module* mod) {
        // function type
        llvm::FunctionType* FT = nullptr;
        if (statement->dataType == "void")
                FT = llvm::FunctionType::get(llvm::Type::getVoidTy(llvmContext), false);
        else if (statement->dataType == "int")
                FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvmContext), false);

        // function
        llvm::Function* F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, statement->value, mod);

        // function block
        llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(llvmContext, "entry", F);

        Builder.SetInsertPoint(entryBlock);
        
        for (parser::Statement* s : statement->statements) {
            compileExpression(s, mod, F);
        }

        llvm::verifyFunction(*F);
    }

    llvm::AllocaInst* allocateEntry(llvm::Function* func, llvm::Type* t, const std::string& name) {
        llvm::IRBuilder<> tmpB(&func->getEntryBlock(), func->getEntryBlock().begin());
        return tmpB.CreateAlloca(t, 0, name.c_str());
    }

    llvm::ReturnInst* compileReturn(parser::Statement* statement, llvm::Module* mod, llvm::Function* func) {
        Builder.CreateRet(compileValueExpression(statement->statements[0], mod, func));
    }

    llvm::StoreInst* compileVariableDefinition(parser::Statement* statement, llvm::Module* mod, llvm::Function* func) {
        llvm::AllocaInst* alloca = allocateEntry(func, compileType(statement->dataType), statement->value);
        llvm::Value* initialValue = compileValueExpression(statement->statements[0], mod, func);

        return Builder.CreateStore(initialValue, alloca);
    }

    llvm::Value* compileValueExpression(parser::Statement* statement, llvm::Module* mod, llvm::Function* func) {
        if (statement->type == parser::StatementType::INTEGER_LITERAL) {
            return llvm::ConstantInt::get(llvmContext, llvm::APInt(32, std::stoi(statement->value)));
        }
    }

    llvm::Value* compileExpression(parser::Statement* statement, llvm::Module* mod, llvm::Function* func) {
        // code block
        if (statement->type == parser::StatementType::CODE_BLOCK) {
            for (parser::Statement* s : statement->statements) {
                compileExpression(s, mod, func);
            }
        }

        // return
        if (statement->type == parser::StatementType::RETURN) {
            compileReturn(statement, mod, func);
            return nullptr;
        }

        // variable definition
        if (statement->type == parser::StatementType::VARIABLE_DEFINITON) {
            compileVariableDefinition(statement, mod, func);
            return nullptr;
        }
    }

    llvm::Type* compileType(const std::string& type) {
        if (type == "int") return llvm::Type::getInt32Ty(llvmContext);
    }

}