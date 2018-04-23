#include "common.h"

FILE *logFile = NULL;

std::unordered_set<char> charSet;
bool defaultCharSet = true;


std::string Common::convertInputTrickyConstStr(std::string &inputStr) {
    std::string outputStr = "";
    std::string innerStr = inputStr.substr(11, inputStr.size() - 11);
    int innerStrLen = static_cast<int>(innerStr.size());
    if (innerStrLen % 4 != 0) {
        fprintf(stdout, "> Error: Constant string conversion error. Exit.\n");
        fprintf(stdout, "         Input encoding: %s\n", inputStr.c_str());
        fflush(stdout);
        exit(0);
    }
    for (int i = 0; i < (innerStrLen / 4); i++) {
        std::string cc = innerStr.substr(i * 4, 4);
        if (cc[0] == '_' && cc[1] == 'x' && isValidHexDigit(cc[2]) && isValidHexDigit(cc[3])) {
            char dc = static_cast<char>(twoHexDigitToChar(cc[2], cc[3]));
            // Check whether the input character in the charSet
            if (charSet.find(dc) == charSet.end()) {
                fprintf(stdout, "> Error: Character '%d' in a constant string is not in the system alphabet.\n", static_cast<int>(dc));
                fprintf(stdout, "         Please set the character set accordingly.\n");
                fflush(stdout);
                exit(0);//注册并重写退出函数，注意资源回收，内存泄漏
            }
            outputStr = outputStr + std::string(1, dc);
        }
    }
    return outputStr;
}

std::string Common::str2RegexStr(std::string &str) {
    std::string res = "";
    int len = static_cast<int>(str.size());
    for (int i = 0; i < len; i++) {
        char nc = str[i];
        // 12 special chars
        if (nc == '\\' || nc == '^' || nc == '$' || nc == '.' || nc == '|' || nc == '?'
            || nc == '*' || nc == '+' || nc == '(' || nc == ')' || nc == '[' || nc == '{') {
            res.append(1, '\\');
        }
        res.append(1, str[i]);
    }
    return res;
}

int Common::twoHexDigitToChar(char a, char b) {
    return (hexDigitToInt(a) * 16 + hexDigitToInt(b));
}

int Common::hexDigitToInt(char a) {
    if ('0' <= a && a <= '9')
        return a - '0';
    else if ('a' <= a && a <= 'f')
        return 10 + a - 'a';
    else if ('A' <= a && a <= 'F')
        return 10 + a - 'A';
    else
        return 0;
}

bool Common::isValidHexDigit(char c) {
    if (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')) {
        return true;
    }
    return false;
}

void Common::setAlphabet() {
    if (defaultCharSet) {
        for (int i = 0; i < 127; ++i) {
            charSet.insert(static_cast<char>(i));
        }
    }
}
