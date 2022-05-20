#include "Parser.hpp"

namespace parser {

    int cTokenI = 0;
    tokenizer::Token* cToken = nullptr;
    std::vector<tokenizer::Token> Tokens;
    std::map<char, int> operator_precedence = { {'<', 20}, {'>', 20}, {'+', 20}, {'-', 20}, {'*', 40}, {'/', 40} };

    tokenizer::Token* Parser::get_next() {
        if (cTokenI >= Tokens.size()) return nullptr;
        cToken = &Tokens[cTokenI++];
        return cToken;
    }
    bool Parser::is_next() {
        return cTokenI < Tokens.size()-1;
    }

    void Parser::saveCompilation(llvm::Module* mod, const std::string& filename) {
        // #ifdef __linux__ 

        auto TargetTriple = llvm::sys::getDefaultTargetTriple();

        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();

        std::string Error;
        auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

        if (!Target) {
            llvm::errs() << Error;
            return;
        }

        auto CPU = "generic";
        auto Features = "";

        llvm::TargetOptions opt;
        auto RM = llvm::Optional<llvm::Reloc::Model>();
        auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

        mod->setDataLayout(TargetMachine->createDataLayout());
        mod->setTargetTriple(TargetTriple);

        std::error_code EC;
        llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);

        if (EC) {
            llvm::errs() << "Could not open file: " << EC.message();
            return;
        }

        llvm::legacy::PassManager pass;
        auto FileType = llvm::CGFT_ObjectFile;

