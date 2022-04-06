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
            compileExpression(s, mod);
        }

        llvm::verifyFunction(*F);
    }

    llvm::ReturnInst* compileReturn(parser::Statement* statement, llvm::Module* mod) {
        Builder.CreateRet(compileValueExpression(statement->statements[0], mod));
    }

    llvm::Value* compileValueExpression(parser::Statement* statement, llvm::Module* mod) {
        if (statement->type == parser::StatementType::INTEGER_LITERAL) {
            return llvm::ConstantInt::get(llvmContext, llvm::APInt(32, std::stoi(statement->value)));
        }
    }

    llvm::Value* compileExpression(parser::Statement* statement, llvm::Module* mod) {
        // code block
        if (statement->type == parser::StatementType::CODE_BLOCK) {
            for (parser::Statement* s : statement->statements) {
                compileExpression(s, mod);
            }
        }

        // return
        if (statement->type == parser::StatementType::RETURN) {
            compileReturn(statement, mod);
            return nullptr;
        }
    }

}