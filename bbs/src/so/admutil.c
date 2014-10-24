/*-------------------------------------------------------*/
/* admutil.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : �������O					 */
/* create : 95/03/29					 */
/* update : 01/03/01					 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;
extern UCACHE *ushm;

extern time_t sudo_uptime;


static int
a_check()	/* dust.100323: ���Ȩ����T�{ �i���� (pseudo 'sudo') */
		/* xatier. �ǹФj�j�W�j�� */
		/* floatj. �ǹп߬O�ڭ̪���w */
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

    if(!vget(b_lines, 0, "\033[1;44m�i���ܡj\033[33;40m �A���K�X���X�ӤT���v��www?\033[;33;40m(Timelimit: 5 min)\033[m", pswd, PSWDLEN + 1, NOECHO))
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
/* ���ȫ��O						 */
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
a_search()	/* itoc.010902: �ɤO�j�M�ϥΪ� */
{
  ACCT acct;
  char c;
  char key[30];

  if (!a_check())
    return 0;

  if (!vget(b_lines, 0, "�п�J����r(�m�W/�ʺ�/�ӷ�/�H�c)�G", key, 30, DOECHO))
    return XEASY;  

  /* itoc.010929.����: �u�O�����ɤO :p �Ҽ{���� reaper ���X�@�� .PASSWDS �A�h�� */

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

	if (vans("�O�_�~��j�M�U�@���H[N] ") != 'y')
	{
	  closedir(dirp);
 	  goto end_search;
 	}
      }
    }
    closedir(dirp);
  }
end_search:
  vmsg("�j�M����");
  return 0;
}


int
a_editbrd()		/* itoc.010929: �ק�ݪO�ﶵ */
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


