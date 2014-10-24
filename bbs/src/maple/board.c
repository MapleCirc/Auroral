/*-------------------------------------------------------*/
/* board.c	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : 看板、群組功能	 			 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;
extern XZ xz[];
extern char xo_pool[];


char brd_bits[MAXBOARD];



static char *class_img = NULL;
static XO board_xo;

static char *levelbar[6][2] = {
  {"\033[;33m＝\033[m", "\033[;33m＝"},
  {"\033[;33m≡\033[1m", "\033[;33m≡"},
  {"\033[;33m－\033[m", "\033[;33m－"},
  {"\033[;33m＝\033[1m", "\033[;33m＝"},
  {"\033[;33m  \033[m", "  "},
  {"\033[;33m＝\033[1;37m", "\033[;33m＝"}
};

char **brd_levelbar = levelbar[0];	/* dust:給新版vs_head()用的 */



int			/* >=1:第幾個板主  0:不是板主 */
is_bm(list, userid)
  char *list;		/* 板主：BM list */
  char *userid;
{
  return str_has(list, userid, strlen(userid));
}


int
bstamp2bno(stamp)	/* 找出對應某 board stamp 的 board number */
  time_t stamp;
{
  BRD *brd;
  int bno, max;

  bno = 0;
  brd = bshm->bcache;
  max = bshm->number;
  for (;;)
  {
    if (stamp == brd->bstamp)
      return bno;
    if (++bno >= max)
      return -1;
    brd++;
  }
}



static inline char
Ben_Perm(bno, ulevel)	/* 決定對各看板的權限 */
  int bno;
  usint ulevel;
{
  usint readlevel, battr;
  uschar bits;
  char *blist, *bname;
  BRD *brd;
  BPAL *bpal;
  int ftype;

#if 0	/* dust.090315:新版"看板好友設定與閱讀等級"說明 */
    ┌────┬────┬────┬────┐
    │        │一般看板│好友看板│秘密看板│
    ├────┼────┼────┼────┤
    │  預設  │  完整  │ 進不去 │ 看不見 │
    ├────┼────┼────┼────┤
    │看板好友│  完整  │  完整  │  完整  │
    ├────┼────┼────┼────┤
    │看板壞人│ 進不去 │ 進不去 │ 看不見 │
    └────┴────┴────┴────┘
#endif

#define FullPower    BRD_L_BIT | BRD_R_BIT | BRD_W_BIT | BRD_X_BIT | BRD_M_BIT
#define ExcuteLevel  BRD_L_BIT | BRD_R_BIT | BRD_W_BIT | BRD_X_BIT
#define NORMAL_BPERM BRD_L_BIT | BRD_R_BIT | BRD_W_BIT

  static uschar bit_data[9] =
  {            /* 預設,      看板好友,     看板壞人 */
/* 公開看板 */ NORMAL_BPERM, NORMAL_BPERM, BRD_L_BIT,
/* 好友看板 */ BRD_L_BIT,    NORMAL_BPERM, BRD_L_BIT,
/* 秘密看板 */ 0,            NORMAL_BPERM, 0,
  };

  brd = bshm->bcache + bno;
  bname = brd->brdname;
  if (!*bname)
    return 0;

  readlevel = brd->readlevel;
  battr = brd->battr;


  /* 通用的判定程序 */

  bpal = bshm->pcache + bno;
  ftype = is_bgood(bpal) ? 1 : is_bbad(bpal) ? 2 : 0;

  if (readlevel == PERM_SYSOP)		/* 秘密看板 */
    bits = bit_data[6 + ftype];
  else if (readlevel == PERM_BOARD)	/* 好友看板 */
    bits = bit_data[3 + ftype];
  else if (ftype)			/* 公開看板且在板好/板壞名單中 */
    bits = bit_data[ftype];
  else if (!readlevel || (readlevel & ulevel))	/* 公開板或自訂的閱讀權限 */
  {
    bits = BRD_L_BIT | BRD_R_BIT;

    if (!brd->postlevel || (brd->postlevel & ulevel))
      bits |= BRD_W_BIT;
  }
  else
  {
    bits = 0;
  }


  /* 管理階層的判定程序 */

  blist = brd->BM;
  /* floatj.091020: 板助也要有板主權限，以 is_bv 判定是否在板助名單內 */
  if(is_bv(bpal) || (ulevel & PERM_BM) && blist[0] > ' ' && is_bm(blist, cuser.userid))
  {
    bits = FullPower;			/* dust.090315:板主有該板的所有權限 */
  }
  /* dust.090315: 一般站務在站務板有所有權限，但是 junk, delete 和 UnAnonymous (注意大小寫) 板例外 */
  else if(ulevel & PERM_ALLADMIN)
  {
    if(!strcmp(brd->class, "站務") && strcmp(bname, "junk") && strcmp(bname, "deleted") && strcmp(bname, "UnAnonymous"))
      bits |= FullPower;

    /* dust.090315:看板總管重新判斷。雖然已經廢掉了，但考慮到相容性還是加進來 */
    if(ulevel & PERM_ALLBOARD)
    {
      if (!readlevel && !(battr & BRD_PERSONAL))	/* dust.090315:公開的公共板可管 */
	bits |= ExcuteLevel;
      else
	bits |= BRD_L_BIT;		/* dust.090315:看板總管必可list */
    }
  }

#ifdef HAVE_MODERATED_BOARD
  if (is_bwater(bpal))		/* dust.090315:判斷是否為水桶 */
    bits &= ~(BRD_W_BIT);
#endif

  return bits;
}



