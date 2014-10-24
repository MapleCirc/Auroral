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
/* �{�ҥΨ禡						 */
/* ----------------------------------------------------- */


void
justify_log(userid, justify)	/* itoc.010822: ���� .ACCT �� justify �o���A��O���b FN_JUSTIFY */
  char *userid;
  char *justify;	/* �{�Ҹ�� RPY:email-reply  KEY:�{�ҽX  POP:pop3�{��  REG:���U�� */
{
  char fpath[64];
  FILE *fp;

  usr_fpath(fpath, userid, FN_JUSTIFY);
  if (fp = fopen(fpath, "a"))		/* �Ϊ��[�ɮסA�i�H�O�s�����{�ҰO�� */
  {
    fprintf(fp, "%s\n", justify);
    fclose(fp);
  }
}


static int
ban_addr(addr)
  char *addr;
{
  char *host;
  char foo[128];	/* SoC: ��m���ˬd�� email address */

  /* Thor.991112: �O���Ψӻ{�Ҫ�email */
  sprintf(foo, "%s # %s (%s)\n", addr, cuser.userid, Now());
  f_cat(FN_RUN_EMAILREG, foo);

  /* SoC: �O���� email ���j�p�g */
  str_lower(foo, addr);

  /* check for acl (lower case filter) */

  host = (char *) strchr(foo, '@');
  *host = '\0';

  /* *.bbs@xx.yy.zz�B*.brd@xx.yy.zz �@�ߤ����� */
  if (host > foo + 4 && (!str_cmp(host - 4, ".bbs") || !str_cmp(host - 4, ".brd")))
    return 1;

  /* ���b�զW��W�Φb�¦W��W */
  return (!acl_has(TRUST_ACLFILE, foo, host + 1) ||
    acl_has(UNTRUST_ACLFILE, foo, host + 1) > 0);
}


/* ----------------------------------------------------- */
/* POP3 �{��						 */
/* ----------------------------------------------------- */


#ifdef HAVE_POP3_CHECK

static int		/* >=0:socket -1:�s�u���� */
Get_Socket(site)	/* site for hostname */
  char *site;
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


static int		/* 0:���\ */
POP3_Check(sock, account, passwd)
  int sock;
  char *account, *passwd;
{
  FILE *fsock;
  char buf[512];

  if (!(fsock = fdopen(sock, "r+")))
  {
    outs("\n�Ǧ^���~�ȡA�Э��մX���ݬ�\n");
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
      outs("\n���ݨt�ζǦ^���~�T���p�U�G\n");
      prints("%s\n", buf);
      sock = -1;
    }
  }
}


static int		/* -1:���䴩 0:�K�X���~ 1:���\ */
do_pop3(addr)		/* itoc.010821: ��g�@�U :) */
  char *addr;
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
  prints("�D��: %s\n�b��: %s\n", ptr, username);
  outs("\033[1;5;36m�s�u���ݥD����...�еy��\033[m\n");
  refresh();

  for (i = 0; str = alias[i]; i++)
  {
    sprintf(buf, "%s%s", str, ptr);	/* itoc.020120: �D���W�٥[�W pop3. �ոլ� */
    if ((sock = Get_Socket(buf)) >= 0)	/* ���o�����B���䴩 POP3 */
      break;
  }

  if (sock < 0)
  {
    outs("�z���q�l�l��t�Τ��䴩 POP3 �{�ҡA�ϥλ{�ҫH�稭���T�{\n\n\033[1;36;5m�t�ΰe�H��...\033[m");
    return -1;
  }

  if (vget(15, 0, "�п�J�H�W�ҦC�X���u�@���b�����K�X�G", buf, 20, NOECHO))
  {
    move(17, 0);
    outs("\033[5;37m�����T�{��...�еy��\033[m\n");

    if (!POP3_Check(sock, username, buf))	/* POP3 �{�Ҧ��\ */
    {
      /* �����v�� */
      sprintf(buf, "POP: %s", addr);
      justify_log(cuser.userid, buf);
      strcpy(cuser.email, addr);
      if (acct_load(&acct, cuser.userid) >= 0)
      {
	time(&acct.tvalid);
	acct_setperm(&acct, PERM_VALID, 0);
      }

      /* �H�H�q���ϥΪ� */
      mail_self(FN_ETC_JUSTIFIED, str_sysop, msg_reg_valid, 0);
      cutmp->status |= STATUS_BIFF;
      vmsg(msg_reg_valid);

      close(sock);
      return 1;
    }
  }

  close(sock);

  /* POP3 �{�ҥ��� */
  outs("�z���K�X�γ\\�����F�A�ϥλ{�ҫH�稭���T�{\n\n\033[1;36;5m�t�ΰe�H��...\033[m");
  return 0;
}
#endif


