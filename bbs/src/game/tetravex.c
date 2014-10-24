/*-------------------------------------------------------*/
/* tetravex.c                                            */
/*-------------------------------------------------------*/
/* target : 方塊拼圖遊戲                                 */
/* create : 10/03/12                                     */
/* author : xatier @tcirc.org                            */
/* Ubuntu 上的經典遊戲 tetravex                          */
/*-------------------------------------------------------*/



#include "bbs.h"
#ifdef HAVE_GAME


#define _1_ "\033[m\033[30;47m１\033[m"		/* 數字 */
#define _2_ "\033[1;32;41m２\033[m"
#define _3_ "\033[1;32;42m３\033[m"
#define _4_ "\033[1;36;44m４\033[m"
#define _5_ "\033[1;33;45m５\033[m"
#define _6_ "\033[1;36;46m６\033[m"
#define _7_ "\033[1;33;44m７\033[m"
#define _8_ "\033[1;32;45m８\033[m"
#define _9_ "\033[1;33;46m９\033[m"
#define _0_ "\033[1;44m０\033[m"

#define _B_ "\033[m\033[1;30m█\033[m"		/* 方塊 */
#define _C_ "\033[m\033[1;33m⊕\033[m"		/* 游標 */
#define _F_ "\033[m\033[1;36m※\033[m"		/* 旗幟 */





enum
{
  SIDE_L = 1,                        /* 在左邊 */
  SIDE_R = 0                         /* 在右邊 */
};

int SIDE;

int cheat[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};	/* 作弊用的彩蛋 */

int i, j;		/* 萬用 i, j */

time_t init_time;       /* 遊戲開始的時間 */
time_t t1, t2;          /* 時間暫停用的 */
time_t dif_t;           /* 開始時間與結束時間的差異，即花了多少時間破關 */
int bonus;		/* 獎金 */
int light;		/* 燈泡 */


typedef struct          /* 方塊結構體 */
{
  int up;               /* 上 */
  int down;             /* 下 */
  int left;             /* 左 */
  int right;            /* 右 */
  int x;                /* X 座標 */
  int y;                /* Y 座標 */
  int no;               /* 編號 */
} Block;

#if 0
  大概長這樣:

        --------
        |  上  |
        |左  右|
        |  下  |
        --------

  其中
    no = x + 3*y
    x = no % 3
    y = no / 3
#endif


Block user[9], puzzle[9], ans[9], copy_user[9];
/* 右邊待拼的 左邊拼一半的 解答 複製使用者的那塊 */
Block null;	/* 空的方塊 */

typedef struct			/* 游標結構體 */
{
  int x;			/* X 座標 */
  int y;			/* Y 座標 */
  int flag;			/* 是否被標記 */
  int catch;			/* 抓哪一塊 */
  int side;			/* 抓哪一邊 */
} CUR;

CUR cur;			/* 我是游標 */
int cur_map[3][6];		/* 游標座標 */

#if 0
  游標的座標系是3*6的長方形

	  0 1 2 3 4 5
	  __________
	0|          |
	1|          |
	2|__________|

 6, 12   6, 20   6, 28 		右邊 X  +36
10, 12  10, 20  10, 28
14, 12  14, 20  14, 28


  val:
    1 = 有格子
    2 = 旗幟
    0 = 空格

#endif


#if 0
   地圖、右邊待拼的、解答

   九宮格長這樣

        X 0   1   2
        -------------
     Y  |   |   |   |  5, 10   5, 18   5, 26   右邊 X  +36
     0  | 0 | 1 | 2 |  6, 10   6, 18   6, 26
        |   |   |   |  7, 10   7, 18   7, 26
        |------------
        |   |   |   |  9, 10   9, 18   9, 26
     1  | 3 | 4 | 5 | 10, 10  10, 18  10, 26
        |   |   |   | 11, 10  11, 18  11, 26
        |------------
        |   |   |   | 13, 10  13, 18  13, 26
     2  | 6 | 7 | 8 | 14, 10  14, 18  14, 26
        |   |   |   | 15, 10  15, 18  15, 26
        -------------


#endif

void init (void);
void draw_all (void);
void out_foot (void);
void draw_num (int);
void swap_block (Block *, Block *);
void draw_block (int , Block);
void draw_cur (int);
void x_cur (void);
int check_ans (void);
void move_block (void);



