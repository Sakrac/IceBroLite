#pragma once

uint32_t BuildExpression(const char *Expr, uint8_t *ops, uint32_t max_ops);
int EvalExpression(const uint8_t *RPN);
int ValueFromExpression( const char* exp );
