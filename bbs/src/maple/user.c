/*-------------------------------------------------------*/
/* user.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : account / user routines		 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern char *ufo_tbl[];


/* ----------------------------------------------------- */
/* 認證用函式						 */
/* ----------------------------------------------------- */


void 
justify_log (	/* itoc.010822: 拿掉 .ACCT 中 justify 這欄位，改記錄在 FN_JUSTIFY */
    char *userid,
    char *justify	/* 認證資料 RPY:email-reply  KEY:認證碼  POP:pop3認證  REG:註冊單 */
)
{
  char fpath[64];
  FILE *fp;

  usr_fpath(fpath, userid, FN_JUSTIFY);
  if (fp = fopen(fpath, "a"))		/* 用附加檔案，可以保存歷次認證記錄 */
  {
    fprintf(fp, "%s\n", justify);
    fclose(fp);
  }
}


static int 
ban_addr (char *addr)
{
  char *host;
  char foo[128];	/* SoC: 放置待檢查的 email address */

  /* Thor.991112: 記錄用來認證的email */
  sprintf(foo, "%s # %s (%s)\n", addr, cuser.userid, Now());
  f_cat(FN_RUN_EMAILREG, foo);

  /* SoC: 保持原 email 的大小寫 */
  str_lower(foo, addr);

  /* check for acl (lower case filter) */

  host = (char *) strchr(foo, '@');
  *host = '\0';

  /* *.bbs@xx.yy.zz、*.brd@xx.yy.zz 一律不接受 */
  if (host > foo + 4 && (!str_cmp(host - 4, ".bbs") || !str_cmp(host - 4, ".brd")))
    return 1;

  /* 不在白名單上或在黑名單上 */
  return (!acl_has(TRUST_ACLFILE, foo, host + 1) ||
    acl_has(UNTRUST_ACLFILE, foo, host + 1) > 0);
}


/* ----------------------------------------------------- */
/* POP3 認證						 */
/* ----------------------------------------------------- */


#ifdef HAVE_POP3_CHECK

static int 
Get_Socket (	/* site for hostname */
    char *site
)
{
  int sock;
  struct sockaddr_in sin;
  struct hostent *host;

  sock = 110;

  /* Getting remote-site data */

  memset((char *) &sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(sock);

  if (!(host = gethostbyname(site)))
    sin.sin_addr.s_addr = inet_addr(site);
  else
    memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);

  /* Getting a socket */

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    return -1;
  }

  /* perform connecting */

  if (connect(sock, (struct sockaddr *) & sin, sizeof(sin)) < 0)
  {
    close(sock);
    return -1;
  }

  return sock;
}


static int 
POP3_Check (int sock, char *account, char *passwd)
{
  FILE *fsock;
  char buf[512];

  if (!(fsock = fdopen(sock, "r+")))
  {
    outs("\n傳回錯誤值，請重試幾次看看\n");
    return -1;
  }

  sock = 1;

  while (1)
  {
    switch (sock)
    {
    case 1:		/* Welcome Message */
      fgets(buf, sizeof(buf), fsock);
      break;

    case 2:		/* Verify Account */
      fprintf(fsock, "user %s\r\n", account);
      fflush(fsock);
      fgets(buf, sizeof(buf), fsock);
      break;

    case 3:		/* Verify Password */
      fprintf(fsock, "pass %s\r\n", passwd);
      fflush(fsock);
      fgets(buf, sizeof(buf), fsock);
      sock = -1;
      break;

    default:		/* 0:Successful 4:Failure  */
      fprintf(fsock, "quit\r\n");
      fclose(fsock);
      return sock;
    }

    if (!strncmp(buf, "+OK", 3))
    {
      sock++;
    }
    else
    {
      outs("\n遠端系統傳回錯誤訊息如下：\n");
      prints("%s\n", buf);
      sock = -1;
    }
  }
}


