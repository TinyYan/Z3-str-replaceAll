
#include "model.h"

std::unordered_map<FA*, int> Model::allFARef;

Model::Model(const Model& model) {
    strVar = model.strVar;
    for (std::unordered_map<std::string, FA*>::iterator it = strVar.begin(); it != strVar.end(); ++it) {
        ++allFARef[it -> second];
    }
}

void Model::addStrVar(const std::string &name) {
    strVar[name] = NULL;
}

void Model::cleanFA(FA *fa) {
    --allFARef[fa];
    if (allFARef[fa] == 0) {
        allFARef.erase(fa);
        fa -> clear();
        delete fa;
    }
}
//fa 的回收不在函数里
void Model::addStrCons(const std::string &name, FA *fa) {
    if (strVar.count(name) == 0 || strVar[name] == NULL) {
        strVar[name] = fa;
        allFARef[fa] = 1;
    }
    else {
        FA *oldFA = strVar[name];
        DFA *oldDFA = NULL;
        DFA *dfa = NULL;
        if (!(oldFA -> isDeterministic())) {
            oldDFA = dynamic_cast<NFA*>(oldFA) -> determine();
        }
        else {
            oldDFA = dynamic_cast<DFA*>(oldFA);
        }
        if (!(fa -> isDeterministic())) {
            dfa = dynamic_cast<NFA*>(fa) -> determine();
        }
        else {
            dfa = dynamic_cast<DFA*>(fa);
        }
        DFA *newDFA = DFA::intersection(*oldDFA, *dfa);
        strVar[name] = newDFA;
        if (!(oldFA -> isDeterministic())) {
            oldDFA -> clear();
            delete oldDFA;
        }
        if (!(fa -> isDeterministic())) {
            dfa -> clear();
            delete dfa;
        }
        cleanFA(oldFA);
        allFARef[newDFA] = 1;
    }
}

void Model::replaceStrCons(const std::string &name, FA *fa) {
    if (strVar[name] == NULL) {
        strVar[name] = fa;
    }
    else {
        FA *oldFA = strVar[name];
        cleanFA(oldFA);
        strVar[name] = fa;
    }
    if (allFARef.count(fa) == 0) {
        allFARef[fa] = 1;
    }
    else {
        ++allFARef[fa];
    }
}

//获得字符串变量的FA，不存在返回NULL
FA *Model::getStrCons(const std::string &name) {
    if (strVar.count(name) == 0) {
        return NULL;
    }
    return strVar[name];
}

//获得字符串的DFA，不存在返回NULL
DFA *Model::getStrDFA(const std::string &name) {
    if (strVar.count(name) == 0) {
        return NULL;
    }
    FA *strFA = strVar[name];
    if (strFA -> isDeterministic()) {
        return dynamic_cast<DFA*>(strFA);
    }
    DFA *strDFA = (dynamic_cast<NFA*>(strFA)) -> determine();
    strVar[name] = strDFA;
    allFARef[strDFA] = 1;
    cleanFA(strFA);
    return dynamic_cast<DFA*>(strVar[name]);
}

TermType Model::getVarType(const std::string &name) {
    if (strVar.count(name) == 1) {
        return StrVar;
    }
    return Unknown;
}

//if it does not have solution, the function return NULL.
std::unordered_map<std::string, std::string> *Model::getSolution() {
    std::unordered_map<std::string, std::string> *solution = new std::unordered_map<std::string, std::string>();
    for (std::unordered_map<std::string, FA*>::iterator it = strVar.begin(); it != strVar.end(); ++it) {
        if (it -> second == NULL) {
            (*solution)[it -> first] = "";
            continue;
        }
        bool flag = true;
        (*solution)[it -> first] = (it -> second) -> getWord(flag);
        if (!flag) {
            delete solution;
            return NULL;
        }
    }
    return solution;
}

std::unordered_map<std::string, std::string> *Model::getSolution(std::unordered_set<std::string> &exceptVars) {
    std::unordered_map<std::string, std::string> *solution = new std::unordered_map<std::string, std::string>();
    for (std::unordered_map<std::string, FA*>::iterator it = strVar.begin(); it != strVar.end(); ++it) {
        //不需要求解的变量
        if (exceptVars.count(it -> first) != 0) {
            continue;
        }
        if (it -> second == NULL) {
            (*solution)[it -> first] = "";
            continue;
        }
        bool flag = true;
        (*solution)[it -> first] = (it -> second) -> getWord(flag);
        if (!flag) {
            delete solution;
            return NULL;
        }
    }
    return solution;
}

Model::~Model() {
    for (std::unordered_map<std::string, FA*>::iterator it = strVar.begin(); it != strVar.end(); ++it) {
        --allFARef[it -> second];
        if (allFARef[it -> second] == 0) {
            allFARef.erase(it -> second);
            (it -> second) -> clear();
            delete it -> second;
        }
    }
}

void Model::clear() {
    //if all objects of Model are destroied correctly, the size of allFARef should be 0.
    for (std::unordered_map<FA*, int>::iterator it = allFARef.begin(); it != allFARef.end(); ++it) {
        (it -> first) -> clear();
        delete it -> first;
    }
}
