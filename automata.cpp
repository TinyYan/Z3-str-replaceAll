//#include "stdafx.h"
#include <stack>
#include<vector>
#include<string>
#include<iostream>
#include<stack>
#include<algorithm>
#include "automata.h"

char emptyChar = static_cast<char>(127);
std::unordered_set<char> faCharSet;
unsigned long long NFAState::size = 0;
unsigned long long DFAState::size = 0;

NFAStateSet NFAState::getStates(char c) {
    NFAStateSet res;
    // c is emptyChar or c is not emptyChar
    if (c == emptyChar) {
        NFAStateSet last;
        NFAStateSet current;
        res.insert(this);
        last.insert(this);
        //fixed point
        while (last.size() != 0) {
            for (NFAStateSet::iterator it = last.begin(); it != last.end(); ++it) {
                NFATransMap &tran = (*it) -> getTransMap();
                if (tran.count(emptyChar) != 0) {
                    NFAStateSet& next = tran[c];
                    for (NFAStateSet::iterator it1 = next.begin(); it1 != next.end(); ++it1) {
                        if (res.find(*it1) != res.end())
                            continue;
                        else {
                            res.insert(*it1);
                            current.insert(*it1);
                        }
                    }
                }
            }
            last.clear();
            last = current;
            current.clear();
        }
        return res;
    } else {
        NFAStateSet emptyTranStates = getStates(emptyChar);
        for (NFAStateSet::iterator it = emptyTranStates.begin(); it != emptyTranStates.end(); ++it) {
            NFATransMap &tran = (*it) -> getTransMap();
            if (tran.count(c) != 0) {
                NFAStateSet& next = tran[c];
                for (NFAStateSet::iterator it1 = next.begin(); it1 != next.end(); ++it1) {
                    res.insert(*it1);
                }
            }
        }
        NFAStateSet last = res;
        NFAStateSet current;
        while (last.size() != 0){
            for (NFAStateSet::iterator it = last.begin(); it != last.end(); ++it) {
                NFAStateSet resEmptyTrans = (*it) -> getStates(emptyChar);
                if (resEmptyTrans.size() != 0){
                    for (NFAStateSet::iterator it1 = resEmptyTrans.begin(); it1 != resEmptyTrans.end(); ++it1) {
                        if (res.find(*(it1)) == res.end()){
                            current.insert(*(it1));
                            res.insert(*(it1));
                        }
                    }
                }
            }
            last.clear();
            last = current;
            current.clear();
        }
    }
    return res;
}

char getStringType(const std::string &s) {
    if (s.size() == 0)
        return 'e';//empty
    if (s[0] != '(')
        return 'c';//const string
    if (s.size() > 3 && s[s.size() - 1] == '*' && s[0] == '(' && s[s.size() - 2] == ')')
        return 's';//star
    if (s.size() > 4 && s[0] == '('&& s[s.size() - 1] == ')') {
        std::stack<char> sta;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] != ')')
                sta.push(s[i]);
            else {
                while (sta.top() != '(')
                    sta.pop();
                sta.pop();
            }
            if (sta.empty()) {
                if (s[i + 1] == '(')
                    return 't';//concat
                else
                    break;
            }
        }
    }
    return 'u';//union
}

//---------------------------------------------------------------------------------------------
//NFA functions

NFA::NFA(NFA &nfa) {
    NFAStateSet& nfaAllStates = nfa.getAllStates();
    std::unordered_map<NFAState*,NFAState*> stateMap;
    for(NFAState* s : nfaAllStates){
        NFAState* newS = mkNewState();
        if (s->isEnd()) {
            addFinalState(newS);
        }
        stateMap.insert(std::make_pair(s,newS));
	 }
    for(NFAState* s : nfaAllStates){
        NFATransMap& transMap = s -> getTransMap();
        for(NFATransMap::iterator it = transMap.begin(); it != transMap.end(); ++it) {
            for(NFAState* s1 : it->second)
                stateMap[s]->addTrans(it->first,stateMap[s1]);
        }
    }
    startState = stateMap[nfa.getStartState()];
}