void
mantime_add(outbno, inbno)
  int outbno;
  int inbno;
{
  /* itoc.050613.註解: 人氣的減少不是在離開看板時，而是在進入新的看板或是離站時，
     這是為了避免 switch 跳看板會算錯人氣 */
  if (outbno >= 0)
    bshm->mantime[outbno]--;		/* 退出上一個板 */
  if (inbno >= 0)
    bshm->mantime[inbno]++;		/* 進入新的板 */
}


static void
brd_usies()
{
  char fpath[64], buf[256];

  brd_fpath(fpath, currboard, "usies");
  if(HAS_PERM(PERM_SpSYSOP))
    sprintf(buf, "%s %s(%s)\n", Now(), str_sysop, SysopLoginID_query(NULL));
  else
    sprintf(buf, "%s %s\n", Now(), cuser.userid);
  f_cat(fpath, buf);
}


int
XoPost(bno)
  int bno;
{
  XO *xo;
  BRD *brd;
  BPAL *bpal;
  int bits;
  char *str, fpath[64];
  int brd_changed = 0, shownote = 1;

  brd = bshm->bcache + bno;
  if (!brd->brdname[0]) /* 已刪除的看板 */
    return -1;

  bpal = bshm->pcache + bno;
  bits = brd_bits[bno];

  if (currbno != bno)   /* 看板沒換通常是因為 every_Z() 回原看板 */
  {
    brd_changed = 1;
#ifdef HAVE_MODERATED_BOARD
    /* dust.090315:為sysop的場合，重新判斷(sysop擁有原本M3設計的權力) */
    if(HAS_PERM(PERM_SpSYSOP))
    {
      if(!(bits & BRD_R_BIT))
	slog("進入看板", brd->brdname);
      bits |= BRD_L_BIT | BRD_R_BIT | BRD_W_BIT | BRD_X_BIT;
    }

    if (!(bits & BRD_R_BIT))
    {
      vmsg("對不起，此板只准板友進入，請向板主申請入境許\可");
      return -1;
    }
#endif

    /* 處理權限 */
    bbstate = STAT_STARTED;
    if (bits & BRD_M_BIT)
      bbstate |= (STAT_BM | STAT_BOARD);
    else if (bits & BRD_X_BIT)
      bbstate |= STAT_BOARD;

    if (bits & BRD_W_BIT)
      bbstate |= STAT_POST;


    mantime_add(currbno, bno);

    currbno = bno;
    currbattr = brd->battr;

    if (!strcmp(brd->brdname, BN_SECURITY))
      currbattr |= BRD_LOG;
    else
      currbattr &= ~(BRD_LOG);

    if(bits & BRD_X_BIT)		/* 有管理權限(X以上) */
      brd_levelbar = levelbar[1];
    else if(!(bits & BRD_W_BIT))	/* 無法發言(水桶或未認證) */
      brd_levelbar = levelbar[2];
    else if(is_bgood(bpal))		/* 看板好友 */
      brd_levelbar = levelbar[3];
    else if(is_bbad(bpal))		/* 看板壞人 */
      brd_levelbar = levelbar[4];
    else				/* 普通 */
      brd_levelbar = levelbar[5];

    strcpy(currboard, brd->brdname);
    str = brd->BM;
    sprintf(currBM, "板主:%s", *str <= ' ' ? "(NULL)" : str);
#ifdef HAVE_BRDMATE
    strcpy(cutmp->reading, currboard);
#endif

    brd_fpath(fpath, currboard, fn_dir);

    xz[XZ_POST - XO_ZONE].xo = xo = xo_get_post(fpath, brd);
  /* itoc.010910: 為 post 量身打造一支 xo_get() */
    xo->key = XZ_POST;
    xo->xyz = brd->title;
    brd_usies();
  }

  if(cuser.ufo & UFO_BRDNOTE)
  {
    if((cuser.ufo2 & UFO2_BRDNOTE) && !brd_changed)
      shownote = 0;
  }
  else if(!(cuser.ufo2 & UFO2_BRDNOTE) && !(bits & BRD_V_BIT))
    goto add_visit_bit;
  else
    shownote = 0;

  if(!(bits & BRD_V_BIT))
add_visit_bit:
    brd_bits[bno] = bits | BRD_V_BIT;

  if(shownote)
  {
    brd_fpath(fpath, currboard, fn_note);
    more(fpath, MFST_NULL);
  }

  rhs_get(brd->bstamp, bno);

  return 0;
}


