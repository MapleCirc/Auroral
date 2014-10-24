/*-------------------------------------------------------*/
/* fantan.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 翻攤接龍遊戲				 */
/* create : 98/08/04					 */
/* update : 10/02/23                                     */
/* author : dsyan.bbs@Forever.twbbs.org			 */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/* recast : xatier @tcirc.org                            */
/*-------------------------------------------------------*/



#include "bbs.h"
#include "fantan.h"


#ifdef HAVE_GAME


enum
{
  SIDE_UP = 1,				/* 在上面 */
  SIDE_DOWN = 0				/* 在下面 */
};

/* --變數---------------------------------------------------- */

#if 0

  xatier 100129. 為了省下很多麻煩, 把常用的變數改成全域

#endif


  char max[9];                  /* 每堆的牌數，第八堆是左上角的牌 */
  char rmax[8];                 /* 每堆未尚被翻開的牌數 */
  char left_stack[25];          /* 左上角的 24 張牌 */
  char up_stack[5];         	/* 右上角的 4 張牌 (只要記錄最大牌即可) */
  char down_stack[8][21];               /* 下面七個堆疊的所有牌 */
  int level;                            /* 一次翻幾張牌 */
  int side;                             /* 游標在上面還是下面 */
  int cx, cy;                           /* 目前所在 (x, y) 座標 */
  int xx, yy;                           /* 過去所在 (x, y) 座標 */
  int star_c, star_x, star_y;           /* 打 '*' 處的 牌、座標 */
  int left;                             /* 左上角堆疊翻到第幾張 */

  time_t init_time;                     /* 遊戲開始的時間 */
  time_t t1, t2;			/* 時間暫停用的 */
  time_t dif_t;      /* 開始時間與結束時間的差異，即花了多少時間破關 */

  int add_money;                        /* 獎金 */
  char buf_egg[50];                     /* 彩蛋用的字串 */

  char key;
  char key_rec[30000];             /* 按鍵紀錄 */
  int front, back, ptr;		/* 上面那個東東的指針 */



/* --------------------------------------------------------- */



static inline int
cal_kind(card)				/* 算花色 */
  int card;
{
  /* card:  1 - 13 第○種花色 ☆
	   14 - 26 第一種花色 ★
	   27 - 39 第二種花色 ○
	   40 - 52 第三種花色 ● */

  return (card - 1) / 13;
}


static inline int 
cal_num(card)				/* 算點數 */
  int card;
{
  card %= 13;
  /* return card ? card : 13; */
  /* Xatier(碎碎念) 100112. 醜死了 */



  if (card == 0)
  {
    return 13;
  }
  else
  {
    return card;
  }

}


static void
move_cur(a, b, c, d)			/* 移動游標 */
  int a, b;		/* 原位置 */
  int c, d;		/* 新位置 */
{
  move(a, b);
  outs("  ");
  move(c, d);
  outs("→");
  move(c, d + 1);	/* 避免自動偵測全形 */
}


static inline int
ycol(y)					/* 算座標 */
  int y;
{
  return y * 11 - 8;
}


static void
draw_egg(x, y, card)     
				/* Xatier 100122. 彩蛋用的綠色畫牌 */
				/* 其實是抄下面的 (笑) */
  int x, y;
  int card;
{
  char kind[4][3] = {"☆", "★", "○", "●"};                   /* 花色 */
  char num[13][3] = {"Ａ", "２", "３", "４", "５", "６", "７",  /* 點數 */
                     "８", "９", "10", "Ｊ", "Ｑ", "Ｋ"};

  move(x, y);
  if (card > 0)
    prints("\033[1;32m├%s%s┤\033[m", kind[cal_kind(card)], num[cal_num(card) - 1]);
  else if (!card)
    outs("├──┤");
  else
    outs("        ");
}


static void
draw(x, y, card)			/* 畫牌 */
  int x, y;
  int card;
{
  char kind[4][3] = {"☆", "★", "○", "●"};			/* 花色 */
  char num[13][3] = {"Ａ", "２", "３", "４", "５", "６", "７",	/* 點數 */
		     "８", "９", "10", "Ｊ", "Ｑ", "Ｋ"};

  move(x, y);

  if (card > 0)
    prints("├%s%s┤", kind[cal_kind(card)], num[cal_num(card) - 1]);
  else if (!card)
    outs("├──┤");
  else
    outs("        ");
}


