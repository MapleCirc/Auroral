/*-------------------------------------------------------*/
/* xyz.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 雜七雜八的外掛				 */
/* create : 01/03/01					 */
/* update : 09/03/20					 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef SP_SEND_MONEY

/* ----------------------------------------------------- */
/* '08年極光特典 - 站長灑錢功能				 */
/* ----------------------------------------------------- */

/* --------------------------------------------- */
/* Link List routines				 */
/* --------------------------------------------- */

struct NOD{
  char id[16];
  int money,gold;
  struct NOD* next;
};

#define LinkList	struct NOD

LinkList *mohead = NULL;	/* head of link list */
LinkList *motail = NULL;	/* tail of link list */

static void
mo_free()
{
  LinkList *next;
  
  while (mohead)
  {
    next = mohead->next;
    free(mohead);
    mohead = next;
  }
  
  mohead = motail = NULL;
}

static LinkList*
mo_has(name)		/* check sth. if it exits in list */
  char *name;
{
  LinkList *list;

  for (list = mohead; list; list = list->next)
  {
    if (!strcmp(list->id, name))
      return list;
  }
  return NULL;
}

static void
mo_add(id, money, gold)		/* add node */
  char *id;
  int money;
  int gold;
{
  if (mohead)
  {
    motail->next = (LinkList*) malloc(sizeof(struct NOD));
    motail = motail->next;
    
    motail->next = NULL;
    strcpy(motail->id, id);
    motail->money = money;
    motail->gold = gold;
  }
  else
  {
    mohead = (LinkList*) malloc(sizeof(struct NOD));
    
    mohead->next = NULL;
    strcpy(mohead->id, id);
    mohead->money = money;
    mohead->gold = gold;
    
    motail = mohead;
  }
}

static int
mo_del(name)		/* delete node (成功回傳1,失敗回傳0) */
  char *name;
{
  LinkList *list, *prev, *next;

  prev = NULL;
  for (list = mohead; list; list = next)
  {
    next = list->next;
    if (!strcmp(list->id, name))
    {
      if (prev == NULL)
	mohead = next;
      else
	prev->next = next;

      if (list == motail)
	motail = prev;

      free(list);
      return 1;
    }
    prev = list;
  }
  return 0;
}

static void
mo_out(row, column, msg)
/* row, col代表繪圖起始位置，msg為一開始顯示的訊息 */
  int row, column;
  char *msg;
{
  int len;
  LinkList *list;
  char buf[64];

  move(row, column);
  outs(msg);

  column = 80;		/* 為了強制換行用...汗 */
  for (list = mohead; list; list = list->next)
  {
    sprintf(buf, "%s-%d+%d", list->id, list->money, list->gold);
    len = strlen(buf) + 1;
    if (column + len > 78)
    {
      if (++row > b_lines - 2)
      {
	move(b_lines, 60);
	outs("\033[1m(下略...)\033[m");
	break;
      }
      else
      {
	column = len;
	outc('\n');
      }
    }
    else
    {
      column += len;
      outc(' ');
    }
    outs(buf);
  }
}

/* -------- End of Linked List routines -------- */


/* 幫這個功能寫的vans，能指定位置 */
static int
setans(row, col, msg)
  int row;
  int col;
  char *msg;
{
  char ans[2];
  return tolower(vget(row, col, msg, ans, 2, DOECHO));
}


