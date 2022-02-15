/*-------------------------------------------------------*/
/* menu.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : menu/help/movie routines		 	 */
/* create : 95/03/29				 	 */
/* update : 10/11/02				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"



extern UCACHE *ushm;
extern FCACHE *fshm;


extern char **brd_levelbar;

/* floatJ.0090607: konami_counter �m�J��*/
static int konami_counter = 0;


static MCD_info curr_CDmeter;

void 
hm_init (void)	/* dust.100315: ���J�ӤH�˼ƭp�ɾ� */
{
  FILE *fp;
  char fpath[64];
  MCD_info meter;

  memset(&curr_CDmeter, 0, sizeof(MCD_info));

  usr_fpath(fpath, cuser.userid, FN_MCD);
  if(fp = fopen(fpath, "rb"))
  {
    while(fread(&meter, sizeof(MCD_info), 1, fp) > 0)
    {						/* �z���w�g�I��p�ɾ� */
      if((meter.warning & DEFMETER_BIT) && DCD_AlterVer(&meter, NULL) > 0)
      {
	meter.warning &= ~DEFMETER_BIT;	/* �٭�^�쥻���Ʀr */
	curr_CDmeter = meter;
	break;
      }
    }
    fclose(fp);
  }
}


/* ----------------------------------------------------- */
/* ���} BBS ��						 */
/* ----------------------------------------------------- */


#define	FN_RUN_NOTE_PAD	"run/note.pad"
#define	FN_RUN_NOTE_TMP	"run/note.tmp"


typedef struct
{
  time_t tpad;
  char msg[400];
}      Pad;


int 
pad_view (void)
{
  int fd, count;
  Pad *pad;

  if ((fd = open(FN_RUN_NOTE_PAD, O_RDONLY)) < 0)
    return XEASY;

  count = 0;
  mgets(-1);

  for (;;)
  {
    pad = mread(fd, sizeof(Pad));
    if (!pad)
    {
      vmsg(NULL);
      break;
    }
    else if (!(count % 5))	/* itoc.020122: �� pad �~�L */
    {
      clear();
      move(0, 23);
      prints("�i �� �� �H �� �d �� �O �j                �� %d ��\n\n", count / 5 + 1);
    }

    outs(pad->msg);
    count++;

    if (!(count % 5))
    {
      move(b_lines, 0);
      outs("�Ы� [SPACE] �~���[��A�Ϋ���L�䵲���G ");
      /* itoc.010127: �ץ��b�������k����ΤU�A������|�����G�h��檺���D */

      if (vkey() != ' ')
	break;
    }
  }

  close(fd);
  return 0;
}


static int 
pad_draw (void)
{
  int i, cc, fdr, color;
  FILE *fpw;
  Pad pad;
  char *str, buf[3][71];

  /* itoc.����: ���Q�ΰ��m�סA�Ӫ� */
  static char pcolors[6] = {31, 32, 33, 34, 35, 36};

  /* itoc.010309: �d���O���Ѥ��P���C�� */
  color = vans("�߱��C�� 1) \033[41m  \033[m 2) \033[42m  \033[m 3) \033[43m  \033[m "
    "4) \033[44m  \033[m 5) \033[45m  \033[m 6) \033[46m  \033[m [Q] ");

  if (color < '1' || color > '6')
    return XEASY;
  else
    color -= '1';

  do
  {
    buf[0][0] = buf[1][0] = buf[2][0] = '\0';
    move(MENU_XPOS, 0);
    clrtobot();
    outs("\n�Яd�� (�ܦh�T��)�A��[Enter]����");
    for (i = 0; (i < 3) &&
      vget(16 + i, 0, "�G", buf[i], 71, DOECHO); i++);
    cc = vans("(S)�s���[�� (E)���s�ӹL (Q)��F�H[S] ");
    if (cc == 'q' || i == 0)
      return 0;
  } while (cc == 'e');

  time(&pad.tpad);

  /* itoc.020812.����: �睊�����ɭԭn�`�N struct Pad.msg[] �O�_���j */
  str = pad.msg;
  sprintf(str, "�~�t\033[1;46m %s �� %s \033[m�u", cuser.userid, cuser.username);

  for (cc = strlen(str); cc < 60; cc += 2)
    strcpy(str + cc, "�w");
  if (cc == 60)
    str[cc++] = ' ';

  sprintf(str + cc,
    "\033[1;44m %s \033[m��\n"
    "�x  \033[1;%dm%-70s\033[m  �x\n"
    "�x  \033[1;%dm%-70s\033[m  �x\n"
    "��  \033[1;%dm%-70s\033[m  ��\n",
    Btime(&pad.tpad),
    pcolors[color], buf[0],
    pcolors[color], buf[1],
    pcolors[color], buf[2]);

  f_cat(FN_RUN_NOTE_ALL, str);

  if (!(fpw = fopen(FN_RUN_NOTE_TMP, "w")))
    return 0;

  fwrite(&pad, sizeof(pad), 1, fpw);

  if ((fdr = open(FN_RUN_NOTE_PAD, O_RDONLY)) >= 0)
  {
    Pad *pp;

    i = 0;
    cc = pad.tpad - NOTE_DUE * 60 * 60;
    mgets(-1);
    while (pp = mread(fdr, sizeof(Pad)))
    {
      fwrite(pp, sizeof(Pad), 1, fpw);
      if ((++i > NOTE_MAX) || (pp->tpad < cc))
	break;
    }
    close(fdr);
  }

  fclose(fpw);

  rename(FN_RUN_NOTE_TMP, FN_RUN_NOTE_PAD);
  pad_view();
  return 0;
}


