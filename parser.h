#ifndef parser_h
#define parser_h

#include "z3.h"
#include "common.h"
#include <string>
#include <stack>
#include <vector>
#include <iostream>

typedef enum {
    ConstStr,
    ConstBool,
    ConstRegex,
    Func,
    Num,
    NumVar,
    BoolVar,
    StrVar,
    RegexVar,
    Quantifier,
    Unknown
} TermType;

typedef enum {
    ReplaceAll,
    Concat,
    Equal,
    RegexIn,
    Str2Reg,
    RegexStar,
    RegexUnion,
    RegexConcat
} FunName;

class Term {
public:
    virtual TermType getType() = 0;
    virtual std::string toString() = 0;
    virtual ~Term() {}
};

class SingleTerm : public Term {
private:
    TermType type;
    std::string name;
public:
    SingleTerm():type(Unknown),name("") {}
    SingleTerm(TermType t, std::string& n):type(t),name(n) {}
    TermType getType() {
        return type;
    }
    void setType(TermType t) {
        type = t;
    }
    std::string getName() {
        return name;
    }
    void setName(std::string& n) {
        name = n;
    }
    std::string toString() {
        return name;
    }
    bool equal(SingleTerm const &term1) {
        if (this -> type == term1.type && this -> name == term1.name) {
            return true;
        }
        return false;
    }
    ~SingleTerm() {}
};

class FunTerm : public Term {
private:
    FunName name;
    int domainSize;
    std::vector<Term*> args;
public:
    FunTerm(const FunTerm& t) {
        name = t.name;
        domainSize = t.domainSize;
        args = t.args;
    }
    
    FunTerm(FunName n, int s, std::vector<Term*>& a) {
        name = n;
        domainSize = s;
        if (s != a.size()) {
            printf("The domain does not match.");
            exit(1);
        }
        for (int i = 0; i < s; ++i){
            args.push_back(a[i]);
        }
    }
    
    TermType getType() {
        return Func;
    }
    
    FunName getName() {
        return name;
    }
    
    int getDomainSize() {
        return domainSize;
    }
    
    Term* getArg(int i) {
        if (i >= domainSize || i < 0) {
            return NULL;
        }
        return args[i];
    }
    
    std::string toString() {
        std::string termStr = "";
        switch (name) {
            case ReplaceAll:
                termStr = termStr + "ReplaceAll (";
                break;
            case Concat:
                termStr = termStr + "Concat (";
                break;
            case Equal:
                termStr = termStr + "= (";
                break;
            case RegexIn:
                termStr = termStr + "RegexIn (";
                break;
            case Str2Reg:
                termStr = termStr + "Str2Reg (";
                break;
            case RegexStar:
                termStr = termStr + "RegexStar (";
                break;
            case RegexUnion:
                termStr = termStr + "RegexUnion (";
                break;
            case RegexConcat:
                termStr = termStr + "RegexConcat (";
                break;
            default:
                break;
        }
        for (int i = 0;  i < domainSize - 1; ++i) {
            termStr = termStr + args[i] -> toString() + ", ";
        }
        termStr = termStr + args[domainSize - 1] -> toString() + ")";
        return termStr;
    }
    
    bool equal(FunTerm *term1) {
        if (name != term1 -> name || domainSize != term1 -> domainSize) {
            return false;
        }
        for (int i = 0; i < domainSize; ++i) {
            if (args[i] -> toString() != (term1 -> args[i]) -> toString() || args[i] -> getType() != (term1 -> args[i]) -> getType()) {
                return false;
            }
            //if the term is function type
            if (args[i] -> getType() == Func) {
                if (!dynamic_cast<FunTerm*>(args[i])->equal((dynamic_cast<FunTerm*>(term1 -> args[i])))) {
                    return false;
                }
            }
        }
        return true;
    }
    
    bool isConstrain() {
        if (name == Equal || name == RegexIn) {
            return true;
        }
        return false;
    }
    
    ~FunTerm() {
        for (size_t i = 0; i < args.size(); ++i) {
            delete args[i];
        }
    }
};


/* The standard expression of the parsing result. If you want to do some concurrent things,
you can change the structure of this class.*/
class StdConstraintSet {
private:
    int tempVarNum;
    std::vector<FunTerm*> constraints;
    
public:
    StdConstraintSet():tempVarNum(0) {}
    
    void addConstraint(FunTerm *c) {
        if (c -> getName() == RegexIn || c -> getName() == Equal) {
            constraints.push_back(c);
        } else {
            fprintf(stdout, "> Error: %s is not a constraint.\n", (c -> toString()).c_str());
            fflush(stdout);
        }
    }
    int getSize() {
        return static_cast<int>(constraints.size());
    }
    FunTerm* getConstraint(int i) {
        if (i >= constraints.size() || i < 0) {
            return NULL;
        }
        return constraints[i];
    }
    std::vector<FunTerm*> &getConstraints() {
        return constraints;
    }
    int getTempVarNum() {
        return tempVarNum;
    }
    int addTempVarNum() {
        return tempVarNum++;
    }
    void print() {
        std::cout << constraints.size() << std::endl;
        for (size_t i = 0; i < constraints.size(); ++i) {
            std::cout << constraints[i] -> toString() << std::endl;
        }
    }
    ~StdConstraintSet() {
        for (size_t i = 0; i < constraints.size(); ++i) {
            delete constraints[i];
        }
    }
};

