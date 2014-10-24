/*-------------------------------------------------------*/
/* dlve.c						 */
/*-------------------------------------------------------*/
/* target : enhanced BBS visual editor			 */
/* create : 10/06/29					 */
/* update : 13/03/14					 */
/*-------------------------------------------------------*/



#include "bbs.h"


#define DLVE_VER        "1.04"




static void
out_of_memory()	/* exception handling of memory allocation */
{
  vmsg("系統記憶體不足");
  abort();
}


/* ----------------------------------------------------- */
/* pfterm common					 */
/* these macros should be defined as in pfterm		 */
/* ----------------------------------------------------- */

#define FTCMD_MAXLEN	(63)
#ifndef ANSI_IS_PARAM
# define ANSI_IS_PARAM(c) (c == ';' || (c >= '0' && c <= '9'))
#endif
#define PFTERM_STILL_IN_ANSI_SEQ(ch, ftcmd_len) (ANSI_IS_PARAM(ch) && ftcmd_len < FTCMD_MAXLEN)



/* ----------------------------------------------------- */
/* List Editor environment settings			 */
/* ----------------------------------------------------- */

/* 單行長度上限，最大99998 */
#define COL_LIMIT 99998

/* 總行數上限，最大99999 */
#define LINE_LIMIT 99999

/* 總字數上限 */
#define TEXT_LIMIT 16777216

/* 額外空間的配置量上限 */
#define MEM_LIMIT 149645

/* 搜尋長度上限 */
#define VESEARCH_MAXLEN 39

/* text space大小，最大255 */
#define TEXTSPACEMAXLEN 119

/* 合併推文時不顯示提示訊息 */
#undef SILENT_MERGE

/* 在編輯器內改變KCP等模式時會寫回ufo */
#define SAVE_CONFIG

/* 監控記憶體使用量 */
#define DBG_MEMORY_MONITOR

/* 端看pmore有沒有定義這項。沒有裝pmore的話請務必#undef這項，否則會編譯錯誤 */
#define PMORE_USE_ASCII_MOVIE

/* pmore語法上色 */
#ifdef PMORE_USE_ASCII_MOVIE
# define ENABLE_PMORE_ASCII_MOVIE_SYNTAX
#endif

/* Auto Merge時，配置快取的大小上限。超過就改用暫存檔 */
#define MERGECACHE_MAXLEN 1024


/* ----------------------------------------------------- */
/* list editor definitions 				 */
/* ----------------------------------------------------- */

#define IS_BIG5LEAD(ch) ((uschar)(ch) >= 0x81)
#define IS_BIG5TAIL(ch) ((uschar)(ch) >= 0x40)
#define ESC_CHR '\033'

#define	FN_BAK		"bak"		/* 要和 edit.c 中定義的相同 */
#define FN_MERGE	"auto.merge"
#define FN_EDITOR_HELP	"etc/help/edit.hlp"
#define FOOT_POSTFIX	"(^W)符號 (^X)檔案選單 (^Z)說明 %*s\033[m\n"
#define EDITOR_HELP_FOOT MFST_NORMAL	/* dust.101010: 極光第四回pmore適應性修正 */


/* DLVE key message */
#define VKFN_NONE	1
#define VKFN_NOCVM	2
#define VKFN_WHOLE	3
#define VKFN_SCROLL	4
#define VKFN_QUIT	5
#define VKFN_MASK	0xFF


/* for internal message */
#define VEMSGR_MASK 0x7FFFFFFF
#define VEX_KEYSORTED 0x80000000

#define VEM_OVR		BFLAG(0)
#define VEM_ARM		BFLAG(1)
#define VEM_DCD		BFLAG(2)
#define VEM_KCP		BFLAG(3)
#define VEM_RCFOOT	BFLAG(4)
#define VEM_IPTB	BFLAG(5)
#define VEM_PSR		BFLAG(6)

#define VES_OVERHARDLIM	BFLAG(12)
#define VES_NOTFOUND	BFLAG(13)
#define VES_BIFF	BFLAG(14)
#define VES_REDRAWALL	BFLAG(15)
#define VES_HEADLINE	BFLAG(16)
#define VES_TAILLINE	BFLAG(17)
#define VES_CURRLINE	BFLAG(18)
#define VES_TOBOTTOM	BFLAG(19)
#define VES_ALLLINE	BFLAG(20)
#define VES_RDRFOOT	BFLAG(21)
#define VES_VTMREFRESH	BFLAG(22)
#define VES_VTMPARTBRK	BFLAG(23)
#define VES_CVM		BFLAG(24)
#define VES_INPUTCHAIN	BFLAG(25)
#define VES_ARMREPOS	BFLAG(26)
#define VES_OVERLINELIM	BFLAG(27)
#define VES_OVERSIZELIM	BFLAG(28)
#define VES_SHUTDOWNOVR	BFLAG(29)
#define VES_LINELOCKED	BFLAG(30)
#define VES_RESERVED	BFLAG(31)	//do no use

#define VES_REDRAWTEXT	(VES_HEADLINE|VES_TAILLINE|VES_CURRLINE|VES_TOBOTTOM|VES_ALLLINE|VES_VTMREFRESH|VES_VTMPARTBRK)
#define VES_ALLWARNING	(VES_OVERHARDLIM|VES_NOTFOUND|VES_OVERLINELIM|VES_OVERSIZELIM|VES_SHUTDOWNOVR|VES_LINELOCKED)



/* for VTM system */
#define VTMCR (vterm->cur_row)
#define VTRST (vterm->row_st)
#define VTMAP(row) (vterm->row_st[row].col_map->map)
#define VTRLEN(row) (vterm->row_st[row].col_map->len)


/* ----------------------------------------------------- */
/* List Editor data type: Text Space			 */
/* ----------------------------------------------------- */

typedef struct TextSpace
{
  struct TextSpace *prev, *next;
  uschar len;
  uschar data[TEXTSPACEMAXLEN];
} txspce;


/* ----------------------------------------------------- */
/* List Editor data type: Line Base			 */
/* ----------------------------------------------------- */

typedef struct LineBase
{
  txspce *space;
  struct LineBase *prev, *next;
  unsigned tlen :24;
  unsigned locked :1;	/* this line cannot be modified */
} linebs;


/* ----------------------------------------------------- */
/* function key information structure			 */
/* ----------------------------------------------------- */

typedef struct VE_KeyFunction
{
  int key_value;
  int (*processor)(struct VE_KeyFunction*);
  unsigned csx_ret :1;		/* 需要做"游標回歸"(將超過該行長度的csx調整回去) */
  unsigned refocus :1;		/* 需要vx_fcs為有效 */
  unsigned chainput :1;		/* 輸入連鎖(重繪減量用) */
  unsigned cs_hrzmove :1;	/* 動作後游標的位置可能會變 */
  unsigned ansi_rfcs :1;	/* refocus的ANSI預覽模式版 */
  unsigned line_modify :1;	/* 這動作會變更內容 */
  unsigned iptb_stop :1;	/* 會中斷符號輸入工具列模式 */
} VEKFN;


/* ----------------------------------------------------- */
/* 虛擬終端機狀態管理系統 (VTM)				 */
/* ----------------------------------------------------- */

/* mapping coordinate x of screen to actual position in textline */
typedef struct
{
  int len;
  int map[0];
}VTM_CMAP;

/* state of each row */
typedef struct
{
  VTM_CMAP *col_map;
  txspce *space;	/* 開頭那格所在的text space */
  unsigned index :8;	/* 及其index */
  unsigned redraw :1;	/* 需要重繪 */
  unsigned dirty :1;	/* 需要更新本行的row state */
  unsigned h_indbc :1;	/* row state: 開頭是否在雙位字裡 */
  unsigned h_ansi :1;	/* row state: 開頭是否在ANSI色碼裡 */
  unsigned ftcmd_len :8;	/* row state: 開頭字元的ftcmd length有多長 */
  unsigned h_dcolor :2;	/* row state: 開頭是否在雙色字序列裡 */
  unsigned t_dbcld :1;	/* row state: 結尾是否在雙位字裡 */
  uschar inherit_attr;	/* row state: ANSI模式下本行結尾的顏色 */
}VTM_RST;

typedef struct
{
  int cur_row;	/* 現在在畫面上的哪一列(重繪用) */
  int sz_row, sz_col;	/* row_st[]與col_map的已配置大小 */
  VTM_RST row_st[0];
}VTM;


/* ----------------------------------------------------- */
/* Auto Merge System					 */
/* ----------------------------------------------------- */

#define AMS_CHECKMAXLEN 7

struct AutoMergeInfo
{
  off_t offset;	/* the offset of  */
  time_t old_mtime;	/* previous modified time */
  char *space;	/* context to merge or tmpfile path */
  int cc_len;	/* check code length */
  uschar check_code[AMS_CHECKMAXLEN];
  uschar use_tmpfile;
};


/* ----------------------------------------------------- */
/* Input Toolbar utility				 */
/* ----------------------------------------------------- */

/* symbols */
static const char * const iptb_list1[] = {
  "＋－×÷＝≠≒±∣∞√°",
  "﹢＜＞≦≧﹤﹥∵∴㏒㏑",
  "∩∪∫∮⊥∠∟⊿"};

static const char * const iptb_list2[] = {
  "↑↓←→↖↗↙↘㊣◎○●",
  "⊕⊙○●△▲☆★◇◆□■",
  "▽▼§￥〒￠￡※♀♂"};

static const char * const iptb_list3[] = {
  "～＠＃＄％︿＆＊？！、。",
  "ˊˇˋ…‥．‧˙﹒·‵′",
  "，﹐：﹕；﹔‘’“”〝〞",
  "〃℃℉㎜㎝㎞㎡㎎㎏㏄"};

static const char * const iptb_list4[] = {
  "「」『』〔〕《》（）【】",
  "｛｝﹛﹜﹝﹞〈〉﹙﹚",
  "﹁﹂﹃﹄︹︺︽︾︵︶︻︼",
  "︷︸︿﹀"};

static const char * const iptb_list5[] = {
  "﹣﹦≡–—╴￣｜∥︱︳",
  "﹉﹊﹍﹎﹋﹌﹏\︴",
  "╱╲／＼∕﹨╳"};

static const char * const iptb_list6[] = {
  "ⅠⅡⅢⅣⅤⅥⅦⅧⅨⅩ"};

static const char * const iptb_list7[] = {
  "ΑΒΓΔΕΖΗΘΙΚΛΜ",
  "ΝΞΟΠΡΣΤΥΦΧΨΩ",
  "α\βγδεζηθικλμ",
  "νξοπρστυφχψω"};

static const char * const iptb_list8[] = {
  "ㄅㄆㄇㄈㄉㄊㄋㄌㄍㄎㄏ",
  "ㄐㄑㄒㄓㄔㄕㄖㄗㄘㄙ",
  "ㄧㄨㄩㄚㄛㄜㄝㄞㄟㄠㄡ",
  "ㄢㄣㄤㄥㄦ"};

static const char * const iptb_list9[] = {
  "＿ˍ▁▂▃▄▅▆▇█◥◤",
  "▏▎▍▌▋▊▉█◢◣"};


/* Catagory */
struct CataInfo
{
  const char *name;
  int size;
  const char * const *list;
};

static const struct CataInfo iptb_catinfo[] = {
  "數學", 3, iptb_list1,
  "圖形", 3, iptb_list2,
  "一般符號", 4, iptb_list3,
  "括號", 4, iptb_list4,
  "線段", 3, iptb_list5,
  "羅馬數字", 1, iptb_list6,
  "希臘字母", 4, iptb_list7,
  "注音", 4, iptb_list8,
  "方塊", 2, iptb_list9};


/* table illustrator */
static const char * const iptb_tbdraw[8] = {
  "─│└┴┘├┼┤┌┬┐",
  "═║╚╩╝╠╬╣╔╦╗",
  "─║╙╨╜╟╫╢╓╥╖",
  "═│╘╧╛╞╪╡╒╤╕",
  "─│╰┴╯├┼┤╭┬╮",
  "═║╰╩╯╠╬╣╭╦╮",
  "─║╙╨╜╟╫╢╓╥╖",
  "═│╘╧╛╞╪╡╒╤╕"};

/* keys of Input Toolbar */
static const char * const iptb_keytable[2] = {"1234567890-=", "0.123456789"};


/* ----------------------------------------------------- */
/* List Editor Structure				 */
/* ----------------------------------------------------- */

typedef struct
{
  linebs *le__vl_init;
  linebs *le__vl_top;
  linebs *le__vl_cur;

  txspce *le__vx_fcs;
  int le__ve_index;

  int le__ve_lno;
  int le__ve_csx;

  int le__ve_row;
  int le__ve_col;

  int le__ve_pglen;

  int le__ve_linesum;
  int le__ve_textsum;
  int le__ve_extsum;

  char iptb_category;
  char iptb_page;
  char iptb_column;
  char iptb_len;

  char hunt[VESEARCH_MAXLEN + 1];
  const char *sg_big5ld;
  char *fpath;
  int ve_op;

} ListEditor;



/* ----------------------------------------------------- */
/* global variables					 */
/* ----------------------------------------------------- */

/* start point of content */
#define vl_init (dive->le__vl_init)

/* the top line of screen */
#define vl_top (dive->le__vl_top)

/* current line */
#define vl_cur (dive->le__vl_cur)

/* current text space (focus) */
#define vx_fcs (dive->le__vx_fcs)

/* current index on fcs */
#define ve_index (dive->le__ve_index)

/* current line number */
#define ve_lno (dive->le__ve_lno)

/* actual position of cursor in current line */
#define ve_csx (dive->le__ve_csx)

/* cursor position on screen */
#define ve_row (dive->le__ve_row)

/* cursor position on screen */
#define ve_col (dive->le__ve_col)

/* length of text region */
#define ve_pglen (dive->le__ve_pglen)

/* window width */
#define ve_width (b_cols - 5)

/* total lines */
#define ve_linesum (dive->le__ve_linesum)

/* total texts */
#define ve_textsum (dive->le__ve_textsum)

/* allocated extend text space */
#define ve_extsum (dive->le__ve_extsum)

/* DLVE internal message carrier */
static int ve_msgr;

static ListEditor *dive;

VTM *vterm;
struct AutoMergeInfo *amsys;

#ifdef DBG_MEMORY_MONITOR
static int allocated_space;
#endif



/* ----------------------------------------------------- */
/* text space operations				 */
/* ----------------------------------------------------- */

#define construct_txspce(_prev, _this, _next) { \
  _this->prev = _prev; \
  _this->next = _next; \
  _this->len = 0; \
}


static txspce*
newtsp(prev, next)
  txspce *prev, *next;
{
  txspce *this;

  if(ve_extsum >= MEM_LIMIT)	/* check memory hard limit */
    return NULL;

  this = (txspce*) malloc(sizeof(txspce));
  if(!this)
    out_of_memory();

  construct_txspce(prev, this, next);
  ve_extsum++;

#ifdef DBG_MEMORY_MONITOR
  allocated_space++;
#endif
  return this;
}


static void
freetsp(txspce *this)
{
  free(this);
  ve_extsum--;

#ifdef DBG_MEMORY_MONITOR
  allocated_space--;
#endif
}


#define vx_isfull(spc) ((spc)? (spc)->len >= TEXTSPACEMAXLEN : 1)


#define vx_iterate(i, spc){ \
  if(++i >= spc->len){ \
    spc = spc->next; \
    i = 0; \
  } \
}


static txspce*
vx_locate(line, pos)
  linebs *line;
  int *pos;
{
  txspce *spc = line->space;

  while(*pos >= spc->len)
  {
    if(!spc->next)
    {
      *pos = spc->len;
      break;
    }

    *pos -= spc->len;
    spc = spc->next;
  }

  return spc;
}


