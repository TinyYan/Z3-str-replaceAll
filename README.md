Z3-str-replaceAll focuses on replaceAll function.



# Installation

## Linux

1. Check out the latest version of Z3-str-replaceAll from the git repo.

   
2. Build  Z3-str-replaceAll
   * Modify variable "Z3_path" in the Z3-str-replaceAll Makefile to indicate the patched Z3 location.

      $ make

       
3. Setup Z3-str-replaceAll driver script
   * In "Z3-str-replaceAll.py", change the value of the variable "solver" to point to the 
     Z3-str-replaceAll binary "str" just built
 
 
4. Run Z3-str-replaceAll

      $./Z3-str-replaceAll.py -f ./tests/replaceAll-001


## Mac OS
0. Install "autoconf", "dos2unix", "gcc"
   * $ brew install autoconf
   * $ brew install dos2unix
   * $ brew install gcc --without-multilib
   
1. Build  Z3-str-replaceAll
   * Modify variable "Z3_path" in the Z3-str-repalceAll Makefile to indicate the patched Z3 location.

      $ make

       
2. Setup Z3-str-replaceAll driver script
   * In "Z3-str-replaceAll.py", change the value of the variable "solver" to point to the 
     Z3-str-replaceAll binary "str" just built
 
 
3. Run Z3-str-replaceAll
   *  ```Z3-str-replaceAll.py -f <inputFile>```, e.g 

      $./Z3-str-replaceAll.py -f ./tests/replaceAll-001



## Cygwin
Assume we are using cygwin version 2.873 (64 bit)

0. Install cygwin with the following additional packages
   * "autoconf 2.5" in "Devel"
   * "dos2unix" in "Untils"
   * "gcc-g++: GNU Compiler Collectoin" in "Devel" 
   * "make: The GNU version of the 'make' utility"
   * "patch" in "Devel"
   * "python: Python language interpreter" in "Python"   
    
1. Build Z3-str-replaceAll
    * Specify Z3 path in Makefile, e.g  
        Z3_path = /home/hi/z3/z3

    
    * make
      
    * Specify z3str binary in wrapper "Z3-str-replaceAll.py", e.g  
        solver = "/home/hi/z3/str/str.exe"

      
    * test     
        $ ./Z3-str-replaceAll.py -f ./tests/replaceAll-001 
 
