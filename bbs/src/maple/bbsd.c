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

/* static int mport; */ /* Thor.990325: ���ݭn�F:P */
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
/* ���} BBS �{��					 */
/* ----------------------------------------------------- */


char*
SysopLoginID_query(id)	/* �O���άd��sysop���n�J�� */
  char *id;		/* NULL:�^��ID , �DNULL:�]�wID */
{
  static char loginID[IDLEN + 1];
  if(id)
    strcpy(loginID, id);
  return loginID;
}


void
circlog(mode, msg)         /* circ god �欰���� */
  char *mode, *msg;
{
  char buf[512];

  /* floatJ.091109: circlog�]�|�bsysop�n�J�ק�l��W��ɳQ�I�s�A�ҥH�]�n�����Hsysop�n�J���u��ID */
  if(HAS_PERM(PERM_SpSYSOP))
    sprintf(buf, "%s %-12s:%s %s\n", Now(), SysopLoginID_query(NULL), mode, msg);
  else
    sprintf(buf, "%s %-12s:%s %s\n", Now(), cuser.userid, mode, msg);

  f_cat(FN_RUN_CIRCGOD, buf);
}


void
slog(mode, msg)		/* sysop �欰���� */
  char *mode, *msg;
{
  char buf[512];

  sprintf(buf, "%s %-12s:%s %s\n", Now(), SysopLoginID_query(NULL), mode, msg);
  f_cat(FN_RUN_SYSOP, buf);
}


void
alog(mode, msg)		/* Admin �欰�O�� */
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
blog(mode, msg)		/* BBS �@��O�� */
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

  mantime_add(currbno, -1);	/* �h�X�̫�ݪ����ӪO */

  utmp_free(cutmp);		/* ���� UTMP shm */

  diff = (time(&cuser.lastlogin) - ap_start) / 60;
  sprintf(fpath, "Stay: %d (%d)", diff, currpid);
  blog(mode, fpath);

  if (cuser.userlevel)
  {
    ve_backup();		/* �s�边�۰ʳƥ� */
    rhs_save();			/* �x�s�\Ū�O���� */
    pmore_saveBP();
  }

#ifndef LOG_BMW	/* �����R�����y */
  usr_fpath(fpath, cuser.userid, fn_amw);
  unlink(fpath);
  usr_fpath(fpath, cuser.userid, fn_bmw);
  unlink(fpath);
#endif

#ifdef MODE_STAT
  log_modes();
