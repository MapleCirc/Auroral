/*-------------------------------------------------------*/
/* post.c       ( NTHU CS MapleBBS Ver 2.39 )		 */
/*-------------------------------------------------------*/
/* target : bulletin boards' routines		 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"

extern BCACHE *bshm;
extern XZ xz[];


extern int wordsnum;		/* itoc.010408: 計算文章字數 */
extern int TagNum;
extern char xo_pool[];
extern char brd_bits[];

extern char **brd_levelbar;	/* dust:給新版vs_head()用的 */

#ifdef HAVE_ANONYMOUS
extern char anonymousid[];	/* itoc.010717: 自定匿名 ID */
#endif


			/* 回覆/轉錄/原創/閱讀中的同主題回覆/閱讀中的同主題轉錄/閱讀中的同主題原創 */
static char *title_type[6] = {"Re", "Fw", "◇", "\033[1;33m=>", "\033[1;33m=>", "\033[1;32m◆"};
/* dust.100922: 改成全域變數供post_title()預覽使用 */


int 
cmpchrono (HDR *hdr)
{
  return hdr->chrono == currchrono;
}


static void 
change_stamp (char *folder, HDR *hdr)
{
  HDR tmp;
  char fpath[64];

  hdr_fpath(fpath, folder, hdr);

  /* stamp更動使用'C'作為檔名開頭 */
  hdr_stamp(folder, HDR_LINK | 'C', &tmp, fpath);

  hdr->stamp = tmp.chrono;
}


/* ----------------------------------------------------- */
/* 改良 innbbsd 轉出信件、連線砍信之處理程序		 */
/* ----------------------------------------------------- */


void 
btime_update (int bno)
{
  if (bno >= 0)
    (bshm->bcache + bno)->btime = -1;	/* 讓 class_item() 更新用 */
}


#ifndef HAVE_NETTOOL
static
#endif
void 
outgo_post (HDR *hdr, char *board)
{
  bntp_t bntp;

  memset(&bntp, 0, sizeof(bntp_t));

  if (board)		/* 新信 */
  {
    bntp.chrono = hdr->chrono;
  }
  else			/* cancel */
  {
    bntp.chrono = -1;
    board = currboard;
  }
  strcpy(bntp.board, board);
  strcpy(bntp.xname, hdr->xname);
  strcpy(bntp.owner, hdr->owner);
  strcpy(bntp.nick, hdr->nick);
  strcpy(bntp.title, hdr->title);
  rec_add("innd/out.bntp", &bntp, sizeof(bntp_t));
}


void 
cancel_post (HDR *hdr)
{
  if ((hdr->xmode & POST_OUTGO) &&		/* 外轉信件 */
    (hdr->chrono > ap_start - 7 * 86400))	/* 7 天之內有效 */
  {
    outgo_post(hdr, NULL);
  }
}


static inline int 
move_post (	/* 將 hdr 從 folder 搬到別的板 */
    HDR *hdr,
    char *folder,
    int by_bm
)
{
  HDR post;
  int xmode;
  char fpath[64], fnew[64], *board;
  struct stat st;

  xmode = hdr->xmode;
  hdr_fpath(fpath, folder, hdr);

  if (!(xmode & POST_BOTTOM))	/* 置底文被砍不用 move_post */
  {
#ifdef HAVE_REFUSEMARK
    board = by_bm && !(xmode & POST_RESTRICT) ? BN_DELETED : BN_JUNK;	/* 加密文章丟去 junk */
#else
    board = by_bm ? BN_DELETED : BN_JUNK;
#endif

    brd_fpath(fnew, board, fn_dir);
    hdr_stamp(fnew, HDR_LINK | 'A', &post, fpath);

    /* 直接複製 trailing data：owner(含)以下所有欄位 */

    memcpy(post.owner, hdr->owner, (&post + 1) - (HDR *)post.owner);

    if (by_bm)
      sprintf(post.title, "%-13s%.59s", cuser.userid, hdr->title);

    rec_bot(fnew, &post, sizeof(HDR));
    btime_update(brd_bno(board));
  }

  by_bm = stat(fpath, &st) ? 0 : st.st_size;

  unlink(fpath);
  btime_update(currbno);
  cancel_post(hdr);

  return by_bm;
}


#ifdef HAVE_DETECT_CROSSPOST
/* ----------------------------------------------------- */
/* 改良 cross post 停權					 */
/* ----------------------------------------------------- */


#define MAX_CHECKSUM_POST	20	/* 記錄最近 20 篇文章的 checksum */
#define MAX_CHECKSUM_LINE	6	/* 只取文章前 6 行來算 checksum */


typedef struct
{
  int sum;			/* 文章的 checksum */
  int total;			/* 此文章已發表幾篇 */
}      CHECKSUM;


static CHECKSUM checksum[MAX_CHECKSUM_POST];
static int checknum = 0;


static inline int 
checksum_add (		/* 回傳本列文字的 checksum */
    char *str
)
{
  int i, len, sum;

  len = strlen(str);

  sum = len;	/* 當字數太少時，前四分之一很可能完全相同，所以將字數也加入 sum 值 */
  for (i = len >> 2; i > 0; i--)	/* 只算前四分之一字元的 sum 值 */
    sum += *str++;

  return sum;
}


static inline int 
checksum_put (int sum)
{
  int i;

  if (sum)
  {
    for (i = 0; i < MAX_CHECKSUM_POST; i++)
    {
      if (checksum[i].sum == sum)
      {
	checksum[i].total++;

	if (checksum[i].total > MAX_CROSS_POST)
	  return 1;
	return 0;	/* total <= MAX_CROSS_POST */
      }
    }

    if (++checknum >= MAX_CHECKSUM_POST)
      checknum = 0;
    checksum[checknum].sum = sum;
    checksum[checknum].total = 1;
  }
  return 0;
}


static int 
checksum_find (char *fpath)
{
  int i, sum;
  char buf[ANSILINELEN];
  FILE *fp;

  sum = 0;
  if (fp = fopen(fpath, "r"))
  {
    for (i = -(LINE_HEADER + 1);;)	/* 前幾列是檔頭 */
    {
      if (!fgets(buf, ANSILINELEN, fp))
	break;

      if (i < 0)	/* 跳過檔頭 */
      {
	i++;
	continue;
      }

      if (*buf == QUOTE_CHAR1 || *buf == '\n' || !strncmp(buf, "※", 2))	 /* 跳過引言 */
	continue;

      sum += checksum_add(buf);
      if (++i >= MAX_CHECKSUM_LINE)
	break;
    }

    fclose(fp);
  }

  return checksum_put(sum);
}


static int 
check_crosspost (
    char *fpath,
    int bno			/* 要轉去的看板 */
)
{
  char *blist, folder[64];
  ACCT acct;
  HDR hdr;

  if (HAS_PERM(PERM_ALLADMIN))
    return 0;

  /* 板主在自己管理的看板不列入跨貼檢查 */
  blist = (bshm->bcache + bno)->BM;
  if (HAS_PERM(PERM_BM) && blist[0] > ' ' && is_bm(blist, cuser.userid))
    return 0;

  if (checksum_find(fpath))
  {
    /* 如果是 cross-post，那麼轉去 BN_SECURITY 並直接停權 */
    brd_fpath(folder, BN_SECURITY, fn_dir);
    hdr_stamp(folder, HDR_COPY | 'A', &hdr, fpath);
    strcpy(hdr.owner, cuser.userid);
    strcpy(hdr.nick, cuser.username);
    sprintf(hdr.title, "%s %s Cross-Post", cuser.userid, Now());
    rec_bot(folder, &hdr, sizeof(HDR));
    btime_update(brd_bno(BN_SECURITY));

    bbstate &= ~STAT_POST;
    cuser.userlevel &= ~PERM_POST;
    cuser.userlevel |= PERM_DENYPOST;
    if (acct_load(&acct, cuser.userid) >= 0)
    {
      acct.tvalid = time(NULL) + CROSSPOST_DENY_DAY * 86400;
      acct_setperm(&acct, PERM_DENYPOST, PERM_POST);
    }
    board_main();
    mail_self(FN_ETC_CROSSPOST, str_sysop, "Cross-Post 停權", 0);
    vmsg("您因為過度 Cross-Post 已被停權");
    return 1;
  }
  return 0;
}
#endif	/* HAVE_DETECT_CROSSPOST */


/* ----------------------------------------------------- */
/* 發表、回應、編輯、轉錄文章				 */
/* ----------------------------------------------------- */


int 
is_author (HDR *hdr)
{
  /* 這裡沒有檢查是不是 guest，注意使用此函式時要特別考慮 guest 情況 */

  /* itoc.070426: 當帳號被清除後，新註冊相同 ID 的帳號並不擁有過去該 ID 發表的文章之所有權 */
  return !strcmp(hdr->owner, cuser.userid) && (hdr->chrono > cuser.firstlogin);
}


#ifdef HAVE_REFUSEMARK
int 
chkrestrict (HDR *hdr)
{
  return !(hdr->xmode & POST_RESTRICT) || is_author(hdr) || (bbstate & STAT_BM);
}
#endif  


#ifdef HAVE_UNANONYMOUS_BOARD
static void 
do_unanonymous (HDR *shdr, int is_anonymous)
{
  HDR hdr;
  char fpath[64], folder[64], title[80], *ttl, aid[16];
  int len;
  FILE *fp;

  brd_fpath(folder, BN_UNANONYMOUS, fn_dir);

  if(!(fp = fdopen(hdr_stamp_time(folder, 'A', &hdr, fpath, shdr->chrono), "w")))
    return;

  strcpy(title, ve_title);

  fprintf(fp, "作者: %s (%s) 看板: %s\n", cuser.userid, cuser.username, currboard);
  fprintf(fp, "標題: %s\n", ve_title);
  fprintf(fp, "時間: %s\n\n", Btime(&shdr->chrono));

  aid_encode(shdr->chrono, aid);
  fprintf(fp, "#AID: %s (%s)\n", aid, currboard);
  fprintf(fp, "real: %s\n", cuser.userid);
  if(is_anonymous)
  {
    fprintf(fp, "anon: %s\n", anonymousid);
    fprintf(fp, "FQDN: %s\n", fromhost);
  }
  fputs("\n\n--\n", fp);

  fclose(fp);

  len = 41 - strlen(currboard);
  ttl = str_ttl(title);
  if(IS_ZHC_LO(ttl, len))
    ttl[len - 1] = ' ';		/* 防止蓋到雙位字的後半 */
  if(strlen(ttl) < len)
    memset(ttl + strlen(ttl), ' ', len - strlen(ttl));
  sprintf(ttl + len, ".%s板", currboard);

  strcpy(hdr.owner, cuser.userid);
  strcpy(hdr.title, title);
  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(brd_bno(BN_UNANONYMOUS));
}


static void
unanony_score(
  HDR *hdr,
  char *anid,
  time_t *pnow,
  char *verb,
  char *reason
)
{
  FILE *fp;
  char fpath[64], fname[32], buf[256], *str;
  time_t chrono;
  int i, c;

  fname[1] = '/';
  fname[2] = 'A';
  for(chrono = hdr->chrono; ; chrono++)
  {
    fname[0] = radix32[chrono & 31];
    archiv32(chrono, fname + 3);
    brd_fpath(fpath, BN_UNANONYMOUS, fname);

    if(!(fp = fopen(fpath, "r")))
      break;

    for(i = 0; i < 4 && (c = fgetc(fp)) != EOF;)
    {
      if(c == '\n')
	i++;
    }

    if(fgets(buf, sizeof(buf), fp) && buf[0] == '#')
    {
      if(hdr->chrono == aid_decode(buf + 6))
      {
	str = strchr(buf + 6, '(');
	strtok(str, ")");
	if(!strcmp(str + 1, currboard))	/* dust.100406: 換過板名的話就會找不到。不管它。 */
	  break;
      }
    }

    fclose(fp);
  }

  if(fp)
  {
    fclose(fp);
    if(fp = fopen(fpath, "a"))
    {
      fprintf(fp, "Time: %s\n", Btime(pnow));
      fprintf(fp, "real: %s\n", cuser.userid);
      fprintf(fp, "anon: %s\n", anid);
      fprintf(fp, "\033[3%s\033[1;30m:\033[m%s\n", verb, reason);
      fputs("\n--\n", fp);

      fclose(fp);
    }
  }
}

