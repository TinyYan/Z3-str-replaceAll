#include "solver.h"


void Solver::printSolution(std::unordered_map<std::string, std::string> *solution) {
    if (solution == NULL) {
        return;
    }
    printf("**********************************\n");
    printf(">> SAT\n");
    for (std::unordered_map<std::string,std::string>::iterator it = solution -> begin(); it != solution -> end(); ++it) {
        if ((it -> first).size() >= 11 && (it -> first).substr(0, 10) == "__teMpVar_") {
            continue;
        }
        printf("%s",(it -> first).c_str());
        printf(" -> ");
        printf("\"%s\"",(it -> second).c_str());
        printf("\n");
    }
}

std::string Solver::replaceAll(std::string &originalStr, std::string &pattern, std::string &replacement) {
    DFA patternDFA(pattern);
    std::string res = "";
    int start = 0;
    while (start < static_cast<int>(originalStr.size())) {
        bool flag = false;
        DFAState* startState = patternDFA.getStartState();
        int end = start;
        int match = start;
        while (end < static_cast<int>(originalStr.size())) {
            DFAState* nextState = startState -> getState(originalStr[end]);
            if (nextState == NULL) {
                break;
            }
            else {
                startState = nextState;
                if (startState -> isEnd()) {
                    flag = true;
                    match = end;
                }
            }
            ++end;
        }
        start = match + 1;
        if (flag) {
            res = res + replacement;
        }
        else {
            res = res + originalStr.substr(match,1);
        }
    }
    patternDFA.clear();
    return res;
}

void Solver::fullSolution(std::unordered_map<std::string, std::string> *solution, StdConstraintSet &stdConstraints) {
    int constraintSize = stdConstraints.getSize();
    if (constraintSize == 0) {
        return;
    }
    for (int i = constraintSize - 1; i >= 0; --i) {
        FunTerm *constraint = stdConstraints.getConstraint(i);
        if (constraint -> getName() == Equal) {
            Term *arg1 = constraint -> getArg(1);
            TermType arg1Type = arg1 -> getType();
            if (arg1Type == Func) {
                FunTerm *arg1FunTerm = dynamic_cast<FunTerm*>(arg1);
                FunName funName = arg1FunTerm -> getName();
                std::string arg0Str = constraint -> getArg(0) -> toString();
                if (funName == ReplaceAll) {
                    std::string funArg0Str = arg1FunTerm -> getArg(0) -> toString();
                    std::string funArg1Str = arg1FunTerm -> getArg(1) -> toString();
                    std::string funArg2Str = arg1FunTerm -> getArg(2) -> toString();
                    (*solution)[arg0Str] = replaceAll((*solution)[funArg0Str], funArg1Str, (*solution)[funArg2Str]);
                }
                else if (funName == Concat) {
                    std::string funArg0Str = arg1FunTerm -> getArg(0) -> toString();
                    std::string funArg1Str = arg1FunTerm -> getArg(1) -> toString();
                    (*solution)[arg0Str] = (*solution)[funArg0Str] + (*solution)[funArg1Str];
                }
            }
            else if (arg1Type == StrVar) {
               std::string arg0Str = constraint -> getArg(0) -> toString();
               std::string arg1Str = constraint -> getArg(1) -> toString();
                (*solution)[arg0Str] = (*solution)[arg1Str];
            }
        }
    }
}