void
init (void)
{
  int N = 50;		/* N is for swap user[] */

  /* init the NULL block */
  null.up = -1;
  null.down = -1;
  null.left = -1;
  null.right = -1;
  null.x = -1;
  null.y = -1;
  null.no = -1;

  int fill[12];         /* 填的數字 */

  for (i = 0; i < 9; i++)
  {
    puzzle[i] = null;		/* 清空puzzle[] */

    /* 因為填在四邊的數字可以為0~9，所以用 -1來初始化 */
    ans[i].up = ans[i].down = ans[i].left = ans[i].right = -1;

    /* 初始化 puzzle[] 和 user[] 的編號與座標 */
    puzzle[i].no = ans[i].no = i;
    puzzle[i].x  = ans[i].x  = ans[i].no % 3;
    puzzle[i].y  = ans[i].y  = ans[i].no / 3;

    cheat[i] = i + 1;		/* 作弊的彩蛋 */
  }

  
  /* init the cur_map[][] */
  for (i = 0; i < 3; i++)
    for (j = 0; j < 6; j++)
      cur_map[i][j] = 0;


  for (i = 0; i < 12; i++)
    fill[i] = rand() % 10;      /* 產生0~9 的12 個亂數 */
  

  /* 初始化解答，相鄰的點數要一樣 */
  ans[0].right = ans[1].left = fill[0];
  ans[1].right = ans[2].left = fill[1];

  ans[3].right = ans[4].left = fill[2];
  ans[4].right = ans[5].left = fill[3];

  ans[6].right = ans[7].left = fill[4];
  ans[7].right = ans[8].left = fill[5];

  ans[0].down = ans[3].up = fill[6];
  ans[1].down = ans[4].up = fill[7];
  ans[2].down = ans[5].up = fill[8];

  ans[3].down = ans[6].up = fill[9];
  ans[4].down = ans[7].up = fill[10];
  ans[5].down = ans[8].up = fill[11];


  /* 補滿其他格子 */
  for (i = 0; i < 9; i++)
  {
    if (ans[i].up == -1)
      ans[i].up = rand() % 10;

    if (ans[i].down == -1)
      ans[i].down = rand() % 10;

    if (ans[i].right == -1)
      ans[i].right = rand() % 10;

    if (ans[i].left == -1)
      ans[i].left = rand() % 10;
  }

  /* 將解答複製到user[] */
  for (i = 0; i < 9; i++)
  {
    user[i] = ans[i];
  }
  

  /* 隨機交換user[] */
  while (N--)
  {
    int t;
    i = rand() % 9;
    j = ((2 * i) % 9);
    swap_block(&user[i], &user[j]);

    t = cheat[i];
    cheat[i] = cheat[j];
    cheat[j] = t;
  }


  /* 畫出框框 */
  move(4, 0);
  outs(TetraVex_BORDER);

  for (i = 0; i < 9; i++)
  {
    draw_block(0, user[i]);	/* 畫出user[] */
    draw_block(1, puzzle[i]);
    
    /* 順便設定 cur_map[][]裡的值 (右邊填滿) */
    cur_map[i / 3][i % 3 + 3] = 1;
    cur_map[i / 3][i % 3] = 0;

    /* 把user[] 複製到 copy_user[] */
    copy_user[i] = user[i];

  }

  /* init our cursor */
  cur.x = 3;
  cur.y = 0;
  cur.flag = 0;

  light = 0;			/* turn off the light */

  SIDE = SIDE_R;		/* we are in the right side ^.< */
  t2 = t1 = init_time = time(0);                /* 開始記錄時間 */
}

void
draw_all (void)			/* 繪出所有的格子 */
{
  for (i = 0; i < 9; i++)
  {
    draw_block(0, user[i]);
    draw_block(1, puzzle[i]);
  }
}



void
out_foot (void)                    /* 底部提示字眼 */
{
  move(b_lines - 1, 0);
  outs(TetraVex_FOOT);
}



void
draw_num (n)			/* 畫出數字 */
int n;
{
  switch (n)
  {
    case 1:
      outs(_1_);
      break;
    case 2:
      outs(_2_);
      break;
    case 3:
      outs(_3_);
      break;
    case 4:
      outs(_4_);
      break;
    case 5:
      outs(_5_);
      break;
    case 6:
      outs(_6_);
      break;
    case 7:
      outs(_7_);
      break;
    case 8:
      outs(_8_);
      break;
    case 9:
      outs(_9_);
      break;
    case 0:
      outs(_0_);
      break;
    case -1:
      outs("  ");
      break;
  }
}


