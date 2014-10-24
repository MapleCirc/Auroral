/*-------------------------------------------------------*/
/* util/transacct.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : M3 ACCT �ഫ�{��				 */
/* create : 98/12/15					 */
/* update : 11/01/02					 */
/* author : mat.bbs@fall.twbbs.org			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/
/* syntax : transacct [userid]				 */
/*-------------------------------------------------------*/


#if 0

  �ϥΤ�k�G

  0. For Maple 3.X To Maple 3.X

  1. �ϥΫe�Х��Q�� backupacct.c �ƥ� .ACCT

  2. �Цۦ�� struct NEW �M struct OLD

  3. �̤��P�� NEW�BOLD �ӧ� trans_acct()�C

#endif


#include "bbs.h"


/* ----------------------------------------------------- */
/* (�s��) �ϥΪ̱b�� .ACCT struct			 */
/* ----------------------------------------------------- */

typedef struct
{
  int userno;                   /* unique positive code */

  char userid[IDLEN + 1];       /* ID */
  char passwd[PASSLEN + 1];     /* �K�X */
  char realname[RNLEN + 1];     /* �u��m�W */
  char username[UNLEN + 1];     /* �ʺ� */
  uschar label_attr;            /* ���Ҧ�m */
  char label[LBLEN + 1];        /* ���� */

  char email[60];               /* �ثe�n�O���q�l�H�c */
  char lasthost[55];            /* �W���n�J�ӷ� */

  uschar signature;             /* �w�]ñ�W�� */
  char sex;                     /* �ʧO, 0:���� �_��:�k�� ����:�k�� */
  char day;                     /* �X�ͤ� */
  char month;                   /* �X�ͤ�� */
  int year;                     /* �X�ͦ~(�褸) */

  usint userlevel;              /* �v�� */
  usint ufo;                    /* habit */
  usint ufo2;                   /* habit: extending */

  int money;                    /* �ȹ� */
  int gold;                     /* ���� */
  short sig_avai;               /* �i��ñ�W�ɼƶq */
  short sig_plus;               /* ñ�W���X�W�q */

  int numlogins;                /* �W������ */
  int numposts;                 /* �o���� */
  int numemails;                /* �H�o Inetrnet E-mail ���� */

  time_t firstlogin;            /* �Ĥ@���W���ɶ� */
  time_t lastlogin;             /* �W�@���W���ɶ� */
  time_t tcheck;                /* �W�� check �H�c/�B�ͦW�檺�ɶ� */
  time_t tvalid;                /* �Y���v�A���v�������ɶ��F
                                   �Y�����v�B�q�L�{�ҡA�q�L�{�Ҫ��ɶ��F
                                   �Y�����v�B���q�L�{�ҡA�{�Ҩ窺 time-seed */
} NEW;


/* ----------------------------------------------------- */
/* (�ª�) �ϥΪ̱b�� .ACCT struct			 */
/* ----------------------------------------------------- */

typedef struct
{
  int userno;                   /* unique positive code */

  char userid[IDLEN + 1];       /* ID */
  char passwd[PASSLEN + 1];     /* �K�X */
  char realname[RNLEN + 1];     /* �u��m�W */
  char username[UNLEN + 1];     /* �ʺ� */
  uschar label_attr;            /* ���Ҧ�m */
  char label[LBLEN + 1];        /* ���� */
  char year;                    /* �ͤ�(����~) */
  char month;                   /* �ͤ�(��) */
  char day;                     /* �ͤ�(��) */
  char sex;                     /* �ʧO 0:���� �_��:�k�� ����:�k�� */
  char email[60];               /* �ثe�n�O���q�l�H�c */
  uschar signature;             /* �w�]ñ�W�� */
  char lasthost[58];            /* �W���n�J�ӷ� */

  usint userlevel;              /* �v�� */
  usint ufo;                    /* habit */
  usint ufo2;                   /* habit: extending */

  int money;                    /* �ȹ� */
  int gold;                     /* ���� */
  short sig_avai;               /* �i��ñ�W�ɼƶq */
  short sig_plus;               /* ñ�W���X�W�q */

  int numlogins;                /* �W������ */
  int numposts;                 /* �o���� */
  int numemails;                /* �H�o Inetrnet E-mail ���� */

  time_t firstlogin;            /* �Ĥ@���W���ɶ� */
  time_t lastlogin;             /* �W�@���W���ɶ� */
  time_t tcheck;                /* �W�� check �H�c/�B�ͦW�檺�ɶ� */
  time_t tvalid;                /* �Y���v�A���v�������ɶ��F
                                   �Y�����v�B�q�L�{�ҡA�q�L�{�Ҫ��ɶ��F
                                   �Y�����v�B���q�L�{�ҡA�{�Ҩ窺 time-seed */
} OLD;



/* ----------------------------------------------------- */
/* �ഫ�D�{��						 */
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
      unlink(buf);			/* itoc.010831: �屼��Ӫ� FN_ACCT */

      printf("%s\n", buf);
      trans_acct(&old, &new);

      fd = open(buf, O_WRONLY | O_CREAT, 0600);	/* itoc.010831: ���طs�� FN_ACCT */
      write(fd, &new, sizeof(NEW));
      close(fd);
    }

    closedir(dirp);
  }

  printf("done. %d => %d\n", sizeof(OLD), sizeof(NEW));
  return 0;
}


