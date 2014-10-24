/*-------------------------------------------------------*/
/* mine.c 踩地雷遊戲					 */
/*-------------------------------------------------------*/
/* target : MineSweeper					 */
/* create : 08/06/04					 */
/* update : 10/09/25					 */
/* author : dust.bbs@tcirc.twbbs.org			 */
/*-------------------------------------------------------*/
/* 1.1以上的繪圖機制是完全建立在 piaip's flat terminal system 上
  安裝本遊戲前請注意站台是否已經安裝 pfterm */



#include "bbs.h"


#ifdef HAVE_GAME

#define GameVer "Ver1.1"

#define FN_MINE_HELP    "etc/game/mine.hlp"
#define FN_MINE_RANK    "run/minerank"
#define FN_MINE_PREC    "mine.rec"

#define MINE 1			/* 該格有地雷 */
#define TAGGED 0x100		/* 該格被標記 */
#define EXPAND 0x200		/* 該格已翻開 */
#define MVISIT 0x400		/* 該格已被拜訪過(shooting mode) */
#define LDIRTY 0x800		/* 該格需要重繪(shooting mode) */
#define TX_MIN 19		/* TX的最小值 */
#define TY_MIN 2		/* TY的最小值 */
#define TCENTER_X 35		/* 遊戲盤位置的參照中心點X */
#define TCENTER_Y 11		/* 遊戲盤位置的參照中心點Y */
#define RV_Time 2500		/* 遠隔透視顯示時間(ms) */
#define TEPA_EffectTime 3700	/* 精神感應作用時間(ms) */
#define STAND_MDELAY 800	/* 替身人型訊息延遲時間(ms) */
#define MSG_DUETIME 5		/* 訊息顯示時間(秒) */
#define REDUCE_RATE (rand()%5 < 2)	/* 地圖邊緣地雷出現減低率:40% */
#define DESC_LINELEN 30		/* 物品描述單行長度 */
#define symMine "⊙"		/* 地雷 */
#define symFlag "※"		/* 旗幟 */
#define symWrongflag "Ｘ"	/* 標示錯誤的旗幟 */
#define symLand "■"		/* 未翻開的地方 */
#define symCursor "□"		/* 自機(for shooting mode) */
#define TELEPATHY_KEY 'v'	/* 精神感應所使用的按鍵 */

#define MINE_NITEM 6		/* 道具種類數量 */
#define INO_EXPL 0		/* 各個item no */
#define INO_MISL 1
#define INO_TEPA 2
#define INO_STAN 3
#define INO_REVI 4
#define INO_GLDF 5

static const char * const symNum[9] = {		/* 顯示出來的數字 */
  "　", "１", "２", "３", "４", "５", "６", "７", "８"
};
static const char * const symNumZero[10] = {	/* 顯示出來的數字(0~9) */
  "０", "１", "２", "３", "４", "５", "６", "７", "８", "９"
};

static int vectors[9][2] = {
  {0,1},{0,-1},{1,0},{-1,0},		/* 黑歷史技術(?) */
  {1,1},{-1,-1},{1,-1},{-1,1},
  {0,0}
};
static const int (* const dir)[2] = (const int const (*)[2])vectors;

static const int item_nlim[MINE_NITEM] = {	/* 身上的道具數量上限 */
  11, 5, 10, 8, 2, 9
};

static const int item_slim[MINE_NITEM] = {	/* 倉庫容量上限 */
  99, 50, 99, 99, 50, 99
};

static const int itemcost[MINE_NITEM] = {	/* 物品價格 */
  250, 748, 277, 371, 850, -1	//正數代表以銀幣計價 負數代表以金幣計價
};

static char * const item_name[MINE_NITEM] = {
  "偵測儀", "飛彈", "精神感應", "替身人型", "遠隔透視", "金手指"
};