static void
vx_remove(node)
  txspce *node;
{
  txspce *prev = node->prev;
  txspce *next = node->next;

  if(prev) prev->next = next;
  if(next) next->prev = prev;

  freetsp(node);
}


static int
vx_strcmp(line, s)	/* simliar to strcmp() */
  linebs *line;
  const uschar *s;
{
  txspce *spc = line->space;
  int i = 0;

  while(*s && spc)
  {
    if(spc->data[i] != *s)
      return 1;

    vx_iterate(i, spc);
    s++;
  }

  return 0;
}



/* ----------------------------------------------------- */
/* line base operations					 */
/* ----------------------------------------------------- */

static linebs*
newlbs(prev, next)
  linebs *prev, *next;
{
  linebs *this;

  this = (linebs*) malloc(sizeof(linebs));
  if(!this)
    out_of_memory();

  this->space = (txspce*) malloc(sizeof(txspce));
  if(!this->space)
    out_of_memory();

  construct_txspce(NULL, this->space, NULL);
  this->prev = prev;
  this->next = next;
  this->tlen = 0;
  this->locked = 0;

  ve_linesum++;

#ifdef DBG_MEMORY_MONITOR
  allocated_space++;
#endif
  return this;
}


static void
freelbs(this)
  linebs *this;
{
  txspce *tmp, *curr;

  curr = this->space;
  while(curr)
  {
    tmp = curr;
    curr = curr->next;
    freetsp(tmp);
  }

  ve_extsum++;	/* 在迴圈中會多扣extsum一次。加回來。 */
  ve_linesum--;

  free(this);
}



/* ----------------------------------------------------- */
/* VTM operation					 */
/* ----------------------------------------------------- */

static void
vtm_init()	/* constructor */
{
  int i;

  vterm = (VTM*)malloc(sizeof(VTM) + sizeof(VTM_RST) * b_lines);
  if(!vterm)
    out_of_memory();

  vterm->sz_row = b_lines;
  vterm->sz_col = b_cols + 2;
  VTMCR = 0;

  for(i = 0; i < vterm->sz_row; i++)
    VTRST[i].col_map = (VTM_CMAP*)malloc(sizeof(VTM_CMAP) + sizeof(int) * vterm->sz_col);
}


static void
vtm_release()	/* destructor */
{
  int i, size;

  size = vterm->sz_row;
  for(i = 0; i < size; i++)
    free(VTRST[i].col_map);

  free(vterm);
}


static void
vtm_resize(old_len, old_wid)
  int old_len, old_wid;
{
  int i;

  if(old_wid != b_cols)
  {
    if(b_cols + 2 > vterm->sz_col)
    {
      vterm->sz_col = b_cols + 2;

      for(i = 0; i < vterm->sz_row; i++)
	VTRST[i].col_map = (VTM_CMAP*)realloc(VTRST[i].col_map, sizeof(VTM_CMAP) + sizeof(int) * vterm->sz_col);
    }
  }

  if(old_len != b_lines)
  {
    if(b_lines > vterm->sz_row)
    {
      vterm = (VTM*)realloc(vterm, sizeof(VTM) + sizeof(VTM_RST) * b_lines);

      i = vterm->sz_row;
      vterm->sz_row = b_lines;
      for(; i < vterm->sz_row; i++)
	VTRST[i].col_map = (VTM_CMAP*)malloc(sizeof(VTM_CMAP) + sizeof(int) * vterm->sz_col);
    }
  }
}


#define vtm_indbc(x) pfterm_indbc(x)


/* is DBC leading character? */
#define vtm_dbclead(x) (((x) < b_cols - 1)? pfterm_indbc((x) + 1) : VTRST[ve_row].t_dbcld)

/* the ARM version of vtm_dbclead() */
#define vtm_dbcld_ARM(x) pfterm_indbc((x) + 1)



/* ----------------------------------------------------- */
/* Auto Merge System Operation				 */
/* ----------------------------------------------------- */

static void
ams_init(fpath)
  char *fpath;
{
  FILE *fp;
  struct stat st;

  if(stat(fpath, &st) < 0 || !(fp = fopen(fpath, "rb")))
    return;

  amsys = (struct AutoMergeInfo*) malloc(sizeof(struct AutoMergeInfo));
  if(!amsys)
    out_of_memory();

  fseeko(fp, 0, SEEK_END);
  amsys->offset = ftello(fp);
  amsys->old_mtime = st.st_mtime;
  amsys->space = NULL;

  if(st.st_size < AMS_CHECKMAXLEN)
    amsys->cc_len = st.st_size;
  else
    amsys->cc_len = AMS_CHECKMAXLEN;

  fseeko(fp, -amsys->cc_len, SEEK_CUR);
  fread(amsys->check_code, 1, amsys->cc_len, fp);

  fclose(fp);
}


static void
ams_release()
{
  if(amsys)
    free(amsys);
}


static void
ams_read(fpath)
  char *fpath;
{
  struct stat st;
  uschar compare[AMS_CHECKMAXLEN];
  FILE *fp;

  if(stat(fpath, &st) < 0)
    return;

  if(amsys->old_mtime == st.st_mtime)	/* 檔案沒有修改過就不需要merge */
    return;

  if(!(fp = fopen(fpath, "rb")))
    return;

  fseeko(fp, amsys->offset - amsys->cc_len, SEEK_SET);
  fread(compare, 1, amsys->cc_len, fp);

  if(!memcmp(amsys->check_code, compare, AMS_CHECKMAXLEN))	/* 檢查碼match */
  {
    off_t len = st.st_size - amsys->offset;

    if(len > MERGECACHE_MAXLEN)
    {
      FILE *fpw;

      amsys->space = (char*)malloc(64 * sizeof(char));
      if(!amsys->space)
	out_of_memory();

      usr_fpath(amsys->space, cuser.userid, FN_MERGE);
      if(fpw = fopen(amsys->space, "wb"))
      {
	char buffer[512];
	int size;

	while((size = fread(buffer, 1, sizeof(buffer), fp)) > 0)
	{
	  fwrite(buffer, 1, size, fpw);
	}

	amsys->use_tmpfile = 1;
	fclose(fpw);
      }
      else
      {
	free(amsys->space);
	amsys->space = NULL;
      }
    }
    else
    {
      amsys->space = (char*)malloc(len + 1);
      if(!amsys->space)
	out_of_memory();

      fread(amsys->space, 1, len, fp);
      amsys->space[len] = '\0';
      amsys->use_tmpfile = 0;
    }
  }

  fclose(fp);
}


static void
ams_merge(fp)
  FILE *fp;
{
  if(amsys->space)
  {
    if(amsys->use_tmpfile)
    {
      f_suck(fp, amsys->space);
      unlink(amsys->space);
    }
    else
      fputs(amsys->space, fp);

    free(amsys->space);
#ifndef SILENT_MERGE
    vmsg("Auto Merge：新推文已合併。");
#endif
  }
}



/* ----------------------------------------------------- */
/* LE key function prototype & key information table	 */
/* ----------------------------------------------------- */

static int vkfn_isprint(VEKFN*);
static int vkfn_symouts(VEKFN*);
static int vkfn_enter(VEKFN*);
static int vkfn_left(VEKFN*);
static int vkfn_right(VEKFN*);
static int vkfn_up(VEKFN*);
static int vkfn_down(VEKFN*);
static int vkfn_del(VEKFN*);
static int vkfn_bksp(VEKFN*);
static int vkfn_tabk(VEKFN*);
static int vkfn_pgup(VEKFN*);
static int vkfn_pgdn(VEKFN*);
static int vkfn_home(VEKFN*);
static int vkfn_end(VEKFN*);
static int vkfn_ctrl_s(VEKFN*);
static int vkfn_ctrl_t(VEKFN*);
static int vkfn_ctrl_n(VEKFN*);
static int vkfn_ctrl_b(VEKFN*);
static int vkfn_ctrl_k(VEKFN*);
static int vkfn_ctrl_y(VEKFN*);
static int vkfn_ctrl_o(VEKFN*);
static int vkfn_ctrl_c(VEKFN*);
static int vkfn_ctrl_u(VEKFN*);
static int vkfn_ins(VEKFN*);
static int vkfn_ctrl_d(VEKFN*);
static int vkfn_ctrl_e(VEKFN*);
static int vkfn_ctrl_v(VEKFN*);
static int vkfn_ctrl_q(VEKFN*);
static int vkfn_ctrl_w(VEKFN*);
static int vkfn_ctrl_g(VEKFN*);
static int vkfn_ctrl_f(VEKFN*);
static int vkfn_ctrl_z(VEKFN*);
static int vkfn_ctrl_p(VEKFN*);
static int vkfn_ctrl_x(VEKFN*);



static VEKFN vekfn_isprint =
  {       0, vkfn_isprint, 1, 1, 1, 1, 1, 1, 1};

static VEKFN vekfn_symouts =
  {       0, vkfn_symouts, 1, 1, 0, 1, 1, 1, 0};

static VEKFN vekfn_table[] = {
  { KEY_LEFT,   vkfn_left, 1, 0, 0, 1, 0, 0, 0},
  {KEY_RIGHT,  vkfn_right, 1, 0, 0, 1, 0, 0, 0},
  {   KEY_UP,     vkfn_up, 0, 0, 0, 0, 0, 0, 0},
  { KEY_DOWN,   vkfn_down, 0, 0, 0, 0, 0, 0, 0},
  {KEY_ENTER,  vkfn_enter, 1, 0, 0, 1, 0, 0, 0},
  {  KEY_DEL,    vkfn_del, 1, 0, 0, 1, 0, 1, 0},
  { KEY_BKSP,   vkfn_bksp, 1, 0, 0, 1, 0, 1, 0},
  {  KEY_TAB,   vkfn_tabk, 1, 1, 1, 1, 1, 1, 0},
  { KEY_PGUP,   vkfn_pgup, 0, 0, 0, 0, 0, 0, 0},
  { KEY_PGDN,   vkfn_pgdn, 0, 0, 0, 0, 0, 0, 0},
  { KEY_HOME,   vkfn_home, 0, 0, 0, 1, 0, 0, 0},
  {  KEY_END,    vkfn_end, 0, 0, 0, 1, 0, 0, 0},
  {Ctrl('S'), vkfn_ctrl_s, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('T'), vkfn_ctrl_t, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('N'), vkfn_ctrl_n, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('B'), vkfn_ctrl_b, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('K'), vkfn_ctrl_k, 1, 1, 0, 1, 1, 1, 0},
  {Ctrl('Y'), vkfn_ctrl_y, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('O'), vkfn_ctrl_o, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('C'), vkfn_ctrl_c, 1, 1, 0, 1, 1, 1, 0},
  {Ctrl('U'), vkfn_ctrl_u, 1, 1, 1, 1, 1, 1, 1},
  {  KEY_INS,    vkfn_ins, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('D'), vkfn_ctrl_d, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('E'), vkfn_ctrl_e, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('V'), vkfn_ctrl_v, 1, 0, 0, 1, 0, 0, 0},
  {Ctrl('Q'), vkfn_ctrl_q, 1, 1, 0, 1, 1, 1, 0},
  {Ctrl('W'), vkfn_ctrl_w, 0, 0, 0, 0, 0, 0, 1},
  {Ctrl('G'), vkfn_ctrl_g, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('F'), vkfn_ctrl_f, 1, 0, 0, 1, 0, 0, 1},
  {Ctrl('Z'), vkfn_ctrl_z, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('P'), vkfn_ctrl_p, 0, 0, 0, 0, 0, 0, 0},
  {Ctrl('X'), vkfn_ctrl_x, 0, 0, 0, 0, 0, 0, 0},
};



/* ----------------------------------------------------- */
/* key information table operation (search & caved!)	 */
/* ----------------------------------------------------- */

static int vekfn_cmp(const void *a, const void *b){
  return *((const int *)a) - *((const int *)b);
}


static void
vekfn_init()
{
  /* 避免不必要的排序動作 */
  if(!(ve_msgr & VEX_KEYSORTED))
    qsort(vekfn_table, sizeof(vekfn_table) / sizeof(VEKFN), sizeof(VEKFN), vekfn_cmp);

  ve_msgr = VEX_KEYSORTED;
}


static VEKFN*
vekfn_findfunc(ch)
  int ch;
{
  if(isprint2(ch))
  {
    vekfn_isprint.key_value = ch;
    return &vekfn_isprint;
  }
  else
    return bsearch(&ch, vekfn_table, sizeof(vekfn_table) / sizeof(VEKFN), sizeof(VEKFN), vekfn_cmp);
}




/* ----------------------------------------------------- */
/* editing operations					 */
/* ----------------------------------------------------- */

/* set focus to invalid value */
#define INVALIDATE() vx_fcs = NULL

/* make focus valid */
#define REFOCUS(){ \
  if(!vx_fcs){ \
    ve_index = ve_csx; \
    vx_fcs = vx_locate(vl_cur, &ve_index); \
  } \
}


/* move cursor to next line */
#define line_forward(){ \
  INVALIDATE(); \
  vl_cur = vl_cur->next; \
  ve_row++; \
  ve_lno++; \
}

/* move cursor to previous line */
#define line_backward(){ \
  INVALIDATE(); \
  vl_cur = vl_cur->prev; \
  ve_row--; \
  ve_lno--; \
}



static int	/* 1:分割失敗(line-locked或overlimit) */
ve_split(line, pos)	/* 將line從(pos-1)與pos之間分成兩行 */
  linebs *line;
  int pos;
{
  txspce *break_point, *newline_head;
  linebs *next;
  int i, len;

  if(ve_linesum >= LINE_LIMIT)
  {
    ve_msgr |= VES_OVERLINELIM;
    return 1;
  }

  if(line->locked && pos < line->tlen && pos > 0)
  {
    ve_msgr |= VES_LINELOCKED;
    return 1;
  }

  next = line->next;
  line->next = newlbs(line, next);
  if(next) next->prev = line->next;

  if(pos < line->tlen)
  {
    i = pos;
    break_point = vx_locate(line, &i);
    newline_head = line->next->space;

    len = break_point->len - i;
    if(len > 0)
      memcpy(newline_head->data, break_point->data + i, len);

    newline_head->next = break_point->next;
    if(newline_head->next)
      newline_head->next->prev = newline_head;
    newline_head->len = len;

    break_point->next = NULL;
    break_point->len = i;
    if(break_point->len == 0 && break_point->prev)	/* 不可以移除該行的 head node */
      vx_remove(break_point);

    len = line->tlen;
    line->tlen = pos;
    line->next->tlen = len - line->tlen;
    if(line->locked)	/* locked line若能split，則由下一行繼承locked屬性 */
    {
      line->locked = 0;
      line->next->locked = 1;
    }
  }

  return 0;
}



static int	/* 1:到行尾了  0:還沒到底 */
skip_ansi_code()
{
  int ftcmd_len = 1, hit_bottom = 0;
  txspce *spc = vx_fcs;
  int idx = ve_index + 1, csx = ve_csx + 1;
  uschar ch;

  while(1)
  {
    if(idx >= spc->len)
    {
      if(spc->next)
      {
	spc = spc->next;
	idx = 0;
      }
      else
	break;
    }

    ch = spc->data[idx++];
    csx++;
    ftcmd_len++;

    if(ftcmd_len == 2)
    {
      if(ch != '[' && !PFTERM_STILL_IN_ANSI_SEQ(ch, ftcmd_len))
      {
	if(ch == '*')
	  return 0;	//本函數不跳過Esc-Star序列，所以直接return.

	break;
      }
    }
    else if(!PFTERM_STILL_IN_ANSI_SEQ(ch, ftcmd_len))
      break;
  }

  vx_fcs = spc;
  ve_index = idx;
  ve_csx = csx;

  if(csx >= vl_cur->tlen)
    hit_bottom = 1;
  else if(ve_index >= vx_fcs->len)
  {
    vx_fcs = vx_fcs->next;
    ve_index = 0;
  }

  return hit_bottom;
}



