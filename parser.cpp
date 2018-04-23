#include "parser.h"
#include <iostream>


Z3_context Z3SMTParser::mk_context_custom(Z3_config cfg) {
    Z3_context ctx;
    Z3_set_param_value(cfg, "MODEL", "true");
    ctx = Z3_mk_context(cfg);
    Z3_set_error_handler(ctx, throw_z3_error);
    return ctx;
}

Z3_context Z3SMTParser::mk_my_context() {
    Z3_config cfg = Z3_mk_config();
    Z3_context ctx;
    ctx = mk_context_custom(cfg);
    Z3_del_config(cfg);
    return ctx;
}

//make custom theory
Z3_theory Z3SMTParser::mk_replace_theory(Z3_context ctx) {
    ReplaceTheoryData *td = (ReplaceTheoryData *) malloc(sizeof(ReplaceTheoryData));
    Z3_theory th = Z3_mk_theory(ctx, "ReplaceAllAttachment", td);
    Z3_sort boolSort = Z3_mk_bool_sort(ctx);
   // Z3_sort intSort = Z3_mk_int_sort(ctx);
    Z3_symbol stringName = Z3_mk_string_symbol(ctx, "String");
    td -> String = Z3_theory_mk_sort(ctx, th, stringName);
    
    Z3_symbol regexSortName = Z3_mk_string_symbol(ctx, "Regex");
    td -> Regex = Z3_theory_mk_sort(ctx, th, regexSortName);
    
    //--------------------------
    //ReplaceAll := string x regex x string --> string
    Z3_symbol replaceAllName = Z3_mk_string_symbol(ctx, "ReplaceAll");
    Z3_sort replaceAllDomain[3];
    replaceAllDomain[0] = td->String;
    replaceAllDomain[1] = td->Regex;
    replaceAllDomain[2] = td->String;
    td->ReplaceAll = Z3_theory_mk_func_decl(ctx, th, replaceAllName, 3, replaceAllDomain, td->String);
    //---------------------------
    
    //---------------------------
    //Concat := string x string --> string
    Z3_symbol concatName = Z3_mk_string_symbol(ctx, "Concat");
    Z3_sort concatDomain[2];
    concatDomain[0] = td->String;
    concatDomain[1] = td->String;
    td->Concat = Z3_theory_mk_func_decl(ctx, th, concatName, 2, concatDomain, td->String);
    
    //===========================
    // Str2Reg := String --> Regex
    Z3_symbol str2RegName = Z3_mk_string_symbol(ctx, "Str2Reg");
    Z3_sort str2RegDomain[1];
    str2RegDomain[0] = td->String;
    td->Str2Reg = Z3_theory_mk_func_decl(ctx, th, str2RegName, 1, str2RegDomain, td->Regex);
    //---------------------------
    // RegexStar := Regex --> Regex
    Z3_symbol regexStarName = Z3_mk_string_symbol(ctx, "RegexStar");
    Z3_sort regexStarDomain[1];
    regexStarDomain[0] = td->Regex;
    td->RegexStar = Z3_theory_mk_func_decl(ctx, th, regexStarName, 1, regexStarDomain, td->Regex);
    //---------------------------
    // RegexIn := String x Regex --> Bool
    Z3_symbol regexInName = Z3_mk_string_symbol(ctx, "RegexIn");
    Z3_sort regexInDomain[2];
    regexInDomain[0] = td->String;
    regexInDomain[1] = td->Regex;
    td->RegexIn = Z3_theory_mk_func_decl(ctx, th, regexInName, 2, regexInDomain, boolSort);
    //---------------------------
    // RegexUnion := Regex x Regex --> Regex
    Z3_symbol regexUnionName = Z3_mk_string_symbol(ctx, "RegexUnion");
    Z3_sort regexUnionDomain[2];
    regexUnionDomain[0] = td->Regex;
    regexUnionDomain[1] = td->Regex;
    td->RegexUnion = Z3_theory_mk_func_decl(ctx, th, regexUnionName, 2, regexUnionDomain, td->Regex);
    //---------------------------
    // RegexConcat := Regex x Regex --> Regex
    Z3_symbol regexConcatName = Z3_mk_string_symbol(ctx, "RegexConcat");
    Z3_sort regexConcatDomain[2];
    regexConcatDomain[0] = td->Regex;
    regexConcatDomain[1] = td->Regex;
    td->RegexConcat = Z3_theory_mk_func_decl(ctx, th, regexConcatName, 2, regexConcatDomain, td->Regex);
    return th;
    
}