NFA::NFA(NFA *nfaP) {
    //接受任意字符串的自动机
    if (nfaP == NULL) {
        startState = mkNewFinalState();
        for (std::unordered_set<char>::iterator it = faCharSet.begin(); it != faCharSet.end(); ++it) {
            startState -> addTrans(*it, startState);
        }
    } else {
        NFAStateSet& nfaAllStates = nfaP -> getAllStates();
        std::unordered_map<NFAState*,NFAState*> stateMap;
        for (NFAState* s : nfaAllStates) {
            NFAState* newS = mkNewState();
            if (s->isEnd()) {
                addFinalState(newS);
            }
            stateMap.insert(std::make_pair(s,newS));
        }
        for (NFAState* s : nfaAllStates) {
            NFATransMap& transMap = s -> getTransMap();
            for (NFATransMap::iterator it = transMap.begin(); it != transMap.end(); ++it) {
                for (NFAState* s1 : it->second)
                    stateMap[s]->addTrans(it->first,stateMap[s1]);
            }
        }
        startState = stateMap[nfaP -> getStartState()];
    }
}

NFA::NFA(DFA &dfa) {
    DFAStateSet& dfaAllStates = dfa.getAllStates();
    std::unordered_map<DFAState*,NFAState*> stateMap;
    for(DFAState* s : dfaAllStates){
        NFAState* newS = mkNewState();
        if (s->isEnd()) {
            addFinalState(newS);
        }
        stateMap.insert(std::make_pair(s,newS));
    }
    for(DFAState* s : dfaAllStates) {
        DFATransMap& transMap = s -> getTransMap();
        for(DFATransMap::iterator it = transMap.begin(); it != transMap.end(); ++it) {
            stateMap[s]->addTrans(it -> first, stateMap[it -> second]);
        }
    }
    startState = stateMap[dfa.getStartState()];
}

NFA::NFA(DFA *dfaP) {
    if (dfaP == NULL) {
        startState = mkNewFinalState();
        for (std::unordered_set<char>::iterator it = faCharSet.begin(); it != faCharSet.end(); ++it) {
            startState -> addTrans(*it, startState);
        }
    } else {
        DFAStateSet& dfaAllStates = dfaP -> getAllStates();
        std::unordered_map<DFAState*,NFAState*> stateMap;
        for(DFAState* s : dfaAllStates){
            NFAState* newS = mkNewState();
            if (s->isEnd()) {
                addFinalState(newS);
            }
            stateMap.insert(std::make_pair(s,newS));
        }
        for(DFAState* s : dfaAllStates) {
            DFATransMap& transMap = s -> getTransMap();
            for(DFATransMap::iterator it = transMap.begin(); it != transMap.end(); ++it) {
                stateMap[s] -> addTrans(it -> first, stateMap[it -> second]);
            }
        }
        startState = stateMap[dfaP -> getStartState()];
    }
}

