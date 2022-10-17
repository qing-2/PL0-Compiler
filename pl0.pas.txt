program pl0(input,output)); (* PL/0编译程序与代码生成解释运行程序 *)
(* PL/0 compiler with code generation *)
label 99; (* 声明出错跳转标记 *)
const (* 常量定义 *)
  norw = 11;     (* of reserved words *) (* 保留字的个数 *)
  txmax = 100;   (* length of identifier table *) (* 标识符表的长度（容量） *)
  nmax = 14;     (* max number of digits in numbers *) (* 数字允许的最长位数 *)
  al = 10;       (* length of identifiers *) (* 标识符最长长度 *)
  amax = 2047;   (* maximum address *) (* 寻址空间 *)
  levmax = 3;    (* max depth of block nesting *) (* 最大允许的块嵌套层数 *)
  cxmax = 200;   (* size of code array *) (* 类PCODE目标代码数组长度（可容纳代码行数） *)
type (* 类型定义 *)
  symbol = (nul, ident, number, plus, minus, times, slash, oddsym,
            eql, neq, lss, leq, gtr, geq, lparen, rparen, comma,
            semicolon, period, becomes, beginsym, endsym, ifsym,
            thensym, whilesym, writesym, readsym, dosym, callsym,
            constsym, varsym, procsym); (* symobl类型标识了不同类型的词汇 *)
  alfa = packed array[1..al] of char; (* alfa类型用于标识符 *)
  object1 = (constant, variable, procedur); (* object1为三种标识符的类型 *)
 
  symset = set of symbol; (* symset是symbol类型的一个集合类型，可用于存放一组symbol *)
  fct = (lit, opr, lod, sto, cal, int, jmp, jpc); (* fct类型分别标识类PCODE的各条指令 *)
  instruction = packed record
    f: fct;       (* function code *)
    l: 0..levmax; (* level *)
    a: 0..amax;   (* displacement addr *)
  end; (* 类PCODE指令类型，包含三个字段：指令f、层差l和另一个操作数a *)
  (*
     lit 0, a  load constant a
     opr 0, a  execute opr a
     lod l, a  load variable l, a
     sto l, a  store variable l, a
     cal l, a  call procedure a at level l
     int 0, a  increment t-register by a
     jmp 0, a  jump to a
     jpc 0, a  jump conditional to a
  *)
var (* 全局变量定义 *)
 
  ch: char; (* last char read *) (* 主要用于词法分析器，存放最近一次从文件中读出的字符 *)
  sym: symbol; (* last symbol read *) (* 词法分析器输出结果之用，存放最近一次识别出来的token的类型 *)
  id: alfa;  (* last identifier read *) (* 词法分析器输出结果之用，存放最近一次识别出来的标识符的名字 *)
  num: integer; (* last number read *) (* 词法分析器输出结果之用，存放最近一次识别出来的数字的值 *)
  cc: integer;  (* character count *) (* 行缓冲区指针 *)
  ll: integer;  (* line length *) (* 行缓冲区长度 *)
  kk: integer;  (* 引入此变量是出于程序性能考虑，见getsym过程注释 *)
  cx: integer;  (* code allocation index *) (* 代码分配指针，代码生成模块总在cx所指位置生成新的代码 *)
  line: array[1..81] of char; (* 行缓冲区，用于从文件读出一行，供词法分析获取单词时之用 *)
  a: alfa; (* 词法分析器中用于临时存放正在分析的词 *)
  code: array[0..cxmax] of instruction; (* 生成的类PCODE代码表，存放编译得到的类PCODE代码 *)
  word: array[1..norw] of alfa; (* 保留字表 *)
  wsym: array[1..norw] of symbol; (* 保留字表中每一个保留字对应的symbol类型 *)
  ssym: array[‘ ‘..’^’] of symbol; (* 一些符号对应的symbol类型表 *)
    (* wirth uses “array[char]” here *)
  mnemonic: array[fct] of packed array[1..5] of char;(* 类PCODE指令助记符表 *)
  declbegsys, statbegsys, facbegsys: symset; (* 声明开始、表达式开始和项开始符号集合 *)
  table: array[0..txmax] of record (* 符号表 *)
    name: alfa; (* 符号的名字 *)
    case kind: object1 of (* 符号的类型 *)
      constant: (* 如果是常量名 *)
        (val: integer); (* val中放常量的值 *)
      variable, procedur:  (* 如果是变量名或过程名 *)
        (level, adr, size: integer) (* 存放层差、偏移地址和大小 *)
        (* “size” lacking in orginal. I think it belons here *)
  end;
  fin, fout: text; (* fin文本文件用于指向输入的源程序文件，fout程序中没有用到 *)
  sfile: string; (* 存放PL/0源程序文件的文件名 *)
 《分析PL0词法分析程序》
(* 出错处理过程error *)
(* 参数：n:出错代码 *)
procedure error(n: integer);
begin
  writeln(‘****’, ‘ ‘: cc-1, ‘!’, n:2); (* 在屏幕cc-1位置显示!与出错代码提示，由于cc
                                           是行缓冲区指针，所以!所指位置即为出错位置 *)
  writeln(fa1, ‘****’, ‘ ‘: cc-1, ‘!’, n:2); (* 在文件cc-1位置输出!与出错代码提示 *)
  err := err + 1 (* 出错总次数加一 *)