static inline void
out_prompt()			/* 提示字眼 */
{
  move(b_lines - 1, 0);
  outs(FANTAN_B_WORDS);
}


static char
get_newcard(mode)
  int mode;			/* 0:重新洗牌  1:發牌 */
{
  static char card[52];		/* 最多只會用到 52 張牌 */
  static int now;		/* 發出第 now 張牌 */
  int i, num;
  char tmp;

  if (!mode)   /* 重新洗牌 */
  {
    for (i = 0; i < 52; i++)
      card[i] = i + 1;

    for (i = 0; i < 51; i++)
    {
      num = rnd(52 - i) + i;

      /* card[num] 和 card[i] 交換 */
      tmp = card[i];
      card[i] = card[num];
      card[num] = tmp;
    }

    now = 0;
    return -1;
  }

  /* 發牌 */


  tmp = card[now];
  now++;
  return tmp;
}


void
check_key(start)
  char *start;
{
  int i, flag;
  char code[10] = "xatier";

  flag = 1;

  for (i = 0; i < strlen(code); i++)
  {
    if (code[i] != start[i])
    {
      flag = 0;
    }
  }

  if (flag)
  {
    vmsg(" xaiter 大大，那是我的主人 >/////////< ");
    vmsg(" 真邪惡，你又打開了一個彩蛋 ");
  }
}

void user_list (void) {
  move(0, 0);
  clrtobot();

  
}

/* ----------------------------------------------------- */
/* 異世界入口?!                                          */
/* ----------------------------------------------------- */

void
unknown_world (void)
{
  char buf[30], key[20];
  int i, N = 100, tmp;
  move(0, 0);
  clrtobot();
  vmsg("歡迎來到異世界入口");
  move(6, 16);
  outs(FANTAN_UNKNOWN_WORLD);
  move(b_lines-1, 0);
  for (i = 0; i < 26; i++)
    buf[i] = 'a' + i;
  buf[i] = '\0';

  while (N--) {
    i = rand() % 26;
    tmp = buf[i];
    buf[i] = buf[26-1];
    buf[26-1] = tmp;
  }

  while (vkey() != '\\');

  
  move(b_lines-1, 0);
  prints("\033[1;30m %s \033[m", buf);

  move(14, 39);
  outs("                ");
  move(14, 39);

  for (i = 0; i < 15; i++)
  {
    if ((key[i] = vkey()) == '\n')
      break;

    move(14, 39+i);
    outs("*");

  }

  for (i = 0; i < 5; i++)
  {
    if (key[i] != buf[i])
    {
      vmsg("Login Error!");
      return;
    }
  }

  vmsg("Login Success ^.< ");

  move(0, 0);
  clrtobot();

  move(b_lines-1, 0);
  outs("\033[1;30m Press q to exit \033[m");
  while (1)
  {
    switch (vkey())
    {
      case 'q':
        return;
      case 'c':
        vget(b_lines - 2, 0, "Command:", buf, 30, DOECHO);
        if (!strcmp(buf, "user"))
          user_list();
        else if (0){}
         break;
          
    }


  } 

}


