#ifndef automata_h
#define automata_h
#include<set>
#include<map>
#include<unordered_map>
#include<unordered_set>
#include<algorithm>

extern char emptyChar;
extern std::unordered_set<char> faCharSet;
class DFAState;
class NFAState;
class DFA;
class NFA;
class FA;

typedef std::unordered_map<char, std::unordered_set<NFAState*> > NFATransMap;
typedef std::unordered_map<char, DFAState*> DFATransMap;
typedef std::unordered_set<NFAState*> NFAStateSet;
typedef std::unordered_set<DFAState*> DFAStateSet;

struct InformationForStr {
    DFAState* state;
    std::unordered_map<DFAState*, DFAState*> mapping;
    friend bool operator== (const InformationForStr& info1,const InformationForStr& info2) {
        if (info1.state == info2.state && info1.mapping == info2.mapping)
            return true;
        else
            return false;
    }
};

struct InformationForReg {
    DFAStateSet preStates;
    DFAStateSet noFinalStates;
    friend bool operator== (const InformationForReg& info1, const InformationForReg& info2) {
        if (info1.noFinalStates == info2.noFinalStates && info1.preStates == info2.preStates)
            return true;
        else
            return false;
        
    }
};

//-------------------------------------------------
class State {
public:
    virtual bool isEnd() = 0;
    virtual void setEnd(bool e) = 0;
    virtual ~State() {}
};
class NFAState : public State
{
private:
    unsigned long long id;
    static unsigned long long size;
    bool end;
    NFATransMap transMap;
    
public:
    friend class NFA;
    
    NFAState():id(size++),end(false) {}
    void setEnd(bool e) { end = e; }
    bool isEnd() { return end; }
    void addTrans(char c, NFAState* s) { transMap[c].insert(s); }
    NFATransMap& getTransMap() { return transMap; }
    bool hasTrans(char c) {
        if (transMap.count(c) != 0)
            return true;
        else
            return false;
    }
    NFAStateSet getStates(char c);
    virtual void* getInformation() { return (void*)&id; };
    virtual ~NFAState() {}
};

class DFAState : public State
{
private:
    unsigned long long id;
    static unsigned long long size;
    bool end;
    DFATransMap transMap;
    
public:
    friend class DFA;
    
    DFAState():id(size++),end(false) {}
    void setEnd(bool e) { end = e; }
    bool isEnd() { return end; }
    void addTrans(char c, DFAState* s) {
        if (transMap.count(c) == 1) {
            fprintf(stdout, "> Error: DFA state can only have one %c-transition.\n", c);
            fflush(stdout);
        }
        else {
            transMap[c] = s;
        }
    }
    DFATransMap& getTransMap() { return transMap; }
    bool hasTrans(char c) {
        if (transMap.count(c) != 0)
            return true;
        else
            return false;
    }
    DFAState* getState(char c) {
        if (transMap.count(c) == 0) {
            return NULL;
        } else {
            return transMap[c];
        }
    }
    virtual void* getInformation() { return (void*)&id; };
    virtual ~DFAState() {}
};

class FA
{
public:
    static void setFACharSet() {
        for (int i = 0; i < 127; ++i) {
            faCharSet.insert(static_cast<char>(i));
        }
    }
    virtual std::string getWord(bool &succ) = 0;
    virtual bool isEmpty() = 0;
    virtual bool accept(std::string &s) = 0;
    virtual void clear() = 0;
    virtual bool isDeterministic() = 0;
    virtual ~FA() {}
};

class NFA : public FA
{
private:
    NFAState* startState;
    NFAStateSet finalStates;
    NFAStateSet allStates;
    
public:
    NFA(): startState(NULL) {};
    NFA(NFA &nfa);
    NFA(NFA *nfaP);
    NFA(DFA &dfa);
    NFA(DFA *dfaP);
    NFA(std::string regex);
    NFA(NFA *nfaP, std::unordered_map<NFAState*,NFAState*> &sMap);
    
    NFAStateSet &getFinalStates() { return finalStates; }
    NFAStateSet &getAllStates() { return allStates; }
    NFAState *getStartState() { return startState; }
    void setStartState(NFAState* const s) { startState = s; addState(startState); }
    void addFinalState(NFAState *s) { finalStates.insert(s); s->setEnd(true); }
    void removeFinalState(NFAState *s) { finalStates.erase(s); s->setEnd(false); }
    void addState(NFAState *s) {allStates.insert(s);}
    NFAState *mkNewState() {
        NFAState *newState = new NFAState();
        addState(newState);
        return newState;
    }
    NFAState *mkNewFinalState() {
        NFAState *newFinalState = mkNewState();
        addFinalState(newFinalState);
        return newFinalState;
    }
    std::string getWord(NFAState *s, bool &succ);
    
    std::string getWord(bool &succ);
    bool isEmpty();
    bool accept(std::string &s);
    void clear();
    bool isDeterministic() {return false;}
    DFA* determine();
    //static NFA* intersection(NFA& fa1, NFA& fa2);
    ~NFA();
};

class DFA : public FA
{
private:
    DFAState* startState;
    DFAStateSet finalStates;
    DFAStateSet allStates;
    
public:
    DFA(): startState(NULL) {};
    DFA(DFA *dfaP);
    DFA(DFA &dfa);
    DFA(std::string regex);
    DFA(DFA* dfaP, std::unordered_map<DFAState*, DFAState*>& sMap);
    
    DFAStateSet &getFinalStates() { return finalStates; }
    DFAStateSet &getAllStates() { return allStates; }
    DFAState *getStartState() { return startState; }
    void setStartState(DFAState *s) { startState = s; addState(startState);}
    void addFinalState(DFAState *s) { finalStates.insert(s); s->setEnd(true); }
    void removeFinalState(DFAState* s) { finalStates.erase(s); s->setEnd(false); }
    void addState(DFAState *s) { allStates.insert(s); }
    DFAState *mkNewState() {
        DFAState *newState = new DFAState();
        addState(newState);
        return newState;
    }
    DFAState *mkNewFinalState() {
        DFAState *newFinalState = mkNewState();
        addFinalState(newFinalState);
        return newFinalState;
    }
    
    std::string getWord(DFAState *s, bool &succ);
    
    std::string getWord(bool &succ);
    bool isEmpty();
    bool accept(std::string &s);
    void clear();
    bool isDeterministic() {return true;}
    static DFA* intersection(DFA& fa1, DFA& fa2);
    ~DFA();
};

//----------------------------------------------------

class AggregateStateForStr :public DFAState {
private:
    InformationForStr* info;
public:
    AggregateStateForStr() {}
    AggregateStateForStr(InformationForStr *i) { info = i; }
    virtual void* getInformation() { return reinterpret_cast<void*>(info); }
    virtual ~AggregateStateForStr() { delete info; }
};

class AggregateStateForReg :public NFAState {
private:
    InformationForReg* info;
public:
    AggregateStateForReg() {}
    AggregateStateForReg(InformationForReg* i) { info = i; }
    virtual void* getInformation() { return reinterpret_cast<void*>(info); }
    virtual ~AggregateStateForReg() { delete info; }
};
#endif