NFA::NFA(std::string regex){
    char type = getStringType(regex);
    if (type == 'e') {
        startState = mkNewFinalState();
    }
    else if (type == 'c') {
        startState = mkNewState();
        NFAState* last = startState;
        for (size_t i = 0; i < regex.size(); ++i) {
            NFAState* next = mkNewState();
            if (regex[i] == '\\') {
                last -> addTrans(regex[++i], next);
            } else {
                last -> addTrans(regex[i], next);
            }
            last = next;
        }
        addFinalState(last);
    }
    else if (type == 's') {
        NFA *temp = new NFA(regex.substr(1, regex.size() - 3));
        finalStates = temp -> getFinalStates();
        startState = temp -> getStartState();
        allStates = temp -> getAllStates();
       // addFinalState(startState);
        for (NFAStateSet::iterator it = finalStates.begin(); it != finalStates.end(); ++it) {
            startState -> addTrans(emptyChar, (*it));
            (*it) -> addTrans(emptyChar, startState);
        }
        delete temp;
    }
    else if (type == 'u') {
        std::string left;
        std::string right;
        std::stack<char> s;
        size_t i = 0;
        for (; i < regex.size(); ++i) {
            if (regex[i] != ')')
                s.push(regex[i]);
            else {
                while (!s.empty()&&s.top() != '(') s.pop();
                if (!s.empty()) s.pop();
                if (s.empty()) break;
            }
        }
        left = regex.substr(1, i - 1);
        right = regex.substr(i + 3, regex.size() - i - 4);
        NFA *rightFA = new NFA(right);
        NFA *leftFA = new NFA(left);
        startState = mkNewState();
        startState -> addTrans(emptyChar, rightFA -> getStartState());
        startState -> addTrans(emptyChar, leftFA -> getStartState());
        NFAStateSet &rightFinal = rightFA -> getFinalStates();
        NFAStateSet &leftFinal = leftFA -> getFinalStates();
        NFAStateSet &rightAll = rightFA -> getAllStates();
        NFAStateSet &leftAll = leftFA -> getAllStates();
        for (NFAStateSet::iterator it = rightAll.begin(); it != rightAll.end(); ++it) {
            addState(*it);
        }
        for (NFAStateSet::iterator it = leftAll.begin(); it != rightAll.end(); ++it) {
            addState(*it);
        }
        for (NFAStateSet::iterator it = rightFinal.begin(); it != rightFinal.end(); ++it) {
            addFinalState(*it);
        }
        for (NFAStateSet::iterator it = leftFinal.begin(); it != rightFinal.end(); ++it) {
            addFinalState(*it);
        }
        delete rightFA;
        delete leftFA;
    }
    else if (type == 't') {
        std::string left;
        std::string right;
        size_t i = 0;
        std::stack<char> sta;
        for (; i < regex.size(); ++i) {
            if (regex[i] != ')')
                sta.push(regex[i]);
            else {
                while (sta.top() != '(')
                    sta.pop();
                sta.pop();
            }
            if (sta.empty())
                break;
        }
        left = regex.substr(1, i - 1);
        right = regex.substr(i + 2,regex.size() - i - 3);
        NFA* rightFA = new NFA(right);
        NFA* leftFA = new NFA(left);
        NFAStateSet& leftFinalState = leftFA -> getFinalStates();
        NFAStateSet& rightFinalState = rightFA -> getFinalStates();
        NFAState* rightStartState = rightFA -> getStartState();
        startState = leftFA -> getStartState();
        for (NFAState* s : leftFinalState) {
            s->addTrans(emptyChar, rightStartState);
            s->setEnd(false);
        }
        for (NFAState* s : rightFinalState) {
            addFinalState(s);
        }
        NFAStateSet& rightAll = rightFA -> getAllStates();
        NFAStateSet& leftAll = leftFA -> getAllStates();
        for (NFAStateSet::iterator it = rightAll.begin(); it != rightAll.end(); ++it) {
            addState(*it);
        }
        for (NFAStateSet::iterator it = leftAll.begin(); it != rightAll.end(); ++it) {
            addState(*it);
        }
        delete leftFA;
        delete rightFA;
    }
}

NFA::NFA(NFA* nfaP,std::unordered_map<NFAState*,NFAState*> &sMap) {
    NFAStateSet& nfaAllStates = nfaP -> getAllStates();
    //std::unordered_map<NFAState*,NFAState*> stateMap;
    for(NFAState* s : nfaAllStates){
        NFAState* newS = mkNewState();
        if (s->isEnd()) {
            addFinalState(newS);
        }
        sMap.insert(std::make_pair(s,newS));
	 }
    for(NFAState *s : nfaAllStates){
        NFATransMap& transMap = s -> getTransMap();
        for(NFATransMap::iterator it = transMap.begin(); it != transMap.end(); ++it){
            for(NFAState* s1 : it->second)
                sMap[s]->addTrans(it->first,sMap[s1]);
		 }
	 }
    startState = sMap[nfaP -> getStartState()];
}

std::string NFA::getWord(bool &succ) {
    if (startState == NULL || finalStates.size() == 0) {
        succ = false;
        return "";
    }
    if (finalStates.size() == 0) {
        succ = false;
        return "";
    }
	NFAStateSet states;
	std::unordered_map<NFAState*, std::string> last;
	std::unordered_map<NFAState*, std::string> current;
	last[startState] = "";
	states.insert(startState);
	while (last.size() != 0) {
		for (std::unordered_map<NFAState*, std::string>::iterator it = last.begin(); it != last.end(); ++it) {
			if ((it->first)->isEnd()) {
                succ = true;
				return it -> second;
			}
			else {
				NFATransMap &transMap = (it->first)->getTransMap();
				for (NFATransMap::iterator it1 = transMap.begin(); it1 != transMap.end(); ++it1) {
					NFAStateSet nextStates = it1->second;
					for (NFAStateSet::iterator it2 = nextStates.begin(); it2 != nextStates.end(); ++it2) {
						if (states.find(*it2) != states.end())
							continue;
						else {
							std::string lastString = it->second;
							if (it1->first != emptyChar){
								current[*it2] = lastString.append(1 ,it1->first);
							}
							else
								current[*it2] = lastString;
							states.insert(*it2);
						}
					}
				}
			}
		}
		last.clear();
		last = current;
		current.clear();
	}
    succ = false;
	return "";
}

