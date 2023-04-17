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
assert 0 0
assert 42 42
# [2] support "+, -" operation
echo [2]
assert 34 '12-34+56'
# [3] support whitespaces
echo [3]
assert 41 ' 12 + 34 - 5 '
# [5] support * / ()
echo [5]
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 17 '1-8/(2*2)+3*6'
# [6] support unary operators like + -
echo [6]
assert 10 '-10+20'
assert 10 '- -10'
assert 10 '- - +10'
assert 48 '------12*+++++----++++++++++4'

# if all fine, echo OK
echo OK