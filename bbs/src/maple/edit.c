/*-------------------------------------------------------*/
/* edit.c       ( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : simple ANSI/Chinese editor			 */
/* create : 95/03/29					 */
/* update : 10/11/08					 */
/*-------------------------------------------------------*/



#include "bbs.h"



int wordsnum;

#define	FN_BAK		"bak"


/* ----------------------------------------------------- */
/* 暫存檔 TBF (Temporary Buffer File) routines		 */
/* ----------------------------------------------------- */

char *
tbf_ask (void)
{
  static char fn_tbf[] = "buf.1";
  int ch, prev;

  do
  {
    prev = fn_tbf[4];
    ch = vget(b_lines, 0, "請選擇暫存檔(1-5, 或0取消)：", fn_tbf + 4, 2, GCARRY);

    if(ch == '0')
    {
      fn_tbf[4] = prev;
      return NULL;
    }

  } while (ch < '1' || ch > '5');

  return fn_tbf;
}


FILE *
tbf_open()
{
  int ans;
  char fpath[64], op[4];
  struct stat st;
  char *ftbf = tbf_ask();

  if(ftbf == NULL)
    return NULL;

  usr_fpath(fpath, cuser.userid, ftbf);
  ans = 'a';

  if (!stat(fpath, &st))
  {
    ans = vans("暫存檔已有資料 (A)附加 (W)覆寫 (Q)取消？[A] ");
    if (ans == 'q')
      return NULL;

    if (ans != 'w')
    {
      /* itoc.030208: 檢查暫存檔大小 */
      if (st.st_size > 8388608)	/* dust.100630: 調整上限至8MB */
      {
	vmsg("暫存檔太大，無法附加");
	return NULL;
      }

      ans = 'a';
    }
  }

  op[0] = ans;
  op[1] = '\0';

  return fopen(fpath, op);
}




#ifdef HAVE_ANONYMOUS
char anonymousid[IDLEN + 1];	/* itoc.010717: 自定匿名 ID */
#endif


void
ve_header(FILE *fp)
{
  time_t now;
  char *title;

  title = ve_title;
  title[72] = '\0';
  time(&now);

  if (curredit & EDIT_MAIL)
  {
    fprintf(fp, "%s %s (%s)\n", str_author1, cuser.userid, cuser.username);
  }
  else
  {
#ifdef HAVE_ANONYMOUS
    if (currbattr & BRD_ANONYMOUS && !(curredit & EDIT_RESTRICT))
    {
      if (!vget(b_lines, 0, "請輸入您想用的ID，也可直接按[Enter]，或是按[r]用真名：", anonymousid, IDLEN, DOECHO))
      {											/* 留 1 byte 加 "." */
	strcpy(anonymousid, STR_ANONYMOUS);
	curredit |= EDIT_ANONYMOUS;
      }
      else if (strcmp(anonymousid, "r"))
      {
	strcat(anonymousid, ".");		/* 若是自定ID，要在後面加個 . 表示不同 */
	curredit |= EDIT_ANONYMOUS;
      }
    }
#endif

    if (!(currbattr & BRD_NOSTAT) && !(curredit & EDIT_RESTRICT))	/* 不計文章篇數 及 加密存檔 不列入統計篇數 */
    {
      /* 產生統計資料 */

      POSTLOG postlog;

#ifdef HAVE_ANONYMOUS
      /* Thor.980909: anonymous post mode */
      if (curredit & EDIT_ANONYMOUS)
	strcpy(postlog.author, anonymousid);
      else
#endif
	strcpy(postlog.author, cuser.userid);

      strcpy(postlog.board, currboard);
      str_ncpy(postlog.title, str_ttl(title), sizeof(postlog.title));
      postlog.date = now;
      postlog.number = 1;

      rec_add(FN_RUN_POST, &postlog, sizeof(POSTLOG));
    }

#ifdef HAVE_ANONYMOUS
    /* Thor.980909: anonymous post mode */
    if (curredit & EDIT_ANONYMOUS)
    {
      fprintf(fp, "%s %s () %s %s\n",
	str_author1, anonymousid,
	curredit & EDIT_OUTGO ? str_post1 : str_post2, currboard);
    }
    else
#endif
    {
      fprintf(fp, "%s %s (%s) %s %s\n",
	str_author1, cuser.userid, cuser.username,
	curredit & EDIT_OUTGO ? str_post1 : str_post2, currboard);
    }
  }

  fprintf(fp, "標題: %s\n時間: %s\n\n", title, Btime(&now));
}


