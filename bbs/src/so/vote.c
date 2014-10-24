/*-------------------------------------------------------*/
/* vote.c	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : boards' vote routines		 	 */
/* create : 95/03/29				 	 */
/* update : 09/05/16				 	 */
/*-------------------------------------------------------*/
/* brd/_/.VCH  : Vote Control Header    目前所有投票索引 */
/* brd/_/@vote : vote history           過去的投票歷史	 */
/* brd/_/@/@__ : vote description       投票說明	 */
/* brd/_/@/I__ : vote selection Items   投票選項	 */
/* brd/_/@/O__ : user's Opinion(comment) 使用者有話要說	 */
/* brd/_/@/L__ : can vote List          可投票名單	 */
/* brd/_/@/G__ : voted id loG file      已領票名單	 */
/* brd/_/@/Z__ : final/temporary result	投票結果	 */
/* brd/_/@/B__ : 賭盤下注紀錄(我英文不好不要叫我寫英文)	 */
/*-------------------------------------------------------*/
/* dust.090426: 1.分離賭盤系統所使用的函數		 */
/*		2.改良賭盤的介面			 */
/*-------------------------------------------------------*/


#include "bbs.h"



extern BCACHE *bshm;
extern XZ xz[];
extern char xo_pool[];


static char *
vch_fpath(fpath, folder, vch)
  char *fpath, *folder;
  VCH *vch;
{
  /* VCH 和 HDR 的 xname 欄位匹配，所以直接借用 hdr_fpath() */
  hdr_fpath(fpath, folder, (HDR *) vch);
  return strrchr(fpath, '@');
}


static int vote_add();


int
vote_result(xo)
  XO *xo;
{
  char fpath[64];

  setdirpath(fpath, xo->dir, "@/@vote");
  /* Thor.990204: 為考慮more 傳回值 */   
  if (more(fpath, MFST_NORMAL) >= 0)
    return XO_HEAD;	/* XZ_POST 和 XZ_VOTE 共用 vote_result() */

  vmsg("目前沒有任何開票的結果");
  return XO_FOOT;
}


static void
vote_item(num, vch)
  int num;
  VCH *vch;
{
  prints("%6d%c%c%c%c%c %-9.8s%-12s %.44s\n",
    num, tag_char(vch->chrono), vch->vgamble, vch->vsort, vch->vpercent, vch->vprivate, 
    vch->cdate, vch->owner, vch->title);
}


static int
vote_body(xo)
  XO *xo;
{
  VCH *vch;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    if (bbstate & STAT_BOARD)
    {
      if (vans("要舉辦投票嗎(Y/N)？[N] ") == 'y')
	return vote_add(xo);
    }
    else
    {
      vmsg("目前並無投票舉行");
    }
    return XO_QUIT;
  }

  vch = (VCH *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    vote_item(++num, vch++);
  } while (num < max);
  clrtobot();

  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
}


static int
vote_head(xo)
  XO *xo;
{
  vs_head("投票/賭盤列表", "投票所");
  prints(NECKER_VOTE, d_cols, "");
  return vote_body(xo);
}


static int
vote_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(VCH));
  return vote_head(xo);
}


static int
vote_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(VCH));
  return vote_body(xo);
}


static void
vch_edit(vch, item, echo)
  VCH *vch;
  int item;		/* vlist 有幾項 */
  int echo;		/* DOECHO:新增  GCARRY:修改 */
{
  int num, row, days;
  char ans[8], buf[80];

  clear();

  row = 4;


  sprintf(buf, "請問每人最多可投幾票？(1~%d) ", item);
  if(echo == GCARRY)
    sprintf(ans, "%d", vch->maxblt);
  vget(++row, 0, buf, ans, 3, echo);
  num = atoi(ans);
  if (num < 1)
    num = 1;
  else if (num > item)
    num = item;
  vch->maxblt = num;

  if(echo == DOECHO)
  {
    move(++row, 0);
    outs("本項投票進行多久(至少一小時)？ ");
    vget(row, 31, "幾天 [0] ", ans, 3, DOECHO);
    days = atoi(ans);
    if (days < 0)
      days = 0;

    sprintf(buf, "幾小時 [%d] ", (days == 0)? 1:0);
    vget(row, 46, buf, ans, 5, DOECHO);
    num = atoi(ans);
    if (num < 1 && days == 0)
      num = 1;
    else if(num < 0)
      num = 0;

    vch->vclose = vch->chrono + (days * 24 + num) * 3600;
    str_stamp(vch->cdate, &vch->vclose);
  }

  vch->vsort = (vget(++row, 0, "開票結果是否排序(Y/N)？[N] ", ans, 3, LCECHO) == 'y') ? 's' : ' ';
  vch->vpercent = (vget(++row, 0, "開票結果是否顯示百分比例(Y/N)？[N] ", ans, 3, LCECHO) == 'y') ? '%' : ' ';
  vch->vprivate = (vget(++row, 0, "是否限制投票名單(Y/N)？[N] ", ans, 3, LCECHO) == 'y') ? ')' : ' ';

  if (vget(++row, 0, "是否限制投票資格(Y/N)？[N] ", ans, 3, LCECHO) == 'y')
  {
    vget(++row, 0, "請問要登入幾次以上才可以參加本次投票？([0]～9999)：", ans, 5, DOECHO);
    num = atoi(ans);
    if (num < 0)
      num = 0;
    vch->limitlogins = num;

    vget(++row, 0, "請問要發文幾次以上才可以參加本次投票？([0]～9999)：", ans, 5, DOECHO);
    num = atoi(ans);
    if (num < 0)
      num = 0;
    vch->limitposts = num;
  }
}


