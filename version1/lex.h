void error(int n)
{
    err++;
    for (int i = 0; i < ll; i++)
    {
        printf("%c", line[i]);
    }
    printf("\n");

    for (int i = 1; i <= cc - 1; i++)
    {
        printf(" ");
    }
    printf("^\n");

    printf("Error %3d: %s\n", n, err_msg[n]);
}

// 从源代码读入一行到缓冲line，每次从line中读取一个字符
void getch()
{
    if (cc == ll) //如果缓冲line读完，再读入一行，更新line ll cc
    {
        ll = cc = 0;
        if (feof(fp)) //检测文件结束符
        {
            printf("\nPROGRAM INCOMPLETE\n");
            exit(1);
        }

        while (!feof(fp) && (ch = getc(fp)) != '\n') //将新的一行字符存入缓冲line
        {
            line[++ll] = ch;
        }
        lc++;
        line[++ll] = ' '; //用空格来代替回车，表示原文档里字符之间的分隔！！！调用此函数时不会有\n了
    }
    ch = line[++cc];
}

// 词法分析，读取一个单词
void getsym()
{
    while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '{')
    {
        if (ch == '{')
        { //忽略注释
            int end = 1;
            while (end)
            {
                getch();
                if (ch == '}')
                    end = 0;
            }
        }
        getch();
    }
    if (ch == EOF)
    {
        exit(0);
    }

    if (isalpha(ch)) // 当前输入为字母,则应该为关键字或标识符
    {
        char a[MAXIDLEN + 1]; // 当前读取到的单词
        int k = 0;
        for (; (isalpha(ch) || isdigit(ch)) && k < MAXIDLEN; k++)
        {
            a[k] = ch;
            getch();
        }

        a[k] = '\0';   // 字符数组和字符串的区别就是结尾少了\0，一定要加上！
        strcpy(id, a); // 保存到全局变量id中，语法分析要用

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
    }
    else if (isdigit(ch))
    { // symbol is a number.
        sym = SYM_NUMBER;
        int k = num = 0;
        while (isdigit(ch))
        {
            num = num * 10 + ch - '0';
            getch();
            k++;
        }
        if (k > MAXNUMLEN)
            error(25); // The number is too great.
    }
    else if (ch == ':')
    {
        getch();
        if (ch == '=')
        {
            sym = SYM_BECOMES; // :=
            getch();
        }
        else
        {
            sym = SYM_NULL; // illegal?
        }
    }
    else if (ch == '>')
    {
        getch();
        if (ch == '=')
        {
            sym = SYM_GEQ; // >=
            getch();
        }
        else
        {
            sym = SYM_GTR; // >
        }
    }
    else if (ch == '<')
    {
        getch();
        if (ch == '=')
        {
            sym = SYM_LEQ; // <=
            getch();
        }
        else if (ch == '>')
        {
            sym = SYM_NEQ; // <>
            getch();
        }
        else
        {
            sym = SYM_LES; // <
        }
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
            getch();
        }
        //不应该出现的字符
        else
        {
            error(0);
            sym = SYM_NULL;
        }
    }
}
