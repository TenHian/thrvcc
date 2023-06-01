#!/bin/bash

# cat <<EOF | riscv64-elf-gcc -xc -c -o tmp2.o -
cat <<EOF | riscv64-elf-gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF

assert() {
  expected="$1"
  input="$2"

  ./thrvcc "$input" > tmp.s || exit
  riscv64-elf-gcc -static -o tmp tmp.s tmp2.o
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
assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
# [2] support "+, -" operation
echo [2]
assert 34 'int main() { return 12-34+56; }'
# [3] support whitespaces
echo [3]
assert 41 'int main() { return  12 + 34 - 5 ; }'
# [5] support * / ()
echo [5]
assert 47 'int main() { return 5+6*7; }'
assert 15 'int main() { return 5*(9-6); }'
assert 17 'int main() { return 1-8/(2*2)+3*6; }'
# [6] support unary operators like + -
echo [6]
assert 10 'int main() { return -10+20; }'
assert 10 'int main() { return - -10; }'
assert 10 'int main() { return - - +10; }'
assert 48 'int main() { return ------12*+++++----++++++++++4; }'

# [7] Support Conditional Operators
echo [7]
assert 0 'int main() { return 0==1; }'
assert 1 'int main() { return 42==42; }'
assert 1 'int main() { return 0!=1; }'
assert 0 'int main() { return 42!=42; }'
assert 1 'int main() { return 0<1; }'
assert 0 'int main() { return 1<1; }'
assert 0 'int main() { return 2<1; }'
assert 1 'int main() { return 0<=1; }'
assert 1 'int main() { return 1<=1; }'
assert 0 'int main() { return 2<=1; }'
assert 1 'int main() { return 1>0; }'
assert 0 'int main() { return 1>1; }'
assert 0 'int main() { return 1>2; }'
assert 1 'int main() { return 1>=0; }'
assert 1 'int main() { return 1>=1; }'
assert 0 'int main() { return 1>=2; }'
assert 1 'int main() { return 5==2+3; }'
assert 0 'int main() { return 6==4+3; }'
assert 1 'int main() { return 0*9+5*2==4+4*(6/3)-2; }'

# [8] Support ";" split statements
echo [8]
assert 3 'int main() { 1; 2;return  3; }'
assert 12 'int main() { 12+23;12+99/3;return 78-66; }'

# [9] Support single-letter var
echo [9]
assert 3 'int main() { int a=3;return a; }'
assert 8 'int main() { int a=3,z=5;return a+z; }'
assert 6 'int main() { int a,b; a=b=3;return a+b; }'
assert 5 'int main() { int a=3, b=4, a=1;return a+b;}'

# [10] Support multi-letter var
echo [10]
assert 3 'int main() { int foo=3;return foo; }'
assert 74 'int main() { int foo2=70; int bar4=4;return foo2+bar4; }'

# [11] support return
echo [11]
assert 1 'int main() { return 1; 2; 3; }'
assert 2 'int main() { 1; return 2; 3; }'
assert 3 'int main() { 1; 2; return 3; }'

# [12] Support {...}
echo [12]
assert 3 'int main() { {1; {2;} return 3;} }'

# [13] Support empty statements
echo [13]
assert 5 'int main() { ;;; return 5; }'

# [14] Support "if" statement
echo [14]
assert 3 'int main() { if (0) return 2; return 3; }'
assert 3 'int main() { if (1-1) return 2; return 3; }'
assert 2 'int main() { if (1) return 2; return 3; }'
assert 2 'int main() { if (2-1) return 2; return 3; }'
assert 4 'int main() { if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 'int main() { if (1) { 1; 2; return 3; } else { return 4; } }'

# [15] Support "for" statement
echo [15]
assert 55 'int main() { int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 'int main() { for (;;) {return 3;} return 5; }'

# [16] Support "while" statement
echo [16]
assert 10 'int main() { int i=0; while(i<10) { i=i+1; } return i; }'

# [19] Support unary "&" "*" pointer operators
echo [19]
assert 3 'int main() { int x=3; return *&x; }'
assert 3 'int main() { int x=3; int *y=&x; int **z=&y; return **z; }'
assert 5 'int main() { int x=3; int *y=&x; *y=5; return x; }'

# [20] Support for arithmetic operations on pointers
echo [20]
assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'
assert 7 'int main() { int x=3; int y=5; *(&y-1)=7; return x; }'
assert 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'

# [21] Support for the "int" keyword to define variables
echo [21]
assert 8 'int main() { int x, y; x=3; y=5; return x+y; }'
assert 8 'int main() { int x=3, y=5; return x+y; }'

# [22] Support for zero-parameter function calls
echo [22]
assert 3 'int main() { return ret3(); }'
assert 5 'int main() { return ret5(); }'
assert 8 'int main() { return ret3()+ret5(); }'

# [23] Support function calls with up to 6 parameters
echo [23]
assert 8 'int main() { return add(3, 5); }'
assert 2 'int main() { return sub(5, 3); }'
assert 21 'int main() { return add6(1,2,3,4,5,6); }'
assert 66 'int main() { return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
assert 136 'int main() { return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'

# [24] Support for zero-parameter function definitions
echo [24]
assert 32 'int main() { return ret32(); } int ret32() { return 32; }'

# [25] Supports function definitions with up to 6 parameters
echo [25]
assert 7 'int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }'
assert 1 'int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }'
assert 3 'int main() { return getmax(2,3); } int getmax(int x, int y) { if (x>y) return x; else return y; }'
assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'

# [26] Support for one-dimensional arrays
echo [26]
assert 3 'int main() { int x[2]; int *y=&x; *y=3; return *x; }'
assert 3 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }'
assert 4 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }'
assert 5 'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }'

# if all fine, echo OK
echo OK
