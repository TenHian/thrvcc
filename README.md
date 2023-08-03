# thrvcc

## Description

A C compiler for the risc-v instruction set, with features being refined.

## Build

### Install Dependencies

Archlinux

```bash
sudo pacman -S base-devel
sudo pacman -S riscv64-elf-binutils riscv64-elf-gcc riscv64-elf-gdb riscv64-elf-newlib
sudo pacman -S qemu-full
```

### Build with Makefile

```bash
# binary
make thrvcc
# test
make test
```

## Usage

```bash
./thrvcc -o out.s in.c
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
- [ ] do for✔ while✔
- [ ] char✔ double float int✔ long✔ register short✔ void✔
- [ ] enum struc✔t typedef✔ union✔
- [ ] const extern signed unsigned static volatile

c99

- [x] _Bool
- [ ] _Complex
- [ ] _Imaginary
- [ ] inline
- [ ] restrict

c11

- [ ] _Alignas
- [ ] _Alignof
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
- [ ] extern
- [ ] static
- [ ] do while
- [ ] signed unsigned
- [ ] function pointer
- [ ] separate out cc1

## Authors and acknowledgment

Authors:  
&ensp;&ensp;TenHian  
Related Projects:  
&ensp;&ensp;[chibicc](https://github.com/rui314/chibicc)  
&ensp;&ensp;[rvcc](https://github.com/sunshaoce/rvcc)  

## License

MIT license