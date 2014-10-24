/*-------------------------------------------------------*/
/* newbrd.c   ( YZU_CSE WindTop BBS )                    */
/*-------------------------------------------------------*/
/* target : 連署功能    			 	 */
/* create : 00/01/02				 	 */
/* update : 10/10/03				 	 */
/*-------------------------------------------------------*/
/* run/newbrd/_/.DIR - newbrd control header		 */
/* run/newbrd/_/@/@_ - newbrd description file		 */
/* run/newbrd/_/@/G_ - newbrd voted id loG file		 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef HAVE_COSIGN

extern XZ xz[];
extern char xo_pool[];
extern BCACHE *bshm;		/* itoc.010805: 開新板用 */

static int nbrd_add();

static char *split_line = "\033[m\033[33m──────────────────────────────\033[m\n";


typedef struct
{
  char userid[IDLEN + 1];
  char email[60];
}      LOG;



static int
cmpbtime(nbrd)
  NBRD *nbrd;
{
  return nbrd->btime == currchrono;
}



static void
nbrd_fpath(fpath, folder, nbrd)
  char *fpath;
  char *folder;
  NBRD *nbrd;
{
  char *str;
  int cc;

  while (cc = *folder++)
  {
    *fpath++ = cc;
    if (cc == '/')
      str = fpath;
  }
  strcpy(str, nbrd->xname);
}



static int
nbrd_body(xo)
  XO *xo;
{
  NBRD *nbrd;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    if (vans("要新增連署項目嗎(Y/N)？[N] ") == 'y')
      return nbrd_add(xo);
    return XO_QUIT;
  }

  nbrd = (NBRD *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;

  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    prints("%6d ", ++num);

    if(nbrd->mode & NBRD_FINISH)
      outc(' ');
    else if(nbrd->mode & NBRD_END)	/* 連署完成 */
      outc('-');
    else if(nbrd->mode & NBRD_EXPIRE)	/* 連署過期 */
      outc('x');
    else				/* 連署中 */
      outc('+');

    prints(" %-5s %-13s [%s] %.*s\n", nbrd->date + 3, nbrd->owner, nbrd->brdname, d_cols + 20, nbrd->title);
    nbrd++;

  }while(num < max);
  clrtobot();

  return XO_FOOT;
}


static int
nbrd_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(NBRD));
  return nbrd_body(xo);
}



static int
nbrd_head(xo)
  XO *xo;
{
  vs_head("看板連署", str_site);
  prints(NECKER_COSIGN, d_cols, "");
  return nbrd_body(xo);
}


static int
nbrd_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(NBRD));
  return nbrd_head(xo);
}



static int
nbrd_find(fpath, brdname)
  char *fpath, *brdname;
{
  NBRD old;
  int fd;
  int rc = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(NBRD)) == sizeof(NBRD))
    {
      if (!str_cmp(old.brdname, brdname) && !(old.mode & NBRD_FINISH))
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
nbrd_stamp(folder, nbrd, fpath)
  char *folder;
  NBRD *nbrd;
  char *fpath;
{
  char *fname;
  char *family = NULL;
  int rc;
  time_t token;

  fname = fpath;
  while (rc = *folder++)
  {
    *fname++ = rc;
    if (rc == '/')
      family = fname;
  }

  fname = family;
  *family++ = '@';

  token = time(0);

  archiv32(token, family);

  nbrd->btime = token;
  str_stamp(nbrd->date, &nbrd->btime);
  strcpy(nbrd->xname, fname);

  rc = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600);
  if(rc >= 0)
  {
    close(rc);
    return 1;
  }
  else
    return 0;
}