void
swap_block (a, b)		/* 交換Block的數字，但是位置不交換 */
Block *a;
Block *b;
{
  int temp;
  Block Temp;

  Temp = *a;			/* 先整顆換過來 */
  *a = *b;
  *b = Temp;

  temp = a->x;			/* 其他的再換回來 */
  a->x = b->x;
  b->x = temp;

  temp = a->y;
  a->y = b->y;
  b->y = temp;

  temp = a->no;
  a->no = b->no;
  b->no = temp;

}


void 
draw_block (side, B)		/* side = 1 左邊，side = 0 右邊 */
int side;
Block B;
{
  int px, py;			/* 畫筆的 X Y 座標 */
  py = 5 + 4 * B.y;
  px = 10 + 8 * B.x;

  if (!side)			/* 在右邊的話 X 要加36 */
    px += 36;

#if 0

　　方塊:

上      █１█
中	１█１
下	█１█

#endif

  /* 出了四邊都是零的狀況例外處理 */
  if (B.up == B.down && B.down == B.left)
    if (B.left == B.right && B.right == 0)
      vmsg("噢不! 程式出了問題，發生NULL方塊事件，請洽開發者xatier處理");


  move(py, px);			/* 上 */
  outs("  ");
  draw_num(B.up);
  outs("  ");

  py++;				/* 中 */
  move(py, px);
  draw_num(B.left);
  move(py, px+4);		/* 中間空著 */
  draw_num(B.right);

  py++;				/* 下 */
  move(py, px);
  outs("  ");
  draw_num(B.down);
  outs("  ");

}



void
draw_cur (kind)			/* 顯示游標 kind: 0 = 游標; 1 = 旗幟 */
int kind;
{
  /* 游標跑到了奇怪的地方 */
  if (cur.x > 5 || cur.x < 0)
    vmsg("噢不! 程式出了問題，游標移到世界盡頭之外，請洽開發人員xatier處理");

  if (SIDE)
  {
    move(6 + 4 * cur.y, 12 + 8 * cur.x);
  }
  else
  {
    move(6 + 4 * cur.y, 12 + 36 + 8 * (cur.x - 3));
  }

  if (kind == 1)
    outs(_F_);
  else if (kind == 0)
    outs(_C_);

  move(b_lines - 1, 0);		/* 把游標移走 */
}



void
x_cur (void)
{
  int foo = 0;
  /* foo用來記錄旗幟的數目，如果大於一支就是有問題 */

  for (i = 0; i < 3; i++)		/* 左邊 */
  {
    for (j = 0; j < 3; j++)
    {
      move(6 + 4 * i, 12 + 8 * j);
      if (!cur_map[i][j] || cur_map[i][j] == 1)
      {
        outs("  ");
      }
      else if (cur_map[i][j] == 2)	/* 這格為升旗狀態 */
      {
        outs(_F_);
        foo++;
      }
    }
  }

  for (i = 0; i < 3; i++)		/* 右邊 */
  {
    for (j = 3; j < 6; j++)
    {
      move(6 + 4 * i, 12 + 36 + 8 * (j-3));
      if (!cur_map[i][j] || cur_map[i][j] == 1)
      {
        outs("  ");
      }
      else if (cur_map[i][j] == 2)
      {
        outs(_F_);
        foo++;
      }
    }
  }
  
  /* 出了兩支旗幟的例外處理 */
  if (foo > 1)
    vmsg("噢不! 程式出了問題，發生旗幟殘影事件，請洽開發者xatier處理");


  /* 現在游標位置 */
  draw_cur(0);
}


int
check_ans (void)
{

  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
      if (cur_map[i][j] == 0)		/* 如果有空格，就非正解 */
        return 0;
    }
  }


  int ker = 1;		/* 判斷是否各數值都相同 */
  for (i = 0; i < 9; i++)
  {
    /* 反正就是有不一樣的就不是正解 */
    if (puzzle[i].up    != ans[i].up)
      ker = 0;
    if (puzzle[i].down  != ans[i].down)
      ker = 0;
    if (puzzle[i].left  != ans[i].left)
      ker = 0;
    if (puzzle[i].right != ans[i].right)
      ker = 0;
  }

  if (!ker)
    return 0;

  if (						/* 多重解避免 -> 手動硬幹 */
	  puzzle[0].right == puzzle[1].left &&
	  puzzle[1].right == puzzle[2].left &&

	  puzzle[3].right == puzzle[4].left &&
	  puzzle[4].right == puzzle[5].left &&

	  puzzle[6].right == puzzle[7].left &&
	  puzzle[7].right == puzzle[8].left &&

	  puzzle[0].down == puzzle[3].up &&
	  puzzle[1].down == puzzle[4].up &&
	  puzzle[2].down == puzzle[5].up &&

	  puzzle[3].down == puzzle[6].up &&
	  puzzle[4].down == puzzle[7].up &&
	  puzzle[5].down == puzzle[8].up 
  )
  {
    return 1;					/* 看起來很三小的寫法 XD */
  }

}

