/******************************************************************************/
/*                                                                            */
/*  Name: Sevens 牌七遊戲                                                     */
/*  Copyright: TCFSH CIRC 27th                                                */
/*  Author: xatier @tcirc.org                                                 */
/*  Create: 2010/07/31                                                        */
/*  Update:                                                                   */
/*  Description: 極光鯨藍(telnet://tcirc.org)上即將推出的新遊戲               */
/*               於 Windows 平台上先寫出來的測試實驗                          */
/*               [ 此為作者升高三暑假打混不念書無聊之作 ]                     */
/*                                                                            */
/******************************************************************************/


/* - include ---------------------------------------------------------------- */

#include "bbs.h"
#ifdef HAVE_GAME

/* - macro ------------------------------------------------------------------ */

#define XX 100              /* empty card */
#define F_COLOR "\033[1;36;40m"
#define E_COLOR "\033[1;32;40m"
#define END_COLOR "\033[m"

/* - pointer arrays --------------------------------------------------------- */

char *NUMBER[] = {"Ａ", "２", "３", "４", "５", "６", "７", "８", "９", "10",
                  "Ｊ", "Ｑ", "Ｋ"};
char *FLOWER[] = {"☆", "★", "○", "●"};


char *ASDF[] = {"0123", "0132", "0213", "0231", "0312", "0321", "1023", "1032",
        "1203", "1230", "1302", "1320", "2013", "2031", "2103", "2130",
        "2301", "2310", "3012", "3021", "3102", "3120", "3201", "3210"};
        /*  "0123"的全排列，出牌順序用 */
char *NAME[] = {" you    ", " xatier ", "  zeta  ", "raincole"};
                 /* 玩家的名字(?) */


/* - Structures ------------------------------------------------------------- */
typedef struct {
  int num;          /* 點數    1, 2, 3 ... 13 */
  int flo;          /* 花色    1 ☆, 2 ★, 3 ○, 4 ● */
} CARD;

typedef struct {
  CARD hand[13];        /* 手牌 */
  int pass;         /* 趴死 */
  char *name;           /* 名字 */
} PLAYER;           /* 玩家 */

typedef struct {
  int n;            /* 指到第幾張牌 */
  int front;            /* 最前面的一張 */
  int back;         /* 最後面的一張 */
} CUR;              /* 游標 */

/* - 全域變數 --------------------------------------------------------------- */

CARD set[52];           /* 整副牌 */
CARD copy[52], temp;        /* 發牌用的  */
CARD table[4][13];      /* 桌子上的牌 */
int fb[4][2];           /* 前一張與後一張 */
PLAYER user, p1, p2, p3;    /* 玩家和三個電腦 */
CUR cur;            /* 游標 */
int light;          /* 開燈 */
int i, j;           /* 萬用 i j */

/* - 函數宣告 --------------------------------------------------------------- */
void init (void);           /* 初始化 */
void draw_card (CARD);          /* 畫牌 */
void sort_card (CARD *);        /* 排序 */
void show_all (void);           /* 螢幕輸出 */
int go (PLAYER *);              /* 自動出牌 */
int throw_card (PLAYER *);      /* 手動出牌 */
void new_cur (int n);           /* 更新游標 */
int check (PLAYER *);           /* 檢查勝利 */
void cls (void);            /* 清屏 */
void win (void);            /* 勝利 */
void lose (void);           /* 失敗 */