#ifdef HAVE_FORCE_BOARD
void
brd_force()			/* itoc.010407: 強制閱讀公告板，且強迫讀最後一篇 */
{
  if (cuser.userlevel)		/* guest 跳過 */
  {
    int bno;
    BRD *brd;
    BST *bst;

    if ((bno = brd_bno(BN_ANNOUNCE)) < 0)
      return;

    brd = bshm->bcache + bno;
    bst = bshm->bstate + bno;
    if (brd->btime < 0)		/* 尚未更新 blast 就不強制閱讀公告板 */
      return;

    while (rhs_unread(brd->bstamp, bst->blast[0], 0))
    {
      vmsg("抱歉，本站有新公告噢，請先閱\讀完再離開 :)");
      XoPost(bno);
      xover(XZ_POST);
    }
  }
}
#endif	/* HAVE_FORCE_BOARD */


/* ----------------------------------------------------- */
/* Class [分類群組]					 */
/* ----------------------------------------------------- */


#ifdef MY_FAVORITE
int class_flag = 0;			/* favorite.c 要用 */
#else
static int class_flag = 0;
#endif


#ifdef AUTO_JUMPBRD
static int class_jumpnext = 0;	/* itoc.010910: 是否跳去下一個未讀板 1:要 0:不要 */
#endif


static int class_flag2 = 0;  /* 1:列出好友/秘密板，且自己又有閱讀權限的 */

static int
class_load(xo)
  XO *xo;
{
  short *cbase, *chead, *ctail;
  int chn;			/* ClassHeader number */
  int pos, max, val;
  BRD *brd;
  char *bits;

  /* lkchu.990106: 防止 未用 account 造出 class.img 或沒有 class 的情況 */
  if (!class_img)
    return 0;

  chn = CH_END - xo->key;

  cbase = (short *) class_img;
  chead = cbase + chn;

  pos = chead[0] + CH_TTLEN;
  max = chead[1];

  chead = (short *) ((char *) cbase + pos);
  ctail = (short *) ((char *) cbase + max);

  max -= pos;

  if (cbase = (short *) xo->xyz)
    cbase = (short *) realloc(cbase, max);
  else
    cbase = (short *) malloc(max);

  xo->xyz = (char *) cbase;

  max = 0;
  brd = bshm->bcache;
  bits = brd_bits;

  do
  {
    chn = *chead++;
    if (chn >= 0)	/* 一般看板 */
    {
      val = bits[chn];
      if(!(brd[chn].brdname[0]))
	continue;

      if (!(val & BRD_L_BIT))
        continue;

      if (class_flag2 && (!(val & BRD_R_BIT) ||
	  (brd[chn].readlevel != PERM_BOARD && brd[chn].readlevel != PERM_SYSOP)))
        continue;
    }

    max++;
    *cbase++ = chn;
  } while (chead < ctail);

  xo->max = max;
  if (xo->pos >= max)
    xo->pos = xo->top = 0;

  return max;
}


static int
XoClass(chn)
  int chn;
{
  XO xo, *xt;

  /* Thor.980727: 解決 XO xo的不確定性, 
                  class_load內部會 initial xo.max, 其他不確定 */
  xo.pos = xo.top = 0;

  xo.key = chn;
  xo.xyz = NULL;
  if (!class_load(&xo))
  {
    if (xo.xyz)
      free(xo.xyz);
    return 0;
  }

  xt = xz[XZ_CLASS - XO_ZONE].xo;
  xz[XZ_CLASS - XO_ZONE].xo = &xo;

#ifdef AUTO_JUMPBRD
  if (cuser.ufo & UFO_JUMPBRD)
    class_jumpnext = 1;	/* itoc.010910: 主動跳去下一個未讀看板 */
#endif
  xover(XZ_CLASS);

  free(xo.xyz);
  xz[XZ_CLASS - XO_ZONE].xo = xt;

  return 1;
}