static void
vch_editG(vch, item, echo)	/* dust.090426: vch_edit()的賭盤專用版 */
  VCH *vch;
  int item;
  int echo;		/* DOECHO:新增  GCARRY:修改 */
{
  int num, row, dyear;
  char ans[8];
  struct tm ntm, ftm, ctm;
  /* ntm:現在時間  ftm:未來的時間  ctm:封盤的時間(修改時才會用到) */
  time_t now;

  clear();

  row = 3;

  if (echo == DOECHO)	/* 只有新增時才能改賭盤的票價 */
  {
    vch->maxblt = 1;	/* 賭盤就只能選一項 */

    vget(++row, 0, "每張售價多少銀幣？(50～10000)：", ans, 6, DOECHO);
    num = atoi(ans);
    if (num < 50)
      num = 50;
    else if (num > 10000)
      num = 10000;
    vch->price = num;
  }

  row++;
  move(++row, 0);
  outs("本項賭盤的結束時間(至少一小時，最長兩年)：");
  /* dust.090426: 事實上只有用年份和月份來判斷而已XD */
  row++;

  time(&now);
  ntm = *localtime(&now);
  ftm = ntm;	/* 初始化ftm */
  if(echo == GCARRY)	/* 修改時，截止時間會有預設值 */
    ctm = *localtime(&vch->vclose);
  else	/* 省麻煩的寫法 */
    ctm = *localtime(&now);

  sprintf(ans, "%d", ctm.tm_year + 1900);
  vget(++row, 0, "    年(西元)：", ans, 5, echo);
  num = atoi(ans);
  dyear = num - 1900 - ntm.tm_year;
  if (dyear > 2 || dyear < 0)
    dyear = 0;
  ftm.tm_year = ntm.tm_year + dyear;

  sprintf(ans, "%d", ctm.tm_mon + 1);
  vget(++row, 0, "  月份(1~12)：", ans, 3, echo);
  num = atoi(ans);
  if (dyear > 2 && num > ntm.tm_mon + 1)
    num = ntm.tm_mon;
  else if(num > 12)
    num = 12;
  else if(num < 1)
    num = 1;
  ftm.tm_mon = num - 1;

  sprintf(ans, "%d", ctm.tm_mday);
  vget(++row, 0, "  日期(1~31)：", ans, 3, echo);
  num = atoi(ans);
  if(num > 31)
    num = 31;
  else if(num < 1)
    num = 0;
  ftm.tm_mday = num;

  sprintf(ans, "%d", ctm.tm_hour);
  vget(++row, 0, "  幾點(0~23)：", ans, 3, echo);
  num = atoi(ans);
  if(num > 23)
    num = 23;
  else if(num < 0)
    num = 0;
  ftm.tm_hour = num;

  sprintf(ans, "%d", ctm.tm_min);
  vget(++row, 0, "  幾分(0~59)：", ans, 3, echo);
  num = atoi(ans);
  if(num > 59)
    num = 59;
  else if(num < 0)
    num = 0;
  ftm.tm_min = num;

  sprintf(ans, "%d", ctm.tm_sec);
  vget(++row, 0, "  幾秒(0~59)：", ans, 3, echo);
  num = atoi(ans);
  if(num > 59)
    num = 59;
  else if(num < 0)
    num = 0;
  ftm.tm_sec = num;

  vch->vclose = mktime(&ftm);
  if(vch->vclose < vch->chrono + 3600)	/* dust.090426: 至少舉辦一小時 */
    vch->vclose = vch->chrono + 3600;

  /* dust.090427: 賭盤不需要下列這些屬性 */
  vch->vsort = ' ';
  vch->vpercent = ' ';
  vch->vprivate = ' ';

  str_stamp(vch->cdate, &vch->vclose);
}


static int
vlist_edit(vlist, n)
  vitem_t vlist[];
  int n;
{
  int item;
  char buf[80];

  clear();

  prints("請依序輸入選項 (最多 %d 項)，按 ENTER 結束：", n);

  strcpy(buf, " ) ");
  for (;;)
  {
    item = 0;
    for (;;)
    {
      buf[0] = radix32[item];
      if (!vget((item & 15) + 3, (item / 16) * 40, buf, vlist[item], sizeof(vitem_t), GCARRY)
        || (++item >= n))
	break;
    }
    if (item && vans("是否重新輸入選項(Y/N)？[N] ") != 'y')
      break;
  }
  return item;
}


static int
vlog_seek(fpath)
  char *fpath;
{
  VLOG old;
  int fd;
  int rc = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(VLOG)) == sizeof(VLOG))
    {
      if (!strcmp(old.userid, cuser.userid))
      {
	rc = 1;
	break;
      }
    }
    close(fd);
  }
  return rc;
}


