/*-------------------------------------------------------*/
/* menu.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : menu/help/movie routines		 	 */
/* create : 95/03/29				 	 */
/* update : 10/11/02				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"



extern UCACHE *ushm;
extern FCACHE *fshm;


extern char **brd_levelbar;

/* floatJ.0090607: konami_counter 彩蛋用*/
static int konami_counter = 0;


static MCD_info curr_CDmeter;

void
hm_init()	/* dust.100315: 載入個人倒數計時器 */
{
  FILE *fp;
  char fpath[64];
  MCD_info meter;

  memset(&curr_CDmeter, 0, sizeof(MCD_info));

  usr_fpath(fpath, cuser.userid, FN_MCD);
  if(fp = fopen(fpath, "rb"))
  {
    while(fread(&meter, sizeof(MCD_info), 1, fp) > 0)
    {						/* 篩掉已經截止的計時器 */
      if((meter.warning & DEFMETER_BIT) && DCD_AlterVer(&meter, NULL) > 0)
      {
	meter.warning &= ~DEFMETER_BIT;	/* 還原回原本的數字 */
	curr_CDmeter = meter;
	break;
      }
    }
    fclose(fp);
  }
}


/* ----------------------------------------------------- */
/* 離開 BBS 站						 */
/* ----------------------------------------------------- */


#define	FN_RUN_NOTE_PAD	"run/note.pad"
#define	FN_RUN_NOTE_TMP	"run/note.tmp"


typedef struct
{
  time_t tpad;
  char msg[400];
}      Pad;


int
pad_view()
{
  int fd, count;
  Pad *pad;

  if ((fd = open(FN_RUN_NOTE_PAD, O_RDONLY)) < 0)
    return XEASY;

  count = 0;
  mgets(-1);

  for (;;)
  {
    pad = mread(fd, sizeof(Pad));
    if (!pad)
    {
      vmsg(NULL);
      break;
    }
    else if (!(count % 5))	/* itoc.020122: 有 pad 才印 */
    {
      clear();
      move(0, 23);
      prints("【 極 光 鯨 藍 留 言 板 】                第 %d 頁\n\n", count / 5 + 1);
    }

    outs(pad->msg);
    count++;

    if (!(count % 5))
    {
      move(b_lines, 0);
      outs("請按 [SPACE] 繼續觀賞，或按其他鍵結束： ");
      /* itoc.010127: 修正在偵測左右鍵全形下，按左鍵會跳離二層選單的問題 */

      if (vkey() != ' ')
	break;
    }
  }

  close(fd);
  return 0;
}


static int
pad_draw()
{
  int i, cc, fdr, color;
  FILE *fpw;
  Pad pad;
  char *str, buf[3][71];

  /* itoc.註解: 不想用高彩度，太花 */
  static char pcolors[6] = {31, 32, 33, 34, 35, 36};

  /* itoc.010309: 留言板提供不同的顏色 */
  color = vans("心情顏色 1) \033[41m  \033[m 2) \033[42m  \033[m 3) \033[43m  \033[m "
    "4) \033[44m  \033[m 5) \033[45m  \033[m 6) \033[46m  \033[m [Q] ");

  if (color < '1' || color > '6')
    return XEASY;
  else
    color -= '1';

  do
  {
    buf[0][0] = buf[1][0] = buf[2][0] = '\0';
    move(MENU_XPOS, 0);
    clrtobot();
    outs("\n請留言 (至多三行)，按[Enter]結束");
    for (i = 0; (i < 3) &&
      vget(16 + i, 0, "：", buf[i], 71, DOECHO); i++);
    cc = vans("(S)存檔觀賞 (E)重新來過 (Q)算了？[S] ");
    if (cc == 'q' || i == 0)
      return 0;
  } while (cc == 'e');

  time(&pad.tpad);

  /* itoc.020812.註解: 改版面的時候要注意 struct Pad.msg[] 是否夠大 */
  str = pad.msg;
  sprintf(str, "╭┤\033[1;46m %s － %s \033[m├", cuser.userid, cuser.username);

  for (cc = strlen(str); cc < 60; cc += 2)
    strcpy(str + cc, "─");
  if (cc == 60)
    str[cc++] = ' ';

  sprintf(str + cc,
    "\033[1;44m %s \033[m╮\n"
    "│  \033[1;%dm%-70s\033[m  │\n"
    "│  \033[1;%dm%-70s\033[m  │\n"
    "╰  \033[1;%dm%-70s\033[m  ╯\n",
    Btime(&pad.tpad),
    pcolors[color], buf[0],
    pcolors[color], buf[1],
    pcolors[color], buf[2]);

  f_cat(FN_RUN_NOTE_ALL, str);

  if (!(fpw = fopen(FN_RUN_NOTE_TMP, "w")))
    return 0;

  fwrite(&pad, sizeof(pad), 1, fpw);

  if ((fdr = open(FN_RUN_NOTE_PAD, O_RDONLY)) >= 0)
  {
    Pad *pp;

    i = 0;
    cc = pad.tpad - NOTE_DUE * 60 * 60;
    mgets(-1);
    while (pp = mread(fdr, sizeof(Pad)))
    {
      fwrite(pp, sizeof(Pad), 1, fpw);
      if ((++i > NOTE_MAX) || (pp->tpad < cc))
	break;
    }
    close(fdr);
  }

  fclose(fpw);

  rename(FN_RUN_NOTE_TMP, FN_RUN_NOTE_PAD);
  pad_view();
  return 0;
}


