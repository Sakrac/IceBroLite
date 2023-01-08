# Expressions

In most fields accepting text input for numeric values natural expressions are allowed,
for instance in the Watch field or address fields. etc.

Certain [Ice Console](ice_console.MD) commands also accept expressions for values such as poke, eval, etc.

* [back to manual](index.md)

## Expression operators are:

* '==': 1 if left is same as right side otherwise 0. Can also use '='
* '<', '<=', '>', '>=": less than, less or equal, greater, greater or equal. 1 if true otherwise 0
* '(', ')': parenthesis can be used to scope parts of expressions
* '{', '}': 16 bit value at address within { and }, for example {$fffe} would be the current interrupt address
* '[', ']': 8 bit value at address within [ and ], for example [1] would be current ROM bank value
* '+': sum of left side and right side
* '-': left side subtracted by right side
* '*': product of left and right sides
* '/': left side divided by right side
* '&': bitwise and
* '&&': conditional and, if left side and right side are both non-zero this is 1 otherwise 0
* '|': bitwise or
* '||': conditional or
* '^': bitwise exclusive or
* '!': "not", if right side is 0 then this is 1, otherwise 0
* '$': prefix for hex values
* '%': prefix for octal values
* 'A', 'X', 'Y': value of register
* 'S': stack pointer
* 'C', 'Z', 'I', 'D', 'V', 'N': value of flag register, each is 0 or 1
* 'P': all above flags in one byte
