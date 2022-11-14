#include "show.h"
#include "lex.h"

// 将标识符填入符号表 (enter object(constant, variable or procedre) into table.)
// 从下标 1 开始
void enter(int kind)
{
    mask *mk;
    tx++;
    strcpy(table[tx].name, id);
    table[tx].kind = kind;

    switch (kind)
    {
    case ID_CONSTANT:
        if (num > MAXADDRESS)
        {
            error(25); // The number is too great.
            num = 0;
        }
        table[tx].value = num;
        break;
    case ID_VARIABLE:
        mk = (mask *)&table[tx];
        mk->level = level;
        mk->address = dx++;
        break;
    case ID_PROCEDURE:
        mk = (mask *)&table[tx];
        mk->level = level;
        mk->address = cx;   // method 1
        break;
    }
}

// 在符号表table中查找id (locates identifier in symbol table.)
int position(char *id)
{
    int i;
    strcpy(table[0].name, id);
    i = tx + 1;
    while (strcmp(table[--i].name, id) != 0)
        ;
    return i;
}

// <常量定义>::=<标识符>=<无符号整数>
void constdeclaration()
{
    if (sym == SYM_IDENTIFIER)
    { //全局变量id中存有已识别的标识符
        getsym();
        if (sym == SYM_EQU || sym == SYM_BECOMES)
        {
            if (sym == SYM_BECOMES)
                error(1); // Found ':=' when expecting '='.
            getsym();
            if (sym == SYM_NUMBER)
            {
                enter(ID_CONSTANT); // 将标识符填入符号表
                getsym();
            }
            else
            {
                error(2); // There must be a number to follow '='.
            }
        }
        else
        {
            error(3); // There must be an '=' to follow the identifier.
        }
    }
    else
    {
        error(4); // There must be an identifier to follow 'const'.
    }
}

// <标识符>
void vardeclaration(void)
{
    if (sym == SYM_IDENTIFIER)
    {
        enter(ID_VARIABLE); // 将变量填入符号表
        getsym();
    }
    else
    {
        error(4); // There must be an identifier to follow 'var'.
    }
}

// 生成汇编指令 (generates (assembles) an instruction.)
void gen(int x, int y, int z)
{ //生成汇编指令
    if (cx > CXMAX)
    { // cx > 500
        printf("Fatal Error: Program too long.\n");
        exit(1);
    }
    // printf("gen code[%d] f=%s l=%d a=%d\n",cx,mnemonic[x],y,z);
    code[cx].f = x;
    code[cx].l = y;
    code[cx++].a = z;
}

// 因子，把待运算的数放到栈中
// 把项和因子独立开处理解决了加减号与乘除号的优先级问题
void factor()
{
    void expression();
    int i;
    if (sym == SYM_IDENTIFIER)
    {
        if ((i = position(id)) == 0)
        {
            error(11); // Undeclared identifier.
        }
        else
        {
            mask *mk;
            switch (table[i].kind)
            {
            case ID_CONSTANT:
                gen(LIT, 0, table[i].value); // 把常数放到栈顶
                break;
            case ID_VARIABLE:
                mk = (mask *)&table[i];
                gen(LOD, level - mk->level, mk->address); // 把变量放到栈顶
                break;
            case ID_PROCEDURE:
                error(21); // Procedure identifier cannot be in an expression.
                break;
            }
        }
        getsym();
    }
    else if (sym == SYM_NUMBER)
    {
        if (num > MAXADDRESS)
        {
            error(25); // The number is too great.
            num = 0;
        }
        gen(LIT, 0, num); // 把常数放到栈顶
        getsym();
    }
    else if (sym == SYM_LPAREN) // (
    {
        getsym();
        expression();          // 递归调用表达式
        if (sym == SYM_RPAREN) // )
        {
            getsym();
        }
        else
        {
            error(22); // Missing ')'.
        }
    }
}

// 项（乘除运算）
void term()
{
    int mulop;
    factor();
    while (sym == SYM_TIMES || sym == SYM_SLASH)
    {
        mulop = sym; // 记录下当前运算符
        getsym();
        factor();
        if (mulop == SYM_TIMES)
        {
            gen(OPR, 0, OPR_MUL); // 将栈顶和次栈顶进行运算
        }
        else
        {
            gen(OPR, 0, OPR_DIV);
        }
    }
}

