
//
//  main.cpp
//  strTest
//
//  Created by 陈艳 on 2017/12/29.
//  Copyright © 2017年 陈艳. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <vector>
#include <unistd.h>
#include <getopt.h>

#ifndef DEBUGLOG
#define DEBUGLOG
#endif

#include "common.h"
#include "parser.h"
#include "model.h"
#include "solver.h"


int main(int argc, char ** argv) {
    std::string inputFile = "";
    int c;
#ifdef DEBUGLOG
    logFile = fopen("./log", "w");
#endif
    static struct option long_options[] =
    {
        { "input", required_argument, 0, 'f' },
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0 }
    };
    
    while (1)
    {
        int option_index = 0;
        c = getopt_long(argc, argv, "hpf:l:", long_options, &option_index);
        
        if (c == -1)
            break;
        
        switch (c)
        {
            case 'f':
            {
                inputFile = std::string(optarg);
                break;
            }
            case 'h':
            {
                break;
            }
            default:
                exit(0);
        }
    }
    FA::setFACharSet();
    StdConstraintSet *stdCons = new StdConstraintSet();
    Z3SMTParser::parse(inputFile, *stdCons);
    Solver::check(*stdCons);
    //stdCons -> print();
    delete stdCons;
    
    
#ifdef DEBUGLOG
    fclose(logFile);
#endif
    return 0;
}