static int 
goodbye (void)
{
  /* itoc.010803: �q�i�������� */
  clear();
  film_out(FILM_GOODBYE, 0);

  switch (vans("G)�H���ӳu M)���i���� N)�d���O Q)�����H[Q] "))
  {
  /* lkchu.990428: ���w�אּ������ */
  case 'g':
  case 'y':
    break;    

  case 'm':
    m_sysop();
    break;

  case 'n':
    /* if (cuser.userlevel) */
    if (HAS_PERM(PERM_POST)) /* Thor.990118: �n��post�~��d��, �������e */
      pad_draw();
    break;

  case 'b':
    /* floatJ.090607: konami_code �m�J */
    if (konami_counter >= 6)
      more("etc/CharlieSP", MFST_NONE);
  case 'q':
  default:
    if(konami_counter >= 6)     /* dust.091010: �bb�o�B�������ܤ����Ҽ{��... */
      konami_counter = 0;

    /* return XEASY; */
    return 0;	/* itoc.010803: �q�F FILM_GOODBYE �n��ø */
  }

#ifdef LOG_BMW
  bmw_log();			/* lkchu.981201: ���y�O���B�z */
#endif

  u_exit("EXIT ");
  exit(0);
}


/* ----------------------------------------------------- */
/* help & menu processring				 */
/* ----------------------------------------------------- */


void 
vs_head (char *title, char *mid)
{
  static char fcheck[64];
  int mlen, left_space, right_space, spc;
  int title_len, currb_len;
  int ansi = 0;


  if (mid)	/* xxxx_head() ���O�� vs_head(title, str_site); */
  {
    clear();
  }
  else		/* menu() ���~�� vs_head(title, NULL); �A�]����椤�L�� clear() */
  {
    move(0, 0);
    clrtoeol();
    mid = str_site;
  }

  if (!fcheck[0])
    usr_fpath(fcheck, cuser.userid, FN_PAYCHECK);

  if (HAS_STATUS(STATUS_BIFF))
  {
    mid = "\033[1;5;37;41m �l�t�ӥm�N�o \033[m\033[1;44m";
    mlen = 14;
    ansi = 1;
  }
  else if (dashf(fcheck))
  {
    mid = "\033[5;33;43m �z���s�䲼�� \033[m\033[1;44m"; 
    mlen = 14;
    ansi = 1;
  }
#ifdef USE_DAVY_REGISTER
  else if (HAS_PERM(PERM_ALLREG))
  {
    DIR *dir;
    struct dirent *fptr;
    int len;
    char flag = 0;

    dir = opendir(FN_REGISTER);

    while ((fptr = readdir(dir)) != NULL)
    {
      if (! ('a' <= fptr->d_name[0] && fptr->d_name[0] <= 'z') )
        continue;
      len = strlen(fptr->d_name);
      if (len > 4)
        if ((fptr->d_name[len - 4] == '.') && \
          (fptr->d_name[len - 3] == 'l') && \
          (fptr->d_name[len - 2] == 'c') && \
          (fptr->d_name[len - 1] == 'k'))
          continue;
      flag = 1;
      break;
    }
    closedir(dir);
    if (flag)
    {
      mid = "\033[5;33;45m ���H����U��C >///< (��) \033[m\033[1;44m";
      mlen = 27;
      ansi = 1;
    }
    else
      mlen = strlen(mid);
  }
#else
  else if (HAS_PERM(PERM_ALLREG) && rec_num(FN_RUN_RFORM, sizeof(RFORM)))
  {
    mid = "\033[5;33;45m ���H����U��C >///< (��) \033[m\033[1;44m";
    mlen = 27;
    ansi = 1;
  }
#endif
  else
    mlen = strlen(mid);

  spc = 78 + d_cols;
  title_len = strlen(title);
  currb_len = strlen(currboard);

  left_space = spc/2 - mlen/2 - title_len - 6;
  if(left_space <1)
    left_space = 1;

  right_space = 70 + d_cols - currb_len - spc/2 + mlen/2 - mlen;
  if(right_space <1)
    right_space = 1;

  spc = title_len + left_space + right_space + currb_len + 14;
  if(mlen + spc > 78 + d_cols)
    mlen = 78 + d_cols - spc;

  if(ansi)
    mlen = 999;

  /* �L�X header */
  prints("\033[32m::\033[1m%s\033[;32;40m:: \033[1;36;44m%*s%.*s%*s\033[30;40m �ݪO%s%s%s\033[m\n",
     title, left_space, "", mlen, mid, right_space, "", brd_levelbar[0], currboard, brd_levelbar[1]);
}



/* ------------------------------------- */
/* �ʵe�B�z				 */
/* ------------------------------------- */

static char feeter[200];


/* dust.100313: �N�Ʀr�Y�g��###K/###M/###G���Φ� */
static int 
abbreviation (
    int n,
    char *str	/* ���ץ���>=6�A�_�h���w�İϷ��쪺�i�� */
)
{
  char symbol;

  if(n < 10000)
  {
    n *= 10;
    symbol = '\0';
  }
  else if(n < 1000000)	/* < million */
  {
    n /= 100;
    symbol = 'K';
  }
  else if(n < 1000000000)	/* < billion */
  {
    n /= 100000;
    symbol = 'M';
  }
  else
  {
    n /= 100000000;
    symbol = 'G';
  }

  if(n < 100 && symbol != '\0')
    n = sprintf(str, "%.1f%c", n / 10.0, symbol) - 1;
  else
    n = sprintf(str, "%d%c", n / 10, symbol) - 1;

  return str[n] == '\0'?  n : n + 1;
}