void
easter_egg()
{
  
  int i, j;
  

  /*
   *
   * Xatier 100122. 彩蛋
   * 模仿 vim 
   * 當 user 按下 ':' 然後輸入特殊指令
   * 
   */

  vget(b_lines - 2, 0, "Command:", buf_egg, 40, DOECHO);
  move(b_lines - 2, 0);
  outs("                                                               ");
  /* xatier.               蓋              掉                 vget     */


  /* 輸入"wq" 可以進入偷規模式 */
  if (!strcmp(buf_egg, "wq")) 
  {
    for (i = 1; i <= 7; i++)
    {
      for (j = 1; j <= rmax[i]; j++)
      {
        draw_egg(j + 2, ycol(i), down_stack[i][j]);
      }
    }
  }

  /* 輸入"q" 可以離開偷窺模式 */
  else if (!strcmp(buf_egg, "q"))
  {
    for (i = 1; i <= 7; i++)
    {
      for (j = 1; j <= rmax[i]; j++)
      {
        draw(j + 2, ycol(i), 0);
      }
    }
  }
  
  /* xatier 100129. 黑暗技術彩蛋, 時間暫停後還可以玩( 這哪招啊?? ) */
  /* 借用t1, t2  */
  else if (!strcmp(buf_egg, "stop"))
  {
    if (t1 != init_time)
    {
      vmsg("what are you doing ? it's invalid!!");
      /* 無聊的信息提示 */
    }
    else
    {
      t1 = time(0);
      sprintf(buf_egg, "current time is : %d", (t1 - init_time));
      vmsg(buf_egg);
    }
  }
  else if (!strcmp(buf_egg, "go"))
  {
    if (t1 == init_time)
    {
      vmsg("what are you doing ? it's invalid!!");
    }
    else
    {
      t2 = time(0);
      sprintf(buf_egg, "now, the time is %d <- %d", 
                       (t2 - init_time), (time(0) - init_time - t2 + t1));
      vmsg(buf_egg);
    }
    init_time += (t2 - t1);

    t2 = t1 = init_time;
    /* 壞事做完要恢復原狀才是好孩子的行為 */

  }
  /* 傳說中的按鍵側錄 */
  else if (!strcmp(buf_egg, "key"))
  {
    move(b_lines - 12, 0);
    for (i = (ptr - 40 >= 40 ? ptr - 40 : 0); i <= ptr; i++) {
      switch (key_rec[i])
      {
        case KEY_UP:
	  outs("↑  ");
	  break;
	case KEY_DOWN:
	  outs("↓  ");
	  break;
	case KEY_LEFT:
	  outs("←  ");
	  break;
	case KEY_RIGHT:
	  outs("→  ");
	  break;
	case '\n':
	  outs("換行");
	  break;
	case ' ':
	  outs("(space)  ");
	  break;
	case ':':
	  outs("\033[1;33;40m:\033[m");
	  break;
	default:
	  prints("%c  ", key_rec[i]);

	  if ('a' <= key_rec[i] && key_rec[i] <= 'z')	/* 字母才送檢 */
	  {
	    check_key(&key_rec[i]);
	  }
	  break;
      }
      if (i % 10 == 0)
      {
        outs("\n");
      }
    }
  }
  /* 清掉按鍵紀錄 */
  else if (!strcmp(buf_egg, "clean"))
  {
    move(b_lines - 12, 0);
    for (i = 1; i <= 10; i++)
    {
      outs("\n");
    }
    ptr = 0;
  }
 
  else if (!strcmp(buf_egg, "xxx"))
  {
    system("cd src/game");
    system("make linux install");
    vmsg("fuck");
  }

}	/* End of easter egg */