static int
vote_add(xo)
  XO *xo;
{
  VCH vch;
  int fd, item, vgamble;
  char *dir, *str, fpath[64], title[TTLEN + 1];
  vitem_t vlist[MAX_CHOICES];
  BRD *brd;
  char *menu[] = {"v辦投票", "g開賭盤", "q取消", NULL};

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;
    
  switch(krans(b_lines, "vq", menu, NULL))
  {
  case 'v':
    vgamble = ' ';
    break;
  case 'g':
    vgamble = '$';
    break;
  default:
    return xo->max ? XO_FOOT : vote_body(xo);	/* 如果沒有任何投票，要回到 vote_body() */
  }

  if (!vget(b_lines, 0, "主題：", title, TTLEN + 1, DOECHO))
    return xo->max ? XO_FOOT : vote_body(xo);	/* itoc.011125: 如果沒有任何投票，要回到 vote_body() */

  dir = xo->dir;
  if ((fd = hdr_stamp(dir, 0, (HDR *) &vch, fpath)) < 0)
  {
    vmsg("無法建立說明檔");
    return XO_FOOT;
  }
  close(fd);

  if(vgamble == ' ')
    vmsg("開始編輯 [投票說明]");
  else
    vmsg("開始編輯 [賭盤說明]");

  fd = vedit(fpath, 0); /* Thor.981020: 注意被talk的問題 */
  if (fd)
  {
    unlink(fpath);
    if(vgamble == ' ')
      vmsg("取消投票");
    else
      vmsg("取消賭盤");
    return vote_head(xo);
  }

  strcpy(vch.title, title);
  str = strrchr(fpath, '@');

  /* --------------------------------------------------- */
  /* 投票選項檔 : Item					 */
  /* --------------------------------------------------- */

  memset(vlist, 0, sizeof(vlist));
  item = vlist_edit(vlist, (vgamble == ' ')? 32 : 22);

  *str = 'I';
  if ((fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
  {
    vmsg("無法建立選項檔！");
    return vote_head(xo);
  }
  write(fd, vlist, item * sizeof(vitem_t));
  close(fd);

  if(vgamble == ' ')
    vch_edit(&vch, item, DOECHO);
  else
    vch_editG(&vch, item, DOECHO);

  vch.vgamble = vgamble;

  strcpy(vch.owner, cuser.userid);

  brd = bshm->bcache + currbno;

  brd->bvote |= (vgamble == ' ')? 1 : 2;
  vch.bstamp = brd->bstamp;

  rec_add(dir, &vch, sizeof(VCH));

  if(vgamble == ' ')
    vmsg("開始投票了！");
  else
    vmsg("賭盤開始！");

  return vote_init(xo);
}


static int
vote_edit(xo)
  XO *xo;
{
  int pos;
  VCH *vch, vxx;
  char *dir, fpath[64], *msg;

  /* Thor: for 修改投票選項 */
  int fd, item;
  vitem_t vlist[MAX_CHOICES];
  char *fname;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  dir = xo->dir;
  vch = (VCH *) xo_pool + (pos - xo->top);

  vxx = *vch;

  /* 修改主題 */
  if (!vget(b_lines, 0, "主題：", vxx.title, TTLEN + 1, GCARRY))
    return XO_FOOT;

  /* 修改說明 */
  fname = vch_fpath(fpath, dir, vch);
  vedit(fpath, 0);	/* Thor.981020: 注意被talk的問題  */

  /* 修改選項 */
  memset(vlist, 0, sizeof(vlist));
  *fname = 'I';
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    read(fd, vlist, sizeof(vlist));
    close(fd);
  }

  item = vlist_edit(vlist, (vch->vgamble == ' ')? 32 : 20);

  if ((fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
  {
    vmsg("無法建立投票選項檔");
    return vote_head(xo);
  }
  write(fd, vlist, item * sizeof(vitem_t));
  close(fd);

  if(vxx.vgamble == ' ')
    vch_edit(&vxx, item, GCARRY);
  else
    vch_editG(&vxx, item, GCARRY);

  if (memcmp(&vxx, vch, sizeof(VCH)))
  {
    if(vxx.vgamble == ' ')
      msg = "確定要修改這項投票嗎(Y/N)？[N] ";
    else
      msg = "確定要修改這項賭盤嗎(Y/N)？[N] ";

    if (vans(msg) == 'y')
    {
      *vch = vxx;
      currchrono = vch->chrono;
      rec_put(dir, vch, sizeof(VCH), pos, cmpchrono);
    }
  }

  return vote_head(xo);
}


static int
vote_query(xo)
  XO *xo;
{
  char *dir, *fname, fpath[64], buf[80];
  VCH *vch;
  int cc, pos;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  dir = xo->dir;
  vch = (VCH *) xo_pool + (pos - xo->top);

  fname = vch_fpath(fpath, dir, vch);

  /* more(fpath, (char *) -1); */
  /* dust.090321: 應該沒有必要再顯示一次投票說明 */

  *fname = 'G';
  sprintf(buf, "已經有 %d 人參加，確定要改變結束時間嗎？(Y/N) [N] ", rec_num(fpath, sizeof(VLOG)));
  if (vans(buf) == 'y')
  {
    vget(b_lines, 0, "請更改結束時間(-n提前n小時/+m延後m小時/0不改)：", buf, 5, DOECHO);
    if (cc = atoi(buf))
    {
      vch->vclose += cc * 3600;
      currchrono = vch->chrono;
      /* dust.090321: 改成結束時間不能距離開始時間超過十年(315569260秒) */
      if(vch->vclose > currchrono + 315569260)
        vch->vclose = currchrono + 315569260;
      else if(vch->vclose < currchrono)
	vch->vclose = currchrono;	/* dust.090321: 以防結束時間比開始時間晚 */
      str_stamp(vch->cdate, &vch->vclose);
      rec_put(dir, vch, sizeof(VCH), pos, cmpchrono);
    }
  }

  return vote_head(xo); 
}


static int
vfyvch(vch, pos)
  VCH *vch;
  int pos;
{
  return Tagger(vch->chrono, pos, TAG_NIN);
}


static void
delvch(xo, vch)
  XO *xo;
  VCH *vch;
{
  int fd;
  char fpath[64], buf[64], *fname;
  char *list = "@IOLGZ";	/* itoc.註解: 清vote file */
  VLOG vlog;
  PAYCHECK paycheck;

  fname = vch_fpath(fpath, xo->dir, vch);

  if (vch->vgamble == '$')	/* itoc.050313: 如果是賭盤被刪除，那麼要退賭金 */
  {
    *fname = 'G';

    if ((fd = open(fpath, O_RDONLY)) >= 0)
    {
      memset(&paycheck, 0, sizeof(PAYCHECK));
      time(&paycheck.tissue);
      sprintf(paycheck.reason, "[賭盤退款] %s板", currboard);
	/* dust.090426: 原因的部份加長(反正現在reason欄擴大過了) */

      while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
      {
	paycheck.money = vlog.numvotes * vch->price;
	usr_fpath(buf, vlog.userid, FN_PAYCHECK);
	rec_add(buf, &paycheck, sizeof(PAYCHECK));
      }
    }
    close(fd);
  }

  while (*fname = *list++)
    unlink(fpath); /* Thor: 確定名字就砍 */
}



static int
vote_delete(xo)
  XO *xo;
{
  int pos;
  VCH *vch;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  vch = (VCH *) xo_pool + (pos - xo->top);

  if (vans(msg_del_ny) == 'y')
  {
    delvch(xo, vch);

    currchrono = vch->chrono;
    rec_del(xo->dir, sizeof(VCH), pos, cmpchrono);    
    return vote_load(xo);
  }

  return XO_FOOT;
}


static int
vote_rangedel(xo)
  XO *xo;
{
  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  return xo_rangedel(xo, sizeof(VCH), NULL, delvch);
}


static int
vote_prune(xo)
  XO *xo;
{
  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  return xo_prune(xo, sizeof(VCH), vfyvch, delvch);
}


static int
vote_pal(xo)		/* itoc.020117: 編輯限制投票名單 */
  XO *xo;
{
  char *fname, fpath[64];
  VCH *vch;
  XO *xt;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  vch = (VCH *) xo_pool + (xo->pos - xo->top);

  if (vch->vprivate != ')')
    return XO_NONE;

  fname = vch_fpath(fpath, xo->dir, vch);
  *fname = 'L';

  xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
  xt->key = PALTYPE_VOTE;
  xover(XZ_PAL);		/* Thor: 進xover前, pal_xo 一定要 ready */

  free(xt);
  return vote_init(xo);
}


static int
vote_chos(vch, xo)	/* dust.090426: 參與投票的主要處理函數 */
  VCH *vch;
  XO *xo;
{
  VCH vbuf;
  VLOG vlog;
  int count, fd;
  usint choice;
  char *dir, *fname, fpath[64], buf[80], *slist[MAX_CHOICES];
  vitem_t vlist[MAX_CHOICES];


  /* --------------------------------------------------- */
  /* 設定檔案路徑					 */
  /* --------------------------------------------------- */

  dir = xo->dir;
  fname = vch_fpath(fpath, dir, vch);

  /* --------------------------------------------------- */
  /* 檢查是否已經投過票					 */
  /* --------------------------------------------------- */

  *fname = 'G';
  if (vlog_seek(fpath))
  {
    vmsg("您已經投過票了！");
    return XO_FOOT;
  }

  /* --------------------------------------------------- */
  /* 檢查投票限制					 */
  /* --------------------------------------------------- */

  if (vch->vprivate == ' ')
  {
    if (cuser.numlogins < vch->limitlogins || cuser.numposts < vch->limitposts)
    {
      vmsg("您不夠資深喔！");
      return XO_FOOT;
    }
  }
  else		/* itoc.020117: 私人投票檢查是否在投票名單中 */
  {
    *fname = 'L';

    /* 由於並不能把自己加入朋友名單，所以要多檢查是否為板主 */
    if (!pal_find(fpath, cuser.userno) && !(bbstate & STAT_BOARD))
    {
      vmsg("您沒有受邀本次私人投票！");
      return XO_FOOT;
    }
  }

  /* --------------------------------------------------- */
  /* 確認進入投票					 */
  /* --------------------------------------------------- */

  if (vans("是否參加投票(Y/N)？[N] ") != 'y')
    return XO_FOOT;

  /* --------------------------------------------------- */
  /* 開始投票，顯示投票說明				 */
  /* --------------------------------------------------- */

  *fname = '@';
  more(fpath, MFST_NORMAL);

  /* --------------------------------------------------- */
  /* 載入投票選項檔					 */
  /* --------------------------------------------------- */

  *fname = 'I';
  if ((fd = open(fpath, O_RDONLY)) < 0)
  {
    vmsg("無法讀取投票選項檔");
    return vote_head(xo);
  }
  count = read(fd, vlist, sizeof(vlist)) / sizeof(vitem_t);
  close(fd);

  for (fd = 0; fd < count; fd++)
    slist[fd] = (char *) &vlist[fd];

  /* --------------------------------------------------- */
  /* 進行投票						 */
  /* --------------------------------------------------- */

  choice = 0;
  sprintf(buf, "投下神聖的 %d 票", vch->maxblt); /* Thor: 顯示最多幾票 */
  vs_bar(buf);

  for (;;)
  {
    choice = bitset(choice, count, vch->maxblt, vch->title, slist);

    vget(b_lines - 1, 0, "我有話要說：", buf, 60, DOECHO);

    fd = vans("投票 (Y)確定 (N)重來 (Q)取消？[N] ");

    if (fd == 'q')
      return vote_head(xo);

    if (fd == 'y')
      break;
  }

  /* --------------------------------------------------- */
  /* 記錄結果：一票也未投的情況 ==> 相當於投廢票	 */
  /* --------------------------------------------------- */

  /* 確定投票尚未截止 */
  /* itoc.050514: 因為板主可以改變開票時間，為了避免使用者會龜在 vget() 或是
     利用 xo_pool[] 未同步來規避 time(0) > vclose 的檢查，所以就得重新載入 VCH */
  if (rec_get(dir, &vbuf, sizeof(VCH), xo->pos) || vch->chrono != vch->chrono || time(0) > vbuf.vclose)
  {
    vmsg("投票已經截止了，請靜候開票");
    return vote_init(xo);
  }

  if (*buf)	/* 寫入使用者意見 */
  {
    FILE *fp;

    *fname = 'O';
    if (fp = fopen(fpath, "a"))
    {
      fprintf(fp, "‧%-12s：%s\n", cuser.userid, buf);
      fclose(fp);
    }
  }

  /* 加入記錄檔 */
  memset(&vlog, 0, sizeof(VLOG));
  strcpy(vlog.userid, cuser.userid);
  vlog.numvotes = 1;
  vlog.choice = choice;
  *fname = 'G';
  rec_add(fpath, &vlog, sizeof(VLOG));

  vmsg("投票完成！");

  return vote_head(xo);
}


static int
gamble_chos(vch, xo)	/* dust.090426: 參與賭盤的主要處理函數 */
  VCH *vch;
  XO *xo;
{
  VCH vbuf;
  VLOG vlog;
  int count, fd, i, k;
  usint choice;
  char *dir, *fname, fpath[64], buf[80], ans[4], *slist[MAX_CHOICES];
  vitem_t vlist[MAX_CHOICES];


  /* --------------------------------------------------- */
  /* 檢查是否有足夠錢					 */
  /* --------------------------------------------------- */

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XO_FOOT;
  }

  if (vch->vgamble == '$')
  {
    if (cuser.money < vch->price)
    {
      vmsg("您的錢不夠參加本賭盤");
      return XO_FOOT;
    }
  }

  /* --------------------------------------------------- */
  /* 設定檔案路徑					 */
  /* --------------------------------------------------- */

  dir = xo->dir;
  fname = vch_fpath(fpath, dir, vch);

  /* --------------------------------------------------- */
  /* 確認進入賭盤					 */
  /* --------------------------------------------------- */

  if (vans("是否參加賭盤(Y/N)？[N] ") != 'y')
    return XO_FOOT;

  /* --------------------------------------------------- */
  /* 開始投票，顯示投票說明				 */
  /* --------------------------------------------------- */

  *fname = '@';
  more(fpath, MFST_NORMAL);

  /* --------------------------------------------------- */
  /* 載入賭盤選項檔					 */
  /* --------------------------------------------------- */

  *fname = 'I';
  if ((fd = open(fpath, O_RDONLY)) < 0)
  {
    vmsg("無法讀取賭盤選項檔");
    return vote_head(xo);
  }
  count = read(fd, vlist, sizeof(vlist)) / sizeof(vitem_t);
  close(fd);

  for (fd = 0; fd < count; fd++)
    slist[fd] = (char *) &vlist[fd];

  /* --------------------------------------------------- */
  /* 進行賭盤						 */
  /* --------------------------------------------------- */
  
  vs_bar("賭盤下注");

  move(1, 0);
  outs("    1.賭盤由該板板主負責舉辦，並決定開獎時間與開獎結果。\n");
  outs("    2.系統會從總賭金中抽取 5% 稅金，其中的 20% 分給舉辦者，但最多 800 銀幣。\n");
  outs("    3.開獎時只會有一種彩券中獎。中獎者依該項的總購買張數均分抽稅後的總賭金。\n");
  outs("      每張獎券無條件捨去小數點後的金額，其差值歸為系統抽稅。\n");
  outs("    4.下注者請自行承擔系統異常導致的損失所帶來的風險。原則上站方不予賠償。\n\n");

  for (i = 0; i < count; i++)
  {
    move(i % 11 + 7, (i < 11)? 0:40);
    prints("%s %c %s", "　", radix32[i], slist[i]);
  }

  move(20, 0);
  prints("下注截止時間：%s", Btime(&vch->vclose));

  for (;;)
  {
    choice = 0;

    k = vans("要下注哪一項(不輸入則取消)？ ") - '0';
    if (k >= 10)
      k -= 'a' - '0' - 10;

    if (k >= 0 && k < count)
    {
      choice ^= (1 << k);
      move(k % 11 + 7, (k < 11)? 0:40);
      outs("ˇ");
    }
    else
      return vote_head(xo);

    sprintf(buf, "每張 %d 銀幣，要買幾張？[1] ", vch->price);
    vget(b_lines, 0, buf, ans, 3, DOECHO);	/* 最多買 99 張，避免溢位 */

    i = atoi(ans);
    if (i < 1)
      i = 1;
    if (vch->price * i > cuser.money)
      i = cuser.money / vch->price;

    fd = i * vch->price;	/* fd 是要付的賭金 */

    sprintf(buf, "要買 %d 張的 [%c] 嗎？ (Y)確定 (N)重來 (Q)取消？[N] ", i, radix32[k]);
    k = i;
    i = vans(buf);

    if (i == 'q')
      return vote_head(xo);

    if (i == 'y')
    {
      if (time(0) > vch->vclose)	/* 因為有vget()，所以還要再檢查一次 */
      {
        vmsg("賭盤已停止下注，請靜候開獎");
        return vote_head(xo);
      }
      break;
    }

    for (i = 0; i < count; i++)
    {
      move(i % 11 + 7, (i < 11)? 0:40);
      outs("　");
    }
  }

  /* --------------------------------------------------- */
  /* 記錄結果						 */
  /* --------------------------------------------------- */

  /* 確定投票尚未截止 */
  /* itoc.050514: 因為板主可以改變開票時間，為了避免使用者會龜在 vget() 或是
     利用 xo_pool[] 未同步來規避 time(0) > vclose 的檢查，所以就得重新載入 VCH */
  if (rec_get(dir, &vbuf, sizeof(VCH), xo->pos) || vch->chrono != vch->chrono || time(0) > vbuf.vclose)
  {
    vmsg("賭盤已停止下注，請靜候開獎");
    return vote_init(xo);
  }

  cuser.money -= fd;	/* fd 是要付的賭金 */

  /* 加入記錄檔 */
  memset(&vlog, 0, sizeof(VLOG));
  strcpy(vlog.userid, cuser.userid);
  vlog.numvotes = k;
  vlog.choice = choice;
  *fname = 'G';
  rec_add(fpath, &vlog, sizeof(VLOG));

  vmsg("下注完成！");

  return vote_head(xo);
}


static int
vote_join(xo)
  XO *xo;
{
  VCH *vch;

  vch = (VCH *) xo_pool + (xo->pos - xo->top);


  /* dust.090430: 參加投票者必須要有發文權限 */
  if(vch->vgamble == ' ' && !(bbstate & STAT_POST))
  {
    vmsg("您在此板無法發言，不能參加投票");
    return XO_FOOT;
  }

  /* --------------------------------------------------- */
  /* 檢查是否已經結束					 */
  /* --------------------------------------------------- */

  if (time(0) > vch->vclose)
  {
    if(vch->vgamble == ' ')
      vmsg("投票已經截止了，請靜候開票");
    else
      vmsg("賭盤已停止下注，請靜候開獎");
    return XO_FOOT;
  }

  /* dust.090426: 分離參與投票與參與賭盤的處理函數 */

  if(vch->vgamble == ' ')
    return vote_chos(vch, xo);
  else
    return gamble_chos(vch, xo);
}



struct Tchoice
{
  int count;
  vitem_t vitem;
};

static int
TchoiceCompare(i, j)
  struct Tchoice *i, *j;
{
  return j->count - i->count;
}


static char *		/* NULL:建立檔案失敗(還沒有人投票) */
draw_vote(fpath, folder, vch, preview)	/* itoc.030906: 投票結果 (與 account.c:draw_vote() 格式相同) */
  char *fpath;
  char *folder;
  VCH *vch;
  int preview;		/* 1:預覽  0:開票 */
{
  struct Tchoice choice[MAX_CHOICES];
  FILE *fp;
  char *fname;
  int total, items, num, fd, ticket, bollt;
  VLOG vlog;

  fname = vch_fpath(fpath, folder, vch);

  /* vote item */

  *fname = 'I';

  items = 0;
  if (fp = fopen(fpath, "r"))
  {
    while (fread(&choice[items].vitem, sizeof(vitem_t), 1, fp) == 1)
    {
      choice[items].count = 0;
      items++;
    }
    fclose(fp);
  }

  if (items == 0)
    return NULL;

  /* 累計投票結果 */

  *fname = 'G';
  bollt = 0;		/* Thor: 總票數歸零 */
  total = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
    {
      for (ticket = vlog.choice, num = 0; ticket && num < items; ticket >>= 1, num++)
      {
	if (ticket & 1)
	{
	  choice[num].count += vlog.numvotes;
	  bollt += vlog.numvotes;
	}
      }
      total++;
    }
    close(fd);
  }

  /* 產生開票結果 */

  *fname = 'Z';
  if (!(fp = fopen(fpath, "w")))
    return NULL;

  fprintf(fp, "\033[1;32m◆[%s] 看板投票：%s\033[m\n\n舉辦人：%s\n\n", currboard, vch->title, vch->owner);
  fprintf(fp, "投票舉辦日期：%s\n\n", Btime(&vch->chrono));
  fprintf(fp, "投票結束時間：%s\n\n", Btime(&vch->vclose));
  if(!preview)
    fprintf(fp, "投票開票時間：%s\n\n", Now());

  fputs("\033[1;32m◆ 投票說明：\033[m\n\n", fp);

  *fname = '@';
  f_suck(fp, fpath);

  fputs("\033[m", fp);	/* dust.090426: 加入ANSI還原碼 */

  fprintf(fp, "\n\n\033[1;32m◆ 投票結果：每人可投 %d 票，共 %d 人參加，投出 %d 票\033[m\n\n",
    vch->maxblt, total, bollt);

  if (vch->vsort == 's')
    qsort(choice, items, sizeof(struct Tchoice), TchoiceCompare);

  if (vch->vpercent == '%')
    fd = BMAX(1, bollt);	/* 避免算比率時，分母為零 */
  else
    fd = 0;

  for (num = 0; num < items; num++)
  {
    ticket = choice[num].count;
    if (fd)
      fprintf(fp, "    %-36s%5d 票 (%4.1f%%)\n", &choice[num].vitem, ticket, 100.0 * ticket / fd);
    else
      fprintf(fp, "    %-36s%5d 票\n", &choice[num].vitem, ticket);
  }


  /* User's Comments */

  *fname = 'O';
  fputs("\n\n\033[1;32m◆ 我有話要說：\033[m\n\n", fp);
  f_suck(fp, fpath);
  fputs("\n\n\n\n", fp);
  fclose(fp);

  /* 最後傳回的 fpath 即為投票結果檔 */
  *fname = 'Z';
  return fname;
}


static char *		/* NULL:建立檔案失敗(還沒有人投票) */
draw_gamble(fpath, folder, vch, preview)	/* dust.090427: 賭盤結果繪製 */
  char *fpath;
  char *folder;
  VCH *vch;
  int preview;		/* 1:預覽  0:開盤 */
{
  struct Tchoice choice[MAX_CHOICES];
  FILE *fp;
  char *fname;
  int total, items, num, fd, ticket, bollt;
  VLOG vlog;

  fname = vch_fpath(fpath, folder, vch);

  /* 計算有幾個選項 */
  *fname = 'I';

  items = 0;
  if (fp = fopen(fpath, "r"))
  {
    while (fread(&choice[items].vitem, sizeof(vitem_t), 1, fp) == 1)
    {
      choice[items].count = 0;
      items++;
    }
    fclose(fp);
  }

  if (items == 0)
    return NULL;

  /* 累計投票結果 */

  *fname = 'G';
  bollt = 0;		/* Thor: 總票數歸零 */
  total = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
    {
      for (ticket = vlog.choice, num = 0; ticket && num < items; ticket >>= 1, num++)
      {
	if (ticket & 1)
	{
	  choice[num].count += vlog.numvotes;
	  bollt += vlog.numvotes;
	}
      }
      total++;
    }
    close(fd);
  }

  /* 產生開票結果檔 */

  *fname = 'Z';
  if (!(fp = fopen(fpath, "w")))
    return NULL;


  if(preview)
  {
    fprintf(fp, "◆ 賭盤主題：%s\n\n\033[1m◆ 下注截止時間：%s\033[m\n\n", vch->title, Btime(&vch->vclose));
    fputs("\033[1;32m◆ 賭盤說明：\033[m\n\n", fp);
  }
  else
  {
    fprintf(fp, "賭盤主題：%s\n\n舉辦時間：%s\n", vch->title, Btime(&vch->chrono));
    fprintf(fp, "截止時間：%s\n\n賭盤說明：\n\n", Btime(&vch->vclose));
  }

  *fname = '@';
  f_suck(fp, fpath);

  fputs("\033[m", fp);	/* dust.090426: 加入ANSI還原碼 */

  if(preview)
    fprintf(fp, "\n\n\033[1;33m◆ 下注狀況：\033[m\n\n");
  else
    fprintf(fp, "\n\n下注狀況：\n\n");


  fd = BMAX(1, bollt);	/* 避免算比率時，分母為零 */

  for (num = 0; num < items; num++)
  {
    ticket = choice[num].count;
    if(preview)
      fprintf(fp, "    %-36s%5d 張 (%4.1f%%) 賠率 1:%.3f\n", &choice[num].vitem, ticket, 100.0 * ticket / fd, 0.95 * (bollt + 1) / (ticket + 1));
    else
      fprintf(fp, "    %-36s%5d 張 (%5.2f%%)\n", &choice[num].vitem, ticket, 100.0 * ticket / fd);
  }

  if(preview)
    fprintf(fp, "\n%s\n   總下注數量：\033[1m%d\033[m 張，每張 \033[1m%d\033[m 銀幣。\n\n\n\n", msg_seperator, bollt, vch->price);

  fclose(fp);

  /* 最後傳回的 fpath 即為投票結果檔 */
  *fname = 'Z';
  return fname;
}