static char * const desc[MINE_NITEM] = {	/* 物品的描述 */
  "偵測游標下的方塊是否有地雷。有地雷的話會自動標記起來，沒有的話則什麼也不做。使用後延遲0.3秒。",
  "轟炸3x3範圍的方塊，清掉攻擊範圍內所有的東西。因此攻擊範圍不能涵蓋\自身。使用後延遲1秒。",
  "顯示目前所在方塊的周圍有多少顆地雷，效果持續3.7秒。",
  "無法主動使用，僅在踩到地雷時人型會代替自己被地雷炸飛，簡單來說就是擋箭牌。在遊戲中可按a停用或啟用人型的自動發動。",
  "顯示5x5範圍的方塊下有無地雷，持續2.5秒。使用期間無法進行任何動作。",
  "傳說中的外掛，使用後會直接過關並得到過關獎勵，但不算入個人最佳紀錄與排行榜。"
};

/* 地圖資訊:[等級][xsize, ysize, 地雷數量] */
static const int map_data[6][3] = {
  {10, 10, 10}, {15, 15, 30}, {20, 20, 60}, {25, 20, 90}, {30, 20, 120}, {30, 20, 150}
};



#define MSG_InRank "\033[1;36m(恭喜進榜！)\033[m"
#define MSG_usedGF "\033[34m(由於使用金手指故不計名次)\033[m"
#define MSG_usedGF_level0 "\033[1;30m(使用了金手指)\033[m"
#define MCHLV_FEETER "\033[m\033[1;36m 難度選擇 \033[37;44m    ↑↓:改變難度      z:選擇      x:回到主選單                      \033[m"
#define MSHOP_FEETER "\033[m\033[1;36m 地雷商店 \033[37;44m  ↑↓:移動游標    z:選擇    x:回主選單        %10d 銀幣       \033[m"
#define MSTOR_FEETER "\033[m\033[1;36m 道具倉庫 \033[37;44m  ↑↓← →:移動游標    z:取出/放入道具    x:回到主選單              \033[m"
#define MRANK_FEETER "\033[m\033[1;36m  排行榜  \033[37;44m    ↑↓:檢視其他難度         x:回到主選單                           \033[m"
#define MDATA_FEETER "\033[m\033[1;36m 個人資料 \033[37;44m  z:進入商店    x:回到主選單    d:刪除資料     %10d 銀幣       \033[m"


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
  NORMAL = 0,		/* 一般模式 */
  SHOOTING,		/* 飛彈發射模式 */
  TELEPATHY		/* 精神感應作用中 */
}Mmode;
static Mmode modes;	/* 操作模式 */

typedef enum
{
  ALIVE = 0,		/* Still Alive */
  STAGECLEAR,		/* 過關 */
  GAMEOVER		/* 踩爆了... */
}MStatus;
static MStatus status;	/* 遊戲狀態 */

typedef enum
{
  MSG_NONE = 0,
  EXPLORER_FOUND,	/* 偵測儀找到地雷 */
  EXPLORER_404,		/* 偵測儀未發現地雷 */
  STAND_DOLL,		/* 替身人型發動 */
  MISSLE_FAILED,	/* 飛彈太接近自機 */
  MISSLE_LAUNCH,	/* 飛彈剛發射後 */
}MineMsg;
static MineMsg globalmsg;
static int submsg;		/* 副訊息 */

typedef struct
{
  time_t sec;
  int csec;
}Timevalue;
static Timevalue BeginTime;	/* 開始時間 */
static Timevalue TelepathyEndT;	/* 精神感應效果作用結束時間 */

static int xsize, ysize;		/* size of map */
static int TX, TY;			/* 地圖的繪圖起始(x, y) */
static int MAP_XMAX, MAP_YMAX;		/* 自訂地圖的寬度上限與長度上限 */
static int cx, cy;			/* current (x, y) */
static int sx, sy;			/* missile sight (x, y) */
static int dpx, dpy;			/* 踩到地雷的位置座標 */
static int mines;			/* 總地雷數 */
static int turned;			/* 已翻開的地面數 */
static int flagged;			/* 已標記的地雷數 */
static int **map;			/* 「遊戲場地」 */
static int **nei;			/* 每格四周有多少地雷 */
static int use_GF;			/* 使用金手指與否 */
static int trace_corrupt;		/* 快速翻開是否被中斷 */
static usint lighton_times;		/* 開燈次數 */