end (* error *);
(* 词法分析过程getsym *)
procedure getsym;
var
  i, j, k: integer;
  (* 读取原程序中下一个字符过程getch *)
  procedure getch;
  begin
    if cc = ll then (* 如果行缓冲区指针指向行缓冲区最后一个字符就从文件读一行到行缓冲区 *)
    begin
      if eof(fin) then (* 如果到达文件末尾 *)
      begin
        write(‘Program incomplete’); (* 出错，退出程序 *)

        close(fin);
            exit;     
       end;
       
      ll := 0; (* 行缓冲区长度置0 *)
      cc := 0; (* 行缓冲区指针置行首 *)
      write(cx: 4, ‘ ‘); (* 输出cx值，宽度为4 *)
      write(fa1, cx: 4, ‘ ‘); (* 输出cx值，宽度为4到文件 *)
      while not eoln(fin) do (* 当未到行末时 *)
      begin
        ll := ll + 1; (* 行缓冲区长度加一 *)
        read(fin, ch); (* 从文件读入一个字符到 ch *)
        write(ch); (* 在屏幕输出ch *)
        write(fa1, ch); (* 把ch输出到文件 *)
        line[ll] := ch; (* 把读到的字符存入行缓冲区相应的位置 *)
      end;
      (* 可见，PL/0源程序要求每行的长度都小于81个字符 *)
      writeln;
      ll := ll + 1; (* 行缓冲区长度加一，用于容纳即将读入的回车符CR *)
      read(fin, line[ll]);(* 把#13(CR)读入行缓冲区尾部 *)
      read(fin, ch); (* 我添加的代码。由于PC上文本文件换行是以#13#10(CR+LF)表示的，
                        所以要把多余的LF从文件读出，这里放在ch变量中是由于ch变量的
                        值在下面即将被改变，把这个多余值放在ch中没有问题 *)
      writeln(fa1);
    end;
    cc := cc + 1; (* 行缓冲区指针加一，指向即将读到的字符 *)
    ch := line[cc] (* 读出字符，放入全局变量ch *)
  end (* getch *);
begin (* getsym *)
  while (ch = ‘ ‘) do getch;
  if ch in [‘a’..’z’] then (* 如果读出的字符是一个字母，说明是保留字或标识符 *)
  begin
    k := 0; (* 标识符缓冲区指针置0 *)
    repeat (* 这个循环用于依次读出源文件中的字符构成标识符 *)
      if k < al then (* 如果标识符长度没有超过最大标识符长度(如果超过，就取前面一部分，把多余的抛弃) *)
      begin
        k := k + 1;
        a[k] := ch;
      end;
      getch (* 读下一个字符 *)
    until not (ch in [‘a’..’z’,’0′..’9′]); (* 直到读出的不是字母或数字，由此可知PL/0的标识符构成规则是:
                                              以字母开头，后面跟若干个字母或数字 *)
    if k >= kk then (* 如果当前获得的标识符长度大于等于kk *)
      kk := k (* 令kk为当前标识符长度 *)
    else
      repeat (* 这个循环用于把标识符缓冲后部没有填入相应字母或空格的空间用空格补足 *)
        a[kk] := ‘ ‘;
        kk := kk – 1
      until kk = k;
    (* 在第一次运行这个过程时，kk的值为al，即最大标识符长度，如果读到的标识符长度小于kk，
       就把a数组的后部没有字母的空间用空格补足。
       这时，kk的值就成为a数组前部非空格字符的个数。以后再运行getsym时，如果读到的标识符长度大于等于kk，
       就把kk的值变成当前标识符的长度。
       这时就不必在后面填空格了，因为它的后面肯定全是空格。反之如果最近读到的标识符长度小于kk，那就需要从kk位置向前，
       把超过当前标识长度的空间填满空格。
       以上的这样一个逻辑，完全是出于程序性能的上考虑。其实完全可以简单的把a数组中a[k]元素以后的空间不管三七二十一全填空格。
    *)
    (* 下面开始二分法查找看读出的标识符是不是保留字之一 *)
    id := a; (* 最后读出标识符等于a *)
    i := 1; (* i指向第一个保留字 *)
    j := norw; (* j指向最后一个保留字 *)
    repeat
      k := (i + j) div 2; (* k指向中间一个保留字 *)
      if id <= word[k] then (* 如果当前的标识符小于k所指的保留字 *)
        j := k – 1; (* 移动j指针 *)
      if id >= word[k] then (* 如果当前的标识符大于k所指的保留字 *)
        i := k + 1 (* 移动i指针 *)
    until i > j; (* 循环直到找完保留字表 *)
    if i – 1 > j then (* 如果i – 1 > j表明在保留字表中找到相应的项，id中存的是保留字 *)
      sym := wsym[k] (* 找到保留字，把sym置为相应的保留字值 *)
    else
      sym := ident (* 未找到保留字，把sym置为ident类型，表示是标识符 *)
  end(* 至此读出字符为字母即对保留字或标识符的处理结束 *)
  else (* 如果读出字符不是字母 *)
    if ch in [‘0’..’9′] then (* 如果读出字符是数字 *)
    begin (* number *) (* 开始对数字进行处理 *)
      k := 0; (* 数字位数 *)
      num := 0; (* 数字置为0 *)
      sym := number; (* 置sym为number，表示这一次读到的是数字 *)
      repeat (* 这个循环依次从源文件中读出字符，组成数字 *)
        num := 10 * num + (ord(ch) – ord(‘0’)); (* num * 10加上最近读出的字符ASCII减’0’的ASCII得到相应的数值 *)
        k := k + 1; (* 数字位数加一 *)
        getch
      until not (ch in [‘0’..’9′]); (* 直到读出的字符不是数字为止 *)
      if k > nmax then (* 如果组成的数字位数大于最大允许的数字位数 *)
        error(30) (* 发出30号错 *)
    end(* 至此对数字的识别处理结束 *)
    else
      if ch = ‘:’ then (* 如果读出的不字母也不是数字而是冒号 *)
      begin
        getch; (* 再读一个字符 *)
        if ch = ‘=’ then (* 如果读到的是等号，正好可以与冒号构成赋值号 *)
        begin
          sym := becomes; (* sym的类型设为赋值号becomes *)
          getch (* 再读出下一个字 *)
        end
        else
          sym := nul; (* 如果不是读到等号，那单独的一个冒号就什么也不是 *)
      end(* 以上完成对赋值号的处理 *)
    else (* 如果读到不是字母也不是数字也不是冒号 *)
      if ch = ‘<‘ then (* 如果读到小于号 *)
      begin
        getch; (* 再读一个字符 *)
        if ch = ‘=’ then (* 如果读到等号 *)
        begin
          sym := leq; (* 购成一个小于等于号 *)
          getch (* 读一个字符 *)
        end
        else (* 如果小于号后不是跟的等号 *)
          sym := lss (* 那就是一个单独的小于号 *)
      end
      else (* 如果读到不是字母也不是数字也不是冒号也不是小于号 *)
        if ch = ‘>’ then (* 如果读到大于号，处理过程类似于处理小于号 *)
        begin
          getch; (* 再读一个字符 *)
          if ch = ‘=’ then (* 如果读到等号 *)
          begin
            sym := geq; (* 购成一个大于等于号 *)
            getch (* 读一个字符 *)
          end
          else (* 如果大于号后不是跟的等号 *)
            sym := gtr (* 那就是一个单独的大于号 *)
        end
        else(* 如果读到不是字母也不是数字也不是冒号也不是小于号也不是大于号 *)
        begin (* 那就说明它不是标识符/保留字，也不是复杂的双字节操作符，应该是一个普通的符号 *)
          sym := ssym[ch]; (* 直接成符号表中查到它的类型，赋给sym *)
          getch (* 读下一个字符 *)
        end
  (* 整个if语句判断结束 *)
end (* getsym *);
(* 词法分析过程getsym总结：从源文件中读出若干有效字符，组成一个token串，识别它的类型
   为保留字/标识符/数字或是其它符号。如果是保留字，把sym置成相应的保留字类型，如果是
   标识符，把sym置成ident表示是标识符，于此同时，id变量中存放的即为保留字字符串或标识
   符名字。如果是数字，把sym置为number,同时num变量中存放该数字的值。如果是其它的操作符，
   则直接把sym置成相应类型。经过本过程后ch变量中存放的是下一个即将被识别的字符 *)
(* 目标代码生成过程gen *)
(* 参数：x:要生成的一行代码的助记符 *)
(*       y, z:代码的两个操作数 *)
(* 本过程用于把生成的目标代码写入目标代码数组，供后面的解释器解释执行 *)
procedure gen(x: fct; y, z: integer);
begin
  if cx > cxmax then (* 如果cx>cxmax表示当前生成的代码行号大于允许的最大代码行数 *)
  begin
    write(‘program too long’); {goto 99}(* 输出”程序太长”，退出 *)
    end;
    with code[cx] do
      begin f := x; l := y; a := z
      end;
   cx := cx + 1
end {gen};

(* 测试当前单词是否合法过程test *)
(* 参数：s1:当语法分析进入或退出某一语法单元时当前单词符合应属于的集合 *)
(*       s2:在某一出错状态下，可恢复语法分析正常工作的补充单词集合 *)
(*       n:出错信息编号，当当前符号不属于合法的s1集合时发出的出错信息 *)
procedure test(s1, s2: symset; n: integer);
begin
  if not (sym in s1) then (* 如果当前符号不在s1中 *)
  begin
    error(n); (* 发出n号错误 *)
    s1 := s1 + s2; (* 把s2集合补充进s1集合 *)
    while not (sym in s1) do (* 通过循环找到下一个合法的符号，以恢复语法分析工作 *)
      getsym
  end
end (* test *);
(* 语法分析过程block *)
(* 参数：lev:这一次语法分析所在的层次 *)
(*       tx:符号表指针 *)
(*       fsys:用于出错恢复的单词集合 *)
procedure block(lev, tx: integer; fsys: symset);
var
  dx: integer; (* data allocation index *) (* 数据段内存分配指针，指向下一个被分配空间在数据段中的偏移位置 *)
  tx0: integer;  (* initial table index *) (* 记录本层开始时符号表位置 *)
  cx0: integer;  (* initial code index *) (* 记录本层开始时代码段分配位置 *)
  (* 登陆符号表过程enter *)
  (* 参数：k:欲登陆到符号表的符号类型 *)
  procedure enter(k: object1);
  begin (* enter object into table *)
    tx := tx + 1; (* 符号表指针指向一个新的空位 *)
    with table[tx] do (* 开始登录 *)
    begin
      name := id; (* name是符号的名字，对于标识符，这里就是标识符的名字 *)
      kind := k; (* 符号类型，可能是常量、变量或过程名 *)
      case k of (* 根据不同的类型进行不同的操作 *)
        constant: (* 如果是常量名 *)
        begin
          if num > amax then (* 在常量的数值大于允许的最大值的情况下 *)
          begin
            error(31); (* 抛出31号错误 *)
            num := 0; (* 实际登陆的数字以0代替 *)
          end;
          val := num (* 如是合法的数值，就登陆到符号表 *)
        end;
        variable: (* 如果是变量名 *)
        begin
          level := lev; (* 记下它所属的层次号 *)
          adr := dx; (* 记下它在当前层中的偏移量 *)
          dx := dx+1; (* 偏移量自增一，为下一次做好准备 *)
        end;
        procedur: (* 如果要登陆的是过程名 *)
          level := lev (* 记录下这个过程所在层次 *)
      end
    end
  end (* enter *);
  (* 登录符号过程没有考虑到重复的定义的问题。如果出现重复定义，则以最后一次的定义为准。 *)
 
  (* 在符号表中查找指定符号所在位置的函数position *)
  (* 参数：id:要找的符号 *)
  (* 返回值：要找的符号在符号表中的位置，如果找不到就返回0 *)
  function position (id: alfa): integer;
  var
    i: integer;
  begin (* find identifier in table *)
    table[0].name := id; (* 先把id放入符号表0号位置 *)
    i := tx; (* 从符号表中当前位置也即最后一个符号开始找 *)
    while table[i].name <> id do (* 如果当前的符号与要找的不一致 *)
      i := i – 1; (* 找前面一个 *)
    position := i (* 返回找到的位置号，如果没找到则一定正好为0 *)
  end(* position *);
  (* 常量声明处理过程constdeclaration *)
  procedure constdeclaration;
  begin
    if sym = ident then (* 常量声明过程开始遇到的第一个符号必然应为标识符 *)
    begin
      getsym; (* 获取下一个token *)
      if sym in [eql, becomes] then (* 如果是等号或赋值号 *)
      begin
        if sym = becomes then (* 如果是赋值号(常量生明中应该是等号) *)
          error(1); (* 抛出1号错误 *)
        (* 这里其实自动进行了错误纠正使编译继续进行，把赋值号当作等号处理 *)
        getsym; (* 获取下一个token，等号或赋值号后应接上数字 *)
        if sym = number then (* 如果的确是数字 *)
        begin
          enter(constant); (* 把这个常量登陆到符号表 *)
          getsym (* 获取下一个token，为后面作准备 *)
        end
        else
          error(2) (* 如果等号后接的不是数字，抛出2号错误 *)
      end
      else
        error(3) (* 如果常量标识符后接的不是等号或赋值号，抛出3号错误 *)
    end
    else
      error(4) (* 如果常量声明过程遇到的第一个符号不为标识符，抛出4号错误 *)
  end(* constdeclaration *);
  (* 变量声明过程vardeclaration *) 
  procedure vardeclaration;
  begin
    if sym = ident then (* 变量声明过程开始遇到的第一个符号必然应为标识符 *)
    begin
      enter(variable); (* 将标识符登陆到符号表中 *)
      getsym (* 获取下一个token，为后面作准备 *)
    end
    else
      error(4) (* 如果变量声明过程遇到的第一个符号不是标识符，抛出4号错误 *)
  end(* vardeclaration *);
  (* 列出当前一层类PCODE目标代码过程listcode *)
  procedure listcode;
  var
    i: integer;
  begin (* list code generated for this block *)
    if listswitch then (* 如果用户选择是要列出代码的情况下才列出代码 *)
    begin
      for i := cx0 to cx – 1 do (* 从当前层代码开始位置到当前代码位置-1处，即为本分程序块 *)
        with code[i] do
        begin
          writeln(i: 4, mnemonic[f]: 5, l: 3, a: 5); (* 显示出第i行代码的助记符和L与A操作数 *)
          (* 我修改的代码：原程序此处在输出i时，没有指定占4个字符宽度，不美观也与下面一句不配套。 *)
          writeln(fa, i: 4, mnemonic[f]: 5, l: 3, a: 5) (* 同时把屏显打印到文件 *)
        end;
    end
  end(* listcode *);
  (* 语句处理过程statement *)
  (* 参数说明：fsys: 如果出错可用来恢复语法分析的符号集合 *)
  procedure statement(fsys: symset);
  var
    i, cx1, cx2: integer;
    (* 表达式处理过程expression *)
    (* 参数说明：fsys: 如果出错可用来恢复语法分析的符号集合 *)

    procedure expression(fsys: symset);
    var
      addop: symbol;
      (* 项处理过程term *)
      (* 参数说明：fsys: 如果出错可用来恢复语法分析的符号集合 *)
      procedure term(fsys: symset);
      var
        mulop: symbol;
        (* 因子处理过程factor *)
        (* 参数说明：fsys: 如果出错可用来恢复语法分析的符号集合 *)
        procedure factor(fsys: symset);
        var
          i: integer;
        begin
          test(facbegsys, fsys, 24); (* 开始因子处理前，先检查当前token是否在facbegsys集合中。 *)
                                     (* 如果不是合法的token，抛24号错误，并通过fsys集恢复使语法处理可以继续进行 *)        
          while sym in facbegsys do (* 循环处理因子 *)
          begin
            if sym = ident then (* 如果遇到的是标识符 *)
            begin
              i := position(id); (* 查符号表，找到当前标识符在符号表中的位置 *)
              if i = 0 then (* 如果查符号表返回为0，表示没有找到标识符 *)
                error(11) (* 抛出11号错误 *)
              else
                with table[i] do (* 如果在符号表中找到了当前标识符的位置，开始生成相应代码 *)
                  case kind of
                    constant: gen(lit, 0, val); (* 如果这个标识符对应的是常量，值为val，生成lit指令，把val放到栈顶 *)
                    variable: gen(lod, lev – level, adr); (* 如果标识符是变量名，生成lod指令， *)
                                                          (* 把位于距离当前层level的层的偏移地址为adr的变量放到栈顶 *)
                    procedur: error(21) (* 如果在因子处理中遇到的标识符是过程名，出错了，抛21号错 *)
                  end;
              getsym (* 获取下一token，继续循环处理 *)
            end
            else
              if sym = number then (* 如果因子处理时遇到数字 *)
              begin
                if num > amax then (* 如果数字的大小超过允许最大值amax *)
                begin
                  error(31); (* 抛出31号错 *)
                  num := 0 (* 把数字按0值处理 *)
                end;
                gen(lit, 0, num); (* 生成lit指令，把这个数值字面常量放到栈顶 *)
                getsym (* 获取下一token *)
              end
              else
                if sym = lparen then (* 如果遇到的是左括号 *)
                begin
                  getsym; (* 获取一个token *)
                  expression( [rparen] + fsys ); (* 递归调用expression子程序分析一个子表达式 *)
                  if sym = rparen then (* 子表达式分析完后，应遇到右括号 *)
                    getsym (* 如果的确遇到右括号，读取下一个token *)
                  else
                    error(22) (* 否则抛出22号错误 *)
                end;
            test(fsys, facbegsys, 23) (* 一个因子处理完毕，遇到的token应在fsys集合中 *)
                                      (* 如果不是，抛23号错，并找到下一个因子的开始，使语法分析可以继续运行下去 *)
          end
        end(* factor *);
      begin (* term *)
        factor([times, slash] + fsys); (* 每一个项都应该由因子开始，因此调用factor子程序分析因子 *)
        while sym in [times, slash] do (* 一个因子后应当遇到乘号或除号 *)
        begin
          mulop := sym; (* 保存当前运算符 *)
          getsym; (* 获取下一个token *)
          factor(fsys + [times, slash]); (* 运算符后应是一个因子，故调factor子程序分析因子 *)
          if mulop = times then (* 如果刚才遇到乘号 *)
            gen(opr, 0, 4) (* 生成乘法指令 *)
          else
            gen(opr, 0, 5) (* 不是乘号一定是除号，生成除法指令 *)
        end
      end (* term *);
    begin (* expression *)
      if sym in [plus, minus] then (* 一个表达式可能会由加号或减号开始，表示正负号 *)
      begin
        addop := sym; (* 把当前的正号或负号保存起来，以便下面生成相应代码 *)
        getsym; (* 获取一个token *)
        term(fsys + [plus, minus]); (* 正负号后面应该是一个项，调term子程序分析 *)
        if addop = minus then (* 如果保存下来的符号是负号 *)
          gen(opr, 0, 1) (* 生成一条1号操作指令：取反运算 *)
        (* 如果不是负号就是正号，不需生成相应的指令 *)
      end
      else (* 如果不是由正负号开头，就应是一个项开头 *)
        term(fsys + [plus, minus]); (* 调用term子程序分析项 *)
      while sym in [plus, minus] do (* 项后应是加运算或减运算 *)
      begin
        addop := sym; (* 把运算符保存下来 *)
        getsym; (* 获取下一个token，加减运算符后应跟的是一个项 *)
        term(fsys + [plus, minus]); (* 调term子程序分析项 *)
        if addop = plus then (* 如果项与项之间的运算符是加号 *)
          gen(opr, 0, 2) (* 生成2号操作指令：加法 *)
        else (* 否则是减法 *)
          gen(opr, 0, 3) (* 生成3号操作指令：减法 *)
      end
    end (* expression *);
    (* 条件处理过程condition *)
    (* 参数说明：fsys: 如果出错可用来恢复语法分析的符号集合 *)
    procedure condition(fsys: symset);
    var
      relop: symbol; (* 用于临时记录token(这里一定是一个二元逻辑运算符)的内容 *)
    begin
      if sym = oddsym then (* 如果是odd运算符(一元) *)
      begin
        getsym; (* 获取下一个token *)
        expression(fsys); (* 对odd的表达式进行处理计算 *)
        gen(opr, 0, 6); (* 生成6号操作指令：奇偶判断运算 *)
      end
      else (* 如果不是odd运算符(那就一定是二元逻辑运算符) *)
      begin
        expression([eql, neq, lss, leq, gtr, geq] + fsys); (* 对表达式左部进行处理计算 *)
        if not (sym in [eql, neq, lss, leq, gtr, geq]) then (* 如果token不是逻辑运算符中的一个 *)
          error(20) (* 抛出20号错误 *)
        else
        begin
          relop := sym; (* 记录下当前的逻辑运算符 *)
          getsym; (* 获取下一个token *)
          expression(fsys); (* 对表达式右部进行处理计算 *)
          case relop of (* 如果刚才的运算符是下面的一种 *)
            eql: gen(opr, 0, 8); (* 等号：产生8号判等指令 *)
            neq: gen(opr, 0, 9); (* 不等号：产生9号判不等指令 *)
            lss: gen(opr, 0, 10); (* 小于号：产生10号判小指令 *)
            geq: gen(opr, 0, 11); (* 大于等号号：产生11号判不小于指令 *)
            gtr: gen(opr, 0, 12); (* 大于号：产生12号判大于指令 *)
            leq: gen(opr, 0, 13); (* 小于等于号：产生13号判不大于指令 *)
          end
        end
      end
    end (* condition *);
  begin (* statement *)
    if sym = ident then (* 所谓”语句”可能是赋值语句，以标识符开头 *)
    begin
      i := position(id); (* 在符号表中查到该标识符所在位置 *)
      if i = 0 then (* 如果没找到 *)
        error(11) (* 抛出11号错误 *)
      else
        if table[i].kind <> variable then (* 如果在符号表中找到该标识符，但该标识符不是变量名 *)
        begin
          error(12); (* 抛出12号错误 *)
          i := 0 (* i置0作为错误标志 *)
        end;
      getsym; (* 获得下一个token，正常应为赋值号 *)
      if sym = becomes then (* 如果的确为赋值号 *)
        getsym (* 获取下一个token，正常应为一个表达式 *)
      else
        error(13); (* 如果赋值语句的左部标识符号后所接不是赋值号，抛出13号错误 *)
      expression(fsys); (* 处理表达式 *)
      if i <> 0 then (* 如果不曾出错，i将不为0，i所指为当前语名左部标识符在符号表中的位置 *)
        with table[i] do
          gen(sto, lev – level, adr) (* 产生一行把表达式值写往指定内存的sto目标代码 *)
    end
    else
      if sym = readsym then (* 如果不是赋值语句，而是遇到了read语句 *)
      begin
        getsym; (* 获得下一token，正常情况下应为左括号 *)
        if sym <> lparen then (* 如果read语句后跟的不是左括号 *)
          error(34) (* 抛出34号错误 *)
        else
          repeat (* 循环得到read语句括号中的参数表，依次产生相应的“从键盘读入”目标代码 *)
            getsym; (* 获得一个token，正常应是一个变量名 *)
            if sym = ident then (* 如果确为一个标识符 *)
            (* 这里略有问题，还应判断一下这个标识符是不是变量名，如果是常量名或过程名应出错 *)
              i := position(id) (* 查符号表，找到它所在位置给i，找不到时i会为0 *)
            else
              i := 0; (* 不是标识符则有问题，i置0作为出错标志 *)
            if i = 0 then (* 如果有错误 *)
              error(35) (* 抛出35号错误 *)
            else (* 否则生成相应的目标代码 *)
              with table[i] do
              begin
                gen(opr, 0, 16); (* 生成16号操作指令:从键盘读入数字 *)
                gen(sto, lev – level, adr) (* 生成sto指令，把读入的值存入指定变量所在的空间 *)
              end;
            getsym (* 获取下一个token，如果是逗号，则read语还没完，否则应当是右括号 *)
          until sym <> comma; (* 不断生成代码直到read语句的参数表中的变量遍历完为止，这里遇到不是逗号，应为右括号 *)
        if sym <> rparen then (* 如果不是我们预想中的右括号 *)
        begin
          error(33); (* 抛出33号错误 *)
          while not (sym in fsys) do (* 依靠fsys集，找到下一个合法的token，恢复语法分析 *)
            getsym
        end
        else
          getsym (* 如果read语句正常结束，得到下一个token，一般为分号或end *)
      end
      else
        if sym = writesym then (* 如果遇到了write语句 *)
        begin
          getsym; (* 获取下一token，应为左括号 *)
          if sym = lparen then (* 如确为左括号 *)
          begin
            repeat (* 依次获取括号中的每一个值，进行输出 *)
              getsym; (* 获得一个token，这里应是一个标识符 *)
              expression([rparen, comma] + fsys); (* 调用expression过程分析表达式，用于出错恢复的集合中加上右括号和逗号 *)
              gen(opr, 0, 14) (* 生成14号指令：向屏幕输出 *)
            until sym <> comma; (* 循环直到遇到的不再是逗号，这时应是右括号 *)
            if sym <> rparen then (* 如果不是右括号 *)
              error(33) (* 抛出33号错误 *)
            else
              getsym (* 正常情况下要获取下一个token，为后面准备好 *)
          end;
          gen(opr, 0, 15) (* 生成一个15号操作的目标代码，功能是输出一个换行 *)
          (* 由此可知PL/0中的write语句与Pascal中的writeln语句类似，是带有输出换行的 *)
        end
        else
          if sym = callsym then (* 如果是call语句 *)
          begin
            getsym; (* 获取token，应是过程名型标识符 *)
            if sym <> ident then (* 如果call后跟的不是标识符 *)
              error(14) (* 抛出14号错误 *)
            else
            begin
              i := position(id); (* 从符号表中找出相应的标识符 *)
              if i = 0 then (* 如果没找到 *)
                error(11) (* 抛出11号错误 *)
              else
                with table[i] do (* 如果找到标识符位于符号表第i位置 *)
                  if kind = procedur then (* 如果这个标识符为一个过程名 *)
                    gen(cal,lev-level,adr) (* 生成cal目标代码，呼叫这个过程 *)
                  else
                    error(15); (* 如果call后跟的不是过程名，抛出15号错误 *)
              getsym (* 获取下一token，为后面作准备 *)
            end
          end
        else
          if sym = ifsym then (* 如果是if语句 *)
          begin
            getsym; (* 获取一token应是一个逻辑表达式 *)
            condition([thensym, dosym] + fsys); (* 对逻辑表达式进行分析计算，出错恢复集中加入then和do语句 *)
            if sym = thensym then (* 表达式后应遇到then语句 *)
              getsym (* 获取then后的token，应是一语句 *)
            else
              error(16); (* 如果if后没有then，抛出16号错误 *)
            cx1 := cx; (* 记下当前代码分配指针位置 *)
            gen(jpc, 0, 0); (* 生成条件跳转指令，跳转位置暂时填0，分析完语句后再填写 *)
            statement(fsys); (* 分析then后的语句 *)
            code[cx1].a:=cx (* 上一行指令(cx1所指的)的跳转位置应为当前cx所指位置 *)
          end
          else
            if sym = beginsym then (* 如果遇到begin *)
            begin
              getsym; (* 获取下一个token *)
              statement([semicolon, endsym] + fsys);(* 对begin与end之间的语句进行分析处理 *)
              while sym in [semicolon] + statbegsys do (* 如果分析完一句后遇到分号或语句开始符循环分析下一句语句 *)
              begin
                if sym = semicolon then (* 如果语句是分号（可能是空语句） *)
                  getsym (* 获取下一token继续分析 *)
                else
                  error(10); (* 如果语句与语句间没有分号，出10号错 *)
                statement([semicolon, endsym] + fsys) (* 分析一个语句 *)
              end;
              if sym = endsym then (* 如果语句全分析完了，应该遇到end *)
                getsym (* 的确是end，读下一token *)
              else
                error(17) (* 如果不是end，抛出17号错 *)
            end
            else
              if sym = whilesym then (* 如果遇到while语句 *)
              begin
                cx1 := cx; (* 记下当前代码分配位置，这是while循环的开始位置 *)
                getsym; (* 获取下一token，应为一逻辑表达式 *)
                condition([dosym] + fsys); (* 对这个逻辑表达式进行分析计算 *)
                cx2 := cx; (* 记下当前代码分配位置，这是while的do中的语句的开始位置 *)
                gen(jpc, 0, 0); (* 生成条件跳转指令，跳转位置暂时填0 *)
                if sym = dosym then (* 逻辑表达式后应为do语句 *)
                  getsym (* 获取下一token *)
                else
                  error(18); (* if后缺少then，抛出18号错误 *)
                statement(fsys); (* 分析do后的语句块 *)
                gen(jmp, 0, cx1); (* 循环跳转到cx1位置，即再次进行逻辑判断 *)
                code[cx2].a := cx (* 把刚才填0的跳转位置改成当前位置，完成while语句的处理 *)
              end;
    test(fsys, [], 19) (* 至此一个语句处理完成，一定会遇到fsys集中的符号，如果没有遇到，就抛19号错 *)
  end(* statement *);
begin (* block *)
  dx := 3; (* 地址指示器给出每层局部量当前已分配到的相对位置。
              置初始值为3的原因是：每一层最开始的位置有三个空间用于存放静态链SL、动态链DL和返回地址RA *)
  tx0 := tx; (* 初始符号表指针指向当前层的符号在符号表中的开始位置 *)
  table[tx].adr := cx; (* 符号表当前位置记下当前层代码的开始位置 *)
  gen(jmp, 0, 0); (* 产生一行跳转指令，跳转位置暂时未知填0 *)
  if lev > levmax then (* 如果当前过程嵌套层数大于最大允许的套层数 *)
    error(32); (* 发出32号错误 *)
  repeat (* 开始循环处理源程序中所有的声明部分 *)
    if sym = constsym then (* 如果当前token是const保留字，开始进行常量声明 *)
    begin
      getsym; (* 获取下一个token，正常应为用作常量名的标识符 *)
      repeat (* 反复进行常量声明 *)
        constdeclaration; (* 声明以当前token为标识符的常量 *)
        while sym = comma do (* 如果遇到了逗号则反复声明下一个常量 *)
        begin
          getsym; (* 获取下一个token，这里正好应该是标识符 *)
          constdeclaration (* 声明以当前token为标识符的常量 *)
        end;
        if sym = semicolon then (* 如果常量声明结束，应遇到分号 *)
          getsym (* 获取下一个token，为下一轮循环做好准备 *)
        else
          error(5) (* 如果常量声明语句结束后没有遇到分号则发出5号错误 *)
      until sym <> ident (* 如果遇到非标识符，则常量声明结束 *)
    end;
    (* 此处的常量声明的语法与课本上的EBNF范式有不同之处：
       它可以接受像下面的声明方法，而根据课本上的EBNF范式不可得出下面的语法：
       const a = 3, b = 3; c = 6; d = 7, e = 8;
       即它可以接受分号或逗号隔开的常量声明，而根据EBNF范式只可接受用逗号隔开的声明 *)
    if sym = varsym then (* 如果当前token是var保留字，开始进行变量声明,与常量声明类似 *)
    begin
      getsym; (* 获取下一个token，此处正常应为用作变量名的一个标识符 *)
      repeat (* 反复进行变量声明 *)
        vardeclaration; (* 以当前token为标识符声明一个变量 *)
        while sym = comma do (* 如果遇到了逗号则反复声明下一个变量 *)
        begin
          getsym; (* 获取下一个token，这里正好应该是标识符 *)
          vardeclaration; (* 声明以当前token为标识符的变量 *)
        end;
        if sym = semicolon then (* 如果变量声明结束，应遇到分号 *)
          getsym (* 获取下一个token，为下一轮循环做好准备 *)
        else
          error(5) (* 如果变量声明语句结束后没有遇到分号则发出5号错误 *)
      until sym <> ident; (* 如果遇到非标识符，则变量声明结束 *)
      (* 这里也存在与上面的常量声明一样的毛病：与PL/0的语法规范有冲突。 *)
    end;
    while sym = procsym do (* 循环声明各子过程 *)
    begin
      getsym; (* 获取下一个token，此处正常应为作为过程名的标识符 *)
      if sym = ident then (* 如果token确为标识符 *)
      begin
        enter(procedur); (* 把这个过程登录到名字表中 *)
        getsym (* 获取下一个token，正常情况应为分号 *)
      end
      else
        error(4); (* 否则抛出4号错误 *)
      if sym = semicolon then (* 如果当前token为分号 *)
        getsym (* 获取下一个token，准备进行语法分析的递归调用 *)
      else
        error(5); (* 否则抛出5号错误 *)
      block(lev + 1, tx, [semicolon] + fsys); (* 递归调用语法分析过程，当前层次加一，同时传递表头索引、合法单词符 *)
      if sym = semicolon then (* 递归返回后当前token应为递归调用时的最后一个end后的分号 *)
      begin
        getsym; (* 获取下一个token *)
        test(statbegsys + [ident, procsym], fsys, 6); (* 检查当前token是否合法，不合法则用fsys恢复语法分析同时抛6号错 *)
      end
      else
        error(5) (* 如果过程声明后的符号不是分号，抛出5号错误 *)
    end;
    test(statbegsys + [ident], declbegsys, 7) (* 检查当前状态是否合法，不合法则用声明开始符号作出错恢复、抛7号错 *)
  until not (sym in declbegsys); (* 直到声明性的源程序分析完毕，继续向下执行，分析主程序 *)
  code[table[tx0].adr].a := cx; (* 把前面生成的跳转语句的跳转位置改成当前位置 *)
  with table[tx0] do (* 在符号表中记录 *)
  begin
    adr := cx; (* 地址为当前代码分配地址 *)
    size := dx; (* 长度为当前数据代分配位置 *)
  end;
  cx0 := cx; (* 记下当前代码分配位置 *)
  gen(int, 0, dx); (* 生成分配空间指令，分配dx个空间 *)
  statement([semicolon, endsym] + fsys); (* 处理当前遇到的语句或语句块 *)
  gen(opr, 0, 0); (* 生成从子程序返回操作指令 *)
  test(fsys, [], 8); (* 用fsys检查当前状态是否合法，不合法则抛8号错 *)
  listcode (* 列出本层的类PCODE代码 *)
end(* block *);
(* PL/0编译器产生的类PCODE目标代码解释运行过程interpret *)
procedure interpret;
const
  stacksize = 500; (* 常量定义，假想的栈式计算机有500个栈单元 *)
var
  p, b, t: integer; (* program base topstack registers *)
  (* p为程序指令指针，指向下一条要运行的代码 *)
  (* b为基址指针，指向每个过程被调用时数据区中分配给它的局部变量数据段基址 *)
  (* t为栈顶寄存器，类PCODE是在一种假想的栈式计算上运行的，这个变量记录这个计算机的当前栈顶位置 *)
  i: instruction; (* i变量中存放当前在运行的指令 *)
  s: array[1..stacksize] of integer; (* datastore *) (* s为栈式计算机的数据内存区 *)
  (* 通过静态链求出数据区基地址的函数base *)
  (* 参数说明：l:要求的数据区所在层与当前层的层差 *)
  (* 返回值：要求的数据区基址 *)
  function base(l: integer): integer;
  var
    b1: integer;
  begin
    b1 := b; (* find base 1 level down *) (* 首先从当前层开始 *)
    while l > 0 do (* 如果l大于0，循环通过静态链往前找需要的数据区基址 *)
    begin
      b1 := s[b1]; (* 用当前层数据区基址中的内容（正好是静态链SL数据，为上一层的基址）的作为新的当前层，即向上找了一层 *)
      l := l – 1 (* 向上了一层，l减一 *)
    end;
    base := b1 (* 把找到的要求的数据区基址返回 *)
  end(* base *);
begin
  writeln(‘start pl0’); (* PL/0程序开始运行 *)
  t := 0; (* 程序开始运行时栈顶寄存器置0 *)
  b := 1; (* 数据段基址为1 *)
  p := 0; (* 从0号代码开始执行程序 *)
  s[1] := 0;
  s[2] := 0;
  s[3] := 0; (* 数据内存中为SL,DL,RA三个单元均为0，标识为主程序 *)
  repeat (* 开始依次运行程序目标代码 *)
    i := code[p]; (* 获取一行目标代码 *)
    p := p + 1; (* 指令指针加一，指向下一条代码 *)
    with i do
      case f of (* 如果i的f，即指令助记符是下面的某种情况，执行不同的功能 *)
        lit: (* 如果是lit指令 *)
        begin
          t := t + 1; (* 栈顶指针上移，在栈中分配了一个单元 *)
          s[t] := a (* 该单元的内容存放i指令的a操作数，即实现了把常量值放到运行栈栈顶 *)
        end;
        opr: (* 如果是opr指令 *)
          case a of (* operator *) (* 根据a操作数不同，执行不同的操作 *)
            0: (* 0号操作为从子过程返回操作 *)
            begin (* return *)
              t := b – 1; (* 释放这段子过程占用的数据内存空间 *)
              p := s[t + 3]; (* 把指令指针取到RA的值，指向的是返回地址 *)
              b := s[t + 2] (* 把数据段基址取到DL的值，指向调用前子过程的数据段基址 *)
            end;
            1: (* 1号操作为栈顶数据取反操作 *)
              s[t] := -s[t]; (* 对栈顶数据进行取反 *)
            2: (* 2号操作为栈顶两个数据加法操作 *)
            begin
              t := t – 1; (* 栈顶指针下移 *)
              s[t] := s[t] + s[t + 1] (* 把两单元数据相加存入栈顶 *)
            end;
            3: (* 3号操作为栈顶两个数据减法操作 *)
            begin
              t := t – 1; (* 栈顶指针下移 *)
              s[t] := s[t] – s[t + 1] (* 把两单元数据相减存入栈顶 *)
            end;
            4: (* 4号操作为栈顶两个数据乘法操作 *)
            begin
              t := t – 1; (* 栈顶指针下移 *)
              s[t] := s[t] * s[t + 1] (* 把两单元数据相乘存入栈顶 *)
            end;
            5: (* 5号操作为栈顶两个数据除法操作 *)
            begin
              t := t – 1; (* 栈顶指针下移 *)
              s[t] := s[t] div s[t + 1] (* 把两单元数据相整除存入栈顶 *)
            end;
            6: (* 6号操作为判奇操作 *)
              s[t] := ord(odd(s[t])); (* 数据栈顶的值是奇数则把栈顶值置1，否则置0 *)
            8: (* 8号操作为栈顶两个数据判等操作 *)
            begin
              t := t – 1; (* 栈顶指针下移 *)
              s[t] := ord(s[t] = s[t + 1]) (* 判等，相等栈顶置1，不等置0 *)
            end;
            9: (* 9号操作为栈顶两个数据判不等操作 *)
            begin
              t := t – 1; (* 栈顶指针下移 *)
              s[t] := ord(s[t] <> s[t + 1]) (* 判不等，不等栈顶置1，相等置0 *)
            end;
            10: (* 10号操作为栈顶两个数据判小于操作 *)
            begin
              t := t – 1; (* 栈顶指针下移 *)
              s[t] := ord(s[t] < s[t + 1]) (* 判小于，如果下面的值小于上面的值，栈顶置1，否则置0 *)
            end;
            11: (* 11号操作为栈顶两个数据判大于等于操作 *)
            begin
              t := t – 1; (* 栈顶指针下移 *)
              s[t] := ord(s[t] >= s[t + 1]) (* 判大于等于，如果下面的值大于等于上面的值，栈顶置1，否则置0 *)
            end;
            12: (* 12号操作为栈顶两个数据判大于操作 *)
            begin
              t := t – 1; (* 栈顶指针下移 *)
              s[t] := ord(s[t] > s[t + 1]) (* 判大于，如果下面的值大于上面的值，栈顶置1，否则置0 *)
            end;
            13: (* 13号操作为栈顶两个数据判小于等于操作 *)
            begin
              t := t – 1; (* 栈顶指针下移 *)
              s[t] := ord(s[t] <= s[t + 1]) (* 判小于等于，如果下面的值小于等于上面的值，栈顶置1，否则置0 *)
            end;
            14: (* 14号操作为输出栈顶值操作 *)
            begin
              write(s[t]); (* 输出栈顶值 *)
              write(fa2, s[t]); (* 同时打印到文件 *)
              t := t – 1 (* 栈顶下移 *)
            end;
            15: (* 15号操作为输出换行操作 *)
            begin
              writeln; (* 输出换行 *)
              writeln(fa2) (* 同时输出到文件 *)
            end;
            16: (* 16号操作是接受键盘值输入到栈顶 *)
            begin
              t := t + 1; (* 栈顶上移，分配空间 *)
              write(‘?’); (* 屏显问号 *)
              write(fa2, ‘?’); (* 同时输出到文件 *)
              readln(s[t]); (* 获得输入 *)
              writeln(fa2, s[t]); (* 把用户输入值打印到文件 *)
            end;
          end; (* opr指令分析运行结束 *)
        lod: (* 如果是lod指令:将变量放到栈顶 *)
        begin
          t := t + 1; (* 栈顶上移，开辟空间 *)
          s[t] := s[base(l) + a] (* 通过数据区层差l和偏移地址a找到变量的数据，存入上面开辟的新空间（即栈顶） *)
        end;
        sto: (* 如果是sto指令 *)
        begin
          s[base(l) + a] := s[t]; (* 把栈顶的值存入位置在数据区层差l偏移地址a的变量内存 *)
          t := t – 1 (* 栈项下移，释放空间 *)
        end;
        cal: (* 如果是cal指令 *)
        begin (* generat new block mark *)
          s[t + 1] := base(l); (* 在栈顶压入静态链SL *)
          s[t + 2] := b; (* 然后压入当前数据区基址，作为动态链DL *)
          s[t + 3] := p; (* 最后压入当前的断点，作为返回地址RA *)
          (* 以上的工作即为过程调用前的保护现场 *)
          b := t + 1; (* 把当前数据区基址指向SL所在位置 *)
          p := a; (* 从a所指位置开始继续执行指令，即实现了程序执行的跳转 *)
        end;
        int: (* 如果是int指令 *)
          t := t + a; (* 栈顶上移a个空间，即开辟a个新的内存单元 *)
        jmp: (* 如果是jmp指令 *)
          p := a; (* 把jmp指令操作数a的值作为下一次要执行的指令地址，实现无条件跳转 *)
        jpc: (* 如果是jpc指令 *)
        begin
          if s[t] = 0 then (* 判断栈顶值 *)
            p := a; (* 如果是0就跳转，否则不跳转 *)
          t := t – 1 (* 释放栈顶空间 *)
        end;
      end(* with,case *)
  until p = 0; (* 如果p等于0，意味着在主程序运行时遇到了从子程序返回指令，也就是整个程序运行的结束 *)
   write(‘ end pl/0’);
end(* interpret *);
begin (* main *)
   writeln(‘please input source program file name:’);
   readln(sfile);
   assign(fin,sfile);
   reset(fin);

   for ch := chr(0) to chr(255) do ssym[ch] := nul;(* 这个循环把ssym数组全部填nul *)
  (* 下面初始化保留字表，保留字长度不到10个字符的，多余位置用空格填充，便于词法分析时以二分法来查找保留字 *)
  word[1] := ‘begin     ‘;
  word[2] := ‘call      ‘;
  word[3] := ‘const     ‘;
  word[4] := ‘do        ‘;
  word[5] := ‘end       ‘;
  word[6] := ‘if        ‘;
  word[7] := ‘odd       ‘;
  word[8] := ‘procedure ‘;
  word[9] := ‘read      ‘;
  word[10] := ‘then      ‘;
  word[11] := ‘var       ‘;
  word[12] := ‘while     ‘;
  word[13] := ‘write     ‘;
  (* 保留字符号列表，在上面的保留字表中找到保留字后可以本表中相应位置该保留字的类型 *)
  wsym[1] := beginsym;
  wsym[2] := callsym;
  wsym[3] := constsym;
  wsym[4] := dosym;
  wsym[5] := endsym;
  wsym[6] := ifsym;
  wsym[7] := oddsym;
  wsym[8] := procsym;
  wsym[9] := readsym;
  wsym[10] := thensym;
  wsym[11] := varsym;
  wsym[12] := whilesym;
  wsym[13] := writesym;
  (* 初始化符号表，把可能出现的符号赋上相应的类型，其余符号由于开始处的循环所赋的类型均为nul *)
  ssym[‘+’] := plus;
  ssym[‘-‘] := minus;
  ssym[‘*’] := times;
  ssym[‘/’] := slash;
  ssym[‘(‘] := lparen;
  ssym[‘)’] := rparen;
  ssym[‘=’] := eql;
  ssym[‘,’] := comma;
  ssym[‘.’] := period;
  ssym[‘#’] := neq;
  ssym[‘;’] := semicolon;
  (* 初始化类PCODE助记符表，这个表主要供输出类PCODE代码之用 *)
  mnemonic[lit] := ‘ lit ‘;
  mnemonic[opr] := ‘ opr ‘;
  mnemonic[lod] := ‘ lod ‘;
  mnemonic[sto] := ‘ sto ‘;
  mnemonic[cal] := ‘ cal ‘;
  mnemonic[int] := ‘ int ‘;
  mnemonic[jmp] := ‘ jmp ‘;
  mnemonic[jpc] := ‘ jpc ‘;

  declbegsys := [constsym, varsym, procsym];
  statbegsys := [beginsym, callsym, ifsym, whilesym];
  facbegsys := [ident, number, lparen];
  (* page(output) *)
 
  err := 0; (* 出错次数置0 *)
  cc := 0; (* 词法分析行缓冲区指针置0 *)
  cx := 0; (* 类PCODE代码表指针置0 *)
  ll := 0; (* 词法分析行缓冲区长度置0 *)
  ch := ‘ ‘; (* 词法分析当前字符为空格 *)
  kk := al; (* 置kk的值为允许的标识符的最长长度，具体用意见getsym过程注释 *)
 
  getsym; (* 首次调用词法分析子程序，获取源程序的第一个词（token） *)
  block(0, 0, [period] + declbegsys + statbegsys); (* 开始进行主程序（也就是第一个分程序）的语法分析 *)
  (* 主程序所在层为0层，符号表暂时为空，符号表指针指0号位置 *)
 
  if sym <> period then  error(9);(* 主程序分析结束，应遇到表明程序结束的句号 *)
   
  if err=0 then interpret else write(‘ errors in pl/0 program’); (* 如果出错次数为0，可以开始解释执行编译产生的代码 *)
 
  99:writeln(* 这个标号原来是用于退出程序的， *)
 
 
end.