#include "Compiler.hpp"

namespace compiler {

    llvm::LLVMContext llvmContext;
    llvm::IRBuilder<> Builder(llvmContext);
    
    llvm::Module* compileModule(std::vector<parser::Statement*> module) {
        // create module
        llvm::Module* mod = new llvm::Module("test", llvmContext);

        compileFunction(module[0], mod);

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
        Builder.CreateRet(llvm::ConstantInt::get(llvmContext, llvm::APInt(32, 0)));

        llvm::verifyFunction(*F);
    }

}