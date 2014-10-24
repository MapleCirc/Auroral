/*-------------------------------------------------------*/
/* km.c         ( NTHU CS MapleBBS Ver 3.10 )            */
/*-------------------------------------------------------*/
/* target : KongMing Chess routines			 */
/* create : 01/02/08					 */
/* update : 09/08/08					 */
/* author : einstein@bbs.tnfsh.tn.edu.tw		 */
/* recast : dust @tcirc.twbbs.org			 */
/*-------------------------------------------------------*/


/* 棋盤檔在 etc/game/km 格式如下：
 *
 * 第一行放總共有幾盤棋盤(三位數)，從第三行開始則是一盤一盤的棋譜。
 *
 * 從井字號(#)開始將代表一個棋盤，該行由編號(三位數)和棋盤名(20字元以內)組成
 *
 * TILE_NOUSE 0 表示不能移動的格子
 * TILE_BLANK 1 表示空格
 * TILE_CHESS 2 表示棋子 */


#include "bbs.h"


#ifdef HAVE_GAME

#define LOG_KM		/* 是否提供記錄棋譜的功能 */
#define RETRACT_CHESS	/* 是否提供悔棋功能 */


#define KM_XPOS 9		/* "棋盤"的起始位置 */
#define KM_YPOS 6
#define MAX_X 7			/* 一定要是奇數 */
#define MAX_Y 7			/* 一定要是奇數 */

#define TILE_NOUSE 0		/* 不能移動的格子 */
#define TILE_BLANK 1		/* 空格 */
#define TILE_CHESS 2		/* 棋子 */

#define cmove(x, y) move(KM_YPOS + y, KM_XPOS + x * 2 + 1)	/* 移動游標 */
#define dmove(x, y) move(KM_YPOS + y, KM_XPOS + x * 2)		/* 繪製時使用 */
#define valid_pos(x, y) (x>=0 && x<MAX_X && y>=0 && y<MAX_Y && board[x][y]!=TILE_NOUSE)
/* 簡單來說就是游標的可移動範圍解 */
#define check(fx, fy ,tx, ty) ((board[(fx+tx)/2][(fy+ty)/2] & TILE_CHESS) && ((abs(fx-tx) == 2 && fy == ty) || (fx == tx && abs(fy-ty) == 2)))
/* 確認可以從(fx, fy)跳到(tx, ty) */



static time_t startime;
static int status;	/* -1:離開遊戲  -2:繪製整個畫面  -3:初始化遊戲  0:未選定棋子  1:已選定棋子 */
static int cx, cy;	/* Current Cursor */
static int num_table;	/* 棋盤總數 */
static int step;	/* 目前的步數 */

static int board[MAX_X][MAX_Y];		/* 棋盤本身 */

#if (defined LOG_KM || defined RETRACT_CHESS)
static int route[MAX_X * MAX_Y][4];	/* 記錄 (fx, fy) -> (tx, ty)，悔棋步數不會超過棋盤大小 */
#endif

static char *piece[4] = {"　", "○", "●", "\033[1;33m●\033[m"};
static char title[21];	/* 棋譜名稱 */



static void
out_info(opt)
  int opt;
{
  move(21, 0);
  clrtoeol();

  if(opt) return;

  prints("目前是第 %d 步，您用了 %d 秒。", step, time(0) - startime);
}