static int
vote_view(xo)
  XO *xo;
{
  char fpath[64];
  VCH *vch;

  vch = (VCH *) xo_pool + (xo->pos - xo->top);

  if ((bbstate & STAT_BOARD) && vch->vgamble == ' ')
  {
    if (draw_vote(fpath, xo->dir, vch, 1))
    {
      more(fpath, MFST_NORMAL);
      unlink(fpath);
      return vote_head(xo);
    }

    vmsg("目前尚未有人投票");
    return XO_FOOT;
  }
  else if(vch->vgamble == '$')
  {
    if (draw_gamble(fpath, xo->dir, vch, 1))
    {
      more(fpath, MFST_NORMAL);
      unlink(fpath);
      return vote_head(xo);
    }

    vmsg("目前尚未有人下注");
    return XO_FOOT;
  }

  return XO_NONE;
}



static void
keeplogV(fnlog, board, title, eb)	/* 賭盤專用的keeplog() */
  char *fnlog;
  char *board;
  char *title;
  int eb;		/* 0:不加上站簽  1:加上站簽 */
{
  HDR hdr;
  char folder[64], fpath[64];
  FILE *fp;

  if (!dashf(fnlog))	/* Kudo.010804: 檔案是空的就不 keeplog */
    return;

  brd_fpath(folder, board, fn_dir);

  if (fp = fdopen(hdr_stamp(folder, 'A', &hdr, fpath), "w"))
  {
    extern char *ebs[EDIT_BANNER_NUM];

    fprintf(fp, "作者: %s (%s)\n標題: %s\n時間: %s\n\n",
      str_sysop, "自動模式", title, Btime(&hdr.chrono));
    f_suck(fp, fnlog);
    if(eb)
      fprintf(fp, ebs[time(0) % EDIT_BANNER_NUM], "sysop (自動模式)", "極光本機");
    fclose(fp);

    strcpy(hdr.title, title);
    strcpy(hdr.owner, str_sysop);
    rec_bot(folder, &hdr, sizeof(HDR));

    btime_update(brd_bno(board));
  }
}