static int 
do_pop3 (		/* itoc.010821: 改寫一下 :) */
    char *addr
)
{
  int sock, i;
  char *ptr, *str, buf[80], username[80];
  char *alias[] = {"", "pop3.", "mail.", NULL};
  ACCT acct;

  strcpy(username, addr);
  *(ptr = strchr(username, '@')) = '\0';
  ptr++;

  clear();
  move(2, 0);
  prints("主機: %s\n帳號: %s\n", ptr, username);
  outs("\033[1;5;36m連線遠端主機中...請稍候\033[m\n");
  refresh();

  for (i = 0; str = alias[i]; i++)
  {
    sprintf(buf, "%s%s", str, ptr);	/* itoc.020120: 主機名稱加上 pop3. 試試看 */
    if ((sock = Get_Socket(buf)) >= 0)	/* 找到這機器且對方支援 POP3 */
      break;
  }

  if (sock < 0)
  {
    outs("您的電子郵件系統不支援 POP3 認證，使用認證信函身分確認\n\n\033[1;36;5m系統送信中...\033[m");
    return -1;
  }

  if (vget(15, 0, "請輸入以上所列出之工作站帳號的密碼：", buf, 20, NOECHO))
  {
    move(17, 0);
    outs("\033[5;37m身分確認中...請稍候\033[m\n");

    if (!POP3_Check(sock, username, buf))	/* POP3 認證成功 */
    {
      /* 提升權限 */
      sprintf(buf, "POP: %s", addr);
      justify_log(cuser.userid, buf);
      strcpy(cuser.email, addr);
      if (acct_load(&acct, cuser.userid) >= 0)
      {
	time(&acct.tvalid);
	acct_setperm(&acct, PERM_VALID, 0);
      }

      /* 寄信通知使用者 */
      mail_self(FN_ETC_JUSTIFIED, str_sysop, msg_reg_valid, 0);
      cutmp->status |= STATUS_BIFF;
      vmsg(msg_reg_valid);

      close(sock);
      return 1;
    }
  }

  close(sock);

  /* POP3 認證失敗 */
  outs("您的密碼或許\打錯了，使用認證信函身分確認\n\n\033[1;36;5m系統送信中...\033[m");
  return 0;
}
#endif


/* ----------------------------------------------------- */
/* 設定 E-mail address					 */
/* ----------------------------------------------------- */


int 
u_addr (void)
{
  char *msg, addr[64];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  /* itoc.050405: 不能讓停權者重新認證，因為會改掉他的 tvalid (停權到期時間) */
  if (HAS_PERM(PERM_ALLDENY))
  {
    vmsg("您被停權，無法改信箱");
    return XEASY;
  }

  film_out(FILM_EMAIL, 0);

  if (vget(b_lines - 2, 0, "E-mail 地址：", addr, sizeof(cuser.email), DOECHO))
  {
    if (not_addr(addr))
    {
      msg = err_email;
    }
    else if (ban_addr(addr))
    {
      msg = "本站不接受您的信箱做為認證地址";
    }
    else
    {
#ifdef EMAIL_JUSTIFY
      if (vans("修改 E-mail 要重新認證，確定要修改嗎(Y/N)？[Y] ") == 'n')
	return 0;

#  ifdef HAVE_POP3_CHECK
      if (vans("是否使用 POP3 認證(Y/N)？[N] ") == 'y')
      {
	if (do_pop3(addr) > 0)	/* 若 POP3 認證成功，則離開，否則以認證信寄出 */
	  return 0;
      }
#  endif

#error dust.101220: this code have not adapted to new version yet.
      if (bsmtp(NULL, NULL, addr, MQ_JUSTIFY) < 0)
      {
	msg = "身分認證信函無法寄出，請正確填寫 E-mail address";
      }
      else
      {
	ACCT acct;

	strcpy(cuser.email, addr);
	cuser.userlevel &= ~PERM_ALLVALID;
	if (acct_load(&acct, cuser.userid) >= 0)
	{
	  strcpy(acct.email, addr);
	  acct_setperm(&acct, 0, PERM_ALLVALID);
	}

	film_out(FILM_JUSTIFY, 0);
	prints("\n%s(%s)您好，由於您更新 E-mail address 的設定，\n\n"
	  "請您儘快到 \033[44m%s\033[m 所在的工作站回覆『身分認證信函』。",
	  cuser.userid, cuser.username, addr);
	msg = NULL;
      }
#else
      msg = NULL;
#endif

    }
    vmsg(msg);
  }

  return 0;
}