#endif



void 
add_post (  /* 發文到看板 */
    char *brdname,        /* 欲 post 的看板 */
    char *fpath,          /* 檔案路徑 */
    char *title,          /* 文章標題 */
    char *authurid,       /* 作者帳號 */
    char *authurnick,     /* 作者暱稱 */
    int xmode
)
{
  HDR hdr;
  char folder[64];

  brd_fpath(folder, brdname, fn_dir);
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  /* 如果authurid為空字串 ，則設為目前使用者ID*/
  if (authurid[0]=='\0')
    strcpy(authurid, cuser.userid) ;

  strcpy(hdr.owner, authurid);

  /* 如果authurnick為空字串 ，則設為目前使用者暱稱*/
  if (authurnick[0]=='\0')
    strcpy(authurnick, cuser.username) ;

  strcpy(hdr.nick, authurnick);

  strcpy(hdr.title, title);

  hdr.xmode = xmode;

  rec_bot(folder, &hdr, sizeof(HDR));

  btime_update(brd_bno(brdname));
}



static int 
do_post (XO *xo, char *title)
{
  /* Thor.981105: 進入前需設好 curredit 及 quote_file */
  HDR hdr, buf;
  char fpath[64], *folder, *nick, *rcpt = NULL;
  const char const *default_prefix[PREFIX_MAX] = DEFAULT_PREFIX;
  char prefix[PREFIX_MAX][PREFIX_LEN + 1], uprefix[PREFIX_LEN + 1];	/* 多一格放空白 */

char *ptr1, *ptr2;

  int mode;
#ifdef HAVE_TEMPLATE
  int ve_op = 1;
#endif
  time_t spendtime, prev, chrono;
  FILE *fp;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  if (!(bbstate & STAT_POST))
  {
#ifdef NEWUSER_LIMIT
    if (cuser.lastlogin - cuser.firstlogin < 3 * 86400)
      vmsg("新手上路，三日後始可張貼文章");
    else
#endif
      vmsg("對不起，您沒有在此發表文章的權限");
    return XO_FOOT;
  }

  brd_fpath(fpath, currboard, FN_POSTLAW);
  clear();
  if(lim_cat(fpath, POSTLAW_LINEMAX) < 0)
    film_out(FILM_POST, 0);

#ifdef POST_PREFIX
  /* 借用 mode、rcpt、fpath */

  if(!title)
  {
    char buf[64];
    int prefix_num, i = 0;

    brd_fpath(fpath, currboard, "prefix");
    if(fp = fopen(fpath, "r"))
    {
      fgets(buf, sizeof(buf), fp);
      sscanf(buf, "%d", &prefix_num);

      for(; i < prefix_num; i++)
      {
	if(!fgets(buf, sizeof(buf), fp))
	  break;
	strtok(buf, "\n");
	str_ncpy(prefix[i], buf, PREFIX_LEN);
      }

      fclose(fp);
    }
    else
      prefix_num = NUM_PREFIX_DEF;

    /* 填滿至 prefix_num 個 */
    for(; i < prefix_num; i++)
      strcpy(prefix[i], default_prefix[i]);

    if(prefix_num > 0)
    {
      int x = 5, y = 21, len;
      int middle_cut = prefix_num > 6;

      move(19,0);
      prints("在\033[1;33m【 %s 】\033[m看板發表文章", currboard);
      move(21, 0);
      outs("類別：");

      for(i = 0; i < prefix_num; i++)
      {
	if(y < 0)	/* 以省略符號印出 */
	{
	  prints(" \033[1;35m%d.\033[30m…", (i + 1) % 10);
	  continue;
	}

	len = strlen(prefix[i]);
	x += len + 3;
	if(y == b_lines)
	{
	  if(x + (prefix_num - i - 1) * 6 > b_cols)
	  {
	    y = -1;
	    i--;
	    continue;
	  }
	}
	else if(x > b_cols || (i == 5 && middle_cut))
	{
	  y++;
	  x = len + 8;
	  middle_cut = 0;
	  outs("\n     ");
	}

	if(i > 0) outc(' ');
	prints("\033[1;35m%d.\033[37m%s", (i + 1) % 10, prefix[i]);
      }

      attrset(7);


      /* dust.070902: 為了輸入上的方便，0其實是10 */

      i = vget(20, 0, "請選擇文章類別(或自行輸入)：", uprefix + 1, PREFIX_LEN - 2, DOECHO) - '0';
      i = (i + 9) % 10;

      /* floatJ: 改跟fgisc一樣可自行輸入，方便 user (創意來源感謝FGISC) */
      if(strlen(uprefix + 1) > 1)	/* 使用者自行輸入prefix */
      {
	uprefix[0] = '[';
	strcat(uprefix, "] ");
	rcpt = uprefix;
      }
      else if(i >= 0 && i < prefix_num)	/* 選擇了存在的文章類別 */
      {
#  ifdef HAVE_TEMPLATE
	ve_op = i;
	curredit |= EDIT_TEMPLATE;
#  endif
	rcpt = prefix[i];
	strcat(rcpt, " ");
      }
    }
  }

  if (!ve_subject(21, title, rcpt))
#else
  if (!ve_subject(21, title, NULL))
#endif
    return XO_HEAD;

  /* 未具備 Internet 權限者，只能在站內發表文章 */
  /* Thor.990111: 沒轉信出去的看板, 也只能在站內發表文章 */

  if (!HAS_PERM(PERM_INTERNET) || (currbattr & BRD_NOTRAN))
    curredit &= ~EDIT_OUTGO;

  utmp_mode(M_POST);
  fpath[0] = '\0';
  time(&spendtime);
#ifdef HAVE_TEMPLATE
  if (vedit(fpath, ve_op) < 0)		/* dust.080908: ve_op給文章範本使用 */
#else
  if (vedit(fpath, 1) < 0)
#endif
  {
    unlink(fpath);
    vmsg(msg_cancel);
    return XO_HEAD;
  }

  spendtime = time(0) - spendtime;	/* itoc.010712: 總共花的時間(秒數) */

  /* build filename */

  folder = xo->dir;
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  /* set owner to anonymous for anonymous board */

#ifdef HAVE_ANONYMOUS
  /* dust.100406: 無論原發文者有沒有匿名這下都得要記錄了... */

  if(currbattr & BRD_ANONYMOUS)
    do_unanonymous(&hdr, curredit & EDIT_ANONYMOUS);

  /* Thor.980727: lkchu新增之[簡單的選擇性匿名功能] */
  if (curredit & EDIT_ANONYMOUS)
  {
    rcpt = anonymousid;	/* itoc.010717: 自定匿名 ID */
    nick = "";	/* dust.100408: 如果是匿名ID的話應該就不需要暱稱了才對 */

#if 0	/* dust.100408: 完全改以Unanonymous板記錄匿名 */
    /* Thor.980727: lkchu patch: log anonymous post */
    /* Thor.980909: gc patch: log anonymous post filename */
    log_anonymous(hdr.xname);
#endif
  }
  else
#endif
  {
    rcpt = cuser.userid;
    nick = cuser.username;
  }
  title = ve_title;
  mode = (curredit & EDIT_OUTGO) ? POST_OUTGO : 0;
#ifdef HAVE_REFUSEMARK
  if (curredit & EDIT_RESTRICT)
    mode |= POST_RESTRICT;
#endif

  hdr.xmode = mode;
  hdr.stamp = 0;	/* dust.091125: for CRH */
  strcpy(hdr.owner, rcpt);
  strcpy(hdr.nick, nick);
  strcpy(hdr.title, title);

  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(currbno);

  if (mode & POST_OUTGO)
    outgo_post(&hdr, currboard);

#if 1	/* itoc.010205: post 完文章就記錄，使不出現未閱讀的＋號 */
  chrono = hdr.chrono;
  prev = ((mode = rec_num(folder, sizeof(HDR)) - 2) >= 0 && !rec_get(folder, &buf, sizeof(HDR), mode)) ? buf.chrono : chrono;
  brh_add(prev, chrono, chrono);
#endif

  clear();
  outs("順利貼出文章，");

  if (currbattr & BRD_NOCOUNT || wordsnum < 15)
  {				/* itoc.010408: 以此減少灌水現象 */
    outs("文章不列入紀錄，敬請包涵。");
  }
  else
  {
    /* itoc.010408: 依文章長度/所費時間來決定要給多少錢；幣制才會有意義 */
    mode = BMIN(wordsnum, spendtime) / 4;
    /* dust.090301: 修改成每四字或四秒一銀幣 */
    prints("這是您的第 %d 篇文章，得 %d 銀。", ++cuser.numposts, mode);
    addmoney(mode);
  }

  /* 回應到原作者信箱 */

  mode = 1;
  if (curredit & EDIT_BOTH)
  {
    rcpt = quote_user;

    if (strchr(rcpt, '@'))	/* 站外 */
    {
      if(bsmtp(fpath, title, rcpt, 0) >= 0)
        mode = 0;	/* dust.101220: 交給mailsender去刪除 */
    }
    else			/* 站內使用者 */
    {
      move(2, 0);
      if(mail_him(fpath, rcpt, title, 0) >= 0)
        outs("回應完成。");
      else
        outs("作者無法收信");
    }
  }

  if(mode)
    unlink(fpath);

  vmsg(NULL);

#ifdef HAVE_ANONYMOUS
  /* gaod.091205: 發匿名文後不取消 EDIT_ANONYMOUS 會導致轉錄文章 header 資訊有誤
 * 		  Thanks for om@cpu.tfcis.org. */
  if (curredit & EDIT_ANONYMOUS)
    curredit &= ~EDIT_ANONYMOUS;
#endif

  return XO_INIT;
}



int 
do_reply (XO *xo, HDR *hdr)
{
  curredit = EDIT_POSTREPLY;

  switch (vans("▲ 回應至 (F)看板 (M)作者信箱 (B)二者皆是 (Q)取消？[F] "))
  {
  case 'm':
    hdr_fpath(quote_file, xo->dir, hdr);
    return do_mreply(hdr, 0);

  case 'q':
    return XO_FOOT;

  case 'b':
    /* 若無寄信的權限，則只回看板 */
    if (HAS_PERM(strchr(hdr->owner, '@') ? PERM_INTERNET : PERM_LOCAL))
      curredit |= EDIT_BOTH;
    break;
  }

  /* Thor.981105: 不論是轉進的, 或是要轉出的, 都是別站可看到的, 所以回信也都應該轉出 */
  if (hdr->xmode & (POST_INCOME | POST_OUTGO))
    curredit |= EDIT_OUTGO;

  hdr_fpath(quote_file, xo->dir, hdr);
  strcpy(quote_user, hdr->owner);
  strcpy(quote_nick, hdr->nick);
  return do_post(xo, hdr->title);
}


static int 
post_reply (XO *xo)
{
  if (bbstate & STAT_POST)
  {
    HDR *hdr;

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif

    if (hdr->xmode & POST_RESERVED)       /* 被鎖定(!)的文章 */
    {
      vmsg("本文已被鎖定，無法回文");
      return XO_NONE;
    }

    return do_reply(xo, hdr);
  }
  return XO_NONE;
}


static int 
post_add (XO *xo)
{
  curredit = EDIT_OUTGO;
  *quote_file = '\0';
  return do_post(xo, NULL);
}


/* ----------------------------------------------------- */
/* 印出 hdr 標題					 */
/* ----------------------------------------------------- */