static int	/* 0:成功  1:異常中止 */
vlog_pay(fpath, choice, fp, vch, comment, answer)	/* 付錢給押對的使用者 */
  char *fpath;			/* 記錄檔路徑 */
  usint choice;			/* 正確的答案 */
  FILE *fp;			/* 寫入的檔案 */
  VCH *vch;
  char *comment;		/* 板主註解 */
  char *answer;			/* 結果選項(字串) */
{
  int correct, bollt;		/* 押對/全部 的票數 */
  int single;			/* 每張中獎的彩券可獲得的銀幣 */
  int commission, tax;		/* 板主抽成金額/系統抽稅金額 */
  int fd, money;
  char buf[64];
  VLOG vlog;
  PAYCHECK paycheck;
  FILE *fpl;			/* dust.090508: 記錄所有下注者用 */
#define GAMBLE_LOG "tmp/gamble_log.tmp"


/* dust.090426: 這個寫法完全沒有考慮到int溢位的問題......
                就假設不會發生吧(?) */

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    /* 第一次迴圈(算出賠率等統計資料) */
    correct = bollt = 0;
    while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
    {
      bollt += vlog.numvotes;
      if (vlog.choice == choice)
	correct += vlog.numvotes;
    }

    /* 給舉辦者抽頭 1%(上限800銀) */
    commission = (int) (vch->price / (double)100 * bollt);
    if(commission > 800)
      commission = 800;

    if(correct > 0)
      single = (double)vch->price * 0.95 * bollt / correct;
    else
      single = 0;

    tax = bollt * vch->price - correct * single - commission;

    memset(&paycheck, 0, sizeof(PAYCHECK));
    if(commission > 0)		/* 抽頭大於零時才有必要寄支票 */
    {
      time(&paycheck.tissue);
      paycheck.money = commission;
      sprintf(paycheck.reason, "[抽頭] %s板", currboard);
      usr_fpath(buf, vch->owner, FN_PAYCHECK);
      rec_add(buf, &paycheck, sizeof(PAYCHECK));
    }

    /* 印出統計資料 */
    fputs("\n\n", fp);
    fprintf(fp, "開獎時間：%s，由 %s 開獎。\n\n", Now(), cuser.userid);
    fprintf(fp, "開獎結果：%s\n\n", answer);

    if(comment[0])	/* 有寫註解才輸出 */
      fprintf(fp, "板主註解：%s\n\n", comment);

    fprintf(fp, "中獎比例：%d張/%d張 (%.2f%%)\n", correct, bollt, correct * 100.0 / bollt);
    fprintf(fp, "總下注金額為 %d 銀幣，系統抽稅 %d 銀幣 (%.2f%%)\n", bollt * vch->price, tax, (double)tax * 100 / vch->price / bollt);
    if(commission > 0)
      fprintf(fp, "舉辦者 %s 抽頭，得到 %d 銀幣 (%.2f%%)\n", vch->owner, commission, (double)commission * 100 / vch->price / bollt);

    if(correct > 0)	/* 有人中獎才寫入 */
    {
      fprintf(fp, "\n每張中獎彩券可得 %d 銀幣，賠率 1:%.3f\n", single, 0.95 * bollt / correct);
      fputs("\n\n以下是中獎名單：\n\n", fp);
    }

    fpl = fopen(GAMBLE_LOG, "w");
    if(fpl)
    {
      fprintf(fpl, "賭盤主題：%s\n", vch->title);
      fprintf(fpl, "開獎時間：%s，由 %s 開獎。\n\n", Now(), cuser.userid);
    }

    /* 第二次迴圈(進行發錢、記錄中獎者與記錄下注紀錄) */
    lseek(fd, (off_t) 0, SEEK_SET);
    time(&paycheck.tissue);
    while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
    {
      if(fpl)
	fprintf(fpl, "%s %08X %d\n", vlog.userid, vlog.choice, vlog.numvotes);
      /* dust.090508: 以十六進位整數表示choice，這樣比較省事(?) */

      if(correct > 0)	/* 有人中獎才檢查 */
      {
	if (vlog.choice == choice)
	{
	  money = single * vlog.numvotes;
	  fprintf(fp, "    恭喜 %12s 買了 %2d 張，共可獲得 %6d 銀幣\n", vlog.userid, vlog.numvotes, money);

	  paycheck.money = money;
	  sprintf(paycheck.reason, "[賭盤中獎] %s板", currboard);
	  usr_fpath(buf, vlog.userid, FN_PAYCHECK);
	  rec_add(buf, &paycheck, sizeof(PAYCHECK));
	}
      }
    }

    if(fpl)
    {
      fclose(fpl);
      sprintf(buf, "[紀錄] %s板-賭盤下注者清單", currboard);
      keeplogV(GAMBLE_LOG, BN_SECURITY, buf, 0);
      unlink(GAMBLE_LOG);
    }

    close(fd);
  }
  else
    return 1;

  return 0;
}


