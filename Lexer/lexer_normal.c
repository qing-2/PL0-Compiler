#include "lexer.h"

void error(int n)
{
	printf("Error %3d: %s\n", n, err_msg[n]);
}

void lexer(FILE *fp)
{
	ch = fgetc(fp);
	while (ch != EOF)
	{
		while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
		{
			ch = fgetc(fp);
		}
		if (isalpha(ch)) // 当前输入为字母,则应该为关键字或标识符
		{
			char a[MAXIDLEN + 1]; // 当前读取到的单词
			int k = 0;
			for (; (isalpha(ch) || isdigit(ch)) && k < MAXIDLEN; k++)
			{
				a[k] = ch;
				ch = fgetc(fp);
			}

			a[k] = '\0'; // 字符数组和字符串的区别就是结尾少了\0，一定要加上！
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
		}
		else if (isdigit(ch))
		{ // symbol is a number.
			sym = SYM_NUMBER;
			int k = 0;
			int num = 0;
			while (isdigit(ch))
			{
				num = num * 10 + ch - '0';
				ch = fgetc(fp);
				k++;
			}
			if (k > MAXNUMLEN)
				error(25); // The number is too great.
			else
			{
				printf("(%d,%d)\n", sym, num);
			}
		}
		else if (ch == ':')
		{
			ch = fgetc(fp);
			if (ch == '=')
			{
				sym = SYM_BECOMES; // :=
				ch = fgetc(fp);
				printf("(%d,:=)\n", sym);
			}
			else
			{
				sym = SYM_NULL; // illegal?
			}
		}
		else if (ch == '>')
		{
			ch = fgetc(fp);
			if (ch == '=')
			{
				sym = SYM_GEQ; // >=
				ch = fgetc(fp);
				printf("(%d,>=)\n", sym);
			}
			else
			{
				sym = SYM_GTR; // >
				printf("(%d,=)\n", sym);
			}
		}
		else if (ch == '<')
		{
			ch = fgetc(fp);
			if (ch == '=')
			{
				sym = SYM_LEQ; // <=
				ch = fgetc(fp);
				printf("(%d,<=)\n", sym);
			}
			else if (ch == '>')
			{
				sym = SYM_NEQ; // <>
				ch = fgetc(fp);
			}
			else
			{
				sym = SYM_LES; // <
				printf("(%d,<)\n", sym);
			}
		}
		else if (ch == '{')
		{ //忽略注释
			int end = 1;
			while (end)
			{
				ch = fgetc(fp);
				if (ch == '}')
					end = 0;
			}
			ch = fgetc(fp);
		}
		else
		{ // other tokens : '+', '-', '*', '/', '(', ')', '=', ',', '.', ';'
			//代码和识别关键字那里类似
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
				ch = fgetc(fp);
			}
			//不应该出现的字符
			else
			{
				printf("Fatal Error: Unknown character.\n");
				exit(1);
			}
		}
	}
	printf("END");
}

int main()
{
	FILE *fp = fopen("source.txt", "r");
	lexer(fp);
	return 0;
}