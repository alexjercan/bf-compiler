# BrainFuck Compiler in C

BrainFuck to Assebly compiler written in C. It uses nasm assembler to compile
to machine code.

I recommend trying out some of the examples from <http://brainfuck.org/>

### Quickstart

```console
gcc main.c -o main
./main -f examples/src/hello-world.bf -o example.asm
nasm -felf64 -g example.asm -o example.o && ld example.o -o example
./example
```