static int
nbrd_add(xo)
  XO *xo;
{
  char fpath[64], path[64];
  FILE *fp;
  NBRD nbrd;
  const int rv_noact = xo->max > 0? XO_FOOT : XO_QUIT;


  if(!HAS_PERM(PERM_POST))
  {
    vmsg("你沒有權限發起連署");
    return rv_noact;
  }

  memset(&nbrd, 0, sizeof(NBRD));

  if (!vget(b_lines, 0, "英文板名：", nbrd.brdname, sizeof(nbrd.brdname), DOECHO))
    return XO_FOOT;

  if(!valid_brdname(nbrd.brdname))
  {
    vmsg("板名不合法");
    return rv_noact;
  }
  else if(brd_bno(nbrd.brdname) >= 0)
  {
    vmsg("已有此板");
    return rv_noact;
  }
  else if(nbrd_find(xo->dir, nbrd.brdname))
  {
    vmsg("此板名正在連署中");
    return XO_FOOT;
  }

  if(!vget(b_lines, 0, "看板主題：", nbrd.title, sizeof(nbrd.title), DOECHO) ||
    !vget(b_lines, 0, "看板分類：", nbrd.class, sizeof(nbrd.class), DOECHO))
    return rv_noact;

#ifdef SYSOP_START_COSIGN
  nbrd.mode = NBRD_NEWBOARD;
#else
  nbrd.mode = NBRD_NEWBOARD | NBRD_START;
#endif

  vmsg("開始編輯連署說明");
  sprintf(path, "tmp/%s.nbrd", cuser.userid);	/* 連署原因的暫存檔案 */
  if(vedit(path, 0) < 0)
  {
    unlink(path);
    vmsg(msg_cancel);
    return nbrd_head(xo);
  }

  if(!nbrd_stamp(xo->dir, &nbrd, fpath))
    return nbrd_head(xo);

  nbrd.etime = nbrd.btime + NBRD_DAY_BRD * 86400;
  nbrd.total = NBRD_NUM_BRD;
  strcpy(nbrd.owner, cuser.userid);

  fp = fopen(fpath, "a");
  fprintf(fp, "作者: %s (%s) 站內: 連署系統\n", cuser.userid, cuser.username);
  fprintf(fp, "標題: [%s] %s\n", nbrd.brdname, nbrd.title);
  fprintf(fp, "時間: %s\n\n", Btime(&nbrd.btime));

  fprintf(fp, "英文板名：%s\n", nbrd.brdname);
  fprintf(fp, "看板分類：%s\n", nbrd.class);
  fprintf(fp, "看板主題：%s\n", nbrd.title);
  fprintf(fp, "板主名稱：%s\n", cuser.userid);
  fprintf(fp, "舉辦日期：%s\n", nbrd.date);
  fprintf(fp, "到期天數：%d\n", NBRD_DAY_BRD);
  fprintf(fp, "需連署人：%d\n", NBRD_NUM_BRD);
  fprintf(fp, split_line);
  fprintf(fp, "連署說明：\n");
  f_suck(fp, path);
  unlink(path);
  fprintf(fp, split_line);
  fclose(fp);

  rec_add(xo->dir, &nbrd, sizeof(NBRD));

  vmsg("連署開始了！");
  return nbrd_init(xo);
}



static int
nbrd_seek(fpath)
  char *fpath;
{
  LOG old;
  int fd;
  int rc = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(LOG)) == sizeof(LOG))
    {
      if (!strcmp(old.userid, cuser.userid) || !str_cmp(old.email, cuser.email))
      {
	rc = 1;
	break;
      }
    }
    close(fd);
  }
  return rc;
}


static void
addreply(hdd, ram)
  NBRD *hdd, *ram;
{
  if (--hdd->total <= 0)
    hdd->mode |= NBRD_END;

  if(hdd->total < 0)
    ram->total = 0;
  else
    ram->total = hdd->total + 1;
}