NFA* Solver::getStrPatternNFA(std::string &pattern) {
    //if the state is not final state, it does not have extend state
    std::unordered_map<int, NFAState*> stateMap;
    int patternSize = static_cast<int>(pattern.size());
    NFA* res = new NFA();
    NFAState* startState = res -> mkNewFinalState();
    NFAState* last = startState;
    res->setStartState(startState);
    stateMap[0] = startState;
    for (int k = 0; k < patternSize - 1; ++k) {
        NFAState* next = res -> mkNewFinalState();
        last->addTrans(pattern[k], next);
        last = next;
        stateMap[k + 1] = next;
    }
    NFAState* trapState = res -> mkNewState();
    last->addTrans(pattern[patternSize - 1], trapState);
    stateMap[patternSize] = trapState;
    //KMP
    std::vector<int> p(patternSize + 1, 0);
    int k = 0;
    for (size_t i = 2; i <= pattern.size(); ++i) {
        while (k > 0 && pattern[k] != pattern[i - 1]) {
            k = p[k];
        }
        if (pattern[k] == pattern[i - 1])
            k = k + 1;
        p[i] = k;
    }
    //according to the result of KMP, create the automata
    for (int j = 1; j < patternSize; ++j) {
        NFAState* prefix = stateMap[p[j]];
        NFATransMap& preStateTrans = prefix -> getTransMap();
        for (NFATransMap::iterator it = preStateTrans.begin(); it != preStateTrans.end(); ++it) {
            NFAStateSet& stateSet = it->second;
            if(it->first != pattern[j])
                stateMap[j]->addTrans(it->first, *(stateSet.begin()));
        }
    }
    std::vector<bool> verifyFlag(patternSize, true);
    k = p[patternSize];
    while (k != 0) {
        verifyFlag[patternSize - k] = false;
        k = p[k];
    }
    last = startState;
    if (patternSize > 1) {
        last = res -> mkNewState();
        for (int i = 0; i < patternSize; ++i) {
            if (verifyFlag[i]) {
                stateMap[i] -> addTrans(pattern[0], last);
            }
        }
        for (int j = 1; j < patternSize - 1; ++j) {
            NFAState *verifyState = res -> mkNewState();
            last -> addTrans(pattern[j], verifyState);
            last = verifyState;
        }
    }
    last -> addTrans(pattern[patternSize - 1], startState);
    return res;
}

int findInfo(std::vector<NFAState*> &states, InformationForReg* info) {
    for (size_t i = 0; i < states.size(); ++i) {
        if (*(reinterpret_cast<InformationForReg*>(states[i]->getInformation())) == *info)
            return i;
    }
    return -1;
}

void addNewTranForReg(char c, std::vector<NFAState*>& states, InformationForReg* info, NFA* res, int i) {
    int stateIndex = findInfo(states,info);
    if (stateIndex == -1) {
        NFAState* nextState = new AggregateStateForReg(info);
        res -> addState(nextState);
        states[i]->addTrans(c, nextState);
        states.push_back(nextState);
        if((info -> preStates).size() == 0)
            res -> addFinalState(nextState);
        //-----------------------------------debug
      /*  for(DFAState *s : info -> noFinalStates) {
            std::cout << s << ":";
        }
        std::cout << nextState << std::endl;
        std::cout << c << "-trans:" << states[i] << nextState << std::endl;*/
    }
    else {
        delete info;
        states[i]->addTrans(c, states[stateIndex]);
       // std::cout << c << "-trans:" << states[i] << states[stateIndex] << std::endl;
    }
    
}

bool containsFinal(DFAStateSet &states, DFAStateSet &finalStates) {
    for (DFAState *s : states) {
        if (finalStates.count(s) == 1) {
            return true;
        }
    }
    return false;
}