void
class_item(num, bno, brdpost)
  int num, bno, brdpost;
{
  char *str1;
  BRD *brd = bshm->bcache + bno;
  BST *bst = bshm->bstate + bno;

  /* 更新bstate[] (原btime_refresh) */
  if(brd->btime < 0)
  {
    char folder[64];
    struct stat st;
    FILE *fp;

    memset(bst, 0, sizeof(BST));

    brd->btime = 1;
    brd_fpath(folder, brd->brdname, fn_dir);
    if(!stat(folder, &st) && st.st_size >= sizeof(HDR) && (fp = fopen(folder, "rb")))
    {
      HDR hdr;
      int n = st.st_size / sizeof(HDR);
      time_t blst_next = 0;

      bst->bpost = n;

      fseeko(fp, -sizeof(HDR), SEEK_END);
      while(n > 0)
      {
	fread(&hdr, sizeof(HDR), 1, fp);
	fseeko(fp, -sizeof(HDR) * 2, SEEK_CUR);

	if(!(hdr.xmode & POST_BOTTOM))
	{
	  if(hdr.xmode & POST_RESTRICT)
	    blst_next = hdr.chrono;
	  else
	    break;
	}

	n--;
      }

      bst->blast[0] = hdr.chrono;
      bst->blast[1] = hdr.stamp;
      bst->blast_around[1] = blst_next? blst_next : hdr.chrono;
      if(fread(&hdr, sizeof(HDR), 1, fp) == 1)
        bst->blast_around[0] = hdr.chrono;
      else
	bst->blast_around[0] = bst->blast[0];

      fclose(fp);
    }
  }


  /* 印出編號/篇數 */
  if (brdpost)
    prints("%6d", bshm->bstate[bno].bpost);
  else
    prints("%6d", num);

  /* 印出 friend/secret 板的符號 */
  if (brd->readlevel == PERM_SYSOP)
    outc(TOKEN_SECRET_BRD);
  else if (brd->readlevel == PERM_BOARD)
    outc(TOKEN_FRIEND_BRD);
  else
    outc(' ');

  /* 印出已讀/未讀 */
  switch(rhs_unread(brd->bstamp, bst->blast[0], bst->blast[1]))
  {
  case 0:
    outs(ICON_READ_BRD);
    break;

  case 1:
    outs(ICON_UNREAD_BRD);
    break;

  case 2:
    if(cuser.ufo & UFO_NODCBUM)
      outs(ICON_UNREAD_BRD);	/* 不使用雙色看板未讀燈，推文未讀用同種符號 */
    else
      outs(ICON_CUNREAD_BRD);
    break;
  }

  prints("%-13s\033[1;3%dm%-5s\033[m", brd->brdname, brd->class[3] & 7, brd->class);

  /* 印出投票/轉信 */
  switch(brd->bvote)
  {
  case 0:
    outs((brd->battr & BRD_NOTRAN) ? ICON_NOTRAN_BRD : ICON_TRAN_BRD);
    break;
  case 1:
    outs(ICON_VOTED_BRD);
    break;
  case 2:
    outs(ICON_GAMBLED_BRD);
    break;
  case 3:
    outs(ICON_VOTGAM_BRD);
    break;
  }


  /* itoc.060530: 借用 str1、num 來處理看板敘述顯示的中文斷字 */
  str1 = brd->title;
  num = (d_cols >> 1) + 33;
  prints(" %-*.*s", num, IS_ZHC_LO(str1, num - 1) ? num - 2 : num - 1, str1);

  /* 印出人氣 */
  bno = bshm->mantime[bno];
  if (bno > 99)
    outs("\033[1;31m爆\033[m");
  else if (bno > 0)
  {
    if (bno > 30)
      attrset(11);
    else if (bno >= 10)
      attrset(15);
    else
      attrset(8);
      
    prints("%2d\033[m", bno);
  }
  else
    outs("\033[1;30m  \033[m");

  prints(" %.*s\n", d_cols - (d_cols >> 1) + 12, brd->BM);
}


static int
class_body(xo)
  XO *xo;
{
  short *chp;
  BRD *bcache;
  BST *bstate;
  int n, cnt, max, chn, brdpost;
#ifdef AUTO_JUMPBRD
  int nextpos;
#endif

  bcache = bshm->bcache;
  bstate = bshm->bstate;
  max = xo->max;
  cnt = xo->top;

#ifdef AUTO_JUMPBRD
  nextpos = 0;

  /* itoc.010910: 搜尋下一個未讀看板 */
  if (class_jumpnext)
  {
    class_jumpnext = 0;
    n = xo->pos;
    chp = (short *) xo->xyz + n;

    while (n < max)
    {
      chn = *chp++;
      if (chn >= 0)
      {
	/* itoc.010407: 改用最後一篇已讀/未讀來判斷 */
	if (rhs_unread(bcache[chn].bstamp, bstate[chn].blast[0], bstate[chn].blast[1]))
	{
	  nextpos = n;
	  break;
	}
      }
      n++;
    }

    /* 下一個未讀板在別頁，要翻過去 */
    if (nextpos >= cnt + XO_TALL)
      return nextpos + XO_MOVE;
  }
#endif

  brdpost = class_flag & UFO_BRDPOST;
  chp = (short *) xo->xyz + cnt;

  n = 3;
  move(3, 0);
  do
  {
    chn = *chp;
    if (cnt < max)
    {
      clrtoeol();
      cnt++;
      if (chn >= 0)		/* 一般看板 */
      {
	class_item(cnt, chn, brdpost);
      }
      else			/* 分類群組 */
      {
	short *chx;
	char *img, *str;

	img = class_img;
	chx = (short *) img + (CH_END - chn);
	str = img + *chx;
	prints("%6d   %-13.13s\033[1;3%dm%-5.5s\033[m%s\n", 
	  cnt, str, str[BNLEN + 4] & 7, str + BNLEN + 1, str + BNLEN + 1 + BCLEN + 1);
      }
      chp++;
    }
    else
    {
      clrtobot();
      break;
    }
  } while (++n < b_lines);

#ifdef AUTO_JUMPBRD
  /* itoc.010910: 下一個未讀板在本頁，要把游標移過去 */
  outf(FEETER_CLASS);   
  return nextpos ? nextpos + XO_MOVE : XO_NONE;
#else
  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
#endif
}