static int
nbrd_reply(xo)
  XO *xo;
{
  NBRD *nbrd;
  char *fname, fpath[64], reason[80];
  LOG rec;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
  fname = NULL;

  if (nbrd->mode & (NBRD_FINISH | NBRD_END))
    return XO_NONE;

  if (time(0) >= nbrd->etime)
  {
    /* 更新硬碟裡的紀錄 */
    if(!(nbrd->mode & NBRD_END))
    {
      nbrd->mode |= NBRD_END;
      currchrono = nbrd->btime;
      rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);
    }

    vmsg("連署已經截止了");
    return XO_FOOT;
  }


  /* --------------------------------------------------- */
  /* 檢查是否已經連署過					 */
  /* --------------------------------------------------- */

  nbrd_fpath(fpath, xo->dir, nbrd);
  fname = strrchr(fpath, '@');
  *fname = 'G';

  if (nbrd_seek(fpath))
  {
    vmsg("您已經連署過了！");
    return XO_FOOT;
  }

  /* --------------------------------------------------- */
  /* 開始連署						 */
  /* --------------------------------------------------- */

  *fname = '@';

  if (vans("要加入連署嗎(Y/N)？[N] ") == 'y')
  {
    FILE *fp;

    if(!vget(b_lines, 0, "我有話要說：", reason, 65, DOECHO))
      return XO_FOOT;

    currchrono = nbrd->btime;
    rec_ref(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime, addreply);

    if (fp = fopen(fpath, "a"))
    {
      fprintf(fp, "%3d -> %s (%s)\n    %s\n", nbrd->total, cuser.userid, fromhost, reason);
      fclose(fp);
    }

    memset(&rec, 0, sizeof(LOG));
    strcpy(rec.userid, cuser.userid);
    strcpy(rec.email, cuser.email);
    *fname = 'G';
    rec_add(fpath, &rec, sizeof(LOG));

    vmsg("加入連署完成");
    return nbrd_init(xo);
  }

  return XO_FOOT;
}



static int
nbrd_finish(xo)
  XO *xo;
{
  NBRD *nbrd;
  char fpath[64], path[64];
  FILE *fp;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);

  if (nbrd->mode & NBRD_FINISH)
    return XO_NONE;

  if (vans("請確定結束連署(Y/N)？[N] ") != 'y')
    return XO_FOOT;

  vmsg("請編輯結束連署原因");
  sprintf(path, "tmp/%s", cuser.userid);	/* 連署原因的暫存檔案 */
  if(vedit(path, 0) < 0)
  {
    unlink(path);
    vmsg(msg_cancel);
    return nbrd_head(xo);
  }

  nbrd_fpath(fpath, xo->dir, nbrd);

  fp = fopen(fpath, "a");
  fprintf(fp, split_line);
  fprintf(fp, "※ %s 於 %s 關閉連署\n\n", HAS_PERM(PERM_SpSYSOP)? SysopLoginID_query(NULL) : cuser.userid, Now());
  fprintf(fp, "結束連署原因：\n\n");
  f_suck(fp, path);
  fclose(fp);
  unlink(path);

  nbrd->mode |= NBRD_FINISH;
  currchrono = nbrd->btime;
  rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);

  return nbrd_head(xo);
}



static int			/* 1:開板成功 */
nbrd_newbrd(nbrd)
  NBRD *nbrd;
{
  BRD newboard;
  ACCT acct;
  /* itoc.030519: 避免重覆開板 */
  if (brd_bno(nbrd->brdname) >= 0)
  {
    vmsg("已有此板");
    return 1;
  }

  memset(&newboard, 0, sizeof(BRD));

  /* itoc.010805: 新看板預設 battr = 不轉信, postlevel = PERM_POST, 看板板主為提起連署者 */
  newboard.battr = BRD_NOTRAN;
  newboard.postlevel = PERM_POST;
  strcpy(newboard.brdname, nbrd->brdname);
  strcpy(newboard.class, nbrd->class);
  strcpy(newboard.title, nbrd->title);
  strcpy(newboard.BM, nbrd->owner);

  if (acct_load(&acct, nbrd->owner) >= 0)
    acct_setperm(&acct, PERM_BM, 0);

  if (brd_new(&newboard) < 0)
    return 0;

  vmsg("新板已成立。記得要手動加入分類群組中");
  return 1;
}