// <表达式>::=[+|-]〈项〉{〈加法运算符〉〈项〉}
void expression()
{
    int addop;
    if (sym == SYM_PLUS || sym == SYM_MINUS)
    {
        addop = sym;
        getsym();
        factor();
        if (addop == SYM_MINUS)
        {
            gen(OPR, 0, OPR_NEG);
        }
        term();
    }
    else
    {
        term();
    }
    while (sym == SYM_PLUS || sym == SYM_MINUS)
    {
        addop = sym;
        getsym();
        term();
        if (addop == SYM_PLUS)
        {
            gen(OPR, 0, OPR_ADD);
        }
        else
        {
            gen(OPR, 0, OPR_MIN);
        }
    }
}

// <条件>
void condition()
{
    int relop;
    if (sym == SYM_ODD) // 单目运算符odd，判断后面的表达式是否为奇数
    {
        getsym();
        expression();
        gen(OPR, 0, OPR_ODD);
    }
    else
    {
        expression();
        relop = sym; // 记录下当前运算符
        getsym();
        expression();
        switch (relop)
        { // 根据比较关系生成指令
        case SYM_EQU: //==
            gen(OPR, 0, OPR_EQU);
            break;
        case SYM_NEQ: //!=
            gen(OPR, 0, OPR_NEQ);
            break;
        case SYM_LES: //<
            gen(OPR, 0, OPR_LES);
            break;
        case SYM_LEQ: //<=
            gen(OPR, 0, OPR_LEQ);
            break;
        case SYM_GTR: //>
            gen(OPR, 0, OPR_GTR);
            break;
        case SYM_GEQ: //>=
            gen(OPR, 0, OPR_GEQ);
            break;
        default:
            error(20);
        }
    }
}

// <语句>::= <赋值语句> | <条件语句> | <过程调用语句> | <复合语句> | null
void statement()
{
    int i = 0, savedCx, savedCx_;

    if (sym == SYM_IDENTIFIER)
    { // <变量赋值语句>--><标识符>:=<表达式>
        if ((i = position(id)) == 0)
        {
            error(11); // Undeclared identifier.
        }
        else if (table[i].kind != ID_VARIABLE)
        {
            error(12); // Illegal assignment.
        }
        getsym();
        if (sym == SYM_BECOMES) // :=
        {
            getsym();
        }
        else
        {
            error(13); // ':=' expected.
        }
        expression(); // 算出赋值号右部表达式的值
        mask *mk;
        mk = (mask *)&table[i];
        if (i)
        {   
            gen(STO, level - mk->level, mk->address); // 将栈顶内容存到刚刚识别到的变量里
        }
    }
    else if (sym == SYM_CALL)
    {
        getsym();
        if (sym != SYM_IDENTIFIER)
        {
            error(14); // There must be an identifier to follow the 'call'.
        }
        else
        {
            if (!(i = position(id)))
            {
                error(11); // Undeclared identifier.
            }
            else if (table[i].kind == ID_PROCEDURE)
            {
                mask *mk;
                mk = (mask *)&table[i];
                gen(CAL, level - mk->level, mk->address);
            }
            else
            {
                error(15); // A constant or variable can not be called.
            }
            getsym();
        }
    }
    else if (sym == SYM_IF)
    {
        getsym();
        condition();
        if (sym == SYM_THEN)
        {
            getsym();
        }
        else
        {
            error(16); // 'then' expected.
        }
        savedCx = cx;
        gen(JPC, 0, 0);   // 条件转移指令，栈顶为非真时跳转到a
        statement();      // 递归调用
        code[savedCx].a = cx; // 设置刚刚那个条件转移指令的跳转位置
    }
    else if (sym == SYM_BEGIN)
    { //复合语句，顺序执行begin和end之间的语句就好
        getsym();
        statement(); // 递归调用
        while (sym == SYM_SEMICOLON)
        {
            if (sym == SYM_SEMICOLON)
            {
                getsym();
            }
            else
            {
                error(10);
            }
            statement();
        }
        if (sym == SYM_END)
        {
            getsym();
        }
        else
        {
            error(17); // ';' or 'end' expected.
        }
    }
    else if (sym == SYM_WHILE)
    {
        getsym();
        savedCx = cx;   // while循环的开始位置
        condition();    // 处理while里的条件
        savedCx_ = cx;  // while的do后的语句后的语句块的开始位置
        gen(JPC, 0, 0); // 条件转移指令，转移位置暂时不知

        if (sym == SYM_DO)
        {
            getsym();
        }
        else
        {
            error(18); // 'do' expected.
        }
        statement();          // 分析do后的语句块
        gen(JMP, 0, savedCx); // 无条件转移指令，跳转到cx1，再次进行逻辑判断
        code[savedCx_].a = cx; // 填写刚才那个条件转移指令的跳转位置，while循环结束
    }
}