/* ----------------------------------------------------- */
/* 填寫註冊單						 */
/* ----------------------------------------------------- */


#ifdef HAVE_REGISTER_FORM

static void 
getfield (int line, int len, char *buf, char *desc, char *hint, char *ex)
{
  move(line - 1, 0);
  if(ex)
    outs(ex);

  move(line + 1, 0);
  if(hint)
    outs(hint);

  vget(line, 0, desc, buf, len, GCARRY);
}


int 
u_register (void)
{
  FILE *fn;
  int ans;
  RFORM rform;
  ACCT acct;
  char fpath[64];
#ifdef USE_DAVY_REGISTER
  int fd;
  char regpath[IDLEN + 10];
#endif

#ifdef JUSTIFY_PERIODICAL
  if (HAS_PERM(PERM_VALID) && cuser.tvalid + VALID_PERIOD - INVALID_NOTICE_PERIOD >= ap_start)
#else
  if (HAS_PERM(PERM_VALID))
#endif
  {
    zmsg("您的身分確認已經完成，不需填寫申請表");
    return XEASY;
  }

#ifdef USE_DAVY_REGISTER
  sprintf(regpath, "reg/%s", cuser.userid);
  str_lower(regpath, regpath);
  if (dashf(regpath))
  {
    zmsg("您的註冊申請單尚在處理中，請耐心等候");
    return XEASY;
  }
#else
  if (fn = fopen(FN_RUN_RFORM, "rb"))
  {
    while (fread(&rform, sizeof(RFORM), 1, fn))
    {
      if ((rform.userno == cuser.userno) && !strcmp(rform.userid, cuser.userid))
      {
	fclose(fn);
	zmsg("您的註冊申請單尚在處理中，請耐心等候");
	return XEASY;
      }
    }
    fclose(fn);
  }
#endif

  if (vans("您確定要填寫註冊單嗎(Y/N)？[N] ") != 'y')
    return XEASY;

  move(1, 0);
  clrtobot();
  prints("\n%s(%s) 您好，請據實填寫以下的資料：\n(按 Enter鍵 完成輸入)",
    cuser.userid, cuser.username);

  memset(&rform, 0, sizeof(RFORM));
  if(acct_load(&acct, cuser.userid) < 0)
    return 0;

#define hint0 "這邊提供修改的機會，如果真實姓名亂填的話註冊單會被退件。"
#define hint1 "\033[1m學校+系別+級數\033[m(民國幾年畢業，\033[1;31m請不要填年級\033[m) >或 \033[1m單位+職稱\033[m"
#define hint2 "包括寢室號碼或門牌號碼。總之請以\033[1;32m信件能寄達\033[m為原則來填寫。"
#define hint3 "要包含\033[1;33m長途撥號的區域碼\033[m。"

#define ex1 "\033[1;30m範例：[\033[37m台中一中99級\033[30m] or [\033[37m台大資工系102級\033[30m] or [\033[37m台灣微軟執行長\033[30m]\033[m"
#define ex2 "\033[1;30m範例：[\033[37m台中市育才街3號\033[30m] or [\033[37m台中一中第一宿舍102室\033[30m]\033[m"
#define ex3 "\033[1;30m範例：[\033[37m(04)22226081\033[30m] or [\033[37m0912345678\033[30m]\033[m"

  for (;;)
  {
    getfield(6, RNLEN + 1, acct.realname, "真實姓名：", hint0, NULL);
    getfield(10, 50, rform.career, "服務單位：", hint1, ex1);
    getfield(14, 60, rform.address, "目前住址：", hint2, ex2);
    getfield(18, 20, rform.phone, "聯絡電話：", hint3, ex3);

    ans = vans("以上資料是否正確(Y/N/Q)？[N] ");
    if (ans == 'q')
      return 0;
    if (ans == 'y')
      break;
  }
#undef hint0
#undef hint1
#undef hint2
#undef hint3
#undef ex1
#undef ex2
#undef ex3

  acct_save(&acct);
  rform.userno = cuser.userno;
  strcpy(rform.userid, cuser.userid);
  time(&rform.rtime);
#ifdef USE_DAVY_REGISTER
  if ((fd = open(regpath, O_RDWR | O_CREAT, 0600)) >= 0)
  {
    write(fd, &rform, sizeof(RFORM));
    close(fd);
  }
#else
  rec_add(FN_RUN_RFORM, &rform, sizeof(RFORM));
#endif

  /* 寫進使用者側的註冊單資料檔 */
  usr_fpath(fpath, cuser.userid, FN_RFINFO);
  if(fn = fopen(fpath, "w"))
  {
    fprintf(fn, "%s\n", rform.phone);
    fprintf(fn, "%s\n", rform.address);
    fclose(fn);
  }
  return 0;
}
#endif