std::string NFA::getWord(NFAState *s, bool &succ) {
    if (startState == NULL || finalStates.size() == 0) {
        succ = false;
        return "";
    }
    if (finalStates.find(s) == finalStates.end()) {
        succ = false;
		return "";
    }
    if (s == NULL) {
        succ = false;
        return "";
    }
	NFAStateSet states;
	std::unordered_map<NFAState*, std::string> last;
	std::unordered_map<NFAState*, std::string> current;
	last[startState] = "";
	states.insert(startState);
	while (last.size() != 0) {
		for (std::unordered_map<NFAState*, std::string>::iterator it = last.begin(); it != last.end(); ++it) {
			if ((it->first) == s) {
                succ = true;
				return it -> second;
			}
			else {
				NFATransMap transMap = (it->first)->getTransMap();
				for (NFATransMap::iterator it1 = transMap.begin(); it1 != transMap.end(); ++it1) {
					NFAStateSet nextStates = it1->second;
					for (NFAStateSet::iterator it2 = nextStates.begin(); it2 != nextStates.end(); ++it2) {
						if (states.find(*it2) != states.end())
							continue;
						else {
							std::string lastString = it->second;
							if (it1->first != emptyChar)
								current[*it2] = lastString.append(1, it1->first);
							else
								current[*it2] = lastString;
							states.insert(*it2);
						}
					}
				}
			}
			//else end
		}
		last.clear();
		last = current;
		current.clear();
	}
    succ = false;
	return "";
}

DFA *NFA::determine(){
	DFA *res = new DFA();
    NFAState *startState = getStartState();
	std::vector<NFAStateSet>* states = new std::vector<NFAStateSet>();
	std::vector<DFAState*> newStates;
	NFAStateSet state;
	DFAState *newState = res -> mkNewState();
    state = startState -> getStates(emptyChar);
	for (NFAState *s : state) {
		if (s -> isEnd()) {
			res -> addFinalState(newState);
		}
	}
	states -> push_back(state);
	newStates.push_back(newState);
	res -> setStartState(newState);
	size_t i = 0;
	while (i != states -> size()) {
		NFATransMap tran;
		for (NFAState *s : (*states)[i]) {
			NFATransMap transMap = s -> getTransMap();
			for (NFATransMap::iterator it = transMap.begin(); it != transMap.end(); ++it) {
				if (it->first == emptyChar)
					continue;
				for (NFAState *s1 : it->second) {
					tran[it->first].insert(s1);
                    NFAStateSet emptyTran = s1 -> getStates(emptyChar);
					for (NFAState *s2 : emptyTran)
						tran[it->first].insert(s2);
				}
			}
		}
		for (NFATransMap::iterator it1 = tran.begin(); it1 != tran.end(); ++it1) {
			bool flag = false;
			for (size_t j = 0; j < states -> size(); ++j) {
				if ((*states)[j] == it1 -> second) {
					newStates[i] -> addTrans(it1 -> first, newStates[j]);
					flag = true;
					break;
				}
			}
			if (!flag) {
				states->push_back(it1->second);
				newState = res -> mkNewState();
				newStates.push_back(newState);
				newStates[i]->addTrans(it1 -> first, newState);
				for (NFAState *s : it1 -> second) {
					if (s -> isEnd()) {
						res->addFinalState(newState);
					}
				}
			}
		}
		++i;
	}
	delete states;
	return res;
}

bool NFA::accept(std::string &s) {
    if (startState == NULL || finalStates.size() == 0) {
        return false;
    }
    NFAStateSet reachStates;
    reachStates.insert(startState);
    for (size_t i = 0; i < s.size(); ++i) {
        NFAStateSet next;
        for (NFAState* state:reachStates) {
            NFAStateSet temp = state -> getStates(s[i]);
            if (temp.size() != 0) {
                for (NFAState* s1 : temp) {
                    next.insert(s1);
                }
            }
        }
        if (next.size() == 0) {
            return false;
        }
        reachStates = next;
    }
    for (NFAState* state : reachStates) {
        if (state -> isEnd())
            return true;
    }
    return false;
}

