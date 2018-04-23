#ifndef solver_h
#define solver_h

#include "common.h"
#include "parser.h"
#include "model.h"
#include "automata.h"
#include <unordered_map>
#include <stack>

class Solver {
private:
    static void printSolution(std::unordered_map<std::string,std::string> *solution);
    static std::string replaceAll(std::string &originalStr, std::string &pattern, std::string &replacement);
    static void fullSolution(std::unordered_map<std::string, std::string> *solution, StdConstraintSet &stdConstraints);
    static NFA *getStrPatternNFA(std::string &pattern);
    static NFA *getRegPatternNFA(std::string &pattern);
    static DFA *getNewZ(DFA *xDFA, DFA *zDFA, DFAStateSet &xStates);
    static DFA *getNewYConcat(DFA *xDFA, DFA *zDFA);
    static NFA *getNewY(DFA *xDFA, DFA *yDFA, std::unordered_map<DFAState*,DFAState*> &stateMap, NFA *patternNFA, std::unordered_set<char> &single);
    static bool solveReplaceAll(FunTerm *funTerm, int i, std::stack<Model*> &models, std::stack<int> &constraintIndex, Model *curModel);
    static bool solveConcat(FunTerm *funTerm, int i, std::stack<Model*> &models, std::stack<int> &constraintIndex, Model *curModel);
public:
    static bool topoSort(std::vector<FunTerm*> &constraints, std::unordered_set<std::string> &exceptVars);
    static void check(StdConstraintSet &stdConstraints);
};

#endif