int 
tag_char (int chrono)
{
  return TagNum && !Tagger(chrono, 0, TAG_NIN) ? '*' : ' ';
}


#ifdef HAVE_DECLARE
static inline int 
cal_day (		/* itoc.010217: 計算星期幾 */
    char *date
)
{
#if 0
   蔡勒公式是一個推算哪一天是星期幾的公式.
   這公式是:
         c                y       26(m+1)
    W= [---] - 2c + y + [---] + [---------] + d - 1
         4                4         10
    W → 為所求日期的星期數. (星期日: 0  星期一: 1  ...  星期六: 6)
    c → 為已知公元年份的前兩位數字.
    y → 為已知公元年份的後兩位數字.
    m → 為月數
    d → 為日數
   [] → 表示只取該數的整數部分 (地板函數)
    ps.所求的月份如果是1月或2月,則應視為上一年的13月或14月.
       所以公式中m的取值範圍不是1到12,而是3到14
#endif

  /* 適用 2000/03/01 至 2099/12/31 */

  int y, m, d;

  y = 10 * ((int) (date[0] - '0')) + ((int) (date[1] - '0'));
  d = 10 * ((int) (date[6] - '0')) + ((int) (date[7] - '0'));
  if (date[3] == '0' && (date[4] == '1' || date[4] == '2'))
  {
    y -= 1;
    m = 12 + (int) (date[4] - '0');
  }
  else
  {
    m = 10 * ((int) (date[3] - '0')) + ((int) (date[4] - '0'));
  }
  return (-1 + y + y / 4 + 26 * (m + 1) / 10 + d) % 7;
}
#endif


/* dust.100920: 改成pfterm base，加入截斷符號 */
/* 不再考慮是否有定義 HAVE_DECLARE */
void 
hdr_outs (		/* print HDR's subject */
    HDR *hdr,
    int lim			/* 印出最多 lim 字的標題 */
)
{
  uschar *title, *mark;
  int ch, len;
  int in_dbc;
  int square_level;	/* 是第幾層中括號 */
#ifdef CHECK_ONLINE
  UTMP *online;
#endif

  /* --------------------------------------------------- */
  /* 印出日期						 */
  /* --------------------------------------------------- */

  /* itoc.010217: 改用星期幾來上色 */
  prints("\033[1;3%dm%s\033[m ", cal_day(hdr->date) + 1, hdr->date + 3);

  /* --------------------------------------------------- */
  /* 印出作者						 */
  /* --------------------------------------------------- */

#ifdef CHECK_ONLINE
  if (online = utmp_seek(hdr))
    outs(COLOR7);
#endif

  mark = hdr->owner;
  len = IDLEN + 1;
  in_dbc = 0;

  while (ch = *mark)
  {
    if (--len <= 0)
    {
      /* 把超過 len 長度的部分直接切掉 */
      /* itoc.060604.註解: 如果剛好切在中文字的一半就會出現亂碼，不過這情況很少發生，所以就不管了 */
      ch = '.';
    }
    else
    {
      /* 站外的作者把 '@' 換成 '.' */
      if (in_dbc || IS_ZHC_HI(ch))	/* 中文字尾碼是 '@' 的不算 */
	in_dbc ^= 1;
      else if (ch == '@')
	ch = '.';
    }
      
    outc(ch);

    if (ch == '.')
      break;

    mark++;
  }

  while (len--)
    outc(' ');

#ifdef CHECK_ONLINE
  if (online)
    outs(str_ransi);
#endif

  /* --------------------------------------------------- */
  /* 印出標題的種類					 */
  /* --------------------------------------------------- */

  if (!chkrestrict(hdr) && !(currbattr & BRD_XRTITLE))
  {
    title = RESTRICTED_TITLE;
    len = 2;
  }
  else
  {
    /* len: 標題是 type[] 裡面的那一種 */
    title = str_ttl(mark = hdr->title);
    len = (title == mark) ? 2 : (*mark == 'R') ? 0 : 1;
    if (!strcmp(currtitle, title))
      len += 3;
  }
  outs(title_type[len]);
  outc(' ');

  /* --------------------------------------------------- */
  /* 印出標題						 */
  /* --------------------------------------------------- */

  mark = title + lim;

  square_level = -1;
  if(len < 3 && *title == '[')
  {
    square_level = 0;
    attrset(15);
  }

  /* 把超過 cc 長度的部分切掉 */
  for(in_dbc = 0; (ch = *title) && title < mark; title++)
  {
    if(square_level >= 0 && !in_dbc)
    {
      switch(ch)
      {
      case '[':
	square_level++;
	break;
      case ']':
	square_level--;
	break;
      }
    }

    outc(ch);
    if(square_level == 0)
    {
      attrset(7);
      square_level--;
    }

    if(in_dbc)
      in_dbc = 0;
    else if(IS_ZHC_HI(ch))
      in_dbc = 1;
  }

  if(*title)
  {
    if(in_dbc) outc('\b');
    attrset(8);
    outc('>');
  }

  attrset(7);

  outc('\n');
}


/* ----------------------------------------------------- */
/* 看板功能表						 */
/* ----------------------------------------------------- */


static int post_body();
static int post_head();


static int 
post_init (XO *xo)
{
  xo_load(xo, sizeof(HDR));
  return post_head(xo);
}


static int 
post_load (XO *xo)
{
  xo_load(xo, sizeof(HDR));
  return post_body(xo);
}



/* dust.100401: 把 post_attr() 移回 post_item() */
/* 註: 以下程式碼無視HAVE_REFUSEMARK、HAVE_LABELMARK undefine的狀況 */
/*     也不管HAVE_SCORE undefine的狀況 */
static void 
post_item (int num, HDR *hdr)
{
  int attr, bottom;
  uschar color;
  register int m, x, s, t, lock, spms, cun, bun;
  usint mode;


  /* 前置處理 */
  mode = hdr->xmode;

  m = mode & POST_MARKED;
  x = mode & POST_RESTRICT;
  s = mode & POST_DONE;
  t = mode & POST_DELETE;
  lock = mode & POST_RESERVED;
  spms = mode & POST_SPMS;

  bottom = mode & POST_BOTTOM;
  cun = 0;
  if(!chkrestrict(hdr) || bottom)	/* 置底文與無法閱讀的文章視為已讀 */
    bun = 0;
  else
  {
    bun = rhs_unread(-1, hdr->chrono, hdr->stamp);

  /* dust.100331: 文章未讀記號僅使用一種顏色 */
    if(!(cuser.ufo & UFO_UNIUNREADM) && bun == 2)	/* CRH unread */
    {
      cun = 1;
      bun = 0;
    }
  }


  if(lock && (bun || cun))
    color = 6;		//*[36m
  else if(lock && m && x)
    color = 9;		//*[1;31m
  else if(lock && x)
    color = 1;		//*[31m
  else if(lock && m)
    color = 15;		//*[1;37m
  else if(cun && (s || x))
    color = 6;		//*[36m
  else if(cun && spms)
    color = 2;		//*[32m
  else if(cun && t)
    color = 8;		//*[1;30m
  else if(cun && m)
    color = 6;		//*[36m
  else if(s && m && x)
    color = 11;		//*[1;33m
  else if(m && (x || s || spms))
    color = 15;		//*[1m
  else if(t && spms)
    color = 12;		//*[1;34m
  else if(x && s)
    color = 1;		//*[31m
  else if(x && t)
    color = 12;		//*[1;34m
  else if(cun)
    color = 2;		//*[32m
  else
    color = 7;		//*[m (*[37m)

  attr = bun ? 0 : 0x20;	/* 已閱讀為小寫，未閱讀為大寫 */

  /* 顯示優先度: ! > S > X > # > T > M */
  if (lock)
    attr = '!';
  else if (s)
    attr |= 'S';        /* qazq.030815: 站方處理完標記ｓ */
  else if (x)
    attr |= 'X';
  else if (spms)	/* dust.080211: 特殊標記(搜尋用)'#' */
  {
    if(attr)
      attr = '#';	/* dust.100331: 修改未讀時的樣子 */
    else
      attr = 'b';
  }
  else if (t)
    attr |= 'T';
  else if (m)
    attr |= 'M';
  else if (!attr || cun)
    attr = '+';
  else
    attr = ' ';


  /* 輸出的部份 */

  if (bottom)			/* 印出文章編號 */
    outs("    "BOTTOM);
  else
    prints("%6d", num);

  outc(tag_char(hdr->chrono));		/* 印出Tag標記 */

  attrset(color);
  outc(attr);			/* 印出文章屬性 */
  attrset(7);

  if (hdr->xmode & POST_SCORE)		/* 印出評分分數 */
  {
    num = hdr->score;

    if(num <= -100)
      outs("\033[1;30mXX");
    else if(num < 0)
      prints("\033[1;32m%2d", -num);
    else if(num == 0)
      outs("\033[1;33m 0");
    else if(num < 100)
      prints("\033[1;31m%2d", num);
    else
      outs("\033[1;31m爆");

    outs("\033[m ");
  }
  else
    outs("   ");

  hdr_outs(hdr, d_cols + 45);
}



static int 
post_body (XO *xo)
{
  HDR *hdr;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    if (bbstate & STAT_POST)
    {
      if (vans("要新增資料嗎(Y/N)？[N] ") == 'y')
	return post_add(xo);
    }
    else
    {
      vmsg("本看板尚無文章");
    }
    return XO_QUIT;
  }

  hdr = (HDR *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    post_item(++num, hdr++);
  } while (num < max);
  clrtobot();

  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
}



static int 
limit_outs (char *str, int lim)
{
  int dword = 0, reduce = 0;
  char *buf, *end, *c_pool;

  /* 動態配置空間，這樣就不必用固定大小的緩衝區，可以解決寬螢幕終端機斷線的問題 */
  c_pool = (char*) malloc(lim + 1);

  if(!c_pool)	/* 空間配置失敗的話就直接跳掉 */
    exit(1);

  buf = c_pool;
  end = str+lim;
  while(str<end && *str)
  {
    if(dword)
    {
      buf[-1] = str[-1];
      *buf = *str;
      dword = 0;
      reduce = 0;
    }
    else if(*str & 0x80)	/* 為雙位字 */
    {
      dword = 1;
      reduce = 1;
    }
    else
      *buf = *str;;

    str++;
    buf++;
  }

  *buf = '\0';
  prints("%s", c_pool);

  free(c_pool);

  return reduce;
}


#define out_space(x) prints("%*s", x, "");

static int 
post_head (XO *xo)
{
  char *btitle;
  int title_start, title_end, title_len;
  int bm_end, right_start;
  int space;

  btitle = xo->xyz;
  title_len = strlen(btitle);
  title_start = (78+d_cols)/2 - title_len/2;
  title_end = title_start + title_len;

  bm_end = 4 + strlen(currBM);
  right_start = 69 + d_cols - strlen(currboard);

  clear();

  /* 印出 header(前段) */
  outs("\033[1;33m☆\033[32m ");
  space = 0;
  if(bm_end+2 > title_start)
  {
    bm_end = title_start - 2;
    space += limit_outs(currBM, bm_end - 6);
    outs("..");
  }
  else
    outs(currBM);
  outs(" \033[36;44m ");

  /* 印出 header(前段) */
  out_space(space + title_start - bm_end - 2);
  space = 0;
  if(title_end > right_start)
  {
    title_end = right_start;
    space += limit_outs(btitle, title_end - right_start);
    outs("\033[30m..\033[36");
  }
  else
    outs(btitle);
  out_space(space + right_start - title_end);

  /* 印出 header(後段) */
  prints(" \033[30;40m 看板%s%s%s\033[m\n", brd_levelbar[0], currboard, brd_levelbar[1]);

  /* 印出 neck */
  prints(NECKER_POST, d_cols, "", currbattr & BRD_NOSCORE ? "╳" : "○", bshm->mantime[currbno]);

  return post_body(xo);
}



