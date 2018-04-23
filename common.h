#ifndef COMMON
#define COMMON

//debug宏
#ifdef DEBUGLOG
#define __debugPrint(_fp, _format, ...) { fprintf( (_fp), (_format), ##__VA_ARGS__); fflush( (_fp) ); }
#define printZ3Node(t, n) {__printZ3Node( (t), (n));}
#else
#define __debugPrint(_fp, _format, ...) {}
#define printZ3Node(t, n) {}
#endif

#include <stdio.h>
#include <string>
#include <unordered_set>

extern FILE *logFile;

extern std::unordered_set<char> charSet;
extern bool defaultCharSet;

class Common {
private:
    static bool isValidHexDigit(char c);
    
    static int hexDigitToInt(char a);
    
    static int twoHexDigitToChar(char a, char b);
    
public:
    static std::string str2RegexStr(std::string &str);
    
    static void setAlphabet();
    
    static std::string convertInputTrickyConstStr(std::string &inputStr);
};

#endif