static int	/* 1:有換行(因為超過COL_LIMIT的緣故) */
add_char(ch)
  uschar ch;
{
  int increase_mode = 1;
  int multiline = 0;

  /* 游標在單行上限的位置時，生出新的一行來 */
  if(ve_csx >= COL_LIMIT)
  {
    if(ve_split(vl_cur, ve_csx))
      return 0;

    line_forward();
    vx_fcs = vl_cur->space;
    ve_index = ve_csx = 0;
    multiline = 1;
  }

  /* overwrite處理 */
  if(ve_msgr & VEM_OVR)
  {
    if(ve_csx >= vl_cur->tlen)
      ;
    else if(ve_msgr & VEM_ARM)
    {
      if(ch == ESC_CHR)
      {
	ve_msgr |= VES_SHUTDOWNOVR | VES_RDRFOOT;	/* 要改變OVR欄位，需要redraw foot */
	ve_msgr &= ~VEM_OVR;
      }
      else if(vx_fcs->data[ve_index] == ESC_CHR)
	increase_mode = skip_ansi_code();
      else
	increase_mode = 0;
    }
    else
      increase_mode = 0;
  }


  /* 進行插入/附加(也就是該行長度會增加的動作) */
  if(increase_mode)
  {
    int i, j, idx = ve_index;
    txspce *spc_i, *spc_j, *fcs = vx_fcs;

    /* overlimit check */
    if(ve_textsum >= TEXT_LIMIT)
    {
      ve_msgr |= VES_OVERSIZELIM;
      return multiline;
    }
    if(ve_linesum >= LINE_LIMIT && vl_cur->tlen >= COL_LIMIT)
    {
      ve_msgr |= VES_OVERLINELIM;
      return multiline;
    }

    /* 行末加字，檢查是否需要新的text space */
    if(idx >= TEXTSPACEMAXLEN && !fcs->next)
    {
      txspce *tmp = newtsp(fcs, NULL);
      if(!tmp)
      {
	ve_msgr |= VES_OVERHARDLIM;
	return multiline;
      }

      fcs = fcs->next = tmp;
      idx = 0;
    }

    if(vx_isfull(fcs))
    {
      if(vx_isfull(fcs->next))
      {
	txspce *next = fcs->next;

	spc_i = newtsp(fcs, next);
	if(!spc_i)
	{
	  ve_msgr |= VES_OVERHARDLIM;
	  return multiline;
	}

	fcs->next = spc_i;
	if(next) next->prev = spc_i;
      }
      else
	spc_i = fcs->next;

      spc_j = spc_i;
      j = spc_j->len;
      i = j - 1;

      spc_i->len++;

      for(; spc_j != fcs || j > idx; i--, j--)
      {
	if(i < 0)
	{
	  spc_i = spc_i->prev;
	  i = TEXTSPACEMAXLEN - 1;
	}

	if(j < 0)
	{
	  spc_j = spc_j->prev;
	  j = TEXTSPACEMAXLEN - 1;
	}

	spc_j->data[j] = spc_i->data[i];
      }
    }
    else
    {
      spc_i = fcs;
      if(spc_i->len > idx)
	memmove(spc_i->data + idx + 1, spc_i->data + idx, spc_i->len - idx);

      spc_i->len++;
    }

    vl_cur->tlen++;
    ve_textsum++;

    if(vl_cur->tlen > COL_LIMIT)
    {
      ve_split(vl_cur, COL_LIMIT);
      multiline = 1;
    }

    vx_fcs = fcs;
    ve_index = idx;
  }


  vx_fcs->data[ve_index] = ch;
  ve_csx++;
  ve_index++;

  /* adjust index */
  if(ve_index >= TEXTSPACEMAXLEN && vx_fcs->next)
  {
    vx_fcs = vx_fcs->next;
    ve_index = 0;
  }

  return multiline;
}



static void
delete_char(n)	/* 從游標所在處開始，刪除後面n個字元 */
  int n;
{
  int rest;

  vl_cur->tlen -= n;
  ve_textsum -= n;

  do
  {
    rest = vx_fcs->len - ve_index;

    if(rest > n)
    {
      vx_fcs->len -= n;
      memmove(&vx_fcs->data[ve_index], &vx_fcs->data[ve_index + n], rest - n);
    }
    else
      vx_fcs->len -= rest;

    if(vx_fcs->len == 0)	/* remove empty node */
    {
      txspce *spc = vx_fcs;
      txspce *next = spc->next;
      txspce *prev = spc->prev;

      if(next || prev)
      {
	if(!prev) vl_cur->space = next;

	if(next)
	{
	  vx_fcs = next;
	  ve_index = 0;
	}
	else	/* 游標在行末 */
	{
	  vx_fcs = prev;
	  ve_index = prev->len;
	}

	vx_remove(spc);
      }
    }
    else if(ve_index >= vx_fcs->len && vx_fcs->next)
    {
      vx_fcs = vx_fcs->next;
      ve_index = 0;
    }

    n -= rest;

  }while(n > 0);
}

static void
delete_char_ARM(n)	/* delete_char()的ARM版本，會跳過ANSI色碼 */
  int n;
{
  do
  {
    if(--vx_fcs->len > ve_index)
      memmove(vx_fcs->data + ve_index, vx_fcs->data + ve_index + 1, vx_fcs->len - ve_index);

    vl_cur->tlen--;
    ve_textsum--;

    if(vx_fcs->len == 0)	/* remove empty node */
    {
      txspce *spc = vx_fcs;
      txspce *next = spc->next;
      txspce *prev = spc->prev;

      if(next || prev)
      {
	if(!prev) vl_cur->space = next;

	if(next)
	{
	  vx_fcs = next;
	  ve_index = 0;
	}
	else	/* 游標在行末 */
	{
	  vx_fcs = prev;
	  ve_index = prev->len;
	}

	vx_remove(spc);
      }
    }
    else if(ve_index >= vx_fcs->len && vx_fcs->next)
    {
      vx_fcs = vx_fcs->next;
      ve_index = 0;
    }

    if(ve_index < vx_fcs->len && vx_fcs->data[ve_index] == ESC_CHR)
      skip_ansi_code();

  }while(--n > 0);
}



static void
cutlistoff(spc)		/* 移除掉spc後的所有node */
  txspce *spc;
{
  txspce *tmp = spc;

  spc = spc->next;
  tmp->next = NULL;
  while(spc)
  {
    tmp = spc;
    spc = spc->next;
    freetsp(tmp);
  }
}


static void
delete_line(line)	/* delete a line and maintain all global pointer */
  linebs *line;
{
  linebs *prev = line->prev;
  linebs *next = line->next;

  if(prev || next)
  {
    linebs *tmp = line;

    if(next)
      next->prev = prev;

    if(prev)
      prev->next = next;

    line = next? next : prev;
    if(tmp == vl_init)
      vl_init = line;
    if(tmp == vl_top)
      vl_top = line;

    if(tmp == vl_cur)
      vl_cur = next;	/* 牽涉到ve_row移動的問題，vl_cur不能修正為vl_cur->prev */

    freelbs(tmp);
  }
  else	/* there's only one line in editor. Do not free it */
  {
    cutlistoff(line->space);

    line->tlen = 0;
    line->space->len = 0;
    line->locked = 0;
  }
}


static int 	/* 1:長度超過COL_LIMIT，無法只結合為一行 */
ve_connect(line)	/* connect line with its next. the next line MUST exist */
  linebs *line;	/* the next line will be delete */
{
  linebs *next = line->next;
  txspce *ptail, *nhead;
  int rest, len, multiline = 0;

  if(line->tlen >= COL_LIMIT && next->tlen > 0)
    return 0;

  if(next->tlen > 0)
  {
    ptail = line->space;
    while(ptail->next)
      ptail = ptail->next;

    nhead = next->space;

    /* merge tail node of current with head node of next */
    rest = TEXTSPACEMAXLEN - ptail->len;

    if(nhead->len - rest > 0)
    {
      txspce *tmp = newtsp(ptail, NULL);
      if(!tmp)
      {
	ve_msgr |= VES_OVERHARDLIM;
	return 0;
      }

      memcpy(tmp->data, nhead->data + rest, nhead->len - rest);
      tmp->len = nhead->len - rest;
      ptail->next = tmp;
    }

    if(rest > 0)
    {
      len = (nhead->len < rest)? nhead->len : rest;
      memcpy(&ptail->data[ptail->len], nhead->data, len);
      ptail->len += len;
    }

    if(ptail->next)
      ptail = ptail->next;
    nhead = nhead->next;
    next->space->next = NULL;

    ptail->next = nhead;
    if(nhead)
      nhead->prev = ptail;

    line->tlen += next->tlen;

    /* 超過單行長度上限，無法只連結成一行 */
    if(line->tlen > COL_LIMIT)
    {
      /* 處理時會先增加一行(split)，之後才刪掉line。為了避免總行數超過限制而中斷split，在此先減一 */
      ve_linesum--;

      ve_split(line, COL_LIMIT);
      ve_linesum++;
      multiline = 1;
    }
  }

  /* 繼承next的locked屬性 */
  if(next->locked)
    line->locked = 1;

  delete_line(next);
  return multiline;
}



static void
add_tab()
{
  int n = TAB_STOP - ((ve_msgr & VEM_ARM)? ve_col : ve_csx) % TAB_STOP;

  for(; n > 0; n--)
  {
    if(vl_cur->tlen >= COL_LIMIT)
      break;

    add_char(' ');
  }
}



static int
add_str(s)
  uschar *s;
{
  int multi_line = 0;

  for(; *s; s++)
  {
    if(*s == '\t')
      add_tab();
    else if(*s == '\n')
    {
      if(ve_split(vl_cur, ve_csx))
	continue;

      line_forward();
      ve_index = ve_csx = 0;
      vx_fcs = vl_cur->space;
      multi_line = 1;
    }
    else if(isprint2(*s) || *s == ESC_CHR)
    {
      if(add_char(*s))
	multi_line = 1;
    }
  }

  return multi_line;
}



void
ve_sendrdrft()
{
  ve_msgr |= VES_RDRFOOT;
}



/* ----------------------------------------------------- */
/* display						 */
/* ----------------------------------------------------- */

#ifdef ENABLE_PMORE_ASCII_MOVIE_SYNTAX

#ifdef PMORE_USE_ASCII_MOVIE
/* pmore movie header support */
unsigned char *mf_movieFrameHeader(unsigned char *, unsigned char *);
#endif

static int
pmore_syntax_render(line, x)	/* 回傳現在游標所在處 */
  linebs *line;
  int x;
{
  txspce *spc;
  int idx = x, k = 0, width, ch;
  uschar attr[] = {15, 14, 13, 12, 11, 10, 9};

  idx = x;
  spc = vx_locate(line, &idx);

  width = b_cols - 1;
  if(width > line->tlen)
    width = line->tlen;

  while(x < width)
  {
    attrset(attr[k]);

    ch = spc->data[idx];
    if(ch == 'P' || ch == 'E')
    {
      outc(ch);
      x++;
    }
    else if(ch == 'S')
    {
      k = (k + 1) % 7;
      outc(ch);
      x++;
      vx_iterate(idx, spc);
      continue;
    }
    else if(isdigit(ch) || ch == '.')
    {
      do
      {
	outc(ch);
	x++;
	vx_iterate(idx, spc);
	if(x >= width) break;

	ch = spc->data[idx];

      }while(isdigit(ch) || ch == '.');
    }
    else if(ch == '#')
    {
      do
      {
	ch = spc->data[idx];
	outc(ch);

	if(ch == '#')
	  attrset(attr[k = (k + 1) % 7]);

	x++;
	vx_iterate(idx, spc);

      }while(x < width);
    }
    else if(ch == ':')
    {
      outc(':');
      vx_iterate(idx, spc);
      x++;
      while(x < width)
      {
	ch = spc->data[idx];
	if(!(isalnum(ch) || ch == ':'))
	  break;

	outc(ch);
	x++;
	vx_iterate(idx, spc);

	if(ch == ':') break;
      }

      k = (k + 1) % 7;
      continue;
    }
    else if(ch == 'I' || ch == 'G')
    {
      int prefix = 1;

      outc(ch);
      attrset(attr[k = (k + 1) % 7]);
      x++;
      vx_iterate(idx, spc);

      while(x < width)
      {
	ch = spc->data[idx];
	if(prefix)
	{
	  if (!strchr(":lpf", ch))
	    break;
	  prefix = 0;
	}
	else
	{
	  if(!(isalnum(ch) || strchr("+-,:", ch)))
	    break;
	}

	outc(ch);

	if(ch == ',')
	{
	  attrset(attr[k = (k + 1) % 7]);
	  prefix = 1;
	}

	x++;
	vx_iterate(idx, spc);
      }
    }
    else if(ch == 'K')
    {
      outc('K');
      x++;
      vx_iterate(idx, spc);

      if(!(spc && spc->data[idx] == '#'))
	break;

      outc('#');
      x++;
      vx_iterate(idx, spc);

      while(x < width)
      {
	ch = spc->data[idx];

	outc(ch);
	x++;
	vx_iterate(idx, spc);

	if(ch == '#') break;
      }

      k = (k + 1) % 7;
      continue;
    }

    break;
  }

  attrset(7);
  return x;
}
#endif