/* dust.100312: Menu footer �˦��j�ܧ� */
static void 
status_foot (void)
{
  static time_t uptime = 0;

  char buf[64], *p, *foot = feeter;
  int space, left, normal_mode, len;
  time_t now;
  struct tm *ptime;
  MCD_info *pmeter;


  time(&now);
  ptime = localtime(&now);

  if(now > uptime)	/* �L�l�]�n��s�ͤ�X�� */
  {
    if(uptime > 0)
    {
      if(cuser.day == ptime->tm_mday && cuser.month == ptime->tm_mon + 1)
	cutmp->status |= STATUS_BIRTHDAY;
      else
	cutmp->status &= ~STATUS_BIRTHDAY;
    }

    uptime = now + 86400 - ptime->tm_hour * 3600 - ptime->tm_min * 60 - ptime->tm_sec;
  }


  /* �e�b����foot */

  normal_mode = 1;
  if(cuser.ufo & UFO_MFMCOMER)	/* ���ļҦ� */
  {
    normal_mode = 0;

    foot += sprintf(foot, COLOR1 " %s %02d\033[5m:\033[m" COLOR1 "%02d ", fshm->date, ptime->tm_hour, ptime->tm_min);

    space = 5 - abbreviation(cuser.gold, buf);
    if(space > 2) space = 2;
    foot += sprintf(foot, "\033[33;43m���� %3s%*s", buf, space, "");

    space = 5 - abbreviation(cuser.money, buf);
    if(space > 2) space = 2;
    foot += sprintf(foot, "\033[37;43m�ȹ� %3s%*s\033[44m", buf, space, "");
  }
		/* �˼ƼҦ�(�x��) */
  else if((cuser.ufo & UFO_MFMGCTD) && fshm->counter.name[0] != '\0')
  {
    pmeter = &fshm->counter;
    goto print_RemainingTime;
  }
		/* �˼ƼҦ�(�ۭq) */
  else if((cuser.ufo & UFO_MFMUCTD) && curr_CDmeter.name[0] != '\0')
  {
    pmeter = &curr_CDmeter;

print_RemainingTime:
    left = DCD_AlterVer(pmeter, buf + 49);

    if(left > 0)
    {
      normal_mode = 0;
      if(left <= pmeter->warning)
	left = 1;
      else
	left = 0;

      space = 17 - strlen(pmeter->name);
      p = strtok(buf + 49, "d");
      strcpy(buf, p);
      len = strlen(p);
      space -= len + 1;
      if(len <= 5)	/* _____d__h or __d__h__m */
      {
	sprintf(buf + len, "\033[30md\033[33m%s\033[30mh", strtok(NULL, "h"));
	space -= 3;
	if(len <= 2)
	{
	  sprintf(buf + len + 19, "\033[33m%s\033[30mm", strtok(NULL, "m"));
	  space -= 3;
	}
      }
      else		/* ________D */
	strcpy(buf + len, "\033[30mD");

      if(space < 0) space = 0;

      foot += sprintf(foot, COLOR1 " %s %02d\033[5m:\033[m" COLOR1 "%02d ", fshm->date, ptime->tm_hour, ptime->tm_min);

      foot += sprintf(foot, "\033[%sm%s\033[%s30m��\033[33m%s \033[44m%*s",
	left? "37;41" : "", pmeter->name, left? "" : "1;", buf, space, "");
    }
  }

  if(normal_mode)
  {
    if(fshm->otherside)
    {
      normal_mode = 0;
      space = strlen(fshm->today);
      left = (20 - space) / 2;
      foot += sprintf(foot, COLOR1 " %s %02d\033[5m:\033[m" COLOR1 "%02d \033[37;45m%*s%-*s\033[44m",
	fshm->date, ptime->tm_hour, ptime->tm_min, left, "", 20 - left, fshm->today);
    }
    else
    {
      space = 20 - strlen(fshm->today);
      if(space > 12)
	space = 12;
      else if(space < 0)
	space = 0;
      foot += sprintf(foot, COLOR1 " %8s %02d\033[5m:\033[m" COLOR1 "%02d %s%*s",
	fshm->today, ptime->tm_hour, ptime->tm_min, FOOT_FIX, space, "");
    }
  }


  /* ��b����foot */

  if(abbreviation(total_user, buf) < 4)
    space = 3;
  else 
    space = 2;

  foot += sprintf(foot, " \033[1;30m�u�W\033[37m %-3s\033[30m�H%*s�ڬO\033[37m %-*s\033[30m[���y]\033[3",
    buf, space, "", normal_mode? 13 : 12, cuser.userid);

  if(cuser.ufo & UFO_QUIET)
    sprintf(foot, "1m���� #");
  else if(cuser.ufo & UFO_PAGER)
  {
#ifdef HAVE_NOBROAD
    if(cuser.ufo & UFO_RCVER)
      sprintf(foot, "2m�n�� \033[33m!");
    else
#endif
      sprintf(foot, "2m�n�� *");
  }
#ifdef HAVE_NOBROAD
  else if (cuser.ufo & UFO_RCVER)
    sprintf(foot, "2m�ڼs \033[33m-");
#endif
  else
    sprintf(foot, "2m����  ");

  outf(feeter);
}


void 
movie (void)
{
  /* Thor: it is depend on which state */
  if ((bbsmode <= M_XMENU) && (cuser.ufo & UFO_MOVIE))
    film_out(FILM_MOVIE, MENU_XNOTE);

  /* itoc.010403: �� feeter �� status �W�ߥX�� */
  status_foot();
}


typedef struct
{
  void *func;
  /* int (*func) (); */
  usint level;
  int umode;
  char *desc;
}      MENU;


#define MENU_LOAD   1
#define MENU_DRAW   2
#define MENU_FILM   4


#define PERM_MENU   PERM_PURGE


static MENU menu_main[];


/* ----------------------------------------------------- */
/* administrator's maintain menu             */
/* ----------------------------------------------------- */

static MENU menu_admin[];


static MENU menu_auroral[] =
{
#ifdef SP_SEND_MONEY
  "bin/xyz.so:year_bonus", PERM_SYSOP, - M_SYSTEM,
  "YearBonus    �C �����x�� �C",
#endif

  "bin/admutil.so:circ_xfile", PERM_ALLADMIN, - M_XFILES,
  "CircKaminoACL�� �l��W�� ��",

  "bin/circgod.so:a_circgod", PERM_ALLADMIN, - M_XFILES,
  "CircKamino   ���l��q�㯫��",

  "bin/pset.so:hm_admin", PERM_SYSOP, - M_SYSTEM,
  "Hourmeter    �]�w�˼ƭp�ɾ�",
#if 0
  test is over
  "bin/regmgr.so:a_regmgr", PERM_SYSOP, - M_SYSTEM,
  "RegisterMgr  �����U����z��",
#endif
  menu_admin, PERM_MENU + Ctrl('A'), M_AMENU,
  "�����S��"
};


