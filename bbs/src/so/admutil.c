/*-------------------------------------------------------*/
/* admutil.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 站長指令					 */
/* create : 95/03/29					 */
/* update : 01/03/01					 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;
extern UCACHE *ushm;

extern time_t sudo_uptime;


static int
a_check()	/* dust.100323: 站務身分確認 進階版 (pseudo 'sudo') */
		/* xatier. 灰塵大大超強的 */
		/* floatj. 灰塵貓是我們的神w */
{
  char pswd[32];
  screen_backup_t old_screen;

  if(time(NULL) - sudo_uptime > 300)
  {
    sudo_uptime = 0;
    scr_dump(&old_screen);
    move(1, 0);
    clrtoln(13);
    outs(SUDO_NOTE);

    if(!vget(b_lines, 0, "\033[1;44m【提示】\033[33;40m 你的密碼有幾個三明治長www?\033[;33;40m(Timelimit: 5 min)\033[m", pswd, PSWDLEN + 1, NOECHO))
    {
      free(old_screen.raw_memory);
      return 0;
    }

    if(chkpasswd(cuser.passwd, pswd))
    {
      vmsg("Sorry, try again.");
      free(old_screen.raw_memory);
      return 0;
    }

    sudo_uptime = time(NULL);
    scr_restore(&old_screen);
  }

  return 1;
}


/* ----------------------------------------------------- */
/* 站務指令						 */
/* ----------------------------------------------------- */


int
a_user()
{
  int ans;
  ACCT acct;
  
  if (!a_check())
    return 0;

  move(1, 0);
  clrtobot();

  while (ans = acct_get(msg_uid, &acct))
  {
    if (ans > 0)
      acct_setup(&acct, 1);
  }
  return 0;
}


int
a_search()	/* itoc.010902: 暴力搜尋使用者 */
{
  ACCT acct;
  char c;
  char key[30];

  if (!a_check())
    return 0;

  if (!vget(b_lines, 0, "請輸入關鍵字(姓名/暱稱/來源/信箱)：", key, 30, DOECHO))
    return XEASY;  

  /* itoc.010929.註解: 真是有夠暴力 :p 考慮先由 reaper 做出一個 .PASSWDS 再去找 */

  for (c = 'a'; c <= 'z'; c++)
  {
    char buf[64];
    struct dirent *de;
    DIR *dirp;

    sprintf(buf, "usr/%c", c);
    if (!(dirp = opendir(buf)))
      continue;

    while (de = readdir(dirp))
    {
      if (acct_load(&acct, de->d_name) < 0)
	continue;

      if (strstr(acct.realname, key) || strstr(acct.username, key) ||  
	strstr(acct.lasthost, key) || strstr(acct.email, key))
      {
	move(1, 0);
	acct_setup(&acct, 1);

	if (vans("是否繼續搜尋下一筆？[N] ") != 'y')
	{
	  closedir(dirp);
 	  goto end_search;
 	}
      }
    }
    closedir(dirp);
  }
end_search:
  vmsg("搜尋完畢");
  return 0;
}


int
a_editbrd()		/* itoc.010929: 修改看板選項 */
{
  int bno;
  BRD *brd;
  char bname[BNLEN + 1];

  if(!a_check())
    return 0;

  bbstate &= ~STAT_CANCEL;
  if (brd = ask_board(bname, BRD_L_BIT, NULL))
  {
    bno = brd - bshm->bcache;
    brd_edit(bno);
  }
  else if(!(bbstate & STAT_CANCEL))
  {
    vmsg(err_bid);
  }

  return 0;
}


/* floatJ: 幻符 */
int
circ_xfile()               /* 設定特殊檔案(circ god使用) */
{

  /* 1. 只有 sysop 才可以繼續  2. 順便log */
  if (strcmp(cuser.userid, "sysop"))
  {
    vmsg("只有 sysop 才可以繼續唷 >.^");
    return 0;
  }
    
  
  circlog("以sysop登入，並編輯召喚名單", fromhost);

  static char *desc[] =
  {
   "召喚電研神名單",
    NULL
  };

  static char *path[] =
  {
    FN_ETC_CIRCGOD,
  };

  x_file(1, desc, path, "");
  return 0;
}