/* normal mode display */
static void
ve_outs(line, row, shift_window)
  linebs *line;
  int row;
  int shift_window;
{
  txspce *spc;
  uschar ch, dbclead_ch;
  int olen, begin_pos, rest, ftcmd_len, pattr, x = 0;
  int idx = 0, in_dbc = 0, in_ansi = 0, double_color = 0;
  int monotone, mono_esc;


  begin_pos = shift_window * ve_width;
  olen = line->tlen - begin_pos;

  /* nothing to output, just return */
  if(olen <= 0)
    return;

  monotone = cuser.ufo2 & UFO2_MONOTONE;
  mono_esc = cuser.ufo2 & UFO2_VEMONOESC;

  /* 把超過螢幕大小的olen調降 */
  rest = olen - (b_cols - 1);
  if(rest > 0)
    olen = b_cols - 1;

  spc = line->space;

  /* phase 1: 準備開頭的資訊，只有shift>0時才有需要 */
  if(begin_pos > 0)
  {
    /* VTM內的資訊需要更新 */
    if(VTRST[row].dirty || (ve_msgr & VES_VTMREFRESH))
    {
      int i, tmpDC = 0;
      VTRST[row].dirty = 0;

      for(i = 0; i < begin_pos; i++)
      {
	ch = spc->data[idx];

	if(double_color == 2)
	  tmpDC = 2;

	if(in_ansi)
	{
	  ftcmd_len++;
	  if(!(ANSI_IS_PARAM(ch) && ftcmd_len < FTCMD_MAXLEN) && !(ftcmd_len == 2 && ch == '['))
	  {
	    in_ansi = 0;

	    if(double_color == 1)
	      double_color = 2;
	  }
	}
	else if(ch == ESC_CHR)
	{
	  if(in_dbc) double_color = 1;
	  in_ansi = 1;
	  ftcmd_len = 1;
	}

	if(in_dbc)
	  in_dbc = 0;
	else if(IS_BIG5LEAD(ch) && double_color != 2)
	  in_dbc = 1;

	if(tmpDC == 2)
	  double_color = tmpDC = 0;

	vx_iterate(idx, spc);
      }

      VTRST[row].space = spc;
      VTRST[row].index = idx;
      VTRST[row].h_ansi = in_ansi;
      VTRST[row].h_dcolor = double_color;
      VTRST[row].ftcmd_len = ftcmd_len;
      VTRST[row].h_indbc = in_dbc;
    }
    else	/* VTM內的資訊沒有問題，直接使用 */
    {
      spc = VTRST[row].space;
      idx = VTRST[row].index;
      in_ansi = VTRST[row].h_ansi;
      ftcmd_len = VTRST[row].ftcmd_len;
      double_color = VTRST[row].h_dcolor;
      in_dbc = VTRST[row].h_indbc;
    }
  }
#ifdef ENABLE_PMORE_ASCII_MOVIE_SYNTAX
  else if(ve_msgr & VEM_PSR)
  {
# define HEADER_MAXLEN 15
    uschar buf[HEADER_MAXLEN + 1], *s;
    int size = 0, fake_idx = 0;
    txspce *fake_spc = line->space;

    while(size < HEADER_MAXLEN)
    {
      buf[size++] = spc->data[fake_idx];

      vx_iterate(fake_idx, fake_spc);
      if(!fake_spc) break;
    }

    if(s = mf_movieFrameHeader(buf, buf + size))
    {
      *s = '\0';
      x = s - buf;
      if(s = strchr(buf, ESC_CHR))	/* 替換掉ESC字元 */
	*s = '*';
      prints("\033[36m%s", buf);

      idx = x = pmore_syntax_render(line, x);
      spc = vx_locate(line, &idx);
    }
  }
#endif


  /* phase 2: 輸出 */
  pattr = -1;
  dbclead_ch = 0;

  while(x <= olen)	/* 本來是 x<olen，因為需要處理畫面右端的雙位字截斷，所以讓迴圈多跑一次 */
  {
    if(x == olen)
    {
      if(rest <= 0 || !in_dbc) break;

      ch = spc->data[idx];
      if(!IS_BIG5TAIL(ch)) break;
      rest--;
    }
    else
      ch = spc->data[idx];


    /* ch為雙色字後半字元 */
    if(double_color == 2)
    {
      double_color = 0;
      if(IS_BIG5TAIL(ch))
      {
	if(!monotone) pattr = 3;
	if(ch > 0x7F)	/* 大於ASCII範圍的tail用問號顯示 */
	  ch = '?';
      }
    }

    /* ch在ANSI色碼序列中 */
    if(in_ansi)
    {
      ftcmd_len++;
      if(!(ANSI_IS_PARAM(ch) && ftcmd_len < FTCMD_MAXLEN) && !(ftcmd_len == 2 && ch == '['))
      {
	in_ansi = 0;

	if(double_color == 1)
	  double_color = 2;
      }
    }
    else if(ch == ESC_CHR)
    {
      if(in_dbc) double_color = 1;
      in_ansi = 1;
      ftcmd_len = 1;
    }

    if(in_dbc)
    {
      if(x == 0)	/* 用空白取代掉雙位字的後半字元，避免開頭亂碼 */
      {
	if(IS_BIG5TAIL(ch))
	  ch = ' ';
      }
      else
      {
	if(IS_BIG5TAIL(ch))
	  outc(dbclead_ch);
	else
	  outs(dive->sg_big5ld);
      }

      dbclead_ch = in_dbc = 0;
    }
    else if(IS_BIG5LEAD(ch) && double_color != 2)
    {
      in_dbc = 1;
      dbclead_ch = ch;	/* 延遲輸出雙位字前半字元 */
      goto phase2_fake_continue;
    }

    if(ch == ESC_CHR)	/* 以星號代替ESC字元輸出 */
    {
      ch = '*';
      if(!mono_esc) pattr = 15;
    }

    if(pattr >= 0)
      attrset(pattr);

    outc(ch);

    if(pattr >= 0)
    {
      attrset(7);
      pattr = -1;
    }


phase2_fake_continue:
    x++;
    vx_iterate(idx, spc);
  }

  if(dbclead_ch)
    outs(dive->sg_big5ld);


  /* phase 3: 畫面右端DBCS狀態處理，只有shift=0時需要做 */
  if(begin_pos == 0)
  {
    if(x > olen || rest <= 1 || !IS_BIG5LEAD(spc->data[idx]))
      VTRST[row].t_dbcld = 0;
    else
    {
      vx_iterate(idx, spc);
      VTRST[row].t_dbcld = IS_BIG5TAIL(spc->data[idx]);
    }
  }

  attrset(7);
}



/* ARM display, no shift window */
static int	//1: inherit attr changed
ve_ansioutx(line, row)
  linebs *line;
  int row;
{
  txspce *spc;
  uschar ch, buf[64], queue[32];
  uschar esbuf[ESCODE_MAXLEN + 1];
  int qsize = 0, qhead = 0;
  int in_seq = 0, ftcmd_len = 0;
  int col = 0, csx = -1, *mapping = VTMAP(row);
  uschar last_attr;


  attrset(row > 0? VTRST[row-1].inherit_attr : 7);

  if(line->tlen > 0)
  {
    int idx = 0;
    spc = line->space;
    esbuf[0] = ESC_CHR;

    while(1)
    {
      /* 準備即將輸出的字元 */
      if(qsize > qhead)	/* queue is not empty, read from it */
      {
	ch = queue[qhead++];
	csx++;
      }
      else if(spc)
      {
	ch = spc->data[idx];
	vx_iterate(idx, spc);
	csx++;
      }
      else if(in_seq >= 2)
	goto flush_EScode;
      else
	break;

      /* 輸出處理 */
      if(ftcmd_len >= 2)
      {
	outc(ch);
	ftcmd_len++;

	if(!(ANSI_IS_PARAM(ch) && ftcmd_len < FTCMD_MAXLEN))
	  ftcmd_len = 0;
      }
      else if(in_seq == 1)
      {
	if(ch == '*')	/* is EScode */
	{
	  esbuf[in_seq++] = ch;
	  ftcmd_len = 0;
	}
	else
	{
	  in_seq = 0;
	  outc(ESC_CHR);
	  outc(ch);
	  if(ch == '[')
	    ftcmd_len = 2;
	  else
	    ftcmd_len = 0;
	}
      }
      else if(in_seq)
      {
	esbuf[in_seq++] = ch;

	/* esbuf滿了，輸出Esc-Star控碼 */
	if(in_seq >= sizeof(esbuf) - 1)
	{
	  int len, submsg;

flush_EScode:
	  esbuf[in_seq] = '\0';
	  len = EScode(buf, esbuf, sizeof(buf), &submsg);

	  csx -= in_seq - 1;

	  if(submsg)	/* 已知的控碼 */
	  {
	    int i, phylen;

	    /* 把esbuf中的剩餘字元丟入queue */
	    qsize = in_seq - len;
	    if(qsize > 0)
	      memcpy(queue, esbuf + len, qsize);
	    qhead = 0;

	    phylen = strlen(buf);
	    if(col + phylen > b_cols)
	      phylen = b_cols - col + 1;

	    /* 輸出變換後的字串 */
	    if(submsg < 0) attrset(10 + submsg);
	    for(i = 0; i < phylen; i++)
	    {
	      outc(buf[i]);
	      mapping[col++] = csx;
	    }
	    if(submsg < 0) attrset(7);

	    csx += len - 1;
	  }
	  else	/* 未知的控碼，扔回queue裡面重新輸出 */
	  {
	    qsize = in_seq - 1;
	    memcpy(queue, esbuf + 1, qsize);
	    qhead = 0;
	  }

	  in_seq = 0;
	}
      }
      else if(ch == ESC_CHR)
      {
	in_seq = 1;
	ftcmd_len = 1;
      }
      else
      {
	if(col > b_cols)
	  break;

	outc(ch);
	mapping[col++] = csx;
      }
    }
  }

  VTRLEN(row) = col;
  mapping[col] = csx + 1;

  pfterm_resetcmd();
  last_attr = VTRST[row].inherit_attr;
  VTRST[row].inherit_attr = attrget();
  attrset(7);

  if(last_attr != VTRST[row].inherit_attr)
    return 1;	/* 本行結尾的顏色發生變動，需要重繪後面的行 */
  else
    return 0;
}



static void
vertical_position()	/* 調整畫面位置到vl_cur剛好位於ve_row的地方 */
{
  int i;
  linebs *line = vl_cur;

  for(i = 0; i < ve_row; i++)
  {
    if(!line->prev)	/* 畫面到頂了。這時候改成調整ve_row */
    {
      ve_row = i;
      break;
    }
    line = line->prev;
  }

  if(line != vl_top)
  {
    vl_top = line;
    ve_msgr |= VES_ALLLINE | VES_VTMREFRESH;
  }
}



/* ve_load(): 將檔案載入到指定的行之後 */
static linebs*	/* 回傳插入區的最後一行 */
ve_load(line, fp, locked)
  linebs *line;	/* 若傳NULL進來代表加在vl_init上 */
  FILE *fp;
  int locked;	/* 載進來的是否都要設為鎖定 */
{
  int tlen = 0, idx = 0, in_tab = 0, newline = 0;
  uschar ch;
  linebs *head_line, *tail_line;
  txspce *fcs;

  /* overlimit check (前置階段) */
  if(ve_linesum >= LINE_LIMIT)
  {
    vmsg("已到達最大行數限制，無法載入檔案");
    return vl_cur;
  }
  if(ve_textsum >= TEXT_LIMIT)
  {
    vmsg("已達編輯器字數限制，無法載入檔案");
    return vl_cur;
  }

  tail_line = head_line = newlbs(line, NULL);
  head_line->locked = locked;
  fcs = head_line->space;

  while(1)
  {
    if(in_tab)
    {
      if(tlen % TAB_STOP != 0)
	ch = ' ';
      else
      {
	in_tab = 0;
	goto ReadInNewCharacter;
      }
    }
    else
    {
ReadInNewCharacter:
      if(fread(&ch, 1, 1, fp) <= 0)
	break;
    }

    if(ve_textsum >= TEXT_LIMIT)
    {
      vmsg("檔案大小超過編輯器字數限制，無法完全載入");
      break;
    }

    if(newline)
    {
InsertNewline:
      if(ve_linesum >= LINE_LIMIT)
      {
	vmsg("檔案超過編輯器行數上限，無法完全載入");
	break;
      }

      tail_line->tlen = tlen;
      tail_line->next = newlbs(tail_line, NULL);
      tail_line = tail_line->next;

      tail_line->locked = locked;
      fcs = tail_line->space;
      idx = tlen = 0;

      newline = 0;
    }

    if(ch == '\n')
      newline = 1;
    else
    {
      if(ch == '\t')
      {
	in_tab = 1;
	ch = ' ';
      }

      /* ban some character */
      if((ch < 0x20 || ch == 0x7f) && ch != ESC_CHR)
	continue;

      /* append new text space */
      if(idx >= TEXTSPACEMAXLEN)
      {
	txspce *tmp = newtsp(fcs, NULL);
	if(!tmp)
	{
	  vmsg("檔案大小超過編輯器記憶體限制，無法完全載入");
	  break;
	}

	fcs = fcs->next = tmp;
	idx = 0;
      }

      if(tlen >= COL_LIMIT)	/* over column limit, forced to change line */
      {
	in_tab = 0;
	goto InsertNewline;
      }

      fcs->data[idx++] = ch;
      fcs->len++;
      tlen++;
      ve_textsum++;
    }
  }

  tail_line->tlen = tlen;

  if(line)
  {
    linebs *lnxt = line->next;

    tail_line->next = lnxt;
    line->next = head_line;
    if(lnxt)
      lnxt->prev = tail_line;
  }
  else
    vl_init = head_line;

  return tail_line;
}



/* IPTB模式下的按鍵處理程序 */
static int	/* 回傳1代表是能由本routine捕捉到的按鍵 */
InputToolbar_routine(ch, pvkfn)
  int ch;
  VEKFN **pvkfn;
{
  char *s;
  int i;

  if(isprint2(ch))
  {
    if(ch == ' ')	/* 空白鍵不會中斷IPTB模式 */
      return 0;

    if(dive->iptb_category == -1)	/* table illustrator */
    {
      /* 輸出表格 */
      if(s = strchr(iptb_keytable[1], ch))
      {
	*pvkfn = &vekfn_symouts;
	dive->iptb_column = s - iptb_keytable[1];

	return 0;	/* 雖然有trap到，但是輸出還是得交給主程式的按鍵處理 */
      }

      switch(tolower(ch))
      {
      case 'd':
      case '/':
	if(dive->iptb_page >> 2)
	  dive->iptb_page -= 4;
	else
	  dive->iptb_page += 4;
	break;

      case 'f':
      case '-':
	if(dive->iptb_page >> 2)
	  dive->iptb_page = (dive->iptb_page + 1) % 4 + 4;
	else
	  dive->iptb_page = (dive->iptb_page + 1) % 4;
	break;

      case 't':
	dive->iptb_category = 0;
	break;

      default:
	goto cancel_IPTB_mode;
      }

      ve_msgr |= VES_RDRFOOT;
      return 1;
    }
    else if(dive->iptb_category > 0)	/* symbols */
    {
      /* 輸出符號 */
      if(s = strchr(iptb_keytable[0], ch))
      {
	i = s - iptb_keytable[0];
	if(i < dive->iptb_len)
	{
	  *pvkfn = &vekfn_symouts;
	  dive->iptb_column = i;
	}

	return 0;
      }

      switch(tolower(ch))
      {
      case 'w':
	dive->iptb_page += iptb_catinfo[dive->iptb_category - 1].size - 1;
	dive->iptb_page %= iptb_catinfo[dive->iptb_category - 1].size;
	dive->iptb_len = -1;
	break;

      case 's':
	dive->iptb_page++;
	dive->iptb_page %= iptb_catinfo[dive->iptb_category - 1].size;
	dive->iptb_len = -1;
	break;

      case 'a':
	if(--dive->iptb_category < 1) dive->iptb_category = 9;
	dive->iptb_page = 0;
	dive->iptb_len = -1;
	break;

      case 'd':
	if(++dive->iptb_category > 9) dive->iptb_category = 1;
	dive->iptb_page = 0;
	dive->iptb_len = -1;
	break;

      case 't':
	dive->iptb_category = 0;
	break;

      case 'q':
	dive->iptb_category = 0;
	dive->iptb_page = 0;
	break;

      default:
	goto cancel_IPTB_mode;
      }

      ve_msgr |= VES_RDRFOOT;
      return 1;
    }
    else	/* 主選單 */
    {
      i = tolower(ch);
      if(i >= '1' && i <= '9')
      {
	dive->iptb_category = i - '0';
	dive->iptb_page = 0;
	dive->iptb_len = -1;
      }
      else if(i == 't')
      {
	dive->iptb_category = -1;
	dive->iptb_page = 0;
	dive->iptb_len = -1;
      }
      else
	goto cancel_IPTB_mode;

      ve_msgr |= VES_RDRFOOT;
      return 1;
    }
  }
  else if(!(*pvkfn)->iptb_stop)
  {
    return 0;
  }

cancel_IPTB_mode:
  VTMCR = ve_pglen - 1;
  ve_pglen++;
  ve_msgr |= VES_RDRFOOT | VES_TOBOTTOM | VES_VTMPARTBRK;
  ve_msgr &= ~VEM_IPTB;

  return ch == Ctrl('W');
}



/* 將編輯器內的文字輸出到檔案(不釋放記憶體) */
static void
ve_dump(fp)
  FILE *fp;
{
  linebs *line;
  txspce *spc;
  uschar *data;
  int i, len;

  for(line = vl_init; line; line = line->next)
  {
    for(spc = line->space; spc; spc = spc->next)
    {
      len = spc->len;
      data = spc->data;

      for(i = 0; i < len; i++)
	fputc(data[i], fp);
    }

    fputc('\n', fp);
  }
}



