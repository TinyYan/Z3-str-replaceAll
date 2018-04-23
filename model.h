//
//  model.h
//  strTest
//
//  Created by 陈艳 on 2018/1/17.
//  Copyright © 2018年 陈艳. All rights reserved.
//

#ifndef model_h
#define model_h

#include <vector>
#include <unordered_map>
#include <unordered_set>
//#include "common.h"
#include "automata.h"
#include "parser.h"

//if there are some other type vars, add maps into model class
class Model {
private:
    //所有自动机的引用次数计数，当计数为0时，从map中删除，并销毁此自动机，所有model共享一份
    static std::unordered_map<FA*, int> allFARef;
    //int tempVarNum;
    std::unordered_map<std::string, FA*> strVar;
    
    void cleanFA(FA *fa);
public:
  //  Model():tempVarNum(0) {}
    
    Model() {}
    
    Model(const Model& model);
    
   /* int getTempVarNum() {
        return tempVarNum;
    }*/
    
    void addStrVar(const std::string &name);
    
    void addStrCons(const std::string &name, FA *fa);
    
    void replaceStrCons(const std::string &name, FA *fa);
    
    FA *getStrCons(const std::string &name);
    
    DFA *getStrDFA(const std::string &name);
    
    TermType getVarType(const std::string &name);
    
    std::unordered_map<std::string, std::string> *getSolution();
    
    std::unordered_map<std::string, std::string> *getSolution(std::unordered_set<std::string> &exceptVars);
    
    ~Model();
    
    static void clear();
    
};

#endif /* model_h */