static int
voting_open(xo, vch, pos)	/* dust.090427: 開票專用函數 */
  XO *xo;
  VCH *vch;
  int pos;
{
  char *dir, *fname, fpath[64], tmpfpath[64], buf[80];
  FILE *fp;

  dir = xo->dir;

  /* 建立投票結果檔 */

  if (!(fname = draw_vote(fpath, dir, vch, 0)))
  {
    vmsg("目前尚未有人投票");
    return XO_FOOT;
  }

  /* 將開票結果 post 到 [BN_RECORD] 與 本看板 */

  if (!(currbattr & BRD_NOVOTE))
  {
    sprintf(buf, "[記錄] %s <<看板選情報導>>", currboard);
    keeplogV(fpath, BN_RECORD, buf, 0);
  }

  keeplogV(fpath, currboard, "[記錄] 選情報導", 1);


  /* 投票結果附加到 @vote */

  setdirpath(tmpfpath, dir, "@/@vote.tmp");
  setdirpath(buf, dir, "@/@vote");
  if (fp = fopen(tmpfpath, "w"))
  {
    fprintf(fp, "\n\033[1;34m%s\033[m\n\n", msg_seperator);
    f_suck(fp, fpath);	/* 接上結果檔 */
    f_suck(fp, buf);	/* 接上@vote */
    fclose(fp);
    rename(tmpfpath, buf);
  }

  delvch(xo, vch);	/* 開完票就刪除 */

  currchrono = vch->chrono;
  rec_del(dir, sizeof(VCH), pos, cmpchrono);

  vmsg("開票完畢");
  return vote_init(xo);
}