struct psr{		/* 個人紀錄結構體 */
  int ptimes;
  time_t playtime;
  usint bestime[6];
  int wtimes[6];
  int item[MINE_NITEM];
  int storage[MINE_NITEM];
  short disable_stand;		/* 是否關閉替身人型的使用 */
  short pt_centisec;
};
static struct psr profile;	/* 個人遊戲紀錄 */

struct rli{		/* 排行榜結構體 */
  char id[IDLEN + 1];
  char date[19];
  usint cost;
};
static struct rli ranklist[6][5];	/* 排行榜 */




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
  int delay;	//使用後延遲(ms)
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
/* 遊戲主程序(Game Main Process) */
/* ----------------------------- */

  /* ----------------------------------- */
  /* Game Main Process: 繪製相關	 */
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
redraw_itemfield(initialize)	//檢查道具欄需不需要重繪
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
      prints("[z]偵測儀(%d) ", profile.item[INO_EXPL]);
    else
      clrtocol(TX_MIN);

    itemlist[INO_EXPL] = profile.item[INO_EXPL];
  }

  if(profile.item[INO_MISL]!=itemlist[INO_MISL])
  {
    move(14, 0);
    if(profile.item[INO_MISL]!=0)
      prints("[x]飛彈(%d) ", profile.item[INO_MISL]);
    else
      clrtocol(TX_MIN);

    itemlist[INO_MISL] = profile.item[INO_MISL];
  }

  if(profile.item[INO_TEPA]!=itemlist[INO_TEPA] || is_TelepathyMode!=(modes==TELEPATHY))
  {
    move(15, 0);
    if(modes == TELEPATHY)
      outs("\033[1;32m<<精神感應發動中>>\033[m");
    else if(profile.item[INO_TEPA]>0)
    {
      prints("[v]精神感應(%d) ", profile.item[INO_TEPA]);
      if(is_TelepathyMode)
	clrtocol(TX_MIN);	//"<<精神感應發動中>>"的長度比較長 清掉
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
      prints("[c]遠隔透視(%d) ", profile.item[INO_REVI]);
    else
      clrtocol(TX_MIN);

    itemlist[INO_REVI] = profile.item[INO_REVI];
  }

  if(profile.item[INO_GLDF]!=itemlist[INO_GLDF])
  {
    move(17, 0);
    if(profile.item[INO_GLDF]>0)
      prints("[i]金手指(%d) ", profile.item[INO_GLDF]);
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
	prints("\033[1;30m<a>啟用替身人型(%d)\033[m", profile.item[INO_STAN]);
      else
	prints("<a>停用替身人型(%d)", profile.item[INO_STAN]);
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
      prints("\033[1;33m在(%d,%d)處發現地雷，已標記。\033[m", cx+1, cy+1);
      break;
    case EXPLORER_404:
      outs("\033[1;33m沒有發現地雷。\033[m");
      break;
    case STAND_DOLL:
      prints("\033[1;33m在(%d,%d)踩到地雷\033[m，但用替身人型避開了。", submsg>>8, submsg&0xFF);
      break;
    case MISSLE_FAILED:
      outs("發射地點太過靠近自己！");
      break;
    case MISSLE_LAUNCH:
      if(submsg)
	prints("共有 %d 個地雷被清除。", submsg);
      else
	outs("沒有地雷被清除。");
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
    prints(" %d 顆  ", flagged);	//更新已標記數
    pre_flagged = flagged;
  }

  if(modes != TELEPATHY)
  {
    move(b_lines, 0);
    prints("過了 %d 秒...", time(NULL) - BeginTime.sec);
  }
}


