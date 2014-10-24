/*-------------------------------------------------------*/
/* acct.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : account / administration routines	 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#define	_ADMIN_C_


#include "bbs.h"

extern BCACHE *bshm;


/* ----------------------------------------------------- */
/* (.ACCT) 使用者帳號 (account) subroutines		 */
/* ----------------------------------------------------- */


int
acct_load(acct, userid)
  ACCT *acct;
  char *userid;
{
  int fd;

  usr_fpath((char *) acct, userid, fn_acct);
  fd = open((char *) acct, O_RDONLY);
  if (fd >= 0)
  {
    /* Thor.990416: 特別注意, 有時 .ACCT的長度會是0 */
    read(fd, acct, sizeof(ACCT));
    close(fd);
  }
  return fd;
}


/* static */	/* itoc.010408: 給其他程式用 */
void
acct_save(acct)
  ACCT *acct;
{
  int fd;
  char fpath[64];

  /* itoc.010811: 若被站長鎖定，就不能寫回自己的檔案 */
  if ((acct->userno == cuser.userno) && HAS_STATUS(STATUS_DATALOCK) && !HAS_PERM(PERM_ALLACCT))
    return;

  usr_fpath(fpath, acct->userid, fn_acct);
  fd = open(fpath, O_WRONLY, 0600);	/* fpath 必須已經存在 */
  if (fd >= 0)
  {
    write(fd, acct, sizeof(ACCT));
    close(fd);
  }
}


int
acct_userno(userid)
  char *userid;
{
  int fd;
  int userno;
  char fpath[64];

  usr_fpath(fpath, userid, fn_acct);
  fd = open(fpath, O_RDONLY);
  if (fd >= 0)
  {
    read(fd, &userno, sizeof(userno));
    close(fd);
    return userno;
  }
  return 0;
}


/* ----------------------------------------------------- */
/* name complete for user ID				 */
/* ----------------------------------------------------- */
/* return value :					 */
/* 0 : 使用直接按 enter ==> cancel			 */
/* -1 : bad user id					 */
/* ow.: 傳回該 userid 之 userno				 */
/* ----------------------------------------------------- */


int
acct_get(msg, acct)
  char *msg;
  ACCT *acct;
{
  outz("★ 輸入首字母後，可以按空白鍵自動搜尋");
  
  if (!vget(1, 0, msg, acct->userid, IDLEN + 1, GET_USER))
    return 0;

  if (acct_load(acct, acct->userid) >= 0)
    return acct->userno;

  vmsg(err_uid);
  return -1;
}


/* ----------------------------------------------------- */
/* bit-wise display and setup				 */
/* ----------------------------------------------------- */


#define BIT_ON		"ˇ"
#define BIT_OFF		"　"

static void
print_bits(str, level)
  char *str;
  usint level;
{
  while(*str)
  {
    outc((level & 1) ? *str : '-');
    level >>= 1;
    str++;
  }
}


static void
bitmsg(msg, str, level)
  char *msg, *str;
  int level;
{
  outs(msg);
  print_bits(str, level);
  outc('\n');
}


usint
bitset(pbits, count, maxon, msg, perms)
  usint pbits;
  int count;			/* 共有幾個選項 */
  int maxon;			/* 最多可以 enable 幾項 */
  char *msg;
  char *perms[];
{
  int i, j, on;

  move(1, 0);
  clrtobot();
  move(3, 0);
  outs(msg);

  for (i = on = 0, j = 1; i < count; i++)
  {
    msg = BIT_OFF;
    if (pbits & j)
    {
      on++;
      msg = BIT_ON;
    }
    move(5 + (i & 15), (i < 16 ? 0 : 40));
    prints("%c %s %s", radix32[i], msg, perms[i]);
    j <<= 1;
  }

  while (i = vans("請按鍵切換設定，或按 [Return] 結束："))
  {
    i -= '0';
    if (i >= 10)
      i -= 'a' - '0' - 10;

    if (i >= 0 && i < count)
    {
      j = 1 << i;
      if (pbits & j)
      {
	on--;
	msg = BIT_OFF;
      }
      else
      {
	if (on >= maxon)
	  continue;
	on++;
	msg = BIT_ON;
      }

      pbits ^= j;
      move(5 + (i & 15), (i < 16 ? 2 : 42));
      outs(msg);
    }
  }
  return (pbits);
}


static usint
setperm(level)
  usint level;
{
  if (HAS_PERM(PERM_SYSOP))
    return bitset(level, NUMPERMS, NUMPERMS, MSG_USERPERM, perm_tbl);

  /* [帳號管理員] 不能管 PERM_SYSOP */
  if (level & PERM_SYSOP)
    return level;

  /* [帳號管理員] 不能更改權限 PERM_ACCOUNTS CHATROOM BOARD SYSOP */
  return bitset(level, NUMPERMS - 4, NUMPERMS - 4, MSG_USERPERM, perm_tbl);
}


/* ----------------------------------------------------- */
/* 帳號管理						 */
/* ----------------------------------------------------- */


