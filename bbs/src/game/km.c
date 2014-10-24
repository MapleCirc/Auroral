/*-------------------------------------------------------*/
/* km.c         ( NTHU CS MapleBBS Ver 3.10 )            */
/*-------------------------------------------------------*/
/* target : KongMing Chess routines			 */
/* create : 01/02/08					 */
/* update : 09/08/08					 */
/* author : einstein@bbs.tnfsh.tn.edu.tw		 */
/* recast : dust @tcirc.twbbs.org			 */
/*-------------------------------------------------------*/


/* �ѽL�ɦb etc/game/km �榡�p�U�G
 *
 * �Ĥ@����`�@���X�L�ѽL(�T���)�A�q�ĤT��}�l�h�O�@�L�@�L�����СC
 *
 * �q���r��(#)�}�l�N�N��@�ӴѽL�A�Ӧ�ѽs��(�T���)�M�ѽL�W(20�r���H��)�զ�
 *
 * TILE_NOUSE 0 ��ܤ��ಾ�ʪ���l
 * TILE_BLANK 1 ��ܪŮ�
 * TILE_CHESS 2 ��ܴѤl */


#include "bbs.h"


#ifdef HAVE_GAME

#define LOG_KM		/* �O�_���ѰO�����Ъ��\�� */
#define RETRACT_CHESS	/* �O�_���Ѯ��ѥ\�� */


#define KM_XPOS 9		/* "�ѽL"���_�l��m */
#define KM_YPOS 6
#define MAX_X 7			/* �@�w�n�O�_�� */
#define MAX_Y 7			/* �@�w�n�O�_�� */

#define TILE_NOUSE 0		/* ���ಾ�ʪ���l */
#define TILE_BLANK 1		/* �Ů� */
#define TILE_CHESS 2		/* �Ѥl */

#define cmove(x, y) move(KM_YPOS + y, KM_XPOS + x * 2 + 1)	/* ���ʴ�� */
#define dmove(x, y) move(KM_YPOS + y, KM_XPOS + x * 2)		/* ø�s�ɨϥ� */
#define valid_pos(x, y) (x>=0 && x<MAX_X && y>=0 && y<MAX_Y && board[x][y]!=TILE_NOUSE)
/* ²��ӻ��N�O��Ъ��i���ʽd��� */
#define check(fx, fy ,tx, ty) ((board[(fx+tx)/2][(fy+ty)/2] & TILE_CHESS) && ((abs(fx-tx) == 2 && fy == ty) || (fx == tx && abs(fy-ty) == 2)))
/* �T�{�i�H�q(fx, fy)����(tx, ty) */



static time_t startime;
static int status;	/* -1:���}�C��  -2:ø�s��ӵe��  -3:��l�ƹC��  0:����w�Ѥl  1:�w��w�Ѥl */
static int cx, cy;	/* Current Cursor */
static int num_table;	/* �ѽL�`�� */
static int step;	/* �ثe���B�� */

static int board[MAX_X][MAX_Y];		/* �ѽL���� */

#if (defined LOG_KM || defined RETRACT_CHESS)
static int route[MAX_X * MAX_Y][4];	/* �O�� (fx, fy) -> (tx, ty)�A���ѨB�Ƥ��|�W�L�ѽL�j�p */
#endif

static char *piece[4] = {"�@", "��", "��", "\033[1;33m��\033[m"};
static char title[21];	/* ���ЦW�� */



static void
out_info(opt)
  int opt;
{
  move(21, 0);
  clrtoeol();

  if(opt) return;

  prints("�ثe�O�� %d �B�A�z�ΤF %d ��C", step, time(0) - startime);
}