/* 搜尋文字 */
static void
ve_search(hunt)
  uschar *hunt;
{
  int i, j, len, row, csx, lno, in_dbc;
  int fail[VESEARCH_MAXLEN];
  linebs *line;
  txspce *spc;

  /* prepare failure function */
  i = 0;
  j = fail[0] = -1;
  len = strlen(hunt);
  while(1)
  {
    while(j > -1 && hunt[i] != hunt[j])
      j = fail[j];

    if(++i >= len)
      break;
    j++;

    if(hunt[i] == hunt[j])
      fail[i] = fail[j];
    else
      fail[i] = j;
  }

  /* Let's search it! */
  row = ve_row;
  lno = ve_lno;
  line = vl_cur;
  j = (ve_msgr & VEM_ARM)? VTMAP(ve_row)[ve_col] : ve_csx;
  spc = vx_locate(line, &j);
  if(vtm_indbc(ve_col))
    in_dbc = 0;
  else if(IS_BIG5LEAD(spc->data[j]))
    in_dbc = 1;
  else
    in_dbc = 0;

  vx_iterate(j, spc);	/* 為了能找到下一個，搜尋都是從下一個字開始 */
  csx = ve_csx + 1;

  i = 0;
  while(1)
  {
    if(!spc)	/* 換到下一行 */
    {
      line = line->next;
      if(!line) break;
      spc = line->space;
      in_dbc = i = j = csx = 0;
      row++;
      lno++;
    }

    if(i > -1)
    {
      if(hunt[i] != spc->data[j])
      {
	do
	{
	  i = fail[i];
	}while(i > -1 && hunt[i] != spc->data[j]);
      }
      else if(i == 0 && in_dbc && IS_BIG5TAIL(spc->data[j]))
	goto search_fake_continue;
    }

    if(++i >= len)	/* 找到了 */
    {
      vl_cur = line;
      ve_csx = csx - len + 1;
      if(row >= ve_pglen || row < 0)	/* 超出畫面的話就需要重新定位 */
      {
	ve_row = ve_pglen / 2;
	vertical_position();
      }
      else
	ve_row = row;
      ve_lno = lno;

      INVALIDATE();
      ve_msgr |= VES_ARMREPOS;
      ve_msgr &= ~VES_NOTFOUND;
      return;
    }

search_fake_continue:

    if(in_dbc)
      in_dbc = 0;
    else if(IS_BIG5LEAD(spc->data[j]))
      in_dbc = 1;

    csx++;
    vx_iterate(j, spc);
  }

  ve_msgr |= VES_NOTFOUND;
}



/* 斷線編輯器自動備份 */
void
ve_backup()
{
  FILE *fp;
  char bakfile[64];

  if(!dive || !vl_init)
    return;

  usr_fpath(bakfile, cuser.userid, FN_BAK);

  if(fp = fopen(bakfile, "w"))
  {
    ve_dump(fp);
    fclose(fp);
  }

  //need free() here?
  //after ve_backup() ends, the process will be terminated
  //so let OS release allocated space
}



/* ----------------------------------------------------- */
/* 文章引用與簽名檔處理					 */
/* ----------------------------------------------------- */

static inline int
is_quotebreakmark(str)
  char *str;
{
  if(str[0] == '-' && str[1] == '-')	/* "--\n", "-- \n", "--", "-- " */
  {
    str += 2;
    if(*str == ' ') str++;
    if(*str == '\n') str++;

    return *str == '\0';
  }

  return 0;
}


static int	/* 0:不引用  1:引用 */
quote_type(str, qlimit)
  char *str;
  int qlimit;
{
  int qlevel = 0;

  while(*str == QUOTE_CHAR1 || *str == QUOTE_CHAR2)
  {
    str++;
    if(*str == ' ')
      str++;
    else
    {
      str--;
      break;
    }

    if(qlevel++ >= qlimit)
      return 0;
  }

  if(qlevel >= qlimit)
  {
    while (*str == ' ')
      str++;

    if (!memcmp(str, "※ ", 3))
      return 0;
  }

  if(*str == '\n')
    return 0;	/* 不引用空行 */

  return 1;
}


static int
quote_line(buf, szbuf, fp, quote_char)
  char *buf;
  int szbuf;
  FILE *fp;
  int quote_char;
{
  int len, total, overlimit = 0, linefeed = 0;

  if(quote_char)
  {
    add_char(quote_char);
    add_char(' ');
    total = 2;
  }
  else
    total = 0;

  while(1)
  {
    len = strlen(buf);
    if(buf[len - 1] == '\n')
    {
      buf[--len] = '\0';
      linefeed = 1;
    }

    if(total < COL_LIMIT)
    {
      total += len;
      if(total > COL_LIMIT)
      {
	buf[COL_LIMIT - total] = '\0';
	total = COL_LIMIT;
      }

      add_str(buf);
      if(ve_msgr & (VES_OVERLINELIM | VES_OVERSIZELIM))
      {
	overlimit = 1;
	break;
      }
    }

    if(linefeed || !fgets(buf, szbuf, fp))
      break;
  }

  return overlimit;
}



static int
ve_quote(line)
  linebs *line;
{
  int i, lno = 1, overlimit = 0;
  char buf[128];
  FILE *fp;
  linebs *backtrack = NULL;	/* for "回文時游標移動到引言結束點"功能 */


  if(line)
  {
    line->next = newlbs(line, NULL);
    line = line->next;
  }
  else
    vl_init = line = newlbs(NULL, NULL);

  vl_cur = line;
  vx_fcs = vl_cur->space;
  ve_csx = ve_index = 0;


  if(*quote_file)
  {
    char *str;
    int op = vans("是否引用原文 Y)引用 N)不引用 A)引用全文 R)重貼全文 1-9)引用層數？[Y] ");

    if(op != 'n')
    {
      if(fp = fopen(quote_file, "r"))
      {
	int quote_char = QUOTE_CHAR1;

	if(op >= '1' && op <= '9')
	  op -= '1';
	else if(op != 'a' && op != 'r')
	  op = 1;		/* default : 2 level */

	if(op != 'a')
	{
	  /* 去掉 header */
	  i = 0;
	  while(fgets(buf, sizeof(buf), fp) && i < 4)
	  {
	    if(*buf == '\n')
	      break;
	    i++;
	  }

	  if(curredit & EDIT_LIST)	/* 去掉 mail list 之 header */
	  {
	    while(fgets(buf, sizeof(buf), fp) && !memcmp(buf, "※ ", 3));
	  }

	  str = buf;
	  if(*quote_nick)
	    str += sprintf(buf, " (%s)", quote_nick);
	  else
	    *buf = '\0';

	  sprintf(++str, "※ 引述《%s%s》之銘言：", quote_user, buf);
	  add_str(str);
	  vl_cur->next = newlbs(vl_cur, NULL);
	  vl_cur = vl_cur->next;
	  vx_fcs = vl_cur->space;
	  ve_csx = ve_index = 0;
	  lno++;

	  backtrack = line;
	}

	if(op == 'r')
	{
	  backtrack = NULL;
	  op = 'a';
	  quote_char = '\0';
	}

	while(fgets(buf, sizeof(buf), fp))
	{
	  if(op == 'a')
	    i = 1;
	  else if(is_quotebreakmark(buf))
	    break;
	  else
	    i = quote_type(buf, op);

	  if(i)
	  {
	    if(quote_line(buf, sizeof(buf), fp, quote_char))
	    {
	      overlimit = 1;
	      break;
	    }

	    vl_cur->next = newlbs(vl_cur, NULL);
	    vl_cur = vl_cur->next;
	    vx_fcs = vl_cur->space;
	    ve_csx = ve_index = 0;
	    lno++;
	  }
	  else
	  {
	    do
	    {
	      if(buf[strlen(buf) - 1] == '\n')
		break;

	    }while(fgets(buf, sizeof(buf), fp));
	  }
	}

	fclose(fp);
      }
    }

    *quote_file = '\0';
  }

  if(backtrack)
    backtrack = vl_cur;
  else
  {
    backtrack = vl_init;
    lno = 1;
  }


  /* 載入簽名檔 */
  i = select_sign();
  if(!overlimit && i != '0')
  {
    char fpath[64];
    sprintf(buf, "%s.%c", FN_SIGN, i);
    usr_fpath(fpath, cuser.userid, buf);

    if(fp = fopen(fpath, "r"))
    {
      vl_cur->next = newlbs(vl_cur, NULL);
      vl_cur = vl_cur->next;
      vx_fcs = vl_cur->space;
      ve_csx = ve_index = 0;

      add_str("--");
      vl_cur->locked = 1;

      ve_load(vl_cur, fp, 1);

      fclose(fp);
    }
  }

  vl_cur = backtrack;
  return lno;
}



static int	/* 0:通過引用量審查 */
quote_check()
{
  linebs *vln;
  txspce *spc;
  int i, tlen;
  int post_line = 0, quot_line = 0;
  const char quote_sym[] = {QUOTE_CHAR1, ' ', '\0'};

  for(vln = vl_init; vln; vln = vln->next)
  {
    tlen = vln->tlen;
    if(tlen == 0) continue;

    if(vln->locked)			/* 維持傳統，簽名檔也算引言 */
    {
      quot_line++;
    }
    else if(!vx_strcmp(vln, quote_sym))	/* 引言 */
    {
      quot_line++;
    }
    else				/* 一般內文 (空白行不算post_line) */
    {
      spc = vln->space;
      i = 0;
      while(spc)
      {
	if(spc->data[i] != ' ')
	  break;
	tlen--;
	vx_iterate(i, spc);
      }

      if(tlen > 0)
	post_line++;
    }
  }

  if(post_line >= (quot_line >> 2))	/* 文章行數要多於引言行數四分之一 */
    return 0;

  if(HAS_PERM(PERM_ALLADMIN))
    return (vans("引言過多 (E)繼續編輯 (W)強制寫入？[E] ") != 'w');

  vmsg("引言太多，請按 Ctrl+Y 來刪除不必要之引言");
  return 1;
}



/* ----------------------------------------------------- */
/* 文章發表相關						 */
/* ----------------------------------------------------- */

static void
words_check()	/* 字數統計 */
{
  linebs *vln;
  const char quote_sym[] = {QUOTE_CHAR1, ' ', '\0'};
  extern int wordsnum;

  wordsnum = 0;

  for(vln = vl_init; vln; vln = vln->next)
  {
    /* locked line, 通常是簽名檔 */
    if(vln->locked)
      continue;

    /* 引言 */
    if(!vx_strcmp(vln, quote_sym) || !vx_strcmp(vln, "※ "))
      continue;

    wordsnum += vln->tlen;
  }
}



#ifdef HAVE_TEMPLATE
static linebs*
ve_template(tp_no)	/* 文章範本功能 */
  int tp_no;
{
  char fpath[64], tpName[32];
  FILE *fp;
  linebs *vln = NULL;

  sprintf(tpName, "template.%d", (tp_no+1)%10);
  brd_fpath(fpath, currboard, tpName);

  if(fp = fopen(fpath, "r"))
  {
    vln = ve_load(vl_init, fp, 0);
    fclose(fp);
  }

  return vln;
}
#endif



/* ----------------------------------------------------- */
/* 各個 list editor 按鍵的函數				 */
/* ----------------------------------------------------- */

static int
vkfn_isprint(pvkfn)
  VEKFN *pvkfn;
{
  VTMCR = ve_row;
  if(add_char(pvkfn->key_value))
  {
    ve_msgr |= VES_TOBOTTOM | VES_ARMREPOS;
    ve_msgr &= ~VES_INPUTCHAIN;
    return VKFN_WHOLE;
  }
  else
  {
    ve_msgr |= VES_CURRLINE | VES_ARMREPOS;
    return VKFN_NOCVM;
  }
}


static int
vkfn_symouts(pvkfn)
  VEKFN *pvkfn;
{
  uschar buf[4];
  const char *s;

  if(dive->iptb_category > 0)
    s = iptb_catinfo[dive->iptb_category - 1].list[dive->iptb_page] + dive->iptb_column * 2;
  else
    s = iptb_tbdraw[dive->iptb_page] + dive->iptb_column * 2;

  buf[0] = s[0];
  buf[1] = s[1];
  buf[2] = '\0';

  VTMCR = ve_row;
  if(add_str(buf))
  {
    ve_msgr |= VES_TOBOTTOM | VES_ARMREPOS;
    return VKFN_WHOLE;
  }
  else
  {
    ve_msgr |= VES_CURRLINE | VES_ARMREPOS;
    return VKFN_NOCVM;
  }
}


static int
vkfn_enter(pvkfn)
  VEKFN *pvkfn;
{
  if(ve_msgr & VEM_ARM)
  {
    if(ve_split(vl_cur, VTMAP(ve_row)[ve_col]))
      return VKFN_NOCVM;
    ve_col = 0;
  }
  else
  {
    if(ve_split(vl_cur, ve_csx))
      return VKFN_NOCVM;
    ve_csx = 0;
  }

  VTMCR = ve_row;
  ve_msgr |= VES_TOBOTTOM;
  line_forward();
  return VKFN_WHOLE;
}


static int
vkfn_left(pvkfn)
  VEKFN *pvkfn;
{
  if(ve_msgr & VEM_ARM)
  {
    INVALIDATE();
    if(ve_col > 0)
    {
      if((ve_msgr & VEM_DCD) && vtm_indbc(ve_col - 1))
	ve_col--;

      ve_col--;
      return VKFN_NOCVM;
    }
    else
    {
      if(!vl_cur->prev)
	return VKFN_NONE;

      line_backward();
      ve_col = b_cols;
      return VKFN_WHOLE;
    }
  }
  else
  {
    if(ve_csx > 0)
    {
      REFOCUS();

      if((ve_msgr & VEM_DCD) && vtm_indbc(ve_col - 1))
      {
	ve_csx--;
	ve_index--;
      }

      ve_csx--;
      if(--ve_index < 0)
      {
	vx_fcs = vx_fcs->prev;
	ve_index += vx_fcs->len;
      }

      return VKFN_NOCVM;
    }
    else
    {
      if(!vl_cur->prev)
	return VKFN_NONE;

      line_backward();
      ve_csx = vl_cur->tlen;
      return VKFN_WHOLE;
    }
  }
}


static int
vkfn_right(pvkfn)
  VEKFN *pvkfn;
{
  if(ve_msgr & VEM_ARM)
  {
    INVALIDATE();
    if(ve_col < VTRLEN(ve_row) && ve_col < b_cols)
    {
      if((ve_msgr & VEM_DCD) && vtm_dbcld_ARM(ve_col))
	ve_col++;

      ve_col++;
      return VKFN_NOCVM;
    }
    else
    {
      if(!vl_cur->next || VTRLEN(ve_row) > b_cols + 1)
	return VKFN_NONE;

      line_forward();
      ve_col = 0;
      return VKFN_WHOLE;
    }
  }
  else
  {
    if(ve_csx < vl_cur->tlen)
    {
      REFOCUS();

      if((ve_msgr & VEM_DCD) && vtm_dbclead(ve_col))
      {
	ve_csx++;
	ve_index++;
      }

      ve_csx++;
      if(++ve_index >= vx_fcs->len && vx_fcs->next)
      {
	ve_index -= vx_fcs->len;
	vx_fcs = vx_fcs->next;
      }

      return VKFN_NOCVM;
    }
    else
    {
      if(!vl_cur->next)
	return VKFN_NONE;

      line_forward();
      ve_csx = 0;
      return VKFN_WHOLE;
    }
  }
}


static int
vkfn_up(pvkfn)
  VEKFN *pvkfn;
{
  if(!vl_cur->prev)
    return VKFN_NONE;

  line_backward();
  ve_msgr |= VES_CVM;
  return VKFN_WHOLE;
}


static int
vkfn_down(pvkfn)
  VEKFN *pvkfn;
{
  if(!vl_cur->next)
    return VKFN_NONE;

  line_forward();
  ve_msgr |= VES_CVM;
  return VKFN_WHOLE;
}