static MENU menu_admin[] =
{
  "bin/admutil.so:a_user", PERM_ALLACCT, - M_SYSTEM,
  "User         �� �U�ȸ�� ��",

  "bin/admutil.so:a_search", PERM_ALLACCT, - M_SYSTEM,
  "Hunt         �� �j�M���� ��",

  "bin/admutil.so:a_editbrd", PERM_ALLBOARD, - M_SYSTEM,
  "QSetBoard    �� �]�w�ݪO ��",

  "bin/innbbs.so:a_innbbs", PERM_ALLBOARD, - M_SYSTEM,
  "InnBBS       �� ��H�]�w ��",

#ifdef HAVE_REGISTER_FORM
  "bin/admutil.so:a_register", PERM_ALLREG, - M_SYSTEM,
  "Register     �� �f���U�� ��",
# ifndef USE_DAVY_REGISTER /* davy.20120624: �s�E���U��f�֨t�� */
  "bin/admutil.so:a_regmerge", PERM_ALLREG, - M_SYSTEM,
  "Merge        �� �_��f�� ��",
# endif
#endif

  "bin/admutil.so:a_xfile", PERM_ALLADMIN, - M_XFILES,
  "Xfile        �� �t���ɮ� ��",

  menu_auroral, PERM_ALLADMIN,  M_AMENU,
  "Auroral      �� �����S�� ��",

  "bin/admutil.so:a_resetsys", PERM_ALLADMIN, - M_SYSTEM,
  "BBSreset     �� ���m�t�� ��",
/*
  "bin/admutil.so:a_cmd", PERM_ALLADMIN, - M_SYSTEM,
  "Commands     �� �t�Ϋ��O ��",
*/
  "bin/admutil.so:a_restore", PERM_SYSOP, - M_SYSTEM,
  "TRestore     �� �٭�ƥ� ��",

  menu_main, PERM_MENU + Ctrl('A'), M_AMENU,
  "�t�κ��@"
};

/* ----------------------------------------------------- */



/* ----------------------------------------------------- */
/* mail menu                         */
/* ----------------------------------------------------- */


static int 
XoMbox (void)
{
  xover(XZ_MBOX);
  return 0;
}


static MENU menu_mail[] =
{
  XoMbox, PERM_BASIC, M_RMAIL,
  "Read         �u �\\Ū�H�� �t",

  m_send, PERM_LOCAL, M_SMAIL,
  "Mail         �u �����H�H �t",

#ifdef MULTI_MAIL  /* Thor.981009: ����R�����B�H */
  m_list, PERM_LOCAL, M_SMAIL,
  "List         �u �s�ձH�H �t",
#endif

  m_internet, PERM_INTERNET, M_SMAIL,
  "Internet     �u �H�̩f�� �t",

#ifdef HAVE_SIGNED_MAIL
  m_verify, 0, M_XMODE,
  "Verify       �u ���ҫH�� �t",
#endif

#ifdef HAVE_MAIL_ZIP
  m_zip, PERM_INTERNET, M_SMAIL,
  "Zip          �u ���]��� �t",
#endif

  m_sysop, 0, M_SMAIL,
  "Yes Sir!     �u ��ѯ��� �t",

  "bin/admutil.so:m_bm", PERM_ALLADMIN, - M_SMAIL,
  "BM All       �u �O�D�q�i �t",    /* itoc.000512: �s�W m_bm */

  "bin/admutil.so:m_all", PERM_ALLADMIN, - M_SMAIL,
  "User All     �u �����q�i �t",    /* itoc.000512: �s�W m_all */

  menu_main, PERM_MENU + Ctrl('A'), M_MMENU,    /* itoc.020829: �� guest �S�ﶵ */
  "�q�l�l��"
};


/* ----------------------------------------------------- */
/* talk menu                         */
/* ----------------------------------------------------- */


static int 
XoUlist (void)
{
  xover(XZ_ULIST);
  return 0;
}


static MENU menu_talk[];


  /* --------------------------------------------------- */
  /* list menu                       */
  /* --------------------------------------------------- */

static MENU menu_list[] =
{
  t_pal, PERM_BASIC, M_PAL,
  "Pal          �� �B�ͦW�� ��",

#ifdef HAVE_LIST
  t_list, PERM_BASIC, M_PAL,
  "List         �� �S�O�W�� ��",
#endif

#ifdef HAVE_ALOHA
  "bin/aloha.so:t_aloha", PERM_PAGE, - M_PAL,
  "Aloha        �� �W���q�� ��",
#endif

#ifdef LOGIN_NOTIFY
  t_loginNotify, PERM_PAGE, M_PAL,
  "Notify       �� �t�Ψ�M ��",
#endif

  menu_talk, PERM_MENU + 'P', M_TMENU,
  "�U���W��"
};


static MENU menu_talk[] =
{
  XoUlist, 0, M_LUSERS,
  "Users        �� �C�ȦW�� ��",

  menu_list, PERM_BASIC, M_TMENU,
  "ListMenu     �� �]�w�W�� ��",

  t_pager, PERM_BASIC, M_XMODE,
  "Pager        �� �����I�s ��",

  t_cloak, PERM_VALID, M_XMODE,
  "Invis        �� �������� ��",

  t_query, 0, M_QUERY,
  "Query        �� �d�ߺ��� ��",

  t_talk, PERM_PAGE, M_PAGE,
  "Talk         �� ���ܺ��� ��",

  /* Thor.990220: ��ĥ~�� */
  "bin/chat.so:t_chat", PERM_CHAT, - M_CHAT,
  "ChatRoom     �� ���f��� ��",

  t_display, PERM_BASIC, M_BMW,
  "Display      �� �s�����y ��",

  t_bmw, PERM_BASIC, M_BMW,
  "Write        �� �^�U���y ��",

  menu_main, PERM_MENU + 'U', M_TMENU,
  "�𶢲��"
};


/* ----------------------------------------------------- */
/* user menu                         */
/* ----------------------------------------------------- */


static MENU menu_user[];


  /* --------------------------------------------------- */
  /* register menu                                      */
  /* --------------------------------------------------- */