/* - 函數定義 --------------------------------------------------------------- */
void init (void)
{
  cur.n = p1.pass = p2.pass = p3.pass = user.pass = light = 0;

  user.name = NAME[0];
  p1.name = NAME[1];
  p2.name = NAME[2];
  p3.name = NAME[3];

  int N = 100;

  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 13; j++)
    {
      /* set[] 是整副牌 */
      set[i*13 + j].num = j+1;
      set[i*13 + j].flo = i+1;
      /* copy[] 是發牌用的, 待會會洗牌 */
      copy[i*13 + j] = set[i*13 + j];
      /* table[][] 是牌桌上的牌 */
      table[i][j].num = XX;
      table[i][j].flo = i;
    }
  }

  /* 洗牌, 使用交換法 */
  srand(time(NULL));
  while (N--)
  {
    i = rand() % 52;
    j = rand() % 52;
    temp    = copy[i];
    copy[i] = copy[j];
    copy[j] = temp;
  }

  /* 發牌 */
  for (i = 0; i < 13; i++) user.hand[i] = copy[i];
  for (; i < 26; i++)   p1.hand[i%13] = copy[i];
  for (; i < 39; i++)   p2.hand[i%13] = copy[i];
  for (; i < 52; i++)   p3.hand[i%13] = copy[i];


  /* 排序, 整理手牌 */
  sort_card (user.hand);
  sort_card (p1.hand);
  sort_card (p2.hand);
  sort_card (p3.hand);

  /* 有 7點的要先出 */
  for (i = 0; i < 13; i++)
  {
    if (user.hand[i].num == 7)
    {
      table[user.hand[i].flo-1][6] = user.hand[i];
      user.hand[i].flo = user.hand[i].num = XX;
    }
    if (p1.hand[i].num == 7)
    {
      table[p1.hand[i].flo-1][6] = p1.hand[i];
      p1.hand[i].flo = p1.hand[i].num = XX;
    }
    if (p2.hand[i].num == 7)
    {
      table[p2.hand[i].flo-1][6] = p2.hand[i];
      p2.hand[i].flo = p2.hand[i].num = XX;
    }
    if (p3.hand[i].num == 7)
    {
      table[p3.hand[i].flo-1][6] = p3.hand[i];
      p3.hand[i].flo = p3.hand[i].num = XX;
    }
  }

  /* 7點丟出去後前一張與後一張分別是 6點與 8點 */
  fb[0][0] = fb[1][0] = fb[2][0] = fb[3][0] = 6;
  fb[0][1] = fb[1][1] = fb[2][1] = fb[3][1] = 8;

  cls();

}

void cls (void)
{
  move(0, 0);
  clrtobot();
  move(0, 0);
}


void draw_card (CARD c)
{
  if (c.num == XX)
    prints("│    │");
  else
    prints("%s│%s%s│" END_COLOR, (c.flo-1)&1 ? F_COLOR : E_COLOR, FLOWER[c.flo-1], NUMBER[c.num-1]);
}


