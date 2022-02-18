#include "lexer.h"

void error(int n)
{
    printf("Error %3d: %s\n", n, err_msg[n]);
}

void lexer(FILE *fp)
{
    int num = 0;          //当前识别中的数字
    int k = 0;            //当前识别中的数字的长度
    char a[MAXIDLEN + 1]; //当前识别中的标识符or关键字
    int a_index = 0;      //当前识别中的标识符or关键字的下标

    ch = fgetc(fp); //获取文件第一个字符

    while (ch != EOF)
    {
        switch (currState)
        {
        case START:
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
            { //不能在switch外面忽略这些字符，在这里，他们不是无效的，他们可以表示标识符的结束等
            }
            else if (ch == '{')
            { //注释{}
                currState = COMMENT;
            }
            else if (isdigit(ch))
            {
                currState = INNUM;
                num = num * 10 + ch - '0';
                k++;
            }
            else if (isalpha(ch))
            {
                currState = INID;
                if (a_index > MAXIDLEN)
                {
                    error(26);
                    exit(1);
                }
                a[a_index] = ch;
                a_index++;
            }
            else if (ch == ':')
                currState = INBECOMES;
            else if (ch == '>')
                currState = GTR;
            else if (ch == '<')
                currState = LES;
            else
            { //单独字符直接识别
                currState = START;
                int i = 1;
                for (; i <= NSYM; i++)
                {
                    if (ch == csym[i])
                        break;
                }
                if (i <= NSYM)
                {
                    sym = ssym[i];
                    printf("(%d,%c)\n", sym, ch);
                }
                else
                {
                    error(0);
                    // exit(1);
                    printf("the char is ---%c---\n", ch);
                }
            }
            break;
        case INNUM:
            if (isdigit(ch))
            {
                num = num * 10 + ch - '0';
            }
            else
            { //token识别完毕
                currState = START;
                ch = ungetc(ch, fp); // 回退该字符，重新识别
                sym = SYM_NUMBER;
                if (k > MAXNUMLEN)
                    error(25);
                else
                {
                    printf("(%d,%d)\n", sym, num);
                }
                k = 0;
                num = 0;
            }
            break;
        case COMMENT:
            if (ch == '}')
            { // 注释结束
                currState = START;
            }
            break;
        case INID:
            if (isalpha(ch) || isdigit(ch))
            {
                if (a_index > MAXIDLEN)
                {
                    error(26);
                    exit(1);
                }
                a[a_index] = ch;
                a_index++;
            }
            else
            { //token识别完毕
                currState = START;
                ch = ungetc(ch, fp); // 回退该字符，重新识别
                a[a_index] = '\0';   // 字符数组和字符串的区别就是结尾少了\0，一定要加上！
                // 检查是否为关键字
                int i = 1;
                for (; i <= NRW; i++)
                {
                    if (strcmp(a, word[i]) == 0)
                        break;
                }
                if (i <= NRW)
                {
                    sym = wsym[i]; // symbol is a reserved word
                }
                else
                {
                    sym = SYM_IDENTIFIER; // symbol is an identifier
                }
                printf("(%d,%s)\n", sym, a);
                a_index = 0;
            }
            break;
        case INBECOMES:
            if (ch == '=')
            {
                currState = BECOMES;
            }
            else
            {
                currState = START;
                ch = ungetc(ch, fp); // 回退该字符，重新识别
                sym = SYM_NULL;
            }
            break;
        case GTR:
            if (ch == '=')
            {
                currState = GEQ;
            }
            else
            { //token识别完毕
                currState = START;
                ch = ungetc(ch, fp); // 回退该字符，重新识别
                sym = SYM_GTR;
                printf("(%d,>)\n", sym);
            }
            break;
        case LES:
            if (ch == '=')
            {
                currState = LEQ;
            }
            else
            { //token识别完毕
                currState = START;
                ch = ungetc(ch, fp); // 回退该字符，重新识别
                sym = SYM_LES;
                printf("(%d,<)\n", sym);
            }
            break;
        case BECOMES: //token识别完毕
            currState = START;
            ch = ungetc(ch, fp); // 回退该字符，重新识别
            sym = SYM_BECOMES;
            printf("(%d,:=)\n", sym);
            break;
        case GEQ: //token识别完毕
            currState = START;
            ch = ungetc(ch, fp); // 回退该字符，重新识别
            sym = SYM_GEQ;
            printf("%d,>=\n", sym);
            break;
        case LEQ: //token识别完毕
            currState = START;
            ch = ungetc(ch, fp); // 回退该字符，重新识别
            sym = SYM_LEQ;
            printf("%d,<=\n", sym);
            break;
        }

        //在最后获取下一个字符
        ch = fgetc(fp);
    }
    printf("END");
}

int main()
{
    //获取待检验文件的指针
    FILE *fp = fopen("source.txt", "r");
    if (!fp)
    {
        printf("File read failed");
    }
    else //将待检验文件放入词法分析器进行分析
        lexer(fp);
    return 0;
}