int
a_xfile()		/* 設定系統檔案 */
{
  if (!a_check())
    return 0;
  static char *desc[] =
  {
    "看板文章期限",

    "身分認證信函",
    "認證通過通知",
    "重新認證通知",

#ifdef HAVE_DETECT_CROSSPOST
    "跨貼停權通知",
#endif
    
    "不雅名單",
    "站務名單",

    "節日",

#ifdef HAVE_WHERE
    "故鄉 ID",
    "故鄉 IP",
    "故鄉 FQDN",
#endif

#ifdef HAVE_TIP
    "每日小秘訣",
#endif

#ifdef HAVE_LOVELETTER
    "情書產生器文庫",
#endif

    "認證白名單",
    "認證黑名單",

    "收信白名單",
    "收信黑名單",

#ifdef HAVE_LOGIN_DENIED
    "拒絕連線名單",
#endif

    NULL
  };

  static char *path[] =
  {
    FN_ETC_EXPIRE,

    FN_ETC_VALID,
    FN_ETC_JUSTIFIED,
    FN_ETC_REREG,

#ifdef HAVE_DETECT_CROSSPOST
    FN_ETC_CROSSPOST,
#endif
    
    FN_ETC_BADID,
    FN_ETC_SYSOP,

    FN_ETC_FEAST,

#ifdef HAVE_WHERE
    FN_ETC_IDHOME,
    FN_ETC_HOST,
    FN_ETC_FQDN,
#endif

#ifdef HAVE_TIP
    FN_ETC_TIP,
#endif

#ifdef HAVE_LOVELETTER
    FN_ETC_LOVELETTER,
#endif

    TRUST_ACLFILE,
    UNTRUST_ACLFILE,

    MAIL_ACLFILE,
    UNMAIL_ACLFILE,

#ifdef HAVE_LOGIN_DENIED
    BBS_ACLFILE,
#endif
  };

  x_file(1, desc, path, "");
  return 0;
}


int
a_resetsys()		/* 重置 */
{
  switch (vans("◎ 系統重設 1)動態看板 2)分類群組 3)指名及擋信 4)全部：[Q] "))
  {
  case '1':
    system("bin/camera -quiet");
    break;

  case '2':
    system("bin/account -nokeeplog");
    rhs_save();
    board_main();
    break;

  case '3':
    system("kill -1 `cat run/bmta.pid`; kill -1 `cat run/bguard.pid`");
    break;

  case '4':
    system("kill -1 `cat run/bmta.pid`; kill -1 `cat run/bguard.pid`; bin/account -nokeeplog; bin/camera -quiet");
    rhs_save();
    board_main();
    break;
  }

  return XEASY;
}

#define CMD_MAKE "make linux install > ~/err.txt"
#define CMD_RESTART "~/runbbs restart > ~/err.txt"
#define CMD_STDOUT "/home/bbs/err.txt"
int a_cmd() {
  char ans;
  char buf[2000];
  FILE *fp;

  ans = 'c';
  ans = vans("MAKE/RESTART/CANCEL? (M/R/C)[C] :");

  chdir(BBSHOME "/src/maple") ;
/*
  switch(ans) {
	case 'm':
		system(CMD_MAKE);
		break;
	case 'r':
		system(CMD_RESTART);
		break;
	default :
		return 0;
  }
*/
  clear();
  sleep(3);
  fp = fopen(CMD_STDOUT,"r");
  while (fgets(buf,2000,fp)!=NULL) {
  	outs(buf);
  }
  fclose(fp);
  
  return 0;
}

/* ----------------------------------------------------- */
/* 還原備份檔						 */
/* ----------------------------------------------------- */


