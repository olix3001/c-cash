#include "Compiler.hpp"

namespace compiler {

    template<typename Base, typename T>
    inline bool instanceof(const T *ptr) {
    return dynamic_cast<const Base*>(ptr) != nullptr;
    }


    llvm::LLVMContext llvmContext;
    llvm::IRBuilder<> Builder(llvmContext);
    
    llvm::Module* compileModule(std::vector<parser::Statement*> module, const std::string& name) {
        // create module
        llvm::Module* mod = new llvm::Module(name, llvmContext);

        for (parser::Statement* s : module) {
            if (s->type == parser::StatementType::FUNCTION_DEFINITION) {
                compileFunction(s, mod);
            } else if (s->type == parser::StatementType::IMPORT) {
                compileImport(s, mod);
            }
        }
        
        std::cout << "\u001B[36m" << mod->getSourceFileName() << " \u001B[32mmodule llvm ir code:\u001B[0m\n";
        mod->print(llvm::errs(), nullptr);

        // llvm::FunctionPassManager* pm = new llvm::FunctionPassManager(mod);

        // pm->add(llvm::createPromoteMemoryToRegisterPass());
        // pm->add(createInstructionCombiningPass());

        // pm->doInitialization();

        return mod;
    }

    std::string base_name(std::string const & path)
    {
        return path.substr(path.find_last_of("/\\") + 1);
    }

    llvm::Module* compileImport(parser::Statement* statement, llvm::Module* mod) {
        // get code
        std::ifstream file;
        file.open(statement->value);

        std::string line, allCode="";
        while (std::getline(file, line)) {
            allCode += line + '\n';
        }


        std::cout << "\u001B[32mCompiling module \u001B[36m" << statement->value << "\u001B[0m\n";

        // tokenize and parse
        std::vector<tokenizer::Token> tokens = tokenizer::tokenize(allCode);
        std::vector<parser::Statement*> AST = parser::Parser::parse(tokens);

        // compile
        llvm::Module* im = compileModule(AST, base_name(statement->value));

        // declare functions in this module
        for (llvm::Function& m : im->getFunctionList()) {
            llvm::Function::Create(m.getFunctionType(), llvm::Function::ExternalLinkage, m.getName(), mod);
        }

        std::string objName = base_name(statement->value) + ".o";

        parser::Parser::saveCompilation(im, objName);
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
        if (statement->value == "void") {
            return Builder.CreateRet(nullptr);
        }
        return Builder.CreateRet(compileValueExpression(statement->statements[0], mod, func, scope));
    }

    llvm::Value* compileGetAlloca(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        return scope->namedValues[statement->value];
    }

    llvm::Value* compileVariableCall(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        return Builder.CreateLoad(static_cast<llvm::AllocaInst*>(scope->namedValues[statement->value])->getAllocatedType(), 
        scope->namedValues[statement->value], statement->value);
    }

