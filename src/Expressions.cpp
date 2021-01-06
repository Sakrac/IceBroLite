#include <inttypes.h>
#include "6510.h"
#include "Sym.h"

// These are expression tokens in order of precedence (last is highest precedence)

enum ExpOp {
	EO_END,
	EO_NONE = EO_END,

	// values
	EO_VAL8,			// fixed value, 8 bits
	EO_VAL16,			// fixed value, 16 bits
	EO_PC,				// current PC
	EO_A,				// accumulator
	EO_X,				// x reg
	EO_Y,				// y reg
	EO_S,				// stack index
	EO_C,				// carry
	EO_Z,				// zero
	EO_I,				// interrupt
	EO_D,				// decimal
	EO_V,				// overflow
	EO_N,				// negative
	EO_FL,				// flags as one byte
	EO_BYTE,			// read byte from memory
	EO_2BYTE,			// read 2 bytes from memory

	// operators
	EO_EQU,				// 1 if left equal to right otherwise 0
	EO_LT,				// 1 if left less than right otherwise 0
	EO_GT,				// 1 if left greater than right otherwise 0
	EO_LTE,				// 1 if left less than or equal to right otherwise 0
	EO_GTE,				// 1 if left greater than or equal to right otherwise 0
	EO_CND,				// &&
	EO_COR,				// ||
	EO_LBR,				// left bracket
	EO_RBR,				// right bracket
	EO_LBC,				// left brace
	EO_RBC,				// right brace
	EO_LPR,				// left parenthesis
	EO_RPR,				// right parenthesis
	EO_ADD,				// +
	EO_SUB,				// -
	EO_MUL,				// *
	EO_DIV,				// /
	EO_AND,				// &
	EO_OR,				// |
	EO_EOR,				// ^
	EO_SHL,				// <<
	EO_SHR,				// >>
	EO_SGN8,			// s8
	EO_SGN16,			// s16
	EO_NOT,				// !
	EO_NEG,				// negate value
	EO_ERR,				// Error

	EO_VALUES = EO_VAL8,
	EO_OPER = EO_EQU

};

typedef const char* ExpStr;

static bool IsAlphabetic(const char c) {
	return (c>='A' && c<='Z') || (c>='a' && c<='z');
}

static bool IsNumeric(const char c) {
	return c>='0' && c<='9';
}

static bool IsAlphaNumeric(const char c) {
	return IsNumeric(c) || IsAlphabetic(c);
}

static bool IsHex(const char c) {
	return IsNumeric(c) || (c>='A' && c<='F') || (c>='a' && c<='f');
}

static uint32_t GetHex(const char c) {
	return IsNumeric(c) ? (c-'0') : ((c>='A' && c<='F') ? (c-'A'+10) : (c-'a'+10));
}

ExpStr SkipWS(ExpStr str) {
	while (*str && *str <= ' ')
		str++;
	return str;
}

uint32_t GetHex(ExpStr &str) {
	uint32_t ret = 0;
	while (IsHex(*str))
		ret = (ret<<4) | GetHex(*str++);
	return ret;
}

uint32_t GetDec(ExpStr &str) {
	uint32_t ret = 0;
	while (IsNumeric(*str))
		ret = ret*10 + (*str++ - '0');
	return ret;
}

uint32_t GetBin(ExpStr &str) {
	uint32_t ret = 0;
	while (*str=='0' || *str=='1')
		ret = ret*2 + (*str++ - '0');
	return ret;
}

char ToUp(char c) {
	if (c>='a' && c<='z')
		return c + 'A' - 'a';
	return c;
}