static int
goodbye()
{
  /* itoc.010803: 秀張離站的圖 */
  clear();
  film_out(FILM_GOODBYE, 0);

  switch (vans("G)隨光而逝 M)報告站長 N)留言板 Q)取消？[Q] "))
  {
  /* lkchu.990428: 內定改為不離站 */
  case 'g':
  case 'y':
    break;    

  case 'm':
    m_sysop();
    break;

  case 'n':
    /* if (cuser.userlevel) */
    if (HAS_PERM(PERM_POST)) /* Thor.990118: 要能post才能留言, 提高門檻 */
      pad_draw();
    break;

  case 'b':
    /* floatJ.090607: konami_code 彩蛋 */
    if (konami_counter >= 6)
      more("etc/CharlieSP", MFST_NONE);
  case 'q':
  default:
    if(konami_counter >= 6)     /* dust.091010: 在b這步按錯的話不必考慮嗎... */
      konami_counter = 0;

    /* return XEASY; */
    return 0;	/* itoc.010803: 秀了 FILM_GOODBYE 要重繪 */
  }

#ifdef LOG_BMW
  bmw_log();			/* lkchu.981201: 水球記錄處理 */
#endif

  u_exit("EXIT ");
  exit(0);
}


/* ----------------------------------------------------- */
/* help & menu processring				 */
/* ----------------------------------------------------- */


void
vs_head(title, mid)
  char *title, *mid;
{
  static char fcheck[64];
  int mlen, left_space, right_space, spc;
  int title_len, currb_len;
  int ansi = 0;


  if (mid)	/* xxxx_head() 都是用 vs_head(title, str_site); */
  {
    clear();
  }
  else		/* menu() 中才用 vs_head(title, NULL); ，因為選單中無需 clear() */
  {
    move(0, 0);
    clrtoeol();
    mid = str_site;
  }

  if (!fcheck[0])
    usr_fpath(fcheck, cuser.userid, FN_PAYCHECK);

  if (HAS_STATUS(STATUS_BIFF))
  {
    mid = "\033[1;5;37;41m 郵差來叮咚囉 \033[m\033[1;44m";
    mlen = 14;
    ansi = 1;
  }
  else if (dashf(fcheck))
  {
    mid = "\033[5;33;43m 您有新支票噢 \033[m\033[1;44m"; 
    mlen = 14;
    ansi = 1;
  }
#ifdef USE_DAVY_REGISTER
  else if (HAS_PERM(PERM_ALLREG))
  {
    DIR *dir;
    struct dirent *fptr;
    int len;
    char flag = 0;

    dir = opendir(FN_REGISTER);

    while ((fptr = readdir(dir)) != NULL)
    {
      if (! ('a' <= fptr->d_name[0] && fptr->d_name[0] <= 'z') )
        continue;
      len = strlen(fptr->d_name);
      if (len > 4)
        if ((fptr->d_name[len - 4] == '.') && \
          (fptr->d_name[len - 3] == 'l') && \
          (fptr->d_name[len - 2] == 'c') && \
          (fptr->d_name[len - 1] == 'k'))
          continue;
      flag = 1;
      break;
    }
    closedir(dir);
    if (flag)
    {
      mid = "\033[5;33;45m 有人填註冊單耶 >///< (心) \033[m\033[1;44m";
      mlen = 27;
      ansi = 1;
    }
    else
      mlen = strlen(mid);
  }
#else
  else if (HAS_PERM(PERM_ALLREG) && rec_num(FN_RUN_RFORM, sizeof(RFORM)))
  {
    mid = "\033[5;33;45m 有人填註冊單耶 >///< (心) \033[m\033[1;44m";
    mlen = 27;
    ansi = 1;
  }
#endif
  else
    mlen = strlen(mid);

  spc = 78 + d_cols;
  title_len = strlen(title);
  currb_len = strlen(currboard);

  left_space = spc/2 - mlen/2 - title_len - 6;
  if(left_space <1)
    left_space = 1;

  right_space = 70 + d_cols - currb_len - spc/2 + mlen/2 - mlen;
  if(right_space <1)
    right_space = 1;

  spc = title_len + left_space + right_space + currb_len + 14;
  if(mlen + spc > 78 + d_cols)
    mlen = 78 + d_cols - spc;

  if(ansi)
    mlen = 999;

  /* 印出 header */
  prints("\033[32m::\033[1m%s\033[;32;40m:: \033[1;36;44m%*s%.*s%*s\033[30;40m 看板%s%s%s\033[m\n",
     title, left_space, "", mlen, mid, right_space, "", brd_levelbar[0], currboard, brd_levelbar[1]);
}



/* ------------------------------------- */
/* 動畫處理				 */
/* ------------------------------------- */

static char feeter[200];


/* dust.100313: 將數字縮寫成###K/###M/###G的形式 */
static int	/* 回傳字串長度 */
abbreviation(n, str)
  int n;
  char *str;	/* 長度必須>=6，否則有緩衝區溢位的可能 */
{
  char symbol;

  if(n < 10000)
  {
    n *= 10;
    symbol = '\0';
  }
  else if(n < 1000000)	/* < million */
  {
    n /= 100;
    symbol = 'K';
  }
  else if(n < 1000000000)	/* < billion */
  {
    n /= 100000;
    symbol = 'M';
  }
  else
  {
    n /= 100000000;
    symbol = 'G';
  }

  if(n < 100 && symbol != '\0')
    n = sprintf(str, "%.1f%c", n / 10.0, symbol) - 1;
  else
    n = sprintf(str, "%d%c", n / 10, symbol) - 1;

  return str[n] == '\0'?  n : n + 1;
}