static void
bm_list(userid)		/* 顯示 userid 是哪些板的板主 */
  char *userid;
{
  int i, num, uidlen, x, y, sp;
  BRD *bcache;

  bcache = bshm->bcache;
  num = bshm->number;
  uidlen = strlen(userid);

  getyx(&y, &x);
  sp = x;
  for(i = 0; i < num; i++)
  {
    if(str_has(bcache[i].BM, userid, uidlen))
    {
      int len = strlen(bcache[i].brdname);

      if(x + len > b_cols - 1)
      {
	if(++y >= b_lines)
	  break;
	x = sp;
	move(y, x);
      }
      outs(bcache[i].brdname);
      outc(' ');

      x += len + 1;
    }
  }
}


static void
adm_log(old, new)
  ACCT *old, *new;
{
  int i;
  usint bit, oldl, newl;
  char *userid, buf[80];

  userid = new->userid;
  alog("異動資料", userid);

  if (strcmp(old->passwd, new->passwd))
    alog("異動密碼", userid);

  if ((old->money != new->money) || (old->gold != new->gold))
  {
    sprintf(buf, "%-13s銀%d→%d 金%d→%d", userid, old->money, new->money, old->gold, new->gold);
    alog("異動金錢", buf);
  }

  /* Thor.990405: log permission modify */
  oldl = old->userlevel;
  newl = new->userlevel;
  for (i = 0, bit = 1; i < NUMPERMS; i++, bit <<= 1)
  {
    if ((newl & bit) != (oldl & bit))
    {
      sprintf(buf, "%-13s%s %s", userid, (newl & bit) ? BIT_ON : BIT_OFF, perm_tbl[i]);
      alog("異動權限", buf);
    }
  }
}


static void
acct_ADMINshow(u, type)
  ACCT *u;
  int type;
{
  int diff;
  usint ulevel = u->userlevel;
  char *uid = u->userid, buf[80];

  outs("\n\033[1m");

  if (type != 2)	/* type == 2: 審註冊單看對方資料 */
    prints("  \033[32m英文代號：\033[37m%-35s\033[32m用戶編號：\033[37m%d\n", uid, u->userno);

  prints("  \033[32m真實姓名：\033[37m%-35s\033[32m擁有銀幣：\033[37m%d\n", u->realname, u->money);

  prints("  \033[32m使用暱稱：\033[37m%-35s\033[32m擁有金幣：\033[37m%d\n", u->username, u->gold);

  prints("  \033[32m性別欄位：\033[37m%-35.2s", "？男女" + u->sex * 2);
  prints("\033[32m出生日期：\033[37m%d/%02d/%02d\n", u->year, u->month, u->day);

  prints("  \033[32m上站次數：\033[37m%-35d\033[32m文章篇數：\033[37m%d\n", u->numlogins, u->numposts);

  if(ulevel & PERM_LABEL)
  {
    outs("  \033[32m特殊標籤：");
    attrset(u->label_attr);
    outs(u->label);
    outs("\033[m\n\033[1m");
  }

  prints("  \033[32m郵件信箱：\033[37m%s\n", u->email);

  prints("  \033[32m註冊日期：\033[37m%s\n", Btime(&u->firstlogin));

  prints("  \033[32m光臨日期：\033[37m%s\n", Btime(&u->lastlogin));

  if (ulevel & PERM_ALLDENY)
  {
    outs("  \033[32m停權天數：\033[37m");
    if ((diff = u->tvalid - time(0)) < 0)
      outs("停權期限已到\n");
    else
      prints("還有 %d 天 %d 小時\n", diff / 86400, (diff % 86400) / 3600);
  }
  else
    prints("  \033[32m身份認證：\033[37m%s\n", (ulevel & PERM_VALID) ? Btime(&u->tvalid) : "未獲得身份認證");

  usr_fpath(buf, uid, fn_dir);
  prints("  \033[32m個人信件：\033[37m%d 封\n", rec_num(buf, sizeof(HDR)));

  prints("  \033[32m上站地點：\033[37m%-35s\033[32m發信次數：\033[37m%d\n", u->lasthost, u->numemails);

  bitmsg("  \033[32m權限等級：\033[37m", STR_PERM, ulevel);

  outs("  \033[32m習慣旗標：\033[37m");
  print_bits(STR_UFO, u->ufo);
  outs(", ");
  print_bits(STR_UFO2, u->ufo2);
  outc('\n');

  if(type != 2 && (ulevel & PERM_BM))
  {
    outs("  \033[32m擔任板主：\033[37m");
    bm_list(uid);
  }

  attrset(7);
}