/* ----------------------------------------------------- */
/* 資料之瀏覽：browse / history				 */
/* ----------------------------------------------------- */


static int 
post_visit (XO *xo)
{
  int ans, row, max;
  HDR *hdr;

  ans = vans("設定所有文章 (U)未讀 (V)已讀 (W)前已讀後未讀 (Q)取消？[Q] ");
  if (ans == 'v' || ans == 'u' || ans == 'w')
  {
    row = xo->top;
    max = xo->max - row + 3;
    if (max > b_lines)
      max = b_lines;

    hdr = (HDR *) xo_pool + (xo->pos - row);
    /* weiyu.041010: 在置底文上選 w 視為全部已讀 */
    rhs_visit((ans == 'u') ? 1 : (ans == 'w' && !(hdr->xmode & POST_BOTTOM)) ? hdr->chrono : 0);

    hdr = (HDR *) xo_pool;
    return post_body(xo);
  }
  return XO_FOOT;
}


static void 
post_history (		/* 將 hdr 這篇加入 brh */
    XO *xo,
    HDR *hdr
)
{
  time_t prev, chrono, next, stamp;
  int pos, top;
  char *dir;
  HDR buf;

  if (hdr->xmode & POST_BOTTOM)	/* 置底文不加入閱讀記錄 */
    return;

  chrono = hdr->chrono;
  stamp = hdr->stamp;

  switch (rhs_unread(-1, chrono, stamp))	/* 如果已在 brh 中，就無需動作 */
  {
  case 0:
    break;

  case 1:
    dir = xo->dir;
    pos = xo->pos;
    top = xo->top;

    pos--;
    if (pos >= top)
    {
      prev = hdr[-1].chrono;
    }
    else
    {
      /* amaki.040302.註解: 在畫面以上，只好讀硬碟 */
      if (!rec_get(dir, &buf, sizeof(HDR), pos))
	prev = buf.chrono;
      else
	prev = chrono;
    }

    pos += 2;
    if (pos < top + XO_TALL && pos < xo->max)
    {
      next = hdr[1].chrono;
    }
    else
    {
      /* amaki.040302.註解: 在畫面以下，只好讀硬碟 */
      if (!rec_get(dir, &buf, sizeof(HDR), pos))
	next = buf.chrono;
      else
	next = chrono;
    }

    brh_add(prev, chrono, next);

  case 2:
    crh_add(chrono, stamp);
  }
}


static int post_switch(XO*);
static int post_aidjump(XO*);

static int 
post_browse (XO *xo)
{
  HDR *hdr;
  int xmode, pos, key;
  char *dir, fpath[64];

  dir = xo->dir;

  for (;;)
  {
    pos = xo->pos;
    hdr = (HDR *) xo_pool + (pos - xo->top);
    xmode = hdr->xmode;

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif

    hdr_fpath(fpath, dir, hdr);

    /* Thor.990204: 為考慮more 傳回值 */   
    if ((key = more(fpath, MFST_AUTO)) < 0)
      break;

    post_history(xo, hdr);
    strcpy(currtitle, str_ttl(hdr->title));

    switch (xo_getch(xo, key))
    {
    case XO_BODY:
      continue;

    case 'y':
    case 'r':
      if (bbstate & STAT_POST)
      {
        if (hdr->xmode & POST_RESERVED)
        {
          vmsg("本文已被鎖定，無法回文");
          break;
        }

	if (do_reply(xo, hdr) == XO_INIT)	/* 有成功地 post 出去了 */
	  return post_init(xo);
      }

      break;

#if 0
    case 'm':
      if ((bbstate & STAT_BOARD) && !(xmode & POST_MARKED))
      {
	/* hdr->xmode = xmode ^ POST_MARKED; */
	/* 在 post_browse 時看不到 m 記號，所以限制只能 mark */
	hdr->xmode = xmode | POST_MARKED;
	currchrono = hdr->chrono;
	rec_put(dir, hdr, sizeof(HDR), pos, cmpchrono);
      }
      break;
#endif

#ifdef HAVE_SCORE
    case '%':
      post_score(xo);
      return post_init(xo);
#endif

    case 's':
      move(0, 0);
      outs("\033[30;47m【 選擇看板 】\033[m\n");
      return post_switch(xo);

    case '#':
      if(post_aidjump(xo) == XZ_POST)
	return XZ_POST;
      else
	break;

    case 'E':
      post_edit(xo);
      break;

    case 'C':	/* itoc.000515: post_browse 時可存入暫存檔 */
      {
	FILE *fp;
	if (fp = tbf_open())
	{ 
	  f_suck(fp, fpath); 
	  fclose(fp);
	}
      }
      break;

    case 'h':
      xo_help("post");
      break;
    }
    break;
  }

  return post_head(xo);
}


/* ----------------------------------------------------- */
/* 精華區						 */
/* ----------------------------------------------------- */


static int 
post_gem (XO *xo)
{
  int level;
  char fpath[64];

  strcpy(fpath, "gem/");
  strcpy(fpath + 4, xo->dir);

  level = 0;
  if (bbstate & STAT_BOARD)
    level ^= GEM_W_BIT;
  if (HAS_PERM(PERM_SYSOP))
    level ^= GEM_X_BIT;
  if (bbstate & STAT_BM)
    level ^= GEM_M_BIT;

  XoGem(fpath, "精華區", level);
  return post_init(xo);
}


/* ----------------------------------------------------- */
/* 進板畫面						 */
/* ----------------------------------------------------- */


static int 
post_memo (XO *xo)
{
  char fpath[64];

  brd_fpath(fpath, currboard, fn_note);
  /* Thor.990204: 為考慮more 傳回值 */   
  if (more(fpath, MFST_NULL) < 0)
  {
    vmsg("本看板沒有「進板畫面」");
    return XO_FOOT;
  }

  return post_head(xo);
}


/* ----------------------------------------------------- */
/* 功能：tag / switch / cross / forward			 */
/* ----------------------------------------------------- */