static int
vkfn_del(pvkfn)
  VEKFN *pvkfn;
{
  int n = 1;

  if(ve_msgr & VEM_ARM)
  {
    if(ve_col < VTRLEN(ve_row))
    {
      if((ve_msgr & VEM_DCD) && vtm_dbcld_ARM(ve_col))
	n++;

      ve_index = ve_csx = VTMAP(ve_row)[ve_col];
      vx_fcs = vx_locate(vl_cur, &ve_index);
      delete_char_ARM(n);

      VTMCR = ve_row;
      ve_msgr |= VES_CURRLINE | VES_ARMREPOS;
    }
    else
    {
      if(!vl_cur->next)
	return VKFN_NONE;
      else if(vl_cur->next->locked && vl_cur->tlen > 0)
      {
	ve_msgr |= VES_LINELOCKED;
	return VKFN_NOCVM;
      }

      ve_connect(vl_cur);

      VTMCR = ve_row;
      ve_msgr |= VES_TOBOTTOM;
    }
  }
  else	//Normal Mode
  {
    if(ve_csx < vl_cur->tlen)
    {
      if((ve_msgr & VEM_DCD) && vtm_dbclead(ve_col))
	n++;

      REFOCUS();
      delete_char(n);

      VTMCR = ve_row;
      ve_msgr |= VES_CURRLINE;
    }
    else
    {
      if(!vl_cur->next)
	return VKFN_NONE;
      else if(vl_cur->next->locked && vl_cur->tlen > 0)
      {
	ve_msgr |= VES_LINELOCKED;
	return VKFN_NOCVM;
      }

      ve_connect(vl_cur);

      VTMCR = ve_row;
      ve_msgr |= VES_TOBOTTOM | VES_VTMPARTBRK;
    }
  }

  return VKFN_NOCVM;
}


static int
vkfn_bksp(pvkfn)
  VEKFN *pvkfn;
{
  int n = 1;

  if(ve_msgr & VEM_ARM)
  {
    if(ve_col > 0)
    {
      ve_col--;
      if((ve_msgr & VEM_DCD) && vtm_indbc(ve_col))
      {
	n++;
	ve_col--;
      }

      ve_index = ve_csx = VTMAP(ve_row)[ve_col];
      vx_fcs = vx_locate(vl_cur, &ve_index);
      delete_char_ARM(n);

      VTMCR = ve_row;
      ve_msgr |= VES_CURRLINE | VES_ARMREPOS;
    }
    else
    {
      if(!vl_cur->prev)
	return VKFN_NONE;
      else if(vl_cur->prev->locked && vl_cur->tlen > 0)
      {
	ve_msgr |= VES_LINELOCKED;
	return VKFN_NOCVM;
      }

      if(vl_top == vl_cur)
      {
	vl_top = vl_top->prev;
	ve_row++;
	ve_msgr |= VES_CURRLINE | VES_ARMREPOS;
      }
      else
	ve_msgr |= VES_TOBOTTOM | VES_ARMREPOS;

      line_backward();
      ve_csx = vl_cur->tlen;

      if(ve_connect(vl_cur))
	ve_msgr |= VES_TOBOTTOM;

      VTMCR = ve_row;
    }
  }
  else	//Normal Mode
  {
    if(ve_csx > 0)
    {
      if((ve_msgr & VEM_DCD) && vtm_indbc(ve_col - 1))
      {
	n++;
	ve_csx--;
	ve_index--;
      }

      REFOCUS();
      ve_csx--;
      if(--ve_index < 0)
      {
	vx_fcs = vx_fcs->prev;
	ve_index += vx_fcs->len;
      }

      delete_char(n);

      VTMCR = ve_row;
      ve_msgr |= VES_CURRLINE;
    }
    else
    {
      if(!vl_cur->prev)
	return VKFN_NONE;
      else if(vl_cur->prev->locked && vl_cur->tlen > 0)
      {
	ve_msgr |= VES_LINELOCKED;
	return VKFN_NOCVM;
      }

      if(vl_top == vl_cur)
      {
	vl_top = vl_top->prev;
	ve_row++;
	ve_msgr |= VES_CURRLINE;
      }
      else
	ve_msgr |= VES_TOBOTTOM;

      line_backward();
      ve_csx = vl_cur->tlen;

      if(ve_connect(vl_cur))
	ve_msgr |= VES_TOBOTTOM | VES_VTMPARTBRK;

      VTMCR = ve_row;
    }
  }
  return VKFN_NOCVM;
}


static int
vkfn_tabk(pvkfn)
  VEKFN *pvkfn;
{
  add_tab();

  VTMCR = ve_row;
  ve_msgr |= VES_CURRLINE | VES_ARMREPOS;
  return VKFN_NOCVM;
}


static int
vkfn_pgup(pvkfn)
  VEKFN *pvkfn;
{
  int i;

  if(!vl_cur->prev)
    return VKFN_NONE;

  INVALIDATE();
  for(i = 1; i < ve_pglen && vl_cur->prev; i++)
  {
    vl_cur = vl_cur->prev;
    ve_lno--;
  }

  vertical_position();
  ve_msgr |= VES_CVM;
  return VKFN_NOCVM;
}


static int
vkfn_pgdn(pvkfn)
  VEKFN *pvkfn;
{
  int i;

  if(!vl_cur->next)
    return VKFN_NONE;

  INVALIDATE();
  for(i = 1; i < ve_pglen && vl_cur->next; i++)
  {
    vl_cur = vl_cur->next;
    ve_lno++;
  }

  vertical_position();
  ve_msgr |= VES_CVM;
  return VKFN_NOCVM;
}


static int
vkfn_home(pvkfn)
  VEKFN *pvkfn;
{
  if(ve_msgr & VEM_ARM)
  {
    if(ve_col == 0 && vx_fcs && ve_csx == 0)
      return VKFN_NONE;

    ve_col = ve_index = ve_csx = 0;
    vx_fcs = vl_cur->space;
  }
  else
  {
    if(ve_csx == 0)
      return VKFN_NONE;

    ve_csx = 0;
    INVALIDATE();
  }

  return VKFN_NOCVM;
}


static int
vkfn_end(pvkfn)
  VEKFN *pvkfn;
{
  /* 為了讓大於tlen的ve_csx回歸, 在此使用 ==, 而不是 >= */
  if(ve_msgr & VEM_ARM)
  {
    if(ve_col == VTRLEN(ve_row))
      return VKFN_NONE;

    ve_col = VTRLEN(ve_row);
    if(ve_col > b_cols)
      ve_col = b_cols;
  }
  else
  {
    if(ve_csx == vl_cur->tlen)
      return VKFN_NONE;

    ve_csx = vl_cur->tlen;
  }

  INVALIDATE();
  return VKFN_NOCVM;
}


static int
vkfn_ctrl_s(pvkfn)
  VEKFN *pvkfn;
{
  if(!vl_cur->prev)
    return VKFN_NONE;

  INVALIDATE();

  if(vl_top != vl_init)
  {
    vl_top = vl_init;
    ve_msgr |= VES_ALLLINE | VES_VTMREFRESH;
  }

  vl_cur = vl_init;
  ve_row = 0;
  ve_lno = 1;
  ve_msgr |= VES_CVM;
  return VKFN_NOCVM;
}


static int
vkfn_ctrl_t(pvkfn)
  VEKFN *pvkfn;
{
  if(!vl_cur->next)
    return VKFN_NONE;

  INVALIDATE();

  do
  {
    vl_cur = vl_cur->next;
    ve_lno++;
    ve_row++;
  }while(vl_cur->next);

  if(ve_row >= ve_pglen)
  {
    ve_row = ve_pglen - 1;
    vertical_position();
  }

  ve_msgr |= VES_CVM;
  return VKFN_NOCVM;
}


static int
vkfn_ctrl_n(pvkfn)
  VEKFN *pvkfn;
{
  if(!vl_top->next)
    return VKFN_NONE;

  if(ve_row == 0)
  {
    line_forward();
    ve_msgr |= VES_CVM;
  }

  return VKFN_SCROLL | 0x100;
}


static int
vkfn_ctrl_b(pvkfn)
  VEKFN *pvkfn;
{
  if(!vl_top->prev)
    return VKFN_NONE;

  if(ve_row == ve_pglen - 1)
  {
    line_backward();
    ve_msgr |= VES_CVM;
  }

  return VKFN_SCROLL;
}


static int
vkfn_ctrl_k(pvkfn)
  VEKFN *pvkfn;
{
  if(ve_csx >= vl_cur->tlen)
    return VKFN_NONE;

  ve_textsum -= (vl_cur->tlen - ve_csx);
  vl_cur->tlen = ve_csx;
  if(ve_index > 0 || !vx_fcs->prev)
    vx_fcs->len = ve_index;
  else
  {
    vx_fcs = vx_fcs->prev;
    ve_index = vx_fcs->len;
  }

  cutlistoff(vx_fcs);

  if(vl_cur->locked && vl_cur->tlen == 0)
    vl_cur->locked = 0;

  VTMCR = ve_row;
  ve_msgr |= VES_CURRLINE;
  return VKFN_NOCVM;
}


static int
vkfn_ctrl_y(pvkfn)
  VEKFN *pvkfn;
{
  linebs *vln = vl_cur;

  ve_textsum -= vln->tlen;

  VTMCR = ve_row;
  if(vln->next)
    ve_msgr |= VES_TOBOTTOM;
  else
  {
    ve_msgr |= VES_CURRLINE;
    if(vln != vl_init)
    {
      line_backward();
      if(vln == vl_top)
	ve_row++;
    }
  }

  if(ve_msgr & VEM_ARM)
    ve_col = 0;
  else
    ve_csx = 0;

  INVALIDATE();
  delete_line(vln);

  ve_msgr |= VES_CVM;
  return VKFN_NOCVM;
}


static int
vkfn_ctrl_o(pvkfn)
  VEKFN *pvkfn;
{
  linebs *vln = vl_cur->next, *tmp;

  if(!vln)
    return VKFN_NONE;
  do
  {
    ve_textsum -= vln->tlen;

    tmp = vln;
    vln = vln->next;
    freelbs(tmp);
  }while(vln);

  vl_cur->next = NULL;

  VTMCR = ve_row;
  ve_msgr |= VES_TOBOTTOM;
  return VKFN_NOCVM;
}


static int
vkfn_ctrl_c(pvkfn)
  VEKFN *pvkfn;
{
#define CTRLC_PROMPT "請輸入 亮度/前景/背景  \033[1;33mB\033[41mR\033[42mG\033[43mY\033[44mL\033[45mP\033[46mC\033[47mW\033[m [0wb]："
  char cbuf[16] = "\033[m";
  int is_OVR;

  if(ve_msgr & VEM_ARM)
  {
    int n, seprator = 0;
    char abuf[4], *ans, *color, *s;
    const char const *table = "BRGYLPCW";

    if(vget(b_lines, 0, CTRLC_PROMPT, abuf, sizeof(abuf), DOECHO | FRCSAV))
    {
      color = cbuf + 2;
      ans = abuf;

      if(isdigit(*ans))
      {
	*color++ = *ans++;
	seprator = 1;
      }

      if(*ans)
      {
	if(*ans != ' ')
	{
	  if(seprator) *color++ = ';' ;

	  if(s = strchr(table, toupper(*ans)))
	    n = s - table;
	  else
	    n = 7;

	  color += sprintf(color, "3%c", n + '0');
	  seprator = 1;
	}

	ans++;
      }

      if(*ans)
      {
	if(seprator) *color++ = ';' ;

	if(s = strchr(table, toupper(*ans)))
	  n = s - table;
	else
	  n = 7;

	color += sprintf(color, "4%c", n + '0');
      }

      strcpy(color, "m");
    }

    ve_msgr |= VES_RDRFOOT;
  }

  VTMCR = ve_row;
  is_OVR = ve_msgr & VEM_OVR;
  ve_msgr &= ~VEM_OVR;

  if(add_str(cbuf))
  {
    ve_msgr |= VES_TOBOTTOM | VES_ARMREPOS | is_OVR;
    return VKFN_WHOLE;
  }
  else
  {
    ve_msgr |= VES_CURRLINE | VES_ARMREPOS | is_OVR;
    return VKFN_NOCVM;
  }
}


static int
vkfn_ctrl_u(pvkfn)
  VEKFN *pvkfn;
{
  VTMCR = ve_row;

  if(add_char(ESC_CHR))
  {
    ve_msgr |= VES_TOBOTTOM | VES_ARMREPOS;
    return VKFN_WHOLE;
  }
  else
  {
    ve_msgr |= VES_CURRLINE | VES_ARMREPOS;
    return VKFN_NOCVM;
  }
}


static int
vkfn_ins(pvkfn)
  VEKFN *pvkfn;
{
  ve_msgr ^= VEM_OVR;
  ve_msgr |= VES_RDRFOOT;
  return VKFN_NOCVM;
}


static int
vkfn_ctrl_d(pvkfn)
  VEKFN *pvkfn;
{
  ve_msgr ^= VEM_DCD;

  ve_msgr |= VES_RDRFOOT;
  return VKFN_NOCVM;
}


static int
vkfn_ctrl_e(pvkfn)
  VEKFN *pvkfn;
{
  ve_msgr ^= VEM_KCP;

  ve_msgr |= VES_RDRFOOT;
  return VKFN_NOCVM;
}


static int
vkfn_ctrl_v(pvkfn)
  VEKFN *pvkfn;
{
  ve_msgr ^= VEM_ARM;

  if(ve_msgr & VEM_ARM)
  {
    ve_msgr |= VES_REDRAWALL | VES_ARMREPOS;
    REFOCUS();
  }
  else
  {
    ve_msgr |= VES_REDRAWALL | VES_VTMREFRESH;
    if(!vx_fcs) ve_csx = VTMAP(ve_row)[ve_col];
  }

  return VKFN_NOCVM;
}


static int
vkfn_ctrl_q(pvkfn)
  VEKFN *pvkfn;
{
  char *menu[] = {"10",
    "1  ＩＤ      (**s)", "2  暱稱      (**n)",
    "3  上站次數  (**l)", "4  發文次數  (**p)",
    "5  金幣      (**g)", "6  銀幣      (**m)",
    "7  現在時刻  (**t)", "8  線上人數  (**u)",
    "9  生日      (**b)", "A  倒數計時  (**C)",
    "0  取消           ",
  NULL};

  char buf[8], esbuf[ESCODE_MAXLEN + 1], *escode;
  int cc, year, mon, day, warning, is_OVR;


  cc = pans((b_lines - 11) / 2 - 3, (b_cols - 37) / 2, "控制碼快速輸入工具", menu);

  if(cc == 'a')
  {
    ve_msgr |= VES_RDRFOOT;

    vget(b_lines, 0, "請輸入西元年：", buf, 5, DOECHO);
    year = atoi(buf);
    if(year <= 0)
      return VKFN_NOCVM;

    vget(b_lines, 22, "月：", buf, 3, DOECHO);
    mon = atoi(buf);
    if(mon <= 0 || mon > 12)
      return VKFN_NOCVM;

    vget(b_lines, 32, "日：", buf, 3, DOECHO);
    day = atoi(buf);
    if(day <= 0 || day > 31)
      return VKFN_NOCVM;

    vget(b_lines, 0, "請輸入警示天數(剩餘低於此天數將變色)，不輸入則不警示：", buf, 3, DOECHO);
    warning = abs(atoi(buf));

    if(vans("是否要在截止時顯示「已經截止」？ (若否則顯示0) (Y/N)[Y] ") == 'n')
    {
      strcpy(esbuf, "\033*Cx");
      escode = esbuf + 4;
    }
    else
    {
      strcpy(esbuf, "\033*C");
      escode = esbuf + 3;
    }

    escode += sprintf(escode, "%04d%02d%02d", abs(year), abs(mon), abs(day));

    if(warning > 0)
      sprintf(escode, "w%02d", warning);
  }
  else if(cc != '0')
    sprintf(esbuf, "\033*%c", "snlpgmtub"[cc - '1']);
  else
    return VKFN_NONE;


  is_OVR = ve_msgr & VEM_OVR;
  ve_msgr &= ~VEM_OVR;
  VTMCR = ve_row;

  if(add_str(esbuf))
  {
    ve_msgr |= VES_TOBOTTOM | VES_ARMREPOS | is_OVR;
    return VKFN_WHOLE;
  }
  else
  {
    ve_msgr |= VES_CURRLINE | VES_ARMREPOS | is_OVR;
    return VKFN_NOCVM;
  }
}