static int
class_neck(xo)
  XO *xo;
{
  move(1, 0);
  prints(NECKER_CLASS, 
    class_flag & UFO_BRDPOST ? "總數" : "編號", 
    d_cols >> 1, "", d_cols - (d_cols >> 1), "");
  return class_body(xo);
}


static int
class_head(xo)
  XO *xo;
{
  vs_head("分類看板", str_site);
  return class_neck(xo);
}


static int
class_init(xo)			/* re-init */
  XO *xo;
{
  class_load(xo);
  return class_head(xo);
}


static int
class_postmode(xo)
  XO *xo;
{
  cuser.ufo ^= UFO_BRDPOST;
  cutmp->ufo = cuser.ufo;
  class_flag ^= UFO_BRDPOST;
  return class_neck(xo);
}


static int
class_namemode(xo)		/* itoc.010413: 看板依照字母/分類排列 */
  XO *xo;
{
  static time_t last = 0;
  time_t now;
 
  if (time(&now) - last < 10)
  {
    vmsg("每十秒鐘只能切換一次");
    return XO_FOOT;
  }
  last = now;

  if (cuser.userlevel)
    rhs_save();

  cuser.ufo ^= UFO_BRDNAME;
  cutmp->ufo = cuser.ufo;
  board_main();			/* 重新載入 class_img */
  return class_neck(xo);
}


static int
class_help(xo)
  XO *xo;
{
  xo_help("class");
  return class_head(xo);
}


static int
class_search(xo)
  XO *xo;
{
  int num, pos, max;
  char buf[BNLEN + 1];

  if (vget(b_lines, 0, MSG_BID, buf, BNLEN + 1, DOECHO))
  {
    short *chp, chn;
    BRD *bcache, *brd;

    str_lowest(buf, buf);

    bcache = bshm->bcache;
    pos = num = xo->pos;
    max = xo->max;
    chp = (short *) xo->xyz;

    do
    {
      if (++pos >= max)
	pos = 0;
      chn = chp[pos];
      if (chn >= 0)
      {
	brd = bcache + chn;
	if (str_str(brd->brdname, buf) || str_sub(brd->title, buf))
	{
	  outf(FEETER_CLASS);	/* itoc.010913: 把 b_lines 填上 feeter */
	  return pos + XO_MOVE;
	}
      }
    } while (pos != num);
  }

  return XO_FOOT;
}


static int
class_searchBM(xo)
  XO *xo;
{
  int num, pos, max;
  char buf[IDLEN + 1];

  if (vget(b_lines, 0, "請輸入板主：", buf, IDLEN + 1, DOECHO))
  {
    short *chp, chn;
    BRD *bcache, *brd;

    str_lower(buf, buf);

    bcache = bshm->bcache;
    pos = num = xo->pos;
    max = xo->max;
    chp = (short *) xo->xyz;

    do
    {
      if (++pos >= max)
	pos = 0;
      chn = chp[pos];
      if (chn >= 0)
      {
	brd = bcache + chn;
	if (str_str(brd->BM, buf))
	{
	  outf(FEETER_CLASS);	/* itoc.010913: 把 b_lines 填上 feeter */
	  return pos + XO_MOVE;
	}
      }
    } while (pos != num);
  }

  return XO_FOOT;
}



static int
class_yank(xo)
  XO *xo;
{
  if (xo->key >= 0)
    return XO_NONE;

  class_flag2 ^= 0x01;
  return class_init(xo);
}



static int
class_visit(xo)		/* itoc.010128: 看板列表設定看板已讀 */
  XO *xo;
{
  short *chp;
  int chn;

  chp = (short *) xo->xyz + xo->pos;
  chn = *chp;
  if (chn >= 0)
  {
    BST *bst = bshm->bstate + chn;
    rhs_get(bshm->bcache[chn].bstamp, chn);

    /* dust.101114: 只將最後一篇加入閱讀紀錄 */
    if(cuser.ufo2 & UFO2_LIMITVISIT)
    {
      switch(rhs_unread(-1, bst->blast[0], bst->blast[1]))
      {
      case 1:
	brh_add(bst->blast_around[0], bst->blast[0], bst->blast_around[1]);

      case 2:
	crh_add(bst->blast[0], bst->blast[1]);
	break;
      }
    }
    else
      rhs_visit(0);
  }
  return class_body(xo);
}


static int
class_unvisit(xo)		/* itoc.010129: 看板列表設定看板未讀 */
  XO *xo;
{
  short *chp;
  int chn;

  chp = (short *) xo->xyz + xo->pos;
  chn = *chp;
  if (chn >= 0)
  {
    BRD *brd;
    brd = bshm->bcache + chn;
    rhs_get(brd->bstamp, chn);
    rhs_visit(1);
  }
  return class_body(xo);
}