void
acct_show(u, adm)
  ACCT *u;
  int adm;		/* 0: user info  1: admin  2: reg-form */
{
  char buf[128], fpath[64], address[80] = "\033[30m(none)\n", phone[80] = "\033[30m(none)\n";
  time_t diff;
  FILE *fp;

  clrtobot();
  if(adm)
  {
    acct_ADMINshow(u, adm);
    return;
  }

  outs(INFO_BANNER);
  outs("\n\033[1m");

  prints("  \033[32m使用者代號：\033[37m%-35s", u->userid);
  prints("\033[30m擁有銀幣\033[37m %d\n", u->money);

  prints("        \033[32m暱稱：\033[37m%-39s", u->username);
  prints("\033[30m金幣\033[37m %d\n", u->gold);

  prints("    \033[32m真實姓名：\033[37m%-39s", u->realname);
  prints("\033[30m上站\033[37m %d \033[30m次\n", u->numlogins);

  prints("        \033[32m性別：\033[37m%.2s\033[37C", "？男女" + u->sex * 2);
  prints("\033[30m文章\033[37m %d \033[30m篇\n", u->numposts);

  prints("    \033[32m出生日期：\033[37m%d 年 %d 月 %d 日\n", u->year, u->month, u->day);

  usr_fpath(fpath, u->userid, FN_RFINFO);
  if(fp = fopen(fpath, "r"))
  {
    fgets(phone, sizeof(phone), fp);
    fgets(address, sizeof(address), fp);
    fclose(fp);
  }
  prints("    \033[32m連絡電話：\033[37m%s", phone);
  prints("    \033[32m註冊地址：\033[37m%s", address);

  prints("  \033[32mE-mail信箱：\033[37m%s\n", u->email);

  prints("    \033[32m註冊日期：\033[37m%s\n", Btime(&u->firstlogin));

  prints("    \033[32m登入時間：\033[37m%-33s", Btime(&u->lastlogin));
  if(u->userlevel & PERM_ALLDENY)
  {
    diff = u->tvalid - time(0);

    if(diff <= 0)
      outs("\033[33m停權期限已到，可自行申請復權");
    else
    {
      char *unit[] = {"天", "小時", "分鐘", "秒"};
      int i, scale[] = {86400, 3600, 60, 1};

      for(i = 0; diff / scale[i] == 0; i++) ;

      prints("\033[31m距離停權到期還有 %d %s", diff / scale[i], unit[i]);
    }
  }
  else
  {
    outs("  \033[30m認證時間");
    if(u->userlevel & PERM_VALID)
      prints("\033[37m %s", strtok(Btime(&u->tvalid), " "));
    else
      outs("\033[36m <尚未完成身份認證>");
  }
  outc('\n');

  usr_fpath(buf, u->userid, fn_dir);
  prints("\033[32m個人信件數量：\033[37m%d 封\n", rec_num(buf, sizeof(HDR)));

  outs("    \033[32m本次停留：\033[37m");
  diff = (time(0) - ap_start) / 60;
  if(diff >= 60)
    prints("%d 小時 ", diff / 60);
  prints("%d 分鐘\n", diff % 60);

#ifdef NEWUSER_LIMIT
  if (u->lastlogin - u->firstlogin < 3 * 86400)
    outs("    \033[35m新手上路：三天後開放權限\n");
#endif

  if(u->userlevel & PERM_BM)
  {
    outs("    \033[32m擔任板主：\033[37m");
    bm_list(u->userid);
  }

  attrset(7);
}


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


int
acct_editlabel(acct, is_adm)
  ACCT *acct;
  int is_adm;
{
  char label[LBLEN + 1], c;
  uschar label_attr;
  char buf[8], *color = "brgylpcw", *s;

  move(13, 0);
  clrtobot();

  outs("\n原標籤 ");
  attrset(acct->label_attr);
  outs(acct->label);
  attrset(7);

  if(!vget(16, 0, "設定標籤為：", label, sizeof(label), DOECHO))
    return 0;

  /* 排除名單 */
  if(!is_adm && belong(FN_ETC_BADLABEL, label))
  {
    vmsg("無法接受這個標籤，請勿使用站務專用標籤或不雅字詞");
    return 0;
  }

  if(!vget(18, 0, "前景色 \033[1;33mB\033[41mR\033[42mG\033[43mY\033[44mL\033[45mP\033[46mC\033[47mW\033[m [W] ", buf, 2, DOECHO))
    c = 'x';
  else
    c = tolower(*buf);
  if(s = strchr(color, c))
    label_attr = s - color;
  else
    label_attr = 7;

  if(!vget(19, 0, "背景色 \033[1;33mB\033[41mR\033[42mG\033[43mY\033[44mL\033[45mP\033[46mC\033[47mW\033[m [B] ", buf, 2, DOECHO))
    c = 'x';
  else
    c = tolower(*buf);
  if(s = strchr(color, c))
    label_attr |= (s - color) << 4;
  else
    label_attr |= 0 << 4;	/* 照標準來寫，方便以後修改預設顏色 */

  if(vget(20, 0, "是否加亮 (Y/N) [Y] ", buf, 2, LCECHO) != 'n')
    label_attr |= 0x08;

  if(is_adm)	/* dust.101005: 有鑒於User list實在太過閃亮，所以限制只有站務能給閃爍 */
  {
    if(vget(21, 0, "是否閃爍 (Y/N) [N] ", buf, 2, LCECHO) == 'y')
      label_attr |= 0x80;
  }

  if(vans(msg_sure_ny) == 'y')	/* 寫進acct中 */
  {
    acct->label_attr = label_attr;
    strcpy(acct->label, label);
    return 1;
  }
}


