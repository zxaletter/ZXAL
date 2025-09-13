This is my compiler which generates x86-64 assembly and uses fasm to create executables  
for programs written in my toy programming language, ZXAL.
ZXAL is a statically typed programming language with a MINIMAL feature set.

Feature Set:
- Types:
  - int
  - char
  - bool

- Arithmetic Ops:
  - add
  - subtract
  - muliply
  - divide 
  - modulo
  
- Logical Ops:
  - &&
  - ||
  - !

- define functions
- define variables
- call functions
- pass invoked functions as arguments to other functions 
- control flow(if, else if, and else statements)  

build process in ZXAL directory:
- make
- ./zxal tests/file.z
- ./tests/file
- echo $?

If you find yourself running the program more than once:
- make clean
- make
- ./zxal tests/file.z
- ./tests/file
- echo $?


So assembly file and executable are in the same directory as the .z file you're compiling.

p.s., as you can see from iv), i dont have print function. Also, functions can have at most six arguments and error handling during typechecking sucks.
For example, if a function return type is not equivalent to the type of the operand of a return statement, assert is invoked.