NFA *Solver::getRegPatternNFA(std::string &pattern) {
    //---------------------------------
    DFA *patternDFA = new DFA(pattern);
    DFAStateSet &patternFinal = patternDFA -> getFinalStates();
    DFAState *patternStart = patternDFA -> getStartState();
    
    //----------------------------------
    NFA *res = new NFA();
    std::vector<NFAState*> states;
    InformationForReg *startInfo = new InformationForReg;
    NFAState *startState = new AggregateStateForReg(startInfo);
    res -> setStartState(startState);
    res -> addFinalState(startState);
    states.push_back(startState);
    NFAState *trapState = res -> mkNewState();
    //-----------------------------------
    
    size_t i = 0;
    while (i != states.size()) {
        NFAState *curState = states[i];
        InformationForReg *curInfo = reinterpret_cast<InformationForReg*>(curState->getInformation());
        DFAStateSet &curPreStates = curInfo -> preStates;
        DFAStateSet &curNofnlStates = curInfo -> noFinalStates;
        std::unordered_map<char, std::unordered_set<DFAState *> > nofnlTrans;
        for (DFAStateSet::iterator it = curNofnlStates.begin(); it != curNofnlStates.end(); ++it) {
            DFATransMap &preTran = (*it) -> getTransMap();
            for (DFATransMap::iterator it1 = preTran.begin(); it1 != preTran.end(); ++it1) {
                 nofnlTrans[it1 -> first].insert(it1 -> second);
            }
        }
        if (curState -> isEnd()) {
            DFATransMap &pStartTrans = patternStart -> getTransMap();
            //verify state
            for (DFATransMap::iterator it = pStartTrans.begin(); it != pStartTrans.end(); ++it) {
                if (nofnlTrans.count(it -> first) != 0) {
                    if (containsFinal(nofnlTrans[it -> first],patternFinal)) {
                        continue;
                    }
                }
                //pattern接受单个字符，转searchleft
                if (patternFinal.count(it -> second) != 0) {
                    InformationForReg *nextInfo = new InformationForReg;
                    if (nofnlTrans.count(it -> first) != 0) {
                        nextInfo -> noFinalStates = nofnlTrans[it -> first];
                    }
                    (nextInfo -> noFinalStates).insert(it -> second);
                    addNewTranForReg(it -> first, states, nextInfo, res,i);
                }
                InformationForReg *nextInfo = new InformationForReg;
                (nextInfo -> preStates).insert(it -> second);
                if (nofnlTrans.count(it -> first) != 0) {
                    nextInfo -> noFinalStates = nofnlTrans[it -> first];
                }
                addNewTranForReg(it -> first, states, nextInfo, res,i);
                /*else {
                    InformationForReg *nextInfo = new InformationForReg;
                    (nextInfo -> preStates).insert(it -> second);
                    if (nofnlTrans.count(it -> first) != 0) {
                        nextInfo -> noFinalStates = nofnlTrans[it -> first];
                    }
                    addNewTranForReg(it -> first, states, nextInfo, res,i);
                }*/
            }
            //search state
            for (DFATransMap::iterator it = pStartTrans.begin(); it != pStartTrans.end(); ++it) {
                nofnlTrans[it -> first].insert(it -> second);
            }
            for (std::unordered_map<char, std::unordered_set<DFAState *> >::iterator it = nofnlTrans.begin(); it != nofnlTrans.end(); ++it) {
                if (!containsFinal(it -> second, patternFinal)) {
                    InformationForReg *nextInfo = new InformationForReg;
                    nextInfo -> noFinalStates = it -> second;
                    addNewTranForReg(it -> first, states, nextInfo, res,i);
                }
                else {
                    curState -> addTrans(it -> first, trapState);
                }
            }
        }
        else {
            std::unordered_map<char, std::unordered_set<DFAState *> > preStateTrans;
            for (DFAStateSet::iterator it = curPreStates.begin(); it != curPreStates.end(); ++it) {
                DFATransMap &preTran = (*it) -> getTransMap();
                for (DFATransMap::iterator it1 = preTran.begin(); it1 != preTran.end(); ++it1) {
                    preStateTrans[it1 -> first].insert(it1 -> second);
                }
            }
            for (std::unordered_map<char, std::unordered_set<DFAState *> >::iterator it = preStateTrans.begin(); it != preStateTrans.end(); ++it) {
                if (nofnlTrans.count(it -> first) != 0) {
                    if (containsFinal(nofnlTrans[it -> first], patternFinal)) {
                        continue;
                    }
                }
                if (containsFinal(it -> second, patternFinal)) {
                    InformationForReg *nextInfo = new InformationForReg;
                    if (nofnlTrans.count(it -> first) != 0) {
                        nextInfo -> noFinalStates = nofnlTrans[it -> first];
                    }
                    for (DFAState *s : it -> second) {
                        (nextInfo -> noFinalStates).insert(s);
                    }
                    addNewTranForReg(it -> first, states, nextInfo, res,i);
                }
                InformationForReg *nextInfo = new InformationForReg;
                if (nofnlTrans.count(it -> first) != 0) {
                    nextInfo -> noFinalStates = nofnlTrans[it -> first];
                }
                for (DFAState *s : it -> second) {
                    (nextInfo -> preStates).insert(s);
                }
                addNewTranForReg(it -> first, states, nextInfo, res,i);
            }
        }
        ++i;
    }
    return res;
}