static int 
post_tag (XO *xo)
{
  HDR *hdr;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if (xo->key == XZ_XPOST)
    pos = hdr->xid;

  if (tag = Tagger(hdr->chrono, pos, TAG_TOGGLE))
  {
    move(3 + cur, 6);
    outc(tag > 0 ? '*' : ' ');
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE; /* lkchu.981201: 跳至下一項 */
}


static int 
post_switch (XO *xo)
{
  int bno;
  BRD *brd;
  char bname[BNLEN + 1];

  bbstate &= ~STAT_CANCEL;
  if (brd = ask_board(bname, BRD_R_BIT, NULL))
  {
    if ((bno = brd - bshm->bcache) >= 0 && currbno != bno)
    {
      XoPost(bno);
      return XZ_POST;
    }
  }
  else if(!(bbstate & STAT_CANCEL))
  {
    vmsg(err_bid);
  }
  return post_head(xo);
}


int 
post_cross (XO *xo)
{
  /* 來源看板 */
  char *dir, *ptr;
  HDR *hdr, xhdr;
  usint srcbattr;
  char srcboard[BNLEN + 1];

  /* 欲轉去的看板 */
  int xbno;
  usint xbattr;
  char xboard[BNLEN + 1], xfolder[64];
  HDR xpost;
  int xhide;	/* 0:轉錄至公開/好友板  1:轉錄至秘密看板 */

  int method;	/* 0:原文轉載  1:從公開看板/精華區/信箱轉錄文章  2:從秘密看板轉錄文章 */
  int rec_at_src;

  FILE *fpr, *fpw;
  char fpath[64], bname[32], buf[256];
  char userid[IDLEN + 1], username[UNLEN + 1];
  int tag, rc, locus;
  char *opts[] = {"1原文轉載", "2轉錄文章", NULL};


  if (!cuser.userlevel)	/* itoc.000213: 避免 guest 轉錄去 sysop 板 */
    return XO_NONE;

  tag = AskTag("轉錄");
  if (tag < 0)
    return XO_FOOT;

  hdr = tag ? &xhdr : (HDR *) xo_pool + (xo->pos - xo->top);

  if(!tag)
  {
    if(hdr->xmode & POST_RESTRICT)
    {
      vmsg("加密文章不能轉錄唷");
      return XO_NONE;
    }
    else if(hdr->xmode & GEM_FOLDER)	/* 非 plain/text 不能轉錄 */
    {
      return XO_NONE;
    }
  }

  dir = xo->dir;

  if (!ask_board(xboard, BRD_W_BIT, "\n\n\033[1;33m請挑選適當的看板，切勿轉錄超過三板。\033[m\n\n") ||
    (*dir == 'b' && !strcmp(xboard, currboard)))	/* 信箱、精華區中可以轉錄至currboard */
    return XO_HEAD;

  /* dust.110120: 文章以外禁止原文轉錄 */
  if((!tag && *dir == 'b' && is_author(hdr)) || HAS_PERM(PERM_ALLBOARD))
    method = krans(2, "21", opts, "") - '1';
  else
    method = 1;

  if (!tag)	/* lkchu.981201: 整批轉錄就不要一一詢問 */
  {
    if (method)
      sprintf(ve_title, "Fw: %.68s", str_ttl(hdr->title));	/* 已有 Re:/Fw: 字樣就只要一個 Fw: */
    else
      strcpy(ve_title, hdr->title);

    if (!vget(2, 0, "標題：", ve_title, TTLEN + 1, GCARRY))
      return XO_HEAD;
  }

#ifdef HAVE_REFUSEMARK
  switch(rc = vget(2, 0, "(S)存檔 (L)站內 (X)密封 (Q)取消？[Q] ", buf, 3, LCECHO))
  {
  case 's':
  case 'l':
  case 'x':
#else
  switch(rc = vget(2, 0, "(S)存檔 (L)站內 (Q)取消？[Q] ", buf, 3, LCECHO))
  {
  case 's':
  case 'l':
#endif
    break;
  default:
    return XO_HEAD;
  }

  if (method && *dir == 'b')	/* 從看板轉出，先檢查此看板是否為秘密板 */
  {
    if (bshm->bcache[currbno].readlevel == PERM_SYSOP)
      method = 2;
  }

  /* 設定轉錄目標的屬性 */
  xbno = brd_bno(xboard);
  xbattr = bshm->bcache[xbno].battr;
  if (bshm->bcache[xbno].readlevel == PERM_SYSOP)
    xhide = 1;
  else
    xhide = 0;

  /* Thor.990111: 在可以轉出前，要檢查有沒有轉出的權力? */
  if (rc == 's' && (!HAS_PERM(PERM_INTERNET) || (xbattr & BRD_NOTRAN)))
    rc = 'l';

  /* itoc.030325: 轉錄呼叫 ve_header，會使用到 currboard、currbattr，先備份起來 */
  strcpy(srcboard, currboard);
  srcbattr = currbattr;

  rec_at_src = !(currbattr & BRD_CROSSNOREC);
  strcpy(currboard, xboard);
  currbattr = xbattr;

  userid[0] = '\0';

  locus = 0;
  do	/* lkchu.981201: 整批轉錄 */
  {
    if (tag)
    {
      EnumTag(hdr, dir, locus, sizeof(HDR));

      if (method)
	sprintf(ve_title, "Fw: %.68s", str_ttl(hdr->title));	/* 已有 Re:/Fw: 字樣就只要一個 Fw: */
      else
	strcpy(ve_title, hdr->title);
    }

    if (hdr->xmode & GEM_FOLDER)	/* 非 plain text 不能轉 */
      continue;

#ifdef HAVE_REFUSEMARK
    if (hdr->xmode & POST_RESTRICT)
      continue;
#endif

    hdr_fpath(fpath, dir, hdr);

#ifdef HAVE_DETECT_CROSSPOST
    if (check_crosspost(fpath, xbno))
      break;
#endif

    brd_fpath(xfolder, xboard, fn_dir);
    fpw = fdopen(hdr_stamp(xfolder, 'A', &xpost, buf), "w");

    if (method)		/* 一般轉錄 */
    {
      int finish = 0;

      ve_header(fpw);

      /* itoc.040228: 如果是從精華區轉錄出來的話，會顯示轉錄自 [currboard] 看板，
	  然而 currboard 未必是該精華區的看板。不過不是很重要的問題，所以就不管了 :p */
      fprintf(fpw, "※ 本文轉錄自 [%s] %s\n\n",
	*dir == 'u' ? cuser.userid : method == 2 ? "秘密" : srcboard,
	*dir == 'u' ? "信箱" : "看板");

      /* Kyo.051117: 若是從秘密看板轉出的文章，刪除文章第一行所記錄的看板名稱 */
      if (method == 2 && (fpr = fopen(fpath, "r")))
      {
	if (fgets(buf, sizeof(buf), fpr) &&
	  ((ptr = strstr(buf, str_post1)) || (ptr = strstr(buf, str_post2))) && (ptr > buf))
	{
	  ptr[-1] = '\n';
	  *ptr = '\0';

	  do
	  {
	    fputs(buf, fpw);
	  } while (fgets(buf, sizeof(buf), fpr));
	  finish = 1;
	}
	fclose(fpr);
      }

      if (!finish)
	f_suck(fpw, fpath);

      /* 於目標加上轉錄的注解 */
      if(*dir == 'u')	/* dust.100321: 從信箱轉出來的改成加上站簽 */
	ve_banner(fpw, 0);
      else
      {
	int len;

	if(method == 2)
	  strcpy(bname, "(Unknown)");
	else
	  sprintf(bname, "[%s]", srcboard);
	len = 15 - strlen(bname);
	memset(buf, ' ', len);

	sprintf(buf + len, FORWARD_BANNER_F, bname, cuser.userid, Now());
	fputs("\n--\n", fpw);
	fputs(buf, fpw);
      }

      fclose(fpw);

      strcpy(xpost.owner, cuser.userid);
      strcpy(xpost.nick, cuser.username);

      /* 從看板轉出時，記錄轉出資訊於原文 */
      if (*dir == 'b' && rec_at_src)
      {			/* dust.100331: 追加轉錄出去不須留下轉錄紀錄 */
	int len;

	fpw = fopen(fpath, "a");

	if(xhide)
	  strcpy(bname, "(Unknown)");
	else
	  sprintf(bname, "[%s]", xboard);
	len = 17 - strlen(bname);
	memset(buf, ' ', len);
	sprintf(buf + len, FORWARD_BANNER_T, bname, cuser.userid, Now());
	fputs(buf, fpw);

	fclose(fpw);
      }
    }
    else		/* 原文轉錄 */
    {
      if(!(fpr = fopen(fpath, "r")))
      {
	fclose(fpw);
	continue;
      }

      /* 站務原文轉錄他人文章 */
      if(HAS_PERM(PERM_ALLBOARD) && strcmp(hdr->owner, cuser.userid))
      {
	if(!userid[0])	/* 備份 ID 和暱稱 */
	{
	  strcpy(userid, cuser.userid);
	  strcpy(username, cuser.username);
	}

	strcpy(cuser.userid, hdr->owner);
	strcpy(cuser.username, hdr->nick);
      }

      ve_header(fpw);

      if(!fgets(buf, sizeof(buf), fpr))
	goto OFW_CloseFile;
      if(!memcmp(buf, str_author1, LEN_AUTHOR1))
      {
	if(!fgets(buf, sizeof(buf), fpr))
	  goto OFW_CloseFile;
	if(!memcmp(buf, "標題:", 5))
	{
	  if(!fgets(buf, sizeof(buf), fpr))
	    goto OFW_CloseFile;
	  if(!memcmp(buf, "時間:", 5))
	  {
	    if(!fgets(buf, sizeof(buf), fpr))
	      goto OFW_CloseFile;
	    if(buf[0] == '\n')
	    {
	      if(!fgets(buf, sizeof(buf), fpr))
		goto OFW_CloseFile;
	    }
	  }
	}
      }

      do
      {
	fputs(buf, fpw);
      } while(fgets(buf, sizeof(buf), fpr));

OFW_CloseFile:
      fclose(fpr);
      fclose(fpw);

      strcpy(xpost.owner, hdr->owner);
      strcpy(xpost.nick, hdr->nick);
    }

    strcpy(xpost.title, ve_title);

    if (rc == 's')
      xpost.xmode = POST_OUTGO;
#ifdef HAVE_REFUSEMARK
    else if (rc == 'x')
      xpost.xmode = POST_RESTRICT;
#endif

    rec_bot(xfolder, &xpost, sizeof(HDR));

    if (rc == 's')
      outgo_post(&xpost, xboard);
  } while (++locus < tag);

  btime_update(xbno);

  /* Thor.981205: check 被轉的板有沒有列入紀錄? */
  if (!(xbattr & BRD_NOCOUNT))
    cuser.numposts += tag ? tag : 1;	/* lkchu.981201: 要算 tag */

  /* 復原 currboard、currbattr */
  strcpy(currboard, srcboard);
  currbattr = srcbattr;

  if(userid[0])
  {
    strcpy(cuser.userid, userid);
    strcpy(cuser.username, username);
  }

  vmsg("轉錄完成");
  return XO_HEAD;
}


int
post_forward(xo)
  XO *xo;
{
  ACCT muser;
  HDR *hdr;

  if (!HAS_PERM(PERM_LOCAL))
    return XO_NONE;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);

  if (hdr->xmode & GEM_FOLDER)	/* 非 plain text 不能轉 */
    return XO_NONE;

#ifdef HAVE_REFUSEMARK
  if (!chkrestrict(hdr))
    return XO_NONE;
#endif

  if (acct_get("轉達信件給：", &muser) > 0)
  {
    strcpy(quote_user, hdr->owner);
    strcpy(quote_nick, hdr->nick);
    hdr_fpath(quote_file, xo->dir, hdr);
    sprintf(ve_title, "%.64s (fwd)", hdr->title);
    move(1, 0);
    clrtobot();
    prints("轉達給: %s (%s)\n標  題: %s\n", muser.userid, muser.username, ve_title);

    mail_send(muser.userid);
    *quote_file = '\0';
  }
  return XO_HEAD;
}


/* ----------------------------------------------------- */
/* 板主功能：mark / delete / label			 */
/* ----------------------------------------------------- */


static int
post_mark(xo)
  XO *xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.090321:管理權力異動 */
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

#ifdef HAVE_LABELMARK
    if (xmode & POST_DELETE)	/* 待砍的文章不能 mark */
      return XO_NONE;
#endif

    hdr->xmode = xmode ^ POST_MARKED;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);

    return xo->pos + 1 + XO_MOVE;	/* lkchu.981201: 跳至下一項 */
  }
  return XO_NONE;
}



static int          /* qazq.030815: 站方處理完成標記Ｓ*/
post_done_mark(xo)
  XO *xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if ((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.091226: 開放給一般板主 */
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

#ifdef HAVE_LABELMARK
    if (xmode & POST_DELETE)    /* 待砍的文章不能S */
      return XO_NONE;
#endif
    if (xmode & POST_SPMS)      /* 含SPMS的文章不能S */
      return XO_NONE;
    if(xmode & POST_RESERVED)   /* 被鎖定的文章 */
      return XO_NONE;

    hdr->xmode = xmode ^ POST_DONE;
    currchrono = hdr->chrono;

    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ?
      hdr->xid : pos, cmpchrono);
    move(3 + cur, 0);
    post_item(pos + 1, hdr);

    return xo->pos + 1 + XO_MOVE;   /* lkchu.981201: 跳至下一項 */
  }
  return XO_NONE;
}



static int
post_spms(xo)
  XO* xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.090321:管理權力異動 */
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

    if(xmode & POST_DONE)
      return XO_NONE;
    if(xmode & POST_RESERVED)   /* 被鎖定的文章 */
      return XO_NONE;

    hdr->xmode = xmode ^ POST_SPMS;
    currchrono = hdr->chrono;

    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ?
      hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);

    return xo->pos + 1 + XO_MOVE;   /* lkchu.981201: 跳至下一項 */
  }
  return XO_NONE;
}



static int
post_reserve(xo)    /* dust.080212: 鎖定文章 */
  XO* xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.090321:管理權力異動 */
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif

    if((xmode & POST_LOCKCLEAR) && !(xmode & POST_RESERVED))
      xmode &= (~POST_LOCKCLEAR);

    hdr->xmode = xmode ^ POST_RESERVED;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);
  }

  return XO_NONE;
}



static int
post_bottom(xo)
  XO *xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.090321:管理權力異動 */
  {
    HDR *hdr, post;
    char fpath[64];
    int fd, fsize;
    struct stat st;

    if ((fd = open(xo->dir, O_RDONLY)) >= 0)
    {
      if (!fstat(fd, &st))
      {
        fsize = st.st_size;
        while ((fsize -= sizeof(HDR)) >= 0)
        {
          lseek(fd, fsize, SEEK_SET);
          read(fd, &post, sizeof(HDR));
          if (!(post.xmode & POST_BOTTOM))
            break;
        }
      }
      close(fd);
      if ((st.st_size - fsize) / sizeof(HDR) > 20)
      {
        vmsg("已達置底文數量上限");
        return XO_FOOT;
      }
    }

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

    hdr_fpath(fpath, xo->dir, hdr);
    hdr_stamp(xo->dir, HDR_LINK | 'A', &post, fpath);
#ifdef HAVE_REFUSEMARK
    post.xmode = POST_BOTTOM | (hdr->xmode & POST_RESTRICT);
#else
    post.xmode = POST_BOTTOM;
#endif
    strcpy(post.owner, hdr->owner);
    strcpy(post.nick, hdr->nick);
    strcpy(post.title, hdr->title);

    rec_add(xo->dir, &post, sizeof(HDR));
    /* btime_update(currbno); */	/* 不需要，因為置底文章不列入未讀 */

    return post_load(xo);	/* 立刻顯示置底文章 */
  }
  return XO_NONE;
}


#ifdef HAVE_REFUSEMARK
static int
post_refuse(xo)		/* itoc.010602: 文章加密 */
  XO *xo;
{
  HDR *hdr;
  int pos, cur;

  if (!cuser.userlevel)	/* itoc.020114: guest 不能對其他 guest 的文章加密 */
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if (is_author(hdr) || (bbstate & STAT_BM))
  {
    hdr->xmode ^= POST_RESTRICT;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    if((hdr->xmode & POST_RESTRICT))
    {
      if(pos + 1 >= xo->max || (hdr + 1)->xmode & POST_BOTTOM)
	btime_update(currbno);
    }

    move(3 + cur, 0);
    post_item(pos + 1, hdr);
  }

  return XO_NONE;
}
#endif


#ifdef HAVE_LABELMARK
static int
post_label(xo)
  XO *xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.090321:管理權力異動 */
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

    if (xmode & (POST_MARKED | POST_DONE))	/* mark和站方處理的文章不能待砍 */
      return XO_NONE;
    if(xmode & POST_RESERVED)   /* 被鎖定的文章 */
      return XO_NONE;

    hdr->xmode = xmode ^ POST_DELETE;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);

    return pos + 1 + XO_MOVE;	/* 跳至下一項 */
  }

  return XO_NONE;
}


