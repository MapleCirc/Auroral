/*-------------------------------------------------------*/
/* gray2.c        ( Tcfsh CIRC 27th )                    */
/*-------------------------------------------------------*/
/* target : 黑白棋遊戲 改良                              */
/* create : 10/12/11                  學測倒數 46 天     */
/* update :   /  /                                       */
/* author : xatier @tcirc.org                            */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME


#define B 4
#define W 2
#define N 0

#define GRAY_RANK_PATH    "run/grayrank"

/* 黑子=4  白子=2  空格=0 */

/* - structure ------------------------------------------ */

typedef struct {
  int x;
  int y;
  int state;
} chess;                /* 棋子 */

typedef struct {
  chess b[9][9];        /* 8x8 棋盤 , 下標 0 不用 */
} chessboard;           /* 棋盤 */

typedef struct {
  int x;
  int y;
} CUR;                  /* 游標 */

/* - global vars----------------------------------------- */

chessboard cur_board;   /* 現在的棋盤       */
int b_occupation;       /* 黑子佔有棋子數   */
int w_occupation;       /* 白子佔有棋子數   */
CUR cur;                /* 游標             */
int black_turn;         /* 輪到黑子(玩家)下 */
int comp_pass;          /* 電腦 Pass        */


                        /* 棋盤位置值 (人工智慧用) */
double map_val [9][9] = 
{   /* 其值為該點與 棋盤中心 之距離開平方根 ; 下標零不用*/
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 2.2248, 2.0739, 1.9514, 1.8803, 1.8803, 1.9514, 2.0739, 2.2248,
    0.0000, 2.0739, 1.8803, 1.7075, 1.5967, 1.5967, 1.7075, 1.8803, 2.0739,
    0.0000, 1.9514, 1.7075, 1.4565, 1.2574, 1.2574, 1.4565, 1.7075, 1.9514,
    0.0000, 1.8803, 1.5967, 1.2574, 0.8409, 0.8409, 1.2574, 1.5967, 1.8803,
    0.0000, 1.8803, 1.5967, 1.2574, 0.8409, 0.8409, 1.2574, 1.5967, 1.8803,
    0.0000, 1.9514, 1.7075, 1.4565, 1.2574, 1.2574, 1.4565, 1.7075, 1.9514,
    0.0000, 2.0739, 1.8803, 1.7075, 1.5967, 1.5967, 1.7075, 1.8803, 2.0739,
    0.0000, 2.2248, 2.0739, 1.9514, 1.8803, 1.8803, 1.9514, 2.0739, 2.2248

};

/* - function declaration ------------------------------- */

void init (void);                         /* 初始化       */
void draw_chess (chess *c);               /* 畫棋子       */
void draw_cur_board (void);               /* 重繪棋盤     */
int  search (chess *c, int dx, int dy);   /* 向外搜索     */
int  do_play (void);                      /* 可以下這步   */
void do_eat (chess *c, int dx, int dy);   /* 吃掉棋子     */
int  check (void);                        /* 檢查遊戲結束 */
void computer_play(void);                 /* 人工智慧     */

/* - function definition -------------------------------- */


void
init (void)
{

  int i, j;
  
  move(0, 0);                             /* 清螢幕       */
  clrtobot();

  move(0, 0);

  outs(Gray_INIT);                        /* 遊戲畫面繪製 */

  for (i = 1; i <= 8; i++)                /* 棋盤清空     */
    for (j = 1; j <= 8; j++)
    {
      cur_board.b[i][j].x = i;
      cur_board.b[i][j].y = j;
      cur_board.b[i][j].state = N;
    }

  /* 一人兩子 */
  cur_board.b[4][4].state = cur_board.b[5][5].state = B;
  cur_board.b[4][5].state = cur_board.b[5][4].state = W;

  cur.x = cur.y = 4;                      /* 游標放擺間   */

  black_turn = 1;                         /* 黑子先下     */

  check();                                /* 更新雙方佔領 */

  draw_cur_board();                       /* 畫出棋盤     */
  move(11 + (cur.y-1), 32 + (cur.x-1) * 2);

}


void 
draw_chess (chess *c)
{
  move(11 + (c->y-1), 31 + (c->x-1) * 2);
  switch (c->state) {
    case N:
      outs("\033[33m■\033[m");
      break;
    case W:
      outs("●");
      break;
    case B:
      outs("○");
    break;
  }
}


void
draw_cur_board (void)
{
  int i, j;

  for (i = 1; i <= 8; i++)                /* 逐一印出棋子 */
    for (j = 1; j <= 8; j++)
      draw_chess(&cur_board.b[i][j]);
  
  move(12, 62);                           /* 輸出雙方佔領 */
  prints("%2d", b_occupation);
  move(13, 62);
  prints("%2d", w_occupation);


}