static MENU menu_register[] =
{
#ifdef EMAIL_JUSTIFY
  u_addr, PERM_BASIC, M_XMODE,
  "Address      �m �q�l�H�c �n",
#endif

#ifdef HAVE_REGISTER_FORM
  u_register, PERM_BASIC, M_UFILES,
  "Register     �m ����U�� �n",
#endif

#ifdef HAVE_REGKEY_CHECK
  u_verify, PERM_BASIC, M_UFILES,
  "Verify       �m ��{�ҽX �n",
#endif

  u_deny, PERM_BASIC, M_XMODE,
  "Perm         �m ��_�v�� �n",

  menu_user, PERM_MENU + 'P', M_UMENU,
  "���U���"
};


static MENU menu_user[] =
{
  u_info, PERM_BASIC, M_XMODE,
  "Info         �m �ӤH��� �n",

  u_setup, 0, M_UFILES,
  "Habit        �m �ߦn�Ҧ� �n",

  menu_register, PERM_BASIC, M_UMENU,
  "Register     �m ���U��� �n",

  u_xfile, PERM_BASIC, M_UFILES,
  "Xfile        �m �ӤH�ɮ� �n",

  u_lock, PERM_BASIC, M_IDLE,
  "Lock         �m ��w�ù� �n",

  pad_view, 0, M_READA,
  "Note         �m �[�ݯd�� �n",

  /* itoc.010309: ���������i�H�g�d���O */
  pad_draw, PERM_POST, M_POST,
  "Pad          �m �߱���~ �n",

  u_log, PERM_BASIC, M_UFILES,
  "ViewLog      �m �W������ �n",

  menu_main, PERM_MENU + 'H', M_UMENU,
  "�ӤH�]�w"
};


#ifdef HAVE_EXTERNAL

/* ----------------------------------------------------- */
/* tool menu                         */
/* ----------------------------------------------------- */


static MENU menu_tool[];


#ifdef HAVE_SONG
  /* --------------------------------------------------- */
  /* song menu                       */
  /* --------------------------------------------------- */

static MENU menu_song[] =
{
  "bin/song.so:XoSongLog", 0, - M_XMODE,
  "KTV          �� �I�q���� ��",

  "bin/song.so:XoSongMain", 0, - M_XMODE,
  "Book         �� �۩ұ��� ��",

  "bin/song.so:XoSongSub", 0, - M_XMODE,
  "Note         �� �q����Z ��",

  menu_tool, PERM_MENU + 'K', M_XMENU,
  "���I�q��"
};
#endif


#ifdef HAVE_GAME

#if 0

  itoc.010426.����:
  �q���C�����ν����סA�����a���n�����A�u�[���A������C

  itoc.010714.����:
  (a) �C�����C�����`��������b 1.01�A�@�ӱߤW���i�� 100 ���C���A
      �Y�N�`�]����J�h���C���A�h 1.01^100 = 2.7 ��/�C���@�ӱߤW�C
  (b) �Y�U�����v�������A�]�������b 1.0 ~ 1.02 �����A�����a�@�w���ȿ��A
      �B�Y�@����̰�����Ȫ����@���A�]���|�ȱo�L�����СC
  (c) ��h�W�A���v�V�C�̨��������� 1.02�A���v�����̨��������� 1.01�C

  itoc.011011.����:
  ���F�קK user multi-login ���Ӭ~���A
  �ҥH�b���C�����}�l�N�n�ˬd�O�_���� login �Y if (HAS_STATUS(STATUS_COINLOCK))�C

#endif

  /* --------------------------------------------------- */
  /* game menu                       */
  /* --------------------------------------------------- */

static MENU menu_game[];

static MENU menu_game1[] =
{
  "bin/mine.so:main_mine", 0, - M_GAME,
  "0MineSweeper  �w ��a�p �w ",

  "bin/liteon.so:main_liteon", 0, - M_GAME,
  "1LightOn     �� �ж��}�O ��",

  "bin/guessnum.so:guessNum", 0, - M_GAME,
  "2GuessNum    �� ���q�Ʀr ��",

  "bin/guessnum.so:fightNum", 0, - M_GAME,
  "3FightNum    �� ���q�Ʀr ��",

  "bin/km.so:main_km", 0, - M_GAME,
  "4KMChess     �� [�թ���] ��",

  "bin/recall.so:main_recall", 0, - M_GAME,
  "5Memorize    �� �O�Ф��� ��",

  "bin/fantan.so:main_fantan", 0, - M_GAME,
  "6Fantan      �u �f�u���s �t",

  "bin/dragon.so:main_dragon", 0, - M_GAME,
  "7Dragon      �� ���s�C�� ��",

   "bin/nine.so:main_nine", 0, - M_GAME,
  "8Nine        �� �Ѧa�E�E ��",

  menu_game, PERM_MENU + '0', M_XMENU,
  "�q���Ѱ�"
};

static MENU menu_game2[] =
{
  "bin/dice.so:main_dice", 0, - M_GAME,
  "0Dice        �� �g�û�l ��",

  "bin/gp.so:main_gp", 0, - M_GAME,
  "1GooodPoker  �� �n�H���J ��",

  "bin/bj.so:main_bj", 0, - M_GAME,
  "2BlackJack   �� �G�Q�@�I ��",

#if 0
  /* dust.090305: �|ĳ�Mĳ�N�o�ӪF��M�ӱ��C */
  "bin/chessmj.so:main_chessmj", 0, - M_GAME,
  "3ChessMJ     �� �H�ѳ±N ��",
#endif

  "bin/seven.so:main_seven", 0, - M_GAME,
  "4Seven       �� �䫰�C�i ��",

  "bin/race.so:main_race", 0, - M_GAME,
  "5Race        �� �i�ɰ��� ��",

  "bin/bingo.so:main_bingo", 0, - M_GAME,
  "6Bingo       �� ���G�j�� ��",

  "bin/marie.so:main_marie", 0, - M_GAME,
  "7Marie       �� �j�p���� ��",

  "bin/bar.so:main_bar", 0, - M_GAME,
  "8Bar         �� �a�x���� ��",

  menu_game, PERM_MENU + '0', M_XMENU,
  "�ի�����"
};

