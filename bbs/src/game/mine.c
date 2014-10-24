/*-------------------------------------------------------*/
/* mine.c ��a�p�C��					 */
/*-------------------------------------------------------*/
/* target : MineSweeper					 */
/* create : 08/06/04					 */
/* update : 10/09/25					 */
/* author : dust.bbs@tcirc.twbbs.org			 */
/*-------------------------------------------------------*/
/* 1.1�H�W��ø�Ͼ���O�����إߦb piaip's flat terminal system �W
  �w�˥��C���e�Ъ`�N���x�O�_�w�g�w�� pfterm */



#include "bbs.h"


#ifdef HAVE_GAME

#define GameVer "Ver1.1"

#define FN_MINE_HELP    "etc/game/mine.hlp"
#define FN_MINE_RANK    "run/minerank"
#define FN_MINE_PREC    "mine.rec"

#define MINE 1			/* �Ӯ榳�a�p */
#define TAGGED 0x100		/* �Ӯ�Q�аO */
#define EXPAND 0x200		/* �Ӯ�w½�} */
#define MVISIT 0x400		/* �Ӯ�w�Q���X�L(shooting mode) */
#define LDIRTY 0x800		/* �Ӯ�ݭn��ø(shooting mode) */
#define TX_MIN 19		/* TX���̤p�� */
#define TY_MIN 2		/* TY���̤p�� */
#define TCENTER_X 35		/* �C���L��m���ѷӤ����IX */
#define TCENTER_Y 11		/* �C���L��m���ѷӤ����IY */
#define RV_Time 2500		/* ���j�z����ܮɶ�(ms) */
#define TEPA_EffectTime 3700	/* �믫�P���@�ήɶ�(ms) */
#define STAND_MDELAY 800	/* �����H���T������ɶ�(ms) */
#define MSG_DUETIME 5		/* �T����ܮɶ�(��) */
#define REDUCE_RATE (rand()%5 < 2)	/* �a����t�a�p�X�{��C�v:40% */
#define DESC_LINELEN 30		/* ���~�y�z������ */
#define symMine "��"		/* �a�p */
#define symFlag "��"		/* �X�m */
#define symWrongflag "��"	/* �Хܿ��~���X�m */
#define symLand "��"		/* ��½�}���a�� */
#define symCursor "��"		/* �۾�(for shooting mode) */
#define TELEPATHY_KEY 'v'	/* �믫�P���ҨϥΪ����� */

#define MINE_NITEM 6		/* �D������ƶq */
#define INO_EXPL 0		/* �U��item no */
#define INO_MISL 1
#define INO_TEPA 2
#define INO_STAN 3
#define INO_REVI 4
#define INO_GLDF 5

static const char * const symNum[9] = {		/* ��ܥX�Ӫ��Ʀr */
  "�@", "��", "��", "��", "��", "��", "��", "��", "��"
};
static const char * const symNumZero[10] = {	/* ��ܥX�Ӫ��Ʀr(0~9) */
  "��", "��", "��", "��", "��", "��", "��", "��", "��", "��"
};

static int vectors[9][2] = {
  {0,1},{0,-1},{1,0},{-1,0},		/* �¾��v�޳N(?) */
  {1,1},{-1,-1},{1,-1},{-1,1},
  {0,0}
};
static const int (* const dir)[2] = (const int const (*)[2])vectors;

static const int item_nlim[MINE_NITEM] = {	/* ���W���D��ƶq�W�� */
  11, 5, 10, 8, 2, 9
};

static const int item_slim[MINE_NITEM] = {	/* �ܮw�e�q�W�� */
  99, 50, 99, 99, 50, 99
};

static const int itemcost[MINE_NITEM] = {	/* ���~���� */
  250, 748, 277, 371, 850, -1	//���ƥN��H�ȹ��p�� �t�ƥN��H�����p��
};

static char * const item_name[MINE_NITEM] = {
  "������", "���u", "�믫�P��", "�����H��", "���j�z��", "�����"
};

static char * const desc[MINE_NITEM] = {	/* ���~���y�z */
  "������ФU������O�_���a�p�C���a�p���ܷ|�۰ʼаO�_�ӡA�S�����ܫh����]�����C�ϥΫ᩵��0.3��C",
  "�F��3x3�d�򪺤���A�M�������d�򤺩Ҧ����F��C�]�������d�򤣯�[�\\�ۨ��C�ϥΫ᩵��1��C",
  "��ܥثe�Ҧb������P�򦳦h�����a�p�A�ĪG����3.7��C",
  "�L�k�D�ʨϥΡA�Ȧb���a�p�ɤH���|�N���ۤv�Q�a�p�����A²��ӻ��N�O�׽b�P�C�b�C�����i��a���ΩαҥΤH�����۰ʵo�ʡC",
  "���5x5�d�򪺤���U���L�a�p�A����2.5��C�ϥδ����L�k�i�����ʧ@�C",
  "�ǻ������~���A�ϥΫ�|�����L���ño��L�����y�A������J�ӤH�̨ά����P�Ʀ�]�C"
};

/* �a�ϸ�T:[����][xsize, ysize, �a�p�ƶq] */
static const int map_data[6][3] = {
  {10, 10, 10}, {15, 15, 30}, {20, 20, 60}, {25, 20, 90}, {30, 20, 120}, {30, 20, 150}
};



#define MSG_InRank "\033[1;36m(���߶i�]�I)\033[m"
#define MSG_usedGF "\033[34m(�ѩ�ϥΪ�����G���p�W��)\033[m"
#define MSG_usedGF_level0 "\033[1;30m(�ϥΤF�����)\033[m"
#define MCHLV_FEETER "\033[m\033[1;36m ���׿�� \033[37;44m    ����:��������      z:���      x:�^��D���                      \033[m"
#define MSHOP_FEETER "\033[m\033[1;36m �a�p�ө� \033[37;44m  ����:���ʴ��    z:���    x:�^�D���        %10d �ȹ�       \033[m"
#define MSTOR_FEETER "\033[m\033[1;36m �D��ܮw \033[37;44m  ������ ��:���ʴ��    z:���X/��J�D��    x:�^��D���              \033[m"
#define MRANK_FEETER "\033[m\033[1;36m  �Ʀ�]  \033[37;44m    ����:�˵���L����         x:�^��D���                           \033[m"
#define MDATA_FEETER "\033[m\033[1;36m �ӤH��� \033[37;44m  z:�i�J�ө�    x:�^��D���    d:�R�����     %10d �ȹ�       \033[m"


#define cmove(x, y) move(TY+(y), TX+(x)*2+1)
#define dmove(x, y) move(TY+(y), TX+(x)*2)
#define inrange(x, y) (x>=0 && x<xsize && y>=0 && y<ysize)
#define SendMsg(msg) globalmsg = msg
#define in_explosion(x, y) (sx-2<x && x<sx+2 && sy-2<y && y<sy+2)
#define on_edge(x, y) (x==0||y==0||x==xsize-1||y==ysize-1)
#define TV_ISGREATER(tv1, tv2) ((tv1.sec-tv2.sec)*100 + (tv1.csec-tv2.csec) > 0)
#define PRINT_INSTRG(i, front, back) prints(front"  %-14s %2d/%2d  "back, item_name[i], profile.storage[i], item_slim[i])
#define PRINT_INHAND(i, front, back) prints(front"  %-14s %2d/%2d  "back, item_name[i], profile.item[i], item_nlim[i])
#define T_TO_F(t) (t.sec + t.csec/100.0)
#define TELEPATHY_RECOVER() \
  if(modes == TELEPATHY){ \
    dmove(cx, cy); \
    outpiece(cx++, cy); \
    cx--; \
  }
#define TV_SETZERO(tv) { \
  tv.sec = 0; \
  tv.csec = 0; \
}
#define BACK_FROM_TELEPATHY_MODE() { \
  modes = NORMAL; \
  TV_SETZERO(TelepathyEndT); \
  dmove(cx, cy); \
  outpiece(cx, cy); \
  move(b_lines, 0); \
  clrtoeol(); \
}
#define GAME_CORRUPT() { \
  getcostime(&difft); \
  add_playtime(&difft); \
  if(lv==0) recover_item(0); \
  SaveProfile(); \
}



typedef enum
{
  NORMAL = 0,		/* �@��Ҧ� */
  SHOOTING,		/* ���u�o�g�Ҧ� */
  TELEPATHY		/* �믫�P���@�Τ� */
}Mmode;
static Mmode modes;	/* �ާ@�Ҧ� */

typedef enum
{
  ALIVE = 0,		/* Still Alive */
  STAGECLEAR,		/* �L�� */
  GAMEOVER		/* ���z�F... */
}MStatus;
static MStatus status;	/* �C�����A */

typedef enum
{
  MSG_NONE = 0,
  EXPLORER_FOUND,	/* ���������a�p */
  EXPLORER_404,		/* ���������o�{�a�p */
  STAND_DOLL,		/* �����H���o�� */
  MISSLE_FAILED,	/* ���u�ӱ���۾� */
  MISSLE_LAUNCH,	/* ���u��o�g�� */
}MineMsg;
static MineMsg globalmsg;
static int submsg;		/* �ưT�� */

typedef struct
{
  time_t sec;
  int csec;
}Timevalue;
static Timevalue BeginTime;	/* �}�l�ɶ� */
static Timevalue TelepathyEndT;	/* �믫�P���ĪG�@�ε����ɶ� */

static int xsize, ysize;		/* size of map */
static int TX, TY;			/* �a�Ϫ�ø�ϰ_�l(x, y) */
static int MAP_XMAX, MAP_YMAX;		/* �ۭq�a�Ϫ��e�פW���P���פW�� */
static int cx, cy;			/* current (x, y) */
static int sx, sy;			/* missile sight (x, y) */
static int dpx, dpy;			/* ���a�p����m�y�� */
static int mines;			/* �`�a�p�� */
static int turned;			/* �w½�}���a���� */
static int flagged;			/* �w�аO���a�p�� */
static int **map;			/* �u�C�����a�v */
static int **nei;			/* �C��|�P���h�֦a�p */
static int use_GF;			/* �ϥΪ�����P�_ */
static int trace_corrupt;		/* �ֳt½�}�O�_�Q���_ */
static usint lighton_times;		/* �}�O���� */