int
search (chess *c, int dx, int dy)
{
  int x = c->x, y = c->y, count = 0;
  int target = (black_turn ? B : W), another = (black_turn ? W : B);
                                          /* 呼 呼!       */
  x += dx;                                /* 移動         */
  y += dy;
                                          /* 棋盤範圍內部 */
  while ((1 <= x && x <= 8) && (1 <= y && y <= 8))
  {
    if (cur_board.b[x][y].state == another)
    {
      count++;                            /* 我可以吃這顆 */
      x += dx;                            /* 繼續往前走   */
      y += dy;
    }
    else if (cur_board.b[x][y].state == target)
        return count;                     /* 搜尋終了     */
    else 
      break;
  }
  return 0;
}


int
do_play (void)
{
  int max = 0, d[8], i;

  if (cur_board.b[cur.x][cur.y].state != N)
    return 0;                             /* 不能下這裡   */

                                          /* 向外搜尋     */
  d[0] = search(&cur_board.b[cur.x][cur.y], -1, -1) ;
  d[1] = search(&cur_board.b[cur.x][cur.y],  0, -1) ;
  d[2] = search(&cur_board.b[cur.x][cur.y],  1, -1) ;

  d[3] = search(&cur_board.b[cur.x][cur.y], -1,  0) ;
  d[4] = search(&cur_board.b[cur.x][cur.y],  1,  0) ;

  d[5] = search(&cur_board.b[cur.x][cur.y], -1,  1) ;
  d[6] = search(&cur_board.b[cur.x][cur.y],  0,  1) ;
  d[7] = search(&cur_board.b[cur.x][cur.y],  1,  1) ;

  for (i = 0; i < 8; i++)                 /* 總吃棋數     */
    max += d[i];

  return max;
}


void
do_eat (chess *c, int dx, int dy)
{
  int x = c->x, y = c->y, count = search(c, dx, dy);
  int target = (black_turn ? B : W), another = (black_turn ? W : B);

  if ( count == 0 )                       /* 這格不能吃啦 */
    return;

  while (count--)                         /* 一顆一顆吃掉 */
  {
    x += dx;
    y += dy;

    if (cur_board.b[x][y].state == another)
      cur_board.b[x][y].state = target;
    else
      break;
    
  }
}

void
computer_play (void)
{
  int i, j, k, d[8], sum;
  double val, max;
  CUR best;

  best.x = 1;
  best.y = 1;
  max = val = 0.0;
  comp_pass = 0;

  for (i = 1; i <= 8; i++)                /* 逐一遍歷棋盤 */
  {
    for (j = 1; j <= 8; j++)
    {

      cur.x = i;                          /* 移動游標     */
      cur.y = j;
      sum = 0;

      if (cur_board.b[cur.x][cur.y].state != N)
        continue;

                                          /* 算出能吃幾顆 */
      d[0] = search(&cur_board.b[cur.x][cur.y], -1, -1);
      d[1] = search(&cur_board.b[cur.x][cur.y],  0, -1);
      d[2] = search(&cur_board.b[cur.x][cur.y],  1, -1);

      d[3] = search(&cur_board.b[cur.x][cur.y], -1,  0);
      d[4] = search(&cur_board.b[cur.x][cur.y],  1,  0);

      d[5] = search(&cur_board.b[cur.x][cur.y], -1,  1);
      d[6] = search(&cur_board.b[cur.x][cur.y],  0,  1);
      d[7] = search(&cur_board.b[cur.x][cur.y],  1,  1);


      for (k = 0; k < 8; k++)             /* 總吃棋數     */
        sum += d[k];

      if (sum == 0)                       /* 沒吃到半顆   */
        continue;

      val = sum * map_val[i][j];          /* 乘上位置權重 */

      if (val > max)                      /* 找最大的     */
      {
        max = val;
        best = cur;
      }
    }
  }

  cur = best;                             

  if (do_play())                          /* 電腦可以下 */
  {

    /* 下這步 */
    cur_board.b[cur.x][cur.y].state = black_turn ? B : W;


    /* 四面八方去吃 */
    do_eat(&cur_board.b[cur.x][cur.y], -1, -1);
    do_eat(&cur_board.b[cur.x][cur.y],  0, -1);
    do_eat(&cur_board.b[cur.x][cur.y],  1, -1);

    do_eat(&cur_board.b[cur.x][cur.y], -1,  0);
    do_eat(&cur_board.b[cur.x][cur.y],  1,  0);

    do_eat(&cur_board.b[cur.x][cur.y], -1,  1);
    do_eat(&cur_board.b[cur.x][cur.y],  0,  1);
    do_eat(&cur_board.b[cur.x][cur.y],  1,  1);

  }
  else
  {
    vmsg(" 電腦無子可下 ");
    comp_pass = 1;
  }

  black_turn = !black_turn;                         /* 玩家換你囉   */

}