static int
class_nextunread(xo)
  XO *xo;
{
  int max, pos, chn;
  short *chp;
  BRD *bcache;
  BST *bstate;

  bcache = bshm->bcache;
  bstate = bshm->bstate;
  max = xo->max;
  pos = xo->pos;
  chp = (short *) xo->xyz + pos;

  while (++pos < max)
  {
    chn = *(++chp);
    if (chn >= 0)	/* 跳過分類 */
    {
      /* itoc.010407: 改用最後一篇已讀/未讀來判斷 */
      if (rhs_unread(bcache[chn].bstamp, bstate[chn].blast[0], bstate[chn].blast[1]))
	return pos + XO_MOVE;
    }
  }

  return XO_NONE;
}


static int
class_edit(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_ALLBOARD | PERM_BM))
  {
    short *chp;
    int chn;

    chp = (short *) xo->xyz + xo->pos;
    chn = *chp;
    if (chn >= 0)
    {
      if (!HAS_PERM(PERM_ALLBOARD))
	brd_title(chn);		/* itoc.000312: 板主修改中文敘述 */
      else
	brd_edit(chn);
      return class_init(xo);
    }
  }
  return XO_NONE;
}


static int
hdr_cmp(a, b)
  HDR *a;
  HDR *b;
{
  /* 先比對分類，再比對板名 */
  int k = strncmp(a->title + BNLEN + 1, b->title + BNLEN + 1, BCLEN);
  return k ? k : str_cmp(a->xname, b->xname);
}


static int
class_newbrd(xo)
  XO *xo;
{
  BRD newboard;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  memset(&newboard, 0, sizeof(BRD));

  /* itoc.010211: 新看板預設 postlevel = PERM_POST; battr = 不轉信 */
  newboard.postlevel = PERM_POST;
  newboard.battr = BRD_NOTRAN;

  if (brd_new(&newboard) < 0)
    return class_head(xo);

  if (xo->key < CH_END)		/* 在分類群組裡面 */
  {
    short *chx;
    char *img, *str;
    char xname[BNLEN + 1], fpath[64];
    HDR hdr;

    img = class_img;
    chx = (short *) img + (CH_END - xo->key);
    str = img + *chx;

    str_ncpy(xname, str, sizeof(xname));
    if (str = strchr(xname, '/'))
      *str = '\0';

    /* 加入分類群組 */
    sprintf(fpath, "gem/@/@%s", xname);
    brd2gem(&newboard, &hdr);
    rec_add(fpath, &hdr, sizeof(HDR));
    rec_sync(fpath, sizeof(HDR), hdr_cmp, NULL);
    vmsg("新板成立");
  }
  else				/* 在看板列表裡面 */
  {
    vmsg("新板成立，記著加入分類群組");
  }

  return class_init(xo);
}


static int
class_browse(xo)
  XO *xo;
{
  short *chp;
  int chn;

  chp = (short *) xo->xyz + xo->pos;
  chn = *chp;
  if (chn < 0)		/* 進入分類 */
  {
    if (!XoClass(chn))
      return XO_NONE;
  }
  else			/* 進入看板 */
  {
    if (XoPost(chn))	/* 無法閱讀該板 */
      return XO_FOOT;
    xover(XZ_POST);
  }

#ifdef AUTO_JUMPBRD
  if (cuser.ufo & UFO_JUMPBRD)
    class_jumpnext = 1;		/* itoc.010910: 只有在離開看板回到看板列表時才需要跳去下一個未讀看板 */
#endif

  return class_head(xo);	/* Thor.980701: 無法清少一點, 因為 XoPost */
}


int
Select()
{
  int bno;
  BRD *brd;
  char bname[BNLEN + 1];

  bbstate &= ~STAT_CANCEL;
  if (brd = ask_board(bname, BRD_R_BIT, NULL))
  {
    bno = brd - bshm->bcache;
    XoPost(bno);
    xover(XZ_POST);
  }
  else if(!(bbstate & STAT_CANCEL))
  {
    vmsg(err_bid);
  }

  return 0;
}


static int
class_switch(xo)
  XO *xo;
{
  Select();
  return class_head(xo);
}


#ifdef MY_FAVORITE

/* ----------------------------------------------------- */
/* MyFavorite [我的最愛]				 */
/* ----------------------------------------------------- */


static inline int
in_favor(brdname)
  char *brdname;
{
  MF mf;
  int fd;
  int rc = 0;
  char fpath[64];

  if (brdname[0])
  {
    mf_fpath(fpath, cuser.userid, FN_MF);

    if ((fd = open(fpath, O_RDONLY)) >= 0)
    {
      while (read(fd, &mf, sizeof(MF)) == sizeof(MF))
      {
	if (!strcmp(brdname, mf.xname))
	{
	  rc = 1;
	  break;
	}
      }
    }
    close(fd);
  }
  return rc;
}