//according to regex functions, add some cases
std::string Z3SMTParser::getStdRegexStr(Z3_theory &t, Z3_ast &regex) {
    Z3_context ctx = Z3_theory_get_context(t);
    ReplaceTheoryData * td = (ReplaceTheoryData*) Z3_theory_get_ext_data(t);
    Z3_func_decl regexFuncDecl = Z3_get_app_decl(ctx, Z3_to_app(ctx, regex));
    if (regexFuncDecl == td -> Str2Reg) {
        Z3_ast regAst = Z3_get_app_arg(ctx, Z3_to_app(ctx, regex), 0);
        std::string conStr = std::string(Z3_ast_to_string(ctx, regAst));
        std::string regStr = "";
        if (conStr.length() >= 11 && conStr.substr(0, 11) == "__cOnStStR_") {
            std::string convertStr = Common::convertInputTrickyConstStr(conStr);
            regStr = Common::str2RegexStr(convertStr);
        }
        return regStr;
    } else if (regexFuncDecl == td -> RegexConcat) {
        Z3_ast reg1Ast = Z3_get_app_arg(ctx, Z3_to_app(ctx, regex), 0);
        Z3_ast reg2Ast = Z3_get_app_arg(ctx, Z3_to_app(ctx, regex), 1);
        std::string reg1Str = getStdRegexStr(t, reg1Ast);
        std::string reg2Str = getStdRegexStr(t, reg2Ast);
        return "(" + reg1Str + ")(" + reg2Str + ")";
    } else if (regexFuncDecl == td -> RegexUnion) {
        Z3_ast reg1Ast = Z3_get_app_arg(ctx, Z3_to_app(ctx, regex), 0);
        Z3_ast reg2Ast = Z3_get_app_arg(ctx, Z3_to_app(ctx, regex), 1);
        std::string reg1Str = getStdRegexStr(t, reg1Ast);
        std::string reg2Str = getStdRegexStr(t, reg2Ast);
        return  "(" + reg1Str + ")|(" + reg2Str + ")";
    } else if (regexFuncDecl == td -> RegexStar) {
        Z3_ast reg1Ast = Z3_get_app_arg(ctx, Z3_to_app(ctx, regex), 0);
        std::string reg1Str = getStdRegexStr(t, reg1Ast);
        return  "(" + reg1Str + ")*";
    } else {
        printf("> Error: unexpected regex operation.\n");
        __debugPrint(logFile, ">> Error: unexpected regex operation.\n");
        __debugPrint(logFile, "   * [regex] ");
        printZ3Node(t, regex);
        __debugPrint(logFile, "\n");
        exit(0);
    }
}

SingleTerm *Z3SMTParser::mkSingleTerm(TermType type, std::string vName) {
    return new SingleTerm(type, vName);
}

FunTerm *Z3SMTParser::mkFunTerm(FunName name, int domainSize, std::vector<Term*> &domain) {
    return new FunTerm(name, domainSize, domain);
}