/* dust.100312: Menu footer 樣式大變更 */
static void
status_foot()
{
  static time_t uptime = 0;

  char buf[64], *p, *foot = feeter;
  int space, left, normal_mode, len;
  time_t now;
  struct tm *ptime;
  MCD_info *pmeter;


  time(&now);
  ptime = localtime(&now);

  if(now > uptime)	/* 過子夜要更新生日旗標 */
  {
    if(uptime > 0)
    {
      if(cuser.day == ptime->tm_mday && cuser.month == ptime->tm_mon + 1)
	cutmp->status |= STATUS_BIRTHDAY;
      else
	cutmp->status &= ~STATUS_BIRTHDAY;
    }

    uptime = now + 86400 - ptime->tm_hour * 3600 - ptime->tm_min * 60 - ptime->tm_sec;
  }


  /* 前半部的foot */

  normal_mode = 1;
  if(cuser.ufo & UFO_MFMCOMER)	/* 金融模式 */
  {
    normal_mode = 0;

    foot += sprintf(foot, COLOR1 " %s %02d\033[5m:\033[m" COLOR1 "%02d ", fshm->date, ptime->tm_hour, ptime->tm_min);

    space = 5 - abbreviation(cuser.gold, buf);
    if(space > 2) space = 2;
    foot += sprintf(foot, "\033[33;43m金幣 %3s%*s", buf, space, "");

    space = 5 - abbreviation(cuser.money, buf);
    if(space > 2) space = 2;
    foot += sprintf(foot, "\033[37;43m銀幣 %3s%*s\033[44m", buf, space, "");
  }
		/* 倒數模式(官方) */
  else if((cuser.ufo & UFO_MFMGCTD) && fshm->counter.name[0] != '\0')
  {
    pmeter = &fshm->counter;
    goto print_RemainingTime;
  }
		/* 倒數模式(自訂) */
  else if((cuser.ufo & UFO_MFMUCTD) && curr_CDmeter.name[0] != '\0')
  {
    pmeter = &curr_CDmeter;

print_RemainingTime:
    left = DCD_AlterVer(pmeter, buf + 49);

    if(left > 0)
    {
      normal_mode = 0;
      if(left <= pmeter->warning)
	left = 1;
      else
	left = 0;

      space = 17 - strlen(pmeter->name);
      p = strtok(buf + 49, "d");
      strcpy(buf, p);
      len = strlen(p);
      space -= len + 1;
      if(len <= 5)	/* _____d__h or __d__h__m */
      {
	sprintf(buf + len, "\033[30md\033[33m%s\033[30mh", strtok(NULL, "h"));
	space -= 3;
	if(len <= 2)
	{
	  sprintf(buf + len + 19, "\033[33m%s\033[30mm", strtok(NULL, "m"));
	  space -= 3;
	}
      }
      else		/* ________D */
	strcpy(buf + len, "\033[30mD");

      if(space < 0) space = 0;

      foot += sprintf(foot, COLOR1 " %s %02d\033[5m:\033[m" COLOR1 "%02d ", fshm->date, ptime->tm_hour, ptime->tm_min);

      foot += sprintf(foot, "\033[%sm%s\033[%s30m剩\033[33m%s \033[44m%*s",
	left? "37;41" : "", pmeter->name, left? "" : "1;", buf, space, "");
    }
  }

  if(normal_mode)
  {
    if(fshm->otherside)
    {
      normal_mode = 0;
      space = strlen(fshm->today);
      left = (20 - space) / 2;
      foot += sprintf(foot, COLOR1 " %s %02d\033[5m:\033[m" COLOR1 "%02d \033[37;45m%*s%-*s\033[44m",
	fshm->date, ptime->tm_hour, ptime->tm_min, left, "", 20 - left, fshm->today);
    }
    else
    {
      space = 20 - strlen(fshm->today);
      if(space > 12)
	space = 12;
      else if(space < 0)
	space = 0;
      foot += sprintf(foot, COLOR1 " %8s %02d\033[5m:\033[m" COLOR1 "%02d %s%*s",
	fshm->today, ptime->tm_hour, ptime->tm_min, FOOT_FIX, space, "");
    }
  }


  /* 後半部的foot */

  if(abbreviation(total_user, buf) < 4)
    space = 3;
  else 
    space = 2;

  foot += sprintf(foot, " \033[1;30m線上\033[37m %-3s\033[30m人%*s我是\033[37m %-*s\033[30m[水球]\033[3",
    buf, space, "", normal_mode? 13 : 12, cuser.userid);

  if(cuser.ufo & UFO_QUIET)
    sprintf(foot, "1m防水 #");
  else if(cuser.ufo & UFO_PAGER)
  {
#ifdef HAVE_NOBROAD
    if(cuser.ufo & UFO_RCVER)
      sprintf(foot, "2m好友 \033[33m!");
    else
#endif
      sprintf(foot, "2m好友 *");
  }
#ifdef HAVE_NOBROAD
  else if (cuser.ufo & UFO_RCVER)
    sprintf(foot, "2m拒廣 \033[33m-");
#endif
  else
    sprintf(foot, "2m接收  ");

  outf(feeter);
}


void
movie()
{
  /* Thor: it is depend on which state */
  if ((bbsmode <= M_XMENU) && (cuser.ufo & UFO_MOVIE))
    film_out(FILM_MOVIE, MENU_XNOTE);

  /* itoc.010403: 把 feeter 的 status 獨立出來 */
  status_foot();
}


typedef struct
{
  void *func;
  /* int (*func) (); */
  usint level;
  int umode;
  char *desc;
}      MENU;


#define MENU_LOAD   1
#define MENU_DRAW   2
#define MENU_FILM   4


#define PERM_MENU   PERM_PURGE


static MENU menu_main[];


/* ----------------------------------------------------- */
/* administrator's maintain menu             */
/* ----------------------------------------------------- */

static MENU menu_admin[];


static MENU menu_auroral[] =
{
#ifdef SP_SEND_MONEY
  "bin/xyz.so:year_bonus", PERM_SYSOP, - M_SYSTEM,
  "YearBonus    ＄ 站長灑錢 ＄",
#endif

  "bin/admutil.so:circ_xfile", PERM_ALLADMIN, - M_XFILES,
  "CircKaminoACL☆ 召喚名單 ☆",

  "bin/circgod.so:a_circgod", PERM_ALLADMIN, - M_XFILES,
  "CircKamino   ☆召喚電研神☆",

  "bin/pset.so:hm_admin", PERM_SYSOP, - M_SYSTEM,
  "Hourmeter    設定倒數計時器",
#if 0
  test is over
  "bin/regmgr.so:a_regmgr", PERM_SYSOP, - M_SYSTEM,
  "RegisterMgr  ☆註冊單試爆☆",
#endif
  menu_admin, PERM_MENU + Ctrl('A'), M_AMENU,
  "極光特典"
};