static int
read_board(stage)	/* 讀取遊戲棋盤 */
  int stage;		/* -1:初始化  -2:釋放記憶體 */
{
  char buf[128];
  static long *bpos;
  long offset;
  FILE *fp;
  int i, j, count;

  if(stage < 0)
  {
    if(stage == -2)
    {
      free(bpos);
      return 1;
    }

    bpos = (long*) malloc(num_table * sizeof(long));
    if(!bpos)
      return 0;

    fp = fopen("etc/game/km", "r");

    count = 0;
    while((j = fgetc(fp)) != EOF)
    {
      if(j == '#')
      {
	offset = ftell(fp) - 1;
	fgets(buf, sizeof(buf), fp);
	sscanf(buf, "%d", &i);
	if(i < 1 || i > num_table)	/* 不在範圍內的編號就跳過 */
	  continue;
	bpos[i - 1] = offset;
	count++;
      }
    }
    fclose(fp);

    if(count != num_table)
    {
      free(bpos);
      return -1;
    }
    else
      return 1;
  }
  else
  {
    fp = fopen("etc/game/km", "r");
    fseek(fp, bpos[stage - 1], SEEK_SET);
    fgets(buf, sizeof(buf), fp);
    sscanf(buf+4, "%20s", title);
    memset(board, 0, sizeof(board));

    count = 0;
    for(j=0;j<MAX_Y;j++)
    {
      for(i=0;i<MAX_X;i++)
      {
	fscanf(fp, "%d", &board[i][j]);
	if(board[i][j] & TILE_CHESS)
	  count++;
      }
    }

    fclose(fp);
    return count;
  }
}


static void
show_board(stage)	/* 顯示全畫面 */
  int stage;
{
  int i, j;

  clear();
  vs_bar("孔明棋");

  move(3, 7);
  prints("No.%d \033[1m「%s」\033[m", stage, title);

  for(i=0;i<MAX_Y;i++)
  {
    move(KM_YPOS + i, KM_XPOS);
    for(j=0;j<MAX_X;j++)
    {
      outs(piece[board[j][i]]);
    }
  }

  move(4, 44);
  outs("↑↓←→  方向鍵");

  move(6, 44);
  outs("[space]   選取/反選取");

  move(9, 44);
  outs("q         離開");
  move(10, 44);
  outs("h         遊戲說明");

  move(12, 44);
  outs("r         悔棋");
  move(13, 44);
  outs("y         重新開始此局");

  if(stage < num_table)
  {
    move(14, 44);
    outs("k         跳至下一關");
  }

  move(16, 44);
  outs("○        空位");
  move(17, 44);
  outs("●        棋子");
  move(18, 44);
  outs("\033[1;33m●\033[m        被選取的棋子");

  if(step)
    out_info(0);
}


static int
live()		/* 看看是否還能繼續遊戲 */
{
  static const int dir[4][2] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}
  };
  int i, j, k, nx, ny, nx2, ny2;

  for (i = 0; i < MAX_X; i++)
  {
    for (j = 0; j < MAX_Y; j++)
    {
      if(!(board[i][j] & TILE_CHESS))	/* (i, j)處沒棋子的話就別跑了吧... */
	continue;

      for (k = 0; k < 4; k++)
      {
	nx = i + dir[k][0];
	ny = j + dir[k][1];
	nx2 = nx + dir[k][0];
	ny2 = ny + dir[k][1];
	if (valid_pos(nx2, ny2) && (board[nx][ny] & TILE_CHESS) && (board[nx2][ny2] & TILE_BLANK))
	  return 1;
      }
    }
  }

  return 0;
}


static inline void
jump(fx, fy, tx, ty)	/* 從 (fx, fy) 跳到 (tx, ty) */
  int fx, fy, tx, ty;
{
  board[fx][fy] = TILE_BLANK;
  dmove(fx, fy);
  outs(piece[1]);

  board[(fx + tx) / 2][(fy + ty) / 2] = TILE_BLANK;
  dmove((fx + tx) / 2, (fy + ty) / 2);
  outs(piece[1]);

  board[tx][ty] = TILE_CHESS;
  dmove(tx, ty);
  outs(piece[2]);

#if (defined LOG_KM || defined RETRACT_CHESS)
  route[step][0] = fx;
  route[step][1] = fy;
  route[step][2] = tx;
  route[step][3] = ty;
#endif
  step++;
}