/* floatJ: �۲� */
int
circ_xfile()               /* �]�w�S���ɮ�(circ god�ϥ�) */
{

  /* 1. �u�� sysop �~�i�H�~��  2. ���Klog */
  if (strcmp(cuser.userid, "sysop"))
  {
    vmsg("�u�� sysop �~�i�H�~��� >.^");
    return 0;
  }
    
  
  circlog("�Hsysop�n�J�A�ýs��l��W��", fromhost);

  static char *desc[] =
  {
   "�l��q�㯫�W��",
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
a_xfile()		/* �]�w�t���ɮ� */
{
  if (!a_check())
    return 0;
  static char *desc[] =
  {
    "�ݪO�峹����",

    "�����{�ҫH��",
    "�{�ҳq�L�q��",
    "���s�{�ҳq��",

#ifdef HAVE_DETECT_CROSSPOST
    "��K���v�q��",
#endif
    
    "�����W��",
    "���ȦW��",

    "�`��",

#ifdef HAVE_WHERE
    "�G�m ID",
    "�G�m IP",
    "�G�m FQDN",
#endif

#ifdef HAVE_TIP
    "�C��p���Z",
#endif

#ifdef HAVE_LOVELETTER
    "���Ѳ��;���w",
#endif

    "�{�ҥզW��",
    "�{�Ҷ¦W��",

    "���H�զW��",
    "���H�¦W��",

#ifdef HAVE_LOGIN_DENIED
    "�ڵ��s�u�W��",
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
a_resetsys()		/* ���m */
{
  switch (vans("�� �t�έ��] 1)�ʺA�ݪO 2)�����s�� 3)���W�ξ׫H 4)�����G[Q] "))
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
/* �٭�ƥ���						 */
/* ----------------------------------------------------- */


static void
show_availability(type)		/* �N BAKPATH �̭��Ҧ��i���^�ƥ����ؿ��L�X�� */
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
    fputs("�� �i�Ѩ��^���ƥ����G\n\n", fp);

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

  ch = vans("�� �٭�ƥ� 1)�ݪO 2)��ذ� 3)�ϥΪ̡G[Q] ") - '1';
  if (ch < 0 || ch >= 3)
    return XEASY;

  type = tpool[ch];
  show_availability(type);

  if (vget(b_lines, 0, "�n���^���ƥ��ؿ��G", date, 20, DOECHO))
  {
    /* �קK�������F�@�Ӧs�b���ؿ��A���O�M type ���X */
    if (strncmp(date, type, strlen(type)))
      return 0;

    sprintf(src, BAKPATH"/%s", date);
    if (!dashd(src))
      return 0;
    ptr = strchr(src, '\0');

    clear();
    move(3, 0);
    outs("���٭�ƥ����ݪO/�ϥΪ̥����w�s�b�C\n"
      "�Y�ӬݪO/�ϥΪ̤w�R���A�Х����s�}�]/���U�@�ӦP�W���ݪO/�ϥΪ̡C\n"
      "�٭�ƥ��ɽнT�{�ӬݪO�L�H�ϥ�/�ϥΪ̤��b�u�W");

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
      /* �ɮפ��s�b�A�q�`�O�]���ƥ��I�ɸӬݪO/�ϥΪ̤w�Q�R���A�άO��ɮڥ��N�٨S���ӬݪO/�ϥΪ� */
      vmsg("�ƥ��ɮפ��s�b�A�иոը�L�ɶ��I���ƥ�");
      return 0;
    }

    if (vans("�٭�ƥ���A�ثe�Ҧ���Ƴ��|�y���A�аȥ��T�w(Y/N)�H[N] ") != 'y')
      return 0;

    alog("�٭�ƥ�", src);

    /* �����Y */
    if (ch == 0)
      ptr = "brd";
    else if (ch == 1)
      ptr = "gem/brd";
    else /* if (ch == 2) */
      sprintf(ptr = date, "usr/%c", *type);
    sprintf(cmd, "tar xfz %s -C %s/", src, ptr);
    /* system(cmd); */

#if 1	/* ��������ʰ��� */
    move(7, 0);
    outs("\n�ХH bbs �����n�J�u�@���A�é�\033[1;36m�a�ؿ�\033[m����\n\n\033[1;33m");
    outs(cmd);
    outs("\033[m\n\n");
#endif

    /* tar ���H��A�٭n������ */
    if (vans("�� ���O Y)�w���\\����H�W���O Q)������G[Q] ") == 'y')
    {
      if (ch == 0)	/* �٭�ݪO�ɡA�n��s�O�� */
      {
	if ((ch = brd_bno(brdname)) >= 0)
	{
	  brd_fpath(src, brdname, fn_pal);
	  bpal = bshm->pcache + ch;
	  bpal->pal_max = image_pal(src, bpal->pal_spool);
	}
      }
      else if (ch == 2)	/* �٭�ϥΪ̮ɡA���٭� userno */
      {
	ch = acct.userno;
	if (acct_load(&acct, type) >= 0)
	{
	  acct.userno = ch;
	  acct_save(&acct);
	}
      }
      vmsg("�٭�ƥ����\\ ");
      return 0;
    }
  }

  vmsg(msg_cancel);
  return 0;
}


#ifdef HAVE_REGISTER_FORM

/* ----------------------------------------------------- */
/* �B�z Register Form					 */
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
    sprintf(title, "[����] ���� %s %s %s �����U��", cuser.userid, pass? "�q�L" : "�h�^", rform->userid);

    /* �峹���Y */
    fprintf(fp, "%s <���F����> (Eye of Daemon)\n", str_author1);
    fprintf(fp, "���D: %s\n�ɶ�: %s\n\n", title, Now());

    /* �峹���e */
    fprintf(fp, "�ӽЮɶ�: %s\n", Btime(&rform->rtime));
    fprintf(fp, "�A�ȳ��: %s\n", rform->career);
    fprintf(fp, "�ثe��}: %s\n", rform->address);
    fprintf(fp, "�s���q��: %s\n", rform->phone);

    if(!pass)	/* �O���h�^��] */
    {
      int i;

      fprintf(fp, "\n�h�^��]�G\n");
      for(i = 1; i < szbox; i++)
      {
	if(checkbox[i])
	  fprintf(fp, "    %s\n", reason[i]);
      }
      if(checkbox[0])
	fprintf(fp, "    %s\n", reason[0]);
    }

    fclose(fp);

    add_post(BN_REGLOG, fpath, title, "<���F����>", "Eye of Daemon", POST_DONE);

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
    prompt, "�п�J�u��m�W", "�иԶ��}���", "�иԶ�s���q��", "�иԶ�A�ȳ��(�Ǯըt��)", "�ХΤ����g�ӽг�",
  };
  int checkbox[sizeof(reason) / sizeof(reason[0])];
  int op, i, n;
  FILE *fp_note;
  HDR hdr;


  move(9, 0);
  clrtobot();
  outs("\n�д��X�h�^���U���]�G\n\n");
  n = sizeof(reason) / sizeof(char*);
  for(i = 1; i < n; i++)
    prints("    %d. %s\n", i, reason[i]);
  prints("    i. [�ۭq]��\n");
  memset(checkbox, 0, sizeof(checkbox));

  while(1)
  {
    op = vans("�̷ӽs�������ﶵ(��Js����)�G");

    i = op - '0';
    if(i >= 1 && i <= n)
    {
      checkbox[i] = !checkbox[i];

      move(11 + i, 0);
      outs(checkbox[i]? "��" : "  ");
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
	if(!vget(b_lines, 0, "�ۭq�G", prompt, sizeof(prompt), DOECHO))
	  continue;

	move(11 + n, 0);
	outs("��");
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
	vmsg("�п�ܰh�^��]");
    }
  }

  /* �H�h�^�H */
  usr_fpath(folder, uid, fn_dir);
  if(fp_note = fdopen(hdr_stamp(folder, 0, &hdr, fpath), "w"))
  {
    fprintf(fp_note, "�@��: %s (%s)\n", cuser.userid, cuser.username);
    fprintf(fp_note, "���D: [�q��] �Э��s��g���U��\n�ɶ�: %s\n\n", Now());

    fputs("�h���]�G\n", fp_note);
    for(i = 1; i < n; i++)
    {
      if(checkbox[i])
	fprintf(fp_note, "    %s\n", reason[i]);
    }
    if(checkbox[0])
      fprintf(fp_note, "    %s\n", prompt);

    fputs("\n\n�]���H�W��]�A�ϵ��U���Ƥ������A\n", fp_note);
    fputs("�̷�\033[1m�x�W�ǳN����BBS���޲z�ϥΤ���\033[m�A�L�k�@�������{�ҡC\n", fp_note);
    fputs("\n�Э��s��g���U��C\n", fp_note);
    fputs("\n                                                    " BBSNAME "����\n", fp_note);
    fclose(fp_note);

    strcpy(hdr.owner, cuser.userid);
    strcpy(hdr.title, "[�q��] �Э��s��g���U��");
    rec_add(folder, &hdr, sizeof(HDR));

    m_biff(rform->userno);
  }

  /* �O���f�� */
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


  vs_bar("�f�֨ϥΪ̵��U���");

  while(read(fd, &rform, sizeof(RFORM)) == sizeof(RFORM))
  {
    move(2, 0);
    prints("�ӽХN��: %s (�ӽЮɶ��G%s)\n", rform.userid, Btime(&rform.rtime));
    prints("�A�ȳ��: %s\n", rform.career);
    prints("�ثe��}: %s\n", rform.address);
    prints("�s���q��: %s\n", rform.phone);
    outs(msg_seperator);
    outc('\n');
    clrtobot();

    if(acct_load(&acct, rform.userid) < 0 || acct.userno != rform.userno)
    {
      vmsg("�d�L���H");
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
	vmsg("���b���w�g�������U");
	op = 'd';
      }
      else if (acct.userlevel & PERM_ALLDENY)
      {
	/* itoc.050405: ���������v�̭��s�{�ҡA�]���|�ﱼ�L�� tvalid (���v����ɶ�) */
	vmsg("���b���ثe�Q���v��");
	op = 'd';
      }
      else
      {
	op = vans("�O�_����(Y/N/Q/Del/Skip)�H[S] ");
      }
    }

    switch (op)
    {
    case 'y':
      /* �O�����U��� */
      sprintf(buf, "REG: %s:%s:%s:by %s", rform.phone, rform.career, rform.address, cuser.userid);
      justify_log(acct.userid, buf);

      /* �����v�� */
      time(&acct.tvalid);
      /* itoc.041025: �o�� acct_setperm() �èS�����b acct_load() �᭱�A�����j�F�@�� vans()�A
	�o�i��y������ acct �h�л\�s .ACCT �����D�C���L�]���O�����~�����v���A�ҥH�N����F */
      acct_setperm(&acct, PERM_VALID, 0);

      /* �H�H�q���ϥΪ� */
      usr_fpath(folder, rform.userid, fn_dir);
      hdr_stamp(folder, HDR_LINK, &hdr, FN_ETC_JUSTIFIED);
      strcpy(hdr.title, msg_reg_valid);
      strcpy(hdr.owner, str_sysop);
      rec_add(folder, &hdr, sizeof(HDR));
      m_biff(rform.userno);

      /* �O���f�� */
      log_register(&rform, 1, NULL, NULL, 0);

      break;

    case 'q':			/* ���� */
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
    vmsg("�ثe�õL�s���U���");
    return XEASY;
  }

  sprintf(buf, "�@�� %d ����ơA�}�l�f�ֶ�(Y/N)�H[N] ", num);
  num = XEASY;

  if (vans(buf) == 'y')
  {
    sprintf(buf, "%s.tmp", FN_RUN_RFORM);
    if (dashf(buf))
    {
      vmsg("�������]�b�f�ֵ��U�ӽг椤");
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
	vmsg("�L�k�}�ҵ��U��Ƥu�@�ɡI");
      }
    }
  }
  return num;
}