static int
nbrd_open(xo)		/* itoc.010805: 開新板連署，連署完畢開新看板 */
  XO *xo;
{
  NBRD *nbrd;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);

  if (nbrd->mode & NBRD_FINISH || !(nbrd->mode & NBRD_NEWBOARD))
    return XO_NONE;

  if (vans("確定要開啟看板嗎(Y/N)？[N] ") == 'y')
  {
    if (nbrd_newbrd(nbrd))
    {
      FILE *fp;
      char fpath[64];

      nbrd_fpath(fpath, xo->dir, nbrd);
      fp = fopen(fpath, "a");
      fprintf(fp, "\n※ %s 於 %s 開啟看板 [%s]\n", HAS_PERM(PERM_SpSYSOP)? SysopLoginID_query(NULL) : cuser.userid, Now(), nbrd->brdname);

      nbrd->mode |= NBRD_FINISH;
      currchrono = nbrd->btime;
      rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);
    }
    return nbrd_head(xo);
  }

  return XO_FOOT;
}


static int
nbrd_browse(xo)
  XO *xo;
{
  int key;
  NBRD *nbrd;
  char fpath[80];

  /* itoc.010304: 為了讓閱讀到一半也可以加入連署，考慮 more 傳回值 */
  for (;;)
  {
    nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
    nbrd_fpath(fpath, xo->dir, nbrd);

    if ((key = more(fpath, MFST_AUTO)) < 0)
      break;

    if (!key)
      key = vkey();

    switch (key)
    {
    case KEY_UP:
    case KEY_PGUP:
    case 'k':
      key = xo->pos - 1;

      if (key < 0)
	break;

      xo->pos = key;

      if (key <= xo->top)
      {
	xo->top = (key / XO_TALL) * XO_TALL;
	nbrd_load(xo);
      }
      continue;

    case KEY_DOWN:
    case KEY_PGDN:
    case 'j':
    case ' ':
      key = xo->pos + 1;

      if (key >= xo->max)
	break;

      xo->pos = key;

      if (key >= xo->top + XO_TALL)
      {
	xo->top = (key / XO_TALL) * XO_TALL;
	nbrd_load(xo);
      }
      continue;

    case 'y':
      nbrd_reply(xo);
      break;
    }
    break;
  }

  return nbrd_head(xo);
}


static int
nbrd_delete(xo)
  XO *xo;
{
  NBRD *nbrd;
  char *fname, fpath[80];
  char *list = "@G";		/* itoc.註解: 清 newbrd file */

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
  if (strcmp(cuser.userid, nbrd->owner) && !HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  if (vans(msg_del_ny) != 'y')
    return XO_FOOT;

  nbrd_fpath(fpath, xo->dir, nbrd);
  fname = strrchr(fpath, '@');
  while (*fname = *list++)
  {
    unlink(fpath);	/* Thor: 確定名字就砍 */
  }

  currchrono = nbrd->btime;
  rec_del(xo->dir, sizeof(NBRD), xo->pos, cmpbtime);
  return nbrd_init(xo);
}


static int
nbrd_edit(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_ALLBOARD))
  {
    char fpath[64];
    NBRD *nbrd;

    nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
    nbrd_fpath(fpath, xo->dir, nbrd);
    vedit(fpath, 0);
    return nbrd_head(xo);
  }

  return XO_NONE;
}