bool NFA::isEmpty() {
    bool succ = false;
    getWord(succ);
	if (succ)
		return false;
	else
		return true;

}
void NFA::clear() {
	for (NFAStateSet::iterator it = allStates.begin(); it != allStates.end(); ++it) {
		delete *it;
	}
}

NFA::~NFA()
{
}

//-----------------------------------------------------------------------------------------------
// DFA functions

DFA::DFA(DFA *dfaP) {
    if (dfaP == NULL) {
        startState = mkNewFinalState();
        for (std::unordered_set<char>::iterator it = faCharSet.begin(); it != faCharSet.end(); ++it) {
            startState -> addTrans(*it, startState);
        }
    } else {
        DFAStateSet& dfaAllStates = dfaP -> getAllStates();
        std::unordered_map<DFAState*,DFAState*> stateMap;
        for(DFAState *s : dfaAllStates){
            DFAState *newS = mkNewState();
            if(s->isEnd()) {
                addFinalState(newS);
            }
            stateMap.insert(std::make_pair(s,newS));
        }
        for(DFAState *s : dfaAllStates){
            DFATransMap& transMap = s -> getTransMap();
            for(DFATransMap::iterator it = transMap.begin(); it != transMap.end(); ++it){
                stateMap[s] -> addTrans(it->first, stateMap[it -> second]);
            }
        }
        startState = stateMap[dfaP -> getStartState()];
    }
}

DFA::DFA(DFA &dfa) {
    DFAStateSet& dfaAllStates = dfa.getAllStates();
    std::unordered_map<DFAState*, DFAState*> stateMap;
    for(DFAState *s : dfaAllStates){
        DFAState *newS = mkNewState();
        if(s->isEnd()) {
            addFinalState(newS);
        }
        stateMap.insert(std::make_pair(s,newS));
    }
    for(DFAState *s : dfaAllStates){
        DFATransMap& transMap = s -> getTransMap();
        for(DFATransMap::iterator it = transMap.begin(); it != transMap.end(); ++it){
            stateMap[s]->addTrans(it->first, stateMap[it -> second]);
        }
    }
    startState = stateMap[dfa.getStartState()];
}

DFA::DFA(std::string regex) {
    NFA regexNFA(regex);
    DFA *regexDFAP = regexNFA.determine();
    new (this) DFA(regexDFAP);
    regexNFA.clear();
    regexDFAP -> clear();
    delete regexDFAP;
}

DFA::DFA(DFA *dfaP, std::unordered_map<DFAState*, DFAState*> &sMap) {
    DFAStateSet& dfaAllStates = dfaP -> getAllStates();
    std::unordered_map<DFAState*,DFAState*> stateMap;
    for (DFAState* s : dfaAllStates) {
        DFAState* newS = mkNewState();
        if(s->isEnd())
            addFinalState(newS);
        sMap.insert(std::make_pair(s,newS));
    }
    for (DFAState *s : dfaAllStates) {
        DFATransMap& transMap = s -> getTransMap();
        for (DFATransMap::iterator it = transMap.begin(); it != transMap.end(); ++it) {
            sMap[s]->addTrans(it -> first, sMap[it -> second]);
        }
    }
    startState = sMap[dfaP -> getStartState()];
}

std::string DFA::getWord(bool &succ) {
    if (startState == NULL || finalStates.size() == 0) {
        succ = false;
        return "";
    }
   /* if (finalStates.size() == 0) {
        succ = false;
        return "";
    }*/
    DFAStateSet states;
    std::unordered_map<DFAState*, std::string> last;
    std::unordered_map<DFAState*, std::string> current;
    last[startState] = "";
    states.insert(startState);
    while (last.size() != 0) {
        for (std::unordered_map<DFAState*, std::string>::iterator it = last.begin(); it != last.end(); ++it) {
            if ((it->first)->isEnd()) {
                succ = true;
                return it -> second;
            }
            else {
                DFATransMap &transMap = (it->first)->getTransMap();
                for (DFATransMap::iterator it1 = transMap.begin(); it1 != transMap.end(); ++it1) {
                    DFAState *nextState = it1->second;
                    if (states.find(nextState) != states.end())
                        continue;
                    else {
                        std::string lastString = it->second;
                        if (it1->first != emptyChar){
                            current[nextState] = lastString.append(1 ,it1->first);
                        }
                        else
                            current[nextState] = lastString;
                        states.insert(nextState);
                    }
                }
            }
        }
        last.clear();
        last = current;
        current.clear();
    }
    succ = false;
    return "";
}