void listcode(int from, int to)
{
    printf("listcode:\n");
    for (int i = from; i < to; i++)
    {
        printf("%5d %s\t%d\t%d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
    }
    printf("\n");
}

int base(int stack[], int currentLevel, int levelDiff)
{
    int b = currentLevel;

    while (levelDiff--)
        b = stack[b];
    return b;
}

void interpret()
{
    int pc = 0;           // program counter
    int stack[STACKSIZE]; // 假想栈
    int top = 0;          // top of stack
    int b = 1;
    instruction i; // instruction register

    printf("Begin executing PL/0 program.\n");
    
    stack[1] = stack[2] = stack[3] = 0;
    do
    {
        printf("%d_", pc);
        i = code[pc++];
        switch (i.f)
        {
        case LIT:
            stack[++top] = i.a;
            break;
        case OPR:
            switch (i.a) // operator
            {
            case OPR_RET:
                top = b - 1;
                pc = stack[top + 3];
                b = stack[top + 2];
                break;
            case OPR_NEG:
                stack[top] = -stack[top];
                break;
            case OPR_ADD:
                top--;
                stack[top] += stack[top + 1];
                break;
            case OPR_MIN:
                top--;
                stack[top] -= stack[top + 1];
                break;
            case OPR_MUL:
                top--;
                stack[top] *= stack[top + 1];
                break;
            case OPR_DIV:
                top--;
                if (stack[top + 1] == 0)
                {
                    fprintf(stderr, "Runtime Error: Divided by zero.\n");
                    fprintf(stderr, "Program terminated.\n");
                    continue;
                }
                stack[top] /= stack[top + 1];
                break;
            case OPR_ODD:
                stack[top] %= 2;
                break;
            case OPR_EQU:
                top--;
                stack[top] = stack[top] == stack[top + 1];
                break;
            case OPR_NEQ:
                top--;
                stack[top] = stack[top] != stack[top + 1];
            case OPR_LES:
                top--;
                stack[top] = stack[top] < stack[top + 1];
                break;
            case OPR_GEQ:
                top--;
                stack[top] = stack[top] >= stack[top + 1];
            case OPR_GTR:
                top--;
                stack[top] = stack[top] > stack[top + 1];
                break;
            case OPR_LEQ:
                top--;
                stack[top] = stack[top] <= stack[top + 1];
            }
            break;
        case LOD:
            stack[++top] = stack[base(stack, b, i.l) + i.a];
            break;
        case STO:
            stack[base(stack, b, i.l) + i.a] = stack[top];
            top--;
            break;
        case CAL:
            stack[top + 1] = base(stack, b, i.l); //静态链（定义关系中的上一层基址）
            stack[top + 2] = b;                   //动态链（执行顺序中的上一层基址）
            stack[top + 3] = pc;                  //返回地址（需要执行的下一条指令地址）
            b = top + 1;                          //当前栈帧的基址
            pc = i.a;
            break;
        case INT:
            top += i.a;
            break;
        case JMP:
            pc = i.a;
            break;
        case JPC:
            if (stack[top] == 0)
                pc = i.a;
            top--;
            break;
        }
    } while (pc);

    printf("\nEnd executing PL/0 program.\n");
}

// <分程序>:=[<常量说明部分>][<变量说明部分>][<过程说明部分>]<语句>
// 一遍扫描，语法分析、语义分析、目标代码生成 一起完成
void block()
{
    //后续变量定义主要用于代码生成
    int savedDx;
    int savedTx;
    int savedCx = cx;
    dx = 3; // 分配3个单元供运行期间存放静态链SL、动态链DL和返回地址RA
    printf("%d.gen JMP\n",savedCx);
    gen(JMP, 0, 0); // 跳转到分程序的开始位置，由于当前还没有知道在何处开始，所以jmp的目标暂时填为0
    // mask *mk = (mask *)&table[tx]; 
    // mk->address = cx; // 记录刚刚在符号表中记录的过程的address，cx是下一条指令的地址

    while (1)
    {
        if (level > MAXLEVEL)
        {
            error(32); // There are too many levels.
        }
        // <常量声明部分>:=const<常量定义>{,<常量定义>};
        if (sym == SYM_CONST)
        {
            getsym();
            if (sym == SYM_IDENTIFIER)
            {
                constdeclaration();
                while (sym == SYM_COMMA) //循环处理id1=num1,id2=num2,……
                {
                    getsym();
                    constdeclaration();
                }
                //常量定义结束
                if (sym == SYM_SEMICOLON)
                {
                    getsym();
                }
                else
                {
                    error(5); // Missing ',' or ';'.
                }
            }
        }
        // <变量声明部分>:=var<变量定义>{,<变量定义>};
        else if (sym == SYM_VAR)
        {
            getsym();
            if (sym == SYM_IDENTIFIER)
            {
                vardeclaration();
                while (sym == SYM_COMMA)
                {
                    getsym();
                    vardeclaration();
                }
                if (sym == SYM_SEMICOLON)
                {
                    getsym();
                }
                else
                {
                    error(5); // Missing ',' or ';'.
                }
            }
        }
        // <过程声明部分>:=procedure<标识符><分程序>;{<过程声明部分>}
        else if (sym == SYM_PROCEDURE)
        {
            getsym();
            if (sym == SYM_IDENTIFIER)
            {
                enter(ID_PROCEDURE);
                getsym();
            }
            else
            {
                error(4); // There must be an identifier to follow 'procedure'.
            }
            if (sym == SYM_SEMICOLON)
            {
                getsym();
            }
            else
            {
                error(5); // Missing ',' or ';'.
            }

            level++;
            savedTx = tx;
            savedDx = dx;

            block(); // 递归调用block，因为根据BNF可知每个procedure都是一个分程序

            tx = savedTx; // 恢复tx,因为里层的符号表对于外层是没用的，局部变量的作用范围！！！
            level--;
            dx = savedDx;

            if (sym == SYM_SEMICOLON)
            {
                getsym();
            }
            else
            {
                error(5); // Missing ',' or ';'.
            }
        }
        else if (sym == SYM_NULL)
        {
            // getsym(); // 出错的情况下跳过出错的符号
            // 暂时不进行容错处理，遇到错误直接报错
            error(27);
            exit(1);
        }
        else
        {
            break;
        }
    }
    // 后续部分主要用于代码生成
    code[savedCx].a = cx; // 这时cx正好指向语句的开始位置，这个位置正是前面的 jmp 指令需要跳转到的位置
    printf("%d.JMP to %d\n",savedCx, cx);
    // mk->address = cx;
    gen(INT, 0, dx); // 为主程序在运行栈中开辟数据区，开辟 dx 个空间，作为这个分程序的第1条指令
    statement(); // 语句
    gen(OPR, 0, OPR_RET); // 从分程序返回（对于 0 层的程序来说，就是程序运行完成，退出）
}

int main()
{
    fp = fopen("../pl0_code/demo2.txt", "r");
    if (!fp)
    {
        printf("File dosen't exist");
        exit(1);
    }
    
    // <程序>:=<分程序>.
    getsym();
    block();
    if (sym != SYM_PERIOD)
        error(9); // '.' expected.

    if (err)
        printf("There are %d error(s) in PL/0 program.\n", err);
    else
        interpret();
    listcode(0, cx); // 输出目标代码
    return 0;
}
