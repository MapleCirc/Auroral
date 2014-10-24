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
/* �Ȧs�� TBF (Temporary Buffer File) routines		 */
/* ----------------------------------------------------- */

char *
tbf_ask()
{
  static char fn_tbf[] = "buf.1";
  int ch, prev;

  do
  {
    prev = fn_tbf[4];
    ch = vget(b_lines, 0, "�п�ܼȦs��(1-5, ��0����)�G", fn_tbf + 4, 2, GCARRY);

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
    ans = vans("�Ȧs�ɤw����� (A)���[ (W)�мg (Q)�����H[A] ");
    if (ans == 'q')
      return NULL;

    if (ans != 'w')
    {
      /* itoc.030208: �ˬd�Ȧs�ɤj�p */
      if (st.st_size > 8388608)	/* dust.100630: �վ�W����8MB */
      {
	vmsg("�Ȧs�ɤӤj�A�L�k���[");
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
char anonymousid[IDLEN + 1];	/* itoc.010717: �۩w�ΦW ID */
#endif


void
ve_header(fp)
  FILE *fp;
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
      if (!vget(b_lines, 0, "�п�J�z�Q�Ϊ�ID�A�]�i������[Enter]�A�άO��[r]�ίu�W�G", anonymousid, IDLEN, DOECHO))
      {											/* �d 1 byte �[ "." */
	strcpy(anonymousid, STR_ANONYMOUS);
	curredit |= EDIT_ANONYMOUS;
      }
      else if (strcmp(anonymousid, "r"))
      {
	strcat(anonymousid, ".");		/* �Y�O�۩wID�A�n�b�᭱�[�� . ��ܤ��P */
	curredit |= EDIT_ANONYMOUS;
      }
    }
#endif

    if (!(currbattr & BRD_NOSTAT) && !(curredit & EDIT_RESTRICT))	/* ���p�峹�g�� �� �[�K�s�� ���C�J�έp�g�� */
    {
      /* ���Ͳέp��� */

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

  fprintf(fp, "���D: %s\n�ɶ�: %s\n\n", title, Btime(&now));
}


int
ve_subject(row, topic, dft)
  int row;
  char *topic;
  char *dft;
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

  return vget(row, 0, "���D�G", title, TTLEN + 1, GCARRY);
}



static void
ve_fromhost(buf)
  char *buf;
{
  unsigned long ip32;
  int i;
  uschar ipv4[4];
  char fromname[48], ipaddr[32], name[48];

  ip32 = cutmp->in_addr;	/* dust.091002: in_addr���ϥΥ� hrs113355 ���� <(_ _)> */
  for(i = 0; i < 4; i++)
  {
    ipv4[i] = ip32 & 0xFF;
    ip32 >>= 8;
  }
  sprintf(ipaddr, "%d.%d.%d.%d", ipv4[3], ipv4[2], ipv4[1], ipv4[0]);

  /* ���� FQDN ����� */
  str_lower(name, fromhost);
  if (!belong_list(FN_ETC_FQDN, name, fromname, sizeof(fromname)))
  {
    if(!belong_list(FN_ETC_HOST, name, fromname, sizeof(fromname)))
    {
      /* �A�� IP ����� */
      if (!belong_list(FN_ETC_HOST, ipaddr, fromname, sizeof(fromname)))
	strcpy(fromname, "");		/* �٬O�S�����ܴN��ť� */
    }
  }

  /* dust: ���ñ�z�檺�S�O���� */
  if(fromname[0] == '\0')	/* �S���������G�m */
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



char *ebs[EDIT_BANNER_NUM] = EDIT_BANNERS;	/* ��b���줤��vote.c�@�� */

void
ve_banner(fp, modify)		/* �[�W�ӷ����T�� */
  FILE *fp;
  int modify;			/* 1:�ק� 0:�D�o�� */
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
/* �s�边�۰ʳƥ�					 */
/* ----------------------------------------------------- */

void
ve_recover()
{
  char fpbak[80], fpath[80];
  int ch;

  usr_fpath(fpbak, cuser.userid, FN_BAK);
  if (dashf(fpbak))
  {
    ch = vans("�z���@�g�峹�|�������A(M)�H��H�c (S)�g�J�Ȧs�� (Q)��F�H[M] ");
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
      mail_self(fpbak, cuser.userid, "���������峹", 0);
    }
    unlink(fpbak);
  }
}



/* ----------------------------------------------------- */
/* ���ñ�W��						 */
/* ----------------------------------------------------- */

static void
show_sign(cur_page)
  int cur_page;
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

  for(; sig_no <= lim; sig_no++)        /* dust.100331: �ϥΪ̦ۭq�i��ñ�W�ɼƶq */
  {
    fpath[len] = sig_no + '1';

    move((sig_no % 3) * (MAXSIGLINES + 1) + 1, 0);
    if(lim_cat(fpath, MAXSIGLINES) >= 0)
    {
      move((sig_no % 3) * (MAXSIGLINES + 1), 0);
      prints("\033[1;36m�i ñ�W�� %c �j\033[m", sig_no + '1');
    }
    else
    {
      move((sig_no % 3) * (MAXSIGLINES + 1), 0);
      prints("\033[1;30m�i ñ�W�� %c �j\033[m", sig_no + '1');
    }
  }
}


int
select_sign()
{
  static char msg[] = "�п��ñ�W�� (1~6, 0=���[ r=�ü� n=����) [0] ";
  int ans, cur_sig, sig_page;
  int sig_avail = cuser.sig_avai;
  int cur_page = 0;

#ifdef HAVE_ANONYMOUS
  /* �b�ΦW�O���L�׬O�_�ΦW�A�����ϥ�ñ�W�� */
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
      /* dust.100331: �e����ܭY�W�L�{�b���i�μƶq�N�令�H�� */
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
/* vedit �^�� -1:�����s��, 0:�����s��					 */
/* --------------------------------------------------------------------- */
/* ve_op:								 */
/*  0 => �º�s���ɮ�							 */
/* -1 => �����x�s�A�Φb�s��@�̤��O�ۤv���峹��				 */
/*  1 => ���ޤ�Pñ�W�ɡA�å[�W���Y�C�Φb�o��峹�P�H�����H		 */
/*  2 => ���ޤ�Pñ�W�ɡA���[�W���Y�C�Φb�H���~�H			 */
/*  0~9 �B curredit & EDIT_TEMPLATE => �ϥΤ峹�˪O�A���J��|�Q���� 1	 */
/* --------------------------------------------------------------------- */
/* ve_op �O 1, 2, �ɡA�ϥΫe�ٱo���w curredit �M quote_file		 */
/* --------------------------------------------------------------------- */

int
vedit(fpath, ve_op)
  char *fpath;
  int ve_op;
{
  return dledit(fpath, ve_op);
}