int 
ve_subject (int row, char *topic, char *dft)
{
  char *title;

  title = ve_title;

  if (topic)
  {
    sprintf(title, "Re: %s", str_ttl(topic));
    title[TTLEN] = '\0';
  }
  else
  {
    if (dft)
      strcpy(title, dft);
    else
      *title = '\0';
  }

  return vget(row, 0, "標題：", title, TTLEN + 1, GCARRY);
}



static void 
ve_fromhost (char *buf)
{
  unsigned long ip32;
  int i;
  uschar ipv4[4];
  char fromname[48], ipaddr[32], name[48];

  ip32 = cutmp->in_addr;	/* dust.091002: in_addr的使用由 hrs113355 提供 <(_ _)> */
  for(i = 0; i < 4; i++)
  {
    ipv4[i] = ip32 & 0xFF;
    ip32 >>= 8;
  }
  sprintf(ipaddr, "%d.%d.%d.%d", ipv4[3], ipv4[2], ipv4[1], ipv4[0]);

  /* 先用 FQDN 做比對 */
  str_lower(name, fromhost);
  if (!belong_list(FN_ETC_FQDN, name, fromname, sizeof(fromname)))
  {
    if(!belong_list(FN_ETC_HOST, name, fromname, sizeof(fromname)))
    {
      /* 再用 IP 做比對 */
      if (!belong_list(FN_ETC_HOST, ipaddr, fromname, sizeof(fromname)))
	strcpy(fromname, "");		/* 還是沒有的話就填空白 */
    }
  }

  /* dust: 防止站簽爆行的特別對應 */
  if(fromname[0] == '\0')	/* 沒有對應的故鄉 */
  {
    if(strlen(fromhost) <= 47)
      strcpy(buf, fromhost);
    else
      strcpy(buf, ipaddr);
  }
  else
  {
    if(strlen(fromhost) <= 44 - strlen(fromname))
      sprintf(buf, "%s (%s)", fromhost, fromname);
    else
      sprintf(buf, "%s (%s)", ipaddr, fromname);
  }
}



char *ebs[EDIT_BANNER_NUM] = EDIT_BANNERS;	/* 放在全域中跟vote.c共用 */

void
ve_banner(		/* 加上來源等訊息 */
  FILE *fp,
  int modify			/* 1:修改 0:非發文 */
)
{
  int i = time(NULL) % EDIT_BANNER_NUM;
  char buf[80];
  char *ebs[EDIT_BANNER_NUM] = EDIT_BANNERS;

  if (!modify)
  {
    ve_fromhost(buf);
    fprintf(fp, ebs[i],
#ifdef HAVE_ANONYMOUS
      (curredit & EDIT_ANONYMOUS) ? STR_ANONYMOUS :
#endif
      cuser.userid,
#ifdef HAVE_ANONYMOUS
      (curredit & EDIT_ANONYMOUS) ? STR_ANONYFROM :
#endif
      buf);

  }
  else
  {
    fprintf(fp, MODIFY_BANNER, cuser.userid, Now());
  }
}



/* ----------------------------------------------------- */
/* 編輯器自動備份					 */
/* ----------------------------------------------------- */

void 
ve_recover (void)
{
  char fpbak[80], fpath[80];
  int ch;

  usr_fpath(fpbak, cuser.userid, FN_BAK);
  if (dashf(fpbak))
  {
    ch = vans("您有一篇文章尚未完成，(M)寄到信箱 (S)寫入暫存檔 (Q)算了？[M] ");
    if (ch == 's')
    {
      char *ftbf = tbf_ask();

      if(ftbf == NULL)
	return;
      usr_fpath(fpath, cuser.userid, ftbf);
      rename(fpbak, fpath);
      return;
    }
    else if (ch != 'q')
    {
      mail_self(fpbak, cuser.userid, "未完成的文章", 0);
    }
    unlink(fpbak);
  }
}