static MENU menu_game3[] =
{
  "bin/pip.so:main_pip", PERM_BASIC, - M_GAME,
  "0Chicken     �� �q�l�p�� ��",

  "bin/pushbox.so:main_pushbox", 0, - M_GAME,
  "1PushBox     �� �ܮw�f�f ��",

  "bin/tetris.so:main_tetris", 0, - M_GAME,
  "2Tetris      �� �Xù���� ��",

  "bin/gray.so:main_gray", 0, - M_GAME,
  "3Gray        �� �L�Ǥj�� ��",

  "bin/tetravex.so:main_tetra", 0, - M_GAME,
  "4TetraVex    �� �|��o�� ��",

  "bin/sevens.so:main_sevens", 0, - M_GAME,
  "5SEVENS      �� �ƤC�C�� ��",

  "bin/gray2.so:main_gray2", 0, - M_GAME,
  "6GRAY2       �� �L�� II  ��",

  menu_game, PERM_MENU + '0', M_XMENU,
  "���a�S��"
};

static MENU menu_game[] =
{
  menu_game1, PERM_BASIC, M_XMENU,
  "1Rhapsody    �i �q���Ѱ� �j",

  menu_game2, PERM_BASIC, M_XMENU,
  "2Esoteria    �i �ի����� �j",

  menu_game3, PERM_BASIC, M_XMENU,
  "3GamerSAR    �i ���a�S�� �j",

  menu_tool, PERM_MENU + '1', M_XMENU,
  "�C���H��"
};
#endif


#ifdef HAVE_BUY
  /* --------------------------------------------------- */
  /* buy menu                        */
  /* --------------------------------------------------- */

static MENU menu_buy[] =
{
  "bin/bank.so:x_bank", PERM_BASIC, - M_GAME,
  "Bank         �� �����Ȧ� ��",

#if 0
  "bin/bank.so:b_invis", PERM_BASIC, - M_GAME,
  "Invis        �� �Ȯ����� ��",

  "bin/bank.so:b_cloak", PERM_BASIC, - M_GAME,
  "Cloak        �� �L������ ��",
#endif

  "bin/bank.so:b_mbox", PERM_BASIC, - M_GAME,
  "Mbox         �� �H�c�L�� ��",

  "bin/bank.so:b_xempt", PERM_BASIC, - M_GAME,
  "Xempt        �� �ä[�O�d ��",

  "bin/bank.so:b_buyhost", PERM_BASIC, - M_GAME,
  "Host         �� �ۭq�G�m ��",

  "bin/bank.so:b_buylabel", PERM_BASIC, - M_GAME,
  "Label        �� �ۭq���� ��",

  "bin/bank.so:b_buysign", PERM_BASIC, - M_GAME,
  "Signature    �X�Rñ�W�ɼƶq",

  menu_tool, PERM_MENU + 'B', M_XMENU,
  "���ĥ���"
};

#endif


  /* --------------------------------------------------- */
  /* other tools menu                    */
  /* --------------------------------------------------- */

static MENU menu_other[] =
{
#if 0   /* dust.090429: �]���{�b���]�p�P�ª����ۮe���t�G�A�Ȯɮ��� */
  "bin/vote.so:vote_all", PERM_BASIC, - M_VOTE, /* itoc.010414: �벼���� */
  "VoteAll      �� �벼���� ��",
#endif

#ifdef HAVE_TIP
  "bin/xyz.so:x_tip", 0, - M_READA,
  "Tip          �� �оǺ��F ��",
#endif

#ifdef HAVE_LOVELETTER
  "bin/xyz.so:x_loveletter", 0, - M_READA,
  "LoveLetter   �� ���Ѽ��g ��",
#endif

  "bin/xyz.so:x_password", PERM_VALID, - M_XMODE,
  "Password     �� �ѰO�K�X ��",

#ifdef HAVE_CLASSTABLE
  "bin/classtable.so:main_classtable", PERM_BASIC, - M_XMODE,
  "ClassTable   �� �\\�Үɬq ��",
#endif

#ifdef HAVE_CREDIT
  "bin/credit.so:main_credit", PERM_BASIC, - M_XMODE,
  "MoneyNote    �� �O�b�⥾ ��",
#endif

#ifdef HAVE_CALENDAR
  "bin/todo.so:main_todo", PERM_BASIC, - M_XMODE,
  "XTodo        �� �ӤH��{ ��",

  "bin/calendar.so:main_calendar", 0, - M_XMODE,
  "YCalendar    �� �U�~��� ��",
#endif

  "bin/xyz.so:pushdoll_maker", PERM_POST, - M_XMODE,
  "RPushdoll    �� ���嫽�� ��",

  menu_tool, PERM_MENU + Ctrl('A'), M_XMENU,    /* itoc.020829: �� guest �S�ﶵ */
  "��L�\\��"
};


static MENU menu_tool[] =
{
#ifdef HAVE_BUY
  menu_buy, PERM_BASIC, M_XMENU,
  "Market       �i ���ĥ��� �j",
#endif

#ifdef HAVE_SONG
  menu_song, 0, M_XMENU,
  "KTV          �i �u���I�q �j",
#endif

#ifdef HAVE_COSIGN
  "bin/newbrd.so:XoNewBoard", PERM_VALID, - M_XMODE,
  "Join         �i �ݪO�s�p �j",
#endif

#ifdef HAVE_GAME
  menu_game, PERM_BASIC, M_XMENU,
  "Game         �i �C���H�� �j",
#endif

  menu_other, 0, M_XMENU,
  "Other        �i ���C���K �j",

/* floatJ: �إ߯��~���եΪ����� */
#if 0
  ���ΤF
  "bin/gray2.so:main_gray2", PERM_ALLADMIN, - M_XMODE,
  "ADMIN TEST    > ���~���� < ",
#endif
  menu_main, PERM_MENU + Ctrl('A'), M_XMENU,    /* itoc.020829: �� guest �S�ﶵ */
  "�ӤH�u��"
};

#endif  /* HAVE_EXTERNAL */


/* ----------------------------------------------------- */
/* main menu                         */
/* ----------------------------------------------------- */