static void
show_availability(type)		/* 將 BAKPATH 裡面所有可取回備份的目錄印出來 */
  char *type;
{
  int tlen, len, col;
  char *fname, fpath[64];
  struct dirent *de;
  DIR *dirp;
  FILE *fp;

  if (dirp = opendir(BAKPATH))
  {
    col = 0;
    tlen = strlen(type);

    sprintf(fpath, "tmp/restore.%s", cuser.userid);
    fp = fopen(fpath, "w");
    fputs("※ 可供取回的備份有：\n\n", fp);

    while (de = readdir(dirp))
    {
      fname = de->d_name;
      if (!strncmp(fname, type, tlen))
      {
	len = strlen(fname) + 2;
	if (b_cols - col < len)
	{
	  fputc('\n', fp);
	  col = len;
	}
	else
	{
	  col += len;
	}
	fprintf(fp, "%s  ", fname);
      }
    }

    fputc('\n', fp);
    fclose(fp);
    closedir(dirp);

    more(fpath, MFST_NONE);
    unlink(fpath);
  }
}


int
a_restore()
{
  int ch;
  char *type, *ptr;
  char *tpool[3] = {"brd", "gem", "usr"};
  char date[20], brdname[BNLEN + 1], src[64], cmd[256];
  ACCT acct;
  BPAL *bpal;

  ch = vans("◎ 還原備份 1)看板 2)精華區 3)使用者：[Q] ") - '1';
  if (ch < 0 || ch >= 3)
    return XEASY;

  type = tpool[ch];
  show_availability(type);

  if (vget(b_lines, 0, "要取回的備份目錄：", date, 20, DOECHO))
  {
    /* 避免站長打了一個存在的目錄，但是和 type 不合 */
    if (strncmp(date, type, strlen(type)))
      return 0;

    sprintf(src, BAKPATH"/%s", date);
    if (!dashd(src))
      return 0;
    ptr = strchr(src, '\0');

    clear();
    move(3, 0);
    outs("欲還原備份的看板/使用者必須已存在。\n"
      "若該看板/使用者已刪除，請先重新開設/註冊一個同名的看板/使用者。\n"
      "還原備份時請確認該看板無人使用/使用者不在線上");

    if (ch == 0 || ch == 1)
    {
      if (!ask_board(brdname, BRD_L_BIT, NULL))
	return 0;
      sprintf(ptr, "/%s%s.tgz", ch == 0 ? "" : "brd/", brdname);
    }
    else /* if (ch == 2) */
    {
      if (acct_get(msg_uid, &acct) <= 0)
	return 0;
      type = acct.userid;
      str_lower(type, type);
      sprintf(ptr, "/%c/%s.tgz", *type, type);
    }

    if (!dashf(src))
    {
      /* 檔案不存在，通常是因為備份點時該看板/使用者已被刪除，或是當時根本就還沒有該看板/使用者 */
      vmsg("備份檔案不存在，請試試其他時間點的備份");
      return 0;
    }

    if (vans("還原備份後，目前所有資料都會流失，請務必確定(Y/N)？[N] ") != 'y')
      return 0;

    alog("還原備份", src);

    /* 解壓縮 */
    if (ch == 0)
      ptr = "brd";
    else if (ch == 1)
      ptr = "gem/brd";
    else /* if (ch == 2) */
      sprintf(ptr = date, "usr/%c", *type);
    sprintf(cmd, "tar xfz %s -C %s/", src, ptr);
    /* system(cmd); */

#if 1	/* 讓站長手動執行 */
    move(7, 0);
    outs("\n請以 bbs 身分登入工作站，並於\033[1;36m家目錄\033[m執行\n\n\033[1;33m");
    outs(cmd);
    outs("\033[m\n\n");
#endif

    /* tar 完以後，還要做的事 */
    if (vans("◎ 指令 Y)已成功\執行以上指令 Q)放棄執行：[Q] ") == 'y')
    {
      if (ch == 0)	/* 還原看板時，要更新板友 */
      {
	if ((ch = brd_bno(brdname)) >= 0)
	{
	  brd_fpath(src, brdname, fn_pal);
	  bpal = bshm->pcache + ch;
	  bpal->pal_max = image_pal(src, bpal->pal_spool);
	}
      }
      else if (ch == 2)	/* 還原使用者時，不還原 userno */
      {
	ch = acct.userno;
	if (acct_load(&acct, type) >= 0)
	{
	  acct.userno = ch;
	  acct_save(&acct);
	}
      }
      vmsg("還原備份成功\ ");
      return 0;
    }
  }

  vmsg(msg_cancel);
  return 0;
}