static int
read_board(stage)	/* Ū���C���ѽL */
  int stage;		/* -1:��l��  -2:����O���� */
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
	if(i < 1 || i > num_table)	/* ���b�d�򤺪��s���N���L */
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
show_board(stage)	/* ��ܥ��e�� */
  int stage;
{
  int i, j;

  clear();
  vs_bar("�թ���");

  move(3, 7);
  prints("No.%d \033[1m�u%s�v\033[m", stage, title);

  for(i=0;i<MAX_Y;i++)
  {
    move(KM_YPOS + i, KM_XPOS);
    for(j=0;j<MAX_X;j++)
    {
      outs(piece[board[j][i]]);
    }
  }

  move(4, 44);
  outs("��������  ��V��");

  move(6, 44);
  outs("[space]   ���/�Ͽ��");

  move(9, 44);
  outs("q         ���}");
  move(10, 44);
  outs("h         �C������");

  move(12, 44);
  outs("r         ����");
  move(13, 44);
  outs("y         ���s�}�l����");

  if(stage < num_table)
  {
    move(14, 44);
    outs("k         ���ܤU�@��");
  }

  move(16, 44);
  outs("��        �Ŧ�");
  move(17, 44);
  outs("��        �Ѥl");
  move(18, 44);
  outs("\033[1;33m��\033[m        �Q������Ѥl");

  if(step)
    out_info(0);
}


static int
live()		/* �ݬݬO�_�ٯ��~��C�� */
{
  static const int dir[4][2] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}
  };
  int i, j, k, nx, ny, nx2, ny2;

  for (i = 0; i < MAX_X; i++)
  {
    for (j = 0; j < MAX_Y; j++)
    {
      if(!(board[i][j] & TILE_CHESS))	/* (i, j)�B�S�Ѥl���ܴN�O�]�F�a... */
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
jump(fx, fy, tx, ty)	/* �q (fx, fy) ���� (tx, ty) */
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
retract()	/* ���ѥ\�� */
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
log_km(stage)		/* �O������ */
  int stage;		/* <0:��l�� */
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
    fprintf(fp, "���D: �թ����� #%03d%s �}�ѹL�{\n�ɶ�: %s\n\n", stage, title, Now());
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

    sprintf(buf, "�թ����� #%03d%s �}�ѹL�{", stage, title);
	  mail_self(fpath, cuser.userid, buf, MAIL_READ);
  }
}
#endif



int
main_km()
{
  char *init_menu[] = {"t���w�ѽL", "r�H���ѽL", "q���}", NULL};
  char *clear_menu_usual[] = {"s�O�s����", "n�~��U�@��", "r���s�D�Ԧ���", "q���}", NULL};
  char *clear_menu_last[] = {"s�O�s����", "r���s�D�Ԧ���", "q���}", NULL};
  char *over_menu[] = {"n���ܤU�@��", "r���s�D�Ԧ���", "q���}", NULL};
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

  switch(krans(b_lines, "tq", init_menu, "�� �թ��ѡG"))
  {
  case 't':
    sprintf(buf, "�п�J�ѽL�s��(1~%d) [1]�G", num_table);
    vget(b_lines, 0, buf, buf + 48, 4, DOECHO);
		  /*  ���·t�L�k XD */
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
    vmsg("�O����t�m���ѡA�Ь����ȳB�z (���M���@�w����)");
    return 0;
  case -1:
    vmsg("�ѽL�ɼƥؤ��X�A�Ь����ȳB�z (���M���@�w����)");
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
      /* �L�� (STAGE CLEAR) */
      if(count == 1 && (board[MAX_X / 2][MAX_Y / 2] & TILE_CHESS))
      {
	vmsg("���߱z���\\�F�I");
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


      /* ���� (GAME OVER) */
      if(!live())
      {
	vmsg("�V�|...�S�ѥi���F...");
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
      switch(vkey())	/* ����B�z */
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
	  }			/* ������a��i�H�� */
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
	if(status == 0 && step > 0)	/* �n�b����l�����A�U�~�஬�� */
	{
	  retract();
	  count++;
	  out_info(0);
        }
	break;
#endif

      case 'k':
	if(stage < num_table)	/* �Y�O�̫�@���N�S�a��i���F... */
	  stage++;

      case 'y':
	status = -3;
	break;
      }		/* End of KEY PROCESS */

      if(status < 0) break;

    }	/* �C���{�ǰj�� */

  }	/* �{���D�n�j�� */

  read_board(-2);
  return 0;
}

#endif