static int 
Gem (void)
{
  /* itoc.001109: �ݪO�`�ަb (A)nnounce �U�� GEM_X_BIT�A��K�}�O */
  XoGem("gem/"FN_DIR, "��اG�i��", (HAS_PERM(PERM_ALLBOARD) ? (GEM_W_BIT | GEM_X_BIT | GEM_M_BIT) : 0));
  return 0;
}


static MENU menu_main[] =
{
  menu_admin, PERM_ALLADMIN, M_AMENU,
  "0Admin      �X ���~�D���x �X",

  Gem, 0, M_GEM,
  "Announce    �i ��ؤ��G�� �i",

  Boards, 0, M_BOARD,
  "Boards      �[ �ݪO�C��� �[",

  Class, 0, M_BOARD,
  "Class       �p �ݪO���հ� �p",

#ifdef MY_FAVORITE
  MyFavorite, PERM_BASIC, M_MF,
  "Favorite    �b �ڪ��̷R�s �b",
#endif

  menu_mail, 0, M_MMENU,
  "Mail        �g �H����ò� �g",

  menu_talk, 0, M_TMENU,
  "Talk        �s �𶢲�Ѧa �s",

  menu_user, 0, M_UMENU,
  "User        �k �ӤH�u��c �k",

#ifdef HAVE_EXTERNAL
  menu_tool, 0, M_XMENU,
  "Xyz         �c ����۫ݩ� �c",
#endif

#if 0   /* itoc.010209: ������ */
  Select, 0, M_BOARD,
  "Select      �m ��ܥD�ݪO �m",
#endif

  goodbye, 0, M_XMODE,
 "Goodbye~    �_ �����T�T�o �_",

  NULL, PERM_MENU + 'B', M_0MENU,
  "�D�\\���"
};



static void 
konami_code (    /* �I�s���禡�öǤJkeycode�� */
    int keycode
)
{
  static char code_queue[6] = {38,38,40,40,37,39};

  if(konami_counter >= 6) konami_counter = 0;
  else if (keycode==code_queue[konami_counter])
    konami_counter++;
  else
    konami_counter = 0;
};