#if (defined LOG_KM || defined RETRACT_CHESS)
static void
retract()	/* 悔棋功能 */
{
  int fx, fy, tx, ty;

  step--; 
  ty = route[step][3];
  tx = route[step][2];
  fy = route[step][1];
  fx = route[step][0];

  board[tx][ty] = TILE_BLANK;
  dmove(tx, ty);
  outs(piece[1]);

  board[(fx + tx) / 2][(fy + ty) / 2] = TILE_CHESS;
  dmove((fx + tx) / 2, (fy + ty) / 2);
  outs(piece[2]);

  board[fx][fy] = TILE_CHESS;
  dmove(fx, fy);
  outs(piece[2]);

  cx = fx;
  cy = fy;
}
#endif


#ifdef LOG_KM
static void
log_km(stage)		/* 記錄棋譜 */
  int stage;		/* <0:初始化 */
{
  static int copy_board[MAX_X][MAX_Y];
  int i, j, k, fx, fy, tx, ty;
  FILE *fp;
  char fpath[64], buf[64];

  if(stage < 0)
  {
    memcpy(copy_board, board, sizeof(copy_board));
  }
  else
  {
    usr_fpath(fpath, cuser.userid, "km.log");

    if(!(fp = fopen(fpath, "w")))
      return;

    fprintf(fp, "%s %s (%s)\n", str_author1, cuser.userid, cuser.username);
    fprintf(fp, "標題: 孔明棋譜 #%03d%s 破解過程\n時間: %s\n\n", stage, title, Now());
    fprintf(fp, "#%03d%s\n\n", stage, title);

    for(k = 0; k <= step; k++)
    {
      fprintf(fp, "\n");
      for(j = 0; j < MAX_Y; j++)
      {
	for(i = 0; i < MAX_X; i++)
	{
	  fprintf(fp, "%s", piece[copy_board[i][j]]);
	}
	fprintf(fp, "\n");
      }
      fprintf(fp, "\n\n");

      fx = route[k][0];
      fy = route[k][1];
      tx = route[k][2];
      ty = route[k][3];
      copy_board[fx][fy] = TILE_BLANK;
      copy_board[(fx + tx) / 2][(fy + ty) / 2] = TILE_BLANK;
      copy_board[tx][ty] = TILE_CHESS;
    }

    ve_banner(fp, 0);
    fclose(fp);

    sprintf(buf, "孔明棋譜 #%03d%s 破解過程", stage, title);
	  mail_self(fpath, cuser.userid, buf, MAIL_READ);
  }
}
#endif