static MENU menu_admin[] =
{
  "bin/admutil.so:a_user", PERM_ALLACCT, - M_SYSTEM,
  "User         ◤ 顧客資料 ◢",

  "bin/admutil.so:a_search", PERM_ALLACCT, - M_SYSTEM,
  "Hunt         ◤ 搜尋引擎 ◢",

  "bin/admutil.so:a_editbrd", PERM_ALLBOARD, - M_SYSTEM,
  "QSetBoard    ◤ 設定看板 ◢",

  "bin/innbbs.so:a_innbbs", PERM_ALLBOARD, - M_SYSTEM,
  "InnBBS       ◤ 轉信設定 ◢",

#ifdef HAVE_REGISTER_FORM
  "bin/admutil.so:a_register", PERM_ALLREG, - M_SYSTEM,
  "Register     ◤ 審註冊單 ◢",
# ifndef USE_DAVY_REGISTER /* davy.20120624: 新‧註冊單審核系統 */
  "bin/admutil.so:a_regmerge", PERM_ALLREG, - M_SYSTEM,
  "Merge        ◤ 復原審核 ◢",
# endif
#endif

  "bin/admutil.so:a_xfile", PERM_ALLADMIN, - M_XFILES,
  "Xfile        ◤ 系統檔案 ◢",

  menu_auroral, PERM_ALLADMIN,  M_AMENU,
  "Auroral      ◤ 極光特典 ◢",

  "bin/admutil.so:a_resetsys", PERM_ALLADMIN, - M_SYSTEM,
  "BBSreset     ◤ 重置系統 ◢",
/*
  "bin/admutil.so:a_cmd", PERM_ALLADMIN, - M_SYSTEM,
  "Commands     ◤ 系統指令 ◢",
*/
  "bin/admutil.so:a_restore", PERM_SYSOP, - M_SYSTEM,
  "TRestore     ◤ 還原備份 ◢",

  menu_main, PERM_MENU + Ctrl('A'), M_AMENU,
  "系統維護"
};

/* ----------------------------------------------------- */



/* ----------------------------------------------------- */
/* mail menu                         */
/* ----------------------------------------------------- */


static int
XoMbox()
{
  xover(XZ_MBOX);
  return 0;
}


static MENU menu_mail[] =
{
  XoMbox, PERM_BASIC, M_RMAIL,
  "Read         ├ 閱\讀信件 ┤",

  m_send, PERM_LOCAL, M_SMAIL,
  "Mail         ├ 站內寄信 ┤",

#ifdef MULTI_MAIL  /* Thor.981009: 防止愛情幸運信 */
  m_list, PERM_LOCAL, M_SMAIL,
  "List         ├ 群組寄信 ┤",
#endif

  m_internet, PERM_INTERNET, M_SMAIL,
  "Internet     ├ 寄依妹兒 ┤",

#ifdef HAVE_SIGNED_MAIL
  m_verify, 0, M_XMODE,
  "Verify       ├ 驗證信件 ┤",
#endif

#ifdef HAVE_MAIL_ZIP
  m_zip, PERM_INTERNET, M_SMAIL,
  "Zip          ├ 打包資料 ┤",
#endif

  m_sysop, 0, M_SMAIL,
  "Yes Sir!     ├ 投書站長 ┤",

  "bin/admutil.so:m_bm", PERM_ALLADMIN, - M_SMAIL,
  "BM All       ├ 板主通告 ┤",    /* itoc.000512: 新增 m_bm */

  "bin/admutil.so:m_all", PERM_ALLADMIN, - M_SMAIL,
  "User All     ├ 全站通告 ┤",    /* itoc.000512: 新增 m_all */

  menu_main, PERM_MENU + Ctrl('A'), M_MMENU,    /* itoc.020829: 怕 guest 沒選項 */
  "電子郵件"
};


/* ----------------------------------------------------- */
/* talk menu                         */
/* ----------------------------------------------------- */


static int
XoUlist()
{
  xover(XZ_ULIST);
  return 0;
}


static MENU menu_talk[];


  /* --------------------------------------------------- */
  /* list menu                       */
  /* --------------------------------------------------- */

static MENU menu_list[] =
{
  t_pal, PERM_BASIC, M_PAL,
  "Pal          → 朋友名單 ←",

#ifdef HAVE_LIST
  t_list, PERM_BASIC, M_PAL,
  "List         → 特別名單 ←",
#endif

#ifdef HAVE_ALOHA
  "bin/aloha.so:t_aloha", PERM_PAGE, - M_PAL,
  "Aloha        → 上站通知 ←",
#endif

#ifdef LOGIN_NOTIFY
  t_loginNotify, PERM_PAGE, M_PAL,
  "Notify       → 系統協尋 ←",
#endif

  menu_talk, PERM_MENU + 'P', M_TMENU,
  "各類名單"
};


static MENU menu_talk[] =
{
  XoUlist, 0, M_LUSERS,
  "Users        → 遊客名單 ←",

  menu_list, PERM_BASIC, M_TMENU,
  "ListMenu     → 設定名單 ←",

  t_pager, PERM_BASIC, M_XMODE,
  "Pager        → 切換呼叫 ←",

  t_cloak, PERM_VALID, M_XMODE,
  "Invis        → 切換隱身 ←",

  t_query, 0, M_QUERY,
  "Query        → 查詢網友 ←",

  t_talk, PERM_PAGE, M_PAGE,
  "Talk         → 情話綿綿 ←",

  /* Thor.990220: 改採外掛 */
  "bin/chat.so:t_chat", PERM_CHAT, - M_CHAT,
  "ChatRoom     → 眾口鑠金 ←",

  t_display, PERM_BASIC, M_BMW,
  "Display      → 瀏覽水球 ←",

  t_bmw, PERM_BASIC, M_BMW,
  "Write        → 回顧水球 ←",

  menu_main, PERM_MENU + 'U', M_TMENU,
  "休閒聊天"
};


/* ----------------------------------------------------- */
/* user menu                         */
/* ----------------------------------------------------- */


static MENU menu_user[];


  /* --------------------------------------------------- */
  /* register menu                                      */
  /* --------------------------------------------------- */