static int 
class_addMF(xo)
  XO *xo;  
{    
  short *chp;
  int chn;
  MF mf;
  char fpath[64];

  if (!cuser.userlevel)
    return XO_NONE;
  
  chp = (short *) xo->xyz + xo->pos;
  chn = *chp;
      
  if (chn >= 0)		/* 一般看板 */
  {
    BRD *bhdr;

    bhdr = bshm->bcache + chn;

    if (!in_favor(bhdr->brdname))
    {
      memset(&mf, 0, sizeof(MF));
      time(&mf.chrono);
      mf.mftype = MF_BOARD;
      strcpy(mf.xname, bhdr->brdname);

      mf_fpath(fpath, cuser.userid, FN_MF);
      rec_add(fpath, &mf, sizeof(MF));
      vmsg("已將此看板加入我的最愛");
    }
    else
    {
      vmsg("此看板已在最愛中。若要重覆加入，請進我的最愛裡新增");
    }
  }
  else			/* 分類群組 */
  {
    short *chx;
    char *img, *str, *ptr;

    img = class_img;
    chx = (short *) img + (CH_END - chn);
    str = img + *chx;

    memset(&mf, 0, sizeof(MF));
    time(&mf.chrono);
    mf.mftype = MF_CLASS;
    ptr = strchr(str, '/');
    strncpy(mf.xname, str, ptr - str);
    strncpy(mf.class, str + BNLEN + 1, BCLEN);
    strcpy(mf.title, str + BNLEN + 1 + BCLEN + 1);

    mf_fpath(fpath, cuser.userid, FN_MF);
    rec_add(fpath, &mf, sizeof(MF));
    vmsg("已將此分類加入我的最愛");
  }

  return XO_FOOT;
}


int
MFclass_browse(name)
  char *name;
{
  int chn, min_chn, len;
  short *chx;
  char *img, cname[BNLEN + 2];

  min_chn = bshm->min_chn;
  img = class_img;

  sprintf(cname, "%s/", name);
  len = strlen(cname);

  for (chn = CH_END - 2; chn >= min_chn; chn--)
  {
    chx = (short *) img + (CH_END - chn);
    if (!strncmp(img + *chx, cname, len))
    {
      if (XoClass(chn))
	return 1;
      break;
    }
  }
  return 0;
}
  
#endif  /* MY_FAVORITE */


#ifdef AUTHOR_EXTRACTION
/* Thor.980818: 想改成以目前的看板列表或分類來找, 不要找全部 */


/* opus.1127 : 計畫重寫, 可 extract author/title */


static int
XoAuthor(xo)
  XO *xo;
{
  int chn, len, max, tag, value;
  short *chp, *chead, *ctail;
  BRD *brd;
  char key[30], author[IDLEN + 1];
  XO xo_a, *xoTmp;
  struct timeval tv = {0, 10};

  vget(b_lines, 0, MSG_XYPOST1, key, 30, DOECHO);
  vget(b_lines, 0, MSG_XYPOST2, author, IDLEN + 1, DOECHO);

  if (!*key && !*author)
    return XO_FOOT;

  str_lowest(key, key);
  str_lower(author, author);
  len = strlen(author);

  chead = (short *) xo->xyz;
  max = xo->max;
  ctail = chead + max;

  tag = 0;
  chp = (short *) malloc(max * sizeof(short));
  brd = bshm->bcache;

  do
  {
    if ((chn = *chead++) >= 0)	/* Thor.980818: 不為 group */
    {
      /* Thor.980701: 尋找指定作者文章, 有則移位置, 並放入 */

      int fsize;
      char *fimage;

      char folder[80];
      HDR *head, *tail;

      sprintf(folder, "《尋找指定標題作者》看板：%s \033[5m...\033[m按任意鍵中斷", brd[chn].brdname);
      outz(folder);
      refresh();

      brd_fpath(folder, brd[chn].brdname, fn_dir);
      fimage = f_map(folder, &fsize);

      if (fimage == (char *) -1)
	continue;

      head = (HDR *) fimage;
      tail = (HDR *) (fimage + fsize);

      while (head <= --tail)
      {
	if ((!*key || str_sub(tail->title, key)) &&
	  (!len || !str_ncmp(tail->owner, author, len)))
	{
	  xo_get(folder)->pos = tail - head;
	  chp[tag++] = chn;
	  break;
	}
      }

      munmap(fimage, fsize);
    }

    /* 使用者可以中斷搜尋 */
    value = 1;
    if (select(1, (fd_set *) &value, NULL, NULL, &tv) > 0)
    {
      vkey();
      break;
    }
  } while (chead < ctail);

  if (!tag)
  {
    free(chp);
    vmsg("空無一物");
    return XO_FOOT;
  }

  xo_a.pos = xo_a.top = 0;
  xo_a.max = tag;
  xo_a.key = 1;			/* all boards */
  /* Thor.990621: 所有的class,board列表下, key < 0, 以 1 與正常模式區分
                  使其不能跑 XO_INIT(裏面的class_load), 如 class_yank,
                  除了防止找出的作者看板列表消失, 也防踢人 */ 
  xo_a.xyz = (char *) chp;

  xoTmp = xz[XZ_CLASS - XO_ZONE].xo;	/* Thor.980701: 記下原來的class_xo */
  xz[XZ_CLASS - XO_ZONE].xo = &xo_a;

#ifdef AUTO_JUMPBRD
  if (cuser.ufo & UFO_JUMPBRD)
    class_jumpnext = 1;	/* itoc.010910: 主動跳去下一個未讀看板 */
#endif
  xover(XZ_CLASS);

  free(chp);
  xz[XZ_CLASS - XO_ZONE].xo = xoTmp;	/* Thor.980701: 還原 class_xo */

  return class_body(xo);
}
#endif