    llvm::Function* createFDeclaration(llvm::Module* mod, const std::string& name, llvm::Type* rt, std::vector<llvm::Type*> at, bool varargs) {
        llvm::FunctionType* ft = llvm::FunctionType::get(rt, at, varargs);
        return llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, mod);
    }

    llvm::Function* compileIntrinsic(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        // printf intrinsic
        if (statement->value == "printf") {
            std::vector<llvm::Type*> at { llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0) };
            return createFDeclaration(mod, "printf", llvm::IntegerType::getInt32Ty(llvmContext), at, true);
        }
        // scanf intrinsic
        if (statement->value == "scanf") {
            std::vector<llvm::Type*> at { llvm::PointerType::get(llvm::Type::getInt8Ty(llvmContext), 0) };
            return createFDeclaration(mod, "scanf", llvm::IntegerType::getInt32Ty(llvmContext), at, true);
        }

        return nullptr;
    }

    llvm::Value* compileFunctionCall(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        llvm::Function* F;

        compileIntrinsic(statement, mod, func, scope);

        F = mod->getFunction(statement->value);
        if (!F) return nullptr;

        std::vector<llvm::Value*> args;
        for (auto arg : statement->statements) {
            args.emplace_back(compileValueExpression(arg, mod, func, scope));
        }

        return Builder.CreateCall(F, args, "calltmp");
    }

    llvm::Value* compileVariableAssignment(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        llvm::Value* val = compileValueExpression(statement->statements[0], mod, func, scope);
        Builder.CreateStore(val, scope->namedValues[statement->value]);

        return Builder.CreateLoad(static_cast<llvm::AllocaInst*>(scope->namedValues[statement->value])->getAllocatedType(), 
        scope->namedValues[statement->value], statement->value);
    }

    llvm::Value* compileVariableDefinition(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        llvm::AllocaInst* alloca = allocateEntry(func, compileType(statement->dataType), statement->value);
        llvm::Value* initialValue = compileValueExpression(statement->statements[0], mod, func, scope);

        scope->namedValues[statement->value] = alloca;
        Builder.CreateStore(initialValue, alloca);

        return Builder.CreateLoad(alloca->getAllocatedType(), scope->namedValues[statement->value], statement->value);
    }

    llvm::Value* compileForStatement(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {

        parser::Scope* loopScope = new parser::Scope(scope);

        // compile before loop
        compileValueExpression(statement->statements[0], mod, func, loopScope);

        // create loop block
        llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(llvmContext, "loop.body", func);
        Builder.CreateBr(loopBB);

        Builder.SetInsertPoint(loopBB);

        // compile function body
        compileExpression(statement->statements[3], mod, func, loopScope);

        // compile after expression
        llvm::Value* afterExpr = compileValueExpression(statement->statements[2], mod, func, loopScope);
        // compile end expression
        llvm::Value* endCond = compileValueExpression(statement->statements[1], mod, func, loopScope);

        // after loop block
        llvm::BasicBlock* afterLoopBB = llvm::BasicBlock::Create(llvmContext, "loop.after", func);

        Builder.CreateCondBr(endCond, loopBB, afterLoopBB);

        Builder.SetInsertPoint(afterLoopBB);

        return nullptr;
    }

    llvm::Value* compileIfStatement(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        // get condition value
        llvm::Value* cond = compileValueExpression(statement->statements[0], mod, func, scope);

        // create then block
        llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(llvmContext, "if.then", func);

        // create else block if exists
        bool hasElse = statement->type == parser::StatementType::IFELSE;
        llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(llvmContext, "if.else", func);;

        // create block to continue code flow
        llvm::BasicBlock *contBB = llvm::BasicBlock::Create(llvmContext, "if.cont", func);

        Builder.CreateCondBr(cond, thenBB, elseBB);

        // compile true code
        Builder.SetInsertPoint(thenBB);
        parser::Scope* trueScope = new parser::Scope(scope);
        compileExpression(statement->statements[1], mod, func, trueScope);
        Builder.CreateBr(contBB);

        thenBB = Builder.GetInsertBlock();

        // compile false code if exists
        Builder.SetInsertPoint(elseBB);
        if (hasElse) {
            parser::Scope* falseScope = new parser::Scope(scope);
            compileExpression(statement->statements[2], mod, func, falseScope);
        }
        Builder.CreateBr(contBB);

        elseBB = Builder.GetInsertBlock();

        Builder.SetInsertPoint(contBB);

        std::vector<llvm::Type*> types;
        std::vector<llvm::Value*> args;
        // Builder.CreateIntrinsic(llvm::Intrinsic::donothing, types, args);


        return nullptr;
    }

    llvm::Value* compileMath(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        llvm::Value* av = compileValueExpression(statement->statements[0], mod, func, scope);
        if (av->getType()->isIntegerTy()) { // integer
            switch (statement->value[0]) {
                case '+':
                    return Builder.CreateAdd(
                        av, 
                        compileValueExpression(statement->statements[1], mod, func, scope),
                        "addtmp"
                    );
                case '-':
                    return Builder.CreateSub(
                        av, 
                        compileValueExpression(statement->statements[1], mod, func, scope),
                        "subtmp"
                    );
                case '*':
                    return Builder.CreateMul(
                        av, 
                        compileValueExpression(statement->statements[1], mod, func, scope),
                        "multmp"
                    );
                case '/':
                    return Builder.CreateSDiv(
                        av, 
                        compileValueExpression(statement->statements[1], mod, func, scope),
                        "divtmp"
                    );
                default:
                    std::cout << "NO OPERATOR\n";
            }
        } else { // floating point
            switch (statement->value[0]) {
                case '+':
                    return Builder.CreateFAdd(
                        av, 
                        compileValueExpression(statement->statements[1], mod, func, scope),
                        "faddtmp"
                    );
                case '-':
                    return Builder.CreateFSub(
                        av, 
                        compileValueExpression(statement->statements[1], mod, func, scope),
                        "fsubtmp"
                    );
                case '*':
                    return Builder.CreateFMul(
                        av, 
                        compileValueExpression(statement->statements[1], mod, func, scope),
                        "fmultmp"
                    );
                case '/':
                    return Builder.CreateFDiv(
                        av, 
                        compileValueExpression(statement->statements[1], mod, func, scope),
                        "fdivtmp"
                    );
                default:
                    std::cout << "NO OPERATOR\n";
            }
        }
    }

    llvm::Value* compileLogicExpr(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        llvm::Value* av = compileValueExpression(statement->statements[0], mod, func, scope);

        if (av->getType()->isIntegerTy()) { // integer
            if (statement->value.size() == 1) {
                switch (statement->value[0]) {
                    case '<':
                        return Builder.CreateICmpSLT(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "lttmp"
                        );
                    case '>':
                        return Builder.CreateICmpSGT(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "gttmp"
                        );
                }
            } else {
                switch(statement->value[0]) {
                    case '<':
                        return Builder.CreateICmpSLE(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "letmp"
                        );
                    case '>':
                        return Builder.CreateICmpSGE(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "getmp"
                        );
                    case '!':
                        return Builder.CreateICmpNE(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "netmp"
                        );
                    case '=':
                        return Builder.CreateICmpEQ(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "eqtmp"
                        );
                        
                }
            }
        } else { // double
            if (statement->value.size() == 1) {
                switch (statement->value[0]) {
                    case '<':
                        return Builder.CreateFCmpOLT(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "flttmp"
                        );
                    case '>':
                        return Builder.CreateFCmpOGT(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "fgttmp"
                        );
                }
            } else {
                switch(statement->value[0]) {
                    case '<':
                        return Builder.CreateFCmpOLE(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "fletmp"
                        );
                    case '>':
                        return Builder.CreateFCmpOGE(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "fgetmp"
                        );
                    case '!':
                        return Builder.CreateFCmpONE(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "fnetmp"
                        );
                    case '=':
                        return Builder.CreateFCmpOEQ(
                            av, 
                            compileValueExpression(statement->statements[1], mod, func, scope), 
                            "feqtmp"
                        );
                        
                }
            }
        }
    }

    llvm::Value* compileTypeCast(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        llvm::Value* v = compileValueExpression(statement->statements[0], mod, func, scope);
        // ty<int> -> ty1<int>
        if ((statement->value == "int" || statement->value == "bool" || statement->value == "long" || statement->value == "char") && v->getType()->isIntegerTy()) {
            return Builder.CreateIntCast(v, compileType(statement->value), true);
        }

        // ty<double> -> ty1<int>
        if (statement->value == "int" && v->getType()->isDoubleTy()) {
            return Builder.CreateFPToSI(v, compileType(statement->value));
        }

        // ty<int> -> ty1<double>
        if (statement->value == "double" && v->getType()->isIntegerTy()) {
            return Builder.CreateSIToFP(v, compileType(statement->value));
        }

        // bitcast for all other types
        return Builder.CreateBitCast(compileValueExpression(statement->statements[0], mod, func, scope), compileType(statement->value), "casttmp");
    }

    llvm::Value* compileString(parser::Statement* statement, llvm::Module* mod, llvm::Function* func, parser::Scope* scope) {
        return Builder.CreateGlobalStringPtr(llvm::StringRef(statement->value), "__const.str");
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
        if (statement->type == parser::StatementType::DOUBLE_LITERAL) {
            return llvm::ConstantFP::get(llvmContext, llvm::APFloat(std::stod(statement->value)));
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
        if (statement->type == parser::StatementType::STRING) {
            return compileString(statement, mod, func, scope);
        }
        if (statement->type == parser::StatementType::GET_ALLOCA) {
            return compileGetAlloca(statement, mod, func, scope);
        }

        // variable definition
        if (statement->type == parser::StatementType::VARIABLE_DEFINITON) {
            return compileVariableDefinition(statement, mod, func, scope);
        }

        if (statement->type == parser::StatementType::VARIABLE_ASSIGNMENT) {
            return compileVariableAssignment(statement, mod, func, scope);
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

        // if statement
        if (statement->type == parser::StatementType::IF || statement->type == parser::StatementType::IFELSE) {
            compileIfStatement(statement, mod, func, scope);
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

        if (statement->type == parser::StatementType::VARIABLE_ASSIGNMENT) {
            return compileVariableAssignment(statement, mod, func, scope);
        }

        if (statement->type == parser::StatementType::FOR_LOOP) {
            return compileForStatement(statement, mod, func, scope);
        }

        return nullptr;
    }

    llvm::Type* compileType(const std::string& type) {
        std::string tn = type;
        bool isPointer = false;
        // pointer type
        if (type[type.size()-1] == '*') {
            isPointer = true;
            tn = tn.substr(0, tn.size()-1);
        }

        llvm::Type* rt;

        // type
        if (tn == "int") rt = llvm::Type::getInt32Ty(llvmContext);
        else if (tn == "long") rt = llvm::Type::getInt64Ty(llvmContext);
        else if (tn == "char") rt = llvm::Type::getInt8Ty(llvmContext);
        else if (tn == "bool") rt = llvm::Type::getInt1Ty(llvmContext);
        else if (tn == "double") rt = llvm::Type::getDoubleTy(llvmContext);
        else if (tn == "void") rt = llvm::Type::getVoidTy(llvmContext);

        if (isPointer) {
            return llvm::PointerType::get(rt, 0);
        }

        return rt;

    }

}