static int
post_delabel(xo)
  XO *xo;
{
  int fdr, fsize, xmode;
  char fnew[64], fold[64], *folder;
  HDR *hdr;
  FILE *fpw;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  if (vans("確定要刪除待砍文章嗎(Y/N)？[N] ") != 'y')
    return XO_FOOT;

  folder = xo->dir;
  if ((fdr = open(folder, O_RDONLY)) < 0)
    return XO_FOOT;

  if (!(fpw = f_new(folder, fnew)))
  {
    close(fdr);
    return XO_FOOT;
  }

  fsize = 0;
  mgets(-1);
  while (hdr = mread(fdr, sizeof(HDR)))
  {
    xmode = hdr->xmode;

    if (!(xmode & POST_DELETE))
    {
      if ((fwrite(hdr, sizeof(HDR), 1, fpw) != 1))
      {
	close(fdr);
	fclose(fpw);
	unlink(fnew);
	return XO_FOOT;
      }
      fsize++;
    }
    else
    {
      /* 連線砍信 */
      cancel_post(hdr);

      hdr_fpath(fold, folder, hdr);
      unlink(fold);
    }
  }
  close(fdr);
  fclose(fpw);

  sprintf(fold, "%s.o", folder);
  rename(folder, fold);
  if (fsize)
    rename(fnew, folder);
  else
    unlink(fnew);

  btime_update(currbno);

  return post_load(xo);
}
#endif


static int
post_delete(xo)
  XO *xo;
{
  int pos, cur, by_BM;
  HDR *hdr;
  char buf[80];

  if (!cuser.userlevel ||
    !strcmp(currboard, BN_DELETED) ||
    !strcmp(currboard, BN_JUNK))
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if ((hdr->xmode & POST_MARKED) || (!(bbstate & STAT_BOARD) && !is_author(hdr)))
    return XO_NONE;

  by_BM = bbstate & STAT_BOARD;

  if (vans(msg_del_ny) == 'y')
  {
    currchrono = hdr->chrono;

    if (!rec_del(xo->dir, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono))
    {
     /* pos = */move_post(hdr, xo->dir, by_BM);

      if (!by_BM && !(currbattr & BRD_NOCOUNT) && !(hdr->xmode & POST_BOTTOM))
      {
	/* itoc.010711: 砍文章要扣錢，算檔案大小 */
	pos = pos >> 3;	/* 相對於 post 時 wordsnum / 10 */
#if 0
	/* itoc.010830.註解: 漏洞: 若 multi-login 砍不到另一隻的錢 */
	if (cuser.money > pos)
	  cuser.money -= pos;
	else
	  cuser.money = 0;
#endif
	if (cuser.numposts > 0)
	  cuser.numposts--;
	sprintf(buf, "%s，您的文章減為 %d 篇", MSG_DEL_OK, cuser.numposts);
	vmsg(buf);
      }

      if (xo->key == XZ_XPOST)
      {
	vmsg("原列表經刪除後混亂，請重進串接模式！");
	return XO_QUIT;
      }
      return XO_LOAD;
    }
  }
  return XO_FOOT;
}


static int
chkpost(hdr)
  HDR *hdr;
{
  return (hdr->xmode & POST_MARKED);
}


static int
vfypost(hdr, pos)
  HDR *hdr;
  int pos;
{
  return (Tagger(hdr->chrono, pos, TAG_NIN) || chkpost(hdr));
}


static void
delpost(xo, hdr)
  XO *xo;
  HDR *hdr;
{
  char fpath[64];

  cancel_post(hdr);
  hdr_fpath(fpath, xo->dir, hdr);
  unlink(fpath);
}


static int
post_rangedel(xo)
  XO *xo;
{
  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  btime_update(currbno);

  return xo_rangedel(xo, sizeof(HDR), chkpost, delpost);
}


static int
post_prune(xo)
  XO *xo;
{
  int ret;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  ret = xo_prune(xo, sizeof(HDR), vfypost, delpost);

  btime_update(currbno);

  if (xo->key == XZ_XPOST && ret == XO_LOAD)
  {
    vmsg("原列表經批次刪除後混亂，請重進串接模式！");
    return XO_QUIT;
  }

  return ret;
}


static int
post_copy(xo)	   /* itoc.010924: 取代 gem_gather */
  XO *xo;
{
  int tag;

  tag = AskTag("看板文章拷貝");

  if (tag < 0)
    return XO_FOOT;

#ifdef HAVE_REFUSEMARK
  gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), chkrestrict);
#else
  gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), NULL);
#endif

  if (bbstate & STAT_BOARD)
  {
#ifdef XZ_XPOST
    if (xo->key == XZ_XPOST)
    {
      zmsg("檔案標記完成。[注意] 您必須先離開串接模式才能進入精華區。");
      return XO_FOOT;
    }
    else
#endif
    {
      zmsg("拷貝完成。[注意] 貼上後才能刪除原文！");
      return post_gem(xo);	/* 拷貝完直接進精華區 */
    }
  }

  zmsg("檔案標記完成。[注意] 您只能在擔任(小)板主所在或個人精華區貼上。");
  return XO_FOOT;
}


static int
post_tbf(xo)
  XO *xo;
{
  if (!cuser.userlevel)
    return XO_NONE;

  xo_tbf(xo, chkrestrict);

  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 站長功能：edit / title				 */
/* ----------------------------------------------------- */


int
post_edit(xo)
  XO *xo;
{
  char fpath[64];
  HDR *hdr;
  FILE *fp;
  int modified = 0;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);

  hdr_fpath(fpath, xo->dir, hdr);

  if (hdr->xmode & POST_RESERVED)	/* 被鎖定(!)的文章 */
  {
    vmsg("本文已被鎖定，無法修改文章");
    return XO_NONE;
  }

  if(currbattr & BRD_LOG)
    return XO_NONE;

  if (HAS_PERM(PERM_ALLBOARD))
  {
#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif
    curredit |= EDIT_MODIFYPOST;
    if(!vedit(fpath, 0) && vans("喵～要留下修改紀錄嗎？ (Y/N) [N] ") == 'y')
    {
      if (fp = fopen(fpath, "a"))
      {
        ve_banner(fp, 1);
        fclose(fp);
        modified = 1;
      }
    }
  }
  else if(cuser.userlevel && is_author(hdr))	/* 原作者修改 */
  {
    curredit |= EDIT_MODIFYPOST;
    if(!vedit(fpath, 0))	/* 若非取消則加上修改資訊 */
    {
      if (fp = fopen(fpath, "a"))
      {
        ve_banner(fp, 1);
        fclose(fp);
        modified = 1;
      }
    }
  }
  else		/* itoc.010301: 提供使用者修改(但不能儲存)其他人發表的文章 */
  {
#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif
    vedit(fpath, -1);
  }

  if(modified && hdr->stamp > 0)
  {	/* dust.100905: 有必要減少記錄頻率嗎? */
    change_stamp(xo->dir, hdr);

    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : xo->pos, cmpchrono);

    /* 加入自己的閱讀記錄 */
    if(!(hdr->xmode & POST_BOTTOM)) 
      crh_add(currchrono, hdr->stamp);

    btime_update(currbno);
  }

  /* return post_head(xo); */
  return XO_HEAD;	/* itoc.021226: XZ_POST 和 XZ_XPOST 共用 post_edit() */
}


void
header_replace(xo, hdr)		/* itoc.010709: 修改文章標題順便修改內文的標題 */
  XO *xo;
  HDR *hdr;
{
  FILE *fpr, *fpw;
  char srcfile[64], tmpfile[64], buf[ANSILINELEN];
  
  hdr_fpath(srcfile, xo->dir, hdr);
  strcpy(tmpfile, "tmp/");
  strcat(tmpfile, hdr->xname);
  f_cp(srcfile, tmpfile, O_TRUNC);

  if (!(fpr = fopen(tmpfile, "r")))
    return;

  if (!(fpw = fopen(srcfile, "w")))
  {
    fclose(fpr);
    return;
  }

  fgets(buf, sizeof(buf), fpr);		/* 加入作者 */
  fputs(buf, fpw);

  fgets(buf, sizeof(buf), fpr);		/* 加入標題 */
  if (!str_ncmp(buf, "標", 2))		/* 如果有 header 才改 */
  {
    strcpy(buf, buf[2] == ' ' ? "標  題: " : "標題: ");
    strcat(buf, hdr->title);
    strcat(buf, "\n");
  }
  fputs(buf, fpw);

  while(fgets(buf, sizeof(buf), fpr))	/* 加入其他 */
    fputs(buf, fpw);

  fclose(fpr);
  fclose(fpw);
  f_rm(tmpfile);
}