static int
vkfn_ctrl_w(pvkfn)
  VEKFN *pvkfn;
{
  ve_pglen--;
  ve_msgr |= VEM_IPTB | VES_RDRFOOT;
  return VKFN_WHOLE;
}


static int
vkfn_ctrl_g(pvkfn)
  VEKFN *pvkfn;
{
  int lno, direct;
  char buf[8];

  ve_msgr |= VES_RDRFOOT;

  if(!vget(b_lines, 0, "移到指定行號：", buf, 6, DOECHO))
    return VKFN_NOCVM;

  lno = atoi(buf);
  if(lno > ve_linesum)
    lno = ve_linesum;
  else if(lno < 1)
    lno = 1;

  direct = lno - ve_lno;
  if(direct > 0)
  {
    do
    {
      vl_cur = vl_cur->next;
      ve_lno++;
      ve_row++;
    }while(ve_lno != lno);
  }
  else if(direct < 0)
  {
    do
    {
      vl_cur = vl_cur->prev;
      ve_lno--;
      ve_row--;
    }while(ve_lno != lno);
  }
  else
    return VKFN_NOCVM;

  INVALIDATE();
  ve_col = ve_csx = 0;

  if(ve_row >= ve_pglen || ve_row < 0)
  {
    ve_row = ve_pglen / 2;
    vertical_position();
  }

  return VKFN_NOCVM;
}


static int
vkfn_ctrl_f(pvkfn)
  VEKFN *pvkfn;
{
  ve_msgr |= VES_RDRFOOT;
  if(!vget(b_lines, 0, "(向下)搜尋：", dive->hunt, VESEARCH_MAXLEN + 1, GCARRY))
    return VKFN_NOCVM;

  ve_search(dive->hunt);

  return VKFN_NOCVM;
}


static int
vkfn_ctrl_z(pvkfn)
  VEKFN *pvkfn;
{
  more(FN_EDITOR_HELP, EDITOR_HELP_FOOT);

  ve_msgr |= VES_REDRAWALL;
  return VKFN_NOCVM;
}


static int
vkfn_ctrl_p(pvkfn)
  VEKFN *pvkfn;
{
  screen_backup_t oldscr;
  int limit;
#ifdef DBG_MEMORY_MONITOR
  int bytes;
#endif

  scr_dump(&oldscr);
  clear();

  outs("\033[30;47mDynamic Line Volumn Editor (ver " DLVE_VER ") 編輯器資訊                              \033[m\n");
  outs("\033[1;30m▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔\033[m\n");
#ifndef DBG_MEMORY_MONITOR
  outc('\n');
#endif

  prints("行數上限：%8d\n\n", LINE_LIMIT);
  prints("單行上限：%8d\n\n", COL_LIMIT);
  prints("字數上限：%8d\n\n", TEXT_LIMIT);

  limit = LINE_LIMIT * sizeof(linebs) + (LINE_LIMIT + MEM_LIMIT) * sizeof(txspce);
  if(limit > 10485760)	//10MB
    prints("\n記憶體用量限制：%.1f MB\n", limit / 1048576.0);
  else
    prints("\n記憶體用量限制：%.1f KB\n", limit / 1024.0);

#ifdef DBG_MEMORY_MONITOR
  attrset(8);
  outs("\n\n<Memory Monitor>\n\n");

  outs("line sum =   ");
  prints("%8d / %8d\n\n", ve_linesum, LINE_LIMIT);

  outs("text sum =   ");
  prints("%8d / %8d\n\n", ve_textsum, TEXT_LIMIT);

  outs("ext-space =  ");
  prints("%8d / %8d\n\n", allocated_space - ve_linesum, MEM_LIMIT);

  outs("memory use = ");
  bytes = ve_linesum * sizeof(linebs) + allocated_space * sizeof(txspce);
  prints("%8d / %8d bytes  (%.2f%%)\n\n", bytes, limit, bytes * 100.0 / limit);

  attrset(7);
#endif

  vmsg(NULL);
  scr_restore(&oldscr);

  return VKFN_NONE;
}


static int
vkfn_ctrl_x(pvkfn)
  VEKFN *pvkfn;
{
  FILE *fp;
  linebs *vln, *tmp;
  txspce *spc;
  uschar *data;
  char buf[80], *tbf_name, *fpath;
  int i, len, ans, ignore_space, quoted_line;
  int ve_op = dive->ve_op;

  char **menu;
  char m3_def_op[] = "SE";

  /* menu1[]: 給不能儲存的狀況使用 */
  char *menu1[] = {"AE",
    "Abort    放棄", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔",
    NULL};

  /* menu2[]: 寄信時使用 */
  char *menu2[] = {"SE",
    "Save     存檔", "Abort    放棄", "Title    改標題", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔",
    NULL};

  /* menu3[]: 轉信板發文存檔時使用 */
  char *menu3[] = {m3_def_op,
    "Save     存為轉信檔", "Local    存為站內檔", "XRefuse  加密存檔", "Abort    放棄", "Title    改標題", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔",
    NULL};

  /* menu4[]: 發文而沒有轉信選項時使用 */
  char *menu4[] = {"SE",
    "Save     存檔", "XRefuse  加密存檔", "Abort    放棄", "Title    改標題", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔",
    NULL};

  /* menu5[]: 通用 */
  char *menu5[] = {"SE",
    "Save     存檔", "Abort    放棄", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔",
    NULL};


  if (ve_op < 0)
    menu = menu1;
  else if (ve_op == 0)
    menu = menu5;
  else if (bbsmode != M_POST)
    menu = menu2;
  else if (curredit & EDIT_OUTGO)	/* 轉信板發文 */
  {
    if((currbattr & BRD_LOCALSAVE) && !(curredit & EDIT_POSTREPLY))
      m3_def_op[0] = 'L';

    menu = menu3;
  }
  else
    menu = menu4;


  switch(pans((b_lines - 1) / 2 - 8, (b_cols - 37) / 2, "檔案選單", menu))
  {
  case 'l':
    curredit &= ~EDIT_OUTGO;

  case 's':
    ans = 0;
    break;

  case 'x':
    curredit |= EDIT_RESTRICT;
    curredit &= ~EDIT_OUTGO;	/* 加密必是 local save */
    ans = 0;
    break;

  case 'a':
    ans = -1;
    break;

  case 't':
    strcpy(buf, ve_title);
    if(!vget(b_lines, 0, "標題：", ve_title, TTLEN + 1, GCARRY))
      strcpy(ve_title, buf);

    ve_msgr |= VES_RDRFOOT;
    return VKFN_NOCVM;

  case 'r':
    tbf_name = tbf_ask();
    ve_msgr |= VES_RDRFOOT;

    if(tbf_name == NULL)
      return VKFN_NOCVM;

    usr_fpath(buf, cuser.userid, tbf_name);

    if(fp = fopen(buf, "rb"))
    {
      ve_load(vl_cur, fp, 0);
      VTMCR = ve_row;
      ve_msgr |= VES_TOBOTTOM | VES_VTMPARTBRK;
      fclose(fp);
    }
    return VKFN_NOCVM;

  case 'w':
    if(fp = tbf_open())
    {
      ve_dump(fp);
      fclose(fp);
    }

    ve_msgr |= VES_RDRFOOT;
    return VKFN_NOCVM;

  case 'd':
    tbf_name = tbf_ask();
    ve_msgr |= VES_RDRFOOT;

    if(tbf_name == NULL)
      return VKFN_NOCVM;

    usr_fpath(buf, cuser.userid, tbf_name);
    unlink(buf);
    return VKFN_NOCVM;

  default:
    return VKFN_NONE;
  }


  fpath = dive->fpath;

  if(ans >= 0)
  {
    if(ve_op == 1 && !(curredit & EDIT_MAIL) && quote_check())
    {
      ve_msgr |= VES_RDRFOOT;
      return VKFN_NOCVM;
    }

    /* 合併編輯時新增的推文(auto merge system ver 1.5) */
    if(amsys)
      ams_read(fpath);

    if (!*fpath)
      usr_fpath(fpath, cuser.userid, fn_note);

    if(!(fp = fopen(fpath, "w")))
    {
      sprintf(fpath, "寫入錯誤：檔案開啟失敗 (%d)", errno);
      vmsg(fpath);
      abort();
    }

    if (ve_op == 1)
    {
      words_check();
      ve_header(fp);
    }
  }


  vln = vl_init;
  vl_init = NULL;
  while(vln)
  {
    if(ans >= 0)	/* 輸出內容 */
    {
      ignore_space = 0;
      spc = vln->space;
      quoted_line = *spc->data == QUOTE_CHAR1;

      for(; spc; spc = spc->next)
      {
	data = spc->data;
	len = spc->len;
	for(i = 0; i < len; i++)
	{
	  if(data[i] == ' ')
	  {
	    if(quoted_line)
	    {
	      quoted_line = 0;
	      goto force_output;
	    }

	    ignore_space++;
	  }
	  else
	  {
	    while(ignore_space > 0)
	    {
	      fputc(' ', fp);
	      ignore_space--;
	    }
force_output:
	    fputc(data[i], fp);
	  }
	}
      }
      fputc('\n', fp);
    }

    tmp = vln;
    vln = vln->next;
    freelbs(tmp);
  }


  if (ans >= 0)
  {
    if (bbsmode == M_POST || bbsmode == M_SMAIL)
      ve_banner(fp, 0);

    if(amsys)
      ams_merge(fp);

    fclose(fp);
  }

  return VKFN_QUIT | (ans + 1) << 8;
}



/* ----------------------------------------------------- */
/* DLVE主程式						 */
/* ----------------------------------------------------- */

static void
load_config()
{
  if(cuser.ufo & UFO_ZHC)
    ve_msgr |= VEM_DCD;

  if(cuser.ufo & UFO_VEDIT)
    ve_msgr |= VEM_RCFOOT;

  if(cuser.ufo & UFO_VEKCP)
    ve_msgr |= VEM_KCP;

  if((cuser.ufo2 & UFO2_PSRENDER) && !(cuser.ufo2 & UFO2_MONOTONE))
    ve_msgr |= VEM_PSR;
}


#ifdef SAVE_CONFIG
static void
save_config()
{
  if(ve_msgr & VEM_DCD)
    cuser.ufo |= UFO_ZHC;
  else
    cuser.ufo &= ~UFO_ZHC;

  if(ve_msgr & VEM_KCP)
    cuser.ufo |= UFO_VEKCP;
  else
    cuser.ufo &= ~UFO_VEKCP;

  if(ve_msgr & VEM_RCFOOT)
    cuser.ufo |= UFO_VEDIT;
  else
    cuser.ufo &= ~UFO_VEDIT;
}
#endif


static void
print_warning(flag, msg)
  usint flag;
  const char *msg;
{
  move(b_lines, 23);

  outs(msg);

  if(!(ve_msgr & VEM_IPTB))
    outs(COLOR2 " (^X)");

  attrset(7);
  ve_msgr &= ~flag;
}