static void
draw_left(style)
  int style;	/* 0:一般模式用  1:鎖定  2:draw_map(0)用 */
{
  int i;
  switch(style)
  {
  case 2:
    move(1, 0);
    prints("共有 %d 顆地雷", mines);
    move(2, 0);
    prints("已標示 %d 顆", flagged);
  case 0:
    move(5, 0);
    outs("[方向鍵]移動游標");
    move(6, 0);
    outs("[空白鍵]翻開");
    move(7, 0);
    outs("[f]標記地雷");
    move(8, 0);
    outs("[d]快速翻開");
    move(9, 0);
    outs("[s]快速標記");
    move(10, 0);
    outs("[m]回主選單");
    move(11, 0);
    outs("[r]另起新局");
    move(12, 0);
    outs("[q]離開遊戲");

    redraw_itemfield(1);
    break;

  case 1:
    move(5, 0);
    clrtocol(TX_MIN);
    move(6, 0);
    outs("請選擇目標。");
    move(7, 0);
    clrtocol(TX_MIN);
    move(8, 0);
    clrtocol(TX_MIN);
    move(9, 0);
    outs(" [z]使用   ");
    move(10, 0);
    clrtocol(TX_MIN);
    move(11, 0);
    outs(" [c]取消   ");
    for(i=12; i<=18; i++)
    {
      move(i, 0);
      clrtocol(TX_MIN);
    }
    break;
  }
}