DFA *Solver::getNewZ(DFA *xDFA, DFA *zDFA, DFAStateSet &xStates) {
    DFA *res = new DFA();
    InformationForStr *startInfo = new InformationForStr;
    for (DFAState *state : xStates) {
        (startInfo -> mapping)[state] = state;
    }
    startInfo -> state = zDFA -> getStartState();
    DFAState* startState = new AggregateStateForStr(startInfo);
    if ((startInfo -> state) -> isEnd()) {
        res -> addFinalState(startState);
    }
    res -> setStartState(startState);
    std::vector<DFAState*> states;
    states.push_back(startState);
    size_t i = 0;
    while (i != states.size()) {
        InformationForStr *curInfo = reinterpret_cast<InformationForStr*>(states[i]->getInformation());
        std::unordered_map<DFAState*, DFAState*> &m = curInfo -> mapping;
        DFAState *stateZ = curInfo -> state;
        DFATransMap &transMap = stateZ -> getTransMap();
        for (DFATransMap::iterator it = transMap.begin(); it != transMap.end(); ++it) {
            char c = it -> first;
            InformationForStr *info = new InformationForStr();
            info -> state = it -> second;
            for (std::unordered_map<DFAState*, DFAState*>::iterator it1 = m.begin(); it1 != m.end(); ++it1) {
                if (it1 -> second == NULL) {
                    (info -> mapping)[it1 -> first] = (DFAState*)NULL;
                }
                else {
                    DFAState *next = (it1 -> second) -> getState(c);
                    if (next == NULL) {
                        (info -> mapping)[it1 -> first] = (DFAState*)NULL;
                    }
                    else {
                        (info -> mapping)[it1 -> first] = next;
                    }
                }
            }
            bool flag = false;
            for (size_t j = 0; j < states.size(); ++j) {
                if (*reinterpret_cast<InformationForStr*>(states[j]->getInformation()) == *info) {
                    flag = true;
                    states[i]->addTrans(c, states[j]);
                    delete info;
                    break;
                }
            }
            if (!flag) {
                DFAState *newState = new AggregateStateForStr(info);
                states[i] -> addTrans(c, newState);
                res -> addState(newState);
                states.push_back(newState);
                if ((info -> state) -> isEnd()) {
                    res -> addFinalState(newState);
                }
            }
        }
        ++i;
    }
    return res;
}

