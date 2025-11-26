# GAsm – Genetic Assembly

GAsm (Genetic Assembly) is a minimalistic, bytecode-based instruction set designed specifically for Genetic Programming (GP).
Its purpose is to represent programs as compact sequences of single-byte opcodes that can be easily mutated, crossed over, evolved, and executed.

Registers:
- A – Accumulator - hold float values, used in float operations
- P – Pointer - integer values, used in accessing P element of at array, Pointer is taken modulo size of the array.
- R – Register array - float array for variables
- I – Input array - float array for inputs and outputs


Each instruction is encoded as a single byte.
Comments are denoted by //.

# Instruction Set

## Data Movement

| Instruction | Opcode | Description    | C equivalent                             |
| ----------- | ------ | -----------------|------------------------- |
| MOV P, A    | 0x00   | Copy accumulator A into program pointer P. A is rounded to the whole number. | P = A |
| MOV A, P    | 0x01   | Load accumulator A with program pointer P. | A = P |
| MOV A, R    | 0x02   | Load accumulator A with register R\[P].        | A = R\[P]|
| MOV A, I    | 0x03   | Load accumulator A with input value I\[P].     | A = I\[P] |
| MOV R, A    | 0x04   | Store accumulator A into register R\[P].       | R\[P] = A |
| MOV I, A    | 0x05   | Store accumulator A into input slot I\[P].     | I\[P] = A |

## Arithmetic Using Register R

| Instruction | Opcode | Description  | C equivalent                   |
| ----------- | ------ | --------------|---------------- |
| ADD R       | 0x10   | Add R to A. | A += R\[P]                   |
| SUB R       | 0x11   | Subtract R from A. | A -= R\[P]            |
| DIV R       | 0x12   | Divide A by R (safe division). | A /= R\[P] |
| MUL R       | 0x13   | Multiply A by R. | A *= R\[P]              |
| SIN R       | 0x14   | Compute sin(R) and store in A. | A = sin(R\[P])|
| COS R       | 0x15   | Compute cos(R) and store in A. | A = cos(R\[P]) |
| EXP R       | 0x16   | Compute exp(R) and store in A. | A = exp(R\[P]) |


## Arithmetic Using Input I

| Instruction | Opcode | Description | C equivalent                   |
| ----------- | ------ | ------------ | ----------------- |
| ADD I       | 0x20   | Add input I to A. | A += I\[P]             |
| SUB I       | 0x21   | Subtract input I from A. | A -= I\[P]       |
| DIV I       | 0x22   | Divide A by input I. | A /= I\[P]          |
| MUL I       | 0x23   | Multiply A by input I. | A *= I\[P]         |
| SIN I       | 0x24   | Compute sin(I) and store in A. | A = sin(\[P]) |
| COS I       | 0x25   | Compute cos(I) and store in A. | A = cos(\[P]) |
| EXP I       | 0x26   | Compute exp(I) and store in A. | A = exp(\[P]) |


## Unary / System Instructions

| Instruction | Opcode | Description                                  | C equivalent |
| ----------- | ------ | -------------------------------------------- | ------------ |
| INC         | 0x30   | Increment P.                                 | P++ |
| DEC         | 0x31   | Decrement P.                                 | P-- | 
| RES         | 0x32   | Reset P to 0.                                | P = 0 |
| SET         | 0x33   | Assign A a deterministicly random constatnt. | A = rand(), with seed the same for every program |

## Loop Instructions

| Instruction | Opcode | Description                                         |C equivalent |
| ----------- | ------ | --------------------------------------------------- | --- |
| FOR         | 0x40   | Initialize loop: set P = 0, increment it with every step by 1 until reached input length. | for (P = 0; P < input_length; P++) {} |
| LOP A       | 0x41   | Loop until A is non-negative.          | while (A >= 0) {} |
| LOP P       | 0x42   | Loop until P is non-negative.                        | while (P >= 0) {} |


## Jump / Conditional Instructions

| Instruction | Opcode | Description                      | C equivalent |
| ----------- | ------ | -------------------------------- | --- |
| JMP I       | 0x50   | Jump if A is greater or equal than I\[P].           | if (A >= I\[P]) {} |
| JMP R       | 0x51   | Jump if A is greater or equal than I\[P].        | if (A >= R\[P]) {} |
| JMP P       | 0x52   | Jump if P is greater or equal 0. | if (P >= 0) {} |

## Random and Termination

| Instruction | Opcode | Description                  | C equivalent |
| ----------- | ------ | ---------------------------- | --- |
| END         | 0x62   | End block of code (loops and conditionals).       |
| rng         | 0x63   | Store a truly random value into A. |

## Example code

```
// I is empty, but it's length is the length of desired fibbonacci sequence
// Initialize Fibonacci base values
RES             // P = 0 (fib0)
MOV A, P        // A = P = 0
MOV I, A        // I0 = A = 0
INC             // P++, P = 1
MOV A, P        // A = 1 (fib1)
MOV I, A        // I1 = A = 1

// main loop
FOR             // P = 0
MOV A, I        // A = I[P] = 0
INC             // P++, P = 1
ADD I           // A += I[P], A = 0 + 1 = 1
INC             // P++, P = 2
MOV I, A        // I[P] = A = 1
END             // P = 1 (next value after 0), until P < input_length

// the for loop destroyed the first 2 values of the sequence
// beacuse the last 2 value went over the input_length so the looped
// to the beginning of the input
// this recovers the first 2 numbers in the sequence
RES             // P = 0 (fib0)
MOV A, P        // A = P = 0
MOV I, A        // I0 = A = 0
INC             // P++, P = 1
MOV A, P        // A = 1 (fib1)
MOV I, A        // I1 = A = 1

```