static MENU menu_register[] =
{
#ifdef EMAIL_JUSTIFY
  u_addr, PERM_BASIC, M_XMODE,
  "Address      《 電子信箱 》",
#endif

#ifdef HAVE_REGISTER_FORM
  u_register, PERM_BASIC, M_UFILES,
  "Register     《 填註冊單 》",
#endif

#ifdef HAVE_REGKEY_CHECK
  u_verify, PERM_BASIC, M_UFILES,
  "Verify       《 填認證碼 》",
#endif

  u_deny, PERM_BASIC, M_XMODE,
  "Perm         《 恢復權限 》",

  menu_user, PERM_MENU + 'P', M_UMENU,
  "註冊選單"
};


static MENU menu_user[] =
{
  u_info, PERM_BASIC, M_XMODE,
  "Info         《 個人資料 》",

  u_setup, 0, M_UFILES,
  "Habit        《 喜好模式 》",

  menu_register, PERM_BASIC, M_UMENU,
  "Register     《 註冊選單 》",

  u_xfile, PERM_BASIC, M_UFILES,
  "Xfile        《 個人檔案 》",

  u_lock, PERM_BASIC, M_IDLE,
  "Lock         《 鎖定螢幕 》",

  pad_view, 0, M_READA,
  "Note         《 觀看留言 》",

  /* itoc.010309: 不必離站可以寫留言板 */
  pad_draw, PERM_POST, M_POST,
  "Pad          《 心情塗鴉 》",

  u_log, PERM_BASIC, M_UFILES,
  "ViewLog      《 上站紀錄 》",

  menu_main, PERM_MENU + 'H', M_UMENU,
  "個人設定"
};


#ifdef HAVE_EXTERNAL

/* ----------------------------------------------------- */
/* tool menu                         */
/* ----------------------------------------------------- */


static MENU menu_tool[];


#ifdef HAVE_SONG
  /* --------------------------------------------------- */
  /* song menu                       */
  /* --------------------------------------------------- */

static MENU menu_song[] =
{
  "bin/song.so:XoSongLog", 0, - M_XMODE,
  "KTV          ♂ 點歌紀錄 ♀",

  "bin/song.so:XoSongMain", 0, - M_XMODE,
  "Book         ♂ 唱所欲言 ♀",

  "bin/song.so:XoSongSub", 0, - M_XMODE,
  "Note         ♂ 歌本投稿 ♀",

  menu_tool, PERM_MENU + 'K', M_XMENU,
  "玩點歌機"
};
#endif


#ifdef HAVE_GAME

#if 0

  itoc.010426.註解:
  益智遊戲不用賭金制度，讓玩家玩好玩的，只加錢，不減錢。

  itoc.010714.註解:
  (a) 每次玩遊戲的總期望值應在 1.01，一個晚上約可玩 100 次遊戲，
      若將總財產投入去玩遊戲，則 1.01^100 = 2.7 倍/每玩一個晚上。
  (b) 若各項機率不均等，也應維持在 1.0 ~ 1.02 之間，讓玩家一定能賺錢，
      且若一直押最高期望值的那一項，也不會賺得過於離譜。
  (c) 原則上，機率越低者其期望值應為 1.02，機率較高者其期望值應為 1.01。

  itoc.011011.註解:
  為了避免 user multi-login 玩來洗錢，
  所以在玩遊戲的開始就要檢查是否重覆 login 即 if (HAS_STATUS(STATUS_COINLOCK))。

#endif

  /* --------------------------------------------------- */
  /* game menu                       */
  /* --------------------------------------------------- */

static MENU menu_game[];

static MENU menu_game1[] =
{
  "bin/mine.so:main_mine", 0, - M_GAME,
  "0MineSweeper  ─ 踩地雷 ─ ",

  "bin/liteon.so:main_liteon", 0, - M_GAME,
  "1LightOn     ♂ 房間開燈 ♀",

  "bin/guessnum.so:guessNum", 0, - M_GAME,
  "2GuessNum    ♂ 玩猜數字 ♀",

  "bin/guessnum.so:fightNum", 0, - M_GAME,
  "3FightNum    ♂ 互猜數字 ♀",

  "bin/km.so:main_km", 0, - M_GAME,
  "4KMChess     ♂ [孔明棋] ♀",

  "bin/recall.so:main_recall", 0, - M_GAME,
  "5Memorize    ♂ 記憶之間 ♀",

  "bin/fantan.so:main_fantan", 0, - M_GAME,
  "6Fantan      ├ 番攤接龍 ┤",

  "bin/dragon.so:main_dragon", 0, - M_GAME,
  "7Dragon      ♂ 接龍遊戲 ♀",

   "bin/nine.so:main_nine", 0, - M_GAME,
  "8Nine        ♂ 天地九九 ♀",

  menu_game, PERM_MENU + '0', M_XMENU,
  "益智天堂"
};

static MENU menu_game2[] =
{
  "bin/dice.so:main_dice", 0, - M_GAME,
  "0Dice        ♂ 狂亂骰子 ♀",

  "bin/gp.so:main_gp", 0, - M_GAME,
  "1GooodPoker  ♂ 好人撲克 ♀",

  "bin/bj.so:main_bj", 0, - M_GAME,
  "2BlackJack   ♂ 二十一點 ♀",

#if 0
  /* dust.090305: 會議決議將這個東西和諧掉。 */
  "bin/chessmj.so:main_chessmj", 0, - M_GAME,
  "3ChessMJ     ♂ 象棋麻將 ♀",
#endif

  "bin/seven.so:main_seven", 0, - M_GAME,
  "4Seven       ♂ 賭城七張 ♀",

  "bin/race.so:main_race", 0, - M_GAME,
  "5Race        ♂ 進賽馬場 ♀",

  "bin/bingo.so:main_bingo", 0, - M_GAME,
  "6Bingo       ♂ 賓果大戰 ♀",

  "bin/marie.so:main_marie", 0, - M_GAME,
  "7Marie       ♂ 大小瑪莉 ♀",

  "bin/bar.so:main_bar", 0, - M_GAME,
  "8Bar         ♂ 吧台瑪莉 ♀",

  menu_game, PERM_MENU + '0', M_XMENU,
  "博奕城市"
};

