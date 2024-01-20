# thrvcc

## Description

A C compiler for the risc-v instruction set, with features being refined.

## Build

### Install Dependencies

base dev tools:

```bash
sudo pacman -S base-devel
```

riscv cross compilation tool chain:

```bash
# for tool chain dependencies, see https://github.com/riscv-collab/riscv-gnu-toolchain#prerequisites
curl https://mirror.iscas.ac.cn/riscv-toolchains/git/riscv-collab/riscv-gnu-toolchain.sh | bash
cd riscv-gnu-toolchain
mkdir build
cd build
../configure --prefix=$HOME/riscv
make -j$(nproc)
```

### Build with Makefile

```bash
cd thrvcc
# if cross compile
export RISCV=~/riscv
# if local riscv
export RISCV=
# binary
make thrvcc
# test
make test
```

## Usage

```bash
./thrvcc -o out.s in.c
./thrvcc -S in.c
./thrvcc in.c
./thrvcc --help
```

## TODO

### operator

arithmetic operator  

- [x] \+ - * / % ++ --  

relational operator

- [x] == != > < >= <=

logical operator

- [x] && || !

bitwise operator

- [x] & | ^ ~ << >>

assignment operator

- [x] = += -= *= /= %= <<= >>= &= ^= |=

pointer operator

- [x] & *

other

- [x] sizeof ?:

### keywords

- [ ] auto
- [x] break continue goto return
- [x] if else switch case default
- [x] do for while
- [ ] char✔ double✔ float✔ int✔ long✔ register short✔ void✔
- [x] enum struct typedef union
- [ ] const extern✔ signed✔ unsigned✔ static✔ volatile

c99

- [x] _Bool
- [ ] _Complex
- [ ] _Imaginary
- [ ] inline
- [ ] restrict

c11

- [x] _Alignas
- [x] _Alignof
- [ ] _Atomic
- [ ] _Generic
- [ ] _Noreturn
- [ ] _Static_assert
- [ ] _Thread_local

### features

- [x] local variable
- [x] branch statement
- [x] cyclic statement
- [x] pointer
- [x] function
- [x] one-dimensional and multi-dimensional arrays
- [x] global variable
- [x] string
- [x] escape character
- [x] comment
- [x] scope
- [x] struct
- [x] union
- [x] short long   long long
- [x] void
- [x] typedef
- [x] sizeof
- [x] type cast
- [x] enum
- [x] uncomplete array
- [x] const expr
- [x] initializer
- [x] extern
- [x] static
- [x] do while
- [x] signed unsigned
- [x] function pointer
- [x] separate out cc1  
  ......

## Authors and acknowledgment

Authors:  
&ensp;&ensp;TenHian  
Related Projects:  
&ensp;&ensp;[chibicc](https://github.com/rui314/chibicc) 

## License

MIT license