/* ----------------------------------------------------- */
/* 填寫註認碼						 */
/* ----------------------------------------------------- */


#ifdef HAVE_REGKEY_CHECK
int 
u_verify (void)
{
  char buf[80], key[10];
  ACCT acct;
  
  if (HAS_PERM(PERM_VALID))
  {
    zmsg("您的身分確認已經完成，不需填寫認證碼");
  }
  else
  {
    if (vget(b_lines, 0, "請輸入認證碼：", buf, 8, DOECHO))
    {
      archiv32(str_hash(cuser.email, cuser.tvalid), key);	/* itoc.010825: 不用開檔了，直接拿 tvalid 來比就是了 */

      if (str_ncmp(key, buf, 7))
      {
	zmsg("抱歉，您的認證碼錯誤");      
      }
      else
      {
	/* 提升權限 */
	sprintf(buf, "KEY: %s", cuser.email);
	justify_log(cuser.userid, buf);
	if (acct_load(&acct, cuser.userid) >= 0)
	{
	  time(&acct.tvalid);
	  acct_setperm(&acct, PERM_VALID, 0);
	}

	/* 寄信通知使用者 */
	mail_self(FN_ETC_JUSTIFIED, str_sysop, msg_reg_valid, 0);
	cutmp->status |= STATUS_BIFF;
	vmsg(msg_reg_valid);
      }
    }
  }

  return XEASY;
}
#endif


/* ----------------------------------------------------- */
/* 恢復權限						 */
/* ----------------------------------------------------- */


int 
u_deny (void)
{
  ACCT acct;
  time_t diff;
  struct tm *ptime;
  char msg[80];

  if (!HAS_PERM(PERM_ALLDENY))
  {
    zmsg("您沒被停權，不需復權");
  }
  else
  {
    if ((diff = cuser.tvalid - time(0)) < 0)      /* 停權時間到了 */
    {
      if (acct_load(&acct, cuser.userid) >= 0)
      {
	time(&acct.tvalid);
#ifdef JUSTIFY_PERIODICAL
	/* xeon.050112: 在認證快到期前時 Cross-Post，然後 tvalid 就會被設定到未來時間，
	   等復權時間到了去復權，這樣就可以避過重新認證，所以復權後要重新認證。 */
	acct_setperm(&acct, 0, PERM_ALLVALID | PERM_ALLDENY);
#else
	acct_setperm(&acct, 0, PERM_ALLDENY);
#endif
	vmsg("下次請勿再犯，請重新上站");
      }
    }
    else
    {
      ptime = gmtime(&diff);
      sprintf(msg, "您還要等 %d 年 %d 天 %d 時 %d 分 %d 秒才能申請復權",
	ptime->tm_year - 70, ptime->tm_yday, ptime->tm_hour, ptime->tm_min, ptime->tm_sec);
      vmsg(msg);
    }
  }

  return XEASY;
}


/* ----------------------------------------------------- */
/* 個人工具						 */
/* ----------------------------------------------------- */


int 
u_info (void)
{
  char *str, username[UNLEN + 1];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  move(1, 0); 
  strcpy(username, str = cuser.username);
  acct_setup(&cuser, 0);
  if (strcmp(username, str))
    memcpy(cutmp->username, str, UNLEN + 1);
  return 0;
}