void Z3SMTParser::mkConstraint(Z3_theory &t, Z3_ast &node, FunName fn, StdConstraintSet &stdConstrains) {
    Z3_context ctx = Z3_theory_get_context(t);
    ReplaceTheoryData *td = (ReplaceTheoryData*) Z3_theory_get_ext_data(t);
    Z3_app funcApp = Z3_to_app(ctx, node);
    std::stack<std::string> tempVar;
    std::stack<Z3_ast> funcNode;
    
    if (fn == RegexIn) {
        Z3_ast arg0 = Z3_get_app_arg(ctx, funcApp, 0);
        Z3_ast arg1 = Z3_get_app_arg(ctx, funcApp, 1);
        std::string stdRegex = getStdRegexStr(t, arg1);
        SingleTerm *term0;
        SingleTerm *term1;
        std::vector<Term*> constraintArgs;
        term1 = mkSingleTerm(ConstRegex, stdRegex);
        T_myZ3Type arg0Type = getNodeType(t, arg0);
        T_myZ3Type arg1Type = getNodeType(t, arg1);
        
        if (arg1Type == my_Z3_Regex_Var) {
            printf("> Error: please don't define a separate Regex variable (");
            printf("%s). Abort\n\n", Z3_ast_to_string(ctx, node));
            delete &stdConstrains;
            exit(0);
        }
        if (arg0Type == my_Z3_Func) {
            int tempVarNum = stdConstrains.addTempVarNum();
            std::string tempVarName = "__teMpVar_" + std::to_string(tempVarNum);
            tempVar.push(tempVarName);
            funcNode.push(arg0);
            term0 = mkSingleTerm(StrVar, tempVarName);
        }
        else if (arg0Type == my_Z3_Str_Var) {
            std::string arg0Str = std::string(Z3_ast_to_string(ctx, arg0));
            if (arg0Str.size() >= 11 && arg0Str.substr(0, 11) == "__cOnStStR_") {
                int tempVarNum = stdConstrains.addTempVarNum();
                std::string tempVarName = "__teMpVar_" + std::to_string(tempVarNum);
                tempVar.push(tempVarName);
                funcNode.push(arg0);
                term0 = mkSingleTerm(StrVar, tempVarName);
            }
            else {
                term0 = mkSingleTerm(StrVar, arg0Str);
            }
        }
        constraintArgs.push_back(term0);
        constraintArgs.push_back(term1);
        stdConstrains.addConstraint(mkFunTerm(RegexIn, 2, constraintArgs));
    }
    else if (fn == Equal) {
        //arg1 is a function or a const
        Z3_ast arg0 = Z3_get_app_arg(ctx, funcApp, 0);
        Z3_ast arg1 = Z3_get_app_arg(ctx, funcApp, 1);
        T_myZ3Type arg0Type = getNodeType(t, arg0);
        T_myZ3Type arg1Type = getNodeType(t, arg1);
        
        //it can not be a function in the left of equal sign
        if (arg0Type == my_Z3_Func && arg1Type == my_Z3_Func) {
            int tempVarNum = stdConstrains.addTempVarNum();
            std::string tempVarName = "__teMpVar_" + std::to_string(tempVarNum);
            tempVar.push(tempVarName);
            funcNode.push(arg1);
            tempVar.push(tempVarName);
            funcNode.push(arg0);
        }
        if (arg0Type == my_Z3_Func && arg1Type == my_Z3_Str_Var) {
            std::string arg1Str = std::string(Z3_ast_to_string(ctx, arg1));
            if (arg1Str.size() >= 11 && arg1Str.substr(0, 11) == "__cOnStStR_") {
                int tempVarNum = stdConstrains.addTempVarNum();
                std::string tempVarName = "__teMpVar_" + std::to_string(tempVarNum);
                tempVar.push(tempVarName);
                funcNode.push(arg1);
                tempVar.push(tempVarName);
                funcNode.push(arg0);
            }
            else {
                tempVar.push(arg1Str);
                funcNode.push(arg0);
            }
        }
        
        if (arg0Type == my_Z3_Str_Var && arg1Type == my_Z3_Func) {
            std::string arg0Str = std::string(Z3_ast_to_string(ctx, arg0));
            if (arg0Str.size() >= 11 && arg0Str.substr(0, 11) == "__cOnStStR_") {
                int tempVarNum = stdConstrains.addTempVarNum();
                std::string tempVarName = "__teMpVar_" + std::to_string(tempVarNum);
                tempVar.push(tempVarName);
                funcNode.push(arg0);
                tempVar.push(tempVarName);
                funcNode.push(arg1);
            }
            else {
                tempVar.push(arg0Str);
                funcNode.push(arg1);
            }
        }
        
        if (arg0Type == my_Z3_Str_Var && arg1Type == my_Z3_Str_Var) {
            std::string arg0Str = std::string(Z3_ast_to_string(ctx, arg0));
            std::string arg1Str = std::string(Z3_ast_to_string(ctx, arg1));
            if (arg0Str.size() >= 11 && arg0Str.substr(0, 11) == "__cOnStStR_") {
                if (arg1Str.size() >= 11 && arg1Str.substr(0, 11) == "__cOnStStR_") {
                    int tempVarNum = stdConstrains.addTempVarNum();
                    std::string tempVarName = "__teMpVar_" + std::to_string(tempVarNum);
                    tempVar.push(tempVarName);
                    funcNode.push(arg0);
                    tempVar.push(tempVarName);
                    funcNode.push(arg1);
                } else {
                    std::vector<Term*> constraintArgs;
                    SingleTerm* term0 = mkSingleTerm(StrVar, arg1Str);
                    constraintArgs.push_back(term0);
                    std::string constStr = std::string(Z3_ast_to_string(ctx, arg0));
                    SingleTerm* term1 = mkSingleTerm(ConstStr, Common::convertInputTrickyConstStr(constStr));
                    constraintArgs.push_back(term1);
                    stdConstrains.addConstraint(mkFunTerm(Equal, 2, constraintArgs));
                    return;
                }
            } else {
                if (arg1Str.size() >= 11 && arg1Str.substr(0, 11) == "__cOnStStR_") {
                    std::vector<Term*> constraintArgs;
                    SingleTerm* term0 = mkSingleTerm(StrVar, arg0Str);
                    constraintArgs.push_back(term0);
                    std::string constStr = std::string(Z3_ast_to_string(ctx, arg1));
                    SingleTerm* term1 = mkSingleTerm(ConstStr, Common::convertInputTrickyConstStr(constStr));
                    constraintArgs.push_back(term1);
                    stdConstrains.addConstraint(mkFunTerm(Equal, 2, constraintArgs));
                    return;
                } else {
                    std::vector<Term*> constraintArgs;
                    SingleTerm* term0 = mkSingleTerm(StrVar, arg0Str);
                    constraintArgs.push_back(term0);
                    SingleTerm* term1 = mkSingleTerm(StrVar, arg1Str);
                    constraintArgs.push_back(term1);
                    stdConstrains.addConstraint(mkFunTerm(Equal, 2, constraintArgs));
                    return;
                }
            }
        }
    }
    
    while (!tempVar.empty()) {
        std::string tempVarName = tempVar.top();
        Z3_ast tempCosNode = funcNode.top();
        tempVar.pop();
        funcNode.pop();
        T_myZ3Type nodeType = getNodeType(t, tempCosNode);
        std::vector<Term*> constraintArgs;
        constraintArgs.push_back(mkSingleTerm(StrVar, tempVarName));
        if (nodeType == my_Z3_Func) {
            Z3_app funcApp = Z3_to_app(ctx, tempCosNode);
            Z3_func_decl funcd = Z3_get_app_decl(ctx, funcApp);
            if (funcd == td -> ReplaceAll) {
                std::vector<Term*> funcArgs;
                Z3_ast firstPara = Z3_get_app_arg(ctx, funcApp, 0);
                Z3_ast secondPara = Z3_get_app_arg(ctx, funcApp, 1);
                Z3_ast thirdPara = Z3_get_app_arg(ctx, funcApp, 2);
                T_myZ3Type firstType = getNodeType(t, firstPara);
                T_myZ3Type thirdType = getNodeType(t, thirdPara);
                Term *term0, *term1, *term2;
                std::string patternRegex = getStdRegexStr(t, secondPara);
                term1 = mkSingleTerm(ConstRegex, patternRegex);
                std::string firstStr = std::string(Z3_ast_to_string(ctx, firstPara));
                std::string thirdStr = std::string(Z3_ast_to_string(ctx, thirdPara));
                if ((firstStr.size() >= 11 && firstStr.substr(0, 11) == "__cOnStStR_") || firstType == my_Z3_Func) {
                    int tempVarNum = stdConstrains.addTempVarNum();
                    std::string tempVarName = "__teMpVar_" + std::to_string(tempVarNum);
                    tempVar.push(tempVarName);
                    funcNode.push(firstPara);
                    term0 = mkSingleTerm(StrVar, tempVarName);
                } else {
                    term0 = mkSingleTerm(StrVar, firstStr);
                }
                if ((thirdStr.size() >= 11 && thirdStr.substr(0, 11) == "__cOnStStR_") || thirdType == my_Z3_Func) {
                    int tempVarNum = stdConstrains.addTempVarNum();
                    std::string tempVarName = "__teMpVar_" + std::to_string(tempVarNum);
                    tempVar.push(tempVarName);
                    funcNode.push(thirdPara);
                    term2 = mkSingleTerm(StrVar, tempVarName);
                } else {
                    term2 = mkSingleTerm(StrVar, std::string(Z3_ast_to_string(ctx, thirdPara)));
                }
                funcArgs.push_back(term0);
                funcArgs.push_back(term1);
                funcArgs.push_back(term2);
                constraintArgs.push_back(mkFunTerm(ReplaceAll, 3, funcArgs));
                stdConstrains.addConstraint(mkFunTerm(Equal, 2, constraintArgs));
            }
            else if (funcd == td -> Concat){
                std::vector<Term*> funcArgs;
                Z3_ast firstPara = Z3_get_app_arg(ctx, funcApp, 0);
                Z3_ast secondPara = Z3_get_app_arg(ctx, funcApp, 1);
                T_myZ3Type firstType = getNodeType(t, firstPara);
                T_myZ3Type secondType = getNodeType(t, secondPara);
                Term *term0, *term1;
                std::string firstStr = std::string(Z3_ast_to_string(ctx, firstPara));
                std::string secondStr = std::string(Z3_ast_to_string(ctx, secondPara));
                if ((firstStr.size() >= 11 && firstStr.substr(0, 11) == "__cOnStStR_") || firstType == my_Z3_Func) {
                    int tempVarNum = stdConstrains.addTempVarNum();
                    std::string tempVarName = "__teMpVar_" + std::to_string(tempVarNum);
                    tempVar.push(tempVarName);
                    funcNode.push(firstPara);
                    term0 = mkSingleTerm(StrVar, tempVarName);
                   // model.addStrVar(tempVarName);
                } else {
                    term0 = mkSingleTerm(StrVar, std::string(Z3_ast_to_string(ctx, firstPara)));
                }
                if ((secondStr.size() >= 11 && secondStr.substr(0, 11) == "__cOnStStR_") || secondType == my_Z3_Func) {
                    int tempVarNum = stdConstrains.addTempVarNum();
                    std::string tempVarName = "__teMpVar_" + std::to_string(tempVarNum);
                    tempVar.push(tempVarName);
                    funcNode.push(secondPara);
                    term1 = mkSingleTerm(StrVar, tempVarName);
                   // model.addStrVar(tempVarName);
                } else {
                    term1 = mkSingleTerm(StrVar, std::string(Z3_ast_to_string(ctx, secondPara)));
                }
                funcArgs.push_back(term0);
                funcArgs.push_back(term1);
                constraintArgs.push_back(mkFunTerm(Concat, 2, funcArgs));
                stdConstrains.addConstraint(mkFunTerm(Equal, 2, constraintArgs));
            }
        } else {
            Term *term1;
            std::string constStr = std::string(Z3_ast_to_string(ctx, tempCosNode));
            term1 = mkSingleTerm(ConstStr, Common::convertInputTrickyConstStr(constStr));
            constraintArgs.push_back(term1);
            stdConstrains.addConstraint(mkFunTerm(Equal, 2, constraintArgs));
        }
    }
}

