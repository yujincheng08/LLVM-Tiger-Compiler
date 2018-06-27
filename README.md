# Tiny Tiger
Compiler of Tiny Tiger using LLVM. 
For language sepeficication please refer to *Morden Compiler Implementation in C*.
## Introduction
Everything is well, including record, array and nested function.
Input or empty string comparision might have some problems to fix. (Therefore merge.tig is not working.)

## Usage
- Compile it with Qt.
- Run it
	```shell
	./TinyTiger
	```
- Input the tiger program, click on compile button. (Run is button not working.)
- On the console, it will output the AST tree and IR code if no syntax or sematic error. And `output.o` is generated.
- Note that if there should not be any generation error output before IR code. If so, report it as a bug with the tiger source code.
- Then link the tiger object file with runtime library.
	```shell
	clang++ output.o runtime.o
	```
	PLEASE USE C++ COMPILER (clang++ or g++ or others).
- Run the Tiger program
	```
	./a.out
	```

## Know Issue
- [ ] If syntax error occurs, you must restart the program. Problem might cause by Pipe or the stringstream(not being cleared after error) in yacc code.
- [ ] Merge.tig is not running. Might cause by empty string comparision?

## TODO
- [ ] Output IR code in the bottom widget in the main windows via pipe.
- [ ] Syntax error alert with lineno.
- [ ] Add type to AST visualization.
- [ ] Editor is not good enough.
- [ ] MIGHT automatically generate executive file rather than object file that has to be linked.