int
dledit(fpath, ve_op)	/* 參數、回傳值的意義和vedit()完全一樣 */
  char *fpath;
  int ve_op;
{
  linebs *vln = NULL;
  VEKFN *pvkfn;
  FILE *fp;
  int ch, i, pos;

  //決定游標垂直移動後在雙位字內，游標要往哪個方向修正
  //若沒有左右互換的設計，不斷垂直移動會導致游標越來越偏向某個方向
  int lineXXXward_shift = 0;

  int key_buf = 0;	/* 輸入連鎖用 */
  int shift_window = 0;
  int vertical_shift = 0;
  int no_CVMfix = cuser.ufo2 & UFO2_CVMNOFIX;
  int scr_length = b_lines, scr_width = b_cols;	/* 檢查螢幕大小變化用 */


  /* check if there's editor has been running */
  if(dive)
    return -1;

  /* initialize all global variable */
  dive = (ListEditor*) malloc(sizeof(ListEditor));
  if(!dive)
    out_of_memory();

  vl_init = NULL;
  ve_pglen = b_lines;
  ve_linesum = 0;
  ve_textsum = 0;
  ve_extsum = 0;
  amsys = NULL;

  dive->iptb_category = 0;
  dive->iptb_page = 0;
  dive->iptb_column = 0;
  dive->iptb_len = -1;
  dive->hunt[0] = '\0';
  if(cuser.ufo2 & UFO2_MONOTONE)
    dive->sg_big5ld = "?";
  else
    dive->sg_big5ld = "\033[1;33m?\033[m";

  vekfn_init();
  vtm_init();
  load_config();


  /* load/create file */
  if(*fpath)
  {
    fp = fopen(fpath, "rb");
    if (fp)
    {
      vln = ve_load(NULL, fp, 0);
      fclose(fp);
    }
    else
    {
      if((i = creat(fpath, 0600)) < 0)
      {
	/* fail to create, just return -1 */
	i = -1;
	goto DLVE_endpoint;
      }

      close(i);
    }
  }
#ifdef HAVE_TEMPLATE
  else if(curredit & EDIT_TEMPLATE)
  {
    vln = ve_template(ve_op);
    ve_op = 1;
    curredit &= ~EDIT_TEMPLATE;
  }
#endif

  if(ve_op > 0)	/* 處理引言 */
  {
    /* ve_quote()一開始的部份是在ve_linesum遠小於LINE_LIMIT的假設下運作 */
    /* 所以請避免呼叫時，ve_linesum已經很接近LINE_LIMIT的情況發生 */
    i = ve_quote(vln);
  }
  else if(curredit & EDIT_MODIFYPOST)	/* AutoMerge啟動 */
  {
    ams_init(fpath);
    curredit &= ~EDIT_MODIFYPOST;
  }


  if(!vl_init)
    vl_init = newlbs(NULL, NULL);

  if((cuser.ufo2 & UFO2_QUOTEMOVE) && ve_op > 0)
  {
    vl_top = vl_cur;
    ve_row = ve_pglen - 1;
    ve_lno = i;
    vertical_position();
  }
  else
  {
    vl_cur = vl_top = vl_init;
    ve_row = 0;
    ve_lno = 1;
  }
  vx_fcs = vl_init->space;
  ve_index = 0;
  ve_csx = 0;
  dive->fpath = fpath;
  dive->ve_op = ve_op;


  /* ----------------------------------- */
  /* 主迴圈：螢幕顯示、按鍵處理		 */
  /* ----------------------------------- */

  ve_msgr |= VES_REDRAWALL | VES_VTMREFRESH;

  for(;;)
  {
    if((ve_msgr & VES_INPUTCHAIN) && num_in_buf() > 0)
    {
      /* 輸入連鎖 */

      key_buf = vkey();
      pvkfn = vekfn_findfunc(key_buf);

      if(!pvkfn || !(pvkfn->chainput) )
	goto process_redraw;

      ch = key_buf;
      key_buf = 0;
    }
    else
    {
process_redraw:
      ve_msgr &= ~VES_INPUTCHAIN;

      /* horizontal positioning (in normal mode) */
      if(!(ve_msgr & VEM_ARM))
      {
	int csx, rest, new_sft;

	csx = ve_csx;
	if(csx > vl_cur->tlen)
	{
	  csx = vl_cur->tlen;
	  if(!(ve_msgr & VEM_KCP))
	    ve_csx = vl_cur->tlen;
	}

	new_sft = csx / ve_width;
	if(new_sft > 0)
	{
	  rest = 3;
	  if(new_sft == 1) rest += 2;

	  if(csx % ve_width < rest)
	    new_sft--;
	}

	if(shift_window != new_sft)
	{
	  shift_window = new_sft;
	  ve_msgr |= VES_ALLLINE | VES_VTMREFRESH;
	}
	pos = ve_col = csx - shift_window * ve_width;
      }


#ifdef EVERY_BIFF
      if(!(ve_msgr & VES_BIFF) && HAS_STATUS(STATUS_BIFF))
	ve_msgr |= VES_BIFF | VES_RDRFOOT;
#endif

      if(ve_msgr & VES_REDRAWALL)
      {
	ve_msgr |= VES_RDRFOOT | VES_ALLLINE;
	ve_msgr &= ~VES_REDRAWALL;
      }

      /* 重繪文字區 */
      if(ve_msgr & VES_REDRAWTEXT)
      {
	int conti_row = ve_pglen;
	int redraw_end = 0;

	/* 決定重繪的區域 */
	if(ve_msgr & VES_ALLLINE)
	{
	  conti_row = -1;
	  redraw_end = ve_pglen - 1;
	  clrregion(0, redraw_end);
	}
	else
	{
	  if(ve_msgr & VES_HEADLINE)
	  {
	    VTRST[0].redraw = 1;
	    clrregion(0, 0);
	  }

	  if(ve_msgr & VES_TOBOTTOM)
	  {
	    VTRST[VTMCR].redraw = 1;
	    conti_row = VTMCR;
	    redraw_end = ve_pglen - 1;
	    clrregion(conti_row, redraw_end);
	  }
	  else
	  {
	    if(ve_msgr & VES_CURRLINE)
	    {
	      VTRST[VTMCR].redraw = 1;
	      redraw_end = VTMCR;
	      clrregion(VTMCR, VTMCR);
	    }

	    if(ve_msgr & VES_TAILLINE)
	    {
	      VTRST[ve_pglen - 1].redraw = 1;
	      redraw_end = ve_pglen - 1;
	      clrregion(redraw_end, redraw_end);
	    }
	  }
	}

	/* 開始重繪 */
	vln = vl_top;
	for(i = 0; i <= redraw_end; i++)
	{
	  if(i > conti_row)
	  {
	    if(ve_msgr & VES_VTMPARTBRK)
	      VTRST[i].dirty = 1;

	    goto ProcessTextRedraw;
	  }

	  if(VTRST[i].redraw)
	  {
ProcessTextRedraw:
	    VTRST[i].redraw = 0;

	    move(i, 0);
	    if(vln)
	    {
	      if(ve_msgr & VEM_ARM)
	      {
		/* 若結尾顏色有變化，則繼續重繪下一行 */
		if(ve_ansioutx(vln, i) && redraw_end < ve_pglen - 1)
		{
		  VTRST[i+1].redraw = 1;
		  redraw_end++;
		}
	      }
	      else
		ve_outs(vln, i, shift_window);
	    }
	    else
	      outc('~');
	  }

	  if(vln) vln = vln->next;
	}

	ve_msgr &= ~VES_REDRAWTEXT;
      }


      /* horizontal positioning (in ANSI preview mode) */
      if(ve_msgr & VEM_ARM)
      {
	int len = VTRLEN(ve_row);

	if(ve_msgr & VES_ARMREPOS)
	{
	  int *map = VTMAP(ve_row);
	  for(i = 0; i < len; i++)
	  {
	    if(map[i] >= ve_csx)
	      break;
	  }
	  ve_col = i;
	  ve_msgr &= ~VES_ARMREPOS;
	}

	if(ve_col > b_cols + 1)
	  ve_col = b_cols + 1;

	if(ve_col > len)
	{
	  pos = len;
	  if(!(ve_msgr & VEM_KCP))
	    ve_col = len;
	}
	else
	  pos = ve_col;
      }


      /* 重繪狀態列 */
      if((ve_msgr & VES_RDRFOOT))
      {
	ve_msgr &= ~VES_RDRFOOT;

	move(b_lines, 0);
	if(ve_msgr & VEM_IPTB)
	{
	  outs(COLOR1);
	  if(dive->iptb_category > 0)
	    prints(" 第 %d/%d 頁 " COLOR2 "   ", dive->iptb_page + 1, iptb_catinfo[dive->iptb_category - 1].size);
	  else
	    outs(" 輸入工具 " COLOR2 "    ");

	  prints("│%s│%s│%s│%s│",
	    ve_msgr & VEM_OVR? "OVR" : "\033[30mOVR\033[37m",
	    ve_msgr & VEM_ARM? "ARM" : "\033[30mARM\033[37m",
	    ve_msgr & VEM_DCD? "DCD" : "\033[30mDCD\033[37m",
	    ve_msgr & VEM_KCP? "KCP" : "\033[30mKCP\033[37m");

	  if(dive->iptb_category == -1)
	  {
	    prints("    (/,d)更換角型  (-,f)更換格線  (t)返回 %*s\033[m\n", d_cols, "");

	    move(b_lines - 1, 0);
	    outs("\033[1;46m【表格繪製】   ");
	    for(i = 0; i < 11; i++)
	    {
	      prints("\033[33m%c\033[37m%.2s ", iptb_keytable[1][i], iptb_tbdraw[dive->iptb_page] + i * 2);
	      if(i % 3 == 1)
		outs("   ");
	    }
	    outs("       \033[m\n");
	  }
	  else if(dive->iptb_category > 0)
	  {
	    const char *s = iptb_catinfo[dive->iptb_category - 1].list[dive->iptb_page];
	    int space = (8 - strlen(iptb_catinfo[dive->iptb_category - 1].name)) / 2;

	    prints("   (w/s)翻頁 (a/d)換分類 (q)返回 (^W)關閉 %*s\033[m\n", d_cols, "");

	    move(b_lines - 1, 0);
	    prints("\033[1;46m%*s【%s】%*s   ", space, "", iptb_catinfo[dive->iptb_category - 1].name, space, "");

	    if(dive->iptb_len < 0) dive->iptb_len = strlen(s) / 2;

	    for(i = 0; i < dive->iptb_len; i++)
	      prints("\033[33m%c\033[37m %.2s ", iptb_keytable[0][i], s + i * 2);

	    for(; i < 12; i++)
	      outs("     ");

	    outs("   \033[m\n");
	  }
	  else
	  {
	    prints("          (t)表格繪製  (^W)關閉  (^Z)說明 %*s\033[m\n", d_cols, "");

	    move(b_lines - 1, 0);
	    outs("\033[1;46m  ");

	    for(i = 0; i < 9; i++)
	      prints("\033[33m%d.\033[37m%s ", i + 1, iptb_catinfo[i].name);

	    outs(" \033[m\n");
	  }
	}
	else
	{
	  outs(COLOR1);
#ifdef EVERY_BIFF
	  if(ve_msgr & VES_BIFF)
	    outs(" 郵差來了 ");
	  else
#endif
	    outs(" 編輯文章 ");

	  if(ve_msgr & VEM_RCFOOT)
	  {
	    prints(COLOR2 "     │%cOVR│%cARM│%cDCD│%cKCP│      ",
	      ve_msgr & VEM_OVR? '*' : ' ',
	      ve_msgr & VEM_ARM? '*' : ' ',
	      ve_msgr & VEM_DCD? '*' : ' ',
	      ve_msgr & VEM_KCP? '*' : ' ');

	    prints(FOOT_POSTFIX, d_cols, "");
	  }
	  else
	  {
	    if(ve_msgr & VEM_ARM)
	      i = pos + 1;
	    else
	      i = pos + shift_window * ve_width + 1;

	    prints(COLOR2 " %5d:", ve_lno);
	    prints(i < 1000? "%3d  " : "%-5d", i);

	    prints(" │%s│%s│%s│%s│  ",
	      ve_msgr & VEM_OVR? "OVR" : "\033[30mOVR\033[37m",
	      ve_msgr & VEM_ARM? "ARM" : "\033[30mARM\033[37m",
	      ve_msgr & VEM_DCD? "DCD" : "\033[30mDCD\033[37m",
	      ve_msgr & VEM_KCP? "KCP" : "\033[30mKCP\033[37m");

	    prints(FOOT_POSTFIX, d_cols, "");
	  }
	}
      }
	/* 更新游標現在所在的位置 */
      else if(!(ve_msgr & VEM_RCFOOT) && !(ve_msgr & VEM_IPTB))
      {
	if(ve_msgr & VEM_ARM)
	  i = pos + 1;
	else
	  i = pos + shift_window * ve_width + 1;

	move(b_lines, 10);
	prints(COLOR2 " %5d:", ve_lno);
	prints(i < 1000? "%3d  " : "%-5d", i);
	attrset(7);
      }


      /* 顯示警告 */
      if(ve_msgr & VES_ALLWARNING)
      {
	if(ve_msgr & VES_OVERLINELIM)
	  print_warning(VES_OVERLINELIM, "\033[1;41m 已到達最大行數限制，無法再增加行數 ");

	if(ve_msgr & VES_OVERSIZELIM)
	  print_warning(VES_OVERSIZELIM, "\033[1;41m 已達編輯器字數限制，無法再增加文字 ");

	if(ve_msgr & VES_SHUTDOWNOVR)
	  print_warning(VES_SHUTDOWNOVR, "\033[1;41m   無法加入色碼，強制解除覆寫模式   ");

	if(ve_msgr & VES_LINELOCKED)
	  print_warning(VES_LINELOCKED,  "\033[1;41m   本行為簽名檔，不能修改只能刪除   ");

	if(ve_msgr & VES_NOTFOUND)
	  print_warning(VES_NOTFOUND,    "\033[1;41m        找不到您想搜尋的字串        ");

	if(ve_msgr & VES_OVERHARDLIM)
	  print_warning(VES_OVERHARDLIM, "\033[1;41m      已達編輯器記憶體用量限制      ");
      }


      move(ve_row, pos);



      /* 中文雙位字位置修正 */
      if((ve_msgr & VES_CVM))
      {
	ve_msgr &= ~VES_CVM;

	if(!(ve_msgr & VEM_DCD) || no_CVMfix)
	  goto re_key;

	if(vtm_indbc(pos))
	{
	  int *x = (ve_msgr & VEM_ARM)? &ve_col : &ve_csx;

	  if(lineXXXward_shift)
	  {
	    (*x)--;
	    lineXXXward_shift = 0;
	  }
	  else
	  {
	    (*x)++;
	    lineXXXward_shift = 1;
	  }

	  /* need to redraw foot, so let's continue */
	  continue;
	}
	else
	  goto re_key;
      }
      else
      {
re_key:
	if(key_buf)
	{
	  ch = key_buf;
	  key_buf = 0;
	}
	else
	{
	  ch = vkey();
	}
      }

    }	/* if(VES_INPUTCHAIN) */


    /* 檢查螢幕尺寸是否變動 */
    if(scr_length != b_lines || scr_width != b_cols)
    {
      vtm_resize(scr_length, scr_width);

      scr_length = b_lines;
      scr_width = b_cols;
      if(ve_msgr & VEM_IPTB)
	ve_pglen = scr_length - 1;
      else
	ve_pglen = scr_length;

      ve_msgr |= VES_REDRAWALL | VES_VTMREFRESH;
      key_buf = ch;
      goto process_redraw;
    }


    /* ----------------- */
    /* 處理按鍵		 */
    /* ----------------- */

    pvkfn = vekfn_findfunc(ch);

    if(ve_msgr & VEM_IPTB)
    {
      if(InputToolbar_routine(ch, &pvkfn))
	continue;
    }

    if(!pvkfn)	/* 找不到按鍵時當成VKFN_NONE */
      goto re_key;


    /* 按鍵共通動作執行 */

    //游標回歸
    if(pvkfn->csx_ret)
    {
      if(ve_msgr & VEM_ARM)
      {
	if(ve_col > VTRLEN(ve_row))
	  ve_col = VTRLEN(ve_row);
      }
      else
      {
	if(ve_csx > vl_cur->tlen)
	  ve_csx = vl_cur->tlen;
      }
    }

    //locked line檢查
    if(vl_cur->locked && pvkfn->line_modify)
    {
      ve_msgr |= VES_LINELOCKED;
      continue;
    }

    //開始連鎖輸入
    if(pvkfn->chainput)
    {
      ve_msgr |= VES_INPUTCHAIN;
    }

    //重定fcs，分成ARM和一般模式
    if(ve_msgr & VEM_ARM)
    {
      if(pvkfn->ansi_rfcs && !vx_fcs)
      {
	ve_csx = ve_index = VTMAP(ve_row)[ve_col];
	vx_fcs = vx_locate(vl_cur, &ve_index);
      }
    }
    else
    {
      if(pvkfn->refocus)
	REFOCUS();
    }

    //游標水平移動後重設修正偏移方向
    if(pvkfn->cs_hrzmove)
    {
      lineXXXward_shift = 0;
    }


    /* 執行動作 */
    i = pvkfn->processor(pvkfn);

    switch(i & VKFN_MASK)
    {
    case VKFN_NONE:
      /* nothing happend, back to key-reading */
      goto re_key;

    case VKFN_NOCVM:
      /* no vertical movement, needn't do scroll-check */
      /* so just go to next loop */
      continue;

    case VKFN_WHOLE:
      /* need scroll-check, so let's continue rest parts of loop */
      break;

    case VKFN_QUIT:
      /* exit DLVE */
      i = (i >> 8) - 1;	/* get return value */
      goto DLVE_endpoint;

    case VKFN_SCROLL:
      /* forced scroll */
      vertical_shift = (i >> 8)? 1 : -1;
      goto process_scroll;
    }


    /* scroll-check (if ve_row is beyond screen) */
    if(ve_row >= ve_pglen)
      vertical_shift = 1;
    else if(ve_row < 0)
      vertical_shift = -1;


process_scroll:

    /* 捲動處理 */
    if(vertical_shift > 0)
    {
      VTM_CMAP *tmp = VTRST[0].col_map;
      memmove(VTRST, VTRST + 1, sizeof(VTM_RST) * (ve_pglen - 1));
      VTRST[ve_pglen - 1].col_map = tmp;
      ve_row--;
      vterm->cur_row--;
      scroll();
      vl_top = vl_top->next;
      VTRST[ve_pglen - 1].dirty = 1;
      ve_msgr |= VES_RDRFOOT | VES_TAILLINE;
      vertical_shift = 0;
    }
    else if(vertical_shift < 0)
    {
      VTM_CMAP *tmp = VTRST[ve_pglen - 1].col_map;
      memmove(VTRST + 1, VTRST, sizeof(VTM_RST) * (ve_pglen - 1));
      VTRST[0].col_map = tmp;
      ve_row++;
      vterm->cur_row++;
      rscroll();
      vl_top = vl_top->prev;
      VTRST[0].dirty = 1;

      ve_msgr |= VES_RDRFOOT | VES_HEADLINE;
      vertical_shift = 0;
    }

  }	/* for(;;) */

DLVE_endpoint:
  vtm_release();
  ams_release();
#ifdef SAVE_CONFIG
  save_config();
#endif

  free(dive);
  dive = NULL;

#ifdef DBG_MEMORY_MONITOR
  if(allocated_space > 0)
  {
    vmsg("warning: memory leak");
    allocated_space = 0;
  }
#endif

  return i;
}

