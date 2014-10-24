/*-------------------------------------------------------*/
/* util/transacct.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : M3 ACCT 轉換程式				 */
/* create : 98/12/15					 */
/* update : 11/01/02					 */
/* author : mat.bbs@fall.twbbs.org			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/
/* syntax : transacct [userid]				 */
/*-------------------------------------------------------*/


#if 0

  使用方法：

  0. For Maple 3.X To Maple 3.X

  1. 使用前請先利用 backupacct.c 備份 .ACCT

  2. 請自行改 struct NEW 和 struct OLD

  3. 依不同的 NEW、OLD 來改 trans_acct()。

#endif


#include "bbs.h"


/* ----------------------------------------------------- */
/* (新的) 使用者帳號 .ACCT struct			 */
/* ----------------------------------------------------- */

typedef struct
{
  int userno;                   /* unique positive code */

  char userid[IDLEN + 1];       /* ID */
  char passwd[PASSLEN + 1];     /* 密碼 */
  char realname[RNLEN + 1];     /* 真實姓名 */
  char username[UNLEN + 1];     /* 暱稱 */
  uschar label_attr;            /* 標籤色彩 */
  char label[LBLEN + 1];        /* 標籤 */

  char email[60];               /* 目前登記的電子信箱 */
  char lasthost[55];            /* 上次登入來源 */

  uschar signature;             /* 預設簽名檔 */
  char sex;                     /* 性別, 0:中性 奇數:男性 偶數:女性 */
  char day;                     /* 出生日 */
  char month;                   /* 出生月份 */
  int year;                     /* 出生年(西元) */

  usint userlevel;              /* 權限 */
  usint ufo;                    /* habit */
  usint ufo2;                   /* habit: extending */

  int money;                    /* 銀幣 */
  int gold;                     /* 金幣 */
  short sig_avai;               /* 可用簽名檔數量 */
  short sig_plus;               /* 簽名檔擴增量 */

  int numlogins;                /* 上站次數 */
  int numposts;                 /* 發表次數 */
  int numemails;                /* 寄發 Inetrnet E-mail 次數 */

  time_t firstlogin;            /* 第一次上站時間 */
  time_t lastlogin;             /* 上一次上站時間 */
  time_t tcheck;                /* 上次 check 信箱/朋友名單的時間 */
  time_t tvalid;                /* 若停權，停權期滿的時間；
                                   若未停權且通過認證，通過認證的時間；
                                   若未停權且未通過認證，認證函的 time-seed */
} NEW;


/* ----------------------------------------------------- */
/* (舊的) 使用者帳號 .ACCT struct			 */
/* ----------------------------------------------------- */

typedef struct
{
  int userno;                   /* unique positive code */

  char userid[IDLEN + 1];       /* ID */
  char passwd[PASSLEN + 1];     /* 密碼 */
  char realname[RNLEN + 1];     /* 真實姓名 */
  char username[UNLEN + 1];     /* 暱稱 */
  uschar label_attr;            /* 標籤色彩 */
  char label[LBLEN + 1];        /* 標籤 */
  char year;                    /* 生日(民國年) */
  char month;                   /* 生日(月) */
  char day;                     /* 生日(日) */
  char sex;                     /* 性別 0:中性 奇數:男性 偶數:女性 */
  char email[60];               /* 目前登記的電子信箱 */
  uschar signature;             /* 預設簽名檔 */
  char lasthost[58];            /* 上次登入來源 */

  usint userlevel;              /* 權限 */
  usint ufo;                    /* habit */
  usint ufo2;                   /* habit: extending */

  int money;                    /* 銀幣 */
  int gold;                     /* 金幣 */
  short sig_avai;               /* 可用簽名檔數量 */
  short sig_plus;               /* 簽名檔擴增量 */

  int numlogins;                /* 上站次數 */
  int numposts;                 /* 發表次數 */
  int numemails;                /* 寄發 Inetrnet E-mail 次數 */

  time_t firstlogin;            /* 第一次上站時間 */
  time_t lastlogin;             /* 上一次上站時間 */
  time_t tcheck;                /* 上次 check 信箱/朋友名單的時間 */
  time_t tvalid;                /* 若停權，停權期滿的時間；
                                   若未停權且通過認證，通過認證的時間；
                                   若未停權且未通過認證，認證函的 time-seed */
} OLD;



/* ----------------------------------------------------- */
/* 轉換主程式						 */
/* ----------------------------------------------------- */


static void
trans_acct(old, new)
  OLD *old;
  NEW *new;
{
  memset(new, 0, sizeof(NEW));


  new->userno = old->userno;

  str_ncpy(new->userid, old->userid, sizeof(new->userid));
  str_ncpy(new->passwd, old->passwd, sizeof(new->passwd));
  str_ncpy(new->realname, old->realname, sizeof(new->realname));
  str_ncpy(new->username, old->username, sizeof(new->username));
  new->label_attr = old->label_attr;
  str_ncpy(new->label, old->label, sizeof(new->label));

  str_ncpy(new->email, old->email, sizeof(new->email));
  str_ncpy(new->lasthost, old->lasthost, sizeof(new->lasthost));

  new->signature = old->signature;
  new->sex = old->sex;
  new->day = old->day;
  new->month = old->month;
  new->year = old->year + 1911;

  new->userlevel = old->userlevel;
  new->ufo = old->ufo;
  new->ufo2 = old->ufo2;

  new->money = old->money;
  new->gold = old->gold;
  new->sig_avai = old->sig_avai;
  new->sig_plus = old->sig_plus;

  new->numlogins = old->numlogins;
  new->numposts = old->numposts;
  new->numemails = old->numemails;

  new->firstlogin = old->firstlogin;
  new->lastlogin = old->lastlogin;
  new->tcheck = old->tcheck;
  new->tvalid = old->tvalid;
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  NEW new;
  char c;
  char buf[64];
  struct dirent *de;
  DIR *dirp;
  OLD old;
  int fd;
  char *str;

  if (argc > 2)
  {
    printf("Usage: %s [userid]\n", argv[0]);
    return -1;
  }

  for (c = 'a'; c <= 'z'; c++)
  {
    sprintf(buf, BBSHOME "/usr/%c", c);
    chdir(buf);

    if (!(dirp = opendir(".")))
      continue;

    while (de = readdir(dirp))
    {
      str = de->d_name;
      if (*str <= ' ' || *str == '.')
	continue;

      if ((argc == 2) && str_cmp(str, argv[1]))
	continue;

      sprintf(buf, "%s/" FN_ACCT, str);
      if ((fd = open(buf, O_RDONLY)) < 0)
	continue;

      read(fd, &old, sizeof(OLD));
      close(fd);
      unlink(buf);			/* itoc.010831: 砍掉原來的 FN_ACCT */

      printf("%s\n", buf);
      trans_acct(&old, &new);

      fd = open(buf, O_WRONLY | O_CREAT, 0600);	/* itoc.010831: 重建新的 FN_ACCT */
      write(fd, &new, sizeof(NEW));
      close(fd);
    }

    closedir(dirp);
  }

  printf("done. %d => %d\n", sizeof(OLD), sizeof(NEW));
  return 0;
}