int
a_regmerge()			/* itoc.000516: �_�u�ɵ��U��״_ */
{
  char fpath[64];
  FILE *fp;

  if(!a_check())
    return 0;

  sprintf(fpath, "%s.tmp", FN_RUN_RFORM);
  if (dashf(fpath))
  {
    vmsg("�Х��T�w�w�L��L�����b�f�ֵ��U��A�H�K�o�ͨƬG");

    if (vans("�T�w�n�Ұʵ��U��״_�\\��(Y/N)�H[N] ") == 'y')
    {
      if (fp = fopen(FN_RUN_RFORM, "a"))
      {
	f_suck(fp, fpath);
	fclose(fp);
	unlink(fpath);
      }
      vmsg("�B�z����");
    }
  }
  else
  {
    vmsg("�ثe�õL�״_���U�椧���n");
  }
  return XEASY;
}
#endif  /* USE_DAVY_REGSITER */
#endif	/* HAVE_REGISTER_FORM */


/* ----------------------------------------------------- */
/* �H�H�������ϥΪ�/�O�D				 */
/* ----------------------------------------------------- */


static void
add_to_list(list, id)
  char *list;
  char *id;		/* ���� end with '\0' */
{
  char *i;

  /* ���ˬd���e�� list �̭��O�_�w�g���F�A�H�K���Х[�J */
  for (i = list; *i; i += IDLEN + 1)
  {
    if (!strncmp(i, id, IDLEN))
      return;
  }

  /* �Y���e�� list �S���A���򪽱����[�b list �̫� */
  str_ncpy(i, id, IDLEN + 1);
}