struct psr{		/* �ӤH�������c�� */
  int ptimes;
  time_t playtime;
  usint bestime[6];
  int wtimes[6];
  int item[MINE_NITEM];
  int storage[MINE_NITEM];
  short disable_stand;		/* �O�_���������H�����ϥ� */
  short pt_centisec;
};
static struct psr profile;	/* �ӤH�C������ */

struct rli{		/* �Ʀ�]���c�� */
  char id[IDLEN + 1];
  char date[19];
  usint cost;
};
static struct rli ranklist[6][5];	/* �Ʀ�] */




/* ----------------------------- */
/* Item effect function table	 */
/* ----------------------------- */

static int explorer(void);
static int missile(void);
static int telepathy(void);
static int remote_viewing(void);
static int golden_finger(void);

typedef struct
{
  int key;
  int ino;
  int delay;	//�ϥΫ᩵��(ms)
  int (*proc)(void);
}ItemFunc;

static const ItemFunc IFT[] =
{
  {'z', INO_EXPL, 300, explorer},
  {'x', INO_MISL, 1000, missile},
  {TELEPATHY_KEY, INO_TEPA, 0, telepathy},
  {'c', INO_REVI, 0, remote_viewing},
  {'i', INO_GLDF, 0, golden_finger},

  {0, 0, 0, NULL}
};



/* ----------------------------- */
/* Common Tools (declaration)	 */
/* ----------------------------- */

static void m_alcasrt(void*);
static int delay_vmsg(char*, int);
static int minekey(void);
static void item_recycler(void);
static void additem(int, int);
static void SaveProfile(void);
static void LoadProfile(void);
static void saverank(void);
static void loadrank(void);
static void mine_vsbar(char*);
static int is_broken_dbcs_string(char*);
static void clrtocol(int x);		//clear [curr_x, x)
static void shuffle_dirs(void);
static void gettimeofnow(Timevalue*);
static int BankerRounding(double);




/* ----------------------------- */
/* �C���D�{��(Game Main Process) */
/* ----------------------------- */

  /* ----------------------------------- */
  /* Game Main Process: ø�s����	 */
  /* ----------------------------------- */


static void
outpiece(x, y)
  int x;
  int y;
{
  if(modes == SHOOTING && x == cx && y == cy)
    outs(symCursor);
  else if(modes == TELEPATHY && x == cx && y == cy)
  {
    attrset(0x20);
    outs(symNumZero[nei[x][y]]);
  }
  else if(map[x][y] & TAGGED)
    outs(symFlag);
  else if(map[x][y] & EXPAND)
    outs(symNum[nei[x][y]]);
  else
    outs(symLand);

  attrset(7);
}


static void
redraw_itemfield(initialize)	//�ˬd�D����ݤ��ݭn��ø
  int initialize;
{
  static int itemlist[MINE_NITEM];
  static int is_TelepathyMode;
  static short disable_stand;

  if(initialize)
    memset(itemlist, 0xFF, sizeof(itemlist));

  if(profile.item[INO_EXPL]!=itemlist[INO_EXPL])
  {
    move(13, 0);
    if(profile.item[INO_EXPL]>0)
      prints("[z]������(%d) ", profile.item[INO_EXPL]);
    else
      clrtocol(TX_MIN);

    itemlist[INO_EXPL] = profile.item[INO_EXPL];
  }

  if(profile.item[INO_MISL]!=itemlist[INO_MISL])
  {
    move(14, 0);
    if(profile.item[INO_MISL]!=0)
      prints("[x]���u(%d) ", profile.item[INO_MISL]);
    else
      clrtocol(TX_MIN);

    itemlist[INO_MISL] = profile.item[INO_MISL];
  }

  if(profile.item[INO_TEPA]!=itemlist[INO_TEPA] || is_TelepathyMode!=(modes==TELEPATHY))
  {
    move(15, 0);
    if(modes == TELEPATHY)
      outs("\033[1;32m<<�믫�P���o�ʤ�>>\033[m");
    else if(profile.item[INO_TEPA]>0)
    {
      prints("[v]�믫�P��(%d) ", profile.item[INO_TEPA]);
      if(is_TelepathyMode)
	clrtocol(TX_MIN);	//"<<�믫�P���o�ʤ�>>"�����פ���� �M��
    }
    else
      clrtocol(TX_MIN);

    itemlist[INO_TEPA] = profile.item[INO_TEPA];
    is_TelepathyMode = modes==TELEPATHY;
  }

  if(profile.item[INO_REVI]!=itemlist[INO_REVI])
  {
    move(16, 0);
    if(profile.item[INO_REVI]>0)
      prints("[c]���j�z��(%d) ", profile.item[INO_REVI]);
    else
      clrtocol(TX_MIN);

    itemlist[INO_REVI] = profile.item[INO_REVI];
  }

  if(profile.item[INO_GLDF]!=itemlist[INO_GLDF])
  {
    move(17, 0);
    if(profile.item[INO_GLDF]>0)
      prints("[i]�����(%d) ", profile.item[INO_GLDF]);
    else
      clrtocol(TX_MIN);

    itemlist[INO_GLDF] = profile.item[INO_GLDF];
  }

  if(profile.item[INO_STAN]!=itemlist[INO_STAN] || profile.disable_stand != disable_stand)
  {
    move(18, 0);
    if(profile.item[INO_STAN]>0)
    {
      if(profile.disable_stand)
	prints("\033[1;30m<a>�ҥδ����H��(%d)\033[m", profile.item[INO_STAN]);
      else
	prints("<a>���δ����H��(%d)", profile.item[INO_STAN]);
    }
    else
      clrtocol(TX_MIN);

    itemlist[INO_STAN] = profile.item[INO_STAN];
    disable_stand = profile.disable_stand;
  }
}


static void
out_info()
{
  static MineMsg currmsg = MSG_NONE;
  static time_t msgcleartime = -1;
  static int pre_flagged = 0;

  if(globalmsg)
  {
    move(b_lines - 1, 0);
    clrtoeol();
    switch(globalmsg)
    {
    case EXPLORER_FOUND:
      prints("\033[1;33m�b(%d,%d)�B�o�{�a�p�A�w�аO�C\033[m", cx+1, cy+1);
      break;
    case EXPLORER_404:
      outs("\033[1;33m�S���o�{�a�p�C\033[m");
      break;
    case STAND_DOLL:
      prints("\033[1;33m�b(%d,%d)���a�p\033[m�A���δ����H���׶}�F�C", submsg>>8, submsg&0xFF);
      break;
    case MISSLE_FAILED:
      outs("�o�g�a�I�ӹL�a��ۤv�I");
      break;
    case MISSLE_LAUNCH:
      if(submsg)
	prints("�@�� %d �Ӧa�p�Q�M���C", submsg);
      else
	outs("�S���a�p�Q�M���C");
      break;
    }
    msgcleartime = time(NULL) + MSG_DUETIME;
    currmsg = globalmsg;
    globalmsg = MSG_NONE;
  }
  else if(currmsg && time(NULL)>msgcleartime)
  {
    move(b_lines - 1, 0);
    clrtoeol();
    msgcleartime = -1;
    currmsg = MSG_NONE;
  }

  redraw_itemfield(0);

  if(pre_flagged != flagged)
  {
    move(2, 6);
    prints(" %d ��  ", flagged);	//��s�w�аO��
    pre_flagged = flagged;
  }

  if(modes != TELEPATHY)
  {
    move(b_lines, 0);
    prints("�L�F %d ��...", time(NULL) - BeginTime.sec);
  }
}


static void
draw_left(style)
  int style;	/* 0:�@��Ҧ���  1:��w  2:draw_map(0)�� */
{
  int i;
  switch(style)
  {
  case 2:
    move(1, 0);
    prints("�@�� %d ���a�p", mines);
    move(2, 0);
    prints("�w�Х� %d ��", flagged);
  case 0:
    move(5, 0);
    outs("[��V��]���ʴ��");
    move(6, 0);
    outs("[�ť���]½�}");
    move(7, 0);
    outs("[f]�аO�a�p");
    move(8, 0);
    outs("[d]�ֳt½�}");
    move(9, 0);
    outs("[s]�ֳt�аO");
    move(10, 0);
    outs("[m]�^�D���");
    move(11, 0);
    outs("[r]�t�_�s��");
    move(12, 0);
    outs("[q]���}�C��");

    redraw_itemfield(1);
    break;

  case 1:
    move(5, 0);
    clrtocol(TX_MIN);
    move(6, 0);
    outs("�п�ܥؼСC");
    move(7, 0);
    clrtocol(TX_MIN);
    move(8, 0);
    clrtocol(TX_MIN);
    move(9, 0);
    outs(" [z]�ϥ�   ");
    move(10, 0);
    clrtocol(TX_MIN);
    move(11, 0);
    outs(" [c]����   ");
    for(i=12; i<=18; i++)
    {
      move(i, 0);
      clrtocol(TX_MIN);
    }
    break;
  }
}


static void
draw_map(style)		/* style => 0:ø�s�a��   1:½�}���a��(�����ɨϥ�) */
  int style;
{
  int x, y;

  switch(style)
  {
  case 0:
    move(1, 0);
    clrtobot();
    draw_left(2);
    for(y=0;y<ysize;y++)
    {
      move(TY+y, TX);
      for(x=0;x<xsize;x++)
	outs(symLand);
    }
    break;

  case 1:
    for(y=0;y<ysize;y++)
    {
      move(TY+y, TX);
      for(x=0;x<xsize;x++)
      {
	if(map[x][y] & TAGGED)
	{
	  if(map[x][y] & MINE)
	    outs(symFlag);
	  else
	    outs(symWrongflag);
	}
	else if(map[x][y] & MINE)
	{
	  if(x==dpx && y==dpy) attrset(0x17);
	  outs(symMine);
	  if(x==dpx && y==dpy) attrset(7);
	}
	else if(map[x][y] & EXPAND)
	  outs(symNum[nei[x][y]]);
	else
	  outs(symLand);
      }
    }
    break;
  }
}


