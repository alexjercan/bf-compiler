# BrainFuck Compiler in C

BrainFuck to Assebly compiler written in C. It uses nasm assembler to compile
to machine code.

I recommend trying out some of the examples from <http://brainfuck.org/>

### Quickstart

```console
gcc main.c -o main
./main -f examples/src/03-hello-world.bf -o example.asm
nasm -felf64 -g example.asm -o example.o && ld example.o -o example
./example
```

### Calculator Example

To get the calculator use the `examples/src/05-adder.bf` file.

```brainfuck
- marker
>,--------------------------------[>>>>>>>>,--------------------------------] read
- marker
< move on last digit +[-<+]- go back to marker 0 > move on first digit
+[- ---------------- >>>>>>>>+]- make all digits between 0 and 9
< move on last digit +[-<+]- go back to marker 0
>>,----------[>>>>>>>>,----------] read
- marker <
< move on last digit +[-<+]- go back to marker 0 >> move on first digit
+[- -------------------------------------- >>>>>>>>+]- make all digits between 0 and 9
< move on last marker
< move on last digit +[-<+]- go back to marker 0
> we are on the first number here

+[->+]- go last marker < move on last digit

+[-
    <<<<<<<

    # add two numbers
    >>>>>>>>>>>> [- <<<<<<<<<<<< + >>>>>>>>>>>>] <<<<<<<<<<<< add the carry
    [->+<]> > ++++++++++ > + << put 10 and 1 for divmod

    # Divmod Algorithm
    # n d 1 0 0 0
    [->-[>+>>]>[[-<+>]+>+>>]<<<<<]
    # 0 0 d_n%d n%d1 n/d 0 0

    # set modulo to actually n%d
    >>-

    # go to the last digit of the next number
    <<<<

    # check if we find a marker
    +
]-

# show the carry which will also be like zero in case no numbers
>>>>>
[++++++++++++++++++++++++++++++++++++++++++++++++ . [-]]

# go to start of the first number
<<<<
+[-
    >>> go to the quot

    ++++++++++++++++++++++++++++++++++++++++++++++++ .

    >>>>> go to the start of next number
    +
]-
```