/* ----------------------------------------------------- */
/* �]�w E-mail address					 */
/* ----------------------------------------------------- */


int
u_addr()
{
  char *msg, addr[64];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  /* itoc.050405: ���������v�̭��s�{�ҡA�]���|�ﱼ�L�� tvalid (���v����ɶ�) */
  if (HAS_PERM(PERM_ALLDENY))
  {
    vmsg("�z�Q���v�A�L�k��H�c");
    return XEASY;
  }

  film_out(FILM_EMAIL, 0);

  if (vget(b_lines - 2, 0, "E-mail �a�}�G", addr, sizeof(cuser.email), DOECHO))
  {
    if (not_addr(addr))
    {
      msg = err_email;
    }
    else if (ban_addr(addr))
    {
      msg = "�����������z���H�c�����{�Ҧa�}";
    }
    else
    {
#ifdef EMAIL_JUSTIFY
      if (vans("�ק� E-mail �n���s�{�ҡA�T�w�n�ק��(Y/N)�H[Y] ") == 'n')
	return 0;

#  ifdef HAVE_POP3_CHECK
      if (vans("�O�_�ϥ� POP3 �{��(Y/N)�H[N] ") == 'y')
      {
	if (do_pop3(addr) > 0)	/* �Y POP3 �{�Ҧ��\�A�h���}�A�_�h�H�{�ҫH�H�X */
	  return 0;
      }
#  endif

#error dust.101220: this code have not adapted to new version yet.
      if (bsmtp(NULL, NULL, addr, MQ_JUSTIFY) < 0)
      {
	msg = "�����{�ҫH��L�k�H�X�A�Х��T��g E-mail address";
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
	prints("\n%s(%s)�z�n�A�ѩ�z��s E-mail address ���]�w�A\n\n"
	  "�бz���֨� \033[44m%s\033[m �Ҧb���u�@���^�Сy�����{�ҫH��z�C",
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
/* ��g���U��						 */
/* ----------------------------------------------------- */


#ifdef HAVE_REGISTER_FORM

static void
getfield(line, len, buf, desc, hint, ex)
  int line, len;
  char *buf, *desc, *hint, *ex;
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
u_register()
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
    zmsg("�z�������T�{�w�g�����A���ݶ�g�ӽЪ�");
    return XEASY;
  }

#ifdef USE_DAVY_REGISTER
  sprintf(regpath, "reg/%s", cuser.userid);
  str_lower(regpath, regpath);
  if (dashf(regpath))
  {
    zmsg("�z�����U�ӽг�|�b�B�z���A�Э@�ߵ���");
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
	zmsg("�z�����U�ӽг�|�b�B�z���A�Э@�ߵ���");
	return XEASY;
      }
    }
    fclose(fn);
  }
#endif

  if (vans("�z�T�w�n��g���U���(Y/N)�H[N] ") != 'y')
    return XEASY;

  move(1, 0);
  clrtobot();
  prints("\n%s(%s) �z�n�A�оڹ��g�H�U����ơG\n(�� Enter�� ������J)",
    cuser.userid, cuser.username);

  memset(&rform, 0, sizeof(RFORM));
  if(acct_load(&acct, cuser.userid) < 0)
    return 0;

#define hint0 "�o�䴣�ѭק諸���|�A�p�G�u��m�W�ö񪺸ܵ��U��|�Q�h��C"
#define hint1 "\033[1m�Ǯ�+�t�O+�ż�\033[m(����X�~���~�A\033[1;31m�Ф��n��~��\033[m) >�� \033[1m���+¾��\033[m"
#define hint2 "�]�A��Ǹ��X�Ϊ��P���X�C�`���ХH\033[1;32m�H���H�F\033[m����h�Ӷ�g�C"
#define hint3 "�n�]�t\033[1;33m���~�������ϰ�X\033[m�C"

#define ex1 "\033[1;30m�d�ҡG[\033[37m�x���@��99��\033[30m] or [\033[37m�x�j��u�t102��\033[30m] or [\033[37m�x�W�L�n�����\033[30m]\033[m"
#define ex2 "\033[1;30m�d�ҡG[\033[37m�x�����|�~��3��\033[30m] or [\033[37m�x���@���Ĥ@�J��102��\033[30m]\033[m"
#define ex3 "\033[1;30m�d�ҡG[\033[37m(04)22226081\033[30m] or [\033[37m0912345678\033[30m]\033[m"

  for (;;)
  {
    getfield(6, RNLEN + 1, acct.realname, "�u��m�W�G", hint0, NULL);
    getfield(10, 50, rform.career, "�A�ȳ��G", hint1, ex1);
    getfield(14, 60, rform.address, "�ثe��}�G", hint2, ex2);
    getfield(18, 20, rform.phone, "�p���q�ܡG", hint3, ex3);

    ans = vans("�H�W��ƬO�_���T(Y/N/Q)�H[N] ");
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

  /* �g�i�ϥΪ̰������U������ */
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
/* ��g���{�X						 */
/* ----------------------------------------------------- */


#ifdef HAVE_REGKEY_CHECK
int
u_verify()
{
  char buf[80], key[10];
  ACCT acct;
  
  if (HAS_PERM(PERM_VALID))
  {
    zmsg("�z�������T�{�w�g�����A���ݶ�g�{�ҽX");
  }
  else
  {
    if (vget(b_lines, 0, "�п�J�{�ҽX�G", buf, 8, DOECHO))
    {
      archiv32(str_hash(cuser.email, cuser.tvalid), key);	/* itoc.010825: ���ζ}�ɤF�A������ tvalid �Ӥ�N�O�F */

      if (str_ncmp(key, buf, 7))
      {
	zmsg("��p�A�z���{�ҽX���~");      
      }
      else
      {
	/* �����v�� */
	sprintf(buf, "KEY: %s", cuser.email);
	justify_log(cuser.userid, buf);
	if (acct_load(&acct, cuser.userid) >= 0)
	{
	  time(&acct.tvalid);
	  acct_setperm(&acct, PERM_VALID, 0);
	}

	/* �H�H�q���ϥΪ� */
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
/* ��_�v��						 */
/* ----------------------------------------------------- */


int
u_deny()
{
  ACCT acct;
  time_t diff;
  struct tm *ptime;
  char msg[80];

  if (!HAS_PERM(PERM_ALLDENY))
  {
    zmsg("�z�S�Q���v�A���ݴ_�v");
  }
  else
  {
    if ((diff = cuser.tvalid - time(0)) < 0)      /* ���v�ɶ���F */
    {
      if (acct_load(&acct, cuser.userid) >= 0)
      {
	time(&acct.tvalid);
#ifdef JUSTIFY_PERIODICAL
	/* xeon.050112: �b�{�ҧ֨���e�� Cross-Post�A�M�� tvalid �N�|�Q�]�w�쥼�Ӯɶ��A
	   ���_�v�ɶ���F�h�_�v�A�o�˴N�i�H�׹L���s�{�ҡA�ҥH�_�v��n���s�{�ҡC */
	acct_setperm(&acct, 0, PERM_ALLVALID | PERM_ALLDENY);
#else
	acct_setperm(&acct, 0, PERM_ALLDENY);
#endif
	vmsg("�U���ФŦA�ǡA�Э��s�W��");
      }
    }
    else
    {
      ptime = gmtime(&diff);
      sprintf(msg, "�z�٭n�� %d �~ %d �� %d �� %d �� %d ��~��ӽд_�v",
	ptime->tm_year - 70, ptime->tm_yday, ptime->tm_hour, ptime->tm_min, ptime->tm_sec);
      vmsg(msg);
    }
  }

  return XEASY;
}


/* ----------------------------------------------------- */
/* �ӤH�u��						 */
/* ----------------------------------------------------- */


int
u_info()
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
u_setup()
{
#if 0
  usint ulevel;
  int len;

  /* itoc.000320: �W��حn��� len �j�p, �]�O�ѤF�� ufo.h ���X�� STR_UFO */

  ulevel = cuser.userlevel;
  if (!ulevel)
    len = NUMUFOS_GUEST;
  else if (ulevel & PERM_ALLADMIN)
    len = NUMUFOS;		/* ADMIN ���F�i�� acl�A�]�i�H�Τ@�몺�����N */
  else if (ulevel & PERM_VALID)
    len = NUMUFOS - 2;		/* �q�L�{�Ҫ� */
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
u_lock()
{
  char buf[PSWDLEN + 1];
  char *menu[] = {"y��w�ù�", "c��w�ù�-�ۭq���A", "n����", NULL};

  switch (krans(b_lines, "yn", menu, NULL))
  {
  case 'c':		/* itoc.011226: �i�ۦ��J�o�b���z�� */
    if (vget(b_lines, 0, "�o�b���z�ѡH ", cutmp->mateid, 9, DOECHO))
      break;

  case 'y':
    strcpy(cutmp->mateid, "����");
    break;

  default:
    return XEASY;
  }

  utmp_mode(M_IDLE);

  bbstate |= STAT_LOCK;		/* lkchu.990513: ��w�ɤ��i�^���y */
  cutmp->status |= STATUS_REJECT;	/* ��w�ɤ������y */

  lim_cat("etc/locked_screen", 18);
  move(15, 47);
  prints("\033[1;35m<%s>\033[m", cutmp->mateid);

  do
  {
    vget(b_lines, 0, "\033[1;33m��\033[m �п�J�z���K�X�H�Ѱ��ù���w���A�G", buf, PSWDLEN + 1, NOECHO);
  } while (chkpasswd(cuser.passwd, buf));

  cutmp->status ^= STATUS_REJECT;
  bbstate ^= STAT_LOCK;

  return 0;
}


int
u_log()
{
  char fpath[64];

  usr_fpath(fpath, cuser.userid, FN_LOG);
  more(fpath, MFST_NORMAL);
  return 0;
}


/* ----------------------------------------------------- */
/* �]�w�ɮ�						 */
/* ----------------------------------------------------- */

#define LFSX 0
#define RFSX 39
#define MAXLINES 9
#define MAXITEM 18
#define XFILE_FEET "\033[1;33m�� \033[m�Ы� \033[1;33m��V�� \033[m���ʴ�СA \033[1;33mEnter \033[m�s��A \033[1;33mdelete \033[m�R���ɮש� \033[1;33mQ \033[m���}�C\n"

void
x_file(echo_path, xlist, flist, fpath)
  int echo_path;
  char *xlist[];		/* description list */
  char *flist[];		/* filename list */
  char *fpath;
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
    vmsg("xlist[] �]�w���~�I");
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
      oc = col * MAXLINES + row;	/* �ɥ�oc */

      if(*flist[oc] == '\033')	/* Dynamic Link Function */
	DL_func(flist[oc] + 1);
      else
      {
	strcpy(fpath, flist[oc]);
	if(vedit(pbuf, 0) < 0)
	  vmsg("��ʤ���");
	else
	  vmsg("��s����");
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
      oc = col * MAXLINES + row;	/* �ɥ�oc */
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
u_xfile()
{
  int i;
  char fpath[64];

  static char *desc[] = 
  {
    "�W���a�I�]�w��",
    "�W����",
    "�s��/�]�wñ�W��",
    "�Ȧs��.1",
    "�Ȧs��.2",
    "�Ȧs��.3",
    "�Ȧs��.4",
    "�Ȧs��.5",
    "�]�w�˼ƭp�ɾ�",
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

