/*-------------------------------------------------------*/
/* bbsd.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* author : opus.bbs@bbs.cs.nthu.edu.tw		 	 */
/* target : BBS daemon/main/login/top-menu routines 	 */
/* create : 95/03/29				 	 */
/* update : 10/10/04				 	 */
/*-------------------------------------------------------*/


#define	_MAIN_C_


#include "bbs.h"
#include "dns.h"

#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/telnet.h>
#include <sys/resource.h>
#include <pthread.h>


#define	QLEN		3
#define	PID_FILE	"run/bbs.pid"
#define	LOG_FILE	"run/bbs.log"
#undef	SERVER_USAGE


static int myports[MAX_BBSDPORT] = BBSD_PORT;

static pid_t currpid;

static pthread_mutex_t rslv_mutex = PTHREAD_MUTEX_INITIALIZER;

extern BCACHE *bshm;
extern UCACHE *ushm;

/* static int mport; */ /* Thor.990325: 不需要了:P */
static u_long tn_addr;

/* Davy.171001: Connection sockaddr infos (used by proxy mode) */
struct sockaddr_in connection_sin;

#ifdef CHAT_SECURE
char passbuf[PSWDLEN + 5];
#endif


#ifdef MODE_STAT
extern UMODELOG modelog;
extern time_t mode_lastchange;
#endif



/* ----------------------------------------------------- */
/* 離開 BBS 程式					 */
/* ----------------------------------------------------- */


char*
SysopLoginID_query(id)	/* 記錄或查詢sysop的登入者 */
  char *id;		/* NULL:回傳ID , 非NULL:設定ID */
{
  static char loginID[IDLEN + 1];
  if(id)
    strcpy(loginID, id);
  return loginID;
}


void
circlog(mode, msg)         /* circ god 行為紀錄 */
  char *mode, *msg;
{
  char buf[512];

  /* floatJ.091109: circlog也會在sysop登入修改召喚名單時被呼叫，所以也要紀錄以sysop登入的真正ID */
  if(HAS_PERM(PERM_SpSYSOP))
    sprintf(buf, "%s %-12s:%s %s\n", Now(), SysopLoginID_query(NULL), mode, msg);
  else
    sprintf(buf, "%s %-12s:%s %s\n", Now(), cuser.userid, mode, msg);

  f_cat(FN_RUN_CIRCGOD, buf);
}


void
slog(mode, msg)		/* sysop 行為紀錄 */
  char *mode, *msg;
{
  char buf[512];

  sprintf(buf, "%s %-12s:%s %s\n", Now(), SysopLoginID_query(NULL), mode, msg);
  f_cat(FN_RUN_SYSOP, buf);
}


void
alog(mode, msg)		/* Admin 行為記錄 */
  char *mode, *msg;
{
  char buf[512];

  if(HAS_PERM(PERM_SpSYSOP))
    slog(mode, msg);
  else
  {
    sprintf(buf, "%s %s %-13s%s\n", Now(), mode, cuser.userid, msg);
    f_cat(FN_RUN_ADMIN, buf);
  }
}


void
blog(mode, msg)		/* BBS 一般記錄 */
  char *mode, *msg;
{
  char buf[512];

  sprintf(buf, "%s %s %-13s%s\n", Now(), mode, cuser.userid, msg);
  f_cat(FN_RUN_USIES, buf);
}



#ifdef MODE_STAT
void
log_modes()
{
  time(&modelog.logtime);
  rec_add(FN_RUN_MODE_CUR, &modelog, sizeof(UMODELOG));
}
#endif


void
u_exit(mode)
  char *mode;
{
  int fd, diff;
  char fpath[80];
  ACCT tuser;

  mantime_add(currbno, -1);	/* 退出最後看的那個板 */

  utmp_free(cutmp);		/* 釋放 UTMP shm */

  diff = (time(&cuser.lastlogin) - ap_start) / 60;
  sprintf(fpath, "Stay: %d (%d)", diff, currpid);
  blog(mode, fpath);

  if (cuser.userlevel)
  {
    ve_backup();		/* 編輯器自動備份 */
    rhs_save();			/* 儲存閱讀記錄檔 */
    pmore_saveBP();
  }

#ifndef LOG_BMW	/* 離站刪除水球 */
  usr_fpath(fpath, cuser.userid, fn_amw);
  unlink(fpath);
  usr_fpath(fpath, cuser.userid, fn_bmw);
  unlink(fpath);
#endif

#ifdef MODE_STAT
  log_modes();
#endif


  /* 寫回 .ACCT */

  if (!HAS_STATUS(STATUS_DATALOCK))	/* itoc.010811: 沒有被站長鎖定，才可以回存 .ACCT */
  {
    usr_fpath(fpath, cuser.userid, fn_acct);
    fd = open(fpath, O_RDWR);
    if (fd >= 0)
    {  
      if (read(fd, &tuser, sizeof(ACCT)) == sizeof(ACCT))
      {
	if (diff >= 1)
	{
	  cuser.numlogins++;	/* Thor.980727.註解: 在站上未超過一分鐘不予計算次數 */
	  addmoney(diff/4);	/* dust.091216: 上站時間 四分鐘加一元 */
	}

	if (HAS_STATUS(STATUS_COINLOCK))	/* itoc.010831: 若是 multi-login 的第二隻以後，不儲存錢幣 */
	{
	  cuser.money = tuser.money;
	  cuser.gold = tuser.gold;
	}

	/* itoc.010811.註解: 如果使用者在線上沒有認證的話，
	  那麼 cuser 及 tuser 的 userlevel/tvalid 是同步的；
	  但若使用者在線上回認證信/填認證碼/被站長審核註冊單..等認證通過的話，
	  那麼 tuser 的 userlevel/tvalid 才是比較新的 */
	cuser.userlevel = tuser.userlevel;
	cuser.tvalid = tuser.tvalid;

	lseek(fd, (off_t) 0, SEEK_SET);
	write(fd, &cuser, sizeof(ACCT));
      }
      close(fd);
    }
  }
}


void
abort_bbs()
{
  if (bbstate)
    u_exit("AXXED");
  exit(0);
}


static void
login_abort(msg)
  char *msg;
{
  outs(msg);
  refresh();
  exit(0);
}


/* Thor.980903: lkchu patch: 不使用上站申請帳號時, 則下列 function均不用 */

#ifdef LOGINASNEW

/* ----------------------------------------------------- */
/* 檢查 user 註冊情況					 */
/* ----------------------------------------------------- */


