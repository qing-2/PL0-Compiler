#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NRW        11     // number of reserved words
#define MAXNUMLEN  14     // maximum number of digits in numbers
#define NSYM       10     // maximum number of symbols in array ssym and csym
#define MAXIDLEN   10     // length of identifiers

char ch;         // last character read
int  sym;        // last symbol read

//对于DFA，增加了状态state和现态currState
enum state {
	START,INNUM,INID,INBECOMES,BECOMES,GTR,GEQ,NEQ,LES,LEQ,END,COMMENT
};
int currState = START;

char csym[NSYM + 1] = {
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';'
};

//关键字
char* word[NRW + 1] = {
	"", /* place holder */
	"begin", "call", "const", "do", "end","if",
	"odd", "procedure", "then", "var", "while"
};
//类别码
enum symtype {
	SYM_NULL,	SYM_IDENTIFIER,	SYM_NUMBER,	SYM_PLUS,	SYM_MINUS,	SYM_TIMES,	SYM_SLASH,	SYM_ODD,	SYM_EQU,	SYM_NEQ,	SYM_LES,	SYM_LEQ,	SYM_GTR,	SYM_GEQ,	SYM_LPAREN,	SYM_RPAREN,	SYM_COMMA,	SYM_SEMICOLON,	SYM_PERIOD,	SYM_BECOMES,    SYM_BEGIN,	SYM_END,	SYM_IF,	SYM_THEN,	SYM_WHILE,	SYM_DO,	SYM_CALL,	SYM_CONST,	SYM_VAR,	SYM_PROCEDURE
};
int wsym[NRW + 1] = {
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE
};
int ssym[NSYM + 1] = {//
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON
};
                                         
//报错信息（相比上一篇文章随意地增加了0、26）
char* err_msg[] =
{
/*  0 */    "Fatal Error:Unknown character.\n",
/*  1 */    "Found ':=' when expecting '='.",
/*  2 */    "There must be a number to follow '='.",
/*  3 */    "There must be an '=' to follow the identifier.",
/*  4 */    "There must be an identifier to follow 'const', 'var', or 'procedure'.",
/*  5 */    "Missing ',' or ';'.",
/*  6 */    "Incorrect procedure name.",
/*  7 */    "Statement expected.",
/*  8 */    "Follow the statement is an incorrect symbol.",
/*  9 */    "'.' expected.",
/* 10 */    "';' expected.",
/* 11 */    "Undeclared identifier.",
/* 12 */    "Illegal assignment.",
/* 13 */    "':=' expected.",
/* 14 */    "There must be an identifier to follow the 'call'.",
/* 15 */    "A constant or variable can not be called.",
/* 16 */    "'then' expected.",
/* 17 */    "';' or 'end' expected.",
/* 18 */    "'do' expected.",
/* 19 */    "Incorrect symbol.",
/* 20 */    "Relative operators expected.",
/* 21 */    "Procedure identifier can not be in an expression.",
/* 22 */    "Missing ')'.",
/* 23 */    "The symbol can not be followed by a factor.",
/* 24 */    "The symbol can not be as the beginning of an expression.",
/* 25 */    "The number is too great.",
/* 26 */    "The identifier is too long",
/* 27 */    "",
/* 28 */    "",
/* 29 */    "",
/* 30 */    "",
/* 31 */    "",
/* 32 */    "There are too many levels."
};