#ifdef HAVE_REGISTER_FORM

/* ----------------------------------------------------- */
/* 處理 Register Form					 */
/* ----------------------------------------------------- */
#ifdef USE_DAVY_REGISTER
int
a_register()
{
  int (*func) ();

  if(!a_check())
    return 0;

  /* Thor.990212: dynamic load , with negative umode */
  {
    void *p = DL_get("bin/regmgr.so:a_regmgr");
    if (!p)
    {
      p = dlerror();
      if (p)
        vmsg((char *)p);
      return 0;
    }
    func = p;
  }

  return (*func)();
}
#else
static void
log_register(rform, pass, reason, checkbox, szbox)
  RFORM *rform;
  int pass;
  char *reason[];
  int checkbox[];
  int szbox;
{
  char fpath[64], title[TTLEN + 1];
  FILE *fp;

  sprintf(fpath, "tmp/log_register.%s", cuser.userid);
  if(fp = fopen(fpath, "w"))
  {
    sprintf(title, "[紀錄] 站務 %s %s %s 的註冊單", cuser.userid, pass? "通過" : "退回", rform->userid);

    /* 文章檔頭 */
    fprintf(fp, "%s <精靈之眼> (Eye of Daemon)\n", str_author1);
    fprintf(fp, "標題: %s\n時間: %s\n\n", title, Now());

    /* 文章內容 */
    fprintf(fp, "申請時間: %s\n", Btime(&rform->rtime));
    fprintf(fp, "服務單位: %s\n", rform->career);
    fprintf(fp, "目前住址: %s\n", rform->address);
    fprintf(fp, "連絡電話: %s\n", rform->phone);

    if(!pass)	/* 記錄退回原因 */
    {
      int i;

      fprintf(fp, "\n退回原因：\n");
      for(i = 1; i < szbox; i++)
      {
	if(checkbox[i])
	  fprintf(fp, "    %s\n", reason[i]);
      }
      if(checkbox[0])
	fprintf(fp, "    %s\n", reason[0]);
    }

    fclose(fp);

    add_post(BN_REGLOG, fpath, title, "<精靈之眼>", "Eye of Daemon", POST_DONE);

    unlink(fpath);
  }
}


static void
regform_sendback(rform, uid)
  RFORM *rform;
  char *uid;
{
  char prompt[64];
  char folder[64], fpath[64];
  char *reason[] = {
    prompt, "請輸入真實姓名", "請詳填住址資料", "請詳填連絡電話", "請詳填服務單位(學校系級)", "請用中文填寫申請單",
  };
  int checkbox[sizeof(reason) / sizeof(reason[0])];
  int op, i, n;
  FILE *fp_note;
  HDR hdr;


  move(9, 0);
  clrtobot();
  outs("\n請提出退回註冊單原因：\n\n");
  n = sizeof(reason) / sizeof(char*);
  for(i = 1; i < n; i++)
    prints("    %d. %s\n", i, reason[i]);
  prints("    i. [自訂]↓\n");
  memset(checkbox, 0, sizeof(checkbox));

  while(1)
  {
    op = vans("依照編號切換選項(輸入s結束)：");

    i = op - '0';
    if(i >= 1 && i <= n)
    {
      checkbox[i] = !checkbox[i];

      move(11 + i, 0);
      outs(checkbox[i]? "ˇ" : "  ");
    }

    if(op == 'i')
    {
      if(checkbox[0])
      {
        move(11 + n, 0);
        outs("  ");
        move(12 + n, 0);
        clrtoeol();
      }
      else
      {
	if(!vget(b_lines, 0, "自訂：", prompt, sizeof(prompt), DOECHO))
	  continue;

	move(11 + n, 0);
	outs("ˇ");
	move(12 + n, 0);
	prints("      \"%s\"", prompt);
      }

      checkbox[0] = !checkbox[0];
    }

    if(op == 's')
    {
      int checksum = 0;
      for(i = 0; i < n; i++)
	checksum += checkbox[i];

      if(checksum > 0)
	break;
      else
	vmsg("請選擇退回原因");
    }
  }

  /* 寄退回信 */
  usr_fpath(folder, uid, fn_dir);
  if(fp_note = fdopen(hdr_stamp(folder, 0, &hdr, fpath), "w"))
  {
    fprintf(fp_note, "作者: %s (%s)\n", cuser.userid, cuser.username);
    fprintf(fp_note, "標題: [通知] 請重新填寫註冊單\n時間: %s\n\n", Now());

    fputs("退件原因：\n", fp_note);
    for(i = 1; i < n; i++)
    {
      if(checkbox[i])
	fprintf(fp_note, "    %s\n", reason[i]);
    }
    if(checkbox[0])
      fprintf(fp_note, "    %s\n", prompt);

    fputs("\n\n因為以上原因，使註冊單資料不齊全，\n", fp_note);
    fputs("依照\033[1m台灣學術網路BBS站管理使用公約\033[m，無法作為身分認證。\n", fp_note);
    fputs("\n請重新填寫註冊單。\n", fp_note);
    fputs("\n                                                    " BBSNAME "站務\n", fp_note);
    fclose(fp_note);

    strcpy(hdr.owner, cuser.userid);
    strcpy(hdr.title, "[通知] 請重新填寫註冊單");
    rec_add(folder, &hdr, sizeof(HDR));

    m_biff(rform->userno);
  }

  /* 記錄審核 */
  log_register(rform, 0, reason, checkbox, sizeof(reason) / sizeof(char*));
}


