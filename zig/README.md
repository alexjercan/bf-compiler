# BrainFuck Compiler in Zig

BrainFuck to Assebly compiler written in zig. It uses nasm assembler to compile
to machine code.

### Quickstart

```console
```
```console
zig build run -- -f ../examples/src/03-hello-world.bf -o example.asm
nasm -felf64 -g example.asm -o example.o && ld example.o -o example
./example
```