int 
u_setup (void)
{
#if 0
  usint ulevel;
  int len;

  /* itoc.000320: 增減項目要更改 len 大小, 也別忘了改 ufo.h 的旗標 STR_UFO */

  ulevel = cuser.userlevel;
  if (!ulevel)
    len = NUMUFOS_GUEST;
  else if (ulevel & PERM_ALLADMIN)
    len = NUMUFOS;		/* ADMIN 除了可用 acl，也可以用一般的隱身術 */
  else if (ulevel & PERM_VALID)
    len = NUMUFOS - 2;		/* 通過認證者 */
  else
    len = NUMUFOS_USER;

  cuser.ufo = cutmp->ufo = bitset(cuser.ufo, len, len, MSG_USERUFO, ufo_tbl);
#else
  void *p = DL_get("bin/ufo.so:ufo_main");

  if (!p)
  {
    p = dlerror();
    if(p) vmsg((char *)p);
  }

  ((int (*)(ACCT*, UTMP*))p) (&cuser, cutmp);
#endif

  return 0;
}


int 
u_lock (void)
{
  char buf[PSWDLEN + 1];
  char *menu[] = {"y鎖定螢幕", "c鎖定螢幕-自訂狀態", "n取消", NULL};

  switch (krans(b_lines, "yn", menu, NULL))
  {
  case 'c':		/* itoc.011226: 可自行輸入發呆的理由 */
    if (vget(b_lines, 0, "發呆的理由？ ", cutmp->mateid, 9, DOECHO))
      break;

  case 'y':
    strcpy(cutmp->mateid, "掛站");
    break;

  default:
    return XEASY;
  }

  utmp_mode(M_IDLE);

  bbstate |= STAT_LOCK;		/* lkchu.990513: 鎖定時不可回水球 */
  cutmp->status |= STATUS_REJECT;	/* 鎖定時不收水球 */

  lim_cat("etc/locked_screen", 18);
  move(15, 47);
  prints("\033[1;35m<%s>\033[m", cutmp->mateid);

  do
  {
    vget(b_lines, 0, "\033[1;33m★\033[m 請輸入您的密碼以解除螢幕鎖定狀態：", buf, PSWDLEN + 1, NOECHO);
  } while (chkpasswd(cuser.passwd, buf));

  cutmp->status ^= STATUS_REJECT;
  bbstate ^= STAT_LOCK;

  return 0;
}


int 
u_log (void)
{
  char fpath[64];

  usr_fpath(fpath, cuser.userid, FN_LOG);
  more(fpath, MFST_NORMAL);
  return 0;
}


/* ----------------------------------------------------- */
/* 設定檔案						 */
/* ----------------------------------------------------- */

#define LFSX 0
#define RFSX 39
#define MAXLINES 9
#define MAXITEM 18
#define XFILE_FEET "\033[1;33m★ \033[m請按 \033[1;33m方向鍵 \033[m移動游標， \033[1;33mEnter \033[m編輯， \033[1;33mdelete \033[m刪除檔案或 \033[1;33mQ \033[m離開。\n"