ExpOp ParseOp(ExpStr &str, uint32_t &v)
{
	str = SkipWS(str);
	switch (char c = *str++) {
		case 0:	return EO_NONE; // end of operation string
		case '=': if (*str=='=') ++str; return EO_EQU;	// = or == are both acceptable equal
		case '<': if (*str=='=') { ++str; return EO_LTE; } else if (str[1]=='<') {	++str; return EO_SHL; } return EO_LT;
		case '>': if (*str=='=') { ++str; return EO_GTE; } else if (str[1]=='>') { ++str; return EO_SHR; } return EO_GT;
		case '(': return EO_LPR;
		case ')': return EO_RPR;
		case '{': return EO_LBC;
		case '}': return EO_RBC;
		case '[': return EO_LBR;
		case ']': return EO_RBR;
		case '+': return EO_ADD;
		case '-': return EO_SUB; // may be negate value => sort out in caller
		case '*': return EO_MUL;
		case '/': return EO_DIV;
		case '&': if (*str=='&') { ++str; return EO_CND; } return EO_AND;
		case '|': if (*str=='|') { ++str; return EO_COR; } return EO_OR;
		case '^': return EO_EOR;
		case '!': return EO_NOT;
		case '$': v = GetHex(str); return v<0x100 ? EO_VAL8 : EO_VAL16;
		case '%': v = GetBin(str); return v<0x100 ? EO_VAL8 : EO_VAL16;
		default:
			if (IsNumeric(c)) {
				v = GetDec(--str);
				return v<0x100 ? EO_VAL8 : EO_VAL16;
			}
			if (IsAlphabetic(c)) {
				char C = ToUp(c);
				if (C=='F' && ToUp(*str)=='L' && !IsAlphaNumeric(str[1])) {
					++str;
					return EO_FL;
				} else if (C=='S' && *str=='8' && !IsAlphaNumeric(str[1])) {
					++str;
					return EO_SGN8;
				} else if (C=='S' && *str=='1' && str[1]=='6' && !IsAlphaNumeric(str[1])) {
					str += 2;
					return EO_SGN16;
				} else if (!IsAlphaNumeric(*str)) {
					switch (C) {
						case 'A': return EO_A;
						case 'X': return EO_X;
						case 'Y': return EO_Y;
						case 'S': return EO_S;
						case 'C': return EO_C;
						case 'Z': return EO_Z;
						case 'I': return EO_I;
						case 'D': return EO_D;
						case 'V': return EO_V;
						case 'N': return EO_N;
						case 'P': return EO_FL;
					}
				} else if ((c=='P' || c=='p') && (*str=='C' || *str=='c') && !IsAlphaNumeric(str[1])) {
					++str;
					return EO_PC;
				}
			}

			if (IsAlphabetic(c) || c=='.' || c=='_') {
				size_t lablen = 0;
				const char *scan = str-1;
				while (*scan && (*scan=='.' || *scan=='_' || IsAlphaNumeric(*scan))) {
					lablen++;
					scan++;
				}
				uint16_t addr;
				if (GetAddress(str-1, lablen, addr)) {
					v = addr;
					str += lablen-1;
					return EO_VAL16;
				}
			}
			break;
	}
	return EO_ERR;
}

#define MAX_EXPR_VALUES 32
#define MAX_EXPR_STACK 32
uint32_t BuildExpression(const char *Expr, uint8_t *ops, uint32_t max_ops)
{
	ExpOp stack[MAX_EXPR_STACK];
	int num_values = 0;
	uint32_t num_ops = 0;
	int sp = 0;
	ExpOp op = EO_NONE, prev_op = EO_NONE;

	while (num_values<MAX_EXPR_VALUES && num_ops<max_ops && sp<MAX_EXPR_STACK) {
		uint32_t v;
		op = ParseOp(Expr, v);
		if (op == EO_NONE || op == EO_ERR)
			break;
		if (op == EO_SUB && prev_op>=EO_OPER && prev_op != EO_RPR && prev_op!=EO_RBR && prev_op!=EO_RBC )
			op = EO_NEG;
		if (op < EO_OPER) {
			ops[num_ops++] = op;
			if (op == EO_VAL8 || op == EO_VAL16) {
				ops[num_ops++] = (uint8_t)v;
				if (op == EO_VAL16)
					ops[num_ops++] = (uint8_t)(v>>8);
			}
		} else if (op == EO_LPR || op == EO_LBR || op == EO_LBC)
			stack[sp++] = op;
		else if (op == EO_RPR || op == EO_RBR || op == EO_RBC) {
			ExpOp opo = op == EO_RPR ? EO_LPR : (op == EO_RBC ? EO_LBC : EO_LBR);
			while (sp && stack[sp-1]!=opo)
				ops[num_ops++] = stack[--sp];
			if (!sp || stack[sp-1]!=opo) {
				op = EO_ERR;
				break;
			}
			sp--; // skip open paren
			if (op==EO_RBR)
				ops[num_ops++] = EO_BYTE;
			else if (op == EO_RBC)
				ops[num_ops++] = EO_2BYTE;
		} else {
			while (sp) {
				ExpOp p = (ExpOp)stack[sp-1];
				if (p==EO_LPR || p==EO_LBR || p==EO_LBC || op>p)
					break;
				ops[num_ops++] = p;
				sp--;
			}
			stack[sp++] = op;
		}
		prev_op = op;
	}
	if (op == EO_NONE) {
		while (sp)
			ops[num_ops++] = stack[--sp];
	}
	ops[num_ops++] = EO_END;
	return num_ops;
}