//add new functions here
class Z3SMTParser {
private:
    typedef struct _replaceTheoryData
    {
        Z3_sort String;
        Z3_sort Regex;
        
        Z3_func_decl ReplaceAll;
        Z3_func_decl Concat;
        
        Z3_func_decl Str2Reg;
        Z3_func_decl RegexStar;
        Z3_func_decl RegexIn;
        Z3_func_decl RegexUnion;
        Z3_func_decl RegexConcat;
        /*
         Z3_func_decl Unroll;
         Z3_func_decl RegexPlus;
         Z3_func_decl RegexCharRange;
         */
    } ReplaceTheoryData;
    
    typedef enum
    {
        my_Z3_ConstStr,    // 0
        my_Z3_ConstBool,   // 1
        my_Z3_Func,        // 2
        my_Z3_Num,         // 3
        my_Z3_Var,         // 4, boolean, bv, int varables
        my_Z3_Str_Var,     // 5
        my_Z3_Regex_Var,   // 8
        my_Z3_Quantifier,  // 7
        my_Z3_Unknown      // 9
    } T_myZ3Type;
    
public:
    
    static Z3_context mk_context_custom(Z3_config cfg);
    
    static void throw_z3_error(Z3_context ctx, Z3_error_code c) {}
    
    static Z3_context mk_my_context();
    
    static Z3_theory mk_replace_theory(Z3_context ctx);
    
    static std::string getStdRegexStr(Z3_theory &t, Z3_ast &regex);
    
    static SingleTerm *mkSingleTerm(TermType type, std::string vName);
    
    static FunTerm *mkFunTerm(FunName name, int domainSize, std::vector<Term*> &domain);
    
    static void checkInputVar(Z3_theory &t, Z3_ast &node, StdConstraintSet& stdConstrains);
    
    static void mkConstraint(Z3_theory &t, Z3_ast &node, FunName fn, StdConstraintSet &stdConstrains);
    
    static void parse(const std::string& fileName, StdConstraintSet& stdConstrains);
    
    static T_myZ3Type getNodeType(Z3_theory t, Z3_ast n) {
        Z3_context ctx = Z3_theory_get_context(t);
        ReplaceTheoryData * td = (ReplaceTheoryData*) Z3_theory_get_ext_data(t);
        Z3_ast_kind z3Kind = Z3_get_ast_kind(ctx, n);
        
        switch (z3Kind) {
            case Z3_NUMERAL_AST: {
                return my_Z3_Num;
                break;
            }
                
            case Z3_APP_AST: {
                Z3_sort s = Z3_get_sort(ctx, n);
                Z3_sort_kind sk = Z3_get_sort_kind(ctx, s);
                Z3_func_decl d = Z3_get_app_decl(ctx, Z3_to_app(ctx, n));
                if (Z3_theory_is_value(t, n)) {
                    if (sk == Z3_BOOL_SORT) {
                        if (d == td->RegexIn) {
                            return my_Z3_Func;
                        } else {
                            return my_Z3_ConstBool;
                        }
                    } else if (sk == Z3_UNKNOWN_SORT) {
                        if (s == td->String) {
                            if (d == td->ReplaceAll || d == td -> Concat) {
                                return my_Z3_Func;
                            } else {
                                return my_Z3_ConstStr;
                            }
                        }
                        if (s == td->Regex) {
                            if (d == td->RegexConcat || d == td->RegexStar || d == td->RegexUnion || d == td->Str2Reg)
                                return my_Z3_Func;
                            else
                                return my_Z3_Regex_Var;
                        }
                    } else if (sk == Z3_ARRAY_SORT) {
                        std::string vName = std::string(Z3_ast_to_string(ctx, n));
                        __debugPrint(logFile, "> [getNodeType] my_Z3_Func: %s\n\n", vName.c_str());
                        return my_Z3_Func;
                    }
                } else {
                    //Z3 native functions fall into this category
                    Z3_decl_kind dk = Z3_get_decl_kind(ctx, d);
                    unsigned domainSize = Z3_get_domain_size(ctx, d);
                    if (dk != Z3_OP_UNINTERPRETED) {
                        // built-in function
                        return my_Z3_Func;
                    } else {
                        if (domainSize != 0) {
                            // "real" UNINTERPRETED function declared in the input
                            return my_Z3_Func;
                        } else {
                            if (s == td->String) {
                                return my_Z3_Str_Var;
                            } else if (s == td->Regex) {
                                return my_Z3_Regex_Var;
                            } else {
                                return my_Z3_Var;
                            }
                        }
                    }
                }
                break;
            }
                
            case Z3_VAR_AST: {
                return my_Z3_Var;
                break;
            }
                
            default: {
                break;
            }
        }
        return my_Z3_Unknown;
    }
    
};


#endif