static MENU menu_game3[] =
{
  "bin/pip.so:main_pip", PERM_BASIC, - M_GAME,
  "0Chicken     ♂ 電子小雞 ♀",

  "bin/pushbox.so:main_pushbox", 0, - M_GAME,
  "1PushBox     ♂ 倉庫番番 ♀",

  "bin/tetris.so:main_tetris", 0, - M_GAME,
  "2Tetris      ♂ 俄羅斯塊 ♀",

  "bin/gray.so:main_gray", 0, - M_GAME,
  "3Gray        ♂ 淺灰大戰 ♀",

  "bin/tetravex.so:main_tetra", 0, - M_GAME,
  "4TetraVex    ♂ 四方惱怒 ♀",

  "bin/sevens.so:main_sevens", 0, - M_GAME,
  "5SEVENS      ７ 排七遊戲 ７",

  "bin/gray2.so:main_gray2", 0, - M_GAME,
  "6GRAY2       ♂ 淺灰 II  ♀",

  menu_game, PERM_MENU + '0', M_XMENU,
  "玩家特區"
};

static MENU menu_game[] =
{
  menu_game1, PERM_BASIC, M_XMENU,
  "1Rhapsody    【 益智天堂 】",

  menu_game2, PERM_BASIC, M_XMENU,
  "2Esoteria    【 博奕城市 】",

  menu_game3, PERM_BASIC, M_XMENU,
  "3GamerSAR    【 玩家特區 】",

  menu_tool, PERM_MENU + '1', M_XMENU,
  "遊戲人生"
};
#endif


#ifdef HAVE_BUY
  /* --------------------------------------------------- */
  /* buy menu                        */
  /* --------------------------------------------------- */

static MENU menu_buy[] =
{
  "bin/bank.so:x_bank", PERM_BASIC, - M_GAME,
  "Bank         ♂ 極光銀行 ♀",

#if 0
  "bin/bank.so:b_invis", PERM_BASIC, - M_GAME,
  "Invis        ♂ 暫時隱身 ♀",

  "bin/bank.so:b_cloak", PERM_BASIC, - M_GAME,
  "Cloak        ♂ 無限隱身 ♀",
#endif

  "bin/bank.so:b_mbox", PERM_BASIC, - M_GAME,
  "Mbox         ♂ 信箱無限 ♀",

  "bin/bank.so:b_xempt", PERM_BASIC, - M_GAME,
  "Xempt        ♂ 永久保留 ♀",

  "bin/bank.so:b_buyhost", PERM_BASIC, - M_GAME,
  "Host         ♂ 自訂故鄉 ♀",

  "bin/bank.so:b_buylabel", PERM_BASIC, - M_GAME,
  "Label        ♂ 自訂標籤 ♀",

  "bin/bank.so:b_buysign", PERM_BASIC, - M_GAME,
  "Signature    擴充簽名檔數量",

  menu_tool, PERM_MENU + 'B', M_XMENU,
  "金融市場"
};

#endif


  /* --------------------------------------------------- */
  /* other tools menu                    */
  /* --------------------------------------------------- */

static MENU menu_other[] =
{
#if 0   /* dust.090429: 因為現在的設計與舊版不相容的緣故，暫時拿掉 */
  "bin/vote.so:vote_all", PERM_BASIC, - M_VOTE, /* itoc.010414: 投票中心 */
  "VoteAll      ♂ 投票中心 ♀",
#endif

#ifdef HAVE_TIP
  "bin/xyz.so:x_tip", 0, - M_READA,
  "Tip          ♂ 教學精靈 ♀",
#endif

#ifdef HAVE_LOVELETTER
  "bin/xyz.so:x_loveletter", 0, - M_READA,
  "LoveLetter   ♂ 情書撰寫 ♀",
#endif

  "bin/xyz.so:x_password", PERM_VALID, - M_XMODE,
  "Password     ♂ 忘記密碼 ♀",

#ifdef HAVE_CLASSTABLE
  "bin/classtable.so:main_classtable", PERM_BASIC, - M_XMODE,
  "ClassTable   ♂ 功\課時段 ♀",
#endif

#ifdef HAVE_CREDIT
  "bin/credit.so:main_credit", PERM_BASIC, - M_XMODE,
  "MoneyNote    ♂ 記帳手札 ♀",
#endif

#ifdef HAVE_CALENDAR
  "bin/todo.so:main_todo", PERM_BASIC, - M_XMODE,
  "XTodo        ♂ 個人行程 ♀",

  "bin/calendar.so:main_calendar", 0, - M_XMODE,
  "YCalendar    ♂ 萬年月曆 ♀",
#endif

  "bin/xyz.so:pushdoll_maker", PERM_POST, - M_XMODE,
  "RPushdoll    ♂ 推文娃娃 ♀",

  menu_tool, PERM_MENU + Ctrl('A'), M_XMENU,    /* itoc.020829: 怕 guest 沒選項 */
  "其他功\能"
};


static MENU menu_tool[] =
{
#ifdef HAVE_BUY
  menu_buy, PERM_BASIC, M_XMENU,
  "Market       【 金融市場 】",
#endif

#ifdef HAVE_SONG
  menu_song, 0, M_XMENU,
  "KTV          【 真情點歌 】",
#endif

#ifdef HAVE_COSIGN
  "bin/newbrd.so:XoNewBoard", PERM_VALID, - M_XMODE,
  "Join         【 看板連署 】",
#endif

#ifdef HAVE_GAME
  menu_game, PERM_BASIC, M_XMENU,
  "Game         【 遊戲人生 】",
#endif

  menu_other, 0, M_XMENU,
  "Other        【 雜七雜八 】",

/* floatJ: 建立站誤測試用的項目 */
#if 0
  不用了
  "bin/gray2.so:main_gray2", PERM_ALLADMIN, - M_XMODE,
  "ADMIN TEST    > 站誤測試 < ",
#endif
  menu_main, PERM_MENU + Ctrl('A'), M_XMENU,    /* itoc.020829: 怕 guest 沒選項 */
  "個人工具"
};

#endif  /* HAVE_EXTERNAL */