void
acct_setup(u, adm)
  ACCT *u;
  int adm;
{
  ACCT x;
  int i, num;
  char *str, buf[80], pass[PSWDLEN + 1];

  acct_show(u, adm);
  memcpy(&x, u, sizeof(ACCT));

  if (adm)
  {
    adm = vans("設定 1)資料 2)權限 3)標籤 Q)取消 [Q] ");
    if (adm == '2')
      goto set_perm;

    if(adm == '3')
    {
      if(acct_editlabel(&x, 1))
	goto update_acct;
      else
	return;
    }

    if (adm != '1')
      return;
  }
  else
  {
    if (vans("修改資料(Y/N)？[N] ") != 'y')
      return;
  }

  move(i = 3, 0);
  clrtobot();

  if (adm)
  {
    str = x.userid;
    for (;;)
    {
      /* itoc.010804.註解: 改使用者代號時請確定該 user 不在站上 */
      vget(i, 0, "使用者代號(不改請按 Enter)：", str, IDLEN + 1, GCARRY);
      if (!str_cmp(str, u->userid) || !acct_userno(str))
	break;
      vmsg("錯誤！已有相同 ID 的使用者");
    }
  }
  else
  {
    vget(i, 0, "請確認密碼：", buf, PSWDLEN + 1, NOECHO);
    if (chkpasswd(u->passwd, buf))
    {
      vmsg("密碼錯誤");
      return;
    }
  }

  /* itoc.030223: 只有 PERM_SYSOP 能變更其他站務的密碼 */
  if (!adm || !(u->userlevel & PERM_ALLADMIN) || HAS_PERM(PERM_SYSOP))
  {
    i++;
    for (;;)
    {
      if (!vget(i, 0, "設定新密碼(不改請按 Enter)：", buf, PSWDLEN + 1, NOECHO))
	break;

      strcpy(pass, buf);
      vget(i + 1, 0, "檢查新密碼：", buf, PSWDLEN + 1, NOECHO);
      if (!strcmp(buf, pass))
      {
	str_ncpy(x.passwd, genpasswd(buf), sizeof(x.passwd));
	break;
      }
    }
  }

  i++;
  str = x.username;
  while (1)
  {
    if (vget(i, 0, "暱    稱：", str, UNLEN + 1, GCARRY))
      break;
  };

  i++;
  do
  {
    sprintf(buf, "%d", u->year);
    vget(i, 0, "出生年：", buf, 5, GCARRY);
    x.year = atoi(buf);
  } while (x.year < 1);
  do
  {
    sprintf(buf, "%d", u->month);
    vget(i, 22, "月份：", buf, 3, GCARRY);
    x.month = atoi(buf);
  } while (x.month < 0 || x.month > 12);
  do
  {
    sprintf(buf, "%d", u->day);
    vget(i, 37, "日：", buf, 3, GCARRY);
    x.day = atoi(buf);
  } while (x.day < 0 || x.day > 31);

  i++;
  sprintf(buf, "性別 (0)中性 (1)男性 (2)女性：[%d] ", u->sex);
  if (vget(i, 0, buf, buf, 3, DOECHO))
    x.sex = (*buf - '0') & 3;

  /* dust.110125: 若 user 是用E-mail認證，當然不能改信箱。
     但是我懶得寫判斷，加上龍夢/極光沒有，就先這樣workround了...... */
#ifndef EMAIL_JUSTIFY
  vget(++i, 0, "電子信箱：", x.email, sizeof(x.email), GCARRY);
#endif

  if (adm)
  {
    i++;
    do
    {
      vget(i, 0, "真實姓名：", x.realname, RNLEN + 1, GCARRY);
    } while (strlen(x.realname) < 2);

    sprintf(buf, "%d", u->userno);
    vget(++i, 0, "用戶編號：", buf, 11, GCARRY);
    if ((num = atoi(buf)) > 0)
      x.userno = num;

    sprintf(buf, "%d", u->numlogins);
    vget(++i, 0, "上線次數：", buf, 11, GCARRY);
    if ((num = atoi(buf)) >= 0)
      x.numlogins = num;

    sprintf(buf, "%d", u->numposts);
    vget(++i, 0, "文章篇數：", buf, 11, GCARRY);
    if ((num = atoi(buf)) >= 0)
      x.numposts = num;

    if(cuser.userlevel & PERM_SpSYSOP)
    {
      sprintf(buf, "%d", u->money);
      vget(++i, 0, "銀    幣：", buf, 11, GCARRY);
      if ((num = atoi(buf)) >= 0)
        x.money = num;

      sprintf(buf, "%d", u->gold);
      vget(++i, 0, "金    幣：", buf, 11, GCARRY);
      if ((num = atoi(buf)) >= 0)
        x.gold = num;
    }

    sprintf(buf, "%d", u->numemails);
    vget(++i, 0, "發信次數：", buf, 11, GCARRY);
    if ((num = atoi(buf)) >= 0)
      x.numemails = num;

    vget(++i, 0, "上站地點：", x.lasthost, sizeof(x.lasthost), GCARRY);
#ifdef EMAIL_JUSTIFY
    vget(++i, 0, "電子信箱：", x.email, sizeof(x.email), GCARRY);
#endif

    if (vans("設定習慣(Y/N)？[N] ") == 'y')
    {
#if 0
      x.ufo = bitset(x.ufo, NUMUFOS, NUMUFOS, MSG_USERUFO, ufo_tbl);
#else
      void *p = DL_get("bin/ufo.so:ufo_main");

      if (!p)
      {
	p = dlerror();
	if(p) vmsg((char *)p);
      }

      ((int (*)(ACCT*, UTMP*))p) (&x, NULL);
#endif
    }

    if (vans("設定權限(Y/N)？[N] ") == 'y')
    {
set_perm:

      i = setperm(num = x.userlevel);

      if (i == num)
      {
	vmsg("取消修改");
	if (adm == '2')
	  return;
      }
      else
      {
	x.userlevel = i;

	/* itoc.011120: 站長放水加上認證通過權限，要附加改認證時間 */
	if ((i & PERM_VALID) && !(num & PERM_VALID))
	  time(&x.tvalid);

	/* itoc.050413: 如果站長手動停權，就要由站長才能來復權 */
	if ((i & PERM_ALLDENY) && (i & PERM_ALLDENY) != (num & PERM_ALLDENY))
	  x.tvalid = INT_MAX;
      }
    }
  }

  if (!memcmp(&x, u, sizeof(ACCT)) || vans(msg_sure_ny) != 'y')
    return;

  if (adm)
  {
update_acct:

    if (str_cmp(u->userid, x.userid))
    { /* Thor: 980806: 特別注意如果 usr每個字母不在同一partition的話會有問題 */
      char dst[80];

      usr_fpath(buf, u->userid, NULL);
      usr_fpath(dst, x.userid, NULL);
      rename(buf, dst);
      /* Thor.990416: 特別注意! .USR並未一併更新, 可能有部分問題 */
    }

    /* itoc.010811: 動態設定線上使用者 */
    /* 被站長改過資料的線上使用者(包括站長自己)，其 cutmp->status 會被加上 STATUS_DATALOCK
       這個旗標，就無法 acct_save()，於是站長便可以修改線上使用者資料 */
    /* 在站長修改過才上線的 ID 因為其 cutmp->status 沒有 STATUS_DATALOCK 的旗標，
       所以將可以繼續存取，所以線上如果同時有修改前、修改後的同一隻 ID multi-login，也是無妨。 */
    utmp_admset(x.userno, STATUS_DATALOCK | STATUS_COINLOCK);

    /* lkchu.981201: security log */
    adm_log(u, &x);
  }
  else
  {
    /* itoc.010804.註解: 線上的 userlevel/tvalid 是舊的，.ACCT 裡才是新的 */
    if (acct_load(u, x.userid) >= 0)
    {
      x.userlevel = u->userlevel;
      x.tvalid = u->tvalid;
    }
  }

  memcpy(u, &x, sizeof(ACCT));
  acct_save(u);
}