static int
mine_yesno(msg, redraw)
  char *msg;
  int redraw;
{
  screen_backup_t oldscreen;
  int cc;

  scr_dump(&oldscreen);
  grayout(0, b_lines, GRAYOUT_DARK);

  move(6, 0); clrtocol(TX_MIN);
  move(7, 0);
  prints(" �u���n%s�ܡH", msg);
  clrtocol(TX_MIN);
  move(8, 0); clrtocol(TX_MIN);
  for(cc = 9; cc <= 12; cc++)
  {
    move(cc, 0);
    outs(" \033[36m�[\033[m");
    clrtocol(TX_MIN);
  }
  move(13, 0); clrtocol(TX_MIN);

  cc = 1;
  for(;;)
  {
    move(9, 3);
    if(cc == 0)
      outs("\033[1;44m[ �֩w���C ]\033[m");
    else
      outs("  �֩w���C  ");

    move(11, 3);
    if(cc == 1)
      outs("\033[1;44m[ �٬O��F ]\033[m");
    else
      outs("  �٬O��F  ");

    move(cc*2+9, 15);
    switch(minekey())
    {
    case KEY_UP:
    case KEY_DOWN:
      cc = !cc;
      break;

    case 'x':
      cc = 1;
    case 'z':
    case '\n':
    case ' ':
      if(cc || redraw)
	scr_restore(&oldscreen);
      else
	free(oldscreen.raw_memory);
      return !cc;
    }
  }

  return 0;
}



  /* ----------------------------------- */
  /* Game Main Process: �������	 */
  /* ----------------------------------- */

static inline void
make_invalidate(x, y, range)
  int x;
  int y;
  int range;
{
  int rx = x + range;
  int ry = y + range;
  int lx = x - range;

  y -= range;
  for(; y <= ry; y++)
  {
    for(x = lx; x <= rx; x++)
    {
      if(inrange(x, y))
	map[x][y] |= LDIRTY;
    }
  }
}


static int	//0:����  1:�T�w
target_lockon(range)
  int range;	//range * range
{
  int x, y, redraw = 1, ret = -1;

  range/=2;
  draw_left(1);
  while(ret < 0)
  {
    if(redraw)
    {
      make_invalidate(sx, sy, range);

      for(x=0;x<xsize;x++)
      {
	for(y=0;y<ysize;y++)
	{
	  if(map[x][y] & LDIRTY)
	  {
	    map[x][y] &= ~LDIRTY;
	    dmove(x, y);
	    if(abs(x-sx)<=range && abs(y-sy)<=range)
	      attrset(0x1F);
	    outpiece(x, y);
	  }
	}
      }

      redraw = 0;
    }

    cmove(sx, sy);
    switch(minekey())
    {
    case KEY_UP:
      make_invalidate(sx, sy, range);
      redraw = 1;
      sy--;
      if(sy<0)
	sy = ysize-1;
      break;

    case KEY_DOWN:
      make_invalidate(sx, sy, range);
      redraw = 1;
      sy++;
      if(sy>=ysize)
	sy = 0;
      break;

    case KEY_LEFT:
      make_invalidate(sx, sy, range);
      redraw = 1;
      sx--;
      if(sx<0)
	sx = xsize-1;
      break;

    case KEY_RIGHT:
      make_invalidate(sx, sy, range);
      redraw = 1;
      sx++;
      if(sx>=xsize)
	sx = 0;
      break;

    case 'z':
      if(modes == SHOOTING && in_explosion(cx, cy))	//�۾��b�F���d�򤺴N�L�k�ϥ�
      {
	SendMsg(MISSLE_FAILED);
	out_info();
	continue;
      }

      ret = 1;
      break;

    case 'c':
      ret = 0;
      break;
    }
  }

  for(y = sy-range; y <= sy+range; y++)
  {
    for(x = sx-range; x <= sx+range; x++)
    {
      if(inrange(x, y))
      {
	dmove(x, y);
	outpiece(x, y);
      }
    }
  }

  return ret;
}


static void
addflag(x, y)
  int x;
  int y;
{
  if(!(map[x][y] & TAGGED))
    flagged++;
  map[x][y]|=TAGGED;
  dmove(x, y);
  outpiece(x, y);
}


static void
rmvflag(x, y)
  int x;
  int y;
{
  if(map[x][y] & TAGGED)
    flagged--;
  map[x][y]&=~TAGGED;
  dmove(x, y);
  outpiece(x, y);
}


static void
expand(x, y)		//[�ť���]½�}
  int x;
  int y;
{
  int i, nx, ny;

  if(!inrange(x, y))
    return;

  if(map[x][y]&TAGGED)	//���W�X�l������L�k½�}
    return;

  if((map[x][y]&EXPAND) && modes!=SHOOTING)	//�D�g���Ҧ��U���L�w½�}�����
    return;

  if(map[x][y]&MVISIT)
    return;

  if(map[x][y]&MINE)	//�I�I
  {
    if(!profile.disable_stand && profile.item[INO_STAN]>0)	//�����H���o�ʡI
    {
      profile.item[INO_STAN]--;
      delay_vmsg("�a�p�I�����H���묹�F...", STAND_MDELAY);
      SendMsg(STAND_DOLL);
      submsg = x+1<<8 | y+1;
      trace_corrupt = 1;
    }
    else
    {
      dpx = x;
      dpy = y;
      status = GAMEOVER;
    }

    return;
  }

  if(!(map[x][y]&EXPAND))
  {
    turned++;
    map[x][y] |= EXPAND;
  }

  dmove(x, y);
  outpiece(x, y);

  if(turned + mines == xsize * ysize)	/* Stage Clear */
  {
    status = STAGECLEAR;
    return;
  }

  if(modes == SHOOTING)
  {
    map[x][y] |= MVISIT;
    for(i=0;i<8;i++)
    {
      nx = x + dir[i][0];
      ny = y + dir[i][1];
      if(inrange(nx, ny) && (map[nx][ny] & EXPAND))
      {
	expand(nx, ny);
	if(status != ALIVE)
	  return;
      }
    }
  }

  if(nei[x][y]==0)
  {
    for(i=0;i<8;i++)
    {
      nx = x + dir[i][0];
      ny = y + dir[i][1];
      expand(nx, ny);
      if(status != ALIVE)
	return;
    }
  }
}


static void
setflag(x, y)		//[f]���X
  int x;
  int y;
{
  if(map[x][y] & EXPAND)
    return ;

  if(map[x][y] & TAGGED)
    rmvflag(x, y);
  else
    addflag(x, y);
}


static void
lazy_setflag(x, y)	//[s]�ֳt�аO
  int x;
  int y;
{
  int i, nx, ny, expanded = 0;

  if(!(map[x][y] & EXPAND) || nei[x][y] < 4)	//��½�}�ζg��a�p�ƤӤ�
    return;

  for(i=0;i<8;i++)
  {
    nx = x + dir[i][0];
    ny = y + dir[i][1];
    if(inrange(nx, ny))
    {
      if(map[nx][ny] & EXPAND)
	expanded++;
    }
    else
      expanded++;
  }

  if(expanded + nei[x][y] != 8)
    return;

  for(i=0;i<8;i++)
  {
    nx = x + dir[i][0];
    ny = y + dir[i][1];
    if(inrange(nx, ny))
    {
      if(!(map[nx][ny] & TAGGED) && !(map[nx][ny] & EXPAND))
	addflag(nx, ny);
    }
  }
}


static void
trace_map(x, y)		//[d]�ֳt½�}
  int x;
  int y;
{
  int i, flags, nx, ny;

  if(!(map[x][y] & EXPAND))	/* �S½�}���g�a����trace */
    return ;

  flags = 0;
  for(i=0;i<8;i++)		/* �p��P��аO�� */
  {
    nx = x + dir[i][0];
    ny = y + dir[i][1];
    if(inrange(nx, ny))
    {
      if(map[nx][ny] & TAGGED)
	flags++;
    }
  }

  if(nei[x][y]==flags)
  {
    trace_corrupt = 0;
    shuffle_dirs();
    for(i=0;i<8;i++)
    {
      nx = x + dir[i][0];
      ny = y + dir[i][1];
      expand(nx, ny);
      if(status != ALIVE || trace_corrupt)
	return;		//�Dalive���A�ιJ�W�j��_�N���X
    }
  }
}


static int
explorer()		//[z]������
{
  if(map[cx][cy] & EXPAND)	/* �w½�}���N������ */
    return 0;

  profile.item[INO_EXPL]--;
  if(map[cx][cy] & MINE)
  {
    SendMsg(EXPLORER_FOUND);
    addflag(cx, cy);
  }
  else
  {
    SendMsg(EXPLORER_404);
    rmvflag(cx, cy);
  }

  return 1;
}



static void
rm_mine(x, y)	//���u�ά��a�p���
{
  int i, nx, ny;

  map[x][y] &= ~MINE;
  mines--;
  submsg++;

  for(i=0;i<8;i++)
  {
    nx = x + dir[i][0];
    ny = y + dir[i][1];
    if(inrange(nx, ny) && nei[nx][ny]>0)
      nei[nx][ny]--;
  }
}

static int
missile()		//[x]���u
{
  int i, nx, ny, used = 0;

  modes = SHOOTING;
  dmove(cx, cy);
  outpiece(cx, cy);
  sx = cx;
  sy = cy;

  if(target_lockon(3))
  {
    profile.item[INO_MISL]--;
    submsg = 0;
    for(i=0;i<9;i++)
    {
      nx = sx + dir[i][0];
      ny = sy + dir[i][1];

      if(inrange(nx, ny))
      {
	if(map[nx][ny] & TAGGED)
	  rmvflag(nx, ny);

	if(map[nx][ny] & MINE)
	  rm_mine(nx, ny);
      }
    }

    for(ny=sy-2;ny<=sy+2;ny++)
    {
      for(nx=sx-2;nx<=sx+2;nx++)
      {
	if(inrange(nx, ny) && (map[nx][ny]&EXPAND))
	{
	  dmove(nx, ny);
	  outpiece(nx, ny);	//��s���񪺼Ʀr
	}
      }
    }

    expand(sx, sy);
    for(nx=0;nx<xsize;nx++)
    {
      for(ny=0;ny<ysize;ny++)
	map[nx][ny] &= ~MVISIT;
    }

    SendMsg(MISSLE_LAUNCH);
    move(1, 5);
    prints("%d ���a�p\n", mines);
    used = 1;
  }

  modes = NORMAL;
  dmove(cx, cy);
  outpiece(cx, cy);

  draw_left(0);
  return used;
}


