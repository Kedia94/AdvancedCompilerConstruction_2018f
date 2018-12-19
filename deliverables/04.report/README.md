# Advance Compiler Construction

Team 5

Member

- Wonjae Jang
- Dongwan Kang



## How to Test

Test target: testname.mod

1. Build snuplc by commane 'make' on source/snuplc directory

2. Go to source/test

3. Run command 'make testname'

4. Following ouput files are generated

   - Original SnuPLc outputs

     - testname
     - testname.mod.s

     - testname.mod.ast
     - testname.mod.ast.dot
     - testname.mod.ast.dot.pdf
     - testname.mod.tac
     - testname.mod.tac.dot
     - testname.mod.tac.dot.pdf

   - Our optimized compiler outputs

     - testname_opt
     - testname_opt.s
     - testname_opt.tac
     - testname_opt.tac.dot
     - testname_opt.tac.dot.pdf

5. Compare 'testname' and 'testname_opt'



## Optimization Option

We implemented constant propagation, dead code elimination, and function inlining. 

There is option to enable or disable each optimization. 

Following code is extracted from snuplc.cpp.

To disable each optimization, please delete a define of selected optimization.

```cpp
 52 #define CONST
 53 #define DEAD
 54 #define INLINE

```