static int
nbrd_setup(xo)
  XO *xo;
{
  int numbers;
  char ans[6];
  NBRD *nbrd, newnh;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  vs_bar("連署設定");
  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
  memcpy(&newnh, nbrd, sizeof(NBRD));

  prints("看板名稱：%s\n看板說明：%4.4s %s\n連署發起：%s\n",
    newnh.brdname, newnh.class, newnh.title, newnh.owner);
  prints("開始時間：%s\n", Btime(&newnh.btime));
  prints("結束時間：%s\n", Btime(&newnh.etime));
  prints("還需人數：%d\n", newnh.total);

  if (vget(8, 0, "(E)設定 (Q)取消？[Q] ", ans, 3, LCECHO) == 'e')
  {
    vget(11, 0, MSG_BID, newnh.brdname, BNLEN + 1, GCARRY);
    vget(12, 0, "看板分類：", newnh.class, sizeof(newnh.class), GCARRY);
    vget(13, 0, "看板主題：", newnh.title, sizeof(newnh.title), GCARRY);
    sprintf(ans, "%d", newnh.total);
    vget(14, 0, "連署人數：", ans, 6, GCARRY);
    numbers = atoi(ans);
    if (numbers <= 500 && numbers >= 1)
      newnh.total = numbers;

    if (memcmp(&newnh, nbrd, sizeof(newnh)) && vans(msg_sure_ny) == 'y')
    {
      memcpy(nbrd, &newnh, sizeof(NBRD));
      currchrono = nbrd->btime;
      rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);
    }
  }

  return nbrd_head(xo);
}


static int
nbrd_uquery(xo)
  XO *xo;
{
  NBRD *nbrd;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);

  move(1, 0);
  clrtobot();
  my_query(nbrd->owner);
  return nbrd_head(xo);
}


static int
nbrd_note(xo)
  XO *xo;
{
  clear();
  lim_cat("etc/help/cosign/note", b_lines);
  vmsg(NULL);

  return XO_HEAD;
}


static int
nbrd_help(xo)
  XO *xo;
{
  xo_help("cosign");
  return nbrd_head(xo);
}



static int
nbrd_trans(xo)	/* dust.101003: 從舊時代轉換過來的功能 */
  XO *xo;
{
  if(HAS_PERM(PERM_SpSYSOP))
  {
    NBRD *nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
    NBRD newh = *nbrd;

    if(newh.mode & NBRD_EXPIRE)
    {
      if(vans("要換成 NBRD_END 嗎(Y/N)？ [N] ") == 'y')
      {
	newh.mode &= ~NBRD_EXPIRE;
	newh.mode |= NBRD_END;
      }
    }
    else if(newh.mode & NBRD_END)
    {
      if(vans("要換成 NBRD_EXPIRE 嗎(Y/N)？ [N] ") == 'y')
      {
	newh.mode |= NBRD_EXPIRE;
	newh.mode &= ~NBRD_END;
      }
    }
    else
      return XO_NONE;

    if(memcmp(&newh, nbrd, sizeof(NBRD)))
    {
      currchrono = newh.btime;
      rec_put(xo->dir, &newh, sizeof(NBRD), xo->pos, cmpbtime);
      return XO_LOAD;
    }

    return XO_FOOT;
  }

  return XO_NONE;
}



static KeyFunc nbrd_cb[] =
{
  XO_INIT, nbrd_init,
  XO_LOAD, nbrd_load,
  XO_HEAD, nbrd_head,
  XO_BODY, nbrd_body,

  'y', nbrd_reply,
  'r', nbrd_browse,
  'o', nbrd_open,
  'c', nbrd_finish,
  'd', nbrd_delete,
  'E', nbrd_edit,
  'B', nbrd_setup,
  'b', nbrd_note,

  Ctrl('E'), nbrd_load,
  Ctrl('P'), nbrd_add,
  Ctrl('Q'), nbrd_uquery,
  Ctrl('G'), nbrd_trans,

  'h', nbrd_help
};


int
XoNewBoard()
{
  XO *xo;
  char fpath[64];

  sprintf(fpath, "run/newbrd/%s", fn_dir);
  xz[XZ_COSIGN - XO_ZONE].xo = xo = xo_new(fpath);
  xz[XZ_COSIGN - XO_ZONE].cb = nbrd_cb;
  xo->key = XZ_COSIGN;
  xo->pos = rec_num(fpath, sizeof(NBRD)) - 1;
  nbrd_note(xo);
  xover(XZ_COSIGN);
  free(xo);

  return 0;
}
#endif	/* HAVE_COSIGN */