void				/* 公開解答XD */
wow (void)
{
  int px, py;                   /* 畫筆的 X Y 座標 */


  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
      py =  5 + 4 * j;
      px = 10 + 8 * i + 36;
      move(py, px);                 /* 上 */
      outs("      ");
      move(py+1, px);
      outs("      ");
      move(py+2, px);
      outs("      ");
      usleep(200);
      draw_block(1, ans[j * 3 + i]);

    }
  }
}


void
move_block (void)
{
  
  if (cur.flag)			/* 已升旗 */
  {
    if (!SIDE)		/* 右邊 */
    {
      /* 這格是空的 */
      if (cur_map[cur.y][cur.x] == 0)
      {
        if (cur.side) 				/* 抓的那塊在左邊 */
        {
          /* 交換 */
          swap_block(&puzzle[cur.catch], &user[3 * cur.y + (cur.x - 3)]);

          /* cur_map[][] 更新 */
          cur_map[cur.y][cur.x] = 1;
          cur_map[cur.catch / 3][cur.catch % 3] = 0;

          /* 降旗 */
          cur.flag = 0;
        }
        else					/* 抓的那塊在右邊 */
        {
          /* 交換 */
          swap_block(&user[cur.catch], &user[3 * cur.y + (cur.x - 3)]);

          /* cur_map[][] 更新 */
          cur_map[cur.y][cur.x] = 1;
          cur_map[cur.catch / 3][cur.catch % 3 + 3] = 0;

          /* 降旗 */
          cur.flag = 0;
        }
      }
      else			/* 這格不是空的，在這裡升旗 */
      {
        /* 不能在同一格 */
        if (! ( (cur.catch / 3 == cur.y) && 
              ( (cur.catch % 3 + 3 * !cur.side) == cur.x) ) )
        {
          /* cur_map[][] 更新 */
          cur_map[cur.catch / 3][cur.catch % 3 + 3 * !cur.side] = 1;
          cur_map[cur.y][cur.x] = 2;

          /* 升旗 */
          cur.flag = 1;

          /* 抓取相關資料 */
          cur.side = SIDE;
          cur.catch = 3 * cur.y + (cur.x - 3);
        }
      }
    }
    else			/* 左邊 */
    {
      /* 這格是空的 */
      if (cur_map[cur.y][cur.x] == 0)
      {
        if (cur.side)                           /* 抓的那塊在左邊 */
        {
          /* 交換 */
          swap_block(&puzzle[cur.catch], &puzzle[3 * cur.y + cur.x]);

          /* cur_map[][] 更新 */
          cur_map[cur.y][cur.x] = 1;
          cur_map[cur.catch / 3][cur.catch % 3] = 0;

          /* 降旗 */
          cur.flag = 0;
        }
        else                                    /* 抓的那塊在右邊 */
        {
          /* 交換 */
          swap_block(&user[cur.catch], &puzzle[3 * cur.y + cur.x]);

          /* cur_map[][] 更新 */
          cur_map[cur.y][cur.x] = 1;
          cur_map[cur.catch / 3][cur.catch % 3 + 3] = 0;

          /* 降旗 */
          cur.flag = 0;
        }
      }
      else                                      /* 這格不是空的，那就在這裡升旗 */
      {
        /* 不能再同一格處理 */
        if (!( (cur.catch / 3 == cur.y) &&
               ((cur.catch % 3 + 3 * !cur.side) == cur.x) ) )
        {
          /* cur_map[][] 更新 */
          cur_map[cur.catch / 3][cur.catch % 3 + 3 * !cur.side] = 1;
          cur_map[cur.y][cur.x] = 2;

          /* 升旗 */
          cur.flag = 1;

          /* 抓取相關資料 */
          cur.side = 1;
          cur.catch = 3 * cur.y + cur.x;
        }
      }
    }
  }
  else		/* 還沒升旗 */
  {
    /* 這格要有東西啊!! */
    if (cur_map[cur.y][cur.x] == 1)
    {
      /* 升旗 */
      cur.flag = 1;

      /* 抓一下相關資料 */
      cur.side = SIDE;
      if (!SIDE)
      {
        cur.catch = 3 * cur.y + (cur.x - 3);
      }
      else
      {
        cur.catch = 3 * cur.y + cur.x;
      }

      /* cur_map[][] 更新 */
      cur_map[cur.catch / 3][cur.catch % 3 + 3 * !cur.side] = 0;
      cur_map[cur.y][cur.x] = 2;
    }
  }
}