void show_all (void)
{

  char buf[10];

/*************************************************************************
 * 三元運算子是為了防止一隻應該是不會發生(Debug 時常會遇到 XD)的 BUG :
 *  *FLOWER[]和 *NUMBER[] 指到 NULL時會印出 "(NULL)" 字串
 *
 *************************************************************************/

  if (user.hand[0].num != XX)
    sprintf(buf, "%s%s",
            FLOWER[user.hand[cur.n].flo-1] ? FLOWER[user.hand[cur.n].flo-1] : "  ",                    NUMBER[user.hand[cur.n].num-1] ? NUMBER[user.hand[cur.n].num-1] : "  ");

  else sprintf(buf, "    ");

  cls();

  outs("╔═════════════════════════════════"
       "════╗\n");
  /* 還未開燈 */
  for (i = 0; i < 13 && !light; i++)
  {
    outs("║");
    /* - 玩家的牌 ------------------------------------------------------- */
    if (i == cur.n)
    {
      move(cur.n+1, 0);
      outs("\033[1;35;40m→\033[m");            /* 箭頭 */
    }
    if (user.hand[i].num != XX)
      prints("%s├%s%s┤" END_COLOR, ((user.hand[i].flo-1)&1 ? F_COLOR : E_COLOR), FLOWER[user.hand[i].flo-1],   NUMBER[user.hand[i].num-1]);
    else outs("        ");              /* empty card */

    if (p1.hand[i].num != XX) outs("\033[1;34;40m" "├◆◆┤" END_COLOR); /* 沒開燈時看不到對手的牌 */
    else outs("        ");
    if (p2.hand[i].num != XX) outs("\033[0;31;40m" "├◇◇┤" END_COLOR);
    else outs("        ");
    if (p3.hand[i].num != XX) outs("\033[1;34;40m" "├◆◆┤" END_COLOR);
    else outs("        ");

    outs(" ◇ ");
    /* - 牌桌的牌 ------------------------------------------------------- */
    draw_card(table[0][i]);
    outs("  ");
    draw_card(table[1][i]);
    outs("  ");
    draw_card(table[2][i]);
    outs("  ");
    draw_card(table[3][i]);

    outs("║\n");
  }
  /* 開燈了 */
  for (i = 0; i < 13 && light; i++)
  {
    outs("║");
    /* - 玩家的牌 ------------------------------------------------------- */
    if (i == cur.n) 
    {
      move(cur.n+1, 0);
      outs("\b\b→");
    }

    draw_card(user.hand[i]);
    draw_card(p1.hand[i]);
    draw_card(p2.hand[i]);
    draw_card(p3.hand[i]);

    outs(" ◆ ");
    /* - 牌桌的牌 ------------------------------------------------------- */
    draw_card(table[0][i]);
    outs("  ");
    draw_card(table[1][i]);
    outs("  ");
    draw_card(table[2][i]);
    outs("  ");
    draw_card(table[3][i]);
    outs("║\n");
  }
  while (user.hand[cur.n].num == XX) cur.n++;

  /*********************************************************************************
   * 礙於版面, 下面的輸出不得不這樣寫 = ="
   * 修改時先把雙引號後面的換行 delet掉看起來會比較整齊
   *********************************************************************************/

  prints("║ %s%s%s%s  ◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆◇◆ ║\n",   NAME[0], NAME[1],
         NAME[2], NAME[3]);
  prints("║   %d       %d       %d       %d     [剩下PASS次數]             "
         "           ◇ ║\n", 4-user.pass, 4-p1.pass, 4-p2.pass, 4-p3.pass);
  outs("╚════════════════╦════════════════"
       "════╣\n");
  outs("                 ╭────╮     ║   按鍵說明:                    "
       "        ║\n");
  outs("  這張牌是 ...   │        │     ║                                "
       "        ║\n");
  prints("                 │ %s   │     ║      ↑ [上一張]  R [重新開始] "
         "        ║\n", buf);
  outs("                 │        │     ║      ↓ [下一張]  Q [結束遊戲] "
       "        ║\n");
  outs("                 │        │     ║      Ｃ [ PASS ]               "
       "        ║\n");
  outs("  [空白鍵出牌]   ╰────╯     ╚════════════════"
       "════╝\n");

  outs("\n");
}


void sort_card (CARD *c)
{
  /* 氣泡排序法 */
  for (i = 0; i < 13; i++)
  {
    for (j = 0; j < 13-1; j++)
    {   /* 比花色 */
      if (c[i].flo < c[j].flo)
      {
        temp = c[i];
        c[i] = c[j];
    c[j] = temp;
      }
      else if (c[i].flo == c[j].flo)
      { /*  比點數 */
        /* 若牌不存在時其值為巨集 XX(也就是 100)所以會被推到最後 */
        if (c[i].num < c[j].num)
        {
          temp = c[i];
          c[i] = c[j];
          c[j] = temp;
        }
      }
    }
  }
}