void
query_bstate(bno)
  int bno;
{
  BST *bst = bshm->bstate + bno;

  clrregion(b_lines - 7, b_lines);

  move(b_lines - 6, 0);
  prints("board stamp = %d\n\n", bshm->bcache[bno].bstamp);

  prints("chrono of prev : %d (%s)\n", bst->blast_around[0], Btime(&bst->blast_around[0]));
  prints("chrono of last : %d (%s)\n", bst->blast[0], Btime(&bst->blast[0]));
  prints("chrono of next : %d (%s)\n", bst->blast_around[1], Btime(&bst->blast_around[1]));
  prints(" stamp of last : %d (%s)\n", bst->blast[1], Btime(&bst->blast[1]));

  vmsg(NULL);
}


static int
class_boardquery(XO *xo)
{
  short chn;

  if(!(cuser.userlevel & PERM_SYSOP))
    return XO_NONE;

  chn = *((short *) xo->xyz + xo->pos);
  if (chn >= 0)
  {
    query_bstate(chn);
    return class_body(xo);
  }
  else
    return XO_NONE;
}


int
class_savebrh(xo)
  XO *xo;
{
  static time_t save_time;

  if(time(0) - save_time < 600)
    vmsg("每十分鐘才能儲存一次看板閱\讀紀錄");
  else
  {
    rhs_save();
    rhs_load();
    time(&save_time);
    vmsg("看板閱\讀紀錄已儲存");
  }

  return XO_NONE;
}


void rhs_dump(void);

static int
class_dumpXRH(XO *xo)
{
  if(cuser.userlevel & PERM_SYSOP)
    rhs_dump();
  return XO_NONE;
}


static KeyFunc class_cb[] =
{
  XO_INIT, class_head,
  XO_LOAD, class_body,
  XO_HEAD, class_head,
  XO_BODY, class_body,

  'r', class_browse,
  '/', class_search,
  '?', class_searchBM,
  's', class_switch,
  'c', class_postmode,
  'S', class_namemode,

  'i', class_yank,
  'v', class_visit,
  'V', class_unvisit,
  '`', class_nextunread,
  'E', class_edit,
  'w', class_savebrh,

#ifdef AUTHOR_EXTRACTION
  'A', XoAuthor,
#endif

#ifdef MY_FAVORITE
  'a', class_addMF,
  'f', class_addMF,
#endif

  Ctrl('P'), class_newbrd,

  'Q', class_boardquery,
  Ctrl('Q'), class_dumpXRH,

  'h', class_help
};


int
Class()
{
  /* XoClass(CH_END - 1); */
  /* Thor.980804: 防止 未用 account 造出 class.img 或沒有 class 的情況 */
  if (!class_img || !XoClass(CH_END - 1))
  {
    vmsg("未定義分組討論區");
    return XEASY;
  }
  return 0;
}


void
board_main()
{
  int fsize, bno, brd_max;

  /* dust.101031: 決定對各看板權限 */
  memset(brd_bits, 0, sizeof(brd_bits));
  brd_max = bshm->number;
  for(bno = 0; bno < brd_max; bno++)
  {
    brd_bits[bno] = Ben_Perm(bno, cuser.userlevel);
  }

  /* dust.101021: 載入閱讀紀錄 */
  rhs_load();

  if (class_img)	/* itoc.030416: 第二次進入 board_main 時，要 free 掉 class_img */
  {
    free(class_img);
  }
  else			/* itoc.040102: 第一次進入 board_main 時，才需要初始化 class_flag */
  {
    class_flag = cuser.ufo & UFO_BRDPOST;	/* 看板列表 1:文章數 0:編號 */

    /* 設定 default board */
    strcpy(currboard, BN_NULL);
    currbno = -1;
  }

  /* class_img = f_img(CLASS_IMGFILE, &fsize); */
  /* itoc.010413: 依照 ufo 來載入不同的 class image */
  class_img = f_img(cuser.ufo & UFO_BRDNAME ? CLASS_IMGFILE_NAME : CLASS_IMGFILE_TITLE, &fsize);

  if (class_img == NULL)
    blog("CACHE", "class.img");

  board_xo.key = CH_END;
  class_load(&board_xo);

  xz[XZ_CLASS - XO_ZONE].xo = &board_xo;	/* Thor: default class_xo */
  xz[XZ_CLASS - XO_ZONE].cb = class_cb;		/* Thor: default class_xo */
}


int
Boards()
{
  /* class_xo = &board_xo; *//* Thor: 已有 default, 不需作此 */

#ifdef AUTO_JUMPBRD
  if (cuser.ufo & UFO_JUMPBRD)
    class_jumpnext = 1;	/* itoc.010910: 主動跳去下一個未讀看板 */
#endif
  xover(XZ_CLASS);

  return 0;
}