int findState(std::vector<std::vector<State*> > &states, std::vector<State*> &state) {
    for (size_t i = 0; i < states.size(); ++i) {
        bool flag = true;
        for (size_t j = 0; j < state.size(); ++j) {
            if (states[i][j] != state[j]) {
                flag = false;
                break;
            }
        }
        if (flag) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void addNewTranNFA(char c, std::vector<std::vector<State*> >& states, std::vector<State*>& state, std::vector<NFAState*>& newStates, NFA* res, int i) {
    int stateIndex = findState(states,state);
    if (stateIndex == -1) {
        NFAState *newState = res -> mkNewState();
        newStates.push_back(newState);
        states.push_back(state);
        newStates[i] -> addTrans(c, newState);
        if (state[0] -> isEnd() && state[1] -> isEnd() && state[2] -> isEnd()) {
            res -> addFinalState(newState);
        }
     /*   for (size_t m = 0; m < state.size(); ++m) {
            std::cout << state[m] << ":";
        }
        std::cout << c << "-tran:" << state[i] << newState <<std::endl;*/
    }
    else {
        newStates[i] -> addTrans(c, newStates[stateIndex]);
    }
}

NFA *Solver::getNewY(DFA *xDFA, DFA *yDFA, std::unordered_map<DFAState*,DFAState*> &stateMap, NFA *patternNFA, std::unordered_set<char> &single) {
    NFA *res = new NFA();
    //new states
    std::vector<NFAState*> states;
    std::vector<std::vector<State*> > stateVecs;
    //------------------------------------------
    NFAState *startState = res -> mkNewState();
    std::vector<State*> startStateVec;
    startStateVec.push_back(yDFA -> getStartState());
    startStateVec.push_back(xDFA -> getStartState());
    startStateVec.push_back(patternNFA -> getStartState());
    states.push_back(startState);
    stateVecs.push_back(startStateVec);
    //--------------------------------------------
    res -> setStartState(startState);
    if (startStateVec[0] -> isEnd() && startStateVec[1] -> isEnd() && startStateVec[2] -> isEnd()) {
        res -> addFinalState(startState);
    }
    
    size_t i = 0;
    
    while (i != stateVecs.size()) {
        DFAState *stateY = dynamic_cast<DFAState*>(stateVecs[i][0]);
        DFAState *stateX = dynamic_cast<DFAState*>(stateVecs[i][1]);
        NFAState *statePattern = dynamic_cast<NFAState*>(stateVecs[i][2]);
        DFATransMap &transMap = stateY -> getTransMap();
        for (DFATransMap::iterator it = transMap.begin(); it != transMap.end(); ++it) {
            std::vector<State*> stateVec(3,0);
            //
            stateVec[0] = it -> second;
            NFAStateSet pStates = statePattern -> getStates(it -> first);
            if (pStates.size() != 0) {
                for (NFAStateSet::iterator it1 = pStates.begin(); it1 != pStates.end(); ++it1) {
                    stateVec[2] = *it1;
                    if (statePattern -> isEnd() && (*it1) -> isEnd()) {
                        //pattern 接受当前字符
                        if (single.count(it -> first) == 1) {
                            DFAState *nextX = stateMap[stateX];
                            if (nextX != NULL) {
                                stateVec[1] = nextX;
                                addNewTranNFA(it -> first, stateVecs, stateVec, states, res, i);
                            }
                        }
                        else {
                            DFAState *nextX = stateX -> getState(it -> first);
                            if (nextX != NULL) {
                                stateVec[1] = nextX;
                                addNewTranNFA(it -> first, stateVecs, stateVec, states, res, i);
                            }
                        }
                    }
                    else if (statePattern -> isEnd() && !((*it1) -> isEnd())) {
                        DFAState *nextX = stateMap[stateX];
                        if (nextX != NULL) {
                            stateVec[1] = nextX;
                            addNewTranNFA(it -> first, stateVecs, stateVec, states, res, i);
                        }
                    }
                    else if (!(statePattern -> isEnd())) {
                        stateVec[1] = stateX;
                        addNewTranNFA(it -> first, stateVecs, stateVec, states, res, i);
                    }
                }
            }
            else {
                if (statePattern -> isEnd()) {
                    stateVec[2] = patternNFA -> getStartState();
                    DFAState *nextX = stateX -> getState(it -> first);
                    if (nextX != NULL) {
                        stateVec[1] = nextX;
                        addNewTranNFA(it -> first, stateVecs, stateVec, states, res, i);
                    }
                }
            }
        }
        ++i;
    }
    return res;
}
//x=replaceAll(y,pattern,z);
bool Solver::solveReplaceAll(FunTerm *funTerm, int i, std::stack<Model*> &models, std::stack<int> &constraintIndex, Model *curModel) {
    std::string xStr = funTerm -> getArg(0) -> toString();
    FunTerm *replaceAllTerm = dynamic_cast<FunTerm*>(funTerm -> getArg(1));
    std::string yStr = replaceAllTerm -> getArg(0) -> toString();
    std::string pattern = replaceAllTerm -> getArg(1) -> toString();
    std::string zStr = replaceAllTerm -> getArg(2) -> toString();
    DFA *xDFA = curModel -> getStrDFA(xStr);
    DFA *yDFA = curModel -> getStrDFA(yStr);
    DFA *zDFA = curModel -> getStrDFA(zStr);
    if (xDFA == NULL) {
        xDFA = new DFA(NULL);
        curModel -> addStrCons(xStr, xDFA);
    }
    if (yDFA == NULL) {
        yDFA = new DFA(NULL);
        curModel -> addStrCons(yStr, yDFA);
    }
    if (zDFA == NULL) {
        zDFA = new DFA(NULL);
        curModel -> addStrCons(zStr, zDFA);
    }
    DFAStateSet &xStates = xDFA -> getAllStates();
    DFA *newZ = getNewZ(xDFA, zDFA, xStates);
    NFA testNFA(pattern);
    std::unordered_set<char> single;
    if ((testNFA.getStartState()) -> isEnd()) {
        return false;
    }
    for (char c : charSet) {
        std::string s;
        s.push_back(c);
        if (testNFA.accept(s)) {
            single.insert(c);
        }
    }
 /*   std::cout << "singleSize" << single.size() <<std::endl;
    std::cout << "-------------------" << std::endl;*/
    testNFA.clear();
    NFA *patternNFA;
    if (pattern[0] != '(') {
        patternNFA = getStrPatternNFA(pattern);
    }
    else {
        patternNFA = getRegPatternNFA(pattern);
    }
    DFAStateSet &newZFinalState = newZ -> getFinalStates();
    //对于新的z的每个终止状态构造约束
    for (DFAState *s : newZFinalState) {
        InformationForStr *curInfo = reinterpret_cast<InformationForStr*>(s ->getInformation());
        std::unordered_map<DFAState*, DFAState*> &m = curInfo -> mapping;
        Model *newModel = new Model((*curModel));
        //-------------------------------------------
        std::unordered_map<DFAState*, DFAState*> sMap;
        DFA *newZCopy = new DFA(newZ, sMap);
        DFAStateSet zCopyFinalState = newZCopy -> getFinalStates();
        for (DFAState *s1: zCopyFinalState) {
            if (s1 != sMap[s]) {
                newZCopy -> removeFinalState(s1);
            }
        }
        newModel -> replaceStrCons(zStr, newZCopy);
        yDFA = newModel -> getStrDFA(yStr);
        NFA *newY = getNewY(xDFA, yDFA, m, patternNFA, single);
        DFAState *start = xDFA -> getStartState();
        newModel -> replaceStrCons(yStr, newY);
        models.push(newModel);
        constraintIndex.push(i + 1);
    }
    patternNFA -> clear();
    delete patternNFA;
    newZ -> clear();
    delete newZ;
    return true;
}

//x = concat(y,z);
bool Solver::solveConcat(FunTerm *funTerm, int i, std::stack<Model*> &models, std::stack<int> &constraintIndex, Model *curModel) {
    std::string xStr = funTerm -> getArg(0) -> toString();
    FunTerm *concatTerm = dynamic_cast<FunTerm*>(funTerm -> getArg(1));
    std::string yStr = concatTerm -> getArg(0) -> toString();;
    std::string zStr = concatTerm -> getArg(1) -> toString();
    DFA *xDFA = curModel -> getStrDFA(xStr);
    DFA *yDFA = curModel -> getStrDFA(yStr);
    DFA *zDFA = curModel -> getStrDFA(zStr);
    if (xDFA == NULL) {
        xDFA = new DFA(NULL);
        curModel -> addStrCons(xStr, xDFA);
    }
    if (yDFA == NULL) {
        yDFA = new DFA(NULL);
        curModel -> addStrCons(yStr, yDFA);
    }
    if (zDFA == NULL) {
        zDFA = new DFA(NULL);
        curModel -> addStrCons(zStr, zDFA);
    }
    DFAStateSet &xAllStates = xDFA -> getAllStates();
    DFAState *xStartState = xDFA -> getStartState();
    DFAStateSet xStartSet;
    xStartSet.insert(xStartState);
    DFAStateSet &xFinalState = xDFA -> getFinalStates();
    DFA *newY = getNewZ(xDFA, yDFA, xStartSet);
    DFAStateSet &newYFinalState = newY -> getFinalStates();
    DFA *newZ = getNewZ(xDFA, zDFA, xAllStates);
    DFAStateSet &newZFinalState = newZ -> getFinalStates();
    for (DFAState *s : newYFinalState) {
        InformationForStr *yInfo = reinterpret_cast<InformationForStr*>(s ->getInformation());
        std::unordered_map<DFAState*, DFAState*> &yMap = yInfo -> mapping;
        DFAState *xyNext = yMap[xStartState];
        if (xyNext == NULL) {
            continue;
        }
        for (DFAState *s1 : newZFinalState) {
            InformationForStr *zInfo = reinterpret_cast<InformationForStr*>(s1 ->getInformation());
            std::unordered_map<DFAState*, DFAState*> &zMap = zInfo -> mapping;
            DFAState *xzNext = zMap[xyNext];
            if (xzNext != NULL && xFinalState.count(xzNext) != 0) {
                Model *newModel = new Model((*curModel));
                std::unordered_map<DFAState*, DFAState*> sYMap;
                std::unordered_map<DFAState*, DFAState*> sZMap;
                DFA *newYCopy = new DFA(newY, sYMap);
                DFA *newZCopy = new DFA(newZ, sZMap);
                DFAStateSet newYCopyFinal = newYCopy -> getFinalStates();
                DFAStateSet newZCopyFinal = newZCopy -> getFinalStates();
                for (DFAState *s2 : newYCopyFinal) {
                    if (s2 != sYMap[s]) {
                        newYCopy -> removeFinalState(s2);
                    }
                }
                for (DFAState *s2 : newZCopyFinal) {
                    if (s2 != sZMap[s1]) {
                        newZCopy -> removeFinalState(s2);
                    }
                }
                newModel -> replaceStrCons(zStr, newZCopy);
                newModel -> addStrCons(yStr, newYCopy);
                newYCopy -> clear();
                delete newYCopy;
                models.push(newModel);
                constraintIndex.push(i + 1);
            }
        }
    }
    newY -> clear();
    delete newY;
    newZ -> clear();
    delete newZ;
    return true;
}
//对约束进行拓扑排序，若是无法进行topo排序证明变量的依赖关系图中有环，返回false,成功则返回true
bool Solver::topoSort(std::vector<FunTerm*> &constraints, std::unordered_set<std::string> &exceptVars) {
    std::vector<FunTerm*> temp;
    std::unordered_map<std::string, std::vector<std::string> > deGraph;
    std::unordered_map<std::string, int> precursor;
    std::unordered_map<std::string, int> succeed;
    for (size_t i = 0; i < constraints.size(); ++i) {
        if (constraints[i] -> getName() == RegexIn) {
            temp.push_back(constraints[i]);
        }
        else if (constraints[i] -> getName() == Equal){
            Term *arg0 = constraints[i] -> getArg(0);
            Term *arg1 = constraints[i] -> getArg(1);
            if ((arg1 -> getType()) == Func) {
                std::string arg0Str = arg0 -> toString();
                int domainSize = (dynamic_cast<FunTerm*>(arg1)) -> getDomainSize();
                if (precursor.count(arg0Str) == 0) {
                    precursor[arg0Str] = i;
                    exceptVars.insert(arg0Str);
                }
                else {
                    fprintf(stdout, "> UNKNOW(1): it is not straight-line.\n");
                    fflush(stdout);
                    return false;
                }
                for (int j = 0; j < domainSize; ++j) {
                    Term *argJ = (dynamic_cast<FunTerm*>(arg1)) -> getArg(j);
                    if (argJ -> getType() == StrVar) {
                        std::string argJStr = argJ -> toString();
                        if (succeed.count(argJStr) == 0) {
                            succeed[argJStr] = 1;
                        }
                        else {
                            ++succeed[argJStr];
                        }
                    }
                }
            }
            else if ((arg1 -> getType()) == StrVar) {
                std::string arg0Str = arg0 -> toString();
                std::string arg1Str = arg1 -> toString();
                if (precursor.count(arg0Str) == 0) {
                    precursor[arg0Str] = i;
                    exceptVars.insert(arg0Str);
                }
                else {
                    fprintf(stdout, "> UNKNOW(2): it is not straight-line.\n");
                    fflush(stdout);
                    return false;
                }
                if (succeed.count(arg1Str) == 0) {
                    succeed[arg1Str] = 1;
                }
                else {
                    ++succeed[arg1Str];
                }
            }
            //在目前语法中只有形如x = "abc"的情况，若是扩展语法，在此处添加else if的情况
            else {
                temp.push_back(constraints[i]);
            }
        }
    }
    while (precursor.size() != 0) {
        std::unordered_map<std::string, int>::iterator it;
        for (it = precursor.begin(); it != precursor.end(); ++it) {
            if (succeed.count(it -> first) == 0 || succeed[it -> first] == 0) {
                temp.push_back(constraints[it -> second]);
                if (constraints[it -> second] -> getName() == Equal) {
                    Term *arg1 = constraints[it -> second] -> getArg(1);
                    if (arg1 -> getType() == Func) {
                        int domainSize = (dynamic_cast<FunTerm*>(arg1)) -> getDomainSize();
                        for (int j = 0; j < domainSize; ++j) {
                            Term *argJ = (dynamic_cast<FunTerm*>(arg1)) -> getArg(j);
                            if (argJ -> getType() == StrVar) {
                                std::string argJStr = argJ -> toString();
                                --succeed[argJStr];
                            }
                        }
                        break;
                    }
                    else if (arg1 -> getType() == StrVar) {
                        std::string arg1Str = arg1 -> toString();
                        --succeed[arg1Str];
                        break;
                    }
                }
            }
        }
        if (it == precursor.end()) {
            fprintf(stdout, "> UNKNOW(3): it is not straight-line.\n");
            fflush(stdout);
            return false;
        }
        precursor.erase(it);
    }
    for (int i = 0; i < temp.size(); ++i) {
        constraints[i] = temp[i];
    }
    return true;
}

void cleanModels(std::stack<Model*> &models) {
    while (models.size() != 0) {
        Model *m = models.top();
        models.pop();
        delete m;
    }
}
void Solver::check(StdConstraintSet &stdConstraints) {
    std::vector<FunTerm*> &constraints = stdConstraints.getConstraints();
    int constraintSize = static_cast<int>(constraints.size());
    std::unordered_set<std::string> exceptVars;
    bool sortFlag = topoSort(constraints, exceptVars);
    if (!sortFlag) {
        delete &stdConstraints;
        exit(0);
    }
    std::stack<Model*> models;
    std::stack<int> constraintIndex;
    models.push(new Model());
    constraintIndex.push(0);
    while (models.size() != 0) {
        Model *curModel = models.top();
        models.pop();
        //当前求解到了第几条约束
        int curIndex = constraintIndex.top();
        constraintIndex.pop();
        //当所有约束求解完时，循环结束
        while (curIndex < constraintSize) {
            FunTerm* curConstraint = constraints[curIndex];
            if (curConstraint -> getName() == RegexIn) {
                std::string arg0Str = (curConstraint -> getArg(0)) -> toString();
                std::string arg1Str = (curConstraint -> getArg(1)) -> toString();
                DFA *dfa = new DFA(arg1Str);
                if ((curModel -> getStrCons(arg0Str)) == NULL) {
                    curModel -> addStrCons(arg0Str, dfa);
                }
                else {
                    curModel -> addStrCons(arg0Str, dfa);
                    delete dfa;
                }
            }
            else if(curConstraint -> getName() == Equal) {
                Term *arg1 = curConstraint -> getArg(1);
                TermType arg1Type = arg1 -> getType();
                if (arg1Type == ConstStr) {
                    std::string arg0Str = (curConstraint -> getArg(0)) -> toString();
                    std::string arg1Str = arg1 -> toString();
                    DFA *dfa = new DFA(arg1Str);
                    if ((curModel -> getStrCons(arg0Str)) == NULL) {
                        curModel -> addStrCons(arg0Str, dfa);
                    }
                    else {
                        curModel -> addStrCons(arg0Str, dfa);
                        delete dfa;
                    }
                }
                //x = y
                else if (arg1Type == StrVar) {
                    std::string arg0Str = (curConstraint -> getArg(0)) -> toString();
                    std::string arg1Str = arg1 -> toString();
                    DFA *xDFA = curModel -> getStrDFA(arg0Str);
                    DFA *yDFA = curModel -> getStrDFA(arg1Str);
                    if (xDFA == NULL && yDFA != NULL) {
                        curModel -> addStrCons(arg0Str, yDFA);
                    }
                    else if (xDFA != NULL && yDFA == NULL) {
                        curModel -> addStrCons(arg1Str, xDFA);
                    }
                    else if (xDFA != NULL && yDFA != NULL) {
                        DFA *newDFA = DFA::intersection(*xDFA, *yDFA);
                        curModel -> replaceStrCons(arg0Str, newDFA);
                        curModel -> replaceStrCons(arg1Str, newDFA);
                    }
                }
                else if (arg1Type == Func) {
                    FunName funName = (dynamic_cast<FunTerm*>(arg1)) -> getName();
                    if (funName == ReplaceAll) {
                        if (!solveReplaceAll(curConstraint,curIndex,models,constraintIndex,curModel)) {
                            printf("**********************************\n");
                            printf(">> UNKNOW\n");
                            cleanModels(models);
                            delete curModel;
                            return;
                        }
                        else {
                            break;
                        }
                    }
                    else if (funName == Concat) {
                        if (!solveConcat(curConstraint,curIndex,models,constraintIndex,curModel)) {
                            printf("**********************************\n");
                            printf(">> UNKNOW\n");
                            cleanModels(models);
                            delete curModel;
                            return;
                        }
                        else {
                            break;
                        }
                    }
                }
            }
            ++curIndex;
        }
        //求解完所有constraint,看是否有解.
        if (curIndex == constraintSize) {
            std::unordered_map<std::string, std::string> *solution = curModel -> getSolution(exceptVars);
            if (solution != NULL) {
                fullSolution(solution, stdConstraints);
                printSolution(solution);
                cleanModels(models);
                delete curModel;
                delete solution;
                return;
            }
        }
        delete curModel;
    }
    printf("**********************************\n");
    printf(">> UNSAT\n");
}