static int
gambling_open(xo, vch, pos)	/* dust.090427: 賭盤開獎專用函數 */
  XO *xo;
  VCH *vch;
  int pos;
{
  int fd, count, i;
  char *dir, *fname, fpath[64], buf[80];
  usint choice;
  char *slist[MAX_CHOICES];
  vitem_t vlist[MAX_CHOICES];
  FILE *fp;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  vch = (VCH *) xo_pool + (pos - xo->top);

  if (time(NULL) < vch->vclose)
  {
    /* dust.090426: 賭盤不能提早開 */
    if(vch->vgamble == '$')
      return XO_NONE;

    if (vans("尚未到原定開票時間，確定要提早開票(Y/N)？[N] ") != 'y')
      return XO_FOOT;
  }

  dir = xo->dir;

  /* 建立投票結果檔 */

  if (!(fname = draw_gamble(fpath, dir, vch, 0)))
  {
    vmsg("尚未有人參加賭盤");
    return XO_FOOT;
  }


  /* 載入賭盤選項檔 */

  *fname = 'I';
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    count = read(fd, vlist, sizeof(vlist)) / sizeof(vitem_t);
    close(fd);

    for (fd = 0; fd < count; fd++)
    {
      slist[fd] = (char *) &vlist[fd];
    }

    /* 板主公佈賭盤結果 */
    choice = 0;
    vs_bar("賭盤開獎");

    for (;;)
    {
      choice = bitset(choice, count, 1, "請選擇賭盤的開獎結果：", slist);
      if(choice == 0)
      {				/* 不輸入答案的話則直接離開 */
	*fname = 'Z';
	unlink(fpath);
	return vote_head(xo);
      }

      vget(b_lines, 0, "請輸入註解：", buf, 60, DOECHO);

      fd = vans("賭盤開獎 (Y)確定 (N)重來 (Q)取消？[N] ");

      if (fd == 'q')
      {
	*fname = 'Z';
	unlink(fpath);
	return vote_head(xo);
      }

      if (fd == 'y')
	break;
    }

    *fname = 'Z';
    if (fp = fopen(fpath, "a"))
    {
      /* 開始發錢 & 紀錄結果。若 return 1 則代表發生異常 */

      for(i=0 ; i < count ; i++)
      {
	if(choice & (1 << i))
	  break;
      }
      if(i >= count) i = count - 1;

      *fname = 'G';
      if(vlog_pay(fpath, choice, fp, vch, buf, slist[i]))
      {
	vmsg("無法載入下注紀錄！");
	*fname = 'Z';
	unlink(fpath);
	return vote_head(xo);
      }

      fputs("\n\n\n", fp);
      fclose(fp);
    }

    /* 開票結果 */
    *fname = 'Z';
  }
  else
  {
    vmsg("無法載入賭盤選項檔");
    *fname = 'Z';
    unlink(fpath);
    return vote_head(xo);
  }


  /* 將開獎結果 post 到 [BN_RECORD] 與 本看板 */

  if (!(currbattr & BRD_NOVOTE))
  {
    sprintf(buf, "[記錄] %s <<看板賭盤紀錄>>", currboard);
    keeplogV(fpath, BN_RECORD, buf, 0);
  }

  keeplogV(fpath, currboard, "[紀錄] 賭盤開獎", 1);


  /* 開獎完就刪除 */
  vch->vgamble = ' ';	/* 令為非賭盤，如此在 delvch 裡面就不會退賭金 */
  delvch(xo, vch);

  currchrono = vch->chrono;
  rec_del(dir, sizeof(VCH), pos, cmpchrono);

  vmsg("開獎完畢");
  return vote_init(xo);
}


static int
vote_open(xo)		/* dust.090427: 投票開票/賭盤開獎 */
  XO *xo;
{
  int pos;
  VCH *vch;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  vch = (VCH *) xo_pool + (pos - xo->top);

  if (time(NULL) < vch->vclose)
  {
    /* dust.090426: 賭盤不能提早開 */
    if(vch->vgamble == '$')
      return XO_NONE;

    if (vans("尚未到原定開票時間，確定要提早開票(Y/N)？[N] ") != 'y')
      return XO_FOOT;
  }

  /* dust.090426: 投票開票和賭盤開獎的處理函數分離 */
  if(vch->vgamble == ' ')
    return voting_open(xo, vch, pos);
  else
    return gambling_open(xo, vch, pos);
}