/* ----------------------------------------------------- */
/* 顯示簽名檔						 */
/* ----------------------------------------------------- */

static void 
show_sign (int cur_page)
{
  int len, sig_no, lim;
  char fpath[64];

  clear();

  usr_fpath(fpath, cuser.userid, FN_SIGN". ");
  len = strlen(fpath) - 1;

  sig_no = cur_page * 3;
  lim = cuser.sig_avai;
  if(lim > sig_no + 2)
    lim = sig_no + 2;

  for(; sig_no <= lim; sig_no++)        /* dust.100331: 使用者自訂可用簽名檔數量 */
  {
    fpath[len] = sig_no + '1';

    move((sig_no % 3) * (MAXSIGLINES + 1) + 1, 0);
    if(lim_cat(fpath, MAXSIGLINES) >= 0)
    {
      move((sig_no % 3) * (MAXSIGLINES + 1), 0);
      prints("\033[1;36m【 簽名檔 %c 】\033[m", sig_no + '1');
    }
    else
    {
      move((sig_no % 3) * (MAXSIGLINES + 1), 0);
      prints("\033[1;30m【 簽名檔 %c 】\033[m", sig_no + '1');
    }
  }
}


int 
select_sign (void)
{
  static char msg[] = "請選擇簽名檔 (1~6, 0=不加 r=亂數 n=換頁) [0] ";
  int ans, cur_sig, sig_page;
  int sig_avail = cuser.sig_avai;
  int cur_page = 0;

#ifdef HAVE_ANONYMOUS
  /* 在匿名板中無論是否匿名，均不使用簽名檔 */
  if((currbattr & BRD_ANONYMOUS) || (cuser.ufo & UFO_NOSIGN))
#else
  if(cuser.ufo & UFO_NOSIGN)
#endif
    return '0';

  msg[16] = sig_avail + '0';
  sig_page = (sig_avail + 2) / 3;

  cur_sig = cuser.signature;
  if(cur_sig > 0 && cur_sig <= 9)
  {
    if(cur_sig > sig_avail)
    {
      /* dust.100331: 前次選擇若超過現在的可用數量就改成隨機 */
      cur_sig = cuser.signature = 'r' - '0';
    }
  }

  cur_sig += '0';

  while(1)
  {
    if(cuser.ufo & UFO_SHOWSIGN)
      show_sign(cur_page);

    msg[42] = cur_sig;
    ans = vans(msg);

    if(ans == 'n')
    {
      cur_page = (cur_page + 1) % sig_page;
      continue;
    }

    if(ans != cur_sig && ((ans >= '0' && ans <= sig_avail + '0') || ans == 'r'))
      cur_sig = ans;

    break;
  }

  if(!(cuser.ufo2 & UFO2_SNRON) || cur_sig != '0')
    cuser.signature = cur_sig - '0';

  if(cur_sig == 'r')
    cur_sig = (rand() % sig_avail) + '1';

  return cur_sig;
}




/* --------------------------------------------------------------------- */
/* vedit 回傳 -1:取消編輯, 0:完成編輯					 */
/* --------------------------------------------------------------------- */
/* ve_op:								 */
/*  0 => 純粹編輯檔案							 */
/* -1 => 不能儲存，用在編輯作者不是自己的文章時				 */
/*  1 => 有引文與簽名檔，並加上檔頭。用在發表文章與寄站內信		 */
/*  2 => 有引文與簽名檔，不加上檔頭。用在寄站外信			 */
/*  0~9 且 curredit & EDIT_TEMPLATE => 使用文章樣板，載入後會被換成 1	 */
/* --------------------------------------------------------------------- */
/* ve_op 是 1, 2, 時，使用前還得指定 curredit 和 quote_file		 */
/* --------------------------------------------------------------------- */

int 
vedit (char *fpath, int ve_op)
{
  return dledit(fpath, ve_op);
}

