#!/bin/bash

assert() {
  expected="$1"
  input="$2"

  ./thrvcc "$input" > tmp.s || exit
  riscv64-elf-gcc -static -o tmp tmp.s
  qemu-riscv64 tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

# [1] return the input val
echo [1]
assert 0 '{ return 0; }'
assert 42 '{ return 42; }'
# [2] support "+, -" operation
echo [2]
assert 34 '{ return 12-34+56; }'
# [3] support whitespaces
echo [3]
assert 41 '{ return  12 + 34 - 5 ; }'
# [5] support * / ()
echo [5]
assert 47 '{ return 5+6*7; }'
assert 15 '{ return 5*(9-6); }'
assert 17 '{ return 1-8/(2*2)+3*6; }'
# [6] support unary operators like + -
echo [6]
assert 10 '{ return -10+20; }'
assert 10 '{ return - -10; }'
assert 10 '{ return - - +10; }'
assert 48 '{ return ------12*+++++----++++++++++4; }'

# [7] Support Conditional Operators
echo [7]
assert 0 '{ return 0==1; }'
assert 1 '{ return 42==42; }'
assert 1 '{ return 0!=1; }'
assert 0 '{ return 42!=42; }'
assert 1 '{ return 0<1; }'
assert 0 '{ return 1<1; }'
assert 0 '{ return 2<1; }'
assert 1 '{ return 0<=1; }'
assert 1 '{ return 1<=1; }'
assert 0 '{ return 2<=1; }'
assert 1 '{ return 1>0; }'
assert 0 '{ return 1>1; }'
assert 0 '{ return 1>2; }'
assert 1 '{ return 1>=0; }'
assert 1 '{ return 1>=1; }'
assert 0 '{ return 1>=2; }'
assert 1 '{ return 5==2+3; }'
assert 0 '{ return 6==4+3; }'
assert 1 '{ return 0*9+5*2==4+4*(6/3)-2; }'

# [8] Support ";" split statements
echo [8]
assert 3 '{ 1; 2;return  3; }'
assert 12 '{ 12+23;12+99/3;return 78-66; }'

# [9] Support single-letter var
echo [9]
assert 3 '{ int a=3;return a; }'
assert 8 '{ int a=3,z=5;return a+z; }'
assert 6 '{ int a,b; a=b=3;return a+b; }'
assert 5 '{ int a=3, b=4, a=1;return a+b;}'

# [10] Support multi-letter var
echo [10]
assert 3 '{ int foo=3;return foo; }'
assert 74 '{ int foo2=70; int bar4=4;return foo2+bar4; }'

# [11] support return
echo [11]
assert 1 '{ return 1; 2; 3; }'
assert 2 '{ 1; return 2; 3; }'
assert 3 '{ 1; 2; return 3; }'

# [12] Support {...}
echo [12]
assert 3 '{ {1; {2;} return 3;} }'

# [13] Support empty statements
echo [13]
assert 5 '{ ;;; return 5; }'

# [14] Support "if" statement
echo [14]
assert 3 '{ if (0) return 2; return 3; }'
assert 3 '{ if (1-1) return 2; return 3; }'
assert 2 '{ if (1) return 2; return 3; }'
assert 2 '{ if (2-1) return 2; return 3; }'
assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'

# [15] Support "for" statement
echo [15]
assert 55 '{ int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 '{ for (;;) {return 3;} return 5; }'

# [16] Support "while" statement
echo [16]
assert 10 '{ int i=0; while(i<10) { i=i+1; } return i; }'

# [19] Support unary "&" "*" pointer operators
echo [19]
assert 3 '{ int x=3; return *&x; }'
assert 3 '{ int x=3; int *y=&x; int **z=&y; return **z; }'
assert 5 '{ int x=3; int *y=&x; *y=5; return x; }'

# [20] Support for arithmetic operations on pointers
echo [20]
assert 3 '{ int x=3; int y=5; return *(&y-1); }'
assert 5 '{ int x=3; int y=5; return *(&x+1); }'
assert 7 '{ int x=3; int y=5; *(&y-1)=7; return x; }'
assert 7 '{ int x=3; int y=5; *(&x+1)=7; return y; }'

# [21] Support for the "int" keyword to define variables
echo [21]
assert 8 '{ int x, y; x=3; y=5; return x+y; }'
assert 8 '{ int x=3, y=5; return x+y; }'

# if all fine, echo OK
echo OK