static void
draw_map(style)		/* style => 0:繪製地圖   1:翻開全地圖(掛掉時使用) */
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
  prints(" 真的要%s嗎？", msg);
  clrtocol(TX_MIN);
  move(8, 0); clrtocol(TX_MIN);
  for(cc = 9; cc <= 12; cc++)
  {
    move(cc, 0);
    outs(" \033[36m︴\033[m");
    clrtocol(TX_MIN);
  }
  move(13, 0); clrtocol(TX_MIN);

  cc = 1;
  for(;;)
  {
    move(9, 3);
    if(cc == 0)
      outs("\033[1;44m[ 肯定的。 ]\033[m");
    else
      outs("  肯定的。  ");

    move(11, 3);
    if(cc == 1)
      outs("\033[1;44m[ 還是算了 ]\033[m");
    else
      outs("  還是算了  ");

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
  /* Game Main Process: 按鍵對應	 */
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


static int	//0:取消  1:確定
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
      if(modes == SHOOTING && in_explosion(cx, cy))	//自機在轟炸範圍內就無法使用
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
expand(x, y)		//[空白鍵]翻開
  int x;
  int y;
{
  int i, nx, ny;

  if(!inrange(x, y))
    return;

  if(map[x][y]&TAGGED)	//插上旗子的方塊無法翻開
    return;

  if((map[x][y]&EXPAND) && modes!=SHOOTING)	//非射擊模式下跳過已翻開的方塊
    return;

  if(map[x][y]&MVISIT)
    return;

  if(map[x][y]&MINE)	//碰！
  {
    if(!profile.disable_stand && profile.item[INO_STAN]>0)	//替身人型發動！
    {
      profile.item[INO_STAN]--;
      delay_vmsg("地雷！替身人型犧牲了...", STAND_MDELAY);
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
setflag(x, y)		//[f]插旗
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
lazy_setflag(x, y)	//[s]快速標記
  int x;
  int y;
{
  int i, nx, ny, expanded = 0;

  if(!(map[x][y] & EXPAND) || nei[x][y] < 4)	//未翻開或週邊地雷數太少
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
trace_map(x, y)		//[d]快速翻開
  int x;
  int y;
{
  int i, flags, nx, ny;

  if(!(map[x][y] & EXPAND))	/* 沒翻開的土地不能trace */
    return ;

  flags = 0;
  for(i=0;i<8;i++)		/* 計算周邊標記數 */
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
	return;		//非alive狀態或遇上強制中斷就跳出
    }
  }
}


static int
explorer()		//[z]偵測儀
{
  if(map[cx][cy] & EXPAND)	/* 已翻開的就不給用 */
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
rm_mine(x, y)	//飛彈用炸地雷函數
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
missile()		//[x]飛彈
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
	  outpiece(nx, ny);	//更新附近的數字
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
    prints("%d 顆地雷\n", mines);
    used = 1;
  }

  modes = NORMAL;
  dmove(cx, cy);
  outpiece(cx, cy);

  draw_left(0);
  return used;
}


static int
telepathy()		//[v]精神感應
{
  modes = TELEPATHY;
  profile.item[INO_TEPA]--;
  gettimeofnow(&TelepathyEndT);
  TelepathyEndT.csec += TEPA_EffectTime/10;
  TelepathyEndT.sec += TelepathyEndT.csec/100;
  TelepathyEndT.csec %= 100;

  move(b_lines, 0);
  outs("發動剩餘:\n");
  return 1;
}


static int
remote_viewing()	//[c]遠隔透視
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
golden_finger()		//[i]金手指
{
  status = STAGECLEAR;
  use_GF = 1;
  profile.item[INO_GLDF]--;
  return 1;
}


static int
switch_stand()		//[a]切換替身人型啟用與否
{
  if(profile.item[INO_STAN]<1)	//沒有替身人型時不給切換
    return 0;

  profile.disable_stand = !profile.disable_stand;
  return 1;
}


static void
light_on()		//沒啥用的密技
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
  /* Game Main Process: 結束處理	 */
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


static int		/* 1:進榜  0:沒有進榜 */
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


static int		/* 1:進榜  0:沒有進榜 */
clearbonus(lv, difft)
  int lv;
  Timevalue *difft;
{
  static const int EPorTEGetP[4] = {1, 5, 17, 33};	/* L1~L4 得到偵測儀或精神感應的機率(X/100) */
  static const int GFGetP[2] = {20, 10};	/* L5以上破關得到Golden Finger的機率(1/X) */
  static const int STGetP[3] = {15, 3, 1};	/* L4以上破關得到替身人型的機率(1/X) */

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

  //印出來
  move(5, 0);
  outs("\033[33mStage Clear Bonus\033[m");
  move(7, 0);
  print_bonus(bonus);

  y = 0;
  for(i = 0; i < MINE_NITEM; i++)
    y += get_item[i];
  if(y > 0)		//有得到道具
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

  return use_GF? 0 : chk_rank(lv-1, difft);	//金手指不列入排行榜
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
    outs("\033[1;30m地圖資訊\033[m");
    move(6, 0);
    prints("   %d*%d:%d", xsize, ysize, mines);

    if(lighton_times > 0)
    {
      move(18, 0);
      outs("\033[1;30m開燈次數\033[m");
      move(19, 0);
      prints("   %d", lighton_times);
    }

    in_rank = 0;
  }
  else
    in_rank = clearbonus(lv, difft);

  move(b_lines, 0);
  prints("\033[1;30m擊破時間\033[m  %d.%02d秒   ", difft->sec, difft->csec);

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
    outs("第一下就踩爆了，你運氣真差......");
  else if(difft->sec < 1)
  {
    prints("開始沒多久(%d.%02dsec)就踩爆了，你運氣真差...", difft->sec, difft->csec);
    i = rand()%3;
    while(i-->=0) outc('.');
  }
  else
  {
    prints("掙扎了 %d.%02d 秒，最後還是爆了...", difft->sec, difft->csec);
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
  char * const fmenu[] = {"回主畫面", "另起新局", "離開遊戲"};
  int key, cc, oc;

  oc = (status == STAGECLEAR)? 0 : 1;

  for(cc = 0; cc < 3; cc++)
  {
    move(cc + 10, 0);
    attrset(6);
    outs("︴");

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
  outs("\033[36m︴\033[m");
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
    case ' ':	//相應於玩遊戲時的鍵位
      return menu_ac[cc] | 0x20;	//轉小寫扔回去

    case 'm':
    case 'r':
    case 'q':
      return key;
    }
  }
}



  /* ----------------------------------- */
  /* Game Main Process: 地圖初始化	 */
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
    "低能力者", "異能力者", "強能力者", " 大能力者 ", " 超能力者 ", " 絕對能力者 ", "Anti-skill"
  };
  char * const comment[7] = {
    "    傳說就此開始", " 適合悠閒的夏日午後", "  踩地雷一般的方向",
    " 熟知踩地雷的玩家們", "    專家們的地盤", "熱衷於踩地雷的狂人們", "     未知的方向"
  };

  int cc, oc;


  move(1, 0);
  clrtobot();

  mine_vsbar("難度選擇");
  subtitle = asdfjkl? special_st : normal_st;

  attrset(10);
  move(6, 29);
  outs("╔            ╗");
  move(7, 29);
  outs("║            ║");
  move(8, 29);
  outs("╚            ╝");
  attrset(7);

  move(15, 31);
  outs("地圖寬度：");

  move(16, 31);
  outs("地圖長度：");

  move(17, 31);
  outs("地雷數目：");

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
	prints("自訂地圖    (%s)", subtitle[(cc+6)%7]);
      else
	prints("Level %s    (%s)", symNum[(cc+6)%7+1], subtitle[(cc+6)%7]);
      clrtoeol();

      move(7, 33);
      attrset(15);
      if(cc==6)
	outs("自訂地圖");
      else
	prints("Level %s", symNum[cc+1]);
      move(7, 45);
      prints("(%s)", subtitle[cc]);
      clrtoeol();

      move(9, 33);
      attrset(8);
      if((cc+1)%7 == 6)
	prints("自訂地圖    (%s)", subtitle[(cc+1)%7]);
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
	outs("？");
	move(16, 41);
	outs("？");
	move(17, 41);
	outs("？\n");
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