static int
belong(flist, key)
  char *flist;
  char *key;
{
  int fd, rc;

  rc = 0;
  if ((fd = open(flist, O_RDONLY)) >= 0)
  {
    mgets(-1);

    while (flist = mgets(fd))
    {
      str_lower(flist, flist);
      if (str_str(key, flist))
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
is_badid(userid)
  char *userid;
{
  int ch;
  char *str;

  if (strlen(userid) < 2)
    return 1;

  if (!is_alpha(*userid))
    return 1;

  if (!str_cmp(userid, STR_NEW))
    return 1;

  str = userid;
  while (ch = *(++str))
  {
    if (!is_alnum(ch))
      return 1;
  }
  return (belong(FN_ETC_BADID, userid));
}


static int
uniq_userno(fd)		/* dust.121014 remake: 不再重複使用userno */
  int fd;
{
  struct stat st;
  int applied_user;
  SCHEMA sp;

  if(fstat(fd, &st) < 0)
    login_abort("fstat fail");

  applied_user = st.st_size / sizeof(SCHEMA);

  if(applied_user >= MAXAPPLY)
    return 0;
  else if(applied_user > 1)
  {
    if(lseek(fd, -sizeof(SCHEMA), SEEK_END) < 0)
      login_abort("lseek fail");

    if(read(fd, &sp, sizeof(SCHEMA)) != sizeof(SCHEMA))
      login_abort("read fail");

    /* .USR的最後一筆一定是當前發出去的UNO中最大的，無論ID欄位有沒有東西 */
    return sp.userno + 1;
  }
  else
    return 1;	/* .USR是空的，從 1 開始 */
}


static void
acct_apply()
{
  SCHEMA slot;
  char buf[80];
  char *userid = cuser.userid;
  int try = 0, fd;
  char *gender_menu[] = {"0中性", "1男性", "2女性", NULL};
  char *commit_menu[] = {"y是，送交！", "n否，我要修改", "q取消註冊", NULL};
  char year[8] = "", month[4] = "", day[4] = "", gender_def[4];

  memset(&cuser, 0, sizeof(ACCT));


  vs_bar(BBSNAME "新帳號註冊系統");

  move(11, 0);
  outs("ID是每個使用者獨一無二的代號，作為身分識別用。\n");
  outs("長度為 \033[1;36m2~12\033[m 個字元，第一個字需為\033[1;32m英文字母\033[m，其他則可以是\033[1;33m英文字母\033[m或\033[1;33m數字\033[m。\n");
  outs("大小寫會被視為相同，例如有了NPC就不能註冊Npc，在登入輸入帳號時也可不必管大小寫\n");
  outs("但在顯示您的ID時，大小寫的差別仍然會出現，並不會被統一轉成大寫或小寫。\n");

  for(;;)
  {
    if(!vget(16, 0, "請輸入您想用的ID：", userid, IDLEN + 1, DOECHO))
      login_abort("\n再見 ...");

    if(is_badid(userid))
    {
      vmsg("無法接受這個代號，請使用英文字母，而且不能含有空格");
    }
    else
    {
      usr_fpath(buf, userid, NULL);
      if(dashd(buf))
	vmsg("此代號已經有人使用");
      else
	break;
    }

    if(++try >= 10)
      login_abort("\n您嘗試錯誤太多次，請下次再來吧");
  }

  move(2, 0);
  prints("ID：%s", userid);

  clrregion(11, 16);


  move(13, 0);
  outs("密碼長度至少為4個字元。不限制組成的字。\n");
  outs("\033[1;31m為了安全性考量，建議不要與ID相似，並且是不容易被猜中的密碼(例如英文與數字混合)\033[m\n");

  for(;;)
  {
    char check[PSWDLEN + 1];

    vget(16, 0, "請輸入密碼：", buf, PSWDLEN + 1, NOECHO);
    if(strlen(buf) < 4)
    {
      vmsg("密碼長度至少要4個字元，請重新輸入");
      continue;
    }

    /* dust.101231: 重複檢查時改成若不輸入則直接重新決定密碼，方便想修改的人 */
    if(!vget(17, 0, "請再次輸入以確認密碼無誤：", check, PSWDLEN + 1, NOECHO))
    {
      clrregion(17, 17);
      continue;
    }

    if(!strcmp(buf, check))
      break;

    vmsg("密碼輸入錯誤，請重新輸入密碼");
    clrregion(17, 17);
  }

  str_ncpy(cuser.passwd, genpasswd(buf), sizeof(cuser.passwd));

  move(3, 0);
  outs("密碼：");
  for(try = strlen(buf); try > 0; try--)
    outc('*');

  clrregion(11, 17);


  for(;;)
  {
    move(b_lines, 0);
    outs("小提示：您可以按\033[1;33m Ctrl+X \033[m開啟/關閉伺服器端的中文字偵測功\能。");


    move(14, 0);
    outs("長度至少要1個字元，將作為您在站上活動時的別名。");

    while(1)
    {
      if(vget(16, 0, "暱稱：", cuser.username, UNLEN + 1, GCARRY))
	break;
    }

    move(4, 0);
    prints("暱稱：%s", cuser.username);

    clrregion(14, 16);


    move(14, 0);
    outs("\033[1;31m請注意：\033[m除了填註冊單時可以修改以及請站務修改，其他時候並沒有辦法修改姓名資料。");

    do
    {
      vget(16, 0, "真實姓名：", cuser.realname, RNLEN + 1, GCARRY);
    } while(strlen(cuser.realname) < 2);

    move(5, 0);
    prints("真實姓名：%s\n", cuser.realname);

    clrtobot();	/* dust.110102: 把底端的小提示清掉 */


    move(14, 0);
    outs("請選擇您的性別。");

    sprintf(gender_def, "%c%c", cuser.sex + '0', cuser.sex + '0');
    cuser.sex = krans(16, gender_def, gender_menu, "") - '0';
    move(6, 0);
    prints("性別：%2.2s", "？男女" + cuser.sex * 2);

    clrregion(14, 16);


    move(13, 0);
    outs("在您生日當天上站時，系統會顯示祝賀生日的圖。\n");
    outs("使用者名單中也會以\033[1;35m特殊顏色\033[m顯示您是當日的壽星。\n");

    while(1)
    {
      if(vget(16, 0, "西元年：", year, 5, GCARRY))
	cuser.year = atoi(year);

      if(cuser.year >= 1)
	break;
      year[0] = '\0';
    }

    while(1)
    {
      if(vget(16, 22, "月份：", month, 3, GCARRY))
	cuser.month = atoi(month);

      if(cuser.month >= 1 && cuser.month <= 12)
	break;
      month[0] = '\0';
    }

    while(1)
    {
      if(vget(16, 38, "日：", day, 3, GCARRY))
	cuser.day = atoi(day);

      if(cuser.day >= 1 && cuser.day <= 31)
	break;
      day[0] = '\0';
    }

    move(7, 0);
    prints("生日：%d/%02d/%02d", cuser.year, cuser.month, cuser.day);

    clrregion(13, 16);


    move(13, 0);
    outs("請填入您的電子郵件信箱地址。在轉寄文章到站外、備份資料時會以這個地址為預設。\n");
    outs("若不填入任何東西會以 BBS E-mail Address 作為預設值。\n");

    if(!vget(16, 0, "E-mail：", cuser.email, sizeof(cuser.email), GCARRY))
      sprintf(cuser.email, "%s.bbs@%s", cuser.userid, str_host);

    move(8, 0);
    prints("E-mail：%s", cuser.email);

    clrregion(13, 16);


    switch(krans(b_lines, "nn", commit_menu, "以上資料是否正確？ "))
    {
    case 'n':
      sprintf(year, "%d", cuser.year);
      sprintf(month, "%d", cuser.month);
      sprintf(day, "%d", cuser.day);

      move(4, 0);
      clrtobot();
      continue;

    case 'q':
      login_abort("\n再見 ...\n");
    }

    break;
  }

  cuser.userlevel = PERM_DEFAULT;
  cuser.ufo = UFO_DEFAULT_NEW;
  cuser.sig_avai = 6;			/* dust.100330: 可用簽名檔數量預設為6 */
  cuser.numlogins = 1;
  cuser.tvalid = ap_start;		/* itoc.030724: 拿上站時間當第一次認證碼的 seed */

  /* Ragnarok.050528: 可能二人同時申請同一個 ID，在此必須再檢查一次 */
  usr_fpath(buf, userid, NULL);
  if(dashd(buf))
  {
    vmsg("此代號剛被註冊走，請重新申請");
    login_abort("");
  }

  /* dispatch unique userno */

  cuser.firstlogin = cuser.lastlogin = cuser.tcheck = ap_start;
  memcpy(slot.userid, userid, IDLEN);

  fd = open(FN_SCHEMA, O_RDWR | O_CREAT, 0600);
  if(fd < 0)
    login_abort("SCHEMA FAIL");

  f_exlock(fd);

  slot.userno = cuser.userno = try = uniq_userno(fd);
  if(try > 0)
    write(fd, &slot, sizeof(slot));

  f_unlock(fd);

  close(fd);

  if(try <= 0)
  {
    vmsg("您申請時註冊人數已達上限");
    login_abort("");
  }

  /* create directory */

  mkdir(buf, 0700);
  strcat(buf, "/@");
  mkdir(buf, 0700);
  usr_fpath(buf, userid, "gem");	/* itoc.010727: 個人精華區 */
  mak_links(buf);
#ifdef MY_FAVORITE
  usr_fpath(buf, userid, "MF");
  mkdir(buf, 0700);
#endif

  usr_fpath(buf, userid, fn_acct);
  fd = open(buf, O_WRONLY | O_CREAT, 0600);
  write(fd, &cuser, sizeof(ACCT));
  close(fd);

  sprintf(buf, "%d", try);	/* 記錄新帳號的UNO */
  blog("APPLY", buf);
}

#endif /* LOGINASNEW */


/* ----------------------------------------------------- */
/* bad login						 */
/* ----------------------------------------------------- */


#define	FN_BADLOGIN	"logins.bad"


static void
logattempt(type, content)
  int type;			/* '-' login failure   ' ' success */
  char *content;
{
  char buf[128], fpath[64];

  sprintf(buf, "%s %c %s\n", Btime(&ap_start), type, content);
    
  usr_fpath(fpath, cuser.userid, FN_LOG);
  f_cat(fpath, buf);

  if (type != ' ')
  {
    usr_fpath(fpath, cuser.userid, FN_BADLOGIN);
    pthread_mutex_lock(&rslv_mutex);
    sprintf(buf, "[%s] %s\n", Btime(&ap_start), fromhost);
    pthread_mutex_unlock(&rslv_mutex);
    f_cat(fpath, buf);
  }
}


/* ----------------------------------------------------- */
/* 登錄 BBS 程式					 */
/* ----------------------------------------------------- */


extern void talk_rqst();
extern void bmw_rqst();


#ifdef HAVE_WHERE
int	/* 1:在list中  0:不在list中 */
belong_list(filelist, key, dst, szdst)
  char *filelist, *key, *dst;
  int szdst;
{
  FILE *fp;
  char buf[80], *str;
  int rc = 0;

  if(!(fp = fopen(filelist, "r")))
    return 0;

  while(fgets(buf, sizeof(buf), fp))
  {
    if(buf[0] == '#')
      continue;

    strtok(buf, " \t\n");
    if(strstr(key, buf))
    {
      if(str = strtok(NULL, "\n"))
      {
	while(*str && isspace(*str))
	  str++;

	str_ncpy(dst, str, szdst);
	rc = 1;
	break;
      }
    }
  }

  fclose(fp);
  return rc;
}


static void
find_fromwhere(from, szfrom)
  char *from;
  int szfrom;
{
  /* dust.101005: 執行這個函數時外頭已經有Mutex了，不需要再鎖 */
  char name[48];
  uschar *addr = (uschar*) &tn_addr;

  /* 先比對 id mapping */
  str_lower(name, cuser.userid);
  if (!belong_list(FN_ETC_IDHOME, name, from, szfrom))
  {
    /* 再比對 FQDN */
    str_lower(name, fromhost);		/* itoc.011011: 大小寫均可，etc/fqdn 裡面都要寫小寫 */
    if (!belong_list(FN_ETC_FQDN, name, from, szfrom))
    {
      /* 再比對 ip */
      sprintf(name, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
      if (!belong_list(FN_ETC_HOST, name, from, szfrom))
	if (cuser.ufo2 & UFO2_HIDEMYIP)
	  str_ncpy(from, "極地冰屋", szfrom); /* davy.110928: 如果有設置隱藏IP，就改成"極地冰屋" */
	else
	  str_ncpy(from, fromhost, szfrom);	/*  如果都沒找到對應故鄉，就是用 fromhost */
    }
  }
}

#endif


static void
utmp_setup(mode)
  int mode;
{
  UTMP utmp;
  uschar *addr;
  
  memset(&utmp, 0, sizeof(utmp));
  memset(utmp.mslot, 0xFF, sizeof(utmp.mslot));

  utmp.pid = currpid;
  utmp.userno = cuser.userno;
  utmp.mode = bbsmode = mode;
  /* utmp.in_addr = tn_addr; */ /* itoc.010112: 改變umtp.in_addr以使ulist_cmp_host正常 */
  addr = (uschar *) &tn_addr;
  utmp.in_addr = (addr[0] << 24) + (addr[1] << 16) + (addr[2] << 8) + addr[3];
  utmp.userlevel = cuser.userlevel;	/* itoc.010309: 把 userlevel 也放入 cache */
  utmp.ufo = cuser.ufo;
  utmp.status = 0;
  utmp.talker = -1;
  
  strcpy(utmp.userid, cuser.userid);
  utmp.label_attr = cuser.label_attr;
  strcpy(utmp.label, cuser.label);

  pthread_mutex_lock(&rslv_mutex);

  str_ncpy(utmp.fromhost, fromhost, sizeof(utmp.fromhost));
  if (utmp.ufo & UFO_CLOAK) /* davy.110928: 隱身不更新出現時間 */
    utmp.login_time = cuser.lastlogin;
  else
    time(&utmp.login_time);
#ifdef DETAIL_IDLETIME
  utmp.idle_time = ap_start;
#endif

#ifdef GUEST_NICK
  if (!cuser.userlevel)		/* guest */
  {
    char *nick[GUEST_NICK_LIST_MAX] = GUEST_NICK_LIST;
		/* dust.101005: 改用rand()取亂數 */
    sprintf(cuser.username, "冰原上的%s", nick[rand() % GUEST_NICK_LIST_MAX]);
  }
#endif	/* GUEST_NICK */

  strcpy(utmp.username, cuser.username);
  
#ifdef HAVE_WHERE

#  ifdef GUEST_WHERE
  if (!cuser.userlevel)		/* guest */
  {
    char *from[GUEST_FROM_LIST_MAX] = GUEST_FROM_LIST;
		/* dust.101005: 改用rand()取亂數 */
    strcpy(utmp.from, from[rand() % GUEST_FROM_LIST_MAX]);
  }
  else
#  endif	/* GUEST_WHERE */
    find_fromwhere(utmp.from, sizeof(utmp.from));

#else
  str_ncpy(utmp.from, fromhost, sizeof(utmp.from));
#endif	/* HAVE_WHERE */

  /* Thor: 告訴User已經滿了放不下... */
  if (!utmp_new(&utmp))
    login_abort("\n您剛剛選的位子已經被人捷足先登了，請下次再來吧");

  pthread_mutex_unlock(&rslv_mutex);

  /* itoc.001223: utmp_new 完再 pal_cache，如果 login_abort 就不做了 */
  pal_cache();
}


/* ----------------------------------------------------- */
/* user login						 */
/* ----------------------------------------------------- */


static int		/* 回傳 multi */
login_user(content, luser)
  char *content;
  ACCT *luser;		/* 給sysop登入用 */
{
  int attempts;		/* 嘗試幾次錯誤 */
  int multi;
  char fpath[64], uid[IDLEN + 1];
#ifndef CHAT_SECURE
  char passbuf[PSWDLEN + 1];
#endif

  move(b_lines, 0);

#if 1
  /* floatJ: 改用 FILM_LOGINMSG (可改變的歡迎訊息) */
  film_out(FILM_LOGINMSG, b_lines);
#else
  outs(" \033[1;33m※\033[m 請以\ \033[1;36m" STR_GUEST "\033[m 參觀，或以 \033[1;35m" STR_NEW "\033[m 註冊。");
#endif

  attempts = 0;
  multi = 0;
  for (;;)
  {
    if (++attempts > LOGINATTEMPTS)
    {
      film_out(FILM_TRYOUT, 0);
      login_abort("\n再見 ...");
    }

    /* floatJ: 因為把Login那行改成GCARRY, 所以要先初始化 uid ，否則一開始會是亂碼 */
    uid[0]='\0';

/* floatJ: 跟 ofo 一樣改流程，會用到 Passwd 這個 goto 標籤  */
Passwd:

    /* floatJ: 小心那個偽半形X ... 直接從pietty複製或編輯會出錯，建議從記事本複製 */
    vget(b_lines -2, 0, "    Login ×  ", uid, IDLEN + 1, GCARRY);

    if (!str_cmp(uid, STR_NEW))
    {
#ifdef LOGINASNEW
      acct_apply(); /* Thor.980917.註解: cuser setup ok */
      logattempt(' ', content);
      break;
#else
      outs("\n本系統目前暫停線上註冊, 請用 " STR_GUEST " 進入");
      continue;
#endif
    }
    else if (!*uid)
    {
      attempts--; /* 若沒輸入 ID，那麼 continue，並且不算在tryout中 */
    }
    else if (str_cmp(uid, STR_GUEST))	/* 一般使用者 */
    {
      usr_fpath(fpath, uid, fn_acct);

      if (!vget(b_lines - 2, 0, " Password ×  "  , passbuf, PSWDLEN +  5, NOECHO))
      {
        *uid = 0;
	attempts--;
	continue;	/* 不打密碼則取消登入,且不算入tryout */
      }

      /* itoc.040110: 在輸入完 ID 及密碼，才載入 .ACCT */
      if (acct_load(&cuser, uid) < 0)
      {
	outf(ERR_UID_ANSI);
	*uid = 0;
	continue;
      }

      if (chkpasswd(cuser.passwd, passbuf))
      {
	logattempt('-', content);
	*passbuf = 0;
	outf(ERR_PASSWD_ANSI);
        if (++attempts > LOGINATTEMPTS)	/* 補測attempts */
        {
	  film_out(FILM_TRYOUT, 0);
	  login_abort("\n再見 ...");
        }
	goto Passwd;
      }
      else
      {
	if (!str_cmp(cuser.userid, str_sysop))
	{
	  move(b_lines -2 , 0);
	  clrtoeol();

	  for(;;)
	  {
	    if(!vget(b_lines - 2, 0, "請輸入站務帳號：", uid, IDLEN+1, DOECHO) && !strcmp(uid, str_sysop))
	    {
	      *uid = '\0';
	      goto Passwd;
	    }

	    if(acct_load(luser, uid)<0)	/* ID不存在的狀況 */
	    {
	      outf(ERR_UID_ANSI);
	      continue;
	    }
	    if(!(luser->userlevel & PERM_SYSOP))
	    {
	      outf("這不是擁有站務權限的帳號。");
	      continue;
	    }
	    else if(!strcmp(luser->userid, str_sysop))  /* 不能用sysop登記 */
	      continue;

	    if(!vget(b_lines - 2, 40, "密碼：", passbuf, PSWDLEN+1, NOECHO))
	      continue;

	    if (chkpasswd(luser->passwd, passbuf))
	    {
	      outf(ERR_PASSWD_ANSI);
	      *passbuf = '\0';
	      continue;
	    }

	    break;
	  }


#ifdef SYSOP_SU
	  /* 簡單的 SU 功能 */
	  if (vans("變更使用者身分(Y/N)？[N] ") == 'y')
	  {
	    for (;;)
	    {
	      if (vget(b_lines - 2, 0, "   [變更帳號] ", uid, IDLEN + 1, DOECHO) && acct_load(&cuser, uid) >= 0)
		break;
	      outf(err_uid);
	      *uid = 0;
	    }
	  }
	  else
#endif
	  {
	    /* SYSOP gets certain permission bits */
	    cuser.userlevel = (PERM_BASIC | PERM_CHAT | PERM_PAGE | PERM_POST | PERM_VALID | PERM_MBOX | PERM_XEMPT | PERM_BM | PERM_SEECLOAK | PERM_SpSYSOP | PERM_SYSOP);
	  }
	}
	else
	  /* 非sysop的ID強制拿掉 PERM_SpSYSOP 和 PERM_SEECLOAK */
	  cuser.userlevel &= ~(PERM_SpSYSOP | PERM_SEECLOAK);

	if (cuser.ufo & UFO_ACL)
	{
	  uschar *addr = (uschar*) &tn_addr;
	  char ipaddr[16];
	  sprintf(ipaddr, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);

	  usr_fpath(fpath, cuser.userid, FN_ACL);
	  /* dust.101005: ACL這種東西還是填IP會比較好吧? */
	  if (!acl_has(fpath, "", ipaddr))
	  {	/* Thor.980728: 注意 acl 檔中要全部小寫 */
	    logattempt('-', content);
	    vmsg("您的上站地點不太對勁，請重新核對acl。");
	    login_abort("\n您的上站地點不太對勁，請核對 [上站地點設定檔]");
	  }
	}

	logattempt(' ', content);

	/* check for multi-session */

	if (!HAS_PERM(PERM_ALLADMIN))
	{
	  UTMP *ui;
	  pid_t pid;

	  if (HAS_PERM(PERM_DENYLOGIN | PERM_PURGE))
	    login_abort("\n這個帳號暫停服務，詳情請向站務洽詢。");

	  if (!(ui = (UTMP *) utmp_find(cuser.userno)))
	    break;		/* user isn't logged in */

	  pid = ui->pid;
	  if (pid && vans("您想踢掉其他重複登入的帳號 (Y/N) 嗎？[Y] ") != 'n' && pid == ui->pid)
	  {
	    if ((kill(pid, SIGTERM) == -1) && (errno == ESRCH))
	      utmp_free(ui);
	    else
	      sleep(3);			/* 被踢的人這時候正在自我了斷 */
	    blog("MULTI", cuser.userid);
	  }

	  /* 線上已有 MULTI_MAX 隻自己，禁止登入 */
	  if ((multi = utmp_count(cuser.userno, 0)) >= MULTI_MAX || (!multi && acct_load(&cuser, uid) < 0))
	  /* yiting.050101: 若剛已踢掉所有 multi-login，那麼重新讀取以套用變更 */
	  {
	    vmsg("重複登入過多，無法登入");
	    login_abort("\n掰咪 ~☆");
	  }
	}
	else	/* dust: 站長重複登入時可以選擇踢掉 */
	{
	  UTMP *ui;
	  pid_t pid;

	  if (!(ui = (UTMP *) utmp_find(cuser.userno)))
	    break;		/* user isn't logged in */

	  if(!(cuser.ufo & UFO_ARLKNQ))
	  {
	    if (pid = ui->pid)
	    {
	      int c;

	      /* dust.100306: 站務habit 詢問是否踢除重複login */
	      sprintf(fpath, "要踢掉重複登入的帳號嗎？(Y/N) [%c] ", (cuser.ufo & UFO_KQDEFYES)? 'Y' : 'N');
	      c = vans(fpath);

	      if(c != 'y' && c != 'n')
		c = (cuser.ufo & UFO_KQDEFYES)? 'y' : 'n';

	      if (c == 'y' && pid == ui->pid)
	      {
		if ((kill(pid, SIGTERM) == -1) && (errno == ESRCH))
		  utmp_free(ui);
		else
		  sleep(3);		/* 被踢的人這時候正在自我了斷 */
		blog("MULTI", cuser.userid);
	      }
	    }
	  }

	}
    /* --------------------------- */
	break;
      }
    }
    else	/* 是使用guest登入 */
    {
      if (acct_load(&cuser, uid) < 0)
      {
	outf("錯誤：guest並不存在");
	continue;
      }

      if((multi = utmp_count(cuser.userno, 0)) >= GUEST_MULTI_MAX)
      {
        vmsg("guest登入人數過多，無法登入。");
        login_abort("\nguest登入人數過多，無法登入......");
      }

      logattempt(' ', content);
      cuser.userlevel = 0;	/* Thor.981207: 怕人亂玩, 強制寫回cuser.userlevel */
      cuser.ufo = UFO_DEFAULT_GUEST;
      break;	/* Thor.980917.註解: cuser setup ok */
    }
  }

  return multi;
}



static void
login_level()
{
  int fd;
  usint level;
  ACCT tuser;
  char fpath[64];

  /* itoc.010804.註解: 有 PERM_VALID 者自動發給 PERM_POST PERM_PAGE PERM_CHAT */
  level = cuser.userlevel | (PERM_ALLVALID ^ PERM_VALID);

  if (!(level & PERM_ALLADMIN))
  {
#ifdef JUSTIFY_PERIODICAL
    if ((level & PERM_VALID) && (cuser.tvalid + VALID_PERIOD < ap_start))
    {
      level ^= PERM_VALID;
      /* itoc.011116: 主動發信通知使用者，一直送信不知道會不會太耗空間 !? */
      mail_self(FN_ETC_REREG, str_sysop, "您的認證已經過期，請重新認證", 0);
    }
#endif

#ifdef NEWUSER_LIMIT
    /* 即使已經通過認證，還是要見習三天 */
    if (ap_start - cuser.firstlogin < 3 * 86400)
      level &= ~PERM_POST;
#endif

    /* itoc.000520: 未經身分認證, 禁止 post/chat/talk/write */
    if (!(level & PERM_VALID))
      level &= ~(PERM_POST | PERM_CHAT | PERM_PAGE);

    if (level & PERM_DENYPOST)
      level &= ~PERM_POST;

    if (level & PERM_DENYTALK)
      level &= ~PERM_PAGE;

    if (level & PERM_DENYCHAT)
      level &= ~PERM_CHAT;

    if ((cuser.numemails >> 4) > (cuser.numlogins + cuser.numposts))
      level |= PERM_DENYMAIL;
  }

  cuser.userlevel = level;

  usr_fpath(fpath, cuser.userid, fn_acct);
  if ((fd = open(fpath, O_RDWR)) >= 0)
  {
    if (read(fd, &tuser, sizeof(ACCT)) == sizeof(ACCT))
    {
      /* itoc.010805.註解: 這次的寫回 .ACCT 是為了讓別人 Query 線上使用者時
	 出現的上站時間/來源正確，以及回存正確的 userlvel */
      tuser.userlevel = level;
      if (cutmp->ufo & UFO_CLOAK) /* davy.110928: 隱身不更新出現時間 */
        tuser.lastlogin = cuser.lastlogin;
      else
        tuser.lastlogin = ap_start;
      strcpy(tuser.lasthost, cuser.lasthost);

      lseek(fd, (off_t) 0, SEEK_SET);
      write(fd, &tuser, sizeof(ACCT));
    }
    close(fd);
  }
}


static void
login_status(multi)
  int multi;
{
  usint status;
  char fpath[64];
  struct tm *ptime;

  status = 0;

  /* itoc.010831: multi-login 的第二隻加上不可變動錢幣的旗標 */
  if (multi)
    status |= STATUS_COINLOCK;

  /* itoc.011022: 加入生日旗標 */
  ptime = localtime(&ap_start);
  if (cuser.day == ptime->tm_mday && cuser.month == ptime->tm_mon + 1)
    status |= STATUS_BIRTHDAY;

  /* 朋友名單同步、清理過期信件 */
  if (ap_start > cuser.tcheck + CHECK_PERIOD)
  {
    outz(MSG_CHKDATA);
    refresh();

    cuser.tcheck = ap_start;
    usr_fpath(fpath, cuser.userid, fn_pal);
    pal_sync(fpath);
#ifdef HAVE_ALOHA
    usr_fpath(fpath, cuser.userid, FN_FRIENZ);
    frienz_sync(fpath);
#endif
#ifdef OVERDUE_MAILDEL
    status |= m_quota();		/* Thor.註解: 資料整理稽核有包含 BIFF check */
#endif
  }
#ifdef OVERDUE_MAILDEL
  else
#endif
    status |= m_query(cuser.userid);

  /* itoc.010924: 檢查個人精華區是否過多 */
#ifndef LINUX	/* 在 Linux 下這檢查怪怪的 */
  {
    struct stat st;
    usr_fpath(fpath, cuser.userid, "gem");
    if (!stat(fpath, &st) && (st.st_size >= 512 * 7))
      status |= STATUS_MGEMOVER;
  }
#endif

  cutmp->status |= status;
}


static void
login_other()
{
  usint status;
  char fpath[64];

  /* 刪除錯誤登入記錄 */
  usr_fpath(fpath, cuser.userid, FN_BADLOGIN);
  if (more(fpath, MFST_NONE) >= 0 && vans("以上為輸入密碼錯誤時的上站地點記錄，要刪除嗎(Y/N)？[Y] ") != 'n')
    unlink(fpath);

  if (!HAS_PERM(PERM_VALID))
    film_out(FILM_NOTIFY, -1);		/* 尚未認證通知 */
#ifdef JUSTIFY_PERIODICAL
  else if (!HAS_PERM(PERM_ALLADMIN) && (cuser.tvalid + VALID_PERIOD - INVALID_NOTICE_PERIOD < ap_start))
    film_out(FILM_REREG, -1);		/* 有效時間逾期 10 天前提出警告 */
#endif

#ifdef NEWUSER_LIMIT
  if (ap_start - cuser.firstlogin < 3 * 86400)
    film_out(FILM_NEWUSER, -1);		/* 即使已經通過認證，還是要見習三天 */
#endif

  status = cutmp->status;

#ifdef OVERDUE_MAILDEL
  if (status & STATUS_MQUOTA)
    film_out(FILM_MQUOTA, -1);		/* 過期信件即將清除警告 */
#endif

  if (status & STATUS_MAILOVER)
    film_out(FILM_MAILOVER, -1);	/* 信件過多或寄信過多 */

  if (status & STATUS_MGEMOVER)
    film_out(FILM_MGEMOVER, -1);	/* itoc.010924: 個人精華區過多警告 */

  if (status & STATUS_BIRTHDAY)
    film_out(FILM_BIRTHDAY, -1);	/* itoc.010415: 生日當天上站有 special 歡迎畫面 */

  ve_recover();				/* 上次斷線，編輯器回存 */
}


static void
tn_login()
{
  int multi;
  char buf[128];
  ACCT sysop_login;

  bbsmode = M_LOGIN;	/* itoc.020828: 以免過久未輸入時 igetch 會出現 movie */

  /* --------------------------------------------------- */
  /* 登錄系統						 */
  /* --------------------------------------------------- */

  /* Thor.990415: 記錄ip, 怕正查不到 */
  pthread_mutex_lock(&rslv_mutex);
  sprintf(buf, "%s ip:%08x (%d)", fromhost, tn_addr, currpid);
  pthread_mutex_unlock(&rslv_mutex);

  multi = login_user(buf, &sysop_login);

  blog("ENTER", buf);

  if(HAS_PERM(PERM_SpSYSOP))
  {
    SysopLoginID_query(sysop_login.userid);
    pthread_mutex_lock(&rslv_mutex);
    sprintf(buf, "%s IP:%08x", fromhost, tn_addr);
    pthread_mutex_unlock(&rslv_mutex);
    slog("登入", buf);
  }

  /* --------------------------------------------------- */
  /* 初始化 utmp、flag、mode、信箱			 */
  /* --------------------------------------------------- */

  bbstate = STAT_STARTED;	/* 進入系統以後才可以回水球 */
  utmp_setup(M_LOGIN);		/* Thor.980917.註解: cutmp, cutmp-> setup ok */
  total_user = ushm->count;	/* itoc.011027: 未進使用者名單前，啟始化 total_user */

  mbox_main();

  hm_init();	/* dust.100315: 載入個人倒數計時器 */

#ifdef MODE_STAT
  memset(&modelog, 0, sizeof(UMODELOG));
  mode_lastchange = ap_start;
#endif

  if (cuser.userlevel)		/* not guest */
  {
    /* ------------------------------------------------- */
    /* 核對 user level 並將 .ACCT 寫回			 */
    /* ------------------------------------------------- */

    /* itoc.030929: 在 .ACCT 寫回以前，不可以有任何 vmsg(NULL) 或 more(xxxx, NULL)
       等的東西，這樣如果 user 在 vmsg(NULL) 時回認證信，才不會被寫回的 cuser 蓋過 */
    if (!(cuser.ufo & UFO_CLOAK)) /* davy.110928: 隱身不更新出現時間 */
      cuser.lastlogin = ap_start;
    pthread_mutex_lock(&rslv_mutex);
    str_ncpy(cuser.lasthost, fromhost, sizeof(cuser.lasthost));
    pthread_mutex_unlock(&rslv_mutex);

    login_level();

    /* ------------------------------------------------- */
    /* 設定 status					 */
    /* ------------------------------------------------- */

    login_status(multi);

    /* ------------------------------------------------- */
    /* 秀些資訊						 */
    /* ------------------------------------------------- */

    login_other();
  }

  srand(ap_start * cuser.userno * currpid);
}


static void
tn_motd()
{
  usint ufo;

  ufo = cuser.ufo;

  if (!(ufo & UFO_MOTD))
  {
    more("gem/@/@-day", MFST_NULL);	/* 今日熱門話題 */
    pad_view();
  }

#ifdef HAVE_NOALOHA
  if (!(ufo & UFO_NOALOHA))
#endif
  {
#ifdef LOGIN_NOTIFY
    loginNotify();
#endif
#ifdef HAVE_ALOHA
    aloha();
#endif
  }

#ifdef HAVE_FORCE_BOARD
  brd_force();	/* itoc.000319: 強制閱讀公告板 */
#endif
}


/* ----------------------------------------------------- */
/* trap signals						 */
/* ----------------------------------------------------- */


static void
tn_signals()
{
  struct sigaction act;

  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  act.sa_handler = (void *) abort_bbs;
  sigaction(SIGBUS, &act, NULL);
  sigaction(SIGSEGV, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGXCPU, &act, NULL);
#ifdef SIGSYS
  /* Thor.981221: easy for porting */
  sigaction(SIGSYS, &act, NULL);/* bad argument to system call */
#endif

  act.sa_handler = (void *) talk_rqst;
  sigaction(SIGUSR1, &act, NULL);

  act.sa_handler = (void *) bmw_rqst;
  sigaction(SIGUSR2, &act, NULL);

  /* 在此借用 sigset_t act.sa_mask */
  sigaddset(&act.sa_mask, SIGPIPE);
  sigprocmask(SIG_BLOCK, &act.sa_mask, NULL);

}


static inline void
tn_main()
{
  int i;

#ifdef HAVE_LOGIN_DENIED
  char ipaddr[16];
  uschar *addr = (uschar*) &tn_addr;

  sprintf(ipaddr, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
  if (acl_has(BBS_ACLFILE, "", ipaddr))
    login_abort("\n貴機器於不被敝站接受");
#endif

  time(&ap_start);

  /* 顯示連線中訊息 */
  move(0, 0);
  outs("\033[1;30m tcirc.org:23 is now connecting......\033[m");
  refresh();
  /* 為避免太快看不到訊息，所以刻意給個延遲 */
  /* dust.101004: 亂數決定延遲時間, 0.6sec ~ 1.0sec */
  srand(ap_start);	/* 只好先set一次... */
  i = 600000 + rand() % 400001;
  usleep(i);

  clear();
  total_user = ushm->count;
  film_out(FILM_OPENING0, 0);
  
  currpid = getpid();

  tn_signals();	/* Thor.980806: 放於 tn_login前, 以便 call in不會被踢 */
  tn_login();

  board_main();
  gem_main();
#ifdef MY_FAVORITE
  mf_main();
#endif
  talk_main();

  tn_motd();

  if(cuser.userlevel)
    pmore_loadBP();	/* dust.091212: 載入pmore的設定 */

  menu();
  abort_bbs();	/* to make sure it will terminate */
}


/* ----------------------------------------------------- */
/* FSA (finite state automata) for telnet protocol	 */
/* ----------------------------------------------------- */


static void
telnet_init()
{
  static char svr[] = 
  {
    IAC, DO, TELOPT_TTYPE,
    IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE,
    IAC, WILL, TELOPT_ECHO,
    IAC, WILL, TELOPT_SGA
  };

  int n, len;
  char *cmd;
  int rset;
  struct timeval to;
  char buf[64];

  /* --------------------------------------------------- */
  /* init telnet protocol				 */
  /* --------------------------------------------------- */

  cmd = svr;

  for (n = 0; n < 4; n++)
  {
    len = (n == 1 ? 6 : 3);
    send(0, cmd, len, 0);
    cmd += len;

    rset = 1;
    /* Thor.981221: for future reservation bug */
    to.tv_sec = 1;
    to.tv_usec = 1;
    if (select(1, (fd_set *) & rset, NULL, NULL, &to) > 0)
      recv(0, buf, sizeof(buf), 0);
  }
}


/* ----------------------------------------------------- */
/* 支援超過 24 列的畫面					 */
/* ----------------------------------------------------- */


static void
term_init()
{
#if 0   /* fuse.030518: 註解 */
  server問：你會改變行列數嗎？(TN_NAWS, Negotiate About Window Size)
  client答：Yes, I do. (TNCH_DO)

  那麼在連線時，當TERM變化行列數時就會發出：
  TNCH_IAC + TNCH_SB + TN_NAWS + 行數列數 + TNCH_IAC + TNCH_SE;
#endif

  /* ask client to report it's term size */
  static char svr[] = 		/* server */
  {
    IAC, DO, TELOPT_NAWS
  };

  int rset;
  char buf[64], *rcv;
  struct timeval to;

  initscr();   

  /* 問對方 (telnet client) 有沒有支援不同的螢幕寬高 */
  send(0, svr, 3, 0);

  rset = 1;
  to.tv_sec = 1;
  to.tv_usec = 1;
  if (select(1, (fd_set *) & rset, NULL, NULL, &to) > 0)
    recv(0, buf, sizeof(buf), 0);

  rcv = NULL;
  if ((uschar) buf[0] == IAC && buf[2] == TELOPT_NAWS)
  {
    /* gslin: Unix 的 telnet 對有無加 port 參數的行為不太一樣 */
    if ((uschar) buf[1] == SB)
    {
      rcv = buf + 3;
    }
    else if ((uschar) buf[1] == WILL)
    {
      if ((uschar) buf[3] != IAC)
      {
	rset = 1;
	to.tv_sec = 1;
	to.tv_usec = 1;
	if (select(1, (fd_set *) & rset, NULL, NULL, &to) > 0)
	  recv(0, buf + 3, sizeof(buf) - 3, 0);
      }
      if ((uschar) buf[3] == IAC && (uschar) buf[4] == SB && buf[5] == TELOPT_NAWS)
	rcv = buf + 6;
    }
  }

  if (rcv)
  {
    b_lines = ntohs(* (short *) (rcv + 2)) - 1;
    b_cols = ntohs(* (short *) rcv) - 1;

    /* b_lines 至少要 23，最多不能超過 T_LINES - 1 */
    if (b_lines >= T_LINES)
      b_lines = T_LINES - 1;
    else if (b_lines < 23)
      b_lines = 23;
    /* b_cols 至少要 79，最多不能超過 T_COLS - 1 */
    if (b_cols >= T_COLS)
      b_cols = T_COLS - 1;
    else if (b_cols < 79)
      b_cols = 79;
  }
  else
  {
    b_lines = 23;
    b_cols = 79;
  }

  resizeterm(b_lines + 1, b_cols + 1);

  d_cols = b_cols - 79;
}


/* ----------------------------------------------------- */
/* stand-alone daemon					 */
/* ----------------------------------------------------- */


static void
start_daemon(port)
  int port; /* Thor.981206: 取 0 代表 *沒有參數* , -1 代表 -i (inetd) */
  /* Davy.171001: 取 -2 代表 -p (proxy mode) */
{
  int n;
  struct linger ld;
  struct sockaddr_in sin;
#ifdef HAVE_RLIMIT
  struct rlimit limit;
#endif
  char buf[80], data[80];
  time_t val;
  proxy_connection_info_t proxy_connection_info;

  /*
   * More idiot speed-hacking --- the first time conversion makes the C
   * library open the files containing the locale definition and time zone.
   * If this hasn't happened in the parent process, it happens in the
   * children, once per connection --- and it does add up.
   */

  time(&val);
  strftime(buf, 80, "%d/%b/%Y %H:%M:%S", localtime(&val));

#ifdef HAVE_RLIMIT
  /* --------------------------------------------------- */
  /* adjust resource : 16 mega is enough		 */
  /* --------------------------------------------------- */

  limit.rlim_cur = limit.rlim_max = 16 * 1024 * 1024;
  /* setrlimit(RLIMIT_FSIZE, &limit); */
  setrlimit(RLIMIT_DATA, &limit);

#ifdef SOLARIS
#define RLIMIT_RSS RLIMIT_AS	/* Thor.981206: port for solaris 2.6 */
#endif

  setrlimit(RLIMIT_RSS, &limit);

  limit.rlim_cur = limit.rlim_max = 0;
  setrlimit(RLIMIT_CORE, &limit);

  limit.rlim_cur = limit.rlim_max = 60 * 20;
  setrlimit(RLIMIT_CPU, &limit);
#endif

  /* --------------------------------------------------- */
  /* speed-hacking DNS resolve				 */
  /* --------------------------------------------------- */

  dns_init();

  /* --------------------------------------------------- */
  /* change directory to bbshome       			 */
  /* --------------------------------------------------- */

  chdir(BBSHOME);
  umask(077);

  /* --------------------------------------------------- */
  /* detach daemon process				 */
  /* --------------------------------------------------- */

  /* The integer file descriptors associated with the streams
     stdin, stdout, and stderr are 0,1, and 2, respectively. */

  close(1);
  close(2);

  if (port == -2) /* Davy.171001: proxy mode */
  {
    /* Give up root privileges: no way back from here */
    setgid(BBSGID);
    setuid(BBSUID);
    n = sizeof(proxy_connection_info);
    recv(0, &proxy_connection_info, n, 0); /* Read remote infos */
    connection_sin.sin_family = AF_INET;
    connection_sin.sin_port = proxy_connection_info.connect_port;
    connection_sin.sin_addr.s_addr = proxy_connection_info.source_addr;
    port = ntohs(connection_sin.sin_port);

    return;
  }

  if (port == -1) /* Thor.981206: inetd -i */
  {
    /* Give up root privileges: no way back from here	 */
    setgid(BBSGID);
    setuid(BBSUID);
#if 1
    n = sizeof(sin);
    if (getsockname(0, (struct sockaddr *) &sin, &n) >= 0)
      port = ntohs(sin.sin_port);
#endif
    /* mport = port; */ /* Thor.990325: 不需要了:P */

    sprintf(data, "%d\t%s\t%d\tinetd -i\n", getpid(), buf, port);
    f_cat(PID_FILE, data);
    return;
  }

  close(0);

  if (fork())
    exit(0);

  setsid();

  if (fork())
    exit(0);

  /* --------------------------------------------------- */
  /* fork daemon process				 */
  /* --------------------------------------------------- */

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;

  if (port == 0) /* Thor.981206: port 0 代表沒有參數 */
  {
    n = MAX_BBSDPORT - 1;
    while (n)
    {
      if (fork() == 0)
	break;

      sleep(1);
      n--;
    }
    port = myports[n];
  }

  n = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  val = 1;
  setsockopt(n, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(val));

  ld.l_onoff = ld.l_linger = 0;
  setsockopt(n, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld));

  /* mport = port; */ /* Thor.990325: 不需要了:P */
  sin.sin_port = htons(port);
  if ((bind(n, (struct sockaddr *) &sin, sizeof(sin)) < 0) || (listen(n, QLEN) < 0))
    exit(1);

  /* --------------------------------------------------- */
  /* Give up root privileges: no way back from here	 */
  /* --------------------------------------------------- */

  setgid(BBSGID);
  setuid(BBSUID);

  /* standalone */
  sprintf(data, "%d\t%s\t%d\n", getpid(), buf, port);
  f_cat(PID_FILE, data);
}


/* ----------------------------------------------------- */
/* reaper - clean up zombie children			 */
/* ----------------------------------------------------- */


static inline void
reaper()
{
  while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
}


#ifdef	SERVER_USAGE
static void
servo_usage()
{
  struct rusage ru;
  FILE *fp;

  fp = fopen("run/bbs.usage", "a");

  if (!getrusage(RUSAGE_CHILDREN, &ru))
  {
    fprintf(fp, "\n[Server Usage] %d: %d\n\n"
      "user time: %.6f\n"
      "system time: %.6f\n"
      "maximum resident set size: %lu P\n"
      "integral resident set size: %lu\n"
      "page faults not requiring physical I/O: %d\n"
      "page faults requiring physical I/O: %d\n"
      "swaps: %d\n"
      "block input operations: %d\n"
      "block output operations: %d\n"
      "messages sent: %d\n"
      "messages received: %d\n"
      "signals received: %d\n"
      "voluntary context switches: %d\n"
      "involuntary context switches: %d\n\n",

      getpid(), ap_start,
      (double) ru.ru_utime.tv_sec + (double) ru.ru_utime.tv_usec / 1000000.0,
      (double) ru.ru_stime.tv_sec + (double) ru.ru_stime.tv_usec / 1000000.0,
      ru.ru_maxrss,
      ru.ru_idrss,
      ru.ru_minflt,
      ru.ru_majflt,
      ru.ru_nswap,
      ru.ru_inblock,
      ru.ru_oublock,
      ru.ru_msgsnd,
      ru.ru_msgrcv,
      ru.ru_nsignals,
      ru.ru_nvcsw,
      ru.ru_nivcsw);
  }

  fclose(fp);
}
#endif


static void
main_term()
{
#ifdef	SERVER_USAGE
  servo_usage();
#endif
  exit(0);
}


static inline void
main_signals()
{
  struct sigaction act;

  /* act.sa_mask = 0; */ /* Thor.981105: 標準用法 */
  sigemptyset(&act.sa_mask);      
  act.sa_flags = 0;

  act.sa_handler = reaper;
  sigaction(SIGCHLD, &act, NULL);

  act.sa_handler = main_term;
  sigaction(SIGTERM, &act, NULL);

#ifdef	SERVER_USAGE
  act.sa_handler = servo_usage;
  sigaction(SIGPROF, &act, NULL);
#endif

  /* sigblock(sigmask(SIGPIPE)); */
}


static void*
thread_QueryDNS(addr)
  void *addr;
{
  char from_buf[48];

  dns_name((unsigned char *) addr, from_buf);

  pthread_mutex_lock(&rslv_mutex);
  strcpy(fromhost, from_buf);

  if(cutmp)	/* dust.101005: UTMP已經初始化完畢, update它 */
  {
    str_ncpy(cutmp->fromhost, fromhost, sizeof(cutmp->fromhost));
#ifdef HAVE_WHERE
#  ifdef GUEST_WHERE
    if(cuser.userlevel)
#  endif
      find_fromwhere(cutmp->from, sizeof(cutmp->from));
#else
    str_ncpy(cutmp->from, fromhost, sizeof(cutmp->from));
#endif
  }

  pthread_mutex_unlock(&rslv_mutex);

  return NULL;
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int csock;			/* socket for Master and Child */
  int value;
  int proxy_mode;
  int *totaluser;
  pthread_t rslv_trd;

  /* --------------------------------------------------- */
  /* setup standalone daemon				 */
  /* --------------------------------------------------- */

  /* Thor.990325: usage, bbsd, or bbsd -i, or bbsd 1234 */
  /* Thor.981206: 取 0 代表 *沒有參數*, -1 代表 -i */
  /* Davy.171001: bbsd -p 代表 proxy 模式，不會另外 accept 連線，使用 -2 */
  value = argc > 1 ? \
    strcmp("-i", argv[1]) ? \
      strcmp("-p", argv[1]) ? atoi(argv[1]) : \
        -2 : \
      -1 : \
    0;
  start_daemon(value);

  main_signals();


  /* --------------------------------------------------- */
  /* attach shared memory & semaphore			 */
  /* --------------------------------------------------- */

#ifdef HAVE_SEM
  sem_init();
#endif
  ushm_init();
  bshm_init();
  fshm_init();
  vo_init();

  /* --------------------------------------------------- */
  /* main loop						 */
  /* --------------------------------------------------- */

  totaluser = &ushm->count;
  /* avgload = &ushm->avgload; */

  proxy_mode = argc > 1 && !strcmp("-p", argv[1]); /* 0: no, 1: yes, 2: stop loop */

  for (; proxy_mode < 2;)
  {
    if (proxy_mode)
    {
      proxy_mode = 2; /* We are not going to run twice loop. */
      csock = 0;
    }
    else /* normal mode */
    {
      value = 1;
      if (select(1, (fd_set *) & value, NULL, NULL, NULL) < 0)
        continue;

      value = sizeof(connection_sin);
      csock = accept(0, (struct sockaddr *) &connection_sin, &value);
      if (csock < 0)
      {
        reaper();
        continue;
      }
    }

    ap_start++;
    argc = *totaluser;
    if (argc >= MAXACTIVE - 5 /* || *avgload > THRESHOLD */ )
    {
      /* 借用 currtitle */
      sprintf(currtitle, "目前線上人數 [%d] 人，系統飽和，請稍後再來\n", argc);
      send(csock, currtitle, strlen(currtitle), 0);
      close(csock);
      continue;
    }

    if (!proxy_mode)
    {
      if (fork())
      {
        close(csock);
        continue;
      }

      dup2(csock, 0);
      close(csock);
    }

    /* ------------------------------------------------- */
    /* ident remote host / user name via RFC931		 */
    /* ------------------------------------------------- */

    tn_addr = connection_sin.sin_addr.s_addr;

    /* dust.101004: paralell DNS query by Pthread */
    {
      uschar *addr = (uschar*) &tn_addr;
      sprintf(fromhost, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
    }
    cutmp = NULL;
    pthread_create(&rslv_trd, NULL, thread_QueryDNS, &connection_sin.sin_addr);

    telnet_init();
    term_init();
    tn_main();
  }
}

