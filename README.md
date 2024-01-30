# BrainFuck Compiler in C

BrainFuck to Assebly compiler written in C. It uses nasm assembler to compile
to machine code.

### Quickstart

```console
gcc main.c -o main
./main -f examples/hello-world.bf -o example.asm
nasm -felf64 -g example.asm -o example.o && ld example.o -o example
./example
```