static int		/* 0:取消  1:初始化成功 */
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
    sprintf(msg, "請輸入地圖的寬(2~%d)：", MAP_XMAX);
    if(!vget(b_lines-2, 0, msg, ans, 4, DOECHO))
      return 0;
    xsize = atoi(ans);
    if(xsize>MAP_XMAX)
      xsize = MAP_XMAX;
    else if(xsize<2)
      xsize = 2;

    sprintf(msg, "請輸入地圖的長(2~%d)：", MAP_YMAX);
    if(!vget(b_lines-1, 0, msg, ans, 4, DOECHO))
      return 0;
    ysize = atoi(ans);
    if(ysize>MAP_YMAX)
      ysize = MAP_YMAX;
    else if(ysize<2)
      ysize = 2;

    i = xsize*ysize*9/10;
    sprintf(msg, "請輸入地雷數(1~%d)：", i);
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
  move(11, 21); outs("──────────────────");
  move(12, 21); outs("    《生成地圖中，請稍候......》");
  move(13, 21); outs("──────────────────");
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
				/* 減少地圖邊緣的地雷出現率 */
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

    /* 初始化nei[][] (檢查每一格周圍的地雷數) */
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

    if(lv!=0)	/* 檢查看看會不會秒殺(自訂地圖不必檢查) */
    {
      if(one_second_clear_check())
	continue;

      for(x=0; x<xsize; x++)	/* 把檢查時所加上的MVISIT拿掉 */
      {
	for(y=0; y<ysize; y++)
	  map[x][y] &= ~MVISIT;
      }
    }

    break;
  }

  getcostime(&difft);
  if(difft.sec < 1 && difft.csec < 40)	//強制延遲400ms
  {
    tv.tv_usec = (40 - difft.csec) * 10000;
    select(0, NULL, NULL, NULL, &tv);
  }

  draw_map(0);
  return 1;
}



  /* ----------------------------------- */
  /* Game Main Process: 核心函數	 */
  /* ----------------------------------- */