/* 主程式(版本: Ver 1.0 release) */
int
year_bonus()
{
  char folder[64], fpath[64], passbuf[16];
  char reason[64], id[16], buf[128], ans[4], *str;
  LinkList *now;
  int modes;
/* 1:每人金額設為不同  2:每人金額設為相同  3:對全站人發錢 */
  int money, gold;
  HDR hdr;
  FILE *fp;
  DIR *dire;
  struct dirent *de;
  PAYCHECK paycheck;
  time_t nowtime;
  char *menu[] = {"a增加", "d刪除", "s完成", "e修改", "q取消", NULL};
  char *main_menu[] = {"1q", "1每人金額設為不同", "2每人金額設為相同", "3對全站人發錢", "Q離開", NULL};

  if(!strcmp(cuser.userid, str_sysop))
  {
    vmsg("不能用sysop發錢");
    return 0;
  }

  /* debug function(check if head is null) */
  modes = 0;		/* modes借來作為除錯用 */
  if(mohead)
  {
    vmsg("錯誤！串列首不是NULL");
    modes = 1;
  }
  if(motail)
  {
    vmsg("錯誤！串列尾不是NULL");
    modes = 1;
  }

  if(modes)
    return 0;
  /* End of debug */


  /* 身分驗證程序 */
  if(!vget(b_lines, 0, "輸入密碼以繼續執行：", passbuf, PSWDLEN + 1, NOECHO))
    return 0;

  if(chkpasswd(cuser.passwd, passbuf))
  {
    vmsg(ERR_PASSWD);
    return 0;
  }

  move(0, 0);
  clrtobot();

  /* draw header */
  outs("\033[1;33;45m ★\033[37m 站長發錢功\能 Ver 1.0(release) \033[m");

  ans[0] = pans(5, 20, "站長發錢功\能", main_menu);	/* 借ans[]來用 */
  switch(ans[0])
  {
  case 'q':
    return 0;

  default:
    modes = ans[0]-'0';
  }

  if(modes==2 || modes==3)
  {
    do
    {
      if (!vget(4, 0, "發給每人銀幣[0]：", buf, 9, DOECHO))
        strcpy(buf, "0");
      money = atoi(buf);
      if (money < 0) money = 0;

      buf[0] = '\0';

      if (!vget(4, 28, "金幣[0]：", buf, 9, DOECHO))
        strcpy(buf, "0");
      gold = atoi(buf);
      if (gold < 0) gold = 0;

      if (money==0 && gold==0) continue;

    }while(setans(4, 55, "確認？(y/N) ")!='y');
  }

  /* 輸入發錢的理由 */
  do
  {
    if(!vget(4, 0, "發錢理由(不輸入則取消執行)：", reason, 40, DOECHO))
      return 0;

    ans[0] = setans(5, 0, "確認。(y/N) ");

    if (ans[0] == 'q')
      return 0;

  }while(ans[0] != 'y');

  move(5, 0);
  clrtoeol();

  /* 發錢給全站 */
  if(modes == 3)
  {
    move(8, 0);
    prints("現在即將發給全站使用者 \033[1m%d \033[m銀幣 與 \033[1m%d \033[m金幣，", money, gold);

    if(setans(10, 4, "請確認。(Y/N) [N] ")!='y')
    {
      vmsg("取消動作");
      return 0;
    }

    /* 發錢者紀錄程序 */ 
    time(&nowtime);
    sprintf(buf, "站長發錢程式紀錄(由 %s 使用)\n時間：%s\n", cuser.userid, Btime(&nowtime));
    f_cat(FN_RUN_BONUS_LOG, buf);
    sprintf(buf, "原因：%s\n\n名單：發給 全站使用者 計[%d 銀幣 + %d 金幣]", reason, money, gold);
    f_cat(FN_RUN_BONUS_LOG, buf);
    f_cat(FN_RUN_BONUS_LOG, "\n-----end of this turn-----\n\n");

    /* 發支票、寄通知信的處理程序 */
    strcpy(folder, "usr/*");
    for(folder[4]='a';folder[4]<='z';folder[4]++)
    {
      dire = opendir(folder);

      if(dire)
      {
        while(de = readdir(dire))
        {
          str = de->d_name;

          /* 使用者存在或者不是sysop、guest的情況下才需要送錢 */
          if(acct_userno(str)<=0 || !strcmp(str, STR_GUEST) || !strcmp(str, str_sysop))
            continue;

          /* 通知信 */
          usr_fpath(fpath, str, fn_dir);
          if (fp = fdopen(hdr_stamp(fpath, 0, &hdr, buf), "w"))
          {
            fprintf(fp, "作者: sysop (%s)\n", SYSOPNICK);
            fprintf(fp, "標題: 站方發錢通知\n");
            fprintf(fp, "時間: %s\n\n", Btime(&nowtime));

            fprintf(fp, "原由：%s\n\n", reason);
            fprintf(fp, "請移駕至 Xyz -> Market -> 銀行 將支票兌現。\n\n"
                        "                                  極光鯨藍站長 (ps:本信不會自動銷毀)\n");
            fclose(fp);

            strcpy(hdr.title, "站方發錢通知");
            strcpy(hdr.owner, "sysop");
            rec_add(fpath, &hdr, sizeof(HDR));
          }

          /* 發送支票 */
          memset(&paycheck, 0, sizeof(PAYCHECK));
          time(&paycheck.tissue);
          paycheck.money = now -> money;
          paycheck.gold = now -> gold;
          strcpy(paycheck.reason, "[站方發錢]");
          usr_fpath(fpath, str, FN_PAYCHECK);
          rec_add(fpath, &paycheck, sizeof(PAYCHECK));
        }

        closedir(dire);
      }
    }

    vmsg("發送完成");
    return 0;
  }



/* ----------------------------------------------------- */
/*							 */
/*  Caution : 下面的部份在 return 前一定要進行 mo_free() */
/*							 */
/* ----------------------------------------------------- */



  do		/* command line process  */
  {
    move(8, 0);
    clrtobot();
    mo_out(11, 0, "\033[1;33m[名單-銀幣+金幣]\033[m");

    ans[0] = krans(6, "aq", menu, "sysop>");

    switch(ans[0]){
      case 'd':
	if(!vget(8, 0, "想要刪除的ID：", id, IDLEN + 1, DOECHO))
	  break;

	mo_del(id);
	break;

      case 'e':
	if(modes == 2)
	  break;
	if(!vget(8, 0, "想要修改的ID：", id, IDLEN + 1, DOECHO))
	  break;
	if((now = mo_has(id)) == NULL)
	{
	  move(6, 0);
	  clrtoeol();
	  outs("找不到這個ID");
	  break;
	}

	if(modes == 1)
        {
          if(!vget(9, 0, "發出銀幣[0] ", buf, 9, DOECHO))
            strcpy(buf, "0");
          money = atoi(buf);
          if(money < 0) money = 0;

          if(!vget(9, 21, " +金幣[0] ", buf, 9, DOECHO))
            strcpy(buf, "0");
          gold = atoi(buf);
          if(gold < 0) gold = 0;

          if(money ==0 && gold ==0)
            break;
        }
	now->money = money;
	now->gold = gold;
	break;

      case 's':
        if (mohead == NULL)
	{
	  mo_free();
	  return 0;
	}else
	  break;

      case 'q':
	mo_free();
	vmsg("取消動作");
	return 0;

      /* case 'a': */
      default:
        if(!vget(8, 0, "ID：", id, IDLEN + 1, DOECHO))
          break;

        if(acct_userno(id) <= 0 || !strcmp(id, STR_GUEST))
          break;

        if(mo_has(id) != NULL)
          break;

        if(modes == 1)
        {
          if(!vget(9, 0, "發出銀幣[0] ", buf, 9, DOECHO))
            strcpy(buf, "0");
          money = atoi(buf);
          if(money < 0) money = 0;

          if(!vget(9, 21, " +金幣[0] ", buf, 9, DOECHO))
            strcpy(buf, "0");
          gold = atoi(buf);
          if(gold < 0) gold = 0;

          if(money ==0 && gold ==0)
            break;
        }

        if(setans((modes == 1)? 9:8, 45, "確認。(Y/n) ") == 'n')
          break;

        mo_add(id, money, gold);

        break;
    }
    /* End of key switch */

  }while(ans[0]!='s');


  time(&nowtime);


  /* 發錢者記錄程序 */
  sprintf(buf, "站長發錢程式紀錄(由 %s 使用)\n時間：%s\n", cuser.userid, Btime(&nowtime));
  f_cat(FN_RUN_BONUS_LOG, buf);
  sprintf(buf, "原因：%s\n\n名單：\n", reason);
  f_cat(FN_RUN_BONUS_LOG, buf);

  /* 發錢處理迴圈 */
  for(now = mohead; now ; now = now->next)
  {
    /* 通知信 */
    usr_fpath(folder, now->id, fn_dir);
    if (fp = fdopen(hdr_stamp(folder, 0, &hdr, fpath), "w"))
    {
      fprintf(fp, "作者: sysop (%s)\n", SYSOPNICK);
      fprintf(fp, "標題: 站方發錢通知\n");
      fprintf(fp, "時間: %s\n\n", Btime(&nowtime));

      fprintf(fp, "原由：%s\n\n", reason);
      fprintf(fp, "請移駕至 Xyz -> Market -> 銀行 將支票兌現。\n\n"
                  "                                  極光鯨藍站長 (ps:本信不會自動銷毀)\n");

      fclose(fp);

      strcpy(hdr.title, "站方發錢通知");
      strcpy(hdr.owner, "sysop");
      rec_add(folder, &hdr, sizeof(HDR));
    }

    /* 發送支票 */
    memset(&paycheck, 0, sizeof(PAYCHECK));
    time(&paycheck.tissue);
    paycheck.money = now -> money;
    paycheck.gold = now -> gold;
    strcpy(paycheck.reason, "[站方發錢]");
    usr_fpath(fpath, now->id, FN_PAYCHECK);
    rec_add(fpath, &paycheck, sizeof(PAYCHECK));

    sprintf(buf, "發給 %-13s 計 [%d 銀 + %d 金]\n", now->id, now->money, now->gold);
    f_cat(FN_RUN_BONUS_LOG, buf);

  }
  f_cat(FN_RUN_BONUS_LOG, "\n-----end of this turn-----\n\n");


  mo_free();

  vmsg("發送完成");
  return 0;
}