#define MAX_EXPR_VALUE_DEPTH 32
int EvalExpression(const uint8_t *RPN)
{
	int values[MAX_EXPR_VALUE_DEPTH];
	int i = 0;
	CPU6510* cpu = GetCurrCPU();
	const CPU6510::Regs &r = cpu->regs;
	bool err = false;

	while (!err && *RPN) {
		uint8_t c = *RPN++;
		switch (c) {
			case EO_VAL8: values[i++] = *RPN++; break;	// fixed value: 8 bits
			case EO_VAL16: values[i++] = RPN[0] + (((int)RPN[1])<<8); RPN += 2; break;	// fixed value: 16 bits
			case EO_PC: values[i++] = r.PC; break;	// current PC
			case EO_A: values[i++] = r.A; break; // accumulator
			case EO_X: values[i++] = r.X; break; // x reg
			case EO_Y: values[i++] = r.Y; break; // y reg
			case EO_S: values[i++] = r.SP; break; // stack index
			case EO_C: values[i++] = (r.FL&F_C) ? 1 : 0; break; // carry
			case EO_Z: values[i++] = (r.FL&F_Z) ? 1 : 0; break; // zero
			case EO_I: values[i++] = (r.FL&F_I) ? 1 : 0; break; // interrupt
			case EO_D: values[i++] = (r.FL&F_D) ? 1 : 0; break; // decimal
			case EO_V: values[i++] = (r.FL&F_V) ? 1 : 0; break; // overflow
			case EO_N: values[i++] = (r.FL&F_N) ? 1 : 0; break; // negative
			case EO_FL: values[i++] = r.FL; break;
			case EO_BYTE:			// read byte from memory
				if (!(err = i<1))
					values[i-1] = cpu->GetByte((uint16_t)values[i-1]);
				break;
			case EO_2BYTE:			// read byte from memory
				if (!(err = i<1))
					values[i-1] = cpu->GetByte((uint16_t)values[i-1]) +
						((uint16_t)cpu->GetByte(uint16_t(values[i-1]+1))<<8);
				break;
			case EO_EQU:				// 1 if left equal to right otherwise 0
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] == values[i];
				}
				break;
			case EO_LT:				// 1 if left less than right otherwise 0
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] < values[i];
				}
				break;
			case EO_GT:				// 1 if left greater than right otherwise 0
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] > values[i];
				}
				break;
			case EO_LTE:				// 1 if left less than or equal to right otherwise 0
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] <= values[i];
				}
				break;
			case EO_GTE:				// 1 if left greater than or equal to right otherwise 0
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] >= values[i];
				}
				break;
			case EO_CND:				// &&
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] && values[i];
				}
				break;
			case EO_COR:				// ||
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] || values[i];
				}
				break;
			case EO_ADD:				// +
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] + values[i];
				}
				break;
			case EO_SUB:				// -
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] - values[i];
				}
				break;
			case EO_MUL:				// *
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] * values[i];
				}
				break;
			case EO_DIV:				// /
				if (!(err = (i<2 || values[i-1]==0))) {
					i--;
					values[i-1] = values[i-1] / values[i];
				}
				break;
			case EO_AND:				// &
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] & values[i];
				}
				break;
			case EO_OR:				// |
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] | values[i];
				}
				break;
			case EO_EOR:				// ^
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] ^ values[i];
				}
				break;
			case EO_SHL:				// <<
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] << values[i];
				}
				break;
			case EO_SHR:				// >>
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] >> values[i];
				}
				break;
			case EO_NEG:				// negate value
				if (!(err = i<1))
					values[i-1] = -values[i-1];
				break;
			case EO_NOT:
				if (!(err = i<1))
					values[i-1] = !values[i-1];
				break;
			case EO_SGN8:
				if (!(err = i<1))
					values[i-1] = (int)(int8_t)values[i-1];
				break;
			case EO_SGN16:
				if (!(err = i<1))
					values[i-1] = (int)(int16_t)values[i-1];
				break;
			default:
				err = true;
				break;
		}
	}
	return (err || i!=1) ? 0 : values[0];
}

int ValueFromExpression( const char* exp )
{
	uint8_t ops[128];
	BuildExpression(exp, ops, sizeof(ops));
	return EvalExpression( ops );
}