static int		/* 1:跳出程式 0:回主畫面 */
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

    if(!TimerSwitch)	/* 延緩計時直到第一次按按鍵時 */
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

    case ' ':		//翻開方塊
      expand(cx, cy);
      out_info();
      break;

    case 'f':		//標記
      setflag(cx, cy);
      out_info();
      break;

    case 'd':		//快速翻開
      trace_map(cx, cy);
      out_info();
      break;

    case 's':		//快速標記
      lazy_setflag(cx, cy);
      out_info();
      break;

    case 'a':		//切換是否啟用替身人型
      if(switch_stand())
	out_info();
      break;

    case 'r':		//另起新局
      if(mine_yesno("重來", 1))
      {
	GAME_CORRUPT();
	if(!init_game(lv))
	  return 0;
	TimerSwitch = 0;
      }
      break;

    case 'm':		//回主選單(ret 0)
    case 'q':		//離開遊戲(ret 1)
      if(mine_yesno("離開", 0))
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

    case TELEPATHY_KEY:	//精神感應作用中就直接吃掉TELEPATHY的key
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
	delay_vmsg("Stage Clear！", 1000);

	draw_stageclear(lv, &difft, mines_bakcup);
	break;

      case GAMEOVER:
	draw_map(1);		//翻開全地圖
	delay_vmsg("碰！！踩到地雷了！", 900);

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
/* 商店介面			 */
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

  mine_vsbar("地雷商店");

  n = 2;
  for(n = 0; n < MINE_NITEM; n++)
  {
    move(2*n+2, 42);
    outs("│");
    move(2*n+2+1, 42);
    prints("│   %-12s %5d %s幣", item_name[n], abs(itemcost[n]), itemcost[n]<0? "金":"銀");
  }
  move(2*n+2, 42);
  outs("│\n─────────────────────┴─────────────────");

  move(16, 0);
  outs(MSHOP_SALES);
  npc_says("要買什麼請自便。");

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
      outs("╔                       ╗");
      move(cc*2 + 4, 45);
      outs("╚                       ╝");
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
	npc_says("你的這項道具已經滿了，無法購買唷。");
	break;
      }

      if(itemcost[cc]>0)
      {
	upper = cuser.money/itemcost[cc];
	if (upper > item_nlim[cc]-profile.item[cc])
	  upper = item_nlim[cc]-profile.item[cc];
	if (upper <= 0)
	{
	  npc_says("你的銀幣不夠。");
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
	  npc_says("你的金幣不夠。");
	  break;
	}
      }

      npc_says("要買多少？");
      sprintf(msg, "請輸入數量： (0~%d) [0] ", upper);
      if(vget(17, 29, msg, buf, 3, DOECHO))
      {
	n = atoi(buf);
	if(n>0)		/* n>0才有必要做下列的事 */
	{
	  if(n > upper)
	    n = upper;

	  spend = itemcost[cc];
	  if(spend>0)		//單位是銀幣

	  {
	    spend *= n;
	    profile.item[cc] += n;
	    addmoney(-spend);
	  }
	  else			//單位是金幣

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
	npc_says("謝謝惠顧。");
      else
	npc_says("要買什麼請自便。");

      break;

    case 'x':
    case 'q':
      SaveProfile();
      return ;
    }
  }	/* Key Cycle */
}




/* ----------------------------- */
/* 排行榜			 */
/* ----------------------------- */