int
main_tetra (void)
{

  
  game_start:		/* 遊戲重新開始 */

  move(0, 0);		/* 清屏 */
  clrtobot();

  out_foot();		/* 腳腳>////< */

  char key;		/* 按鍵 */
  
  init();
  draw_all();
  move(b_lines - 1, 0);
  x_cur();

  
  while (1)
  {
    /*--計--時--器-------------------------------------------*/

    move(b_lines - 2, 0);
    prints("\033[1;32;40m過了 %d 秒... \033[m", (time(0) - init_time));

    /*-------------------------------------------------------*/

    key = vkey();
    switch (key)
    {

      /* 離開與重新 */
      case 'q':                 /* quit the game */
        vmsg("掰掰囉~~");
        return 0;

      case 'r':
        vmsg("重新開始!");
        goto game_start;                /* restart the game */


/* - 方 向 鍵 處 理 --------------------------------------------- */
      case KEY_UP:
        cur.y--;
        if (cur.y < 0)
          cur.y = 2;
        draw_cur(0);
        break;

      case KEY_DOWN:
        cur.y++;
        if (cur.y > 2)
          cur.y = 0;
        draw_cur(0);
        break;

      case KEY_LEFT:
        cur.x--;
        if (cur.x < 0)
          cur.x = 5;
        if (cur.x > 2)
          SIDE = SIDE_R;
        else
          SIDE = SIDE_L;
        draw_cur(0);
        break;

      case KEY_RIGHT:
        cur.x++;
        if (cur.x > 5)
          cur.x = 0;
        if (cur.x > 2)
          SIDE = SIDE_R;
        else
          SIDE = SIDE_L;
        draw_cur(0);
        break;
/* -------------------------------------------------------------- */


      /* 移動 */
      case ' ':
        move_block();
        break;
      
      /* 作弊的彩蛋 */
      case '\\':
        light = !light;		/* 開 / 關燈 */

        if (light)		/* 開燈 */
        {
	  move(b_lines - 5 , 10);
          outs("傳說中的解答(竊笑) --> ");
          move(b_lines - 7, 35);
          outs("+-----+");
          for (i = 0; i < 3; i++)
          {
            move(b_lines - (6-i), 35);
            outs("|");
            for (j = 0; j < 3; j++)
            {
              prints("%d|", cheat[i*3 + j]);
            }
          }
          move(b_lines - 3, 35);
          outs("+-----+");
        }
        else 			/* 關燈 */
        {
          move(b_lines - 7, 35);
          outs("        ");
          move(b_lines - 6, 10);
          outs("\n\n\n\n");
        } 

        break;

      /* 暫停 */
      case 'p':
        /* stop */
        t1 = time(0);
        vmsg("遊戲暫停  按任意鍵繼續...");
        /* go */
        t2 = time(0);
        init_time += (t2 - t1);
        t2 = t1 = init_time;    /* 恢復原狀 */
        break;

      case 'w':
        wow();			/* 告訴使用者解答 */
        char buff[5];
        vget(b_lines - 2, 0, "要繼續嗎?y/[N]", buff, 2, DOECHO);
 
        if (buff[0] == 'y' || buff[0] == 'Y')
          goto game_start;
        else 
          goto game_over;

        break;

      case 'h':
        t1 = time(0);		/* 要把時間暫停 */
        more("etc/game/tetravex.hlp", MFST_NULL);
        move(0, 0);
        clrtobot();
        /* 畫出框框 */
        move(4, 0);
        outs(TetraVex_BORDER);

        out_foot();           /* 腳腳 >////< */

        /* 時間繼續跑 */
        t2 = time(0);
        init_time += (t2 - t1);
        t2 = t1 = init_time;    /* 恢復原狀 */

        break;
    }

    draw_all();
    move(b_lines - 1, 0);
    x_cur();

    if (check_ans())
    {

      char buf[80];

      dif_t = time(0) - init_time;
      
      bonus = (int)(50  + 0.5 * (80 - (int)dif_t));
      if (bonus < 0)
        bonus = 0;
      sprintf(buf, " Wow! %s Win the game in %d secs!!"
                    , cuser.userid, dif_t);
      vmsg(buf);
      sprintf(buf, " Bonus =  %d ", bonus);
      vmsg(buf);
      addmoney(bonus);

      goto game_over;
    }
  }

  game_over:
  return 0;
}

#endif