int
main_km()
{
  char *init_menu[] = {"t指定棋盤", "r隨機棋盤", "q離開", NULL};
  char *clear_menu_usual[] = {"s保存棋譜", "n繼續下一關", "r重新挑戰此關", "q離開", NULL};
  char *clear_menu_last[] = {"s保存棋譜", "r重新挑戰此關", "q離開", NULL};
  char *over_menu[] = {"n跳至下一關", "r重新挑戰此關", "q離開", NULL};
  char buf[64], **ClearMenu, *Default;
  int stage, count, ans, fx, fy;
#ifdef LOG_KM
  int saved;
#endif
  FILE *fp;


  if (!(fp = fopen("etc/game/km", "r")))
    return 0;

  fgets(buf, sizeof(buf), fp);
  fclose(fp);
  num_table = atoi(buf);
  if(num_table < 1)
    return 0;

  switch(krans(b_lines, "tq", init_menu, "◇ 孔明棋："))
  {
  case 't':
    sprintf(buf, "請輸入棋盤編號(1~%d) [1]：", num_table);
    vget(b_lines, 0, buf, buf + 48, 4, DOECHO);
		  /*  ↑黑暗兵法 XD */
    stage = atoi(buf + 48);
    if (stage < 1 || stage > num_table)
      stage = 1;

    break;

  case 'r':
    stage = rand() % num_table;
    break;

  case 'q':
    return 0;
  }

  switch(read_board(-1))
  {
  case 0:
    vmsg("記憶體配置失敗，請洽站務處理 (雖然不一定有用)");
    return 0;
  case -1:
    vmsg("棋盤檔數目不合，請洽站務處理 (雖然不一定有用)");
    return 0;
  }


  status = -3;
  while(1)
  {
    if(status == -1)
      break;
    else if(status == -3)
    {
      count = read_board(stage);
      step = 0;
      startime = time(0);
      cx = MAX_X/2;
      cy = MAX_Y/2;
#ifdef LOG_KM
      log_km(-1);
#endif
      status = -2;
    }

    if(status == -2)
      show_board(stage);

    status = 0;
    while(1)
    {
      /* 過關 (STAGE CLEAR) */
      if(count == 1 && (board[MAX_X / 2][MAX_Y / 2] & TILE_CHESS))
      {
	vmsg("恭喜您成功\了！");
	out_info(1);
#ifdef LOG_KM
	saved = 0;
#endif
	if(stage < num_table)
	{
	  ClearMenu = clear_menu_usual;
	  Default = "nn";
	}
	else
	{
	  ClearMenu = clear_menu_last;
	  Default = "qq";
	}

	while(1)
	{
#ifdef LOG_KM
	  if(!saved)
	    ans = krans(b_lines, "ss", ClearMenu, "Stage Clear. ");
	  else
#endif
	    ans = krans(b_lines, Default, ClearMenu + 1, "Stage Clear. ");

	  switch(ans)
	  {
#ifdef LOG_KM
	  case 's':
	    log_km(stage);
	    saved = 1;
	    continue;
#endif
	  case 'n':
	    stage++;
	  case 'r':
	    status = -3;
	    break;

	  case 'q':
	    status = -1;
	    break;
	  }

	  break;
	}

	break;
      }		/* End of STAGE CLEAR */


      /* 死局 (GAME OVER) */
      if(!live())
      {
	vmsg("糟糕...沒棋可走了...");
	out_info(1);
	if(stage < num_table)
	  ans = krans(b_lines, "rr", over_menu, "Game Over. ");
	else
	  ans = krans(b_lines, "rr", over_menu + 1, "Game Over. ");;

	  switch(ans)
	  {
	  case 'n':
	    stage++;
	  case 'r':
	    status = -3;
	    break;

	  case 'q':
	    status = -1;
	    break;
	  }

	break;
      }		/* End of GAME OVER */


      cmove(cx, cy);
      switch(vkey())	/* 按鍵處理 */
      {
      case KEY_UP:
	if(valid_pos(cx, cy - 1))
	  cy--;
	break;

      case KEY_DOWN:
	if(valid_pos(cx, cy + 1))
	  cy++;
	break;

      case KEY_LEFT:
	if(valid_pos(cx - 1, cy))
	  cx--;
	break;

      case KEY_RIGHT:
	if(valid_pos(cx + 1, cy))
	  cx++;
	break;

      case ' ':
	if(status)
	{
	  if(cx == fx && cy == fy)
	  {
	    dmove(fx, fy);
	    outs(piece[2]);
	    status = 0;
	  }			/* 選跳的地方可以跳 */
	  else if((board[cx][cy] & TILE_BLANK) && check(fx, fy, cx, cy))
	  {
	    jump(fx, fy, cx, cy);
	    count--;
	    status = 0;
	    out_info(0);
	  }
	}
	else if(board[cx][cy] & TILE_CHESS)
	{
	  fx = cx;
	  fy = cy;
	  dmove(fx, fy);
	  outs(piece[3]);
	  status = 1;
	}
	break;

      case 'q':
	status = -1;
	break;

      case 'h':
	more("etc/game/km.hlp", MFST_NORMAL);
	status = -2;
	break;

#ifdef RETRACT_CHESS
      case 'r':
	if(status == 0 && step > 0)	/* 要在未選子的狀態下才能悔棋 */
	{
	  retract();
	  count++;
	  out_info(0);
        }
	break;
#endif

      case 'k':
	if(stage < num_table)	/* 若是最後一關就沒地方可跳了... */
	  stage++;

      case 'y':
	status = -3;
	break;
      }		/* End of KEY PROCESS */

      if(status < 0) break;

    }	/* 遊戲程序迴圈 */

  }	/* 程式主要迴圈 */

  read_board(-2);
  return 0;
}

#endif

