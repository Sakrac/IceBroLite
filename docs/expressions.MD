# Expressions

Expressions make it easy to work with live CPU state directly in the debugger. In watch fields, address fields and several console commands, you can type natural expressions that are evaluated against the current CPU state whenever the emulator is paused.

Certain [Ice Console](ice_console.MD) commands also accept expressions, including `poke` and `eval`.

* [back to manual](index.md)

## Supported operators

* `==` or `=`: compare equality; returns `1` when true and `0` when false
* `<`, `<=`, `>`, `>=`: comparison operators
* `(` and `)`: grouping
* `{` and `}`: read a 16-bit value from memory at the enclosed address, for example `{$fffe}` reads the IRQ vector
* `[` and `]`: read an 8-bit value from the enclosed address, for example `[1]` reads the current ROM bank value
* `+`, `-`, `*`, `/`: arithmetic operators
* `&`, `|`, `^`: bitwise operators
* `&&` and `||`: logical operators
* `!`: logical negation
* `$`: hexadecimal prefix
* `%`: octal prefix
* `A`, `X`, `Y`: CPU registers
* `S`: stack pointer
* `C`, `Z`, `I`, `D`, `V`, `N`: CPU flags
* `P`: all flags packed into one byte