int go (PLAYER *p)
{
  int over = 0;     /* 出完了沒 */

  /* - 超強 CIRC 外星科技 電腦出牌人工智慧模擬 ---------------------------- */
  /*                                                                        */
  /* *asdf  : 枚舉各種出牌優先順序("0123"的全排列)擇其一而用之              */
  /* fb_01_ : 隨機從大或小先接上去                                          */
  /*                                                                        */
  /* ---------------------------------------------------------------------- */
  char *asdf = ASDF[rand()%24];     /* 優先出牌花色順序 */
  int fb_01_ = rand() % 2;      /* 從大或小開始出牌 */


  for (i = 0; i < 4 && !over; i++)  /* 依序看各種花色是否有牌出 */
  {
    for (j = 0; j < 13; j++)        /* 檢視手中的牌 */
    {
      if (p->hand[j].flo == asdf[i]-'0'+1)      /* 花色正確 */
      {
        if (p->hand[j].num == fb[asdf[i]-'0'][fb_01_])  /* 點數正確 */
    {
      /* 更新桌上牌堆 */
          table[p->hand[j].flo-1][p->hand[j].num-1].num = p->hand[j].num;
      table[p->hand[j].flo-1][p->hand[j].num-1].flo = p->hand[j].flo;

      /* 更新前一章後一張 */
      fb[asdf[i]-'0'][fb_01_] = p->hand[j].num + (fb_01_? 1: -1);

      /* 把手中的牌丟掉 */
      p->hand[j].flo = p->hand[j].num = XX;

      over = 1;                 /* 出完了 */
      break;
    }
    if (p->hand[j].num == fb[asdf[i]-'0'][!fb_01_]) /* 點數正確 */
    {
          /* 更新桌上牌堆 */
          table[p->hand[j].flo-1][p->hand[j].num-1].num = p->hand[j].num;
      table[p->hand[j].flo-1][p->hand[j].num-1].flo = p->hand[j].flo;

      /* 更新前一章後一張 */
      fb[asdf[i]-'0'][!fb_01_] = p->hand[j].num - (fb_01_? 1: -1);

      /* 把手中的牌丟掉 */
      p->hand[j].flo = p->hand[j].num = XX;

          over = 1;                 /* 出完了 */
      break;
    }
      }
    }
  }
  if (!over) p->pass++;                 /* 沒出牌喔 */
  sort_card(p->hand);
  return over;
}


int throw_card (PLAYER *p)
{
  int over = 0;
  for (i = 0; i < 4; i++)
  {
    if (p->hand[cur.n].flo == i+1 && !over)     /* 花色正確 */
    {
      if (p->hand[cur.n].num == fb[i][0])       /* 點數正確 */
      {
    /* 更新桌上牌堆 */
        table[p->hand[cur.n].flo-1][p->hand[cur.n].num-1].num = p->hand[cur.n].num;
    table[p->hand[cur.n].flo-1][p->hand[cur.n].num-1].flo = p->hand[cur.n].flo;

    /* 更新前一章後一張 */
    fb[i][0] = p->hand[cur.n].num - 1;

    /* 把手中的牌丟掉 */
    p->hand[cur.n].flo = p->hand[cur.n].num = XX;

    over = 1;                   /* 出完了 */
    break;
      }
      if (p->hand[cur.n].num == fb[i][1])       /* 點數正確 */
      {
        /* 更新桌上牌堆 */
        table[p->hand[cur.n].flo-1][p->hand[cur.n].num-1].num = p->hand[cur.n].num;
    table[p->hand[cur.n].flo-1][p->hand[cur.n].num-1].flo = p->hand[cur.n].flo;

    /* 更新前一章後一張 */
    fb[i][1] = p->hand[cur.n].num + 1;

    /* 把手中的牌丟掉 */
    p->hand[cur.n].flo = p->hand[cur.n].num = XX;
    over = 1;                   /* 出完了 */
    break;
      }
    }
  }
  if (!over) p->pass++;     /* 沒出牌喔 */
  sort_card(p->hand);
  return over;
}


void new_cur (int n)
{
  /* 重新計算 cur.front與 cur.back的位置 */
  for (i = 0, cur.front = 0; i < 13; i++)
  {
    if (user.hand[i].num == XX) cur.front++;
    else break;
  }

  for (i = 12, cur.back = 12; i >= 0; i--)
  {
    if (user.hand[i].num == XX) cur.back--;
    else break;
  }
  /* n值決定游標的移動: n = 1 下移, n = 0不動, n = -1上移 */
  if (n > 0)
  {
    do {cur.n++;} while (cur.n < cur.back && user.hand[cur.n].num == XX);
    if (cur.n > cur.back) cur.n = cur.front;
  }
  else if (n == 0)
  {
    while (cur.n < cur.back && user.hand[cur.n].num == XX) cur.n++;
    if (cur.n > cur.back) cur.n = cur.front;
  }
  else if (n < 0)
  {
    do {cur.n--;} while (cur.n > cur.front && user.hand[cur.n].num == XX);
    if (cur.n < cur.front) cur.n = cur.back;
  }

  if (cur.front == cur.back) cur.n = cur.front;
}


