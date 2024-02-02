,>,, read the inputs
< go back on the first cell
------------------------------------------------ convert first cell to digit
> go to second number
------------------------------------------------ convert second number to digit
< go back to first cell
[ if the first cell is zero we completed the addition
->+< move one from first to second cell
]
> move to the actual sum

modulo 10 operation for max 19
[->>+<<[->>+<<[->>+<<[->>+<<[->>+<<[->>+<<[->>+<<[->>+<<[->>+<<[->>+<< move 10 from cell to reminder
    >>----------
    <+
    <
    [->>+<<[->>+<<[->>+<<[->>+<<[->>+<<[->>+<<[->>+<<[->>+<<[->>+<<[->>+<<]]]]]]]]]] move 10 from cell to reminder
]]]]]]]]]]

printing time
> go on the first digit (the carry)
[
++++++++++++++++++++++++++++++++++++++++++++++++ . convert digit to int and print
[-]
]
> go to the next digit
++++++++++++++++++++++++++++++++++++++++++++++++ . convert digit to int and print