static int
telepathy()		//[v]�믫�P��
{
  modes = TELEPATHY;
  profile.item[INO_TEPA]--;
  gettimeofnow(&TelepathyEndT);
  TelepathyEndT.csec += TEPA_EffectTime/10;
  TelepathyEndT.sec += TelepathyEndT.csec/100;
  TelepathyEndT.csec %= 100;

  move(b_lines, 0);
  outs("�o�ʳѾl:\n");
  return 1;
}


static int
remote_viewing()	//[c]���j�z��
{
  screen_backup_t old_screen;
  int x, y, used = 0;

  sx = cx;
  sy = cy;
  if(target_lockon(5))
  {
    profile.item[INO_REVI]--;
    scr_dump(&old_screen);

    grayout(0, b_lines, GRAYOUT_DARK);
    attrset(15);
    for(y=sy-2;y<=sy+2;y++)
    {
      for(x=sx-2;x<=sx+2;x++)
      {
	if(inrange(x, y))
	{
	  dmove(x, y);
	  if(map[x][y] & MINE)
	  {
	    attrset(11);
	    outs(symMine);
	    attrset(15);
	  }
	  else if(map[x][y] & EXPAND)
	    outs(symNum[nei[x][y]]);
	  else
	    outs(symLand);
	}
      }
    }
    attrset(7);

    cmove(cx, cy);
    refresh();
    usleep(RV_Time*1000);

    scr_restore(&old_screen);
    used = 1;
  }

  draw_left(0);
  return used;
}


static int
golden_finger()		//[i]�����
{
  status = STAGECLEAR;
  use_GF = 1;
  profile.item[INO_GLDF]--;
  return 1;
}


static int
switch_stand()		//[a]���������H���ҥλP�_
{
  if(profile.item[INO_STAN]<1)	//�S�������H���ɤ�������
    return 0;

  profile.disable_stand = !profile.disable_stand;
  return 1;
}


static void
light_on()		//�Sԣ�Ϊ��K��
{
  int x, y;
  screen_backup_t old_screen;

  lighton_times++;
  scr_dump(&old_screen);
  for(y=0;y<ysize;y++)
  {
    move(TY+y, TX);
    for(x=0;x<xsize;x++)
    {
      if(map[x][y] & MINE)
	outs(symMine);
      else if(map[x][y] & EXPAND)
	outs(symNum[nei[x][y]]);
      else
	outs(symLand);
    }
  }

  grayout(0, b_lines, GRAYOUT_DARK);

  move(b_lines, 0);
  outs("\033[1;30mPress any key to continue...\033[m");
  clrtoeol();
  vkey();

  scr_restore(&old_screen);
}



  /* ----------------------------------- */
  /* Game Main Process: �����B�z	 */
  /* ----------------------------------- */

static void
getcostime(Timevalue *tv)
{
  gettimeofnow(tv);

  tv->sec -= BeginTime.sec;
  tv->csec -= BeginTime.csec;
  if(tv->csec < 0)
  {
    tv->sec--;
    tv->csec += 100;
  }
}


static void
add_playtime(Timevalue *tv)
{
  profile.playtime += tv->sec;
  profile.pt_centisec += tv->csec;
  if(profile.pt_centisec >= 100)
  {
    profile.playtime += profile.pt_centisec/100;
    profile.pt_centisec %= 100;
  }
}


static void
recover_item(arg)
  int arg;
{
  static int itemlist[MINE_NITEM];
  int i;

  if(arg == -1)		/* initialize */
  {
    for(i=0;i<MINE_NITEM;i++)
      itemlist[i] = profile.item[i];
  }
  else			/* recover */
  {
    for(i=0;i<MINE_NITEM;i++)
      profile.item[i] = itemlist[i];
  }
}