void 
x_file (
    int echo_path,
    char *xlist[],		/* description list */
    char *flist[],		/* filename list */
    char *fpath
)
{
  int n, oc, row, col, key, redraw_foot;
  screen_backup_t old_screen;
  char pbuf[64];
  const char const *s, *encode = "123456789ABCDEFGHI";
  int exist[MAXITEM];

  strcpy(pbuf, fpath);
  fpath = pbuf + strlen(pbuf);

  move(MENU_XPOS, 0);
  clrtobot();

  row = MENU_XPOS;
  col = LFSX;
  for(n = 0; xlist[n]; n++, row++)
  {
    if(n >= MAXITEM)
      break;
    else if(n == MAXLINES)
    {
      col = RFSX;
      row = MENU_XPOS;
    }

    if(*flist[n] == '\033')
      exist[n] = 1;
    else
    {
      strcpy(fpath, flist[n]);
      exist[n] = dashf(pbuf);
    }

    move(row, col);
    if(exist[n])
      attrset(7);
    else
      attrset(8);

    prints("   %c) %-14s \033[m", encode[n], xlist[n]);

    if(echo_path)
      outs(flist[n]);
  }

  if(n <= 0)
  {
    vmsg("xlist[] 設定有誤！");
    return;
  }

  redraw_foot = 1;
  row = col = 0;
  oc = -1;
  do
  {
    if(redraw_foot)
    {
      move(b_lines, 0);
      outs(XFILE_FEET);
      redraw_foot = 0;
    }

    if(col * MAXLINES + row != oc)
    {
      if(oc >= 0)
      {
	move(oc % MAXLINES + MENU_XPOS, oc < MAXLINES? LFSX : RFSX);
	if(*flist[oc] != '\033' && !exist[oc])
	  attrset(8);
	else
	  attrset(7);
	prints("   %c) %-14s ", encode[oc], xlist[oc]);
	if(echo_path)
	  outs(flist[oc]);
      }

      oc = col * MAXLINES + row;
      move(row + MENU_XPOS, col? RFSX : LFSX);
      attrset(15);
      prints(">  %c) %-14s ", encode[oc], xlist[oc]);
      if(echo_path)
	outs(flist[oc]);
      attrset(7);
    }

    move(row + MENU_XPOS, col? RFSX + 1 : LFSX + 1);
    key = vkey();

    switch(key)
    {
    case '\n':
      scr_dump(&old_screen);
      oc = col * MAXLINES + row;	/* 借用oc */

      if(*flist[oc] == '\033')	/* Dynamic Link Function */
	DL_func(flist[oc] + 1);
      else
      {
	strcpy(fpath, flist[oc]);
	if(vedit(pbuf, 0) < 0)
	  vmsg("原封不動");
	else
	  vmsg("更新完畢");
      }

      scr_restore(&old_screen);
      break;

    case KEY_LEFT:
      if(col == 0)
	key = 'Q';
      else
	col = 0;
      break;

    case KEY_RIGHT:
      if(n > MAXLINES)
      {
	col = 1;
	if(MAXLINES + row >= n)
	  row = n - MAXLINES - 1;
      }
      break;

    case KEY_UP:
      if(--row < 0)
      {
	row = MAXLINES - 1;
	if(col * MAXLINES + row >= n)
	  row = (n - 1) % MAXLINES;
      }
      break;

    case KEY_DOWN:
      row++;
      if(row >= MAXLINES || col * MAXLINES + row >= n)
	row = 0;
      break;

    case KEY_DEL:
      oc = col * MAXLINES + row;	/* 借用oc */
      if(*flist[oc] != '\033')
      {
	if(vans(msg_sure_ny) == 'y')
	{
	  strcpy(fpath, flist[oc]);
	  unlink(pbuf);
	  exist[oc] = 0;
	}
	redraw_foot = 1;
      }
      break;

    default:
      key = toupper(key);
      if(s = strchr(encode, key))
      {
	int cc = s - encode;
	if(cc < n)
	{
	  col = cc / MAXLINES;
	  row = cc % MAXLINES;
	}
      }
    }

  }while(key != 'Q');
}



int 
u_xfile (void)
{
  int i;
  char fpath[64];

  static char *desc[] = 
  {
    "上站地點設定檔",
    "名片檔",
    "編輯/設定簽名檔",
    "暫存檔.1",
    "暫存檔.2",
    "暫存檔.3",
    "暫存檔.4",
    "暫存檔.5",
    "設定倒數計時器",
    NULL
  };

  static char *path[] = 
  {
    "acl",
    "plans",
    "\033bin/pset.so:u_sign",
    "buf.1",
    "buf.2",
    "buf.3",
    "buf.4",
    "buf.5",
    "\033bin/pset.so:hm_user"
  };

  i = HAS_PERM(PERM_ALLADMIN) ? 0 : 1;
  usr_fpath(fpath, cuser.userid, "");
  x_file(0, &desc[i], &path[i], fpath);
  return 0;
}

