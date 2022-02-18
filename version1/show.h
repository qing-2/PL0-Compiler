#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NRW 11			 // number of reserved words
#define MAXNUMLEN 14	 // maximum number of digits in numbers
#define NSYM 10			 // maximum number of symbols in array ssym and csym
#define MAXIDLEN 10		 // length of identifiers
#define TXMAX 500		 // length of identifier table
#define MAXLEVEL 3		 // maximum depth of nesting block
#define MAXINS 8		 // maximum number of instructions
#define CXMAX 500		 // size of code array
#define MAXADDRESS 32767 // maximum address
#define STACKSIZE 200	 // maximum storage

char ch = ' '; // last character read 最新读取到的字符
FILE *fp;	   // 文件指针

// 以下三个全局变量，是记录当前读取到的单词，构造符号表
// 在getsym()中被赋值
int sym;			   // last symbol read
char id[MAXIDLEN + 1]; // last identifier read
int num;			   // last number read

// 以下变量便于报错时知道是哪一行哪一列出错的
// 在getch()中用的到
int cc = 0;	   // character count (当前已读的字符个数)
int ll = 0;	   // line length
int lc = 1;	   // line count
int err = 0;   //error count
char line[80]; //输入缓冲

int kk; //不知道啥用

// 定义类别码
enum symtype
{
	SYM_NULL,
	SYM_IDENTIFIER,
	SYM_NUMBER,
	SYM_PLUS,
	SYM_MINUS,
	SYM_TIMES,
	SYM_SLASH,
	SYM_ODD,
	SYM_EQU,
	SYM_NEQ,
	SYM_LES,
	SYM_LEQ,
	SYM_GTR,
	SYM_GEQ,
	SYM_LPAREN,
	SYM_RPAREN,
	SYM_COMMA,
	SYM_SEMICOLON,
	SYM_PERIOD,
	SYM_BECOMES,
	SYM_BEGIN,
	SYM_END,
	SYM_IF,
	SYM_THEN,
	SYM_WHILE,
	SYM_DO,
	SYM_CALL,
	SYM_CONST,
	SYM_VAR,
	SYM_PROCEDURE
};

// 定义关键字
char *word[NRW + 1] = {
	"", /* place holder */
	"begin", "call", "const", "do", "end", "if",
	"odd", "procedure", "then", "var", "while"};
// 关键字对应的类别码
int wsym[NRW + 1] = {
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE};

// 定义运算符和界符
char csym[NSYM + 1] = {
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';'};
// 运算符和界符对应的类别码
int ssym[NSYM + 1] = { //
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON};

// 定义标识符的类别码，这个是在符号表的kind中使用的码
enum idtype
{
	ID_CONSTANT,
	ID_VARIABLE,
	ID_PROCEDURE
};

// PL/0语言的目标代码的指令
char *mnemonic[MAXINS] = {
	"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC"};
// 指令的功能码
enum opcode
{
	LIT,
	OPR,
	LOD,
	STO,
	CAL,
	INT,
	JMP,
	JPC
};
// 指令的操作码
enum oprcode
{
	OPR_RET,
	OPR_NEG,
	OPR_ADD,
	OPR_MIN,
	OPR_MUL,
	OPR_DIV,
	OPR_ODD,
	OPR_EQU,
	OPR_NEQ,
	OPR_LES,
	OPR_LEQ,
	OPR_GTR,
	OPR_GEQ
};

// 对常量声明语句的信息进行存储
typedef struct
{
	char name[MAXIDLEN + 1];
	int kind;
	int value;
} comtab;

// 对变量和过程声明语句的信息进行存储
typedef struct
{
	char name[MAXIDLEN + 1]; // 变量名、过程名
	int kind;	   // 变量、过程
	short level;   // 嵌套级别/层次差
	short address; // 存储位置的相对地址
} mask;
comtab table[TXMAX]; // 符号表table[500]，存储常量、变量、 过程

// 目标代码code将会对执行语句的信息进行转换和存储
typedef struct
{
	int f; // function code
	int l; // level difference
	int a; // displacement address
} instruction;
instruction code[CXMAX];                 

int level = 0; // 嵌套级别。函数可以嵌套，主程序是0层，在主程序中定义的过程是1层，最多三层
int cx = 0;	   // index of current instruction to be generated.
int tx = 0;	   // index of table
int dx = 0;	   // data allocation index

//报错信息（相比上一篇文章随意地增加了0、26）
char *err_msg[] =
	{
		/*  0 */ "Fatal Error:Unknown character.\n",
		/*  1 */ "Found ':=' when expecting '='.",
		/*  2 */ "There must be a number to follow '='.",
		/*  3 */ "There must be an '=' to follow the identifier.",
		/*  4 */ "There must be an identifier to follow 'const', 'var', or 'procedure'.",
		/*  5 */ "Missing ',' or ';'.",
		/*  6 */ "Incorrect procedure name.",
		/*  7 */ "Statement expected.",
		/*  8 */ "Follow the statement is an incorrect symbol.",
		/*  9 */ "'.' expected.",
		/* 10 */ "';' expected.",
		/* 11 */ "Undeclared identifier.",
		/* 12 */ "Illegal assignment.",
		/* 13 */ "':=' expected.",
		/* 14 */ "There must be an identifier to follow the 'call'.",
		/* 15 */ "A constant or variable can not be called.",
		/* 16 */ "'then' expected.",
		/* 17 */ "';' or 'end' expected.",
		/* 18 */ "'do' expected.",
		/* 19 */ "Incorrect symbol.",
		/* 20 */ "Relative operators expected.",
		/* 21 */ "Procedure identifier can not be in an expression.",
		/* 22 */ "Missing ')'.",
		/* 23 */ "The symbol can not be followed by a factor.",
		/* 24 */ "The symbol can not be as the beginning of an expression.",
		/* 25 */ "The number is too great.",
		/* 26 */ "The identifier is too long",
		/* 27 */ "",
		/* 28 */ "",
		/* 29 */ "",
		/* 30 */ "",
		/* 31 */ "",
		/* 32 */ "There are too many levels."};