void Z3SMTParser::checkInputVar(Z3_theory &t, Z3_ast &node, StdConstraintSet& stdConstrains) {
    Z3_context ctx = Z3_theory_get_context(t);
    T_myZ3Type nodeType = getNodeType(t, node);
    
    if (nodeType == my_Z3_Str_Var) {
        std::string vName = std::string(Z3_ast_to_string(ctx, node));
        if (vName.length() >= 11 && vName.substr(0, 11) == "__cOnStStR_") {
            return;
        }
        if (vName.length() >= 3 && vName.substr(0, 3) == "$$_") {
            printf("> Error: please don't define a variable with a prefix \"$$_\" (");
            printf("%s). Abort\n\n", Z3_ast_to_string(ctx, node));
            delete &stdConstrains;
            exit(0);
        }
        if (vName.length() >= 10 && vName.substr(0, 10) == "__teMpVar_") {
            printf("> Error: please don't define a variable with a prefix \"__teMpVar_\" (");
            printf("%s). Abort\n\n", Z3_ast_to_string(ctx, node));
            delete &stdConstrains;
            exit(0);
        }
        //model.addStrVar(vName);
    } else if (getNodeType(t, node) == my_Z3_Func) {
        Z3_app func_app = Z3_to_app(ctx, node);
        int argCount = Z3_get_app_num_args(ctx, func_app);
        
        ReplaceTheoryData * td = (ReplaceTheoryData*) Z3_theory_get_ext_data(t);
        Z3_func_decl funcd = Z3_get_app_decl(ctx, Z3_to_app(ctx, node));
        Z3_decl_kind dk = Z3_get_decl_kind(ctx, funcd);
        if(funcd == td->RegexIn) {
            mkConstraint(t,node, RegexIn, stdConstrains);
        }
        if(dk == Z3_OP_EQ) {
            mkConstraint(t,node, Equal, stdConstrains);
        }
        for (int i = 0; i < argCount; i++) {
            Z3_ast argAst = Z3_get_app_arg(ctx, func_app, i);
            checkInputVar(t, argAst, stdConstrains);
        }
    } else if (nodeType == my_Z3_Regex_Var) {
        printf("> Error: please don't define a separate Regex variable (");
        printf("%s). Abort\n\n", Z3_ast_to_string(ctx, node));
        delete &stdConstrains;
        exit(0);
    } else {
        std::string vName = std::string(Z3_ast_to_string(ctx, node));
    }
}

void Z3SMTParser::parse(const std::string& fileName, StdConstraintSet& stdConstrains) {
    if (fileName == "") {
        printf("No input file is provided. \n");
        return;
    }
    Z3_context ctx = mk_my_context();
    Z3_theory th = mk_replace_theory(ctx);
    ctx = Z3_theory_get_context(th);
    Common::setAlphabet();
    Z3_ast fs = Z3_parse_smtlib2_file(ctx, fileName.c_str(), 0, 0, 0, 0, 0, 0);
    
    checkInputVar(th, fs, stdConstrains);

}