static void
make_bm_list(list)
  char *list;
{
  BRD *head, *tail;
  char *ptr, *str, buf[BMLEN + 1];

  /* �h bshm ����X�Ҧ� brd->BM */

  head = bshm->bcache;
  tail = head + bshm->number;
  do				/* �ܤ֦� note �@�O�A������ݪO���ˬd */
  {
    ptr = buf;
    strcpy(ptr, head->BM);

    while (*ptr)	/* �� brd->BM �� bm1/bm2/bm3/... �U�� bm ��X�� */
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
  char *title;		/* �H�󪺼��D */
  char *fpath;		/* �H���ɮ� */
  char *list;		/* �H�H���W�� */
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

  if (vans("�n�H�H�������Ҧ��O�D(Y/N)�H[N] ") != 'y')
    return XEASY;

  strcpy(ve_title, "[�O�D�q�i] ");
  if (!vget(1, 0, "���D�G", ve_title, TTLEN + 1, GCARRY))
    return 0;

  usr_fpath(fpath, cuser.userid, "sysmail");
  if (fp = fopen(fpath, "w"))
  {
    fprintf(fp, "�� [�O�D�q�i] �����q�i�A���H�H�G�U�O�D\n");
    fprintf(fp, "-------------------------------------------------------------------------\n");
    fclose(fp);
  }

  curredit = EDIT_MAIL;
  *quote_file = '\0';
  if (vedit(fpath, 1) >= 0)
  {
    vmsg("�ݭn�@�q�Z�����ɶ��A�Э@�ߵ���");

    size = (IDLEN + 1) * MAXBOARD * 4;	/* ���]�C�O�|�ӪO�D�w���� */
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

  if (vans("�n�H�H�������ϥΪ�(Y/N)�H[N] ") != 'y')
    return XEASY;    

  strcpy(ve_title, "[�t�γq�i] ");
  if (!vget(1, 0, "���D�G", ve_title, TTLEN + 1, GCARRY))
    return 0;

  usr_fpath(fpath, cuser.userid, "sysmail");
  if (fp = fopen(fpath, "w"))
  {
    fprintf(fp, "�� [�t�γq�i] �����q�i�A���H�H�G�����ϥΪ�\n");
    fprintf(fp, "-------------------------------------------------------------------------\n");
    fclose(fp);
  }

  curredit = EDIT_MAIL;
  *quote_file = '\0';
  if (vedit(fpath, 1) >= 0)
  {
    vmsg("�ݭn�@�q�Z�����ɶ��A�Э@�ߵ���");

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