static int
post_title(xo)
  XO *xo;
{
  HDR *fhdr, mhdr;
  int i, pos, cur;
  int modify = 0, bm_done;
  char *title, ans[3];
  char *menu[] = {"Y萌\306\356萌\306\356", "N我不萌貓耳的", "C貓也可以", NULL};
  screen_backup_t old_screen;

  if (!cuser.userlevel)	/* itoc.000213: 避免 guest 在 sysop 板改標題 */
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  fhdr = (HDR *) xo_pool + cur;

  if(is_author(fhdr))
    bm_done = 0;
  else
  {
    if(HAS_PERM(PERM_ALLBOARD) && chkrestrict(fhdr))
      bm_done = 0;
    else if(bbstate & STAT_BOARD)
      bm_done = 1;
    else
      return XO_NONE;
  }

  memcpy(&mhdr, fhdr, sizeof(HDR));

  vget(b_lines, 0, "標題：", mhdr.title, TTLEN + 1, GCARRY);

  if(HAS_PERM(PERM_ALLBOARD))
  {
    scr_dump(&old_screen);
    grayout(0, b_lines - 2, GRAYOUT_DARK);
    title = str_ttl(mhdr.title);
    i = (title == mhdr.title)? 5 : (mhdr.title[0] == 'R') ? 3 : 4;
    move(b_lines, 0);
    prints("%s %s\033[m\n", title_type[i], title);

    switch(krans(b_lines - 1, "nn", menu, "喵～？ "))
    {
    case 'c':
      vget(b_lines, 0, "作者：", mhdr.owner, 73 /* sizeof(mhdr.owner) */, GCARRY);
                /* Thor.980727: sizeof(mhdr.owner) = 80 會超過一行 */
      vget(b_lines, 0, "暱稱：", mhdr.nick, sizeof(mhdr.nick), GCARRY);
      vget(b_lines, 0, "日期：", mhdr.date, sizeof(mhdr.date), GCARRY);

    case 'y':
      modify = 1;
    }

    scr_restore(&old_screen);
  }
  else if(memcmp(fhdr, &mhdr, sizeof(HDR)))
  {
    scr_dump(&old_screen);
    grayout(0, b_lines - 2, GRAYOUT_DARK);
    title = str_ttl(mhdr.title);
    i = (title == mhdr.title)? 5 : (mhdr.title[0] == 'R') ? 3 : 4;
    move(b_lines, 0);
    prints("%s %s\033[m\n", title_type[i], title);

    if(vget(b_lines - 1, 0, msg_sure_ny, ans, sizeof(ans), LCECHO) == 'y')
      modify = 1;

    scr_restore(&old_screen);
  }
  else
    return XO_FOOT;

  if(modify)
  {
    memcpy(fhdr, &mhdr, sizeof(HDR));
    currchrono = fhdr->chrono;
    rec_put(xo->dir, fhdr, sizeof(HDR), xo->key == XZ_XPOST ? fhdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(++pos, fhdr);

    /* itoc.010709: 修改文章標題順便修改內文的標題 */
    /* dust.100922: 追加如果是板主改的標題就不改內文標題 */
    if(!bm_done)
      header_replace(xo, fhdr);
  }

  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 額外功能：write / score				 */
/* ----------------------------------------------------- */


int
post_write(xo)			/* itoc.010328: 丟線上作者水球 */
  XO *xo;
{
  if (HAS_PERM(PERM_PAGE))
  {
    HDR *hdr;
    UTMP *up;

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

    if (!(hdr->xmode & POST_INCOME) && (up = utmp_seek(hdr)))
      do_write(up);
  }
  return XO_NONE;
}


#ifdef HAVE_SCORE

static int curraddscore;

static void
addscore(hdd, ram)
  HDR *hdd, *ram;
{
  /* itoc.030618: 造一個新的 chrono 及 xname */
  hdd->stamp = ram->stamp;
  hdd->xmode |= POST_SCORE;
  hdd->score += curraddscore;
}


static int
post_resetscore(xo)
  XO *xo;
{
  if (bbstate & STAT_BOARD) /* bm only */
  {
    HDR *hdr;
    int pos, cur, score;
    char ans[3];

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    switch (vans("◎評分設定 1)自訂 2)清除 3)爆 4)爛 [Q] "))
		/* 這裡會有幻視現象...... */
    {
    case '1':
#if 1
      if (!vget(b_lines, 0, "主人～你要改成多少呢？(大眼汪汪地盯著你看) ", ans, 5, DOECHO))
#else
      if (!vget(b_lines, 0, "輸入分數(-100 ~ 100)：", ans, 5, DOECHO))
#endif
        return XO_FOOT;
      score = atoi(ans);
      if (score>100) score = 100;
      else if (score<-100) score = -100;
      hdr->xmode |= POST_SCORE;		/* 原文可能無評分 */
      hdr->score = score;
      break;

    case '2':
      hdr->xmode &= ~POST_SCORE; 	/* 清除就不需要POST_SCORE了 */
      hdr->score = 0;
      break;

    case '3':
      hdr->xmode |= POST_SCORE;		/* 原文可能無評分 */
      hdr->score = 100;
      break;

    case '4':
      hdr->xmode |= POST_SCORE;		/* 原文可能無評分 */
      hdr->score = -100;
      break;
    case '5':
      vmsg("ker ker");			/* just for a stupid test XD */
      break;

    default:
      return XO_FOOT;
    }

    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);
  }
  return XO_LOAD;
}


int
post_score(xo)
  XO *xo;
{
  HDR *hdr;
  FILE *fp;

  int pos, cur, ans, vtlen, maxlen, idlen;
  char fpath[64], reason[80], vtbuf[32], prompt[48];
  char *dir, *userid, *verb;
  time_t now;

#ifdef HAVE_ANONYMOUS
  char uid[IDLEN + 2];
#endif

  /* 評分的權限和發表文章相同 */
  if ((currbattr & BRD_NOSCORE) || !(bbstate & STAT_POST))
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

#ifdef HAVE_REFUSEMARK
  if (!chkrestrict(hdr))	/* 看不到的文不能評分 */
    return XO_NONE;
#endif

  if (hdr->xmode & POST_RESERVED)	/* 被鎖定(!)的文章 */
  {
    vmsg("本文已被鎖定，無法推文");
    return XO_NONE;
  }

  ans = vans("★ 評分  0/5)說話 1)推文 2)唾棄 3)自訂推 4)自訂呸？[Q] ");
  if(ans == '0' || ans == '5' || ans == '%')
  {
    verb = "3m說";
    vtlen = 2;

    curraddscore = 0;
  }
  else if(ans == '1' || ans == '!')
  {
    verb = "1m推";
    vtlen = 2;

    curraddscore = 1;
  }
  else if(ans == '2' || ans == '@')
  {
    verb = "2m呸";
    vtlen = 2;

    curraddscore = -1;
  }
  else if(ans == '3' || ans == '#')
  {
    if(!vget(b_lines, 0, "請輸入動詞：", &vtbuf[2], 5, DOECHO))
      return XO_FOOT;

    memcpy(vtbuf, "1m", sizeof(char) * 2);
    verb = vtbuf;
    vtlen = strlen(vtbuf+2);

    curraddscore = 1;
  }
  else if(ans == '4' || ans == '$')
  {
    if(!vget(b_lines, 0, "請輸入動詞：", &vtbuf[2], 5, DOECHO))
      return XO_FOOT;

    memcpy(vtbuf, "2m", sizeof(char) * 2);
    verb = vtbuf;
    vtlen = strlen(vtbuf+2);

    curraddscore = -1;
  }
  else 
    return XO_FOOT;

#ifdef HAVE_ANONYMOUS
  /* 匿名推文功能 */
  if (currbattr & BRD_ANONYMOUS)
  {
    userid = uid;
    if (!vget(b_lines, 0, "請輸入想用的ID，或輸入\033[1mr\033[m使用真名 (不輸入則使用預設ID)：", userid, IDLEN + 1, DOECHO))
      userid = STR_ANONYMOUS;
    else if (userid[0] == 'r' && userid[1] == '\0')
      userid = cuser.userid;
    else
      strcat(userid, ".");		/* 自定的話，最後加 '.' */
  }
  else
#endif
    userid = cuser.userid;

  idlen = strlen(userid);
  if(idlen <= 9)
  {
    maxlen = 62 - vtlen - idlen;
    sprintf(prompt, "→ \033[36m%s \033[3%s\033[1;30m:\033[m", userid, verb);
  }
  else	/* ID長度10以上的特殊排版 */
  {
    const char *prefix[] = {"﹣", " ", ""};
    int i = idlen - 10;

    if(i > 2) i = 2;

    if(idlen <= 12)
      maxlen = 53 - vtlen;
    else
      maxlen = 65 - vtlen - idlen;
    sprintf(prompt, "%s\033[36m%s \033[3%s\033[1;30m:\033[m", prefix[i], userid, verb);
  }

  if(!vget(b_lines, 0, prompt, reason, maxlen, DOECHO))
    return XO_FOOT;

  time(&now);

  dir = xo->dir;
  hdr_fpath(fpath, dir, hdr);

  if (fp = fopen(fpath, "a"))
  {
    struct tm *ptime = localtime(&now);

    fprintf(fp, "%s%-*s\033[1;30m%02d/%02d\033[;30m%c\033[1m%02d:%02d\033[m\n",
      prompt, maxlen, reason, ptime->tm_mon + 1, ptime->tm_mday,
      (ptime->tm_year + 49) % 50 + ((ptime->tm_year + 49) % 50 < 25 ? 'A' : 'a' - 25),
      ptime->tm_hour, ptime->tm_min);

    fclose(fp);
  }

#ifdef HAVE_ANONYMOUS
  if(userid != cuser.userid)	/* 記錄匿名發文 */
    unanony_score(hdr, userid, &now, verb, reason);
#endif

  change_stamp(dir, hdr);

  currchrono = hdr->chrono;
  rec_ref(dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono, addscore);

  /* 加入自己的閱讀紀錄 */
  if(!(hdr->xmode & POST_BOTTOM))	/* 置底文沒有閱讀紀錄，不能加 */
    crh_add(currchrono, hdr->stamp);

  btime_update(currbno);

  return XO_LOAD;
}
#endif	/* HAVE_SCORE */


#ifdef MOVE_CAPABLE
static int
post_move(xo)
  XO *xo;
{
  HDR *hdr, mhdr;
  char buf[40];
  int pos, new_pos;
  int supermode = HAS_PERM(PERM_ALLBOARD);

  if(!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  hdr = (HDR *) xo_pool + (pos - xo->top);

  if(!supermode && !(hdr->xmode & POST_BOTTOM))	/* 一般人不能移動非置底文 */
    return XO_NONE;

  switch(vget(b_lines, 0, "請輸入新位置(直接指定/+往後/-往前)：", buf, 6, DOECHO))
  {
  case '\0':
    return XO_FOOT;

  case '+':
    new_pos = pos + abs(atoi(buf + 1));
    break;

  case '-':
    new_pos = pos - abs(atoi(buf + 1));
    break;

  default:
    new_pos = atoi(buf) - 1;
  }

  if(new_pos < 0)
    new_pos = 0;
  else if(new_pos >= xo->max)
    new_pos = xo->max - 1;

  if(!supermode)	/* 一般人不能移動到置底文區外 */
  {
    rec_get(xo->dir, &mhdr, sizeof(HDR), new_pos);
    if(!(mhdr.xmode & POST_BOTTOM))
      return XO_FOOT;
  }

  if(new_pos != pos)
  {
    currchrono = hdr->chrono;
    if(!rec_del(xo->dir, sizeof(HDR), pos, cmpchrono))
    {
      rec_ins(xo->dir, hdr, sizeof(HDR), new_pos, 1);
      xo->pos = new_pos;
      return XO_LOAD;
    }
  }

  return XO_FOOT;
}
#endif			/* MOVE_CAPABLE */


static int
post_query(xo)		/* dust.091213: 顯示文章資訊 */
  XO *xo;
{
  HDR *ghdr;
  char owner[80], aid[16];
  char xmodes[64] = "";
  int cx, cy, y, i, mode;

  ghdr = (HDR *) xo_pool + (xo->pos - xo->top);
  
  /* 處理作者資訊 */
  strcpy(owner, ghdr->owner);
  strtok(owner, "@");
  if(strlen(owner) > 24)
    owner[24] = '\0';

  if(strtok(NULL, "\n"))
    strcat(owner, " \033[1;30m(站外發信)\033[m");
  else
    strcat(owner, " \033[1;30m(站內發表)\033[m");

  /* 處理文章屬性資訊 */
  mode = ghdr->xmode;
  if(mode & POST_RESERVED)
    strcat(xmodes, "!  ");
  if(mode & POST_DONE)
    strcat(xmodes, "S  ");
  if(mode & POST_RESTRICT)
    strcat(xmodes, "X  ");
  if(mode & POST_SPMS)
    strcat(xmodes, "#  ");
  if(mode & POST_MARKED)
    strcat(xmodes, "M  ");
  if(mode & POST_DELETE)
    strcat(xmodes, "T  ");
  
  if(strlen(xmodes) > 25)
    xmodes[25] = '\0';
  else if(xmodes[0] == '\0')
    sprintf(xmodes, "\033[1;30m(無)\033[m");

  aid_encode(ghdr->chrono, aid);


  /* dust.111111: 改成先畫框再填字，提高泛用度 */
  getyx(&cy, &cx);
  if(cy <= b_lines / 2 + 1)
    y = cy + 1;
  else
    y = cy - 5;

  clrregion(y, y + 4);
  grayout(0, y - 1, GRAYOUT_DARK);
  grayout(y + 5, b_lines, GRAYOUT_DARK);

  /* 畫框 */
  move(y, 0);
  outs("┌");
  for(i = 2; i < b_cols - 3; i += 2) outs("─");
  outs("┐");

  for(i = 1; i < 4; i++)
  {
    move(y + i, 0);
    outs("│");
    move(y + i, b_cols - (b_cols % 2? 3 : 2));
    outs("│");
  }

  move(y + 4, 0);
  outs("└");
  for(i = 2; i < b_cols - 3; i += 2) outs("─");
  outs("┘");

  /* 印出文章資訊 */
  move(y + 1, 3);
  prints("文章代碼(AID): \033[1m#%s (%s)   \033[36m[" BBSNAME "]\033m", aid, currboard);

  move(y + 2, 3);
  if(chkrestrict(ghdr) || (currbattr & BRD_XRTITLE))
  {
    if(strlen(ghdr->title) > 67 + d_cols)
      outs(ghdr->title);
    else
      prints("\033[1m標題:\033[m %s", ghdr->title);
  }
  else
    outs("\033[1m標題:\033[m " RESTRICTED_TITLE);

  move(y + 3, 3);
  prints("\033[1m作者:\033[m %s", owner);
  move(y + 3, 45);
  prints("\033[1m屬性:\033[m %s", xmodes);

  move(cy, cx);
  vkey();

  return post_head(xo);
}


static int
post_state(xo)		/* dust.091213: 顯示文章資訊(站務專用版) */
  XO *xo;
{
  HDR *ghdr;
  char fpath[64];
  struct stat st;
  int mode;
  char xmodes[64] = "";

  if(!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  ghdr = (HDR *) xo_pool + (xo->pos - xo->top);

  hdr_fpath(fpath, xo->dir, ghdr);
  mode = ghdr->xmode;

  /* dust.071116 :未來新增屬性時要手動修改這裡...... */
  if(mode & POST_RESERVED)
    strcat(xmodes, "!  ");
  if(mode & POST_DONE)
    strcat(xmodes, "S  ");
  if(mode & POST_RESTRICT)
    strcat(xmodes, "X  ");
  if(mode & POST_SPMS)
    strcat(xmodes, "#  ");
  if(mode & POST_MARKED)
    strcat(xmodes, "M  ");
  if(mode & POST_DELETE)
    strcat(xmodes, "T  ");

  if(*xmodes == '\0')
    strcpy(xmodes,"\033[1;30m(無)\033[m");

  move(8, 0);
  clrtobot();

  /* floatJ.080308: 修改視覺樣式 */

  /* dust.071126: 這裡的符號不太被vim支援，修改時請小心"幻視"現象 */
  outs("\033[1;35m▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁\033[m\n");
  outs("\033[1;33;45m★\033[37m   以下是您所查詢的文章資訊　　　　                                         \033[m\n");

  prints("\n\033[1m編號: \033[m%d", xo->pos+1);

  outs("\n\033[1m標題: \033[m");
  if (chkrestrict(ghdr) || (currbattr & BRD_XRTITLE))
    outs(ghdr->title);
  else
    outs(RESTRICTED_TITLE);

  prints("\n\033[1m作者: \033[m%s", ghdr->owner);
  prints("\n\033[1m暱稱: \033[m%s", ghdr->nick);
  prints("\n\033[1m日期: \033[m%s", ghdr->date);
  prints("\n\033[1m屬性: \033[m%s", xmodes);

  outs("\n\033[1m路徑: \033[m");
  if(chkrestrict(ghdr))
    outs(fpath);
  else
    outs("\033[31m(Sorry, but you don\'t have the permission.)\033[m");

  mode = stat(fpath, &st);
  outs("\n\033[1m大小: \033[m");
  if(!mode)
    prints("%d byte%s", st.st_size, st.st_size > 1 ? "s" : "");
  else
    outs("\033[1;30m(讀取失敗)\033m");

  outs("\n\033[1mMDT : \033[m");
  if(!mode)
    outs(Btime(&st.st_mtime));
  else
    outs("\033[1;30m(讀取失敗)\033m");

  prints("\n\033[1mCHT : \033[m%s (%d)", Btime(&ghdr->chrono), ghdr->chrono);
  prints("\n\033[1mSTT : \033[m%s (%d)", Btime(&ghdr->stamp), ghdr->stamp);

  outs("\n\033[35m───────────────────────────────────────\033[m");
  vmsg(NULL);

  return post_body(xo);
}


static int
post_aidjump(xo)	/* dust.091215: AID跳躍功能(ver 1.1) */
  XO *xo;
{
  char aid[32], *brdname, fpath[64];
  int pos, max, szbuf, chrono, i, match, newbno;
  usint ufo_backup;
  HDR *buffer;
  FILE *fp;


  if (!vget(b_lines, 0, "請輸入文章代碼(AID)： #", aid, sizeof(aid), DOECHO))
    return XO_FOOT;

  /* AID解碼 */
  chrono = aid_decode(aid);
  if(chrono == -1)	/* 無法解析成chrono，代表所輸入的AID格式錯誤 */
  {
    vmsg("這不是正確格式的AID，請重新輸入");
    return XO_FOOT;
  }

  /* 板名解析 (AID jump ver 1.1 特有功能) */
  brdname = strchr(aid, '(');
  if(brdname)
  {
    strtok(brdname++, ")");	/* 標準格式會帶有括號，去掉 */
    if(valid_brdname(brdname))
    {
      newbno = brd_bno(brdname);
      if(newbno >= 0 && (brd_bits[newbno] & BRD_L_BIT))
      {
	if(newbno == currbno)	/* 是當前看板，不需要換xo_pool[] */
	  brdname = NULL;
	else if(!(brd_bits[newbno] & BRD_R_BIT))
	{
	  vmsg("無法閱\讀此看板");
	  return XO_FOOT;
	}
      }
      else
      {
	vmsg("此看板不存在");
	return XO_FOOT;
      }
    }
    else
    {
      vmsg("這不是正確格式的AID，請重新輸入");
      return XO_FOOT;
    }
  }

  if(brdname)
    brd_fpath(fpath, brdname, fn_dir);
  else
    brd_fpath(fpath, currboard, fn_dir);

  max = szbuf = rec_num(fpath, sizeof(HDR));

  if(max <= 0)		/* 該看板沒有文章就直接當成找不到文章 */
  {
    vmsg("找不到文章");
    return XO_FOOT;
  }

  if(!(fp = fopen(fpath, "rb")))
  {
    move(b_lines, 0);
    clrtoeol();
    outs("AID jump error: 無法開啟此看板的索引檔 ");
    vkey();
    return XO_FOOT;
  }

  /* 決定 I/O buffer 的大小 */
  do
  {
    buffer = (HDR*) malloc(sizeof(HDR) * szbuf);
    szbuf = szbuf/2 + 1;
  }while(!buffer && szbuf > 2);

  if(!buffer)
  {
    fclose(fp);
    move(b_lines, 0);
    clrtoeol();
    outs("AID jump error: 搜尋用空間配置失敗。請再試一次或稍後再試 ");
    vkey();
    return XO_FOOT;
  }

  /* 搜尋程序 */
  match = 0;
  i = szbuf;
  for(pos = 0; pos < max; pos++)
  {
    if(i >= szbuf)
    {
      fread(buffer, szbuf, sizeof(HDR), fp);
      i = 0;
    }

    if(buffer[i].chrono == chrono)
    {
      match = 1;
      break;
    }

    i++;
  }

  free(buffer);
  fclose(fp);

  if(!match)	/* 沒找到文章 */
  {
    vmsg("找不到文章");
    return XO_FOOT;
  }

  if(brdname)		/* AID jump v1.1: 跨看板式AID */
  {
    brd_bits[newbno] |= BRD_V_BIT;
    ufo_backup = cuser.ufo;
    cuser.ufo &= ~UFO_BRDNOTE;
    XoPost(newbno);
    cuser.ufo = ufo_backup;
  }

  xz[XZ_POST - XO_ZONE].xo->pos = pos;	/* 跳至該篇文章 */
  return XZ_POST;
}


int
post_thelastNBpost(xo)
  XO *xo;
{
  int pos, cur;
  FILE *fp;
  HDR hdr;

  cur = xo->max - 1;
  pos = xo->pos;

  if(cur > pos && (fp = fopen(xo->dir, "rb")))
  {
    fseeko(fp, -sizeof(HDR), SEEK_END);

    do
    {
      if(fread(&hdr, 1, sizeof(HDR), fp) <= 0)
	break;

      if(!(hdr.xmode & POST_BOTTOM))
	break;

      fseeko(fp, -(sizeof(HDR) * 2), SEEK_CUR);
    }while(--cur > pos);

    fclose(fp);
  }

  return cur <= pos? xo->max - 1 : cur;
}



static int
post_help(xo)
  XO *xo;
{
  xo_help("post");
  /* return post_head(xo); */
  return XO_HEAD;		/* itoc.001029: 與 xpost_help 共用 */
}


KeyFunc post_cb[] =
{
  XO_INIT, post_init,
  XO_LOAD, post_load,
  XO_HEAD, post_head,
  XO_BODY, post_body,

  'r', post_browse,
  's', post_switch,
  KEY_TAB, post_gem,
  'z', post_gem,

  'y', post_reply,
  'd', post_delete,
  'v', post_visit,
  'x', post_cross,		/* 在 post/mbox 中都是小寫 x 轉看板，大寫 X 轉使用者 */
  'X', post_forward,
  't', post_tag,
  'E', post_edit,
  'T', post_title,
  'm', post_mark,
  '_', post_bottom,
  'D', post_rangedel,
  '#', post_aidjump,
  '@', post_spms,
  '!', post_reserve,

#ifdef HAVE_SCORE
  '%', post_score,
  'i', post_resetscore,
#endif

  'w', post_write,

  'b', post_memo,
  'c', post_copy,
  'C', post_tbf,
  'g', gem_gather,
  'Q', post_query,
  Ctrl('B'), post_state,
  
  Ctrl('P'), post_add,
  Ctrl('D'), post_prune,
  Ctrl('Q'), xo_uquery,
  Ctrl('O'), xo_usetup,
  Ctrl('S'), post_done_mark,    /* qazq.030815: 站方處理完成標記Ｓ*/
#ifdef HAVE_REFUSEMARK
  Ctrl('Y'), post_refuse,
#endif

#ifdef HAVE_LABELMARK
  'n', post_label,
  Ctrl('N'), post_delabel,
#endif

  'B' | XO_DL, (void *) "bin/manage.so:post_manage",
  'R' | XO_DL, (void *) "bin/vote.so:vote_result",
  'V' | XO_DL, (void *) "bin/vote.so:XoVote",
  'I' | XO_DL, (void *) "bin/manage.so:post_binfo",

#ifdef HAVE_TERMINATOR
  Ctrl('X') | XO_DL, (void *) "bin/manage.so:post_terminator",
#endif

  '~', XoXselect,		/* itoc.001220: 搜尋作者/標題 */
  'S', XoXsearch,		/* itoc.001220: 搜尋相同標題文章 */
  'a', XoXauthor,		/* itoc.001220: 搜尋作者 */
  '/', XoXtitle,		/* itoc.001220: 搜尋標題 */
  'f', XoXfull,			/* itoc.030608: 全文搜尋 */
  'G', XoXmark,			/* itoc.010325: 搜尋 mark 文章 */
  Ctrl('G'), XoXuni,
  'L', XoXlocal,		/* itoc.010822: 搜尋本地文章 */

#ifdef HAVE_XYNEWS
  'u', XoNews,			/* itoc.010822: 新聞閱讀模式 */
#endif

#ifdef MOVE_CAPABLE
  'M', post_move,
#endif

  'h', post_help
};


KeyFunc xpost_cb[] =
{
  XO_INIT, xpost_init,
  XO_LOAD, xpost_load,
  XO_HEAD, xpost_head,
  XO_BODY, post_body,		/* Thor.980911: 共用即可 */

  'r', xpost_browse,
  'y', post_reply,
  't', post_tag,
  'x', post_cross,
  'X', post_forward,
  'c', post_copy,
  'C', post_tbf,
  'g', gem_gather,
  'm', post_mark,
  '@', post_spms,
  '!', post_reserve,
  'd', post_delete,		/* Thor.980911: 方便板主 */
  'E', post_edit,		/* itoc.010716: 提供 XPOST 中可以編輯標題、文章，加密 */
  'T', post_title,
#ifdef HAVE_SCORE
  '%', post_score,
  'i', post_resetscore, 
#endif
  'w', post_write,
  'b', post_memo,
  'Q', post_query,
  Ctrl('B'), post_state,
#ifdef HAVE_REFUSEMARK
  Ctrl('Y'), post_refuse,
#endif
#ifdef HAVE_LABELMARK
  'n', post_label,
#endif

  '~', XoXselect,
  'S', XoXsearch,
  'a', XoXauthor,
  '/', XoXtitle,
  'f', XoXfull,
  'G', XoXmark,
  'L', XoXlocal,
  Ctrl('G'), XoXuni,
  'I' | XO_DL, (void *) "bin/manage.so:post_binfo",

  Ctrl('P'), post_add,
  Ctrl('D'), post_prune,
  Ctrl('Q'), xo_uquery,
  Ctrl('O'), xo_usetup,
  Ctrl('S'), post_done_mark,    /* qazq.030815: 站方處理完成標記Ｓ*/

  'h', post_help		/* itoc.030511: 共用即可 */
};


#ifdef HAVE_XYNEWS
KeyFunc news_cb[] =
{
  XO_INIT, news_init,
  XO_LOAD, news_load,
  XO_HEAD, news_head,
  XO_BODY, post_body,

  'r', XoXsearch,

  'h', post_help		/* itoc.030511: 共用即可 */
};
#endif	/* HAVE_XYNEWS */