static int
scan_register_form(fd)
  int fd;
{
  ACCT acct;
  RFORM rform;
  HDR hdr;
  char buf[256], folder[64];
  int op;


  vs_bar("審核使用者註冊資料");

  while(read(fd, &rform, sizeof(RFORM)) == sizeof(RFORM))
  {
    move(2, 0);
    prints("申請代號: %s (申請時間：%s)\n", rform.userid, Btime(&rform.rtime));
    prints("服務單位: %s\n", rform.career);
    prints("目前住址: %s\n", rform.address);
    prints("連絡電話: %s\n", rform.phone);
    outs(msg_seperator);
    outc('\n');
    clrtobot();

    if(acct_load(&acct, rform.userid) < 0 || acct.userno != rform.userno)
    {
      vmsg("查無此人");
      op = 'd';
    }
    else
    {
      acct_show(&acct, 2);

#ifdef JUSTIFY_PERIODICAL
      if (acct.userlevel & PERM_VALID && acct.tvalid + VALID_PERIOD - INVALID_NOTICE_PERIOD >= acct.lastlogin)
#else
      if (acct.userlevel & PERM_VALID)
#endif
      {
	vmsg("此帳號已經完成註冊");
	op = 'd';
      }
      else if (acct.userlevel & PERM_ALLDENY)
      {
	/* itoc.050405: 不能讓停權者重新認證，因為會改掉他的 tvalid (停權到期時間) */
	vmsg("此帳號目前被停權中");
	op = 'd';
      }
      else
      {
	op = vans("是否接受(Y/N/Q/Del/Skip)？[S] ");
      }
    }

    switch (op)
    {
    case 'y':
      /* 記錄註冊資料 */
      sprintf(buf, "REG: %s:%s:%s:by %s", rform.phone, rform.career, rform.address, cuser.userid);
      justify_log(acct.userid, buf);

      /* 提升權限 */
      time(&acct.tvalid);
      /* itoc.041025: 這個 acct_setperm() 並沒有緊跟在 acct_load() 後面，中間隔了一個 vans()，
	這可能造成拿舊 acct 去覆蓋新 .ACCT 的問題。不過因為是站長才有的權限，所以就不改了 */
      acct_setperm(&acct, PERM_VALID, 0);

      /* 寄信通知使用者 */
      usr_fpath(folder, rform.userid, fn_dir);
      hdr_stamp(folder, HDR_LINK, &hdr, FN_ETC_JUSTIFIED);
      strcpy(hdr.title, msg_reg_valid);
      strcpy(hdr.owner, str_sysop);
      rec_add(folder, &hdr, sizeof(HDR));
      m_biff(rform.userno);

      /* 記錄審核 */
      log_register(&rform, 1, NULL, NULL, 0);

      break;

    case 'q':			/* 結束 */
      do
      {
	rec_add(FN_RUN_RFORM, &rform, sizeof(RFORM));
      } while(read(fd, &rform, sizeof(RFORM)) == sizeof(RFORM));

    case 'd':
      break;

    case 'n':
      regform_sendback(&rform, acct.userid);
      break;

    default:			/* put back to regfile */
      rec_add(FN_RUN_RFORM, &rform, sizeof(RFORM));
    }
  }
}


