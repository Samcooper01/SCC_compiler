# High-Level Constructs Reference

This document outlines the high-level constructs for a custom assembly-like language, detailing memory management, variable declarations, expressions, control structures, and comments.

## High-Level Constructs

### Memory Management
- These need to go before the CODE_BEGIN section


```
PROG_MEMORY_START (addr in decimal)
```

Description: Specifies the starting address for program memory.
Example: ```PROG_MEMORY_START 0```

```
PROG_MEMORY_END (addr in decimal)
```
Description: Specifies the ending address for program memory.
Example: ```PROG_MEMORY_END 1000```

```
DATA_MEMORY_START (addr in decimal)
```

Description: Specifies the starting address for data memory.
Example: ```DATA_MEMORY_START 1000```
```
DATA_MEMORY_END (addr in decimal)
```

Description: Specifies the ending address for data memory.
Example: ```DATA_MEMORY_END 2000```

## Variables
```
var name = initial value
```
- Description: Declares a variable with an initial decimal value.
- Initial Value: Must be a decimal number.
- Name Restrictions: Variable names cannot consist solely of digits.
- Value Cap: Values are limited to 2^16 bits.

Example:
```
var counter = 10
var maxValue = 65535
```

## Expressions
(name) = (name or constant) (operator) (name or constant)

- Description: Assigns a result of an operation between variables or constants to a variable.
- Possible Operators: + - * /

Example:
```
total = value1 + value2
result = counter * 2
```

## Control Structures
```
loop <amount>
{
// Instructions
}
```
- Description: Repeats the enclosed instructions a specified number of times.
- Amount: If the amount is -1, the loop runs indefinitely. Otherwise, it iterates based on the value in the DR register.

Example:
```
loop 5
{
total = total + 1
}
```

## If Statement

```
if(<var_name> == <value>)
{
// Instructions
}
```

- Description: Executes the enclosed instructions if the specified variable equals the given value.
- Example:
```
if(counter == 0)
{
total = 0
}
```

## Comments

Syntax: ```//COMMENT```

- Description: Used to add comments to the code for documentation purposes.

Example:
```
// This is a comment
var temp = 5 
```