#endif


  /* �g�^ .ACCT */

  if (!HAS_STATUS(STATUS_DATALOCK))	/* itoc.010811: �S���Q������w�A�~�i�H�^�s .ACCT */
  {
    usr_fpath(fpath, cuser.userid, fn_acct);
    fd = open(fpath, O_RDWR);
    if (fd >= 0)
    {  
      if (read(fd, &tuser, sizeof(ACCT)) == sizeof(ACCT))
      {
	if (diff >= 1)
	{
	  cuser.numlogins++;	/* Thor.980727.����: �b���W���W�L�@���������p�⦸�� */
	  addmoney(diff/4);	/* dust.091216: �W���ɶ� �|�����[�@�� */
	}

	if (HAS_STATUS(STATUS_COINLOCK))	/* itoc.010831: �Y�O multi-login ���ĤG���H��A���x�s���� */
	{
	  cuser.money = tuser.money;
	  cuser.gold = tuser.gold;
	}

	/* itoc.010811.����: �p�G�ϥΪ̦b�u�W�S���{�Ҫ��ܡA
	  ���� cuser �� tuser �� userlevel/tvalid �O�P�B���F
	  ���Y�ϥΪ̦b�u�W�^�{�ҫH/��{�ҽX/�Q�����f�ֵ��U��..���{�ҳq�L���ܡA
	  ���� tuser �� userlevel/tvalid �~�O����s�� */
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


/* Thor.980903: lkchu patch: ���ϥΤW���ӽбb����, �h�U�C function������ */

#ifdef LOGINASNEW

/* ----------------------------------------------------- */
/* �ˬd user ���U���p					 */
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
uniq_userno(fd)		/* dust.121014 remake: ���A���ƨϥ�userno */
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

    /* .USR���̫�@���@�w�O��e�o�X�h��UNO���̤j���A�L��ID��즳�S���F�� */
    return sp.userno + 1;
  }
  else
    return 1;	/* .USR�O�Ū��A�q 1 �}�l */
}


static void
acct_apply()
{
  SCHEMA slot;
  char buf[80];
  char *userid = cuser.userid;
  int try = 0, fd;
  char *gender_menu[] = {"0����", "1�k��", "2�k��", NULL};
  char *commit_menu[] = {"y�O�A�e��I", "n�_�A�ڭn�ק�", "q�������U", NULL};
  char year[8] = "", month[4] = "", day[4] = "", gender_def[4];

  memset(&cuser, 0, sizeof(ACCT));


  vs_bar(BBSNAME "�s�b�����U�t��");

  move(11, 0);
  outs("ID�O�C�ӨϥΪ̿W�@�L�G���N���A�@�������ѧO�ΡC\n");
  outs("���׬� \033[1;36m2~12\033[m �Ӧr���A�Ĥ@�Ӧr�ݬ�\033[1;32m�^��r��\033[m�A��L�h�i�H�O\033[1;33m�^��r��\033[m��\033[1;33m�Ʀr\033[m�C\n");
  outs("�j�p�g�|�Q�����ۦP�A�Ҧp���FNPC�N������UNpc�A�b�n�J��J�b���ɤ]�i�����ޤj�p�g\n");
  outs("���b��ܱz��ID�ɡA�j�p�g���t�O���M�|�X�{�A�ä��|�Q�Τ@�ন�j�g�Τp�g�C\n");

  for(;;)
  {
    if(!vget(16, 0, "�п�J�z�Q�Ϊ�ID�G", userid, IDLEN + 1, DOECHO))
      login_abort("\n�A�� ...");

    if(is_badid(userid))
    {
      vmsg("�L�k�����o�ӥN���A�Шϥέ^��r���A�ӥB����t���Ů�");
    }
    else
    {
      usr_fpath(buf, userid, NULL);
      if(dashd(buf))
	vmsg("���N���w�g���H�ϥ�");
      else
	break;
    }

    if(++try >= 10)
      login_abort("\n�z���տ��~�Ӧh���A�ФU���A�ӧa");
  }

  move(2, 0);
  prints("ID�G%s", userid);

  clrregion(11, 16);


  move(13, 0);
  outs("�K�X���צܤ֬�4�Ӧr���C������զ����r�C\n");
  outs("\033[1;31m���F�w���ʦҶq�A��ĳ���n�PID�ۦ��A�åB�O���e���Q�q�����K�X(�Ҧp�^��P�Ʀr�V�X)\033[m\n");

  for(;;)
  {
    char check[PSWDLEN + 1];

    vget(16, 0, "�п�J�K�X�G", buf, PSWDLEN + 1, NOECHO);
    if(strlen(buf) < 4)
    {
      vmsg("�K�X���צܤ֭n4�Ӧr���A�Э��s��J");
      continue;
    }

    /* dust.101231: �����ˬd�ɧ令�Y����J�h�������s�M�w�K�X�A��K�Q�ק諸�H */
    if(!vget(17, 0, "�ЦA����J�H�T�{�K�X�L�~�G", check, PSWDLEN + 1, NOECHO))
    {
      clrregion(17, 17);
      continue;
    }

    if(!strcmp(buf, check))
      break;

    vmsg("�K�X��J���~�A�Э��s��J�K�X");
    clrregion(17, 17);
  }

  str_ncpy(cuser.passwd, genpasswd(buf), sizeof(cuser.passwd));

  move(3, 0);
  outs("�K�X�G");
  for(try = strlen(buf); try > 0; try--)
    outc('*');

  clrregion(11, 17);


  for(;;)
  {
    move(b_lines, 0);
    outs("�p���ܡG�z�i�H��\033[1;33m Ctrl+X \033[m�}��/�������A���ݪ�����r�����\\��C");


    move(14, 0);
    outs("���צܤ֭n1�Ӧr���A�N�@���z�b���W���ʮɪ��O�W�C");

    while(1)
    {
      if(vget(16, 0, "�ʺ١G", cuser.username, UNLEN + 1, GCARRY))
	break;
    }

    move(4, 0);
    prints("�ʺ١G%s", cuser.username);

    clrregion(14, 16);


    move(14, 0);
    outs("\033[1;31m�Ъ`�N�G\033[m���F����U��ɥi�H�ק�H�νЯ��ȭק�A��L�ɭԨèS����k�ק�m�W��ơC");

    do
    {
      vget(16, 0, "�u��m�W�G", cuser.realname, RNLEN + 1, GCARRY);
    } while(strlen(cuser.realname) < 2);

    move(5, 0);
    prints("�u��m�W�G%s\n", cuser.realname);

    clrtobot();	/* dust.110102: �⩳�ݪ��p���ܲM�� */


    move(14, 0);
    outs("�п�ܱz���ʧO�C");

    sprintf(gender_def, "%c%c", cuser.sex + '0', cuser.sex + '0');
    cuser.sex = krans(16, gender_def, gender_menu, "") - '0';
    move(6, 0);
    prints("�ʧO�G%2.2s", "�H�k�k" + cuser.sex * 2);

    clrregion(14, 16);


    move(13, 0);
    outs("�b�z�ͤ��ѤW���ɡA�t�η|��ܯ��P�ͤ骺�ϡC\n");
    outs("�ϥΪ̦W�椤�]�|�H\033[1;35m�S���C��\033[m��ܱz�O��骺�جP�C\n");

    while(1)
    {
      if(vget(16, 0, "�褸�~�G", year, 5, GCARRY))
	cuser.year = atoi(year);

      if(cuser.year >= 1)
	break;
      year[0] = '\0';
    }

    while(1)
    {
      if(vget(16, 22, "����G", month, 3, GCARRY))
	cuser.month = atoi(month);

      if(cuser.month >= 1 && cuser.month <= 12)
	break;
      month[0] = '\0';
    }

    while(1)
    {
      if(vget(16, 38, "��G", day, 3, GCARRY))
	cuser.day = atoi(day);

      if(cuser.day >= 1 && cuser.day <= 31)
	break;
      day[0] = '\0';
    }

    move(7, 0);
    prints("�ͤ�G%d/%02d/%02d", cuser.year, cuser.month, cuser.day);

    clrregion(13, 16);


    move(13, 0);
    outs("�ж�J�z���q�l�l��H�c�a�}�C�b��H�峹�쯸�~�B�ƥ���Ʈɷ|�H�o�Ӧa�}���w�]�C\n");
    outs("�Y����J����F��|�H BBS E-mail Address �@���w�]�ȡC\n");

    if(!vget(16, 0, "E-mail�G", cuser.email, sizeof(cuser.email), GCARRY))
      sprintf(cuser.email, "%s.bbs@%s", cuser.userid, str_host);

    move(8, 0);
    prints("E-mail�G%s", cuser.email);

    clrregion(13, 16);


    switch(krans(b_lines, "nn", commit_menu, "�H�W��ƬO�_���T�H "))
    {
    case 'n':
      sprintf(year, "%d", cuser.year);
      sprintf(month, "%d", cuser.month);
      sprintf(day, "%d", cuser.day);

      move(4, 0);
      clrtobot();
      continue;

    case 'q':
      login_abort("\n�A�� ...\n");
    }

    break;
  }

  cuser.userlevel = PERM_DEFAULT;
  cuser.ufo = UFO_DEFAULT_NEW;
  cuser.sig_avai = 6;			/* dust.100330: �i��ñ�W�ɼƶq�w�]��6 */
  cuser.numlogins = 1;
  cuser.tvalid = ap_start;		/* itoc.030724: ���W���ɶ���Ĥ@���{�ҽX�� seed */

  /* Ragnarok.050528: �i��G�H�P�ɥӽЦP�@�� ID�A�b�������A�ˬd�@�� */
  usr_fpath(buf, userid, NULL);
  if(dashd(buf))
  {
    vmsg("���N����Q���U���A�Э��s�ӽ�");
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
    vmsg("�z�ӽЮɵ��U�H�Ƥw�F�W��");
    login_abort("");
  }

  /* create directory */

  mkdir(buf, 0700);
  strcat(buf, "/@");
  mkdir(buf, 0700);
  usr_fpath(buf, userid, "gem");	/* itoc.010727: �ӤH��ذ� */
  mak_links(buf);
#ifdef MY_FAVORITE
  usr_fpath(buf, userid, "MF");
  mkdir(buf, 0700);
#endif

  usr_fpath(buf, userid, fn_acct);
  fd = open(buf, O_WRONLY | O_CREAT, 0600);
  write(fd, &cuser, sizeof(ACCT));
  close(fd);

  sprintf(buf, "%d", try);	/* �O���s�b����UNO */
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
/* �n�� BBS �{��					 */
/* ----------------------------------------------------- */


extern void talk_rqst();
extern void bmw_rqst();


#ifdef HAVE_WHERE
int	/* 1:�blist��  0:���blist�� */
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
  /* dust.101005: ����o�Ө�Ʈɥ~�Y�w�g��Mutex�F�A���ݭn�A�� */
  char name[48];
  uschar *addr = (uschar*) &tn_addr;

  /* ����� id mapping */
  str_lower(name, cuser.userid);
  if (!belong_list(FN_ETC_IDHOME, name, from, szfrom))
  {
    /* �A��� FQDN */
    str_lower(name, fromhost);		/* itoc.011011: �j�p�g���i�Aetc/fqdn �̭����n�g�p�g */
    if (!belong_list(FN_ETC_FQDN, name, from, szfrom))
    {
      /* �A��� ip */
      sprintf(name, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
      if (!belong_list(FN_ETC_HOST, name, from, szfrom))
	if (cuser.ufo2 & UFO2_HIDEMYIP)
	  str_ncpy(from, "���a�B��", szfrom); /* davy.110928: �p�G���]�m����IP�A�N�令"���a�B��" */
	else
	  str_ncpy(from, fromhost, szfrom);	/*  �p�G���S�������G�m�A�N�O�� fromhost */
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
  /* utmp.in_addr = tn_addr; */ /* itoc.010112: ����umtp.in_addr�H��ulist_cmp_host���` */
  addr = (uschar *) &tn_addr;
  utmp.in_addr = (addr[0] << 24) + (addr[1] << 16) + (addr[2] << 8) + addr[3];
  utmp.userlevel = cuser.userlevel;	/* itoc.010309: �� userlevel �]��J cache */
  utmp.ufo = cuser.ufo;
  utmp.status = 0;
  utmp.talker = -1;
  
  strcpy(utmp.userid, cuser.userid);
  utmp.label_attr = cuser.label_attr;
  strcpy(utmp.label, cuser.label);

  pthread_mutex_lock(&rslv_mutex);

  str_ncpy(utmp.fromhost, fromhost, sizeof(utmp.fromhost));
  if (utmp.ufo & UFO_CLOAK) /* davy.110928: ��������s�X�{�ɶ� */
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
		/* dust.101005: ���rand()���ü� */
    sprintf(cuser.username, "�B��W��%s", nick[rand() % GUEST_NICK_LIST_MAX]);
  }
#endif	/* GUEST_NICK */

  strcpy(utmp.username, cuser.username);
  
#ifdef HAVE_WHERE

#  ifdef GUEST_WHERE
  if (!cuser.userlevel)		/* guest */
  {
    char *from[GUEST_FROM_LIST_MAX] = GUEST_FROM_LIST;
		/* dust.101005: ���rand()���ü� */
    strcpy(utmp.from, from[rand() % GUEST_FROM_LIST_MAX]);
  }
  else
#  endif	/* GUEST_WHERE */
    find_fromwhere(utmp.from, sizeof(utmp.from));

#else
  str_ncpy(utmp.from, fromhost, sizeof(utmp.from));
#endif	/* HAVE_WHERE */

  /* Thor: �i�DUser�w�g���F�񤣤U... */
  if (!utmp_new(&utmp))
    login_abort("\n�z���諸��l�w�g�Q�H�������n�F�A�ФU���A�ӧa");

  pthread_mutex_unlock(&rslv_mutex);

  /* itoc.001223: utmp_new ���A pal_cache�A�p�G login_abort �N�����F */
  pal_cache();
}


/* ----------------------------------------------------- */
/* user login						 */
/* ----------------------------------------------------- */


static int		/* �^�� multi */
login_user(content, luser)
  char *content;
  ACCT *luser;		/* ��sysop�n�J�� */
{
  int attempts;		/* ���մX�����~ */
  int multi;
  char fpath[64], uid[IDLEN + 1];
#ifndef CHAT_SECURE
  char passbuf[PSWDLEN + 1];
#endif

  move(b_lines, 0);

#if 1
  /* floatJ: ��� FILM_LOGINMSG (�i���ܪ��w��T��) */
  film_out(FILM_LOGINMSG, b_lines);
#else
  outs(" \033[1;33m��\033[m �ХH\ \033[1;36m" STR_GUEST "\033[m ���[�A�ΥH \033[1;35m" STR_NEW "\033[m ���U�C");
#endif

  attempts = 0;
  multi = 0;
  for (;;)
  {
    if (++attempts > LOGINATTEMPTS)
    {
      film_out(FILM_TRYOUT, 0);
      login_abort("\n�A�� ...");
    }

    /* floatJ: �]����Login����令GCARRY, �ҥH�n����l�� uid �A�_�h�@�}�l�|�O�ýX */
    uid[0]='\0';

/* floatJ: �� ofo �@�˧�y�{�A�|�Ψ� Passwd �o�� goto ����  */
Passwd:

    /* floatJ: �p�ߨ��Ӱ��b��X ... �����qpietty�ƻs�νs��|�X���A��ĳ�q�O�ƥ��ƻs */
    vget(b_lines -2, 0, "    Login ��  ", uid, IDLEN + 1, GCARRY);

    if (!str_cmp(uid, STR_NEW))
    {
#ifdef LOGINASNEW
      acct_apply(); /* Thor.980917.����: cuser setup ok */
      logattempt(' ', content);
      break;
#else
      outs("\n���t�Υثe�Ȱ��u�W���U, �Х� " STR_GUEST " �i�J");
      continue;
#endif
    }
    else if (!*uid)
    {
      attempts--; /* �Y�S��J ID�A���� continue�A�åB����btryout�� */
    }
    else if (str_cmp(uid, STR_GUEST))	/* �@��ϥΪ� */
    {
      usr_fpath(fpath, uid, fn_acct);

      if (!vget(b_lines - 2, 0, " Password ��  "  , passbuf, PSWDLEN +  5, NOECHO))
      {
        *uid = 0;
	attempts--;
	continue;	/* �����K�X�h�����n�J,�B����Jtryout */
      }

      /* itoc.040110: �b��J�� ID �αK�X�A�~���J .ACCT */
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
        if (++attempts > LOGINATTEMPTS)	/* �ɴ�attempts */
        {
	  film_out(FILM_TRYOUT, 0);
	  login_abort("\n�A�� ...");
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
	    if(!vget(b_lines - 2, 0, "�п�J���ȱb���G", uid, IDLEN+1, DOECHO) && !strcmp(uid, str_sysop))
	    {
	      *uid = '\0';
	      goto Passwd;
	    }

	    if(acct_load(luser, uid)<0)	/* ID���s�b�����p */
	    {
	      outf(ERR_UID_ANSI);
	      continue;
	    }
	    if(!(luser->userlevel & PERM_SYSOP))
	    {
	      outf("�o���O�֦������v�����b���C");
	      continue;
	    }
	    else if(!strcmp(luser->userid, str_sysop))  /* �����sysop�n�O */
	      continue;

	    if(!vget(b_lines - 2, 40, "�K�X�G", passbuf, PSWDLEN+1, NOECHO))
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
	  /* ²�檺 SU �\�� */
	  if (vans("�ܧ�ϥΪ̨���(Y/N)�H[N] ") == 'y')
	  {
	    for (;;)
	    {
	      if (vget(b_lines - 2, 0, "   [�ܧ�b��] ", uid, IDLEN + 1, DOECHO) && acct_load(&cuser, uid) >= 0)
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
	  /* �Dsysop��ID�j��� PERM_SpSYSOP �M PERM_SEECLOAK */
	  cuser.userlevel &= ~(PERM_SpSYSOP | PERM_SEECLOAK);

	if (cuser.ufo & UFO_ACL)
	{
	  uschar *addr = (uschar*) &tn_addr;
	  char ipaddr[16];
	  sprintf(ipaddr, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);

	  usr_fpath(fpath, cuser.userid, FN_ACL);
	  /* dust.101005: ACL�o�تF���٬O��IP�|����n�a? */
	  if (!acl_has(fpath, "", ipaddr))
	  {	/* Thor.980728: �`�N acl �ɤ��n�����p�g */
	    logattempt('-', content);
	    vmsg("�z���W���a�I���ӹ�l�A�Э��s�ֹ�acl�C");
	    login_abort("\n�z���W���a�I���ӹ�l�A�Юֹ� [�W���a�I�]�w��]");
	  }
	}

	logattempt(' ', content);

	/* check for multi-session */

	if (!HAS_PERM(PERM_ALLADMIN))
	{
	  UTMP *ui;
	  pid_t pid;

	  if (HAS_PERM(PERM_DENYLOGIN | PERM_PURGE))
	    login_abort("\n�o�ӱb���Ȱ��A�ȡA�Ա��ЦV���Ȭ��ߡC");

	  if (!(ui = (UTMP *) utmp_find(cuser.userno)))
	    break;		/* user isn't logged in */

	  pid = ui->pid;
	  if (pid && vans("�z�Q�𱼨�L���Ƶn�J���b�� (Y/N) �ܡH[Y] ") != 'n' && pid == ui->pid)
	  {
	    if ((kill(pid, SIGTERM) == -1) && (errno == ESRCH))
	      utmp_free(ui);
	    else
	      sleep(3);			/* �Q�𪺤H�o�ɭԥ��b�ۧڤF�_ */
	    blog("MULTI", cuser.userid);
	  }

	  /* �u�W�w�� MULTI_MAX ���ۤv�A�T��n�J */
	  if ((multi = utmp_count(cuser.userno, 0)) >= MULTI_MAX || (!multi && acct_load(&cuser, uid) < 0))
	  /* yiting.050101: �Y��w�𱼩Ҧ� multi-login�A���򭫷sŪ���H�M���ܧ� */
	  {
	    vmsg("���Ƶn�J�L�h�A�L�k�n�J");
	    login_abort("\n�T�} ~��");
	  }
	}
	else	/* dust: �������Ƶn�J�ɥi�H��ܽ� */
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

	      /* dust.100306: ����habit �߰ݬO�_�𰣭���login */
	      sprintf(fpath, "�n�𱼭��Ƶn�J���b���ܡH(Y/N) [%c] ", (cuser.ufo & UFO_KQDEFYES)? 'Y' : 'N');
	      c = vans(fpath);

	      if(c != 'y' && c != 'n')
		c = (cuser.ufo & UFO_KQDEFYES)? 'y' : 'n';

	      if (c == 'y' && pid == ui->pid)
	      {
		if ((kill(pid, SIGTERM) == -1) && (errno == ESRCH))
		  utmp_free(ui);
		else
		  sleep(3);		/* �Q�𪺤H�o�ɭԥ��b�ۧڤF�_ */
		blog("MULTI", cuser.userid);
	      }
	    }
	  }

	}
    /* --------------------------- */
	break;
      }
    }
    else	/* �O�ϥ�guest�n�J */
    {
      if (acct_load(&cuser, uid) < 0)
      {
	outf("���~�Gguest�ä��s�b");
	continue;
      }

      if((multi = utmp_count(cuser.userno, 0)) >= GUEST_MULTI_MAX)
      {
        vmsg("guest�n�J�H�ƹL�h�A�L�k�n�J�C");
        login_abort("\nguest�n�J�H�ƹL�h�A�L�k�n�J......");
      }

      logattempt(' ', content);
      cuser.userlevel = 0;	/* Thor.981207: �ȤH�ê�, �j��g�^cuser.userlevel */
      cuser.ufo = UFO_DEFAULT_GUEST;
      break;	/* Thor.980917.����: cuser setup ok */
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

  /* itoc.010804.����: �� PERM_VALID �̦۰ʵo�� PERM_POST PERM_PAGE PERM_CHAT */
  level = cuser.userlevel | (PERM_ALLVALID ^ PERM_VALID);

  if (!(level & PERM_ALLADMIN))
  {
#ifdef JUSTIFY_PERIODICAL
    if ((level & PERM_VALID) && (cuser.tvalid + VALID_PERIOD < ap_start))
    {
      level ^= PERM_VALID;
      /* itoc.011116: �D�ʵo�H�q���ϥΪ̡A�@���e�H�����D�|���|�ӯӪŶ� !? */
      mail_self(FN_ETC_REREG, str_sysop, "�z���{�Ҥw�g�L���A�Э��s�{��", 0);
    }
#endif

#ifdef NEWUSER_LIMIT
    /* �Y�Ϥw�g�q�L�{�ҡA�٬O�n���ߤT�� */
    if (ap_start - cuser.firstlogin < 3 * 86400)
      level &= ~PERM_POST;
#endif

    /* itoc.000520: ���g�����{��, �T�� post/chat/talk/write */
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
      /* itoc.010805.����: �o�����g�^ .ACCT �O���F���O�H Query �u�W�ϥΪ̮�
	 �X�{���W���ɶ�/�ӷ����T�A�H�Φ^�s���T�� userlvel */
      tuser.userlevel = level;
      if (cutmp->ufo & UFO_CLOAK) /* davy.110928: ��������s�X�{�ɶ� */
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

  /* itoc.010831: multi-login ���ĤG���[�W���i�ܰʿ������X�� */
  if (multi)
    status |= STATUS_COINLOCK;

  /* itoc.011022: �[�J�ͤ�X�� */
  ptime = localtime(&ap_start);
  if (cuser.day == ptime->tm_mday && cuser.month == ptime->tm_mon + 1)
    status |= STATUS_BIRTHDAY;

  /* �B�ͦW��P�B�B�M�z�L���H�� */
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
    status |= m_quota();		/* Thor.����: ��ƾ�z�]�֦��]�t BIFF check */
#endif
  }
#ifdef OVERDUE_MAILDEL
  else
#endif
    status |= m_query(cuser.userid);

  /* itoc.010924: �ˬd�ӤH��ذϬO�_�L�h */
#ifndef LINUX	/* �b Linux �U�o�ˬd�ǩǪ� */
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

  /* �R�����~�n�J�O�� */
  usr_fpath(fpath, cuser.userid, FN_BADLOGIN);
  if (more(fpath, MFST_NONE) >= 0 && vans("�H�W����J�K�X���~�ɪ��W���a�I�O���A�n�R����(Y/N)�H[Y] ") != 'n')
    unlink(fpath);

  if (!HAS_PERM(PERM_VALID))
    film_out(FILM_NOTIFY, -1);		/* �|���{�ҳq�� */
#ifdef JUSTIFY_PERIODICAL
  else if (!HAS_PERM(PERM_ALLADMIN) && (cuser.tvalid + VALID_PERIOD - INVALID_NOTICE_PERIOD < ap_start))
    film_out(FILM_REREG, -1);		/* ���Įɶ��O�� 10 �ѫe���Xĵ�i */
#endif

#ifdef NEWUSER_LIMIT
  if (ap_start - cuser.firstlogin < 3 * 86400)
    film_out(FILM_NEWUSER, -1);		/* �Y�Ϥw�g�q�L�{�ҡA�٬O�n���ߤT�� */
#endif

  status = cutmp->status;

#ifdef OVERDUE_MAILDEL
  if (status & STATUS_MQUOTA)
    film_out(FILM_MQUOTA, -1);		/* �L���H��Y�N�M��ĵ�i */
#endif

  if (status & STATUS_MAILOVER)
    film_out(FILM_MAILOVER, -1);	/* �H��L�h�αH�H�L�h */

  if (status & STATUS_MGEMOVER)
    film_out(FILM_MGEMOVER, -1);	/* itoc.010924: �ӤH��ذϹL�hĵ�i */

  if (status & STATUS_BIRTHDAY)
    film_out(FILM_BIRTHDAY, -1);	/* itoc.010415: �ͤ��ѤW���� special �w��e�� */

  ve_recover();				/* �W���_�u�A�s�边�^�s */
}


static void
tn_login()
{
  int multi;
  char buf[128];
  ACCT sysop_login;

  bbsmode = M_LOGIN;	/* itoc.020828: �H�K�L�[����J�� igetch �|�X�{ movie */

  /* --------------------------------------------------- */
  /* �n���t��						 */
  /* --------------------------------------------------- */

  /* Thor.990415: �O��ip, �ȥ��d���� */
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
    slog("�n�J", buf);
  }

  /* --------------------------------------------------- */
  /* ��l�� utmp�Bflag�Bmode�B�H�c			 */
  /* --------------------------------------------------- */

  bbstate = STAT_STARTED;	/* �i�J�t�ΥH��~�i�H�^���y */
  utmp_setup(M_LOGIN);		/* Thor.980917.����: cutmp, cutmp-> setup ok */
  total_user = ushm->count;	/* itoc.011027: ���i�ϥΪ̦W��e�A�ҩl�� total_user */

  mbox_main();

  hm_init();	/* dust.100315: ���J�ӤH�˼ƭp�ɾ� */

#ifdef MODE_STAT
  memset(&modelog, 0, sizeof(UMODELOG));
  mode_lastchange = ap_start;
#endif

  if (cuser.userlevel)		/* not guest */
  {
    /* ------------------------------------------------- */
    /* �ֹ� user level �ñN .ACCT �g�^			 */
    /* ------------------------------------------------- */

    /* itoc.030929: �b .ACCT �g�^�H�e�A���i�H������ vmsg(NULL) �� more(xxxx, NULL)
       �����F��A�o�˦p�G user �b vmsg(NULL) �ɦ^�{�ҫH�A�~���|�Q�g�^�� cuser �\�L */
    if (!(cuser.ufo & UFO_CLOAK)) /* davy.110928: ��������s�X�{�ɶ� */
      cuser.lastlogin = ap_start;
    pthread_mutex_lock(&rslv_mutex);
    str_ncpy(cuser.lasthost, fromhost, sizeof(cuser.lasthost));
    pthread_mutex_unlock(&rslv_mutex);

    login_level();

    /* ------------------------------------------------- */
    /* �]�w status					 */
    /* ------------------------------------------------- */

    login_status(multi);

    /* ------------------------------------------------- */
    /* �q�Ǹ�T						 */
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
    more("gem/@/@-day", MFST_NULL);	/* ����������D */
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
  brd_force();	/* itoc.000319: �j��\Ū���i�O */
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

  /* �b���ɥ� sigset_t act.sa_mask */
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
    login_abort("\n�Q�����󤣳Q�ͯ�����");
#endif

  time(&ap_start);

  /* ��ܳs�u���T�� */
  move(0, 0);
  outs("\033[1;30m tcirc.org:23 is now connecting......\033[m");
  refresh();
  /* ���קK�ӧ֬ݤ���T���A�ҥH��N���ө��� */
  /* dust.101004: �üƨM�w����ɶ�, 0.6sec ~ 1.0sec */
  srand(ap_start);	/* �u�n��set�@��... */
  i = 600000 + rand() % 400001;
  usleep(i);

  clear();
  total_user = ushm->count;
  film_out(FILM_OPENING0, 0);
  
  currpid = getpid();

  tn_signals();	/* Thor.980806: ��� tn_login�e, �H�K call in���|�Q�� */
  tn_login();

  board_main();
  gem_main();
#ifdef MY_FAVORITE
  mf_main();
#endif
  talk_main();

  tn_motd();

  if(cuser.userlevel)
    pmore_loadBP();	/* dust.091212: ���Jpmore���]�w */

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
/* �䴩�W�L 24 �C���e��					 */
/* ----------------------------------------------------- */


static void
term_init()
{
#if 0   /* fuse.030518: ���� */
  server�ݡG�A�|���ܦ�C�ƶܡH(TN_NAWS, Negotiate About Window Size)
  client���GYes, I do. (TNCH_DO)

  ����b�s�u�ɡA��TERM�ܤƦ�C�ƮɴN�|�o�X�G
  TNCH_IAC + TNCH_SB + TN_NAWS + ��ƦC�� + TNCH_IAC + TNCH_SE;
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

  /* �ݹ�� (telnet client) ���S���䴩���P���ù��e�� */
  send(0, svr, 3, 0);

  rset = 1;
  to.tv_sec = 1;
  to.tv_usec = 1;
  if (select(1, (fd_set *) & rset, NULL, NULL, &to) > 0)
    recv(0, buf, sizeof(buf), 0);

  rcv = NULL;
  if ((uschar) buf[0] == IAC && buf[2] == TELOPT_NAWS)
  {
    /* gslin: Unix �� telnet �靈�L�[ port �Ѽƪ��欰���Ӥ@�� */
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

    /* b_lines �ܤ֭n 23�A�̦h����W�L T_LINES - 1 */
    if (b_lines >= T_LINES)
      b_lines = T_LINES - 1;
    else if (b_lines < 23)
      b_lines = 23;
    /* b_cols �ܤ֭n 79�A�̦h����W�L T_COLS - 1 */
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
  int port; /* Thor.981206: �� 0 �N�� *�S���Ѽ�* , -1 �N�� -i (inetd) */
  /* Davy.171001: �� -2 �N�� -p (proxy mode) */
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
    /* mport = port; */ /* Thor.990325: ���ݭn�F:P */

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

  if (port == 0) /* Thor.981206: port 0 �N��S���Ѽ� */
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

  /* mport = port; */ /* Thor.990325: ���ݭn�F:P */
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

  /* act.sa_mask = 0; */ /* Thor.981105: �зǥΪk */
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

  if(cutmp)	/* dust.101005: UTMP�w�g��l�Ƨ���, update�� */
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
  /* Thor.981206: �� 0 �N�� *�S���Ѽ�*, -1 �N�� -i */
  /* Davy.171001: bbsd -p �N�� proxy �Ҧ��A���|�t�~ accept �s�u�A�ϥ� -2 */
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
      /* �ɥ� currtitle */
      sprintf(currtitle, "�ثe�u�W�H�� [%d] �H�A�t�ι��M�A�еy��A��\n", argc);
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

