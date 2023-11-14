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

> Attention!!!
> 
> If you want to compile this project from source code, you should read the Makefile carefully. In addition, there are the following points to note:
> 
> 1. I developed this program on ArchLinux x86_64, and ArchLinux provides a comprehensive package, so I was able to get the riscv cross-compilation toolchain directly from pacman. If you can't use pacman, make sure there are no problems with your cross-compilation toolchain.
> 
> 2. Check the following snippet in main.c:  
>    
>    ```c
>    // [attention]
>    // if you are cross-compiling, change this path to the path corresponding to ricsv toolchain
>    // must be an absulte path
>    // else leave it empty
>    static char *RVPath = "/usr";
>    ```
>    
>    If you cross-compile, make sure RVPath is your riscv toolchain path, such as "RVPath/bin/riscv64-elf-as". If you compile on RISC-V machine, *RVPath = "".

```bash
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