std::string DFA::getWord(DFAState *s, bool &succ) {
    if (startState == NULL || finalStates.size() == 0) {
        succ = false;
        return "";
    }
    if (finalStates.find(s) == finalStates.end()) {
        succ = false;
        return "";
    }
    if (s == NULL) {
        succ = false;
        return "";
    }
    DFAStateSet states;
    std::unordered_map<DFAState*, std::string> last;
    std::unordered_map<DFAState*, std::string> current;
    last[startState] = "";
    states.insert(startState);
    while (last.size() != 0) {
        for (std::unordered_map<DFAState*, std::string>::iterator it = last.begin(); it != last.end(); ++it) {
            if ((it->first) == s) {
                succ = true;
                return it -> second;
            }
            else {
                DFATransMap transMap = (it->first)->getTransMap();
                for (DFATransMap::iterator it1 = transMap.begin(); it1 != transMap.end(); ++it1) {
                     DFAState *nextState = it1->second;
                    if (states.find(nextState) != states.end())
                        continue;
                    else {
                        std::string lastString = it->second;
                        if (it1->first != emptyChar)
                            current[nextState] = lastString.append(1, it1->first);
                        else
                            current[nextState] = lastString;
                        states.insert(nextState);
                    }
                }
            }
            //else end
        }
        last.clear();
        last = current;
        current.clear();
    }
    succ = false;
    return "";
}

DFA* DFA::intersection(DFA &dfa1, DFA &dfa2) {
    DFA* res = new DFA();
    DFAState* dfa1Start = dfa1.getStartState();
    DFAState* dfa2Start = dfa2.getStartState();
    std::vector<std::pair<DFAState*, DFAState*> > oldStatePair;
    std::vector<DFAState*> newStates;
    std::pair<DFAState*, DFAState*> startStatePair = std::make_pair(dfa1Start, dfa2Start);
    DFAState* newStartState = res -> mkNewState();
    res -> setStartState(newStartState);
    if (dfa1Start -> isEnd() && dfa2Start -> isEnd()) {
        res -> addFinalState(newStartState);
    }
    oldStatePair.push_back(startStatePair);
    newStates.push_back(newStartState);
    int i = 0;
    while (i != oldStatePair.size()) {
        std::unordered_map<char, std::pair<DFAState*, DFAState*> > tran;
        DFATransMap &firstTransMap = (oldStatePair[i].first) -> getTransMap();
        for (DFATransMap::iterator it = firstTransMap.begin(); it != firstTransMap.end(); ++it) {
            DFAState *secondTranState = (oldStatePair[i].second) ->getState(it -> first);
            bool flag = false;
            if (secondTranState == NULL) {
                continue;
            } else {
                std::pair<DFAState*, DFAState*> nextStatePair = std::make_pair(it -> second, secondTranState);
                for (size_t j = 0; j < oldStatePair.size(); ++j) {
                    if (oldStatePair[j] == nextStatePair) {
                        newStates[i] -> addTrans(it -> first, newStates[j]);
                        flag = true;
                        break;
                    }
                }
                if (!flag) {
                    DFAState *nextNewState = res -> mkNewState();
                    oldStatePair.push_back(nextStatePair);
                    newStates.push_back(nextNewState);
                    newStates[i] -> addTrans(it -> first, nextNewState);
                    if ((nextStatePair.first) -> isEnd() && (nextStatePair.second) -> isEnd()) {
                        res -> addFinalState(nextNewState);
                    }
                }
            }
        }
        ++i;
    }
    return  res;
}

bool DFA::accept(std::string &s) {
    DFAState *reachState;
    reachState = startState;
    for (size_t i = 0; i < s.size(); ++i) {
        reachState = reachState -> getState(s[i]);
        if (reachState == NULL) {
            return NULL;
        }
    }
    if (reachState -> isEnd()) {
        return true;
    }
    return false;
}

bool DFA::isEmpty() {
    bool succ = false;
    getWord(succ);
    if (succ)
        return false;
    else
        return true;
    
}
void DFA::clear() {
    for (DFAStateSet::iterator it = allStates.begin(); it != allStates.end(); ++it) {
        delete *it;
    }
}

DFA::~DFA()
{
}