#undef LinkList			/* 把該識別字還給別人 */

#endif



#ifdef HAVE_TIP

/* ----------------------------------------------------- */
/* 每日小秘訣						 */
/* ----------------------------------------------------- */

int
x_tip()
{
  int i, j;
  char msg[128];
  FILE *fp;

  if (!(fp = fopen(FN_ETC_TIP, "r")))
    return XEASY;

  fgets(msg, 128, fp);
  j = atoi(msg);		/* 第一行記錄總篇數 */
  i = time(0) % j + 1;
  j = 0;

  while (j < i)			/* 取第 i 個 tip */
  {
    fgets(msg, 128, fp);
    if (msg[0] == '#')
      j++;
  }

  move(12, 0);
  clrtobot();
  fgets(msg, 128, fp);
  prints("\033[1;36m每日小祕訣：\033[m\n");
  prints("            %s", msg);
  fgets(msg, 128, fp);
  prints("            %s", msg);
  vmsg(NULL);
  fclose(fp);
  return 0;
}
#endif	/* HAVE_TIP */


#ifdef HAVE_LOVELETTER 

/* ----------------------------------------------------- */
/* 情書產生器						 */
/* ----------------------------------------------------- */

int
x_loveletter()
{
  FILE *fp;
  int start_show;	/* 1:開始秀 */
  int style;		/* 0:開頭 1:正文 2:結尾 */
  int line;
  char buf[128];
  char header[3][5] = {"head", "body", "foot"};	/* 開頭、正文、結尾 */
  int num[3];

  /* etc/loveletter 前段是#head 中段是#body 後段是#foot */
  /* 行數上限：#head五行  #body八行  #foot五行 */

  if (!(fp = fopen(FN_ETC_LOVELETTER, "r")))
    return XEASY;

  /* 前三行記錄篇數 */
  fgets(buf, 128, fp);
  num[0] = atoi(buf + 5);
  num[1] = atoi(buf + 5);
  num[2] = atoi(buf + 5);

  /* 決定要選第幾篇 */
  line = time(0);
  num[0] = line % num[0];
  num[1] = (line >> 1) % num[1];
  num[2] = (line >> 2) % num[2];

  vs_bar("情書產生器");

  start_show = style = line = 0;

  while (fgets(buf, 128, fp))
  {
    if (*buf == '#')
    {
      if (!strncmp(buf + 1, header[style], 4))  /* header[] 長度都是 5 bytes */
	num[style]--;

      if (num[style] < 0)	/* 已經 fget 到要選的這篇了 */
      {
	outc('\n');
	start_show = 1;
	style++;
      }
      else
      {
	start_show = 0;
      }
      continue;
    }

    if (start_show)
    {
      if (line >= (b_lines - 5))	/* 超過螢幕大小了 */
	break;

      outs(buf);
      line++;
    }
  }

  fclose(fp);
  vmsg(NULL);

  return 0;
}
#endif	/* HAVE_LOVELETTER */


