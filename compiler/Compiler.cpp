#include "Compiler.hpp"

namespace compiler {

    template<typename Base, typename T>
    inline bool instanceof(const T *ptr) {
    return dynamic_cast<const Base*>(ptr) != nullptr;
    }


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

        std::vector<llvm::Type*> argsT;

        // arguments declaration
        for (auto arg : statement->args) {
            argsT.emplace_back(compileType(arg.first));
        }

        // function type
        llvm::FunctionType* FT = llvm::FunctionType::get(compileType(statement->dataType), argsT, false);

        // function
        llvm::Function* F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, statement->value, mod);

        // function block
        llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(llvmContext, "entry", F);

        Builder.SetInsertPoint(entryBlock);
        parser::Scope* funcScope = new parser::Scope();

        // arguments definition
        int index = 0;
        for (auto& arg : F->args()) {
            arg.setName(statement->args[index++].second);

            llvm::AllocaInst* alloca = allocateEntry(F, arg.getType(), std::string(arg.getName()));
            Builder.CreateStore(&arg, alloca);
            
            funcScope->namedValues[std::string(arg.getName())] = alloca;
        }
        
        for (parser::Statement* s : statement->statements) {
            compileExpression(s, mod, F, funcScope);
        }

        llvm::verifyFunction(*F);

        return F;
    }

    llvm::AllocaInst* allocateEntry(llvm::Function* func, llvm::Type* t, const std::string& name) {
        llvm::IRBuilder<> tmpB(&func->getEntryBlock(), func->getEntryBlock().begin());
        return tmpB.CreateAlloca(t, 0, name.c_str());
    }

    llvm::ReturnInst* compileReturn(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        return Builder.CreateRet(compileValueExpression(statement->statements[0], mod, func, scope));
    }

    llvm::Value* compileVariableCall(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        return Builder.CreateLoad(scope->namedValues[statement->value], statement->value);
    }

    llvm::Value* compileFunctionCall(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        llvm::Function* F = mod->getFunction(statement->value);
        if (!F) return nullptr;

        std::vector<llvm::Value*> args;
        for (auto arg : statement->statements) {
            args.emplace_back(compileValueExpression(arg, mod, func, scope));
        }

        return Builder.CreateCall(F, args, "calltmp");
    }

    llvm::StoreInst* compileVariableDefinition(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        llvm::AllocaInst* alloca = allocateEntry(func, compileType(statement->dataType), statement->value);
        llvm::Value* initialValue = compileValueExpression(statement->statements[0], mod, func, scope);

        scope->namedValues[statement->value] = alloca;
        return Builder.CreateStore(initialValue, alloca);
    }

    llvm::Value* compileMath(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        switch (statement->value[0]) {
            case '+':
                return Builder.CreateAdd(
                    compileValueExpression(statement->statements[0], mod, func, scope), 
                    compileValueExpression(statement->statements[1], mod, func, scope),
                    "addtmp"
                );
            case '-':
                return Builder.CreateSub(
                    compileValueExpression(statement->statements[0], mod, func, scope), 
                    compileValueExpression(statement->statements[1], mod, func, scope),
                    "subtmp"
                );
            case '*':
                return Builder.CreateMul(
                    compileValueExpression(statement->statements[0], mod, func, scope), 
                    compileValueExpression(statement->statements[1], mod, func, scope),
                    "multmp"
                );
            case '/':
                return Builder.CreateSDiv(
                    compileValueExpression(statement->statements[0], mod, func, scope), 
                    compileValueExpression(statement->statements[1], mod, func, scope),
                    "divtmp"
                );
            default:
                std::cout << "NO OPERATOR\n";
        }
    }

    llvm::Value* compileLogicExpr(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        if (statement->value.size() == 1) {
            switch (statement->value[0]) {
                case '<':
                    return Builder.CreateICmpSLT(
                        compileValueExpression(statement->statements[0], mod, func, scope), 
                        compileValueExpression(statement->statements[1], mod, func, scope), 
                        "lttmp"
                    );
                case '>':
                    return Builder.CreateICmpSGT(
                        compileValueExpression(statement->statements[0], mod, func, scope), 
                        compileValueExpression(statement->statements[1], mod, func, scope), 
                        "gttmp"
                    );
            }
        } else {
            switch(statement->value[0]) {
                case '<':
                    return Builder.CreateICmpSLE(
                        compileValueExpression(statement->statements[0], mod, func, scope), 
                        compileValueExpression(statement->statements[1], mod, func, scope), 
                        "letmp"
                    );
                case '>':
                    return Builder.CreateICmpSGE(
                        compileValueExpression(statement->statements[0], mod, func, scope), 
                        compileValueExpression(statement->statements[1], mod, func, scope), 
                        "getmp"
                    );
                case '!':
                    return Builder.CreateICmpNE(
                        compileValueExpression(statement->statements[0], mod, func, scope), 
                        compileValueExpression(statement->statements[1], mod, func, scope), 
                        "netmp"
                    );
                case '=':
                    return Builder.CreateICmpEQ(
                        compileValueExpression(statement->statements[0], mod, func, scope), 
                        compileValueExpression(statement->statements[1], mod, func, scope), 
                        "eqtmp"
                    );
                    
            }
        }
    }

    llvm::Value* compileTypeCast(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        llvm::Value* v = compileValueExpression(statement->statements[0], mod, func, scope);
        // ty<int> -> ty1<int>
        if ((statement->value == "int" || statement->value == "bool" || statement->value == "long" || statement->value == "char") && v->getType()->isIntegerTy()) {
            return Builder.CreateIntCast(v, compileType(statement->value), true);
        }

        // bitcast for all other types
        return Builder.CreateBitCast(compileValueExpression(statement->statements[0], mod, func, scope), compileType(statement->value), "casttmp");
    }

    llvm::Value* compileValueExpression(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        if (statement->type == parser::StatementType::INTEGER_LITERAL) {
            return llvm::ConstantInt::get(llvmContext, llvm::APInt(32, std::stoi(statement->value)));
        }
        if (statement->type == parser::StatementType::LONG_LITERAL) {
            return llvm::ConstantInt::get(llvmContext, llvm::APInt(64, std::stol(statement->value)));
        }
        if (statement->type == parser::StatementType::CHAR_LITERAL) {
            return llvm::ConstantInt::get(llvmContext, llvm::APInt(8, statement->value[0]));
        }
        if (statement->type == parser::StatementType::BOOLEAN_LITERAL) {
            return llvm::ConstantInt::get(llvmContext, llvm::APInt(1, (statement->value == "true" ? 1 : 0)));
        }
        if (statement->type == parser::StatementType::VARIABLE_CALL) {
            return compileVariableCall(statement, mod, func, scope);
        }
        if (statement->type == parser::StatementType::FUNCTION_CALL) {
            return compileFunctionCall(statement, mod, func, scope);
        }
        if (statement->type == parser::StatementType::MATH) {
            return compileMath(statement, mod, func, scope);
        }
        if (statement->type == parser::StatementType::LOGIC_EXPRESSION) {
            return compileLogicExpr(statement, mod, func, scope);
        }
        if (statement->type == parser::StatementType::TYPE_CAST) {
            return compileTypeCast(statement, mod, func, scope);
        }
    }

    llvm::Value* compileExpression(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        // code block
        if (statement->type == parser::StatementType::CODE_BLOCK) {
            for (parser::Statement* s : statement->statements) {
                compileExpression(s, mod, func, scope);
            }
        }

        // return
        if (statement->type == parser::StatementType::RETURN) {
            compileReturn(statement, mod, func, scope);
            return nullptr;
        }

        // variable definition
        if (statement->type == parser::StatementType::VARIABLE_DEFINITON) {
            compileVariableDefinition(statement, mod, func, scope);
            return nullptr;
        }

        if (statement->type == parser::StatementType::FUNCTION_CALL) {
            return compileFunctionCall(statement, mod, func, scope);
        }

        return nullptr;
    }

    llvm::Type* compileType(const std::string& type) {
        if (type == "int") return llvm::Type::getInt32Ty(llvmContext);
        if (type == "long") return llvm::Type::getInt64Ty(llvmContext);
        if (type == "char") return llvm::Type::getInt8Ty(llvmContext);
        if (type == "bool") return llvm::Type::getInt1Ty(llvmContext);
        if (type == "void") return llvm::Type::getVoidTy(llvmContext);
    }

}