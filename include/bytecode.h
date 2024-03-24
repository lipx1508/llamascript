#ifndef LLAMA_BYTECODE_H
#define LLAMA_BYTECODE_H

#include <cstdint>
#include <cstddef>

/* 

Quick reference sheet:
    [ti] -> t = instruction argument, i = index (unsigned integer)
    [si] -> s = stack, i = access (signed integer)
    [a]  -> a = current address
    |O|  -> O = instruction name

Follows a C-like syntax for what it does

*/

#define LLAMA_OP_NOP    0x00 // Does nothing
#define LLAMA_OP_JP     0x01 // Sets [a] to [t+0]
#define LLAMA_OP_JZ     0x02 // Sets [a] to [t+0] if [s-1] is false
#define LLAMA_OP_JNZ    0x03 // Sets [a] to [t+0] if [s-1] is false
#define LLAMA_OP_BLOCK  0x05 // Execute scope
#define LLAMA_OP_IF     0x06 // Execute scope if [s-1] is true
#define LLAMA_OP_ELSE   0x07 // Pops the current scope and starts a new one
#define LLAMA_OP_LOOP   0x08 // Execute scope forever
#define LLAMA_OP_REPEAT 0x09 // Repeats the current scope
#define LLAMA_OP_BREAK  0x0a // Breaks out the current scope
#define LLAMA_OP_FORIN  0x0b // Iterates over an array 
#define LLAMA_OP_END    0x0f // Pops the current scope

#define LLAMA_OP_PUSHNULL   0x10 // Pushes a null value to the stack
#define LLAMA_OP_PUSHTRUE   0x11 // Pushes a boolean with the value
#define LLAMA_OP_PUSHFALSE  0x12
#define LLAMA_OP_PUSHINT    0x13
#define LLAMA_OP_PUSHFLOAT  0x14 // Pushes a floating point
#define LLAMA_OP_PUSHSTRING 0x15 // Pushes a string [t+0] to the stack
#define LLAMA_OP_PUSHLIST   0x16 // Pushes an empty list to the stack
#define LLAMA_OP_PUSHOBJECT 0x17 // Pushes an object of type [t+0] to the stack
#define LLAMA_OP_PUSHDYN    0x18 // Pushes a dynamic value to the stack
#define LLAMA_OP_PUSHFUNC   0x1e // Pushes a function to the stack

#define LLAMA_OP_SETGLOBAL   0x20
#define LLAMA_OP_GETGLOBAL   0x21
#define LLAMA_OP_SETPROPERTY 0x22
#define LLAMA_OP_GETPROPERTY 0x23
#define LLAMA_OP_SETINDEX    0x24
#define LLAMA_OP_GETINDEX    0x25
#define LLAMA_OP_NEWGLOBAL   0x26
#define LLAMA_OP_NEWLOCAL    0x27
#define LLAMA_OP_POP         0x2e
#define LLAMA_OP_POPN        0x2f

#define LLAMA_OP_ADD     0x30 // Pushes the addition of [s-2] and [s-1]
#define LLAMA_OP_SUB     0x31 // Pushes the subtraction of [s-2] by [s-1]
#define LLAMA_OP_MUL     0x32 // Pushes the multiplication of [s-2] by [s-1]
#define LLAMA_OP_DIV     0x33 // Pushes the division of [s-2] by [s-1]
#define LLAMA_OP_MOD     0x34 // Pushes the remainder of the division of [s-2] by [s-1]
#define LLAMA_OP_POW     0x35 // Pushes [s-1] to the power of [s-2]
#define LLAMA_OP_NEGATE  0x36 // Negates [s-1]
#define LLAMA_OP_PROMOTE 0x37 // TODO: check if unary plus operator is actually needed
#define LLAMA_OP_BITNOT  0x38 // Pushes the bitwise NOT of [s-1]
#define LLAMA_OP_BITAND  0x39 // Pushes the bitwise AND of [s-2] by [s-1]
#define LLAMA_OP_BITOR   0x3a // Pushes the bitwise OR of [s-2] by [s-1]
#define LLAMA_OP_BITXOR  0x3b // Pushes the bitwise XOR of [s-2] by [s-1]
#define LLAMA_OP_BITSHL  0x3c // Pushes the bitwise shift of [s-2] to the left by [s-1] offset
#define LLAMA_OP_BITSHR  0x3d // Pushes the bitwise shift of [s-2] to the right by [s-1] offset
#define LLAMA_OP_BITROL  0x3e // Pushes the bitwise rotate of [s-2] to the left by [s-1] offset
#define LLAMA_OP_BITROR  0x3f // Pushes the bitwise rotate of [s-2] to the right by [s-1] offset

#define LLAMA_OP_NOT 0x40 // Size of ([s-1]) in bytes
#define LLAMA_OP_AND 0x41
#define LLAMA_OP_OR  0x42
#define LLAMA_OP_EQ  0x43
#define LLAMA_OP_LT  0x44
#define LLAMA_OP_LE  0x45
#define LLAMA_OP_GT  0x46
#define LLAMA_OP_GE  0x47
#define LLAMA_OP_NE  0x48

#define LLAMA_OP_SIZEOF     0x50 // Size of ([s-1]) in bytes
#define LLAMA_OP_LENOF      0x51 // Length of ([s-1])
#define LLAMA_OP_TYPEOF     0x52 // Type of ([s-1]) as a string
#define LLAMA_OP_INSTANCEOF 0x53 // If ([s-2]) is inherit from ([s-1])
#define LLAMA_OP_THIS       0x54 // Push a reference to the current object being accessed
#define LLAMA_OP_AS         0x55 // Convert ([s-2]) to the type of the string ([s-1])

#define LLAMA_OP_CALL    0x60 // Calls a function with ([t+0]) arguments avaliable on the stack
#define LLAMA_OP_CALLV   0x61 // Same as |CALL| but with void return
#define LLAMA_OP_RETURN  0x65 // Returns ([s-1]) from a function
#define LLAMA_OP_RETURNV 0x66 // Returns void from a function

#define LLAMA_OP_REF         0x70 // TODO: plan the implementation of this
#define LLAMA_OP_REFGLOBAL   0x71 // Pushes a reference to a global
#define LLAMA_OP_REFPROPERTY 0x72 // Pushes a reference to a property
#define LLAMA_OP_REFINDEX    0x73 // Pushes a reference to an index
#define LLAMA_OP_REFSET      0x78 // Sets the value of the reference ([s-2]) to ([s-1])

#define LLAMA_OP_BREAKPOINT 0x80
#define LLAMA_OP_TYPECHECK  0x81

#define LLAMA_OPFLAG_STACKARG (1 << 0) // Uses stack indexes as arguments
#define LLAMA_OPFLAG_CONSTARG (1 << 1) // Uses constant pool indexes as arguments
#define LLAMA_OPFLAG_IMMUTARG (1 << 2) // Uses plain, immutable arguments
#define LLAMA_OPFLAG_ISBLOCK  (1 << 3) // Creates a new scope
#define LLAMA_OPFLAG_ISEND    (1 << 4) // Pops a scope
#define LLAMA_OPFLAG_ISTRAP   (1 << 5) // Creates a trap

#define GET_OP(__name)   (LLAMA_OP_ ## __name)
#define GET_FLAG(__name) (LLAMA_OPFLAG_ ## __name)

#endif