int
a_register()
{
  int num;
  char buf[80];

  if(!a_check())
    return 0;

  num = rec_num(FN_RUN_RFORM, sizeof(RFORM));
  if (num <= 0)
  {
    vmsg("目前並無新註冊資料");
    return XEASY;
  }

  sprintf(buf, "共有 %d 筆資料，開始審核嗎(Y/N)？[N] ", num);
  num = XEASY;

  if (vans(buf) == 'y')
  {
    sprintf(buf, "%s.tmp", FN_RUN_RFORM);
    if (dashf(buf))
    {
      vmsg("有站長也在審核註冊申請單中");
    }
    else
    {
      int fd;

      rename(FN_RUN_RFORM, buf);
      fd = open(buf, O_RDONLY);
      if (fd >= 0)
      {
	scan_register_form(fd);
	close(fd);
	unlink(buf);
	num = 0;
      }
      else
      {
	vmsg("無法開啟註冊資料工作檔！");
      }
    }
  }
  return num;
}


int
a_regmerge()			/* itoc.000516: 斷線時註冊單修復 */
{
  char fpath[64];
  FILE *fp;

  if(!a_check())
    return 0;

  sprintf(fpath, "%s.tmp", FN_RUN_RFORM);
  if (dashf(fpath))
  {
    vmsg("請先確定已無其他站長在審核註冊單，以免發生事故");

    if (vans("確定要啟動註冊單修復功\能(Y/N)？[N] ") == 'y')
    {
      if (fp = fopen(FN_RUN_RFORM, "a"))
      {
	f_suck(fp, fpath);
	fclose(fp);
	unlink(fpath);
      }
      vmsg("處理完畢");
    }
  }
  else
  {
    vmsg("目前並無修復註冊單之必要");
  }
  return XEASY;
}
#endif  /* USE_DAVY_REGSITER */
#endif	/* HAVE_REGISTER_FORM */


/* ----------------------------------------------------- */
/* 寄信給全站使用者/板主				 */
/* ----------------------------------------------------- */


static void
add_to_list(list, id)
  char *list;
  char *id;		/* 未必 end with '\0' */
{
  char *i;

  /* 先檢查先前的 list 裡面是否已經有了，以免重覆加入 */
  for (i = list; *i; i += IDLEN + 1)
  {
    if (!strncmp(i, id, IDLEN))
      return;
  }

  /* 若之前的 list 沒有，那麼直接附加在 list 最後 */
  str_ncpy(i, id, IDLEN + 1);
}


static void
make_bm_list(list)
  char *list;
{
  BRD *head, *tail;
  char *ptr, *str, buf[BMLEN + 1];

  /* 去 bshm 中抓出所有 brd->BM */

  head = bshm->bcache;
  tail = head + bshm->number;
  do				/* 至少有 note 一板，不必對看板做檢查 */
  {
    ptr = buf;
    strcpy(ptr, head->BM);

    while (*ptr)	/* 把 brd->BM 中 bm1/bm2/bm3/... 各個 bm 抓出來 */
    {
      if (str = strchr(ptr, '/'))
	*str = '\0';
      add_to_list(list, ptr);
      if (!str)
	break;
      ptr = str + 1;
    }      
  } while (++head < tail);
}