#if 0	/* itoc.010805.註解 */

  認證成功只加上 PERM_VALID，讓 user 在下次進站才自動得到 PERM_POST | PERM_PAGE | PERM_CHAT
  以免新手上路、停權的功能失效

  但重填 email 拿掉認證者需拿掉 PERM_VALID | PERM_POST | PERM_PAGE | PERM_CHAT
  否則 user 可以在下次進站前任意使用 bbs_post

#endif

#if 0	/* itoc.010831.註解 */

  因為線上 cuser.userlevel 並不是最新的，使用者如果在線上認證或是被停權，
  硬碟中的 .ACCT 寫的才是正確的 userlevel，
  所以要先讀出 .ACCT，加入 level 後再蓋回去。

  使用 acct_seperm(&acct, adm) 之前要先 acct_load(&acct, userid)，
  其中 &acct 不能是 &cuser。
  使用者要重新上站才會換成新的權限。

#endif

void
acct_setperm(u, levelup, leveldown)	/* itoc.000219: 加/減權限程式 */
  ACCT *u;
  usint levelup;		/* 加權限 */
  usint leveldown;		/* 減權限 */
{
  u->userlevel |= levelup;
  u->userlevel &= ~leveldown;

  acct_save(u);
}


/* ----------------------------------------------------- */
/* 增加金銀幣						 */
/* ----------------------------------------------------- */


void
addmoney(addend)
  int addend;
{
  if (addend < (INT_MAX - cuser.money))	/* 避免溢位 */
    cuser.money += addend;
  else
    cuser.money = INT_MAX;
}


void
addgold(addend)
  int addend;
{
  if (addend < (INT_MAX - cuser.gold))	/* 避免溢位 */
    cuser.gold += addend;
  else
    cuser.gold = INT_MAX;
}


/* ----------------------------------------------------- */
/* 看板管理						 */
/* ----------------------------------------------------- */


#ifndef HAVE_COSIGN
static
#endif
int			/* 1:合法的板名 */
valid_brdname(brd)
  char *brd;
{
  int ch;

  if (!is_alnum(*brd))
    return 0;

  while (ch = *++brd)
  {
    if (!is_alnum(ch) && ch != '.' && ch != '-' && ch != '_')
      return 0;
  }
  return 1;
}