/* ----------------------------------------------------- */
/* main menu                         */
/* ----------------------------------------------------- */


static int
Gem()
{
  /* itoc.001109: 看板總管在 (A)nnounce 下有 GEM_X_BIT，方便開板 */
  XoGem("gem/"FN_DIR, "精華佈告欄", (HAS_PERM(PERM_ALLBOARD) ? (GEM_W_BIT | GEM_X_BIT | GEM_M_BIT) : 0));
  return 0;
}


static MENU menu_main[] =
{
  menu_admin, PERM_ALLADMIN, M_AMENU,
  "0Admin      Φ 站誤主控台 Φ",

  Gem, 0, M_GEM,
  "Announce    ξ 精華公佈欄 ξ",

  Boards, 0, M_BOARD,
  "Boards      Ω 看板列表區 Ω",

  Class, 0, M_BOARD,
  "Class       φ 看板分組區 φ",

#ifdef MY_FAVORITE
  MyFavorite, PERM_BASIC, M_MF,
  "Favorite    η 我的最愛群 η",
#endif

  menu_mail, 0, M_MMENU,
  "Mail        μ 信件典藏盒 μ",

  menu_talk, 0, M_TMENU,
  "Talk        ω 休閒聊天地 ω",

  menu_user, 0, M_UMENU,
  "User        π 個人工具箱 π",

#ifdef HAVE_EXTERNAL
  menu_tool, 0, M_XMENU,
  "Xyz         θ 社辦招待所 θ",
#endif

#if 0   /* itoc.010209: 選單長度 */
  Select, 0, M_BOARD,
  "Select      σ 選擇主看板 σ",
#endif

  goodbye, 0, M_XMODE,
 "Goodbye~    δ 極光掰掰囉 δ",

  NULL, PERM_MENU + 'B', M_0MENU,
  "主功\能表"
};



static void             /* floatJ.090607: konami 彩蛋 */
konami_code(keycode)    /* 呼叫此函式並傳入keycode值 */
  int keycode;
{
  static char code_queue[6] = {38,38,40,40,37,39};

  if(konami_counter >= 6) konami_counter = 0;
  else if (keycode==code_queue[konami_counter])
    konami_counter++;
  else
    konami_counter = 0;
};