static void
make_all_list(list)
  char *list;
{
  int fd;
  SCHEMA schema;

  if ((fd = open(FN_SCHEMA, O_RDONLY)) < 0)
    return;

  while (read(fd, &schema, sizeof(SCHEMA)) == sizeof(SCHEMA))
    add_to_list(list, schema.userid);

  close(fd);
}


static void
send_list(title, fpath, list)
  char *title;		/* 信件的標題 */
  char *fpath;		/* 信件的檔案 */
  char *list;		/* 寄信的名單 */
{
  char folder[64], *ptr;
  HDR mhdr;

  for (ptr = list; *ptr; ptr += IDLEN + 1)
  {
    usr_fpath(folder, ptr, fn_dir);
    if (hdr_stamp(folder, HDR_LINK, &mhdr, fpath) >= 0)
    {
      strcpy(mhdr.owner, str_sysop);
      strcpy(mhdr.title, title);
      mhdr.xmode = 0;
      rec_add(folder, &mhdr, sizeof(HDR));
    }
  }
}


static void
biff_bm()
{
  UTMP *utmp, *uceil;

  utmp = ushm->uslot;
  uceil = (void *) utmp + ushm->offset;
  do
  {
    if (utmp->pid && (utmp->userlevel & PERM_BM))
      utmp->status |= STATUS_BIFF;
  } while (++utmp <= uceil);
}


static void
biff_all()
{
  UTMP *utmp, *uceil;

  utmp = ushm->uslot;
  uceil = (void *) utmp + ushm->offset;
  do
  {
    if (utmp->pid)
      utmp->status |= STATUS_BIFF;
  } while (++utmp <= uceil);
}


int
m_bm()
{
  char *list, fpath[64];
  FILE *fp;
  int size;

  if (vans("要寄信給全站所有板主(Y/N)？[N] ") != 'y')
    return XEASY;

  strcpy(ve_title, "[板主通告] ");
  if (!vget(1, 0, "標題：", ve_title, TTLEN + 1, GCARRY))
    return 0;

  usr_fpath(fpath, cuser.userid, "sysmail");
  if (fp = fopen(fpath, "w"))
  {
    fprintf(fp, "※ [板主通告] 站長通告，收信人：各板主\n");
    fprintf(fp, "-------------------------------------------------------------------------\n");
    fclose(fp);
  }

  curredit = EDIT_MAIL;
  *quote_file = '\0';
  if (vedit(fpath, 1) >= 0)
  {
    vmsg("需要一段蠻長的時間，請耐心等待");

    size = (IDLEN + 1) * MAXBOARD * 4;	/* 假設每板四個板主已足夠 */
    if (list = (char *) malloc(size))
    {
      memset(list, 0, size);

      make_bm_list(list);
      send_list(ve_title, fpath, list);

      free(list);
      biff_bm();
    }
  }
  else
  {
    vmsg(msg_cancel);
  }

  unlink(fpath);

  return 0;
}


int
m_all()
{
  char *list, fpath[64];
  FILE *fp;
  int size;

  if (vans("要寄信給全站使用者(Y/N)？[N] ") != 'y')
    return XEASY;    

  strcpy(ve_title, "[系統通告] ");
  if (!vget(1, 0, "標題：", ve_title, TTLEN + 1, GCARRY))
    return 0;

  usr_fpath(fpath, cuser.userid, "sysmail");
  if (fp = fopen(fpath, "w"))
  {
    fprintf(fp, "※ [系統通告] 站長通告，收信人：全站使用者\n");
    fprintf(fp, "-------------------------------------------------------------------------\n");
    fclose(fp);
  }

  curredit = EDIT_MAIL;
  *quote_file = '\0';
  if (vedit(fpath, 1) >= 0)
  {
    vmsg("需要一段蠻長的時間，請耐心等待");

    size = (IDLEN + 1) * rec_num(FN_SCHEMA, sizeof(SCHEMA));
    if (list = (char *) malloc(size))
    {
      memset(list, 0, size);

      make_all_list(list);
      send_list(ve_title, fpath, list);

      free(list);
      biff_all();
    }
  }
  else
  {
    vmsg(msg_cancel);
  }

  unlink(fpath);

  return 0;
}