static void
rank_list()
{
  int lv, olv, i;
  const char const *xth[] = {"1st", "2nd", "3rd", "4th", "5th"};

  move(1, 0);
  clrtobot();

  mine_vsbar("排行榜");
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
	outs("\033[1;33m▲\033[m");
      }

      if(lv==6)
      {
	move(6, 0);
	clrtoeol();
      }
      else if(olv==6)
      {
	move(6, 11);
	outs("\033[1;33m▼\033[m");
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
	  prints(" %-12s %6d.%02d秒  ", ranklist[lv-1][i].id, ranklist[lv-1][i].cost/100, ranklist[lv-1][i].cost%100);
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
/* 倉庫介面			 */
/* ----------------------------- */

static void
storage_list(previous_cursor)
  int *previous_cursor;
{
  int i, cc, oc, n, upper;
  char buf[4], msg[32];

  move(1, 0);
  clrtobot();

  mine_vsbar("道具倉庫");

  move(7, 0);
  outs("      ┌ 倉庫 ────\242\033[30mw\033[m數量/上限 ┐   ┌ 身上 ────\242\033[30mw\033[m持有/上限 ┐\n");
  outs("      │                          │   │                          │\n");
  for(i=0; i<MINE_NITEM; i++)
  {
    outs("      │");
    PRINT_INSTRG(i, " ", " ");
    outs("│   │");
    PRINT_INHAND(i, " ", " ");
    outs("│\n");
  }
  outs("      │                          │   │                          │\n");
  outs("      └─────────────┘   └─────────────┘");

  move(b_lines, 0);
  outs(MSTOR_FEETER);

  cc = *previous_cursor;
  oc = -1;
  for(;;)
  {
    if(oc!=cc)		/* 光棒繪製 */
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
      if(cc>>8 == 0)	/* 取出道具 */
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
	    outs("倉庫沒有東西可以取出。");
	  else
	    outs("身上已經滿了。");
	  break;
	}

	sprintf(msg, "要取出多少？ (0~%d) [0] ", upper);
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
      else		/* 存入道具 */
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
	    outs("身上沒有東西可以放入倉庫。");
	  else
	    outs("倉庫已經滿了。");
	  break;
	}

	sprintf(msg, "要放入多少？ (0~%d) [0] ", upper);
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
/* 玩家資料			 */
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
  outs("ＩＤ               Level１   Level２   Level３   Level４   Level５   Level６\n\n");
  outs("               最\n");
  outs("               佳\n");
  outs("暱稱           紀\n");
  outs("               錄\n\n");
  outs("               過\n");
  outs("總遊戲次數     關\n");
  outs("               回\n");
  outs("               數");

  move(4, 0);
  prints("  %s", cuser.userid);
  move(8, 0);
  prints("  %s", cuser.username);
  move(12, 0);
  prints("  %d 次", profile.ptimes);

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
  outs("┌ 持有道具 ─── 數量/上限\033[30m\242\033[mw┐");
  for(i=0; i<MINE_NITEM; i++)
  {
    move(14+i, 25);
    prints("│%-18s %2d/%2d    │", item_name[i], profile.item[i], item_nlim[i]);
  }
  move(20, 25);
  outs("└──────────────┘\n");

  outs("總遊戲時數：");
  i = sec = profile.playtime;
  if(profile.pt_centisec >= 50)
    sec++;
  if(sec>0)
  {
    day = sec/86400;
    if(day>0)
      prints("\033[1m%d\033[m天 ", day);
    sec %= 86400;

    hr = sec/3600;
    if(hr>0)
      prints("\033[1m%02d\033[m時 ", hr);
    sec %= 3600;

    min = sec/60;
    if(min>0)
      prints("\033[1m%02d\033[m分 ", min);
    sec %= 60;

    if(sec>0)
      prints("\033[1m%02d\033[m秒 ", sec);
  }
  else
    outs("\033[1m0\033[m秒");

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
      if(vans("警告：刪除後就沒辦法復原，你確定要刪除嗎(Y/N)？[N] ") == 'y')
      {
	if(!vget(b_lines, 0, "請輸入密碼以繼續執行：", buf, PSWDLEN + 1, NOECHO))
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
/* 主程序			 */
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
    "  Start Game   開始遊戲", "   Mine Shop   地雷商店",
    "  Item Depot   道具倉庫", "Ranking Result  排行榜",
    "   Game Help   遊戲說明", "  Player Data 查看個人資料",
    "     Quit      離開遊戲"
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

  item_recycler();	/* 道具回收程序 */

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

    if(oc!=cc)		/* 移動了光棒 */
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
    vmsg("主記憶體用罄(Bad Allocation)");
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
  if(len < 18)		//strlen("請按任意鍵繼續... ")
    len = 18;
  if(len % 2)
    len++;
  y = (b_lines - 4) / 2;
  x = (b_cols - 8 - len) / 2;

  strcpy(buf, "╭");
  for (i = -4; i < len; i += 2)
    strcat(buf, "─");
  strcat(buf, "╮");
  move(y++, x);
  outstr(buf);

  sprintf(buf, "│" COLOR4 "  %-*s  \033[m│", len, "請按任意鍵繼續... ");
  move(y++, x);
  outstr(buf);

  strcpy(buf, "├");
  for (i = -4; i < len; i += 2)
    strcat(buf, "─");
  strcat(buf, "┤");
  move(y++, x);
  outstr(buf);

  sprintf(buf, "│\033[30;47m  %-*s  \033[m│", len, msg);
  move(y++, x);
  outstr(buf);

  strcpy(buf, "╰");
  for (i = -4; i < len; i += 2)
    strcat(buf, "─");
  strcat(buf, "╯");
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
minekey()	//踩地雷專用vkey()，會將鍵值轉換成小寫後輸出
{
  int c;
  c = vkey();
  if(c >= 'A' && c <= 'Z')
    return c + 32;
  else
    return c;
}


static void
item_recycler()		/* 道具回溯裝置 */
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

	if(itemcost[i] >= 0)	/* 單位是銀幣 */
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
  prints("\033[1;33;44m【 %s 】\033[m", title);
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