void 
menu (void)
{
  MENU *menu, *mptr, *table[12];
  usint level, mode;
  int cc, cx;			/* current / previous cursor position */
  int max, mmx;			/* current / previous menu max */
  int cmd, depth;
  char *str;
  char mfr_mode_def[] = "Dq";
  char *mfr_mode[] = {mfr_mode_def, "Default    �з�", "Economic   ���ĸ�T", "Auroral    �����˼�", "User       �ۭq�˼�", "Quit       ���}", NULL};


  mode = MENU_LOAD | MENU_DRAW | MENU_FILM;
  menu = menu_main;
  level = cuser.userlevel;
  depth = mmx = 0;

  for (;;)
  {
    if (mode & MENU_LOAD)
    {
      for (max = -1;; menu++)
      {
	cc = menu->level;
	if (cc & PERM_MENU)
	{

#ifdef	MENU_VERBOSE
	  if (max < 0)		/* �䤣��A�X�v�����\��A�^�W�@�h�\��� */
	  {
	    menu = (MENU *) menu->func;
	    continue;
	  }
#endif

	  break;
	}
	if (cc && !(cc & level))	/* �����v���~�q�X */
	  continue;

	table[++max] = menu;
      }

      if (mmx < max)
	mmx = max;

      if ((depth == 0) && HAS_STATUS(STATUS_BIFF))	/* �Ĥ@���W���Y���s�H�A�i�J Mail ��� */
	cmd = 'M';
      else
	cmd = cc ^ PERM_MENU;	/* default command */
      utmp_mode(menu->umode);
      cc = 0;		/* dust.080214: ���w��Ц�m */
    }
    if (mode & MENU_DRAW)
    {
      if (mode & MENU_FILM)
      {
	clear();
	note_setrand();
	movie();
	cx = -1;
      }

      vs_head(menu->desc, NULL);

      mode = 0;
      do
      {
	move(MENU_XPOS + mode, 0);
	clrtoeol();
	if (mode <= max)
	{
	  mptr = table[mode];
	  str = mptr->desc;
	  move(MENU_XPOS + mode, MENU_YPOS);
	  prints("  \033[1;30m[\033[36m%c\033[30m]\033[m%s ", *str++, str + 1);
	}
      } while (++mode <= mmx);

      mmx = max;
      mode = 0;
    }

    switch (cmd)
    {

    case KEY_DOWN:
      konami_code(40);  /* floatJ.090607: konami_code �U */

      cc = (cc == max) ? 0 : cc + 1;
      break;

    case KEY_UP:
      konami_code(38);  /* floatJ.090607: konami_code �W */

      cc = (cc == 0) ? max : cc - 1;
      break;

    /* floatJ: ���s�����ʺA�ݪO */
    case '>':
    case '.':  
      note_direction(1);
      movie();
      break;
    case '<':
    case ',':
      note_direction(-1);
      movie();
      break;

    case Ctrl('X'):	/* dust.100313: ������檬�A�C�Ҧ� */
      if(!cuser.userlevel)
	continue;

      if(cuser.ufo & UFO_MFMCOMER)
	mfr_mode_def[0] = 'E';
      else if(cuser.ufo & UFO_MFMGCTD)
	mfr_mode_def[0] = 'A';
      else if(cuser.ufo & UFO_MFMUCTD)
	mfr_mode_def[0] = 'U';
      else
	mfr_mode_def[0] = 'D';

      switch(pans(7, 18, "�п�ܪ��A�C�Ҧ�", mfr_mode))
      {
      case 'd':
	cuser.ufo &= ~UFO_MFMCOMER;
	cuser.ufo &= ~UFO_MFMGCTD;
	cuser.ufo &= ~UFO_MFMUCTD;
	status_foot();
	break;
      case 'e':
	cuser.ufo |= UFO_MFMCOMER;
	cuser.ufo &= ~UFO_MFMGCTD;
	cuser.ufo &= ~UFO_MFMUCTD;
	status_foot();
	break;
      case 'a':
	cuser.ufo &= ~UFO_MFMCOMER;
	cuser.ufo |= UFO_MFMGCTD;
	cuser.ufo &= ~UFO_MFMUCTD;
	status_foot();
	break;
      case 'u':
	cuser.ufo &= ~UFO_MFMCOMER;
	cuser.ufo &= ~UFO_MFMGCTD;
	cuser.ufo |= UFO_MFMUCTD;
	status_foot();
      }
      break;

    case Ctrl('A'):	/* itoc.020829: �w�]�ﶵ�Ĥ@�� */
    case KEY_HOME:
      cc = 0;
      break;

    case KEY_END:
      cc = max;
      break;

    case KEY_PGUP:
      cc = (cc == 0) ? max : 0;
      break;

    case KEY_PGDN:
      cc = (cc == max) ? 0 : max;
      break;

    case '\n':
    case KEY_RIGHT:
      konami_code(39);  /* floatJ.090607: konami_code �k */
      mptr = table[cc];
      cmd = mptr->umode;
#if 1
     /* Thor.990212: dynamic load , with negative umode */
      if (cmd < 0)
      {
	void *p = DL_get(mptr->func);
	if (!p)
	{
#  if 1
	  p = dlerror();
	  if(p)
	    vmsg((char *)p);
#  endif
	  break;
	}
	mptr->func = p;
	cmd = -cmd;
	mptr->umode = cmd;
      }
#endif
      utmp_mode(cmd);

      if (cmd <= M_XMENU)	/* �l�ؿ��� mode �n <= M_XMENU */
      {
	extern time_t sudo_uptime;

	menu->level = PERM_MENU + mptr->desc[0];
	menu = (MENU *) mptr->func;

	mode = MENU_LOAD | MENU_DRAW;
	/* for Admin Pseudo Sudo */
	if(bbsmode == M_AMENU && sudo_uptime > 0 && time(NULL) - sudo_uptime <= 300)
	  mode |= MENU_FILM;

	depth++;
	continue;
      }

      {
	int (*func) ();

	func = mptr->func;
	mode = (*func) ();
      }

      utmp_mode(menu->umode);

      if (mode == XEASY)
      {
	outf(feeter);
	mode = 0;
      }
      else
      {
	mode = MENU_DRAW | MENU_FILM;
      }

      cmd = mptr->desc[0];
      continue;

#ifdef EVERY_Z
    case Ctrl('Z'):
      every_Z(0);
      goto every_key;

    case Ctrl('U'):
      every_U(0);
      goto every_key;
#endif

    /* itoc.010911: Select everywhere�A���A����O�b M_0MENU */
    case 's':
    case Ctrl('S'):
      utmp_mode(M_BOARD);
      Select();
      goto every_key;

#ifdef MY_FAVORITE
    /* itoc.010911: Favorite everywhere�A���A����O�b M_0MENU */
    case 'f':
    case Ctrl('F'):
      if (cuser.userlevel)	/* itoc.010407: �n�ˬd�v�� */
      {
	utmp_mode(M_MF);
	MyFavorite();
      }
      goto every_key;
#endif

    /* itoc.020301: Read currboard in M_0MENU */
    case 'r':
      if (bbsmode == M_0MENU)
      {
	if (currbno >= 0)
	{
	  utmp_mode(M_BOARD);
	  XoPost(currbno);
	  xover(XZ_POST);
#ifndef ENHANCED_VISIT
	  time(&brd_visit[currbno]);
#endif
	}
	goto every_key;
      }
      goto default_key;	/* �Y���b M_0MENU ���� r ���ܡA�n�����@����� */

every_key:	/* �S����B�z���� */
      utmp_mode(menu->umode);
      mode = MENU_DRAW | MENU_FILM;
      cmd = table[cc]->desc[0];
      continue;

    case KEY_LEFT:
      konami_code(37);  /* floatJ.090607: konami_code �� */
    case 'e':
      if (depth > 0)
      {
	menu->level = PERM_MENU + table[cc]->desc[0];
	menu = (MENU *) menu->func;
	mode = MENU_LOAD | MENU_DRAW;
	if(depth == 1)	/* dust.091009�^��ڥؿ��ɭ��s��ܰʺA�ݪO */
	  mode |= MENU_FILM;
        depth--;
	continue;
      }
      cmd = 'G';

default_key:
    default:

      if (cmd >= 'a' && cmd <= 'z')
	cmd ^= 0x20;			/* �ܤj�g */

      cc = 0;
      for (;;)
      {
	if (table[cc]->desc[0] == cmd)
	  break;
	if (++cc > max)
	{
	  cc = cx;
	  goto menu_key;
	}
      }
    }

    if (cc != cx)	/* �Y��в��ʦ�m */
    {
#ifdef CURSOR_BAR
      if (cx >= 0)
      {
	move(MENU_XPOS + cx, MENU_YPOS);
	if (cx <= max)
	{
	  mptr = table[cx];
	  str = mptr->desc;
	  /* floatJ.080510: �ոլݧ��ܤ@�U�~�[ :p */
	  prints("  \033[1;30m[\033[36m%c\033[30m]\033[m%s ", *str, str + 1);
	}
	else
	{
	  outs("  ");
	}
      }
      move(MENU_XPOS + cc, MENU_YPOS);
      mptr = table[cc];
      str = mptr->desc;
      prints(COLOR8 "> [%c]%s \033[m", *str, str + 1);
      cx = cc;
#else		/* �S�� CURSOR_BAR */
      if (cx >= 0)
      {
	move(MENU_XPOS + cx, MENU_YPOS);
	outc(' ');
      }
      move(MENU_XPOS + cc, MENU_YPOS);
      outc('>');
      cx = cc;
#endif
    }
    else		/* �Y��Ъ���m�S���� */
    {
#ifdef CURSOR_BAR
      move(MENU_XPOS + cc, MENU_YPOS);
      mptr = table[cc];
      str = mptr->desc;
      prints(COLOR8 "> [%c]%s \033[m", *str, str + 1);
#else
      move(MENU_XPOS + cc, MENU_YPOS + 1);
#endif
    }

menu_key:
    cmd = vkey();
  }
}