static int
vote_tag(xo)
  XO *xo;
{
  VCH *vch;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  vch = (VCH *) xo_pool + cur;

  if (tag = Tagger(vch->chrono, pos, TAG_TOGGLE))
  {
    move(3 + cur, 6);
    outc(tag > 0 ? '*' : ' ');
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE;	/* lkchu.981201: 跳至下一項 */
}


static int
vote_help(xo)
  XO *xo;
{
  xo_help("vote");
  return vote_head(xo);
}


static KeyFunc vote_cb[] =
{
  XO_INIT, vote_init,
  XO_LOAD, vote_load,
  XO_HEAD, vote_head,
  XO_BODY, vote_body,

  'r', vote_join,	/* itoc.010901: 按右鍵比較方便 */
  'v', vote_join,
  'R', vote_result,

  'V', vote_view,
  'E', vote_edit,
  'o', vote_pal,
  'd', vote_delete,
  'D', vote_rangedel,
  't', vote_tag,
  'b', vote_open,

  Ctrl('D'), vote_prune,
  Ctrl('G'), vote_pal,
  Ctrl('P'), vote_add,
  Ctrl('Q'), vote_query,

  'h', vote_help
};


int
XoVote(xo)
  XO *xo;
{
  char fpath[64];

  /* dust.090430: guest無法投票或參加賭盤 */
  if (!cuser.userlevel)
    return XO_NONE;

  setdirpath(fpath, xo->dir, FN_VCH);
  if (!(bbstate & STAT_BOARD) && !rec_num(fpath, sizeof(VCH)))
  {
    vmsg("目前沒有投票舉行");
    return XO_FOOT;
  }

  xz[XZ_VOTE - XO_ZONE].xo = xo = xo_new(fpath);
  xz[XZ_VOTE - XO_ZONE].cb = vote_cb;
  xover(XZ_VOTE);
  free(xo);

  return XO_INIT;
}


#if 0	/* dust.090429: 因為現在的設計與舊版不相容的緣故，暫時拿掉 */
int
vote_all()		/* itoc.010414: 投票中心 */
{
  typedef struct
  {
    char brdname[BNLEN + 1];
    char class[BCLEN + 1];
    char title[BTLEN + 1];
    char BM[BMLEN + 1];
    char bvote;
  } vbrd_t;

  extern char brd_bits[];
  char *str;
  char fpath[64];
  int num, pageno, pagemax, redraw;
  int ch, cur;
  BRD *bhead, *btail;
  XO *xo;
  vbrd_t vbrd[MAXBOARD], *vb;

  bhead = bshm->bcache;
  btail = bhead + bshm->number;
  cur = 0;
  num = 0;

  do
  {
    str = &brd_bits[cur];
    ch = *str;
    if (bhead->bvote && (ch & BRD_W_BIT))
    {
      vb = vbrd + num;
      strcpy(vb->brdname, bhead->brdname);
      strcpy(vb->class, bhead->class);
      strcpy(vb->title, bhead->title);
      strcpy(vb->BM, bhead->BM);
      vb->bvote = bhead->bvote;
      num++;
    }
    cur++;
  } while (++bhead < btail);

  if (!num)
  {
    vmsg("目前站內並沒有任何投票");
    return XEASY;
  }

  num--;
  pagemax = num / XO_TALL;
  pageno = 0;
  cur = 0;
  redraw = 1;

  do
  {
    if (redraw)
    {
      /* itoc.註解: 盡量做得像 xover 格式 */
      vs_head("投票中心", str_site);
      prints(NECKER_VOTEALL, d_cols >> 1, "", d_cols - (d_cols >> 1), "");

      redraw = pageno * XO_TALL;	/* 借用 redraw */
      ch = BMIN(num, redraw + XO_TALL - 1);
      move(3, 0);
      do
      {
	vb = vbrd + redraw;
	/* itoc.010909: 板名太長的刪掉、加分類顏色。假設 BCLEN = 4 */
	prints("%6d   %-13s\033[1;3%dm%-5s\033[m%s %-*.*s %.*s\n",
	  redraw + 1, vb->brdname,
	  vb->class[3] & 7, vb->class,
	  vb->bvote > 0 ? ICON_VOTED_BRD : ICON_GAMBLED_BRD,
	  (d_cols >> 1) + 34, (d_cols >> 1) + 33, vb->title, d_cols - (d_cols >> 1) + 13, vb->BM);

	redraw++;
      } while (redraw <= ch);

      outf(FEETER_VOTEALL);
      move(3 + cur, 0);
      outc('>');
      redraw = 0;
    }

    switch (ch = vkey())
    {
    case KEY_RIGHT:
    case '\n':
    case ' ':
    case 'r':
      vb = vbrd + (cur + pageno * XO_TALL);

      /* itoc.060324: 等同進入新的看板，XoPost() 有做的事，這裡幾乎都要做 */
      if (!vb->brdname[0])	/* 已刪除的看板 */
	break;

      redraw = brd_bno(vb->brdname);	/* 借用 redraw */
      if (currbno != redraw)
      {
	ch = brd_bits[redraw];

	/* 處理權限 */
	if (ch & BRD_M_BIT)
	  bbstate |= (STAT_BM | STAT_BOARD | STAT_POST);
	else if (ch & BRD_X_BIT)
	  bbstate |= (STAT_BOARD | STAT_POST);
	else if (ch & BRD_W_BIT)
	  bbstate |= STAT_POST;

	mantime_add(currbno, redraw);

	currbno = redraw;
	bhead = bshm->bcache + currbno;
	currbattr = bhead->battr;
	strcpy(currboard, bhead->brdname);
	str = bhead->BM;
	sprintf(currBM, "板主：%s", *str <= ' ' ? "徵求中" : str);
#ifdef HAVE_BRDMATE
	strcpy(cutmp->reading, currboard);
#endif

	brd_fpath(fpath, currboard, fn_dir);
#ifdef AUTO_JUMPPOST
	xz[XZ_POST - XO_ZONE].xo = xo = xo_get_post(fpath, bhead);	/* itoc.010910: 為 XoPost 量身打造一支 xo_get() */
#else
	xz[XZ_POST - XO_ZONE].xo = xo = xo_get(fpath);
#endif
	xo->key = XZ_POST;
	xo->xyz = bhead->title;
      }

      sprintf(fpath, "brd/%s/%s", currboard, FN_VCH);
      xz[XZ_VOTE - XO_ZONE].xo = xo = xo_new(fpath);
      xz[XZ_VOTE - XO_ZONE].cb = vote_cb;
      xover(XZ_VOTE);
      free(xo);
      redraw = 1;
      break;

    default:
      ch = xo_cursor(ch, pagemax, num, &pageno, &cur, &redraw);
      break;
    }
  } while (ch != 'q');

  return 0;
}
#endif