static int		/* 1:�i�]  0:�S���i�] */
chk_rank(lv, difft)
  int lv;
  Timevalue *difft;
{
  int i, j;
  time_t now = time(NULL);
  struct tm *t = localtime(&now);;
  usint ct = (usint)difft->sec*100+difft->csec;

  /* check personal best record */
  if(ct < profile.bestime[lv] || profile.bestime[lv] == 0)
    profile.bestime[lv] = ct;

  loadrank();

  /* check common ranking list */
  for(i=0;i<5;i++)
  {
    if(ranklist[lv][i].cost > ct || ranklist[lv][i].cost == 0)
      break;
  }

  if(i>=5)
    return 0;

  for(j=4;j>i;j--)
    ranklist[lv][j] = ranklist[lv][j-1];

  ranklist[lv][i].cost = ct;
  strcpy(ranklist[lv][i].id, cuser.userid);
  sprintf(ranklist[lv][i].date, "%d/%02d/%02d %02d:%02d",
    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

  saverank();
  return 1;
}


static void
print_bonus(bonus)
  int bonus;
{
  char digits[8];
  int i;

  for(i = 0; bonus > 0; i++)
  {
    digits[i] = bonus % 10;
    bonus /= 10;
  }

  clrtocol(8-i);
  attrset(15);
  while(--i>=0)
    outs(symNumZero[digits[i]]);
  attrset(7);
}


static int		/* 1:�i�]  0:�S���i�] */
clearbonus(lv, difft)
  int lv;
  Timevalue *difft;
{
  static const int EPorTEGetP[4] = {1, 5, 17, 33};	/* L1~L4 �o�찻�����κ믫�P�������v(X/100) */
  static const int GFGetP[2] = {20, 10};	/* L5�H�W�}���o��Golden Finger�����v(1/X) */
  static const int STGetP[3] = {15, 3, 1};	/* L4�H�W�}���o������H�������v(1/X) */

  static const int base_bonus[6][3] = {
    {30, 20, 15}, {125, 25, 77}, {288, 62, 177}, {538, 162, 340}, {835, 265, 556}, {1380, 820, 1080}
  };
  static const int baseT[6] = 	{3, 24, 53, 99, 146, 300};
  static const int decayT[6] = 	{21, 60, 146, 303, 464, 1200};
  static const int stdT[6] = 	{30, 82, 206, 404, 637, 1200};

  int i, y, bonus;
  int get_item[MINE_NITEM] = {0};
  Timevalue costime = *difft;

  profile.wtimes[lv-1]++;

  //clear bonus
  bonus = base_bonus[lv-1][0];
  if(use_GF)
    bonus += base_bonus[lv-1][1];
  else if(costime.sec >= baseT[lv-1] && costime.sec < decayT[lv-1])
    bonus -= BankerRounding((T_TO_F(costime)-baseT[lv-1])/3);
  else
  {
    bonus -= (decayT[lv-1] - baseT[lv-1])/3;
    if(costime.sec < stdT[lv-1])
      bonus -= BankerRounding(T_TO_F(costime)-decayT[lv-1]);
    else
    {
      bonus -= stdT[lv-1] - decayT[lv-1];
      bonus -= BankerRounding((T_TO_F(costime)-stdT[lv-1])*(bonus-base_bonus[lv-1][2])/stdT[lv-1]);
      if(bonus < base_bonus[lv-1][2])
	bonus = base_bonus[lv-1][2];
    }
  }

  addmoney(bonus);

  //special bonus
  if(!use_GF)
  {
    if(lv>=4 && rand()%STGetP[lv-5] == 0)
      get_item[INO_STAN] = 1;

    if(lv>=5)
    {
      get_item[INO_EXPL] = lv-4;
      get_item[INO_TEPA] = lv-4;

      if(rand()%GFGetP[lv-5] == 0)
	get_item[INO_GLDF] = 1;

      if(lv==6)
      {
	get_item[INO_MISL] = 1;
	if(rand()%3 == 0)
	  get_item[INO_STAN]++;
      }
    }
    else
    {
      if(rand()%100 < EPorTEGetP[lv-1])
      {
	if(rand()%3 == 0)
	  get_item[INO_TEPA] = 1;
	else
	  get_item[INO_EXPL] = 1;
      }
    }
  }

  //�L�X��
  move(5, 0);
  outs("\033[33mStage Clear Bonus\033[m");
  move(7, 0);
  print_bonus(bonus);

  y = 0;
  for(i = 0; i < MINE_NITEM; i++)
    y += get_item[i];
  if(y > 0)		//���o��D��
  {
    move(15, 0);
    outs("\033[1;30mSpecial Bonus\033[m");

    y = 16;
    for(i = 0; i < MINE_NITEM; i++)
    {
      if(get_item[i] > 0)
      {
	additem(i, get_item[i]);
	move(y++, 0);
	prints("   %s x%d", item_name[i], get_item[i]);
      }
    }
  }

  return use_GF? 0 : chk_rank(lv-1, difft);	//��������C�J�Ʀ�]
}


static void
draw_stageclear(lv, difft, mines)
  int lv;
  Timevalue *difft;
  int mines;
{
  int i, in_rank;

  for(i = 5; i <= 19; i++)
  {
    if(i>9 && i<14) continue;
    move(i, 0);
    clrtocol(TX_MIN);
  }

  if(lv==0)
  {
    move(5, 0);
    outs("\033[1;30m�a�ϸ�T\033[m");
    move(6, 0);
    prints("   %d*%d:%d", xsize, ysize, mines);

    if(lighton_times > 0)
    {
      move(18, 0);
      outs("\033[1;30m�}�O����\033[m");
      move(19, 0);
      prints("   %d", lighton_times);
    }

    in_rank = 0;
  }
  else
    in_rank = clearbonus(lv, difft);

  move(b_lines, 0);
  prints("\033[1;30m���}�ɶ�\033[m  %d.%02d��   ", difft->sec, difft->csec);

  if(in_rank)
    outs(MSG_InRank);
  else if(use_GF)
  {
    if(lv == 0)
      outs(MSG_usedGF_level0);
    else
      outs(MSG_usedGF);
  }
  clrtoeol();
}


static void
draw_gameover(difft)
  Timevalue *difft;
{
  int i;

  for(i = 5; i <= 7; i++)
  {
    move(i, 0);
    clrtocol(TX_MIN);
  }

  move(8, 0);
  outs("\033[1;31mGame Over\033[m");
  clrtocol(TX_MIN);
  move(9, 0);
  clrtocol(TX_MIN);

  for(i = 14; i <= 19; i++)
  {
    move(i, 0);
    clrtocol(TX_MIN);
  }

  move(b_lines, 0);
  if(turned == 0)
    outs("�Ĥ@�U�N���z�F�A�A�B��u�t......");
  else if(difft->sec < 1)
  {
    prints("�}�l�S�h�[(%d.%02dsec)�N���z�F�A�A�B��u�t...", difft->sec, difft->csec);
    i = rand()%3;
    while(i-->=0) outc('.');
  }
  else
  {
    prints("�ä�F %d.%02d ��A�̫��٬O�z�F...", difft->sec, difft->csec);
    if(difft->sec >= 90)
    {
      outc('.');
      if(difft->sec >= 270)
      {
	outc('.');
	if(difft->sec >= 600)
	  outc('.');
      }
    }
  }

  clrtoeol();
}


static int
game_finished()
{
  const char menu_ac[] = {'M',        'R',        'Q'};
  char * const fmenu[] = {"�^�D�e��", "�t�_�s��", "���}�C��"};
  int key, cc, oc;

  oc = (status == STAGECLEAR)? 0 : 1;

  for(cc = 0; cc < 3; cc++)
  {
    move(cc + 10, 0);
    attrset(6);
    outs("�[");

    if(cc == oc)
    {
      attrset(0x4F);
      prints(" [%c]", menu_ac[cc]);
    }
    else
    {
      attrset(7);
      prints(" (%c)", menu_ac[cc]);
    }
    prints(" %s ", fmenu[cc]);

    attrset(7);
    clrtocol(TX_MIN);
  }
  move(13, 0);
  outs("\033[36m�[\033[m");
  clrtocol(TX_MIN);

  cc = oc;
  move(cc + 10, 16);
  for(;;)
  {
    if(oc != cc)
    {
      move(oc + 10, 2);
      prints(" (%c) %s ", menu_ac[oc], fmenu[oc]);

      move(cc + 10, 2);
      attrset(0x4F);
      prints(" [%c] %s ", menu_ac[cc], fmenu[cc]);
      attrset(7);

      oc = cc;
    }

    switch(key = minekey())
    {
    case KEY_UP:
      cc = (cc+2)%3;
      break;

    case KEY_DOWN:
      cc = (cc+1)%3;
      break;

    case 'z':
    case '\n':
    case ' ':	//�����󪱹C���ɪ����
      return menu_ac[cc] | 0x20;	//��p�g���^�h

    case 'm':
    case 'r':
    case 'q':
      return key;
    }
  }
}



  /* ----------------------------------- */
  /* Game Main Process: �a�Ϫ�l��	 */
  /* ----------------------------------- */

static void
fake_floodfill(x, y, count)
  int x;
  int y;
  int *count;
{
  int i, nx, ny;

  map[x][y] |= MVISIT;
  (*count)++;

  if(nei[x][y]!=0)
    return;

  for(i=0;i<8;i++)
  {
    nx = x + dir[i][0];
    ny = y + dir[i][1];
    if(inrange(nx,ny))
    {
      if(!(map[nx][ny]&MVISIT) && !(map[nx][ny]&MINE))
	fake_floodfill(nx, ny, count);
    }
  }
}


static int
one_second_clear_check()
{
  int x, y, count = 0;

  for(x=0;x<xsize;x++)
  {
    for(y=0;y<ysize;y++)
    {
      if(nei[x][y]!=0 || (map[x][y]&MINE))
	continue;

      fake_floodfill(x, y, &count);
      return count+mines == xsize*ysize;
    }
  }

  return 0;
}


static int asdfjkl = 0;
static int
chos_lv()
{
  char * const *subtitle;
  char * const normal_st[7] = {
    "Beginner Mode", "Easy Mode", "Normal Mode", "Hard Mode", "Expert Mode", "Lunatic Mode", "Extra Mode"
  };
  char * const special_st[7] = {
    "�C��O��", "����O��", "�j��O��", " �j��O�� ", " �W��O�� ", " �����O�� ", "Anti-skill"
  };
  char * const comment[7] = {
    "    �ǻ��N���}�l", " �A�X�y�����L��ȫ�", "  ��a�p�@�몺��V",
    " ������a�p�����a��", "    �M�a�̪��a�L", "���J���a�p���g�H��", "     ��������V"
  };

  int cc, oc;


  move(1, 0);
  clrtobot();

  mine_vsbar("���׿��");
  subtitle = asdfjkl? special_st : normal_st;

  attrset(10);
  move(6, 29);
  outs("��            ��");
  move(7, 29);
  outs("��            ��");
  move(8, 29);
  outs("��            ��");
  attrset(7);

  move(15, 31);
  outs("�a�ϼe�סG");

  move(16, 31);
  outs("�a�Ϫ��סG");

  move(17, 31);
  outs("�a�p�ƥءG");

  move(b_lines, 0);
  outs(MCHLV_FEETER);


  cc = 0;
  oc = -1;
  for(;;)
  {
    if(oc!=cc)
    {
      move(5, 33);
      attrset(8);
      if((cc+6)%7 == 6)
	prints("�ۭq�a��    (%s)", subtitle[(cc+6)%7]);
      else
	prints("Level %s    (%s)", symNum[(cc+6)%7+1], subtitle[(cc+6)%7]);
      clrtoeol();

      move(7, 33);
      attrset(15);
      if(cc==6)
	outs("�ۭq�a��");
      else
	prints("Level %s", symNum[cc+1]);
      move(7, 45);
      prints("(%s)", subtitle[cc]);
      clrtoeol();

      move(9, 33);
      attrset(8);
      if((cc+1)%7 == 6)
	prints("�ۭq�a��    (%s)", subtitle[(cc+1)%7]);
      else
	prints("Level %s    (%s)", symNum[(cc+1)%7+1], subtitle[(cc+1)%7]);
      clrtoeol();

      if(!asdfjkl)
      {
	move(12, 27);
	clrtoeol();
	attrset(11);
	outs(comment[cc]);
      }

      attrset(7);
      if(cc==6)
      {
	move(15, 41);
	outs("�H");
	move(16, 41);
	outs("�H");
	move(17, 41);
	outs("�H\n");
      }
      else
      {
	move(15, 41);
	prints("%d", map_data[cc][0]);
	move(16, 41);
	prints("%d", map_data[cc][1]);
	move(17, 41);
	prints("%d (%.1f%%) ", map_data[cc][2], 100.0*map_data[cc][2]/(map_data[cc][0]*map_data[cc][1]));
      }

      oc = cc;
      move(b_lines, 79);
    }

    switch(minekey())
    {
    case KEY_UP:
      cc = (cc+6)%7;
      break;

    case KEY_DOWN:
      cc = (cc+1)%7;
      break;

    case 'z':
    case '\n':
      if(cc == 6)
	return 0;
      else
	return cc + 1;

    case 'x':
    case 'q':
      return -1;

    }
  }

  return -1;
}


static int		/* 0:����  1:��l�Ʀ��\ */
init_game(lv)
  int lv;
{
  int i, x, y, nx, ny;
  char msg[64], ans[8];
  Timevalue difft;
  struct timeval tv = {0, 0};

  if(lv<0)
    return 0;

  if(lv>=1 && lv<=6)
  {
    xsize = map_data[lv-1][0];
    ysize = map_data[lv-1][1];
    mines = map_data[lv-1][2];
  }
  else
  {
    move(b_lines-3, 0);
    clrtobot();
    sprintf(msg, "�п�J�a�Ϫ��e(2~%d)�G", MAP_XMAX);
    if(!vget(b_lines-2, 0, msg, ans, 4, DOECHO))
      return 0;
    xsize = atoi(ans);
    if(xsize>MAP_XMAX)
      xsize = MAP_XMAX;
    else if(xsize<2)
      xsize = 2;

    sprintf(msg, "�п�J�a�Ϫ���(2~%d)�G", MAP_YMAX);
    if(!vget(b_lines-1, 0, msg, ans, 4, DOECHO))
      return 0;
    ysize = atoi(ans);
    if(ysize>MAP_YMAX)
      ysize = MAP_YMAX;
    else if(ysize<2)
      ysize = 2;

    i = xsize*ysize*9/10;
    sprintf(msg, "�п�J�a�p��(1~%d)�G", i);
    if(!vget(b_lines, 0, msg, ans, 4, DOECHO))
      return 0;
    mines = atoi(ans);
    if(mines>i)
      mines = i;
    else if(mines<1)
      mines = 1;
    recover_item(-1);
  }

  clrregion(10, 14);
  attrset(8);
  move(11, 21); outs("�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w");
  move(12, 21); outs("    �m�ͦ��a�Ϥ��A�еy��......�n");
  move(13, 21); outs("�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w");
  attrset(7);

  gettimeofnow(&BeginTime);
  refresh();

  turned = 0;
  flagged = 0;
  cx = 0;
  cy = 0;
  modes = NORMAL;
  globalmsg = MSG_NONE;
  status = ALIVE;
  use_GF = 0;
  lighton_times = 0;
  redraw_itemfield(1);
  TV_SETZERO(TelepathyEndT);
  profile.ptimes++;
  TX = 35 - xsize/2*2;
  if(TX<TX_MIN)
    TX = TX_MIN;
  TY = 11 - ysize/2;
  if(TY<TY_MIN)
    TY = TY_MIN;

  for(;;)
  {
    for(i = 0; i < MAP_XMAX; i++)
    {
      memset(map[i], 0, sizeof(int) * MAP_YMAX);
      memset(nei[i], 0, sizeof(int) * MAP_YMAX);
    }

    if(lv!=0)
    {
      map[0][0] |= MINE;
      map[0][ysize-1] |= MINE;
      map[xsize-1][0] |= MINE;
      map[xsize-1][ysize-1] |= MINE;
    }
    for(i=0;i<mines;i++)
    {
      do
      {
	x = rand()%xsize;
	y = rand()%ysize;
				/* ��֦a����t���a�p�X�{�v */
      }while((map[x][y]&MINE) || (on_edge(x, y) && REDUCE_RATE));
      map[x][y] |= MINE;
    }
    if(lv!=0)
    {
      map[0][0] &= ~MINE;
      map[0][ysize-1] &= ~MINE;
      map[xsize-1][0] &= ~MINE;
      map[xsize-1][ysize-1] &= ~MINE;
    }

    /* ��l��nei[][] (�ˬd�C�@��P�򪺦a�p��) */
    for(x=0; x<xsize; x++)
    {
      for(y=0; y<ysize; y++)
      {
	for(i=0; i<8; i++)
	{
	  nx = x + dir[i][0];
	  ny = y + dir[i][1];
	  if(inrange(nx, ny))
	  {
	    if(map[nx][ny] & MINE)
	      nei[x][y]++;
	  }
	}
      }
    }

    if(lv!=0)	/* �ˬd�ݬݷ|���|���(�ۭq�a�Ϥ����ˬd) */
    {
      if(one_second_clear_check())
	continue;

      for(x=0; x<xsize; x++)	/* ���ˬd�ɩҥ[�W��MVISIT���� */
      {
	for(y=0; y<ysize; y++)
	  map[x][y] &= ~MVISIT;
      }
    }

    break;
  }

  getcostime(&difft);
  if(difft.sec < 1 && difft.csec < 40)	//�j���400ms
  {
    tv.tv_usec = (40 - difft.csec) * 10000;
    select(0, NULL, NULL, NULL, &tv);
  }

  draw_map(0);
  return 1;
}



  /* ----------------------------------- */
  /* Game Main Process: �֤ߨ��	 */
  /* ----------------------------------- */

static int		/* 1:���X�{�� 0:�^�D�e�� */
MineSweeper(lv)
  int lv;
{
  int key, rest_t, TimerSwitch = 0;
  Timevalue difft;
  const ItemFunc *iter;
  int mines_bakcup;

  if(!init_game(lv))
    return 0;

  for(;;)
  {
    if(modes == TELEPATHY)
    {
      gettimeofnow(&difft);
      if(TV_ISGREATER(TelepathyEndT, difft))
      {
	rest_t = (TelepathyEndT.sec - difft.sec)*100;
	rest_t += TelepathyEndT.csec - difft.csec;
	move(b_lines, 9);
	prints("%d.%02ds", rest_t/100, rest_t%100);
	dmove(cx, cy);
	outpiece(cx, cy);
      }
      else
      {
	BACK_FROM_TELEPATHY_MODE();
	out_info();
      }
    }

    cmove(cx, cy);
    key = minekey();

    if(!TimerSwitch)	/* ���w�p�ɪ���Ĥ@��������� */
    {
      gettimeofnow(&BeginTime);
      TimerSwitch = 1;
      mines_bakcup = mines;
      out_info();
    }

    switch(key)
    {
    case KEY_UP:
      TELEPATHY_RECOVER();
      cy--;
      if(cy<0)
	cy = ysize-1;
      break;

    case KEY_DOWN:
      TELEPATHY_RECOVER();
      cy++;
      if(cy>=ysize)
	cy = 0;
      break;

    case KEY_LEFT:
      TELEPATHY_RECOVER();
      cx--;
      if(cx<0)
	cx = xsize-1;
      break;

    case KEY_RIGHT:
      TELEPATHY_RECOVER();
      cx++;
      if(cx>=xsize)
	cx = 0;
      break;

    case ' ':		//½�}���
      expand(cx, cy);
      out_info();
      break;

    case 'f':		//�аO
      setflag(cx, cy);
      out_info();
      break;

    case 'd':		//�ֳt½�}
      trace_map(cx, cy);
      out_info();
      break;

    case 's':		//�ֳt�аO
      lazy_setflag(cx, cy);
      out_info();
      break;

    case 'a':		//�����O�_�ҥδ����H��
      if(switch_stand())
	out_info();
      break;

    case 'r':		//�t�_�s��
      if(mine_yesno("����", 1))
      {
	GAME_CORRUPT();
	if(!init_game(lv))
	  return 0;
	TimerSwitch = 0;
      }
      break;

    case 'm':		//�^�D���(ret 0)
    case 'q':		//���}�C��(ret 1)
      if(mine_yesno("���}", 0))
      {
	GAME_CORRUPT();
	return key == 'q';
      }
      break;

    case '\\':
      if(lv == 0)
      {
	light_on();
	out_info();
	break;
      }

    case TELEPATHY_KEY:	//�믫�P���@�Τ��N�����Y��TELEPATHY��key
      if(modes == TELEPATHY)
	break;
    default:
      for(iter = IFT; iter->key; iter++)
      {
	if(key == iter->key)
	{
	  if(profile.item[iter->ino] > 0)
	  {
	    BACK_FROM_TELEPATHY_MODE();
	    if(iter->proc() && iter->delay > 0)
	      usleep(iter->delay * 1000);
	    out_info();
	  }
	  break;
	}
      }
    }		//key process

    if(status != ALIVE)
    {
      GAME_CORRUPT();

      switch(status)
      {
      case STAGECLEAR:
	delay_vmsg("Stage Clear�I", 1000);

	draw_stageclear(lv, &difft, mines_bakcup);
	break;

      case GAMEOVER:
	draw_map(1);		//½�}���a��
	delay_vmsg("�I�I�I���a�p�F�I", 900);

	draw_gameover(&difft);
	break;
      }

      switch(game_finished())
      {
      case 'm':
	return 0;

      case 'r':
	if(!init_game(lv))
	  return 0;
	TimerSwitch = 0;
	break;

      case 'q':
	return 1;
      }
    }
  }		/* for(;;) */
}




/* ----------------------------- */
/* �ө�����			 */
/* ----------------------------- */

static void
item_desc(no, maxline)
  int no;
  int *maxline;
{
  int i, y, len;
  char line[DESC_LINELEN+1];

  move(6, 2);
  prints("%s (%d/%d)", item_name[no], profile.item[no], item_nlim[no]);
  clrtocol(25);
  len = strlen(desc[no]);
  for(i=0, y=8; i < len; y++)
  {
    str_ncpy(line, (char*)(desc[no]+i), sizeof(line));
    i += DESC_LINELEN;
    if(is_broken_dbcs_string(line))
    {
      line[DESC_LINELEN-1] = '\0';
      i--;
    }
    move(y, 6);
    prints("%-30s", line);
  }
  if(y>*maxline)
    *maxline = y;

  for(;y<*maxline;y++)
  {
    move(y, 6);

    for(i=0;i<DESC_LINELEN;i++)
      outc(' ');
  }
}


static void
npc_says(msg)
  char *msg;
{
  int len = strlen(msg) + 4;
  int i;

  move(16, 13);
  clrtoeol();
  attrset(0x57);
  for(i = 0; i < len; i++)
    outc(' ');

  move(18, 13);
  clrtoeol();
  for(i = 0; i < len; i++)
    outc(' ');

  move(17, 13);
  clrtoeol();
  prints("  \033[1m%s  \033[m", msg);
}


static void
shop()
{
  int cc, oc, upper, n, spend, bought;
  int desc_maxline = 9;
  char buf[8], msg[56];

  move(1, 0);
  clrtobot();

  mine_vsbar("�a�p�ө�");

  n = 2;
  for(n = 0; n < MINE_NITEM; n++)
  {
    move(2*n+2, 42);
    outs("�x");
    move(2*n+2+1, 42);
    prints("�x   %-12s %5d %s��", item_name[n], abs(itemcost[n]), itemcost[n]<0? "��":"��");
  }
  move(2*n+2, 42);
  outs("�x\n�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�r�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w");

  move(16, 0);
  outs(MSHOP_SALES);
  npc_says("�n�R����Ц۫K�C");

  move(b_lines, 0);
  prints(MSHOP_FEETER, cuser.money);

  cc = 0;
  oc = 1;
  for(;;)
  {
    if(oc != cc)
    {
      move(oc*2 + 2, 45);
      clrtoeol();
      move(oc*2 + 4, 45);
      clrtoeol();
      attrset(12);
      move(cc*2 + 2, 45);
      outs("��                       ��");
      move(cc*2 + 4, 45);
      outs("��                       ��");
      attrset(7);
      item_desc(cc, &desc_maxline);
      oc = cc;
    }

    move(b_lines, 79);
    switch(minekey())
    {
    case KEY_UP:
      cc = (cc+5)%6;
      break;

    case KEY_DOWN:
      cc = (cc+1)%6;
      break;

    case 'z':
    case '\n':
      bought = 0;
      if(profile.item[cc] >= item_nlim[cc])
      {
	npc_says("�A���o���D��w�g���F�A�L�k�ʶR��C");
	break;
      }

      if(itemcost[cc]>0)
      {
	upper = cuser.money/itemcost[cc];
	if (upper > item_nlim[cc]-profile.item[cc])
	  upper = item_nlim[cc]-profile.item[cc];
	if (upper <= 0)
	{
	  npc_says("�A���ȹ������C");
	  break;
	}
      }
      else
      {
	upper = cuser.gold/(-itemcost[cc]);
	if (upper > item_nlim[cc]-profile.item[cc])
	  upper = item_nlim[cc]-profile.item[cc];
	if (upper <= 0)
	{
	  npc_says("�A�����������C");
	  break;
	}
      }

      npc_says("�n�R�h�֡H");
      sprintf(msg, "�п�J�ƶq�G (0~%d) [0] ", upper);
      if(vget(17, 29, msg, buf, 3, DOECHO))
      {
	n = atoi(buf);
	if(n>0)		/* n>0�~�����n���U�C���� */
	{
	  if(n > upper)
	    n = upper;

	  spend = itemcost[cc];
	  if(spend>0)		//���O�ȹ�

	  {
	    spend *= n;
	    profile.item[cc] += n;
	    addmoney(-spend);
	  }
	  else			//���O����

	  {
	    spend *= -n;
	    profile.item[cc] += n;
	    addgold(-spend);
	  }
	  item_desc(cc, &desc_maxline);
	  move(b_lines, 0);
	  prints(MSHOP_FEETER, cuser.money);
	  bought = 1;
	}
      }

      if(bought)
	npc_says("���´f�U�C");
      else
	npc_says("�n�R����Ц۫K�C");

      break;

    case 'x':
    case 'q':
      SaveProfile();
      return ;
    }
  }	/* Key Cycle */
}




/* ----------------------------- */
/* �Ʀ�]			 */
/* ----------------------------- */

static void
rank_list()
{
  int lv, olv, i;
  const char const *xth[] = {"1st", "2nd", "3rd", "4th", "5th"};

  move(1, 0);
  clrtobot();

  mine_vsbar("�Ʀ�]");
  loadrank();

  move(b_lines, 0);
  outs(MRANK_FEETER);

  move(5, 8);
  outs("\033[1mLevel \033[m");

  lv = 1;
  olv = 6;
  for(;;)
  {
    if(olv!=lv)
    {
      if(lv==1)
      {
	move(4, 0);
	clrtoeol();
      }
      else if(olv==1)
      {
	move(4, 11);
	outs("\033[1;33m��\033[m");
      }

      if(lv==6)
      {
	move(6, 0);
	clrtoeol();
      }
      else if(olv==6)
      {
	move(6, 11);
	outs("\033[1;33m��\033[m");
      }

      move(5, 14);
      attrset(14);
      outs(symNum[lv]);
      attrset(7);

      for(i=0;i<5;i++)
      {
	move(8 + i*2, 17);
	attrset(15);
	if(ranklist[lv-1][i].cost>0)
	{
	  outs(xth[i]);
	  prints(" %-12s %6d.%02d��  ", ranklist[lv-1][i].id, ranklist[lv-1][i].cost/100, ranklist[lv-1][i].cost%100);
	  outs(ranklist[lv-1][i].date);
	}
	else
	  prints("%s ------------  ----------  ----/--/-- --:--", xth[i]);
	attrset(7);
      }
      olv = lv;
    }

    move(b_lines, 79);
    switch(minekey())
    {
    case KEY_UP:
      if(lv > 1)
	lv--;
      break;

    case KEY_DOWN:
      if(lv < 6)
	lv++;
      break;

    case 'x':
    case 'q':
      return;

    case 'z':
      if(!asdfjkl)
      {
	char buf[28];
	screen_backup_t old_screen;
	scr_dump(&old_screen);
	grayout(0, b_lines, GRAYOUT_DARK);
	move(b_lines - 3, 0);
	outs("Why are you really here?\n");
	outs("I want to make this land safe!\n");
	outs("Why are you here, soldier!\n");
	vget(b_lines, 0, "", buf, sizeof(buf), DOECHO);
	if(!strcmp(buf, "I'm here because I'm bored!"))
	  asdfjkl = 1;
	scr_restore(&old_screen);
      }
    }
  }
}




/* ----------------------------- */
/* �ܮw����			 */
/* ----------------------------- */

static void
storage_list(previous_cursor)
  int *previous_cursor;
{
  int i, cc, oc, n, upper;
  char buf[4], msg[32];

  move(1, 0);
  clrtobot();

  mine_vsbar("�D��ܮw");

  move(7, 0);
  outs("      �z �ܮw �w�w�w�w\242\033[30mw\033[m�ƶq/�W�� �{   �z ���W �w�w�w�w\242\033[30mw\033[m����/�W�� �{\n");
  outs("      �x                          �x   �x                          �x\n");
  for(i=0; i<MINE_NITEM; i++)
  {
    outs("      �x");
    PRINT_INSTRG(i, " ", " ");
    outs("�x   �x");
    PRINT_INHAND(i, " ", " ");
    outs("�x\n");
  }
  outs("      �x                          �x   �x                          �x\n");
  outs("      �|�w�w�w�w�w�w�w�w�w�w�w�w�w�}   �|�w�w�w�w�w�w�w�w�w�w�w�w�w�}");

  move(b_lines, 0);
  outs(MSTOR_FEETER);

  cc = *previous_cursor;
  oc = -1;
  for(;;)
  {
    if(oc!=cc)		/* ����ø�s */
    {
      if(oc>=0)
      {
	move((oc&0xFF)+9, oc>>8? 41:8);
	if(oc&0x100)
	  PRINT_INHAND(oc&0xFF, " ", " ");
	else
	  PRINT_INSTRG(oc&0xFF, " ", " ");
      }

      attrset(0x4F);
      move((cc&0xFF)+9, cc>>8? 41:8);
      if(cc&0x100)
	PRINT_INHAND(cc&0xFF, "[", "]");
      else
	PRINT_INSTRG(cc&0xFF, "[", "]");
      attrset(7);

      oc = cc;
    }

    move(b_lines, 79);
    switch(minekey())
    {
    case KEY_UP:
      if((cc&0xFF) > 0)
	cc--;
      else
	cc += MINE_NITEM-1;
      break;

    case KEY_DOWN:
      if((cc&0xFF) < MINE_NITEM-1)
	cc++;
      else
	cc -= MINE_NITEM-1;
      break;

    case KEY_LEFT:
      cc &= 0xFF;
      break;

    case KEY_RIGHT:
      cc |= 0x100;
      break;

    case 'z':
    case '\n':
      if(cc>>8 == 0)	/* ���X�D�� */
      {
	i = cc&0xFF;

	upper = profile.storage[i];
	if(upper > item_nlim[i] - profile.item[i])
	  upper = item_nlim[i] - profile.item[i];
	if(upper <= 0)
	{
	  move(18, 13);
	  clrtoeol();
	  if(profile.storage[i] == 0)
	    outs("�ܮw�S���F��i�H���X�C");
	  else
	    outs("���W�w�g���F�C");
	  break;
	}

	sprintf(msg, "�n���X�h�֡H (0~%d) [0] ", upper);
	if(vget(18, 13, msg, buf, 3, DOECHO))
	{
	  n = atoi(buf);
	  if(n>0)
	  {
	    if(n>upper)
	      n = upper;

	    profile.item[i] += n;
	    profile.storage[i] -= n;
	    move(i+9, 26);
	    prints("\033[1;44m%2d/%2d\033[m", profile.storage[i], item_slim[i]);
	    move(i+9, 59);
	    prints("%2d/%2d", profile.item[i], item_nlim[i]);
	  }
	}
	move(18, 13);
	clrtoeol();
      }
      else		/* �s�J�D�� */
      {
	i = cc&0xFF;

	upper = profile.item[i];
	if(upper > item_slim[i] - profile.storage[i])
	  upper = item_slim[i] - profile.storage[i];
	if(upper <= 0)
	{
	  move(18, 13);
	  clrtoeol();
	  if(profile.item[i]==0)
	    outs("���W�S���F��i�H��J�ܮw�C");
	  else
	    outs("�ܮw�w�g���F�C");
	  break;
	}

	sprintf(msg, "�n��J�h�֡H (0~%d) [0] ", upper);
	if(vget(18, 13, msg, buf, 3, DOECHO))
	{
	  n = atoi(buf);
	  if(n>0)
	  {
	    if(n>upper)
	      n=upper;

	    profile.item[i] -= n;
	    profile.storage[i] += n;
	    move(i+9, 26);
	    prints("%2d/%2d", profile.storage[i], item_slim[i]);
	    move(i+9, 59);
	    prints("\033[1;44m%2d/%2d\033[m", profile.item[i], item_nlim[i]);
	  }
	}
	move(18, 13);
	clrtoeol();
      }
      break;

    case 'x':
    case 'q':
      *previous_cursor = cc;
      return;
    }
  }
}




/* ----------------------------- */
/* ���a���			 */
/* ----------------------------- */

static void
view_prec()
{
  int i, day, hr, min;
  time_t sec;
  char buf[16], fpath[64];

  move(1, 0);
  clrtobot();

  move(2, 0);
  outs("�ע�               Level��   Level��   Level��   Level��   Level��   Level��\n\n");
  outs("               ��\n");
  outs("               ��\n");
  outs("�ʺ�           ��\n");
  outs("               ��\n\n");
  outs("               �L\n");
  outs("�`�C������     ��\n");
  outs("               �^\n");
  outs("               ��");

  move(4, 0);
  prints("  %s", cuser.userid);
  move(8, 0);
  prints("  %s", cuser.username);
  move(12, 0);
  prints("  %d ��", profile.ptimes);

  for(i=0;i<6;i++)
  {
    if(profile.bestime[i])
    {
      hr = profile.bestime[i]/100;
      min = profile.bestime[i]%100;
      sprintf(buf, "%d", hr);
      day = strlen(buf);
      if(day>6)
      {
	if(min >= 50)
	  hr++;
	sprintf(buf, "%ds", hr);
      }
      else if(day>5)
      {
	if(min%10>5)
	  min+=5;
	if(min >= 100)
	{
	  hr++;
	  min%=100;
	}
	sprintf(buf, "%d.%ds", hr, min/10);	//6.1
      }
      else
	sprintf(buf, "%d.%02ds", hr, min);	//5.2
    }
    else
      sprintf(buf, "-------");
    move(5, (9-strlen(buf))/2 + i*10 + 18);
    outs(buf);

    sprintf(buf, "%d", profile.wtimes[i]);
    move(10, (9-strlen(buf))/2 + i*10 + 18);
    outs(buf);
  }

  move(13, 25);
  outs("�z �����D�� �w�w�w �ƶq/�W��\033[30m\242\033[mw�{");
  for(i=0; i<MINE_NITEM; i++)
  {
    move(14+i, 25);
    prints("�x%-18s %2d/%2d    �x", item_name[i], profile.item[i], item_nlim[i]);
  }
  move(20, 25);
  outs("�|�w�w�w�w�w�w�w�w�w�w�w�w�w�w�}\n");

  outs("�`�C���ɼơG");
  i = sec = profile.playtime;
  if(profile.pt_centisec >= 50)
    sec++;
  if(sec>0)
  {
    day = sec/86400;
    if(day>0)
      prints("\033[1m%d\033[m�� ", day);
    sec %= 86400;

    hr = sec/3600;
    if(hr>0)
      prints("\033[1m%02d\033[m�� ", hr);
    sec %= 3600;

    min = sec/60;
    if(min>0)
      prints("\033[1m%02d\033[m�� ", min);
    sec %= 60;

    if(sec>0)
      prints("\033[1m%02d\033[m�� ", sec);
  }
  else
    outs("\033[1m0\033[m��");

  move(b_lines, 0);
  prints(MDATA_FEETER, cuser.money);

  for(;;)
  {
    switch(minekey())
    {
    case 'x':
    case 'q':
      break;

    case 'z':
    case '\n':
      shop();
      break;

    case 'd':
      move(b_lines, 0);
      clrtoeol();
      if(vans("ĵ�i�G�R����N�S��k�_��A�A�T�w�n�R����(Y/N)�H[N] ") == 'y')
      {
	if(!vget(b_lines, 0, "�п�J�K�X�H�~�����G", buf, PSWDLEN + 1, NOECHO))
	  break;
	if(chkpasswd(cuser.passwd, buf))
	  break;
	usr_fpath(fpath, cuser.userid, FN_MINE_PREC);
	unlink(fpath);
	memset(&profile, 0, sizeof(struct psr));
      }
      else
      {
	move(b_lines, 0);
	clrtoeol();
	prints(MDATA_FEETER, cuser.money);
	continue;
      }
      break;

    default:
      continue;

    }

    break;
  }		/* Key Process */

}




/* ----------------------------- */
/* �D�{��			 */
/* ----------------------------- */

int
main_mine()
{
  #define MY 14
  #define MX 26
  #define MMENU_LEN 7
  int i, cc, oc, redraw, quit;
  int shop_cursor, stli_cursor;
  char *mmenu[MMENU_LEN] = {
    "  Start Game   �}�l�C��", "   Mine Shop   �a�p�ө�",
    "  Item Depot   �D��ܮw", "Ranking Result  �Ʀ�]",
    "   Game Help   �C������", "  Player Data �d�ݭӤH���",
    "     Quit      ���}�C��"
  };


  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  MAP_XMAX = (b_cols - 19) / 2;
  MAP_YMAX = b_lines - 3;

  m_alcasrt(map = (int**) malloc(sizeof(int*) * MAP_XMAX));
  for(i = 0; i < MAP_XMAX; i++)
    m_alcasrt(map[i] = (int*) malloc(sizeof(int) * MAP_YMAX));

  m_alcasrt(nei = (int**) malloc(sizeof(int*) * MAP_XMAX));
  for(i = 0; i < MAP_XMAX; i++)
    m_alcasrt(nei[i] = (int*) malloc(sizeof(int) * MAP_YMAX));

  LoadProfile();

  item_recycler();	/* �D��^���{�� */

  oc = cc = 0;
  redraw = 1;
  quit = 0;
  shop_cursor = stli_cursor = 0;
  for(;;)
  {
    if(redraw)
    {
      clear();
      outs(MINE_BAR);
      outs(MINE_NOTE);
      for(i=0;i<MMENU_LEN;i++)
      {
	move(MY + i, MX);
	if(i==cc)
	  prints("\033[1m%s\033[m", mmenu[i]);
	else
	  prints("\033[1;30m%s\033[m", mmenu[i]);
      }
      move(b_lines, 1);
      prints("\033[;34m%s\033[m", GameVer);
      redraw = 0;
    }

    move(b_lines, 79);
    switch(minekey())
    {
    case KEY_UP:
      cc = (cc-1+MMENU_LEN)%MMENU_LEN;
      break;

    case KEY_DOWN:
      cc = (cc+1)%MMENU_LEN;
      break;

    case KEY_RIGHT:
    case '\n':
    case 'z':
      switch(cc)
      {
      case 0:
	if(MineSweeper(chos_lv()))
	  quit = 1;
	break;

      case 1:
	shop(&shop_cursor);
	break;

      case 2:
	storage_list(&stli_cursor);
	break;

      case 3:
	rank_list();
	break;

      case 4:
	more(FN_MINE_HELP, MFST_MINESWEEPER);
	break;

      case 5:
	view_prec();
	break;

      case 6:
	quit = 1;
      }

      redraw = 1;
      break;

    case 'x':
    case 'q':
    case KEY_LEFT:
      quit = 1;
    }

    if(quit)
      break;

    if(oc!=cc)		/* ���ʤF���� */
    {
      move(oc+MY, MX);
      clrtoeol();
      prints("\033[1;30m%s\033[m", mmenu[oc]);

      move(cc+MY, MX);
      clrtoeol();
      prints("\033[1m%s\033[m", mmenu[cc]);

      oc = cc;
    }
  }	/* Main Cycle */

  SaveProfile();

  for(i = 0; i < MAP_XMAX; i++)
    free(map[i]);
  free(map);
  for(i = 0; i < MAP_XMAX; i++)
    free(nei[i]);
  free(nei);

  return 0;
}




/* ----------------------------- */
/* Common Tools (implementation) */
/* ----------------------------- */

static void
m_alcasrt(p)
  void *p;
{
  if(p == NULL)
  {
    vmsg("�D�O������j(Bad Allocation)");
    abort_bbs();
  }
}


static int
delay_vmsg(msg, msec)
  char *msg;
  int msec;
{
  screen_backup_t old_screen;
  int i, y , x, len;
  char buf[80];

  scr_dump(&old_screen);

  len = strlen(msg);
  if(len < 18)		//strlen("�Ы����N���~��... ")
    len = 18;
  if(len % 2)
    len++;
  y = (b_lines - 4) / 2;
  x = (b_cols - 8 - len) / 2;

  strcpy(buf, "�~");
  for (i = -4; i < len; i += 2)
    strcat(buf, "�w");
  strcat(buf, "��");
  move(y++, x);
  outstr(buf);

  sprintf(buf, "�x" COLOR4 "  %-*s  \033[m�x", len, "�Ы����N���~��... ");
  move(y++, x);
  outstr(buf);

  strcpy(buf, "�u");
  for (i = -4; i < len; i += 2)
    strcat(buf, "�w");
  strcat(buf, "�t");
  move(y++, x);
  outstr(buf);

  sprintf(buf, "�x\033[30;47m  %-*s  \033[m�x", len, msg);
  move(y++, x);
  outstr(buf);

  strcpy(buf, "��");
  for (i = -4; i < len; i += 2)
    strcat(buf, "�w");
  strcat(buf, "��");
  move(y++, x);
  outstr(buf);

  move(b_lines, 0);
  refresh();
  usleep(msec * 1000);

  i = vkey();
  scr_restore(&old_screen);
  return i;
}


static int
minekey()	//��a�p�M��vkey()�A�|�N����ഫ���p�g���X
{
  int c;
  c = vkey();
  if(c >= 'A' && c <= 'Z')
    return c + 32;
  else
    return c;
}


static void
item_recycler()		/* �D��^���˸m */
{
  int i, d;
  int recycled = 0;

  for(i=0;i<MINE_NITEM;i++)
  {
    if(profile.item[i] > item_nlim[i])
    {
      profile.storage[i] += profile.item[i] - item_nlim[i];
      profile.item[i] = item_nlim[i];
      if(profile.storage[i] > item_slim[i])
      {
	recycled = 1;
	d = profile.storage[i] - item_slim[i];
	profile.storage[i] = item_slim[i];

	if(itemcost[i] >= 0)	/* ���O�ȹ� */
	  addmoney(d*itemcost[i]);
	else
	  addgold(d*(-itemcost[i]));
      }
    }
  }

  if(recycled)
    SaveProfile();
}


static void
additem(ino, n)
  int ino;
  int n;
{
  profile.item[ino] += n;
  if(profile.item[ino] > item_nlim[ino])
  {
    profile.storage[ino] += profile.item[ino] - item_nlim[ino];
    profile.item[ino] = item_nlim[ino];
    if(profile.storage[ino] > item_slim[ino])
      profile.storage[ino] = item_slim[ino];
  }
}


static void
SaveProfile()
{
  FILE *fp;
  char fpath[64];

  usr_fpath(fpath, cuser.userid, FN_MINE_PREC);
  if(fp = fopen(fpath, "wb"))
  {
    fwrite(&profile, 1, sizeof(struct psr), fp);
    fclose(fp);
  }
}


static void
LoadProfile()
{
  FILE *fp;
  char fpath[64];

  usr_fpath(fpath, cuser.userid, FN_MINE_PREC);
  if(fp = fopen(fpath, "rb"))
  {
    fread(&profile, 1, sizeof(struct psr), fp);
    fclose(fp);
  }
  else
  {
    memset(&profile, 0, sizeof(struct psr));
    SaveProfile();
  }
}


static void
saverank()
{
  FILE *fp;

  if(fp = fopen(FN_MINE_RANK, "wb"))
  {
    fwrite(ranklist, 30, sizeof(struct rli), fp);
    fclose(fp);
  }
}


static void
loadrank()
{
  FILE *fp;

  if(fp = fopen(FN_MINE_RANK, "rb"))
  {
    fread(ranklist, 30, sizeof(struct rli), fp);
    fclose(fp);
  }
  else
  {
    memset(ranklist, 0, sizeof(ranklist));
    saverank();
  }
}


static void
mine_vsbar(title)
  char *title;
{
  move(2, 0);
  prints("\033[1;33;44m�i %s �j\033[m", title);
}


static int
is_broken_dbcs_string(str)
  char *str;
{
  int in_zhc = 0;

  while(*str)
  {
    if(in_zhc)
      in_zhc = 0;
    else if(IS_ZHC_HI(*str))
      in_zhc = 1;
    str++;
  }

  return in_zhc;
}


static void
clrtocol(x)
  int x;
{
  int i;
  getyx(NULL, &i);
  for(; i<x; i++)
    outc(' ');
}


static void
shuffle_dirs()
{
  int i, j, tmp;
  for(i=0;i<8;i++)
  {
    j = rand()%8;
    tmp = vectors[i][0];		//x direction
    vectors[i][0] = vectors[j][0];
    vectors[j][0] = tmp;
    tmp = vectors[i][1];		//y direction
    vectors[i][1] = vectors[j][1];
    vectors[j][1] = tmp;
  }
}


static void
gettimeofnow(tv)
  Timevalue *tv;
{
  struct timeval now;

  gettimeofday(&now, NULL);
  tv->sec = now.tv_sec;
  now.tv_usec /= 1000;
  if(now.tv_usec%10 >= 5)
    now.tv_usec += 10;
  tv->csec = now.tv_usec/10;
}


static int
BankerRounding(a)
  double a;
{
  int x, non_zero = 0, is_odd;
  char buf[32], *p;

  sprintf(buf, "%lf", a);
  x = atoi(buf);
  if(p = strchr(buf, '.'))
  {
    is_odd = (p[-1]-'0')%2;
    p++;
    if(isdigit(*p))
    {
      if(*p>='6')
	x++;
      else if(*p=='5')
      {
	p++;
	while(*p)
	  non_zero += *p++ - '0';

	if(non_zero || is_odd)
	  x++;
      }
    }
  }

  return x;
}

#endif