        if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
            llvm::errs() << "TargetMachine can't emit a file of this type";
            return;
        }

        pass.run(*mod);
        dest.flush();

        // #else
        // return;
        // #endif

    }

    std::vector<Statement*> Parser::parse(std::vector<tokenizer::Token> tokens) {
        int cTokenITMP = cTokenI;
        tokenizer::Token* cTokenTMP = cToken;
        auto tokensTMP = Tokens;

        cTokenI = 0;
        Tokens = tokens;
        std::vector<Statement*> result;

        std::cout << "Compilation debug:\n";

        get_next();
        while(is_next()) {

            // parse function declaration
            std::optional<Statement*> definition = expect_function();
            if (definition.has_value()) {
                result.emplace_back(definition.value());
            } else if ((definition = expect_import()).has_value()) {
                result.emplace_back(definition.value());
            } else {
                error(cToken, "Expected global definition like function, variable or class");
            }
        }

        std::cout << result.size() << '\n';

        cTokenI = cTokenITMP;
        cToken = cTokenTMP;
        Tokens = tokensTMP;

        return result;
    }

    std::optional<tokenizer::Token*> Parser::expect_identifier(const std::string& name = std::string()) {
        if (cToken->type != tokenizer::TokenType::IDENTIFIER) { return std::nullopt; }
        if (!name.empty() && cToken->value != name) { return std::nullopt; }

        std::regex pattern("[A-Za-z_]\\w*");
        std::smatch result;
        if(!regex_match(cToken->value, result, pattern)) { return std::nullopt; }

        tokenizer::Token* returnToken = cToken;
        get_next();
        return returnToken;
    }

    std::optional<tokenizer::Token*> Parser::expect_double() {
        if(cToken->type != tokenizer::TokenType::DOUBLE ) { return std::nullopt; }

        tokenizer::Token* returnToken = cToken;
        get_next();
        return returnToken;
    }

    std::optional<tokenizer::Token*> Parser::expect_integer() {
        if(cToken->type != tokenizer::TokenType::INTEGER ) { return std::nullopt; }

        tokenizer::Token* returnToken = cToken;
        get_next();
        return returnToken;
    }
    std::optional<tokenizer::Token*> Parser::expect_long_int() {
        if(cToken->type != tokenizer::TokenType::INTEGER ) { return std::nullopt; }
        tokenizer::Token* returnToken = cToken;
        get_next();
        if(cToken->type != tokenizer::TokenType::IDENTIFIER || cToken->value != "l") { cTokenI-=2; get_next(); return std::nullopt; }

        get_next();
        return returnToken;
    }

    std::optional<Statement*> Parser::expect_array() {
        if (!expect_operator("[").has_value()) { return std::nullopt; }
        Statement* stmt = new Statement(StatementType::ARRAY_DEFINITION, "");

        bool isFirst = true;
        while (!expect_operator("]").has_value()) {
            if (!isFirst) {
                if (!expect_operator(",").has_value()) { error(cToken, "Expected ',' to separate array arguments"); }
            }
            isFirst = false;

            std::optional<Statement*> v = expect_value_expression(false, false);
            if (!v.has_value()) { error(cToken, "Expected value in array definition"); }
            stmt->statements.emplace_back(v.value());
        }

        return stmt;
    }

    std::optional<Statement*> Parser::expect_string() {
        if (!expect_operator("\"").has_value()) { return std::nullopt; }
        Statement* stmt = new Statement(StatementType::STRING, "");
        while (!expect_operator("\"").has_value()) {
            stmt->value += cToken->value + " ";
            get_next();
        }
        if (stmt->value[stmt->value.size() - 1] == ' ') {
            stmt->value = stmt->value.substr(0, stmt->value.size()-1);
        }
        return stmt;
    }

    std::optional<tokenizer::Token*> Parser::expect_char() {
        if(cToken->type != tokenizer::TokenType::OPERATOR ) { return std::nullopt; }
        get_next();

        if(cToken->type != tokenizer::TokenType::IDENTIFIER || cToken->value.size() != 1) { cTokenI-=2; get_next(); return std::nullopt; }
        tokenizer::Token* returnToken = cToken;
        get_next();

        if(cToken->type != tokenizer::TokenType::OPERATOR ) { cTokenI-=3; get_next(); return std::nullopt; }

        get_next();
        return returnToken;
    }
    std::optional<tokenizer::Token*> Parser::expect_boolean() {
        if(cToken->type != tokenizer::TokenType::IDENTIFIER ) { return std::nullopt; }

        if (cToken->value != "true" && cToken->value != "false") { return std::nullopt; }
        tokenizer::Token* returnToken = cToken;
        get_next();
        // returnToken->value = (cToken->value != "true" ? "1" : "0");
        return returnToken;
    }

    std::optional<tokenizer::Token*> Parser::expect_operator(const std::string &name) {
        if(cToken->type != tokenizer::TokenType::OPERATOR ) { return std::nullopt; }
        if(!name.empty() && cToken->value != name) { return std::nullopt; }

        tokenizer::Token* returnToken = cToken;
        get_next();
        return returnToken;
    }

    std::optional<tokenizer::Token*> Parser::expect_type(const std::string& name = std::string()) {
        if(cToken->type != tokenizer::TokenType::IDENTIFIER ) { return std::nullopt; }
        if (std::find(std::begin(data_types), std::end(data_types), cToken->value) == std::end(data_types)) { return std::nullopt; }
        if(!name.empty() && cToken->value != name) { return std::nullopt; }

        tokenizer::Token* returnToken = cToken;
        get_next();

        if(expect_operator("*").has_value()) { // pointer type
            returnToken->value = returnToken->value + '*';
        }

        if(expect_operator("[").has_value()) { // array type
            std::optional<tokenizer::Token*> num = expect_integer();
            if (!expect_operator("]").has_value()) { error(cToken, "Expected ']'"); }
            returnToken->value = returnToken->value + '[' + num.value()->value + ']';
        }

        return returnToken;
    }

    std::optional<Statement*> Parser::expect_array_call() {
        int tBegin = cTokenI;
        // TODO: allow nested array call and other statements than variable call
        std::optional<Statement*> nameToken = expect_variable_call();
        if (!nameToken.has_value()) return std::nullopt;

        if (!expect_operator("[").has_value()) { cTokenI = tBegin - 1; get_next(); return std::nullopt; }

        std::optional<tokenizer::Token*> index = expect_integer();
        if (!index.has_value()) { error(cToken, "Expected array index"); }
        if (!expect_operator("]").has_value()) { error(cToken, "Expected ']'"); }


        Statement* acs = new Statement(StatementType::ARRAY_CALL, "");
        Statement* is = new Statement(StatementType::INTEGER_LITERAL, index.value()->value);
        acs->statements.emplace_back(nameToken.value());
        acs->statements.emplace_back(is);


        return acs;
    }

    std::optional<Statement*> Parser::expect_variable_call() {
        std::optional<tokenizer::Token*> nameToken = expect_identifier();
        if (!nameToken.has_value()) return std::nullopt;

        return new Statement(StatementType::VARIABLE_CALL, nameToken.value()->value);
    }

    std::optional<Statement*> Parser::expect_type_cast() {
        if (!expect_operator("#").has_value()) { return std::nullopt; }

        // expect type to cast to
        std::optional<tokenizer::Token*> typeToken = expect_type();
        if (!typeToken.has_value()) { --cTokenI; get_next(); return std::nullopt; }

        // expect value to cast
        std::optional<Statement*> val = expect_value_expression(false, false);
        if (!val.has_value()) { error(cToken, "Expected value for a type cast"); }

        Statement* stmt = new Statement(StatementType::TYPE_CAST, typeToken.value()->value);
        stmt->statements.emplace_back(val.value());

        return stmt;
    }

    std::optional<Statement*> Parser::expect_variable_assignment() {
        // expect variable name
        std::optional<tokenizer::Token*> nameToken = expect_identifier();
        if (!nameToken.has_value()) { return std::nullopt; }

        // expect initialization
        if (!expect_operator("=").has_value()) { cTokenI -= 2; get_next(); return std::nullopt; }

        // expect value
        std::optional<Statement*> defVal = expect_value_expression(false, false);
        if (!defVal.has_value()) { error(cToken, "Expected variable value (a)"); }

        Statement* stmt = new Statement(StatementType::VARIABLE_ASSIGNMENT, nameToken.value()->value);
        stmt->statements.emplace_back(defVal.value());
        
        return stmt;
    }

    std::optional<Statement*> Parser::expect_variable_definition() {
        if (!expect_identifier("var").has_value()) { return std::nullopt; }

        // expect variable type
        std::optional<tokenizer::Token*> typeToken = expect_type();
        if (!typeToken.has_value()) { --cTokenI; return std::nullopt; }

        // expect variable name
        std::optional<tokenizer::Token*> nameToken = expect_identifier();
        if (!nameToken.has_value()) { cTokenI-=2; return std::nullopt; }

        Statement* stmt = new Statement(StatementType::VARIABLE_DEFINITON, nameToken.value()->value);
        stmt->dataType = typeToken.value()->value;

        // expect initialization
        if (!expect_operator("=").has_value()) { return stmt; }

        // expect default value
        std::optional<Statement*> defVal = expect_value_expression(false, false);
        if (!defVal.has_value()) { error(cToken, "Expected variable initial value"); }

        stmt->statements.emplace_back(defVal.value());
        
        return stmt;
    }

    std::optional<Statement*> Parser::expect_import() {
         // expect "import" keyword
        if (!expect_identifier("import").has_value()) { return std::nullopt; }

        std::string name = "";
        while (!expect_operator(";").has_value()) { 
            name += cToken->value;
            get_next();
        }

        return new Statement(StatementType::IMPORT, name);
    }

    std::optional<Statement*> Parser::expect_function() {
        // expect "def" keyword
        if (!expect_identifier("def").has_value()) { return std::nullopt; }

        // expect function type
        std::optional<tokenizer::Token*> typeToken = expect_type();
        if (!typeToken.has_value()) { cTokenI-=2; get_next(); return std::nullopt; }

        // expect function name
        std::optional<tokenizer::Token*> nameToken = expect_identifier();
        if (!nameToken.has_value()) { cTokenI-=3; get_next(); return std::nullopt; }

        Statement* fd = new Statement(StatementType::FUNCTION_DEFINITION, nameToken.value()->value);
        fd->dataType = typeToken.value()->value;

        // arguments
        bool isFirst = true;
        if (!expect_operator("(").has_value()) { error(cToken, "Expected '('"); }
        while (!expect_operator(")").has_value()) {
            if (!isFirst) {
                if (!expect_operator(",").has_value()) { error(cToken, "Expected ',' to separate function arguments"); }
            }
            std::optional<tokenizer::Token*> vt = expect_type();
            if (!vt.has_value()) { error(cToken, "Expected argument or ')'"); }

            std::optional<tokenizer::Token*> vn = expect_identifier();
            if (!vt.has_value()) { error(cToken, "Expected argument name"); }

            std::pair<std::string, std::string> arg;
            arg.first = vt.value()->value;
            arg.second = vn.value()->value;

            fd->args.emplace_back(arg);
            isFirst = false;
        }

        // function
        std::optional<Statement*> expr = expect_expression();
        if (!expr.has_value()) { error(cToken, "Expected function body"); }
        fd->statements.emplace_back(expr.value());


        return fd;
    }

    std::optional<Statement*> Parser::expect_get_alloca()  {
        if (!expect_operator("&").has_value()) { return std::nullopt; }
        std::optional<tokenizer::Token*> nameToken = expect_identifier();
        if (!nameToken.has_value()) return std::nullopt;

        Statement* stmt = new Statement(StatementType::GET_ALLOCA, nameToken.value()->value);

        return stmt;
    }

    std::optional<Statement*> Parser::expect_function_call() {
        std::optional<tokenizer::Token*> fName = expect_identifier();
        if (!fName.has_value()) {return std::nullopt;}

        Statement* fptr = new Statement(StatementType::FUNCTION_CALL, fName.value()->value);

        if (!expect_operator("(").has_value()) { cTokenI-=2; get_next(); return std::nullopt; }
        bool isFirst = true;
        while(!expect_operator(")").has_value()) {
            if (!isFirst) {
                if (!expect_operator(",").has_value()) { error(cToken, "Expected ',' to separate function arguments"); }
            }

            std::optional<Statement*> argS = expect_value_expression(false, false);
            if (argS.has_value()) {
                fptr->statements.emplace_back(argS.value());
            }
            isFirst = false;
        }

        return fptr;
    }

    std::optional<Statement*> Parser::expect_for() {
        // expect "if" keyword
        if (!expect_identifier("for").has_value()) { return std::nullopt; }
        Statement* FOR = new Statement(StatementType::FOR_LOOP, "");

        // expect condition
        if (!expect_operator("(").has_value()) { error(cToken, "Expected '('"); }

        // before
        std::optional<Statement*> beforeS = expect_value_expression(false, false);
        if (!expect_operator(";").has_value()) { error(cToken, "Expected ';' (fl1)"); }

        std::optional<Statement*> testS = expect_value_expression(false, false);
        if (!expect_operator(";").has_value()) { error(cToken, "Expected ';' (fl2)"); }

        std::optional<Statement*> afterS = expect_value_expression(false, false);

        if (!expect_operator(")").has_value()) { error(cToken, "Expected ')'"); }

        FOR->statements.emplace_back(beforeS.value());
        FOR->statements.emplace_back(testS.value());
        FOR->statements.emplace_back(afterS.value());

        // expect function block
        std::optional<Statement*> forBlock = expect_expression();
        if (!forBlock.has_value()) { error(cToken, "expected for loop code block"); }
        FOR->statements.emplace_back(forBlock.value());

        return FOR;
    }

    std::optional<Statement*> Parser::expect_if() {
        // expect "if" keyword
        if (!expect_identifier("if").has_value()) { return std::nullopt; }
        Statement* IF = new Statement(StatementType::IF, "");

        // expect condition
        if (!expect_operator("(").has_value()) { error(cToken, "Expected '('"); }
        std::optional<Statement*> cond = expect_value_expression(false, false);
        if (!cond.has_value()) { error(cToken, "Expected if condition"); }
        IF->statements.emplace_back(cond.value());
        if (!expect_operator(")").has_value()) { error(cToken, "Expected ')'"); }

        // expect function block
        std::optional<Statement*> ifBlock = expect_expression();
        if (!ifBlock.has_value()) { error(cToken, "expected if code block"); }
        IF->statements.emplace_back(ifBlock.value());

        // check for else statement
        if (!expect_identifier("else").has_value()) { return IF; }
        IF->type = StatementType::IFELSE;

        // expect function block
        std::optional<Statement*> elseBlock = expect_expression();
        if (!elseBlock.has_value()) { error(cToken, "expected else code block"); }
        IF->statements.emplace_back(elseBlock.value());


        return IF;
    }

    std::optional<Statement*> Parser::expect_logic_expression() {
        int tokenIB = cTokenI;
        std::optional<Statement*> LHS = expect_value_expression(false, true);
        if (!LHS.has_value()) { return std::nullopt; }

        auto tmp = expect_logic_RHS(LHS.value());
        if (!tmp.has_value()) {
            cTokenI = tokenIB-1;
            get_next();
            return std::nullopt;
        }

        return tmp;
    }
    std::optional<Statement*> Parser::expect_logic_RHS(Statement* LHS) {
        std::optional<tokenizer::Token*> OP = expect_operator("");

        if (!OP.has_value() || std::find(std::begin(logic_ops), std::end(logic_ops), OP.value()->value) == std::end(logic_ops)) { return std::nullopt; }
        std::optional<tokenizer::Token*> OPI = expect_operator("=");

        std::optional<Statement*> RHS = expect_value_expression(false, false);
        if (!RHS.has_value()) { error(cToken, "Expected right side of logic operation"); }

        Statement* ms = new Statement(StatementType::LOGIC_EXPRESSION, OP.value()->value + (OPI.has_value() ? OPI.value()->value : ""));
        ms->statements.emplace_back(LHS);
        ms->statements.emplace_back(RHS.value());

        return ms;
    }


    int get_precedence(tokenizer::Token* token) {
        if (!isascii(token->value[0])) {
            return -1;
        }

        int prec = operator_precedence[token->value[0]];
        if (prec <= 0) {
            return -1;
        }
        return prec;
    }
    std::optional<Statement*> Parser::expect_binary_expression() {
        int tokenIB = cTokenI;
        std::optional<Statement*> LHS = expect_value_expression(true, true);
        if (!LHS.has_value()) { return std::nullopt; }
        // Statement* ms = new Statement(StatementType::INTEGER_LITERAL, "1");
        // return ms;
        auto tmp = expect_binary_RHS(0, LHS.value(), tokenIB);
        if (!tmp.has_value()) { 
        }
        return tmp;
    }
    std::optional<Statement*> Parser::expect_binary_RHS(int prec, Statement* LHS, int tokenIB) {
        std::optional<tokenizer::Token*> OP = expect_operator("");

        if (!OP.has_value() || std::find(std::begin(math_ops), std::end(math_ops), OP.value()->value) == std::end(math_ops)) { cTokenI = tokenIB-1; get_next(); return std::nullopt; }

        std::optional<Statement*> RHS = expect_value_expression(false, false);
        if (!RHS.has_value()) { error(cToken, "Expected right side of binary operation"); }

        Statement* ms = new Statement(StatementType::MATH, OP.value()->value);
        ms->statements.emplace_back(LHS);
        ms->statements.emplace_back(RHS.value());

        return ms;
    }

    std::optional<Statement*> Parser::expect_value_expression(bool skipBin, bool skipLog) {

        std::optional<Statement*> cs;

        // variable definition
        if ((cs = expect_variable_definition()).has_value()) {
            return cs.value();
        }


        // get alloca
        if ((cs = expect_get_alloca()).has_value()) { 
            return cs.value();
        }


        // type cast
        if ((cs = expect_type_cast()).has_value()) { 
            return cs.value();
        }


        // binary expression
        if (!skipBin && (cs = expect_binary_expression()).has_value()) {
            return cs.value();
        }


        if (!skipLog && (cs = expect_logic_expression()).has_value()) {
            return cs.value();
        }

        // variable assignment
        if (!skipLog && (cs = expect_variable_assignment()).has_value()) {
            return cs.value();
        }


        // array
        if ((cs = expect_array_call()).has_value()) {
            return cs.value();
        }

        // function call
        if ((cs = expect_function_call()).has_value()) { 
            return cs.value();
        }

        // variable
        if ((cs = expect_variable_call()).has_value()) {
            return cs.value();
        }


        // array
        if ((cs = expect_array()).has_value()) {
            return cs.value();
        }

        // string
        if ((cs = expect_string()).has_value()) {
            return cs.value();
        }

        // primitives
        std::optional<tokenizer::Token*> ct;
        if ((ct = expect_long_int()).has_value()) {
            return new Statement(StatementType::LONG_LITERAL, ct.value()->value);
        }
        if ((ct = expect_char()).has_value()) {
            return new Statement(StatementType::CHAR_LITERAL, ct.value()->value);
        }
        if ((ct = expect_double()).has_value()) {
            return new Statement(StatementType::DOUBLE_LITERAL, ct.value()->value);
        }
        if ((ct = expect_integer()).has_value()) {
            return new Statement(StatementType::INTEGER_LITERAL, ct.value()->value);
        }
        if ((ct = expect_boolean()).has_value()) {
            return new Statement(StatementType::BOOLEAN_LITERAL, ct.value()->value);
        }

        return std::nullopt;

    }

    std::optional<Statement*> Parser::expect_expression(bool skip_semicolon) {
        // block
        if (expect_operator("{").has_value()) {
            Statement* stmt = new Statement(StatementType::CODE_BLOCK, "");
            while (true) {
                if (expect_operator("}").has_value()) break;

                std::optional<Statement*> expr = expect_expression();
                stmt->statements.emplace_back(expr.value());

                if (!expr.has_value()) { error(cToken, "Expected expression or '}'"); }
            }
            return stmt;
        }

        std::optional<Statement*> temp;

        // if block
        temp = expect_if();
        if (temp.has_value()) {
            return temp.value();
        }

        // for loop block
        temp = expect_for();
        if (temp.has_value()) {
            return temp.value();
        }

        // return
        if (expect_identifier("return").has_value()) {
            std::optional<Statement*> retVal = expect_value_expression(false, false);
            if (!retVal.has_value()) { 
                if (expect_operator(";").has_value()) {
                    // return without return value
                    Statement* retExpr = new Statement(StatementType::RETURN, "void");
                    return retExpr;
                } else {
                    error(cToken, "Expected return value");
                }
            }

            Statement* retExpr = new Statement(StatementType::RETURN, "value");
            retExpr->statements.emplace_back(retVal.value());

            if (!skip_semicolon && !expect_operator(";").has_value()) { error(cToken, "Expected ';' (r)"); }
            return retExpr;
        }

        // variable assignment
        temp = expect_variable_assignment();
        if (temp.has_value()) {
            if (!skip_semicolon && !expect_operator(";").has_value()) { error(cToken, "Expected ';' (va)"); }
            return temp.value();
        }

        // variable definition
        temp = expect_variable_definition();
        if (temp.has_value()) {
            if (!skip_semicolon && !expect_operator(";").has_value()) { error(cToken, "Expected ';' (vd)"); }
            return temp.value();
        }


        // function call
        temp = expect_function_call();
        if (temp.has_value()) {
            if (!skip_semicolon && !expect_operator(";").has_value()) { error(cToken, "Expected ';' (fc)"); }
            return temp.value();
        }

        return std::nullopt;
    }


    void Parser::error(tokenizer::Token* token, const std::string& message) {
        std::cout << token->lineNo << ":" << token->charNo << std::endl;
        throw std::runtime_error(message);
    }

}