int                            /* 0: 還未分勝負  1: 玩家勝   2:電腦勝 */
check (void)
{
  int i, j;
  int b = 0, w = 0;

  for (i = 1; i <= 8; i++)                /* 計算佔領棋數 */
    for (j = 1; j <= 8; j++)
    {
      switch (cur_board.b[i][j].state)
      {
        case B:
          b++;
          break;
        case W:
          w++;
          break;
      }
    }

  b_occupation = b;                       /* 用這裡來更新 */
  w_occupation = w;

  draw_cur_board ();


  if (w == 0 && b > 0)                    /* 輸贏判定     */
    return 1;
  else if (b == 0 && w > 0)
    return 2;
  else if ((b+w) == 64)
    return (b > w ? 1 : 2);
  else
    return 0;
}


int 
main_gray2 (void)
{
  
  char key, msg[20];
  int pass, done, i, j;
  CUR cur_temp;


game_start:  
  init();

  FILE *fp = fopen(GRAY_RANK_PATH, "wb");

  fprintf(fp, "test %s", cuser.userid);


  for (;;)
  {
    pass = done = 0;


                                          /* 檢查玩家可否下棋 */
    cur_temp = cur;
    for (i = 1; i <= 8; i++)
    {
      for (j = 1; j <= 8; j++)
      {
        cur.x = i;
        cur.y = j;
        if (!do_play())
          pass++;
      }
    }
    cur = cur_temp;


    if (pass == 64)
    {
      if (comp_pass)
      {
        vmsg("雙方無子可下 和局!");
        goto game_over;
      }
      else
      {
        vmsg(" 囧... 無子可下 ");
        black_turn = 0;
        done = 1;
      }

      goto comp;                          /* 輪到電腦下棋   */
    }




    switch (key = vkey())
    {

      /* - 離開與重新 --------------------------------------*/
      case 'q':                           /* 離開遊戲       */
        vmsg("掰掰囉~~");
        goto game_over;

      case 'r':
        vmsg("重新開始!");
        goto game_start;                  /* 重完           */



      /* - 方向鍵處理 ------------------------------------- */
      case KEY_UP:
        cur.y--;
        if (cur.y < 1)
          cur.y = 8;
        break;

      case KEY_DOWN:
        cur.y++;
        if (cur.y > 8)
          cur.y = 1;
        break;

      case KEY_LEFT:
        cur.x--;
        if (cur.x < 1)
          cur.x = 8;
        break;

      case KEY_RIGHT:
        cur.x++;
        if (cur.x > 8)
          cur.x = 1;
        break;


      /* - 下這裡 ----------------------------------------- */
      case ' ': case '\n':
        if (do_play())                    /* 可以下這個位置 */
        {

          /* 下這步 */
          cur_board.b[cur.x][cur.y].state = black_turn ? B : W;

          /* 四面八方去吃 */
          do_eat(&cur_board.b[cur.x][cur.y], -1, -1);
          do_eat(&cur_board.b[cur.x][cur.y],  0, -1);
          do_eat(&cur_board.b[cur.x][cur.y],  1, -1);
          do_eat(&cur_board.b[cur.x][cur.y], -1,  0);
          do_eat(&cur_board.b[cur.x][cur.y],  1,  0);
          do_eat(&cur_board.b[cur.x][cur.y], -1,  1);
          do_eat(&cur_board.b[cur.x][cur.y],  0,  1);
          do_eat(&cur_board.b[cur.x][cur.y],  1,  1);

          black_turn = 0;
          done = 1;

        }
        break;

    }                                     /* end of switch  */


    draw_cur_board();

    switch (check())
    {
      case 1:
        sprintf(msg, "玩家贏棋[%d : %d]", b_occupation, w_occupation);
        vmsg(msg);
        goto game_over;
        break;
      case 2:
        sprintf(msg, "玩家輸棋  [%d : %d]", b_occupation, w_occupation);
        vmsg(msg);
        goto game_over;
        break;
    }

    draw_cur_board();

comp:

    if (done)
      computer_play();


    switch (check())
    {
      case 1:
        sprintf(msg, "玩家贏旗  [%d : %d]", b_occupation, w_occupation);
        vmsg(msg);
        goto game_over;
        break;
      case 2:
        sprintf(msg, "玩家輸棋  [%d : %d]", b_occupation, w_occupation);
        vmsg(msg);
        goto game_over;
        break;
    }

    draw_cur_board();

    move(11 + (cur.y-1), 32 + (cur.x-1) * 2);

  } 


game_over:
  move(b_lines, 0);
  outs("按任意鍵離開...");
  vkey();
  return 0;
}

#endif