/* ----------------------------------------------------- */
/* 密碼忘記，重設密碼					 */
/* ----------------------------------------------------- */


int
x_password()
{
  int i;
  ACCT acct;
  FILE *fp;
  char fpath[80], email[60], passwd[PSWDLEN + 1];
  time_t now;

  vmsg("當其他使用者忘記密碼時，重送新密碼至該使用者的信箱");

  if (acct_get(msg_uid, &acct) > 0)
  {
    time(&now);

    if (acct.lastlogin > now - 86400 * 10)
    {
      vmsg("該使用者必須十天以上未上站方可重送密碼");
      return 0;
    }

    vget(b_lines - 2, 0, "請輸入認證時的 Email：", email, 40, DOECHO);

    if (str_cmp(acct.email, email))
    {
      vmsg("這不是該使用者認證時用的 Email");
      return 0;
    }

    if (not_addr(email) || !mail_external(email))
    {
      vmsg(err_email);
      return 0;
    }

    vget(b_lines - 1, 0, "請輸入真實姓名：", fpath, RNLEN + 1, DOECHO);
    if (strcmp(acct.realname, fpath))
    {
      vmsg("這不是該使用者的真實姓名");
      return 0;
    }

    if (vans("資料正確，請確認是否產生新密碼(Y/N)？[N] ") != 'y')
      return 0;

    sprintf(fpath, "%s 改了 %s 的密碼", cuser.userid, acct.userid);
    blog("PASSWD", fpath);

    /* 亂數產生 A~Z 組合的密碼八碼 */
    for (i = 0; i < PSWDLEN; i++)
      passwd[i] = rnd(26) + 'A';
    passwd[PSWDLEN] = '\0';

    /* 重新 acct_load 載入一次，避免對方在 vans() 時登入會有洗錢的效果 */
    if (acct_load(&acct, acct.userid) >= 0)
    {
      str_ncpy(acct.passwd, genpasswd(passwd), PASSLEN + 1);
      acct_save(&acct);
    }

    sprintf(fpath, "tmp/sendpass.%s", cuser.userid);
    if (fp = fopen(fpath, "w"))
    {
      fprintf(fp, "%s 為您申請了新密碼\n\n", cuser.userid);
      fprintf(fp, BBSNAME "ID : %s\n\n", acct.userid);
      fprintf(fp, BBSNAME "新密碼 : %s\n", passwd);
      fclose(fp);

      bsmtp(fpath, BBSNAME "新密碼通知信", email, MQ_NOATTACH);
    }
  }

  return 0;
}


int
pushdoll_maker()	/* dust.100401: 推娃格式生成機 */
{
#define PUSH_LEN 50
  char buf[80];
  int i;
  FILE *fp;

  if (!(fp = tbf_open()))
  {
    vmsg("暫存檔開啟失敗");
    return XEASY;
  }

  clear();
  for(i = 0; i <= b_lines; i++)
  {
    sprintf(buf, "%2d) ", i + 1);
    if(!vget(i, 0, buf, buf + 9, PUSH_LEN, DOECHO))
      break;

    fprintf(fp, "%%%d\n%s\n", !i, buf + 9);
  }
  fclose(fp);

  return 0;
}



/* dust:站務專用的神秘測試功能 */
/* xatier: 現在不是有偽極光了XD */
/* dust.reply: 這是在你入學以前就存在著的設計了... */
int
admin_test()
{
#if 1
  vmsg("唔喔馬布希");
  return XEASY;
#else

#endif
}