static int
brd_set(brd, row)
  BRD *brd;
  int row;
{
  int i, BMlen, len;
  char *brdname, buf[80], userid[IDLEN + 2];
  ACCT acct;

  i = row;
  brdname = brd->brdname;
  strcpy(buf, brdname);

  for (;;)
  {
    if (!vget(i, 0, MSG_BID, brdname, BNLEN + 1, GCARRY))
    {
      if (i == 1)	/* 開新板若無輸入板名表示離開 */
	return -1;

      strcpy(brdname, buf);	/* Thor: 若是清空則設為原名稱 */
      continue;
    }

    if (!valid_brdname(brdname))
      continue;

    if (!str_cmp(buf, brdname))	/* Thor: 與舊板原名相同則跳過 */
      break;

    if (brd_bno(brdname) >= 0)
      outs("\n錯誤！板名雷同");
    else
      break;
  }

  vget(++i, 0, "看板分類：", brd->class, BCLEN + 1, GCARRY);
  vget(++i, 0, "看板主題：", brd->title, BTLEN + 1, GCARRY);

  /* vget(++i, 0, "板主名單：", brd->BM, BMLEN + 1, GCARRY); */

  /* itoc.010212: 開新板/修改看板自動加上板主權限. */
  /* 目前的作法是一輸入完 id 就加入板主權限，即使最後選擇不變動，
     如果因此多加了板主權限，在 reaper.c 中拿下 */

  i += 4;
  move(i - 2, 0);
  prints("目前板主為 %s\n請輸入新的板主名單，或按 [Return] 不改", brd->BM);

  strcpy(buf, brd->BM);
  BMlen = strlen(buf);

  while (vget(i, 0, "請輸入板主，結束請按 Enter，清掉所有板主請打「無」：", userid, IDLEN + 1, DOECHO))
  {
    if (!strcmp(userid, "無"))
    {
      buf[0] = '\0';
      BMlen = 0;
    }
    else if (is_bm(buf, userid))	/* 刪除舊有的板主 */
    {
      len = strlen(userid);
      if (BMlen == len)
      {
	buf[0] = '\0';
      }
      else if (!str_cmp(buf + BMlen - len, userid))	/* 名單上最後一位，ID 後面不接 '/' */
      {
	buf[BMlen - len - 1] = '\0';			/* 刪除 ID 及前面的 '/' */
	len++;
      }
      else						/* ID 後面會接 '/' */
      {
	str_lower(userid, userid);
	strcat(userid, "/");
	len++;
	brdname = str_str(buf, userid);
        strcpy(brdname, brdname + len);
      }
      BMlen -= len;
    }
    else if (acct_load(&acct, userid) >= 0 && !is_bm(buf, userid))	/* 輸入新板主 */
    {
      len = strlen(userid);
      if (BMlen)
      {
	len++;		/* '/' + userid */
	if (BMlen + len > BMLEN)
	{
	  vmsg("板主名單過長，無法將這 ID 設為板主");
	  continue;
	}
	sprintf(buf + BMlen, "/%s", acct.userid);
	BMlen += len;
      }
      else
      {
	strcpy(buf, acct.userid);
	BMlen = len;
      }      

      acct_setperm(&acct, PERM_BM, 0);
    }
    else
      continue;

    move(i - 2, 0);
    prints("目前板主為 %s", buf);
    clrtoeol();
  }
  strcpy(brd->BM, buf);


#ifdef HAVE_MODERATED_BOARD
  /* itoc.011208: 改用較便利的看板權限設定 */
  switch (vget(++i, 0, "看板權限 A)一般 B)自定 C)秘密 D)好友？[Q] ", buf, 3, LCECHO))
  {
  case 'c':
    brd->readlevel = PERM_SYSOP;	/* 秘密看板 */
    brd->postlevel = 0;
    brd->battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;

  case 'd':
    brd->readlevel = PERM_BOARD;	/* 好友看板 */
    brd->postlevel = 0;
    brd->battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;
#else
  switch (vget(++i, 0, "看板權限 A)一般 B)自定？[Q] ", buf, 3, LCECHO))
  {
#endif

  case 'a':
    brd->readlevel = 0;
    brd->postlevel = PERM_POST;		/* 一般看板發表權限為 PERM_POST */
    brd->battr &= ~(BRD_NOSTAT | BRD_NOVOTE);	/* 拿掉好友＆秘密板屬性 */
    break;

  case 'b':
    if (vget(++i, 0, "閱\讀權限(Y/N)？[N] ", buf, 3, LCECHO) == 'y')
    {
      brd->readlevel = bitset(brd->readlevel, NUMPERMS, NUMPERMS, MSG_READPERM, perm_tbl);
      move(2, 0);
      clrtobot();
      i = 1;
    }

    if (vget(++i, 0, "發表權限(Y/N)？[N] ", buf, 3, LCECHO) == 'y')
    {
      brd->postlevel = bitset(brd->postlevel, NUMPERMS, NUMPERMS, MSG_POSTPERM, perm_tbl);
      move(2, 0);
      clrtobot();
      i = 1;
    }
    break;

  default:	/* 預設不變動 */
    break;
  }

  if (vget(++i, 0, "設定屬性(Y/N)？[N] ", buf, 3, LCECHO) == 'y')
    brd->battr = bitset(brd->battr, NUMBATTRS, NUMBATTRS, MSG_BRDATTR, battr_tbl);

  return 0;
}


int			/* 0:開板成功 -1:開板失敗 */
brd_new(brd)
  BRD *brd;
{
  int bno;
  char fpath[64];
  char title[TTLEN + 1];
  FILE *fp;
  int xmode = 0;


  vs_bar("建立新板");

  if (brd_set(brd, 1))
    return -1;

  if (vans(msg_sure_ny) != 'y')
    return -1;

  time(&brd->bstamp);

  /* dust.091010:若在"個人"分類中就加上BRD_PERSONAL */
  if(!strcmp(brd->class, "個人"))
    brd->battr |= BRD_PERSONAL;

  if ((bno = brd_bno("")) >= 0)
  {
    rec_put(FN_BRD, brd, sizeof(BRD), bno, NULL);
  }
  /* Thor.981102: 防止超過shm看板個數 */
  else if (bshm->number >= MAXBOARD)
  {
    vmsg("超過系統所能容納看板個數，請調整系統參數");
    return -1;
  }
  else if (rec_add(FN_BRD, brd, sizeof(BRD)) < 0)
  {
    vmsg("無法建立新板");
    return -1;
  }

  gem_fpath(fpath, brd->brdname, NULL);
  mak_dirs(fpath);
  mak_dirs(fpath + 4);

  bshm_reload();		/* force reload of bcache */

  strcpy(fpath, "run/newbrdlog");   /* 暫存檔 */

  alog("建立看板", brd->brdname);	/* dust.090320:開板要記錄。 */

  if (fp = fopen(fpath, "w"))
  {
    sprintf(title, "新冰屋 [%s] 落成",  brd->brdname);

    /* 文章檔頭 */

    fprintf(fp, "%s %s (%s)\n",
    str_author1, "[冰屋管理員]", "系統公告");
    fprintf(fp, "標題: %s\n時間: %s\n\n", title, Now());

    /* 文章內容 */

    fprintf(fp, "\033[38m冰屋名稱：\033[m\033[1m%s\033[m\n", brd->brdname);
    fprintf(fp, "\033[38m冰屋門牌：\033[m\033[1m%s\033[m\n\n", brd->title);
    fprintf(fp, "\033[38m冰屋主人：\033[m\033[1m%s\033[m\n", brd->BM);
    fprintf(fp, "\033[38m冰屋分區：\033[m\033[1m%s\033[m\n\n", brd->class);

    if (brd->readlevel == PERM_SYSOP)         /* 秘密看板 */
      fprintf(fp, "\033[38m冰屋屬性：\033[m\033[1;31m秘密冰屋\033[m\n\n");
    else if (brd->readlevel == PERM_BOARD)        /* 好友看板 */
      fprintf(fp, "\033[38m冰屋屬性：\033[m\033[1;33m好友冰屋\033[m\n\n");


    fprintf(fp, "\033[38m落成時間：\033[m\033[1;36m%s\033[m\n\n", Now());


    fprintf(fp, "此冰屋由本站客服 \033[1;33m%s\033[m 建立，\n", cuser.userid);
    fprintf(fp, "歡迎大家多多光臨噢  :) \n");
    fclose(fp);

    xmode |= POST_DONE;

    /* 是秘密看板或好友看板的話要加密 */
    if (brd->readlevel == PERM_SYSOP || brd->readlevel == PERM_BOARD)
      xmode |= POST_RESTRICT;

    add_post("NewIceHouse", fpath, title, "[冰屋管理員]", "[冰屋管理員]", xmode);
    unlink(fpath);
  }    

  rhs_save();
  board_main();			/* reload brd_bits[] */

  return 0;
}


void
brd_classchange(folder, oldname, newbrd)	/* itoc.020117: 異動 @Class 中的看板 */
  char *folder;
  char *oldname;
  BRD *newbrd;		/* 若為 NULL，表示要刪除看板 */
{
  int pos, xmode;
  char fpath[64];
  HDR hdr;

  pos = 0;
  while (!rec_get(folder, &hdr, sizeof(HDR), pos))
  {
    xmode = hdr.xmode & (GEM_BOARD | GEM_FOLDER);

    if (xmode == (GEM_BOARD | GEM_FOLDER))	/* 看板精華區捷徑 */
    {
      if (!strcmp(hdr.xname, oldname))
      {
	if (newbrd)	/* 看板更名 */
	{
	  brd2gem(newbrd, &hdr);
	  rec_put(folder, &hdr, sizeof(HDR), pos, NULL);
	}
	else		/* 看板刪除 */
	{
	  rec_del(folder, sizeof(HDR), pos, NULL);
	  continue;	/* rec_del 以後不需要 pos++ */
	}
      }
    }
    else if (xmode == GEM_FOLDER)		/* 分類 recursive 進去砍 */
    {
      hdr_fpath(fpath, folder, &hdr);
      brd_classchange(fpath, oldname, newbrd);
    }
    pos++;
  }
}


static inline void
strcat_brdreadlevel(buf, rlevel)
  char *buf;
  usint rlevel;
{
  switch(rlevel)
  {
    case PERM_SYSOP:
      strcat(buf, "秘密板");
      break;
    case PERM_BOARD:
      strcat(buf, "好友板");
      break;
    case 0:
      strcat(buf, "公開板");
      break;
    default :
      strcat(buf, "(自訂)");
  }
}

static void
adm_brdlog(old, new)
  BRD *old, *new;
{
  int i;
  usint bit, oldl, newl;
  char *brdname, buf[256];

  brdname = new->brdname;

  if(strcmp(old->brdname, brdname))
  {
    sprintf(buf, "%s→%s", old->brdname, brdname);
    alog("異動看板板名", buf);
  }

  if (strcmp(old->BM, new->BM))
  {
    sprintf(buf, "%-13s：\n   %s\n→ %s", brdname, old->BM, new->BM);
    alog("異動看板板主", buf);
  }

  if(old->readlevel != new->readlevel)
  {
    sprintf(buf, "%-13s", brdname);

    strcat_brdreadlevel(buf, old->readlevel);	/* dust:這是為了code好看才寫成這樣。*/
    strcat(buf, "→");
    strcat_brdreadlevel(buf, new->readlevel);

    alog("異動看板權限", buf);
  }

  /* 看板旗標修改 */
  oldl = old->battr;
  newl = new->battr;
  for (i = 0, bit = 1; i < NUMBATTRS; i++, bit <<= 1)
  {
    if ((newl & bit) != (oldl & bit))
    {
      sprintf(buf, "%-13s%s %s", brdname, (newl & bit) ? BIT_ON : BIT_OFF, battr_tbl[i]);
      alog("異動看板旗標", buf);
    }
  }
}


void
brd_edit(bno)
  int bno;
{
  BRD *bhdr, newbh;
  char *bname, src[64], dst[64];;

  vs_bar("看板設定");
  bhdr = bshm->bcache + bno;
  memcpy(&newbh, bhdr, sizeof(BRD));
  prints("看板名稱：%s\n看板說明：[%s] %s\n板主名單：%s\n",
    newbh.brdname, newbh.class, newbh.title, newbh.BM);

  bitmsg(MSG_READPERM, STR_PERM, newbh.readlevel);
  bitmsg(MSG_POSTPERM, STR_PERM, newbh.postlevel);
  bitmsg(MSG_BRDATTR, STR_BATTR, newbh.battr);

  switch (vget(8, 0, "(D)刪除 (E)設定 (A)設定屬性 (Q)取消？[Q] ", src, 3, LCECHO))
  {
  case 'd':

    if (vget(9, 0, msg_sure_ny, src, 3, LCECHO) != 'y')
    {
      vmsg(MSG_DEL_CANCEL);
    }
    else
    {
      bname = bhdr->brdname;
      if (*bname)	/* itoc.000512: 同時砍除同一個看板會造成精華區、看板全毀 */
      {
	alog("刪除看板", bname);

	gem_fpath(src, bname, NULL);
	f_rm(src);
	f_rm(src + 4);
	brd_classchange("gem/@/@"CLASS_INIFILE, bname, NULL);	/* itoc.020117: 刪除 @Class 中的看板精華區捷徑 */
	memset(&newbh, 0, sizeof(BRD));
	sprintf(newbh.title, "[%s] deleted by %s", bname, cuser.userid);
	memcpy(bhdr, &newbh, sizeof(BRD));
	rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);

	/* itoc.050531: 砍板會造成看板不是按字母排序，所以要修正 numberOld */
	if (bshm->numberOld > bno)
	  bshm->numberOld = bno;

	vmsg("刪板完畢");
      }
    }
    break;

  case 'e':

    move(9, 0);
    outs("直接按 [Return] 不修改該項設定");

    if (!brd_set(&newbh, 11))
    {
      if (memcmp(&newbh, bhdr, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
      {
	bname = bhdr->brdname;
	if (strcmp(bname, newbh.brdname))	/* 看板更名要移目錄 */
	{
	  /* Thor.980806: 特別注意如果看板不在同一partition裡的話會有問題 */
	  gem_fpath(src, bname, NULL);
	  gem_fpath(dst, newbh.brdname, NULL);
	  rename(src, dst);
	  rename(src + 4, dst + 4);
	  /* brd_classchange("gem/@/@"CLASS_INIFILE, bname, &newbh); */
	  /* itoc.050329: 異動 @Class 中的看板精華區捷徑 */

	  /* itoc.050520: 改了板名會造成看板不是按字母排序，所以要修正 numberOld */
	  if (bshm->numberOld > bno)
	    bshm->numberOld = bno;
	}
	adm_brdlog(bhdr, &newbh);
	memcpy(bhdr, &newbh, sizeof(BRD));
	rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);
        brd_classchange("gem/@/@"CLASS_INIFILE, bname, &newbh);
      }
    }
    vmsg("設定完畢");
    break;

  case 'a':
    newbh.battr = bitset(newbh.battr, NUMBATTRS, NUMBATTRS, MSG_BRDATTR, battr_tbl);
    if(memcmp(&newbh, bhdr, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
    {
      adm_brdlog(bhdr, &newbh);
      memcpy(bhdr, &newbh, sizeof(BRD));
      rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);
    }
    break;
  }
}


void
brd_title(bno)		/* itoc.000312: 板主修改中文敘述 */
  int bno;
{
  BRD *bhdr, newbh;
  char *blist;

  bhdr = bshm->bcache + bno;
  memcpy(&newbh, bhdr, sizeof(BRD));

  blist = bhdr->BM;

  if (blist[0] > ' ' && is_bm(blist, cuser.userid))
  {
    if (vans("是否修改看板標題(Y/N)？[N] ") == 'y')
    {
      vget(b_lines, 0, "看板標題：", newbh.title, BTLEN + 1, GCARRY);
      memcpy(bhdr, &newbh, sizeof(BRD));
      rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);
      brd_classchange("gem/@/@"CLASS_INIFILE, bhdr->brdname, &newbh);
    }
  }
}