void
menu()
{
  MENU *menu, *mptr, *table[12];
  usint level, mode;
  int cc, cx;			/* current / previous cursor position */
  int max, mmx;			/* current / previous menu max */
  int cmd, depth;
  char *str;
  char mfr_mode_def[] = "Dq";
  char *mfr_mode[] = {mfr_mode_def, "Default    標準", "Economic   金融資訊", "Auroral    極光倒數", "User       自訂倒數", "Quit       離開", NULL};


  mode = MENU_LOAD | MENU_DRAW | MENU_FILM;
  menu = menu_main;
  level = cuser.userlevel;
  depth = mmx = 0;

  for (;;)
  {
    if (mode & MENU_LOAD)
    {
      for (max = -1;; menu++)
      {
	cc = menu->level;
	if (cc & PERM_MENU)
	{

#ifdef	MENU_VERBOSE
	  if (max < 0)		/* 找不到適合權限之功能，回上一層功能表 */
	  {
	    menu = (MENU *) menu->func;
	    continue;
	  }
#endif

	  break;
	}
	if (cc && !(cc & level))	/* 有該權限才秀出 */
	  continue;

	table[++max] = menu;
      }

      if (mmx < max)
	mmx = max;

      if ((depth == 0) && HAS_STATUS(STATUS_BIFF))	/* 第一次上站若有新信，進入 Mail 選單 */
	cmd = 'M';
      else
	cmd = cc ^ PERM_MENU;	/* default command */
      utmp_mode(menu->umode);
      cc = 0;		/* dust.080214: 指定游標位置 */
    }
    if (mode & MENU_DRAW)
    {
      if (mode & MENU_FILM)
      {
	clear();
	note_setrand();
	movie();
	cx = -1;
      }

      vs_head(menu->desc, NULL);

      mode = 0;
      do
      {
	move(MENU_XPOS + mode, 0);
	clrtoeol();
	if (mode <= max)
	{
	  mptr = table[mode];
	  str = mptr->desc;
	  move(MENU_XPOS + mode, MENU_YPOS);
	  prints("  \033[1;30m[\033[36m%c\033[30m]\033[m%s ", *str++, str + 1);
	}
      } while (++mode <= mmx);

      mmx = max;
      mode = 0;
    }

    switch (cmd)
    {

    case KEY_DOWN:
      konami_code(40);  /* floatJ.090607: konami_code 下 */

      cc = (cc == max) ? 0 : cc + 1;
      break;

    case KEY_UP:
      konami_code(38);  /* floatJ.090607: konami_code 上 */

      cc = (cc == 0) ? max : cc - 1;
      break;

    /* floatJ: 按鈕切換動態看板 */
    case '>':
    case '.':  
      note_direction(1);
      movie();
      break;
    case '<':
    case ',':
      note_direction(-1);
      movie();
      break;

    case Ctrl('X'):	/* dust.100313: 切換選單狀態列模式 */
      if(!cuser.userlevel)
	continue;

      if(cuser.ufo & UFO_MFMCOMER)
	mfr_mode_def[0] = 'E';
      else if(cuser.ufo & UFO_MFMGCTD)
	mfr_mode_def[0] = 'A';
      else if(cuser.ufo & UFO_MFMUCTD)
	mfr_mode_def[0] = 'U';
      else
	mfr_mode_def[0] = 'D';

      switch(pans(7, 18, "請選擇狀態列模式", mfr_mode))
      {
      case 'd':
	cuser.ufo &= ~UFO_MFMCOMER;
	cuser.ufo &= ~UFO_MFMGCTD;
	cuser.ufo &= ~UFO_MFMUCTD;
	status_foot();
	break;
      case 'e':
	cuser.ufo |= UFO_MFMCOMER;
	cuser.ufo &= ~UFO_MFMGCTD;
	cuser.ufo &= ~UFO_MFMUCTD;
	status_foot();
	break;
      case 'a':
	cuser.ufo &= ~UFO_MFMCOMER;
	cuser.ufo |= UFO_MFMGCTD;
	cuser.ufo &= ~UFO_MFMUCTD;
	status_foot();
	break;
      case 'u':
	cuser.ufo &= ~UFO_MFMCOMER;
	cuser.ufo &= ~UFO_MFMGCTD;
	cuser.ufo |= UFO_MFMUCTD;
	status_foot();
      }
      break;

    case Ctrl('A'):	/* itoc.020829: 預設選項第一個 */
    case KEY_HOME:
      cc = 0;
      break;

    case KEY_END:
      cc = max;
      break;

    case KEY_PGUP:
      cc = (cc == 0) ? max : 0;
      break;

    case KEY_PGDN:
      cc = (cc == max) ? 0 : max;
      break;

    case '\n':
    case KEY_RIGHT:
      konami_code(39);  /* floatJ.090607: konami_code 右 */
      mptr = table[cc];
      cmd = mptr->umode;
#if 1
     /* Thor.990212: dynamic load , with negative umode */
      if (cmd < 0)
      {
	void *p = DL_get(mptr->func);
	if (!p)
	{
#  if 1
	  p = dlerror();
	  if(p)
	    vmsg((char *)p);
#  endif
	  break;
	}
	mptr->func = p;
	cmd = -cmd;
	mptr->umode = cmd;
      }
#endif
      utmp_mode(cmd);

      if (cmd <= M_XMENU)	/* 子目錄的 mode 要 <= M_XMENU */
      {
	extern time_t sudo_uptime;

	menu->level = PERM_MENU + mptr->desc[0];
	menu = (MENU *) mptr->func;

	mode = MENU_LOAD | MENU_DRAW;
	/* for Admin Pseudo Sudo */
	if(bbsmode == M_AMENU && sudo_uptime > 0 && time(NULL) - sudo_uptime <= 300)
	  mode |= MENU_FILM;

	depth++;
	continue;
      }

      {
	int (*func) ();

	func = mptr->func;
	mode = (*func) ();
      }

      utmp_mode(menu->umode);

      if (mode == XEASY)
      {
	outf(feeter);
	mode = 0;
      }
      else
      {
	mode = MENU_DRAW | MENU_FILM;
      }

      cmd = mptr->desc[0];
      continue;

#ifdef EVERY_Z
    case Ctrl('Z'):
      every_Z(0);
      goto every_key;

    case Ctrl('U'):
      every_U(0);
      goto every_key;
#endif

    /* itoc.010911: Select everywhere，不再限制是在 M_0MENU */
    case 's':
    case Ctrl('S'):
      utmp_mode(M_BOARD);
      Select();
      goto every_key;

#ifdef MY_FAVORITE
    /* itoc.010911: Favorite everywhere，不再限制是在 M_0MENU */
    case 'f':
    case Ctrl('F'):
      if (cuser.userlevel)	/* itoc.010407: 要檢查權限 */
      {
	utmp_mode(M_MF);
	MyFavorite();
      }
      goto every_key;
#endif

    /* itoc.020301: Read currboard in M_0MENU */
    case 'r':
      if (bbsmode == M_0MENU)
      {
	if (currbno >= 0)
	{
	  utmp_mode(M_BOARD);
	  XoPost(currbno);
	  xover(XZ_POST);
#ifndef ENHANCED_VISIT
	  time(&brd_visit[currbno]);
#endif
	}
	goto every_key;
      }
      goto default_key;	/* 若不在 M_0MENU 中按 r 的話，要視為一般按鍵 */

every_key:	/* 特殊鍵處理結束 */
      utmp_mode(menu->umode);
      mode = MENU_DRAW | MENU_FILM;
      cmd = table[cc]->desc[0];
      continue;

    case KEY_LEFT:
      konami_code(37);  /* floatJ.090607: konami_code 左 */
    case 'e':
      if (depth > 0)
      {
	menu->level = PERM_MENU + table[cc]->desc[0];
	menu = (MENU *) menu->func;
	mode = MENU_LOAD | MENU_DRAW;
	if(depth == 1)	/* dust.091009回到根目錄時重新顯示動態看板 */
	  mode |= MENU_FILM;
        depth--;
	continue;
      }
      cmd = 'G';

default_key:
    default:

      if (cmd >= 'a' && cmd <= 'z')
	cmd ^= 0x20;			/* 變大寫 */

      cc = 0;
      for (;;)
      {
	if (table[cc]->desc[0] == cmd)
	  break;
	if (++cc > max)
	{
	  cc = cx;
	  goto menu_key;
	}
      }
    }

    if (cc != cx)	/* 若游標移動位置 */
    {
#ifdef CURSOR_BAR
      if (cx >= 0)
      {
	move(MENU_XPOS + cx, MENU_YPOS);
	if (cx <= max)
	{
	  mptr = table[cx];
	  str = mptr->desc;
	  /* floatJ.080510: 試試看改變一下外觀 :p */
	  prints("  \033[1;30m[\033[36m%c\033[30m]\033[m%s ", *str, str + 1);
	}
	else
	{
	  outs("  ");
	}
      }
      move(MENU_XPOS + cc, MENU_YPOS);
      mptr = table[cc];
      str = mptr->desc;
      prints(COLOR8 "> [%c]%s \033[m", *str, str + 1);
      cx = cc;
#else		/* 沒有 CURSOR_BAR */
      if (cx >= 0)
      {
	move(MENU_XPOS + cx, MENU_YPOS);
	outc(' ');
      }
      move(MENU_XPOS + cc, MENU_YPOS);
      outc('>');
      cx = cc;
#endif
    }
    else		/* 若游標的位置沒有變 */
    {
#ifdef CURSOR_BAR
      move(MENU_XPOS + cc, MENU_YPOS);
      mptr = table[cc];
      str = mptr->desc;
      prints(COLOR8 "> [%c]%s \033[m", *str, str + 1);
#else
      move(MENU_XPOS + cc, MENU_YPOS + 1);
#endif
    }

menu_key:
    cmd = vkey();
  }
}