int check (PLAYER *p)
{
  int ok;

  if (p->pass >= 4)     /* 使用四次 pass以上就輸囉 */
  {
    show_all();
    prints("%s have passed 4 times!!\twant to play another game? [y/n]", p->name);

    if (!strcmp(p->name, NAME[0]))
      lose();
    else
      win();

    while (1)
      switch (vkey())
      {
        case 'n': case 'N':
      return 2;
    case 'y': case 'Y':
      return 1;
      }
  }

  for (i = 0, ok = 1; i < 13; i++)
  {
    if (p->hand[i].num != XX)
    {
      ok = 0;               /* 手牌還有剩 */
      return 0;
    }
  }

  if (ok)                       /* 出光光 */
  {
    show_all();
    prints("%s win the game!!\tDo you want to play another game? [y/n]", p->name);

    if (!strcmp(p->name, NAME[0]))
      win();
    else
      lose();

    while (1)
      switch (vkey())
      {
        case 'n': case 'N':
          return 2;
    case 'y': case 'Y':
          return 1;
      }
  }
}


void win (void)
{
  vmsg("Bonus: ＄400");
  cuser.money += 400;
}

void lose (void)
{
  vmsg("Penalty: ＄100");
  cuser.money = cuser.money >= 100 ? cuser.money - 100: 0;
}


int main_sevens ()
{

          ;;;;;;;;; ;;;;;;   ;;;;;;;;     ;;;;;;;;
         ;;          ;;     ;;    ;;     ;;
        ;;          ;;     ;;;;;;;;     ;;
       ;;          ;;     ;;;;         ;;
      ;;          ;;     ;;  ;;       ;;
     ;;          ;;     ;;    ;;     ;;
    ;;;;;;;;  ;;;;;;   ;;      ;;   ;;;;;;;;

    /* xatier. 無聊用分號畫圖XDD */

  game_start:
  init();
  int ok = 0;       /* OK 意為玩家有出牌或PASS了, 即玩家宣告"結束這回合" */

  for (;;)
  {
    /* 排序, 整理手牌 */
    sort_card (user.hand);
    sort_card (p1.hand);
    sort_card (p2.hand);
    sort_card (p3.hand);

    new_cur(0);

    show_all();

    switch (vkey())
    {
      /* -Debug ------------------------------------------------------- */
      case 'A':
        ok = go(&user);
	if (!ok) user.pass--;       /* 沒出牌 */
	new_cur(0);
	break;
      case 'S':
	ok = 1;
	break;
      /* -End of Debug ------------------------------------------------ */

      case KEY_UP:
      case 'z': case 'Z':       /* 上移 */
	new_cur(-1);
	break;
      case KEY_DOWN:
      case 'x': case 'X':       /* 下移 */
	new_cur(1);
	break;
      case 'c': case 'C':       /* pass */
	ok = 1;
	user.pass++;
	break;
      case KEY_RIGHT:
      case ' ':             /* 出牌 */
	ok = throw_card(&user);
	if (!ok) user.pass--;       /* 沒出牌 */
	new_cur(0);
	break;
      case KEY_HOME:
        cur.n = cur.front;
        break;
      case KEY_END:
        cur.n = cur.back;
        break;
      case '\\':
	light = !light;         /* 開燈 */
	break;
      case 'q':case 'Q':
	goto game_over;         /* 離開 */
      case 'r':case 'R':        /* 重玩 */
	goto game_start;
    }

    switch (check(&user))
    {
      case 2: goto game_over;
      case 1: goto game_start;
      case 0: break;
    }

    if (ok)
    {
      /* - computer player 1 ------------------------------------------ */
      go(&p1);
      switch (check(&p1))
      {
        case 2: goto game_over;
        case 1: goto game_start;
        case 0: break;
      }
      /* - computer player 2 ------------------------------------------ */
      go(&p2);
      switch (check(&p2))
      {
        case 2: goto game_over;
    case 1: goto game_start;
    case 0: break;
      }
      /* - computer player 3 ------------------------------------------ */
      go(&p3);
      switch (check(&p3))
      {
        case 2: goto game_over;
        case 1: goto game_start;
        case 0: break;
      }
      ok = 0;
    }
  }
  game_over:
  return 0;
}

#endif