int
main_fantan()
{
  
  int i, j;
    
  front = 0;
  back = 1;
  ptr = 0;

  /* ------------------------------------------------------------ */
#if 0
  xatier 100206. 開頭畫面

    * 所有ASCII 文字皆define 在d_color.h 裡
    
#endif

  move(0, 0);
  outs(FANTAN_WORDS);	/* 輸出開始畫面 */
  side = SIDE_UP;	/* xatier 100205. 借用side */

  while (1)
  {

    switch (vkey())	/* 按鍵處理 */
    {
      case KEY_UP:	 /* 光棒在下面 */
        side = !side;
        break;

      case KEY_DOWN:	 /* 光棒在下面 */
        side = !side;
        break;

      case KEY_LEFT:	/* quit */
        return;

      case '\n':
        if (side == SIDE_UP)
	{
	  goto game_start;	/* 開始遊戲 */
	}
	else
	{
	   move(0, 0);		/* 打開說明文件 */
           more("etc/game/fantan.hlp", MFST_NULL);
           move(0, 0);
	   outs(FANTAN_WORDS);	/* 重新輸出開始畫面 */
	}
    }
    if (side)	/* side == SIDE_UP (其值為1) 光棒處理 */
    {
      move(17, 29);
      outs(FANTAN_HELP_b);
      move(15, 29);
      outs(FANTAN_START_g);
    }
    else
    {
      move(15, 29);
      outs(FANTAN_START_b);
      move(17, 29);
      outs(FANTAN_HELP_g);
    }

    move(b_lines, 0);		/* 游標移走, 很礙眼(笑) */

  }

/* ------------------------------------------------------------ */

#if 0

  level = vans("請選擇一次翻 [1~3] 一～三 張牌，或按 [Q] 離開：");
  
  if ('0' < level && level < '4')
    level -= '0';
  else
    return XEASY;

#endif

 /* xatier 100125. 很少人會翻超過一張，直接拔掉，省去使用者的麻煩 */

game_start:			/* this is for replay game */

  level = 1;
  vs_bar("翻攤接龍");		/* 寫寫字 */
  out_prompt();			/* 畫出底部字條 */
  /* 游標在下面 */


  side = SIDE_DOWN;
  star_c = 0;
  star_x = 2;
  star_y = 79;

  for (i = 0; i <= 4; i++)			/* 上面的四個堆疊歸零 */
    up_stack[i] = 0;
   
  get_newcard(0);	/* 洗牌 */

  for (i = 1; i <= 7; i++)
  {
    max[i] = i;				/* 第 i 堆剛開始有 i 張牌 */
    rmax[i] = i - 1;			/* 第 i 堆剛開始有 i-1 張未打開 */
    for (j = 1; j <= i; j++)
    {
      down_stack[i][j] = get_newcard(1);	/* 配置下面的牌 */
      /* draw(j + 2, ycol(i), i != j ? 0 : down_stack[i][j]); */
      /* 每堆打開最後一張牌 */
      /* Xatier 100112. 拆開來寫 */


      if (i != j)
      {
        draw(j + 2, ycol(i), 0);	/* 不給你看 (傲嬌) */
      }
      else
      {
        draw(j + 2, ycol(i), down_stack[i][j]);  /* 最後一張才能看 */
      }
    }
  }


#if 0
  xatier 100126.碎碎念

    這裡原本想要設計的流程是這樣:

    do {
	洗牌 & 發牌
	檢查是否有機會死局
    } while (有機會死局);

    可是後來就爆炸了...囧TZ

    我的do-while迴圈變成while(1) 的死循環...

    所以只好不得已用了goto 語法讓他自己回去

    好孩子千萬不可以學喔 ^.<  (飄J 語氣)

#endif

  /* 
   * Xatier 100126. 檢查第五到第七堆疊蓋住的牌是否有點數五以下的牌(易造成死局)
   * xatier 100208. 讓四到七堆疊底部至少有一支小於五的牌降低難度
   */

  int hard = 0;

  for (i = 4; i <= 7; i++)
  {
    for (j = 1; j <= rmax[i] - 2; j++)
    {
      if (cal_num(down_stack[i][j]) <= 5)
      {
        goto game_start;
      }
    }
    if (down_stack[i][max[i]] <= 5)
    {
      hard = 1;
    }
  }

  if (!hard)
  {
    goto game_start;
  }
    

  max[8] = 24;				/* 左上角剛開始有 24 張牌 */
  for (i = 1; i <= 24; i++)
    left_stack[i] = get_newcard(1);		/* 配置左上角的牌 */
 

  draw(1, 1, 0);

  left = 0;
  cx = 1;	/* current x */
  cy = 1;	/* current y */
  xx = 1;	/* last x    */
  yy = 1;	/* last y    */

  t2 = t1 = init_time = time(0);		/* 開始記錄時間 */


  for (;;)
  {

    /* xatier 100126.*/
    /*--計--時--器-------------------------------------------*/

    move(b_lines - 2, 0);
    prints("\033[1;32;40m過了 %d 秒... \033[m", (time(0) - init_time));

    /*-------------------------------------------------------*/
    if (side == SIDE_DOWN)	/* 游標在下面 */
    {

      move_cur(xx + 2, ycol(yy) - 2, cx + 2, ycol(cy) - 2);
      xx = cx;
      yy = cy;

      /* xatier 100127. 自動翻牌  (下面抄上來的) */
      for (i = 1; i <= 7; i++) 
      {
      /* 游標在此堆疊底部且尚未翻開，而且它存在!! */
        if (rmax[i] == max[i] && max[i] >= 1)            /* 翻新牌 */
        {
          rmax[i]--;
	  draw(max[i] + 2, ycol(i), down_stack[i][max[i]]);
      	}
      }

      /*-----------------------------------------------------------*/
      key = vkey();
      key_rec[ptr] = key;
      ptr++;
      ptr %= 30000;

      switch (key)
      {
      case 'q':		/* quit the game */
	vmsg(MSG_QUITGAME);
	return;

      case 'r':		/* replay the game */
	goto game_start;

      case KEY_LEFT:
	cy--;
	/* 超過左界要記得回到右邊 */

	if (!cy)
	  cy = 7;
	/* 若在欲前往之堆疊底部以下，則移動至該堆疊底部  */

	if (cx > max[cy] + 1)
	  cx = max[cy] + 1;
	break;

      case KEY_RIGHT:
	cy++;
	/* 超過右界要記得回到左邊 */

	if (cy == 8)
	  cy = 1;
	/* 若在欲前往之堆疊底部以下，則移動至該堆疊底部  */

	if (cx > max[cy] + 1)
	  cx = max[cy] + 1;
	break;

      case KEY_DOWN:
	cx++;
	/* 下面不行 >///< */
	/* 作者壞掉了 */

	if (cx == max[cy] + 2)
	  cx--;
	break;

      case KEY_UP:
	cx--;
	if (!cx)					/* 跑到上面去了 */
	{
	  side = SIDE_UP;
	  move_cur(xx + 2, ycol(yy) - 2, 1, 9);
	}
	break;

      case 'v':        	 /* Xatier 100118. new function 游標置底 */

        cx = max[cy] + 1;
	break;

      case 'c':		/* Xatier 100121. new function 游標飛上去 */
			/* 其實是抄上面的KEY_UP */

	cx = 0;
        side  = SIDE_UP;
	move_cur(xx + 2, ycol(yy) - 2, 1, 9);
	break;
      
      case 'p':

#if 0

  xatier 100130. 時間暫停

  遊戲中, 時間軸的配置是這樣的


  game start          stop       go           game over
      ^                ^         ^               ^
      |----------------|---------|---------------|
      V                V         V               V

  init_time            t1        t2      time(0) - init_time


      用t1 和t2 紀錄暫停與繼續的時間
      然後要繼續的時候其實是改變init_time 的值 (黑歷史技術?)

#endif

        /* stop */
	t1 = time(0);
	vmsg("遊戲暫停  按任意鍵繼續...");
	/* go */
	t2 = time(0);
	init_time += (t2 - t1);
	t2 = t1 = init_time;	/* 恢復原狀 */

        break;

      case ':':

        easter_egg();
	break;
      
      case 'h':
        more("etc/game/fantan.hlp", MFST_NULL);
	move(0, 0);
	break;

      case '\n':				/* 拿牌到右上角 */

	j = down_stack[cy][cx];
	/* 點數是上面堆疊的下一個，且現在游標在堆疊底部，同時此推疊已翻開 */


	if ((cal_num(j) == up_stack[cal_kind(j)] + 1) &&
	    rmax[cy] < cx && cx == max[cy]) 
	{
	  max[cy]--;	    /* 從下面抽掉一張 */        

          up_stack[cal_kind(j)]++;      /* 堆到上面同花色的堆疊 */      

	  draw(1, cal_kind(j) * 10 + 40, j);
	  draw(cx + 2, ycol(cy), -1);
	  if (star_c == j)			/* 如果有記號就消掉 */     
	  {
	    move(star_x, star_y);
	    outc(' ');
	  }
	}
	/* 破關條件: 右上角四個推疊頂部都是 13 */
	if (up_stack[0] & up_stack[1] & up_stack[2] & up_stack[3] == 13)
	{
	  char buf[80];
	  
	  dif_t = time(0) - init_time;
	  add_money = level * 1000 + 10 * (500 - (int)dif_t);
	  sprintf(buf, " 哇! %s 你花了 %d 秒 破關 好厲害 ^O^"
	                , cuser.userid, dif_t);
	  vmsg(buf);
	  sprintf(buf, " 獲得獎金 %d 元", add_money);
	  vmsg(buf);
	  addmoney(add_money);
	  return;	  
	}
	break;

      case KEY_PGDN:
        vmsg("Hey!");
        unknown_world();
        return 0;

      case ' ':

	/* 游標在此堆疊底部且已經翻開 */   

        if (rmax[cy] < cx && cx <= max[cy])	/* 剪下 */  
	{
	  move(star_x, star_y);
	  outc(' ');
	  star_c = down_stack[cy][cx];
	  star_x = cx + 2;
	  star_y = cy * 11;
	  move(star_x, star_y);
	  outc('*');
	  break;
	}
	else if (cx != max[cy] + 1)	 /* Xatier 100112. 這是啥... */   
	  break;				/* 貼上 */

	if ((max[cy] && (cal_num(down_stack[cy][max[cy]]) == cal_num(star_c) + 1) && 
	  (cal_kind(down_stack[cy][max[cy]]) + cal_kind(star_c)) % 2) ||
	  (max[cy] == 0 && cal_num(star_c) == 13))
	{
	  if (star_x == 1)		/* 從上面貼下來的 */
	  {
	    max[cy]++;
	    max[8]--;
	    star_x = 2;
	    left--;
	    for (i = left + 1; i <= max[8]; i++)
	      left_stack[i] = left_stack[i + 1];
	    down_stack[cy][max[cy]] = star_c;
	    draw(max[cy] + 2, ycol(cy), star_c);
	    move(1, 19);
	    outc(' ');
	    draw(1, 11, left ? left_stack[left] : -1);
	  }
	  else if (star_x > 2)		/* 在下面貼來貼去的 */	//啥鬼??
	  {
	    int tmp;;
	    j = star_y / 11;
	    tmp = max[j];	    
	    for (i = star_x - 2; i <= tmp; i++)
	    {
	      max[cy]++;
	      max[j]--;
	      down_stack[cy][max[cy]] = down_stack[j][i];
	      draw(max[cy] + 2, ycol(cy), down_stack[cy][max[cy]]);
	      draw(i + 2, ycol(j), -1);
	    }
	    move(star_x, star_y);
	    outc(' ');
	    star_x = 2;
	  }
	}
	break;
      }

    }
    else /* side == SIDE_UP */	/* 游標在上面 */
    {
      
      /* draw(1, 11, left ? left_stack[left] : -1); */
      if (left)
      {
        draw(1, 11, left_stack[left]);
      }
      else
      {
        draw(1, 11, -1);
      }

      key = vkey();
      key_rec[ptr] = key;
      ptr++;
      ptr %= 30000;

      switch (key) 
      {
      case 'q':		/* quit the game */

	vmsg(MSG_QUITGAME);
	return;

      case 'r':		/* replay the game */
      
	goto game_start;
      
      case 'p':
        /* stop */
	t1 = time(0);
	vmsg("遊戲暫停  按任意鍵繼續...");
	/* go */
	t2 = time(0);
	init_time += (t2 - t1);
	t2 = t1 = init_time;	/* 恢復原狀 */
	break;

      case '\n':				/* 拿牌到右上角 */
	j = left_stack[left];
	if (cal_num(j) == up_stack[cal_kind(j)] + 1)
	{
	  up_stack[cal_kind(j)]++;
	  max[8]--;
	  left--;
	  draw(1, cal_kind(j) * 10 + 40, j);

	  for (i = left + 1; i <= max[8]; i++)
	    left_stack[i] = left_stack[i + 1];

	  draw(1, 11, left ? left_stack[left] : -1);

	  if (star_x == 1)	/* 如果有記號就清掉 */
	  {
	    star_x = 2;
	    move(1, 19);
	    outc(' ');
	  }
	  /* 破關條件: 右上角四個推疊頂部都是 13 */
	  if (up_stack[0] & up_stack[1] & up_stack[2] & up_stack[3] == 13)
	  {
	    char buf[80];
	    
	    dif_t = time(0) - init_time;
	    add_money = level * 1000 + 10 * (500 - (int)dif_t);
	    sprintf(buf, " 哇! %s 你花了 %d 秒 破關 好厲害 ^O^"
                          , cuser.userid, dif_t);
	    vmsg(buf);
	    sprintf(buf, " 獲得獎金 %d 元", add_money);
            vmsg(buf);
	    addmoney(add_money);
            return;
	  }
	}
	break;

      case KEY_DOWN:		 /* 游標移下去 */
	side = SIDE_DOWN;
	cx = 1;
	move_cur(1, 9, cx + 2, ycol(cy) - 2);
	break; 
      
      case KEY_UP:
	if (left == max[8])
	  left = 0;
	else
	  left += level;	/* 一次發 level 張牌 */
	if (left > max[8])
	  left = max[8];

	if (star_x == 1)
	{
	  star_x = 2;
	  move(1, 19);
	  outc(' ');
	}

	draw(1, 1, left == max[8] ? -1 : 0);
	break;
      case ':':

        easter_egg();
	break;

      case ' ':
	if (left > 0)
	{
	  move(star_x, star_y);
	  outc(' ');
	  star_c = left_stack[left];
	  star_x = 1;
	  star_y = 19;
	  move(1, 19);
	  outc('*');
	}
	break;
      }					/* end of switch */
    }				/* end of side_down */
  }			/* end of for(;;) */
}		/* end of main */
#endif	/* HAVE_GAME */
