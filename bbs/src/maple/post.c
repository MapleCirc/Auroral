/*-------------------------------------------------------*/
/* post.c       ( NTHU CS MapleBBS Ver 2.39 )		 */
/*-------------------------------------------------------*/
/* target : bulletin boards' routines		 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"

extern BCACHE *bshm;
extern XZ xz[];


extern int wordsnum;		/* itoc.010408: �p��峹�r�� */
extern int TagNum;
extern char xo_pool[];
extern char brd_bits[];

extern char **brd_levelbar;	/* dust:���s��vs_head()�Ϊ� */

#ifdef HAVE_ANONYMOUS
extern char anonymousid[];	/* itoc.010717: �۩w�ΦW ID */
#endif


			/* �^��/���/���/�\Ū�����P�D�D�^��/�\Ū�����P�D�D���/�\Ū�����P�D�D��� */
static char *title_type[6] = {"Re", "Fw", "��", "\033[1;33m=>", "\033[1;33m=>", "\033[1;32m��"};
/* dust.100922: �令�����ܼƨ�post_title()�w���ϥ� */


int
cmpchrono(hdr)
  HDR *hdr;
{
  return hdr->chrono == currchrono;
}


static void
change_stamp(folder, hdr)
  char *folder;
  HDR *hdr;
{
  HDR tmp;
  char fpath[64];

  hdr_fpath(fpath, folder, hdr);

  /* stamp��ʨϥ�'C'�@���ɦW�}�Y */
  hdr_stamp(folder, HDR_LINK | 'C', &tmp, fpath);

  hdr->stamp = tmp.chrono;
}


/* ----------------------------------------------------- */
/* ��} innbbsd ��X�H��B�s�u��H���B�z�{��		 */
/* ----------------------------------------------------- */


void
btime_update(bno)
  int bno;
{
  if (bno >= 0)
    (bshm->bcache + bno)->btime = -1;	/* �� class_item() ��s�� */
}


#ifndef HAVE_NETTOOL
static 			/* �� enews.c �� */
#endif
void
outgo_post(hdr, board)
  HDR *hdr;
  char *board;
{
  bntp_t bntp;

  memset(&bntp, 0, sizeof(bntp_t));

  if (board)		/* �s�H */
  {
    bntp.chrono = hdr->chrono;
  }
  else			/* cancel */
  {
    bntp.chrono = -1;
    board = currboard;
  }
  strcpy(bntp.board, board);
  strcpy(bntp.xname, hdr->xname);
  strcpy(bntp.owner, hdr->owner);
  strcpy(bntp.nick, hdr->nick);
  strcpy(bntp.title, hdr->title);
  rec_add("innd/out.bntp", &bntp, sizeof(bntp_t));
}


void
cancel_post(hdr)
  HDR *hdr;
{
  if ((hdr->xmode & POST_OUTGO) &&		/* �~��H�� */
    (hdr->chrono > ap_start - 7 * 86400))	/* 7 �Ѥ������� */
  {
    outgo_post(hdr, NULL);
  }
}


static inline int		/* �^�Ǥ峹 size �h���� */
move_post(hdr, folder, by_bm)	/* �N hdr �q folder �h��O���O */
  HDR *hdr;
  char *folder;
  int by_bm;
{
  HDR post;
  int xmode;
  char fpath[64], fnew[64], *board;
  struct stat st;

  xmode = hdr->xmode;
  hdr_fpath(fpath, folder, hdr);

  if (!(xmode & POST_BOTTOM))	/* �m����Q�夣�� move_post */
  {
#ifdef HAVE_REFUSEMARK
    board = by_bm && !(xmode & POST_RESTRICT) ? BN_DELETED : BN_JUNK;	/* �[�K�峹��h junk */
#else
    board = by_bm ? BN_DELETED : BN_JUNK;
#endif

    brd_fpath(fnew, board, fn_dir);
    hdr_stamp(fnew, HDR_LINK | 'A', &post, fpath);

    /* �����ƻs trailing data�Gowner(�t)�H�U�Ҧ���� */

    memcpy(post.owner, hdr->owner, (&post + 1) - (HDR *)post.owner);

    if (by_bm)
      sprintf(post.title, "%-13s%.59s", cuser.userid, hdr->title);

    rec_bot(fnew, &post, sizeof(HDR));
    btime_update(brd_bno(board));
  }

  by_bm = stat(fpath, &st) ? 0 : st.st_size;

  unlink(fpath);
  btime_update(currbno);
  cancel_post(hdr);

  return by_bm;
}


#ifdef HAVE_DETECT_CROSSPOST
/* ----------------------------------------------------- */
/* ��} cross post ���v					 */
/* ----------------------------------------------------- */


#define MAX_CHECKSUM_POST	20	/* �O���̪� 20 �g�峹�� checksum */
#define MAX_CHECKSUM_LINE	6	/* �u���峹�e 6 ��Ӻ� checksum */


typedef struct
{
  int sum;			/* �峹�� checksum */
  int total;			/* ���峹�w�o��X�g */
}      CHECKSUM;


static CHECKSUM checksum[MAX_CHECKSUM_POST];
static int checknum = 0;


static inline int
checksum_add(str)		/* �^�ǥ��C��r�� checksum */
  char *str;
{
  int i, len, sum;

  len = strlen(str);

  sum = len;	/* ��r�ƤӤ֮ɡA�e�|�����@�ܥi�৹���ۦP�A�ҥH�N�r�Ƥ]�[�J sum �� */
  for (i = len >> 2; i > 0; i--)	/* �u��e�|�����@�r���� sum �� */
    sum += *str++;

  return sum;
}


static inline int		/* 1:�Ocross-post 0:���Ocross-post */
checksum_put(sum)
  int sum;
{
  int i;

  if (sum)
  {
    for (i = 0; i < MAX_CHECKSUM_POST; i++)
    {
      if (checksum[i].sum == sum)
      {
	checksum[i].total++;

	if (checksum[i].total > MAX_CROSS_POST)
	  return 1;
	return 0;	/* total <= MAX_CROSS_POST */
      }
    }

    if (++checknum >= MAX_CHECKSUM_POST)
      checknum = 0;
    checksum[checknum].sum = sum;
    checksum[checknum].total = 1;
  }
  return 0;
}


static int			/* 1:�Ocross-post 0:���Ocross-post */
checksum_find(fpath)
  char *fpath;
{
  int i, sum;
  char buf[ANSILINELEN];
  FILE *fp;

  sum = 0;
  if (fp = fopen(fpath, "r"))
  {
    for (i = -(LINE_HEADER + 1);;)	/* �e�X�C�O���Y */
    {
      if (!fgets(buf, ANSILINELEN, fp))
	break;

      if (i < 0)	/* ���L���Y */
      {
	i++;
	continue;
      }

      if (*buf == QUOTE_CHAR1 || *buf == '\n' || !strncmp(buf, "��", 2))	 /* ���L�ި� */
	continue;

      sum += checksum_add(buf);
      if (++i >= MAX_CHECKSUM_LINE)
	break;
    }

    fclose(fp);
  }

  return checksum_put(sum);
}


static int
check_crosspost(fpath, bno)
  char *fpath;
  int bno;			/* �n��h���ݪO */
{
  char *blist, folder[64];
  ACCT acct;
  HDR hdr;

  if (HAS_PERM(PERM_ALLADMIN))
    return 0;

  /* �O�D�b�ۤv�޲z���ݪO���C�J��K�ˬd */
  blist = (bshm->bcache + bno)->BM;
  if (HAS_PERM(PERM_BM) && blist[0] > ' ' && is_bm(blist, cuser.userid))
    return 0;

  if (checksum_find(fpath))
  {
    /* �p�G�O cross-post�A������h BN_SECURITY �ê������v */
    brd_fpath(folder, BN_SECURITY, fn_dir);
    hdr_stamp(folder, HDR_COPY | 'A', &hdr, fpath);
    strcpy(hdr.owner, cuser.userid);
    strcpy(hdr.nick, cuser.username);
    sprintf(hdr.title, "%s %s Cross-Post", cuser.userid, Now());
    rec_bot(folder, &hdr, sizeof(HDR));
    btime_update(brd_bno(BN_SECURITY));

    bbstate &= ~STAT_POST;
    cuser.userlevel &= ~PERM_POST;
    cuser.userlevel |= PERM_DENYPOST;
    if (acct_load(&acct, cuser.userid) >= 0)
    {
      acct.tvalid = time(NULL) + CROSSPOST_DENY_DAY * 86400;
      acct_setperm(&acct, PERM_DENYPOST, PERM_POST);
    }
    board_main();
    mail_self(FN_ETC_CROSSPOST, str_sysop, "Cross-Post ���v", 0);
    vmsg("�z�]���L�� Cross-Post �w�Q���v");
    return 1;
  }
  return 0;
}
#endif	/* HAVE_DETECT_CROSSPOST */


/* ----------------------------------------------------- */
/* �o��B�^���B�s��B����峹				 */
/* ----------------------------------------------------- */


int
is_author(hdr)
  HDR *hdr;
{
  /* �o�̨S���ˬd�O���O guest�A�`�N�ϥΦ��禡�ɭn�S�O�Ҽ{ guest ���p */

  /* itoc.070426: ��b���Q�M����A�s���U�ۦP ID ���b���ä��֦��L�h�� ID �o���峹���Ҧ��v */
  return !strcmp(hdr->owner, cuser.userid) && (hdr->chrono > cuser.firstlogin);
}


#ifdef HAVE_REFUSEMARK
int		/* 1:�i�H�\Ū  0:����\Ū */
chkrestrict(hdr)
  HDR *hdr;
{
  return !(hdr->xmode & POST_RESTRICT) || is_author(hdr) || (bbstate & STAT_BM);
}
#endif  


#ifdef HAVE_UNANONYMOUS_BOARD
static void
do_unanonymous(shdr, is_anonymous)
  HDR *shdr;
  int is_anonymous;
{
  HDR hdr;
  char fpath[64], folder[64], title[80], *ttl, aid[16];
  int len;
  FILE *fp;

  brd_fpath(folder, BN_UNANONYMOUS, fn_dir);

  if(!(fp = fdopen(hdr_stamp_time(folder, 'A', &hdr, fpath, shdr->chrono), "w")))
    return;

  strcpy(title, ve_title);

  fprintf(fp, "�@��: %s (%s) �ݪO: %s\n", cuser.userid, cuser.username, currboard);
  fprintf(fp, "���D: %s\n", ve_title);
  fprintf(fp, "�ɶ�: %s\n\n", Btime(&shdr->chrono));

  aid_encode(shdr->chrono, aid);
  fprintf(fp, "#AID: %s (%s)\n", aid, currboard);
  fprintf(fp, "real: %s\n", cuser.userid);
  if(is_anonymous)
  {
    fprintf(fp, "anon: %s\n", anonymousid);
    fprintf(fp, "FQDN: %s\n", fromhost);
  }
  fputs("\n\n--\n", fp);

  fclose(fp);

  len = 41 - strlen(currboard);
  ttl = str_ttl(title);
  if(IS_ZHC_LO(ttl, len))
    ttl[len - 1] = ' ';		/* ����\������r����b */
  if(strlen(ttl) < len)
    memset(ttl + strlen(ttl), ' ', len - strlen(ttl));
  sprintf(ttl + len, ".%s�O", currboard);

  strcpy(hdr.owner, cuser.userid);
  strcpy(hdr.title, title);
  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(brd_bno(BN_UNANONYMOUS));
}


static void
unanony_score(hdr, anid, pnow, verb, reason)
  HDR *hdr;
  char *anid;
  time_t *pnow;
  char *verb;
  char *reason;
{
  FILE *fp;
  char fpath[64], fname[32], buf[256], *str;
  time_t chrono;
  int i, c;

  fname[1] = '/';
  fname[2] = 'A';
  for(chrono = hdr->chrono; ; chrono++)
  {
    fname[0] = radix32[chrono & 31];
    archiv32(chrono, fname + 3);
    brd_fpath(fpath, BN_UNANONYMOUS, fname);

    if(!(fp = fopen(fpath, "r")))
      break;

    for(i = 0; i < 4 && (c = fgetc(fp)) != EOF;)
    {
      if(c == '\n')
	i++;
    }

    if(fgets(buf, sizeof(buf), fp) && buf[0] == '#')
    {
      if(hdr->chrono == aid_decode(buf + 6))
      {
	str = strchr(buf + 6, '(');
	strtok(str, ")");
	if(!strcmp(str + 1, currboard))	/* dust.100406: ���L�O�W���ܴN�|�䤣��C���ޥ��C */
	  break;
      }
    }

    fclose(fp);
  }

  if(fp)
  {
    fclose(fp);
    if(fp = fopen(fpath, "a"))
    {
      fprintf(fp, "Time: %s\n", Btime(pnow));
      fprintf(fp, "real: %s\n", cuser.userid);
      fprintf(fp, "anon: %s\n", anid);
      fprintf(fp, "\033[3%s\033[1;30m:\033[m%s\n", verb, reason);
      fputs("\n--\n", fp);

      fclose(fp);
    }
  }
}

#endif



void
add_post(brdname, fpath, title, authurid, authurnick, xmode)  /* �o���ݪO */
  char *brdname;        /* �� post ���ݪO */
  char *fpath;          /* �ɮ׸��| */
  char *title;          /* �峹���D */
  char *authurid;       /* �@�̱b�� */
  char *authurnick;     /* �@�̼ʺ� */
  int xmode;
{
  HDR hdr;
  char folder[64];

  brd_fpath(folder, brdname, fn_dir);
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  /* �p�Gauthurid���Ŧr�� �A�h�]���ثe�ϥΪ�ID*/
  if (authurid[0]=='\0')
    strcpy(authurid, cuser.userid) ;

  strcpy(hdr.owner, authurid);

  /* �p�Gauthurnick���Ŧr�� �A�h�]���ثe�ϥΪ̼ʺ�*/
  if (authurnick[0]=='\0')
    strcpy(authurnick, cuser.username) ;

  strcpy(hdr.nick, authurnick);

  strcpy(hdr.title, title);

  hdr.xmode = xmode;

  rec_bot(folder, &hdr, sizeof(HDR));

  btime_update(brd_bno(brdname));
}



static int
do_post(xo, title)
  XO *xo;
  char *title;
{
  /* Thor.981105: �i�J�e�ݳ]�n curredit �� quote_file */
  HDR hdr, buf;
  char fpath[64], *folder, *nick, *rcpt = NULL;
  const char const *default_prefix[PREFIX_MAX] = DEFAULT_PREFIX;
  char prefix[PREFIX_MAX][PREFIX_LEN + 1], uprefix[PREFIX_LEN + 1];	/* �h�@���ť� */

char *ptr1, *ptr2;

  int mode;
#ifdef HAVE_TEMPLATE
  int ve_op = 1;
#endif
  time_t spendtime, prev, chrono;
  FILE *fp;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  if (!(bbstate & STAT_POST))
  {
#ifdef NEWUSER_LIMIT
    if (cuser.lastlogin - cuser.firstlogin < 3 * 86400)
      vmsg("�s��W���A�T���l�i�i�K�峹");
    else
#endif
      vmsg("�藍�_�A�z�S���b���o��峹���v��");
    return XO_FOOT;
  }

  brd_fpath(fpath, currboard, FN_POSTLAW);
  clear();
  if(lim_cat(fpath, POSTLAW_LINEMAX) < 0)
    film_out(FILM_POST, 0);

#ifdef POST_PREFIX
  /* �ɥ� mode�Brcpt�Bfpath */

  if(!title)
  {
    char buf[64];
    int prefix_num, i = 0;

    brd_fpath(fpath, currboard, "prefix");
    if(fp = fopen(fpath, "r"))
    {
      fgets(buf, sizeof(buf), fp);
      sscanf(buf, "%d", &prefix_num);

      for(; i < prefix_num; i++)
      {
	if(!fgets(buf, sizeof(buf), fp))
	  break;
	strtok(buf, "\n");
	str_ncpy(prefix[i], buf, PREFIX_LEN);
      }

      fclose(fp);
    }
    else
      prefix_num = NUM_PREFIX_DEF;

    /* �񺡦� prefix_num �� */
    for(; i < prefix_num; i++)
      strcpy(prefix[i], default_prefix[i]);

    if(prefix_num > 0)
    {
      int x = 5, y = 21, len;
      int middle_cut = prefix_num > 6;

      move(19,0);
      prints("�b\033[1;33m�i %s �j\033[m�ݪO�o��峹", currboard);
      move(21, 0);
      outs("���O�G");

      for(i = 0; i < prefix_num; i++)
      {
	if(y < 0)	/* �H�ٲ��Ÿ��L�X */
	{
	  prints(" \033[1;35m%d.\033[30m�K", (i + 1) % 10);
	  continue;
	}

	len = strlen(prefix[i]);
	x += len + 3;
	if(y == b_lines)
	{
	  if(x + (prefix_num - i - 1) * 6 > b_cols)
	  {
	    y = -1;
	    i--;
	    continue;
	  }
	}
	else if(x > b_cols || (i == 5 && middle_cut))
	{
	  y++;
	  x = len + 8;
	  middle_cut = 0;
	  outs("\n     ");
	}

	if(i > 0) outc(' ');
	prints("\033[1;35m%d.\033[37m%s", (i + 1) % 10, prefix[i]);
      }

      attrset(7);


      /* dust.070902: ���F��J�W����K�A0���O10 */

      i = vget(20, 0, "�п�ܤ峹���O(�Φۦ��J)�G", uprefix + 1, PREFIX_LEN - 2, DOECHO) - '0';
      i = (i + 9) % 10;

      /* floatJ: ���fgisc�@�˥i�ۦ��J�A��K user (�зN�ӷ��P��FGISC) */
      if(strlen(uprefix + 1) > 1)	/* �ϥΪ̦ۦ��Jprefix */
      {
	uprefix[0] = '[';
	strcat(uprefix, "] ");
	rcpt = uprefix;
      }
      else if(i >= 0 && i < prefix_num)	/* ��ܤF�s�b���峹���O */
      {
#  ifdef HAVE_TEMPLATE
	ve_op = i;
	curredit |= EDIT_TEMPLATE;
#  endif
	rcpt = prefix[i];
	strcat(rcpt, " ");
      }
    }
  }

  if (!ve_subject(21, title, rcpt))
#else
  if (!ve_subject(21, title, NULL))
#endif
    return XO_HEAD;

  /* ����� Internet �v���̡A�u��b�����o��峹 */
  /* Thor.990111: �S��H�X�h���ݪO, �]�u��b�����o��峹 */

  if (!HAS_PERM(PERM_INTERNET) || (currbattr & BRD_NOTRAN))
    curredit &= ~EDIT_OUTGO;

  utmp_mode(M_POST);
  fpath[0] = '\0';
  time(&spendtime);
#ifdef HAVE_TEMPLATE
  if (vedit(fpath, ve_op) < 0)		/* dust.080908: ve_op���峹�d���ϥ� */
#else
  if (vedit(fpath, 1) < 0)
#endif
  {
    unlink(fpath);
    vmsg(msg_cancel);
    return XO_HEAD;
  }

  spendtime = time(0) - spendtime;	/* itoc.010712: �`�@�᪺�ɶ�(���) */

  /* build filename */

  folder = xo->dir;
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  /* set owner to anonymous for anonymous board */

#ifdef HAVE_ANONYMOUS
  /* dust.100406: �L�׭�o��̦��S���ΦW�o�U���o�n�O���F... */

  if(currbattr & BRD_ANONYMOUS)
    do_unanonymous(&hdr, curredit & EDIT_ANONYMOUS);

  /* Thor.980727: lkchu�s�W��[²�檺��ܩʰΦW�\��] */
  if (curredit & EDIT_ANONYMOUS)
  {
    rcpt = anonymousid;	/* itoc.010717: �۩w�ΦW ID */
    nick = "";	/* dust.100408: �p�G�O�ΦWID�������ӴN���ݭn�ʺ٤F�~�� */

#if 0	/* dust.100408: ������HUnanonymous�O�O���ΦW */
    /* Thor.980727: lkchu patch: log anonymous post */
    /* Thor.980909: gc patch: log anonymous post filename */
    log_anonymous(hdr.xname);
#endif
  }
  else
#endif
  {
    rcpt = cuser.userid;
    nick = cuser.username;
  }
  title = ve_title;
  mode = (curredit & EDIT_OUTGO) ? POST_OUTGO : 0;
#ifdef HAVE_REFUSEMARK
  if (curredit & EDIT_RESTRICT)
    mode |= POST_RESTRICT;
#endif

  hdr.xmode = mode;
  hdr.stamp = 0;	/* dust.091125: for CRH */
  strcpy(hdr.owner, rcpt);
  strcpy(hdr.nick, nick);
  strcpy(hdr.title, title);

  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(currbno);

  if (mode & POST_OUTGO)
    outgo_post(&hdr, currboard);

#if 1	/* itoc.010205: post ���峹�N�O���A�Ϥ��X�{���\Ū���ϸ� */
  chrono = hdr.chrono;
  prev = ((mode = rec_num(folder, sizeof(HDR)) - 2) >= 0 && !rec_get(folder, &buf, sizeof(HDR), mode)) ? buf.chrono : chrono;
  brh_add(prev, chrono, chrono);
#endif

  clear();
  outs("���Q�K�X�峹�A");

  if (currbattr & BRD_NOCOUNT || wordsnum < 15)
  {				/* itoc.010408: �H���������{�H */
    outs("�峹���C�J�����A�q�Х]�[�C");
  }
  else
  {
    /* itoc.010408: �̤峹����/�ҶO�ɶ��ӨM�w�n���h�ֿ��F����~�|���N�q */
    mode = BMIN(wordsnum, spendtime) / 4;
    /* dust.090301: �ק令�C�|�r�Υ|��@�ȹ� */
    prints("�o�O�z���� %d �g�峹�A�o %d �ȡC", ++cuser.numposts, mode);
    addmoney(mode);
  }

  /* �^�����@�̫H�c */

  mode = 1;
  if (curredit & EDIT_BOTH)
  {
    rcpt = quote_user;

    if (strchr(rcpt, '@'))	/* ���~ */
    {
      if(bsmtp(fpath, title, rcpt, 0) >= 0)
        mode = 0;	/* dust.101220: �浹mailsender�h�R�� */
    }
    else			/* �����ϥΪ� */
    {
      move(2, 0);
      if(mail_him(fpath, rcpt, title, 0) >= 0)
        outs("�^�������C");
      else
        outs("�@�̵L�k���H");
    }
  }

  if(mode)
    unlink(fpath);

  vmsg(NULL);

#ifdef HAVE_ANONYMOUS
  /* gaod.091205: �o�ΦW��ᤣ���� EDIT_ANONYMOUS �|�ɭP����峹 header ��T���~
 * 		  Thanks for om@cpu.tfcis.org. */
  if (curredit & EDIT_ANONYMOUS)
    curredit &= ~EDIT_ANONYMOUS;
#endif

  return XO_INIT;
}



int
do_reply(xo, hdr)
  XO *xo;
  HDR *hdr;
{
  curredit = EDIT_POSTREPLY;

  switch (vans("�� �^���� (F)�ݪO (M)�@�̫H�c (B)�G�̬ҬO (Q)�����H[F] "))
  {
  case 'm':
    hdr_fpath(quote_file, xo->dir, hdr);
    return do_mreply(hdr, 0);

  case 'q':
    return XO_FOOT;

  case 'b':
    /* �Y�L�H�H���v���A�h�u�^�ݪO */
    if (HAS_PERM(strchr(hdr->owner, '@') ? PERM_INTERNET : PERM_LOCAL))
      curredit |= EDIT_BOTH;
    break;
  }

  /* Thor.981105: ���׬O��i��, �άO�n��X��, ���O�O���i�ݨ쪺, �ҥH�^�H�]��������X */
  if (hdr->xmode & (POST_INCOME | POST_OUTGO))
    curredit |= EDIT_OUTGO;

  hdr_fpath(quote_file, xo->dir, hdr);
  strcpy(quote_user, hdr->owner);
  strcpy(quote_nick, hdr->nick);
  return do_post(xo, hdr->title);
}


static int
post_reply(xo)
  XO *xo;
{
  if (bbstate & STAT_POST)
  {
    HDR *hdr;

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif

    if (hdr->xmode & POST_RESERVED)       /* �Q��w(!)���峹 */
    {
      vmsg("����w�Q��w�A�L�k�^��");
      return XO_NONE;
    }

    return do_reply(xo, hdr);
  }
  return XO_NONE;
}


static int
post_add(xo)
  XO *xo;
{
  curredit = EDIT_OUTGO;
  *quote_file = '\0';
  return do_post(xo, NULL);
}


/* ----------------------------------------------------- */
/* �L�X hdr ���D					 */
/* ----------------------------------------------------- */


int
tag_char(chrono)
  int chrono;
{
  return TagNum && !Tagger(chrono, 0, TAG_NIN) ? '*' : ' ';
}


#ifdef HAVE_DECLARE
static inline int
cal_day(date)		/* itoc.010217: �p��P���X */
  char *date;
{
#if 0
   ���Ǥ����O�@�ӱ�����@�ѬO�P���X������.
   �o�����O:
         c                y       26(m+1)
    W= [---] - 2c + y + [---] + [---------] + d - 1
         4                4         10
    W �� ���ҨD������P����. (�P����: 0  �P���@: 1  ...  �P����: 6)
    c �� ���w�������~�����e���Ʀr.
    y �� ���w�������~��������Ʀr.
    m �� �����
    d �� �����
   [] �� ��ܥu���Ӽƪ���Ƴ��� (�a�O���)
    ps.�ҨD������p�G�O1���2��,�h�������W�@�~��13���14��.
       �ҥH������m�����Ƚd�򤣬O1��12,�ӬO3��14
#endif

  /* �A�� 2000/03/01 �� 2099/12/31 */

  int y, m, d;

  y = 10 * ((int) (date[0] - '0')) + ((int) (date[1] - '0'));
  d = 10 * ((int) (date[6] - '0')) + ((int) (date[7] - '0'));
  if (date[3] == '0' && (date[4] == '1' || date[4] == '2'))
  {
    y -= 1;
    m = 12 + (int) (date[4] - '0');
  }
  else
  {
    m = 10 * ((int) (date[3] - '0')) + ((int) (date[4] - '0'));
  }
  return (-1 + y + y / 4 + 26 * (m + 1) / 10 + d) % 7;
}
#endif


/* dust.100920: �令pfterm base�A�[�J�I�_�Ÿ� */
/* ���A�Ҽ{�O�_���w�q HAVE_DECLARE */
void
hdr_outs(hdr, lim)		/* print HDR's subject */
  HDR *hdr;
  int lim;			/* �L�X�̦h lim �r�����D */
{
  uschar *title, *mark;
  int ch, len;
  int in_dbc;
  int square_level;	/* �O�ĴX�h���A�� */
#ifdef CHECK_ONLINE
  UTMP *online;
#endif

  /* --------------------------------------------------- */
  /* �L�X���						 */
  /* --------------------------------------------------- */

  /* itoc.010217: ��άP���X�ӤW�� */
  prints("\033[1;3%dm%s\033[m ", cal_day(hdr->date) + 1, hdr->date + 3);

  /* --------------------------------------------------- */
  /* �L�X�@��						 */
  /* --------------------------------------------------- */

#ifdef CHECK_ONLINE
  if (online = utmp_seek(hdr))
    outs(COLOR7);
#endif

  mark = hdr->owner;
  len = IDLEN + 1;
  in_dbc = 0;

  while (ch = *mark)
  {
    if (--len <= 0)
    {
      /* ��W�L len ���ת������������� */
      /* itoc.060604.����: �p�G��n���b����r���@�b�N�|�X�{�ýX�A���L�o���p�ܤֵo�͡A�ҥH�N���ޤF */
      ch = '.';
    }
    else
    {
      /* ���~���@�̧� '@' ���� '.' */
      if (in_dbc || IS_ZHC_HI(ch))	/* ����r���X�O '@' ������ */
	in_dbc ^= 1;
      else if (ch == '@')
	ch = '.';
    }
      
    outc(ch);

    if (ch == '.')
      break;

    mark++;
  }

  while (len--)
    outc(' ');

#ifdef CHECK_ONLINE
  if (online)
    outs(str_ransi);
#endif

  /* --------------------------------------------------- */
  /* �L�X���D������					 */
  /* --------------------------------------------------- */

  if (!chkrestrict(hdr) && !(currbattr & BRD_XRTITLE))
  {
    title = RESTRICTED_TITLE;
    len = 2;
  }
  else
  {
    /* len: ���D�O type[] �̭������@�� */
    title = str_ttl(mark = hdr->title);
    len = (title == mark) ? 2 : (*mark == 'R') ? 0 : 1;
    if (!strcmp(currtitle, title))
      len += 3;
  }
  outs(title_type[len]);
  outc(' ');

  /* --------------------------------------------------- */
  /* �L�X���D						 */
  /* --------------------------------------------------- */

  mark = title + lim;

  square_level = -1;
  if(len < 3 && *title == '[')
  {
    square_level = 0;
    attrset(15);
  }

  /* ��W�L cc ���ת��������� */
  for(in_dbc = 0; (ch = *title) && title < mark; title++)
  {
    if(square_level >= 0 && !in_dbc)
    {
      switch(ch)
      {
      case '[':
	square_level++;
	break;
      case ']':
	square_level--;
	break;
      }
    }

    outc(ch);
    if(square_level == 0)
    {
      attrset(7);
      square_level--;
    }

    if(in_dbc)
      in_dbc = 0;
    else if(IS_ZHC_HI(ch))
      in_dbc = 1;
  }

  if(*title)
  {
    if(in_dbc) outc('\b');
    attrset(8);
    outc('>');
  }

  attrset(7);

  outc('\n');
}


/* ----------------------------------------------------- */
/* �ݪO�\���						 */
/* ----------------------------------------------------- */


static int post_body();
static int post_head();


static int
post_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(HDR));
  return post_head(xo);
}


static int
post_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(HDR));
  return post_body(xo);
}



/* dust.100401: �� post_attr() ���^ post_item() */
/* ��: �H�U�{���X�L��HAVE_REFUSEMARK�BHAVE_LABELMARK undefine�����p */
/*     �]����HAVE_SCORE undefine�����p */
static void
post_item(num, hdr)
  int num;
  HDR *hdr;
{
  int attr, bottom;
  uschar color;
  register int m, x, s, t, lock, spms, cun, bun;
  usint mode;


  /* �e�m�B�z */
  mode = hdr->xmode;

  m = mode & POST_MARKED;
  x = mode & POST_RESTRICT;
  s = mode & POST_DONE;
  t = mode & POST_DELETE;
  lock = mode & POST_RESERVED;
  spms = mode & POST_SPMS;

  bottom = mode & POST_BOTTOM;
  cun = 0;
  if(!chkrestrict(hdr) || bottom)	/* �m����P�L�k�\Ū���峹�����wŪ */
    bun = 0;
  else
  {
    bun = rhs_unread(-1, hdr->chrono, hdr->stamp);

  /* dust.100331: �峹��Ū�O���ȨϥΤ@���C�� */
    if(!(cuser.ufo & UFO_UNIUNREADM) && bun == 2)	/* CRH unread */
    {
      cun = 1;
      bun = 0;
    }
  }


  if(lock && (bun || cun))
    color = 6;		//*[36m
  else if(lock && m && x)
    color = 9;		//*[1;31m
  else if(lock && x)
    color = 1;		//*[31m
  else if(lock && m)
    color = 15;		//*[1;37m
  else if(cun && (s || x))
    color = 6;		//*[36m
  else if(cun && spms)
    color = 2;		//*[32m
  else if(cun && t)
    color = 8;		//*[1;30m
  else if(cun && m)
    color = 6;		//*[36m
  else if(s && m && x)
    color = 11;		//*[1;33m
  else if(m && (x || s || spms))
    color = 15;		//*[1m
  else if(t && spms)
    color = 12;		//*[1;34m
  else if(x && s)
    color = 1;		//*[31m
  else if(x && t)
    color = 12;		//*[1;34m
  else if(cun)
    color = 2;		//*[32m
  else
    color = 7;		//*[m (*[37m)

  attr = bun ? 0 : 0x20;	/* �w�\Ū���p�g�A���\Ū���j�g */

  /* ����u����: ! > S > X > # > T > M */
  if (lock)
    attr = '!';
  else if (s)
    attr |= 'S';        /* qazq.030815: ����B�z���аO�� */
  else if (x)
    attr |= 'X';
  else if (spms)	/* dust.080211: �S��аO(�j�M��)'#' */
  {
    if(attr)
      attr = '#';	/* dust.100331: �ק良Ū�ɪ��ˤl */
    else
      attr = 'b';
  }
  else if (t)
    attr |= 'T';
  else if (m)
    attr |= 'M';
  else if (!attr || cun)
    attr = '+';
  else
    attr = ' ';


  /* ��X������ */

  if (bottom)			/* �L�X�峹�s�� */
    outs("    "BOTTOM);
  else
    prints("%6d", num);

  outc(tag_char(hdr->chrono));		/* �L�XTag�аO */

  attrset(color);
  outc(attr);			/* �L�X�峹�ݩ� */
  attrset(7);

  if (hdr->xmode & POST_SCORE)		/* �L�X�������� */
  {
    num = hdr->score;

    if(num <= -100)
      outs("\033[1;30mXX");
    else if(num < 0)
      prints("\033[1;32m%2d", -num);
    else if(num == 0)
      outs("\033[1;33m 0");
    else if(num < 100)
      prints("\033[1;31m%2d", num);
    else
      outs("\033[1;31m�z");

    outs("\033[m ");
  }
  else
    outs("   ");

  hdr_outs(hdr, d_cols + 45);
}



static int
post_body(xo)
  XO *xo;
{
  HDR *hdr;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    if (bbstate & STAT_POST)
    {
      if (vans("�n�s�W��ƶ�(Y/N)�H[N] ") == 'y')
	return post_add(xo);
    }
    else
    {
      vmsg("���ݪO�|�L�峹");
    }
    return XO_QUIT;
  }

  hdr = (HDR *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    post_item(++num, hdr++);
  } while (num < max);
  clrtobot();

  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: �� b_lines ��W feeter */
}



static int		/* �^�� 0:�S������r�Q�ٲ�  1:������r�Q�ٲ� */
limit_outs(str, lim)
  char *str;
  int lim;
{
  int dword = 0, reduce = 0;
  char *buf, *end, *c_pool;

  /* �ʺA�t�m�Ŷ��A�o�˴N�����ΩT�w�j�p���w�İϡA�i�H�ѨM�e�ù��׺ݾ��_�u�����D */
  c_pool = (char*) malloc(lim + 1);

  if(!c_pool)	/* �Ŷ��t�m���Ѫ��ܴN�������� */
    exit(1);

  buf = c_pool;
  end = str+lim;
  while(str<end && *str)
  {
    if(dword)
    {
      buf[-1] = str[-1];
      *buf = *str;
      dword = 0;
      reduce = 0;
    }
    else if(*str & 0x80)	/* ������r */
    {
      dword = 1;
      reduce = 1;
    }
    else
      *buf = *str;;

    str++;
    buf++;
  }

  *buf = '\0';
  prints("%s", c_pool);

  free(c_pool);

  return reduce;
}


#define out_space(x) prints("%*s", x, "");

static int
post_head(xo)
  XO *xo;
{
  char *btitle;
  int title_start, title_end, title_len;
  int bm_end, right_start;
  int space;

  btitle = xo->xyz;
  title_len = strlen(btitle);
  title_start = (78+d_cols)/2 - title_len/2;
  title_end = title_start + title_len;

  bm_end = 4 + strlen(currBM);
  right_start = 69 + d_cols - strlen(currboard);

  clear();

  /* �L�X header(�e�q) */
  outs("\033[1;33m��\033[32m ");
  space = 0;
  if(bm_end+2 > title_start)
  {
    bm_end = title_start - 2;
    space += limit_outs(currBM, bm_end - 6);
    outs("..");
  }
  else
    outs(currBM);
  outs(" \033[36;44m ");

  /* �L�X header(�e�q) */
  out_space(space + title_start - bm_end - 2);
  space = 0;
  if(title_end > right_start)
  {
    title_end = right_start;
    space += limit_outs(btitle, title_end - right_start);
    outs("\033[30m..\033[36");
  }
  else
    outs(btitle);
  out_space(space + right_start - title_end);

  /* �L�X header(��q) */
  prints(" \033[30;40m �ݪO%s%s%s\033[m\n", brd_levelbar[0], currboard, brd_levelbar[1]);

  /* �L�X neck */
  prints(NECKER_POST, d_cols, "", currbattr & BRD_NOSCORE ? "��" : "��", bshm->mantime[currbno]);

  return post_body(xo);
}



/* ----------------------------------------------------- */
/* ��Ƥ��s���Gbrowse / history				 */
/* ----------------------------------------------------- */


static int
post_visit(xo)
  XO *xo;
{
  int ans, row, max;
  HDR *hdr;

  ans = vans("�]�w�Ҧ��峹 (U)��Ū (V)�wŪ (W)�e�wŪ�᥼Ū (Q)�����H[Q] ");
  if (ans == 'v' || ans == 'u' || ans == 'w')
  {
    row = xo->top;
    max = xo->max - row + 3;
    if (max > b_lines)
      max = b_lines;

    hdr = (HDR *) xo_pool + (xo->pos - row);
    /* weiyu.041010: �b�m����W�� w ���������wŪ */
    rhs_visit((ans == 'u') ? 1 : (ans == 'w' && !(hdr->xmode & POST_BOTTOM)) ? hdr->chrono : 0);

    hdr = (HDR *) xo_pool;
    return post_body(xo);
  }
  return XO_FOOT;
}


static void
post_history(xo, hdr)		/* �N hdr �o�g�[�J brh */
  XO *xo;
  HDR *hdr;
{
  time_t prev, chrono, next, stamp;
  int pos, top;
  char *dir;
  HDR buf;

  if (hdr->xmode & POST_BOTTOM)	/* �m���夣�[�J�\Ū�O�� */
    return;

  chrono = hdr->chrono;
  stamp = hdr->stamp;

  switch (rhs_unread(-1, chrono, stamp))	/* �p�G�w�b brh ���A�N�L�ݰʧ@ */
  {
  case 0:
    break;

  case 1:
    dir = xo->dir;
    pos = xo->pos;
    top = xo->top;

    pos--;
    if (pos >= top)
    {
      prev = hdr[-1].chrono;
    }
    else
    {
      /* amaki.040302.����: �b�e���H�W�A�u�nŪ�w�� */
      if (!rec_get(dir, &buf, sizeof(HDR), pos))
	prev = buf.chrono;
      else
	prev = chrono;
    }

    pos += 2;
    if (pos < top + XO_TALL && pos < xo->max)
    {
      next = hdr[1].chrono;
    }
    else
    {
      /* amaki.040302.����: �b�e���H�U�A�u�nŪ�w�� */
      if (!rec_get(dir, &buf, sizeof(HDR), pos))
	next = buf.chrono;
      else
	next = chrono;
    }

    brh_add(prev, chrono, next);

  case 2:
    crh_add(chrono, stamp);
  }
}


static int post_switch(XO*);
static int post_aidjump(XO*);

static int
post_browse(xo)
  XO *xo;
{
  HDR *hdr;
  int xmode, pos, key;
  char *dir, fpath[64];

  dir = xo->dir;

  for (;;)
  {
    pos = xo->pos;
    hdr = (HDR *) xo_pool + (pos - xo->top);
    xmode = hdr->xmode;

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif

    hdr_fpath(fpath, dir, hdr);

    /* Thor.990204: ���Ҽ{more �Ǧ^�� */   
    if ((key = more(fpath, MFST_AUTO)) < 0)
      break;

    post_history(xo, hdr);
    strcpy(currtitle, str_ttl(hdr->title));

    switch (xo_getch(xo, key))
    {
    case XO_BODY:
      continue;

    case 'y':
    case 'r':
      if (bbstate & STAT_POST)
      {
        if (hdr->xmode & POST_RESERVED)
        {
          vmsg("����w�Q��w�A�L�k�^��");
          break;
        }

	if (do_reply(xo, hdr) == XO_INIT)	/* �����\�a post �X�h�F */
	  return post_init(xo);
      }

      break;

#if 0
    case 'm':
      if ((bbstate & STAT_BOARD) && !(xmode & POST_MARKED))
      {
	/* hdr->xmode = xmode ^ POST_MARKED; */
	/* �b post_browse �ɬݤ��� m �O���A�ҥH����u�� mark */
	hdr->xmode = xmode | POST_MARKED;
	currchrono = hdr->chrono;
	rec_put(dir, hdr, sizeof(HDR), pos, cmpchrono);
      }
      break;
#endif

#ifdef HAVE_SCORE
    case '%':
      post_score(xo);
      return post_init(xo);
#endif

    case 's':
      move(0, 0);
      outs("\033[30;47m�i ��ܬݪO �j\033[m\n");
      return post_switch(xo);

    case '#':
      if(post_aidjump(xo) == XZ_POST)
	return XZ_POST;
      else
	break;

    case 'E':
      post_edit(xo);
      break;

    case 'C':	/* itoc.000515: post_browse �ɥi�s�J�Ȧs�� */
      {
	FILE *fp;
	if (fp = tbf_open())
	{ 
	  f_suck(fp, fpath); 
	  fclose(fp);
	}
      }
      break;

    case 'h':
      xo_help("post");
      break;
    }
    break;
  }

  return post_head(xo);
}


/* ----------------------------------------------------- */
/* ��ذ�						 */
/* ----------------------------------------------------- */


static int
post_gem(xo)
  XO *xo;
{
  int level;
  char fpath[64];

  strcpy(fpath, "gem/");
  strcpy(fpath + 4, xo->dir);

  level = 0;
  if (bbstate & STAT_BOARD)
    level ^= GEM_W_BIT;
  if (HAS_PERM(PERM_SYSOP))
    level ^= GEM_X_BIT;
  if (bbstate & STAT_BM)
    level ^= GEM_M_BIT;

  XoGem(fpath, "��ذ�", level);
  return post_init(xo);
}


/* ----------------------------------------------------- */
/* �i�O�e��						 */
/* ----------------------------------------------------- */


static int
post_memo(xo)
  XO *xo;
{
  char fpath[64];

  brd_fpath(fpath, currboard, fn_note);
  /* Thor.990204: ���Ҽ{more �Ǧ^�� */   
  if (more(fpath, MFST_NULL) < 0)
  {
    vmsg("���ݪO�S���u�i�O�e���v");
    return XO_FOOT;
  }

  return post_head(xo);
}


/* ----------------------------------------------------- */
/* �\��Gtag / switch / cross / forward			 */
/* ----------------------------------------------------- */


static int
post_tag(xo)
  XO *xo;
{
  HDR *hdr;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if (xo->key == XZ_XPOST)
    pos = hdr->xid;

  if (tag = Tagger(hdr->chrono, pos, TAG_TOGGLE))
  {
    move(3 + cur, 6);
    outc(tag > 0 ? '*' : ' ');
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE; /* lkchu.981201: ���ܤU�@�� */
}


static int
post_switch(xo)
  XO *xo;
{
  int bno;
  BRD *brd;
  char bname[BNLEN + 1];

  bbstate &= ~STAT_CANCEL;
  if (brd = ask_board(bname, BRD_R_BIT, NULL))
  {
    if ((bno = brd - bshm->bcache) >= 0 && currbno != bno)
    {
      XoPost(bno);
      return XZ_POST;
    }
  }
  else if(!(bbstate & STAT_CANCEL))
  {
    vmsg(err_bid);
  }
  return post_head(xo);
}


int
post_cross(xo)
  XO *xo;
{
  /* �ӷ��ݪO */
  char *dir, *ptr;
  HDR *hdr, xhdr;
  usint srcbattr;
  char srcboard[BNLEN + 1];

  /* ����h���ݪO */
  int xbno;
  usint xbattr;
  char xboard[BNLEN + 1], xfolder[64];
  HDR xpost;
  int xhide;	/* 0:����ܤ��}/�n�ͪO  1:����ܯ��K�ݪO */

  int method;	/* 0:������  1:�q���}�ݪO/��ذ�/�H�c����峹  2:�q���K�ݪO����峹 */
  int rec_at_src;

  FILE *fpr, *fpw;
  char fpath[64], bname[32], buf[256];
  char userid[IDLEN + 1], username[UNLEN + 1];
  int tag, rc, locus;
  char *opts[] = {"1������", "2����峹", NULL};


  if (!cuser.userlevel)	/* itoc.000213: �קK guest ����h sysop �O */
    return XO_NONE;

  tag = AskTag("���");
  if (tag < 0)
    return XO_FOOT;

  hdr = tag ? &xhdr : (HDR *) xo_pool + (xo->pos - xo->top);

  if(!tag)
  {
    if(hdr->xmode & POST_RESTRICT)
    {
      vmsg("�[�K�峹���������");
      return XO_NONE;
    }
    else if(hdr->xmode & GEM_FOLDER)	/* �D plain/text ������� */
    {
      return XO_NONE;
    }
  }

  dir = xo->dir;

  if (!ask_board(xboard, BRD_W_BIT, "\n\n\033[1;33m�ЬD��A���ݪO�A��������W�L�T�O�C\033[m\n\n") ||
    (*dir == 'b' && !strcmp(xboard, currboard)))	/* �H�c�B��ذϤ��i�H�����currboard */
    return XO_HEAD;

  /* dust.110120: �峹�H�~�T������� */
  if((!tag && *dir == 'b' && is_author(hdr)) || HAS_PERM(PERM_ALLBOARD))
    method = krans(2, "21", opts, "") - '1';
  else
    method = 1;

  if (!tag)	/* lkchu.981201: �������N���n�@�@�߰� */
  {
    if (method)
      sprintf(ve_title, "Fw: %.68s", str_ttl(hdr->title));	/* �w�� Re:/Fw: �r�˴N�u�n�@�� Fw: */
    else
      strcpy(ve_title, hdr->title);

    if (!vget(2, 0, "���D�G", ve_title, TTLEN + 1, GCARRY))
      return XO_HEAD;
  }

#ifdef HAVE_REFUSEMARK
  switch(rc = vget(2, 0, "(S)�s�� (L)���� (X)�K�� (Q)�����H[Q] ", buf, 3, LCECHO))
  {
  case 's':
  case 'l':
  case 'x':
#else
  switch(rc = vget(2, 0, "(S)�s�� (L)���� (Q)�����H[Q] ", buf, 3, LCECHO))
  {
  case 's':
  case 'l':
#endif
    break;
  default:
    return XO_HEAD;
  }

  if (method && *dir == 'b')	/* �q�ݪO��X�A���ˬd���ݪO�O�_�����K�O */
  {
    if (bshm->bcache[currbno].readlevel == PERM_SYSOP)
      method = 2;
  }

  /* �]�w����ؼЪ��ݩ� */
  xbno = brd_bno(xboard);
  xbattr = bshm->bcache[xbno].battr;
  if (bshm->bcache[xbno].readlevel == PERM_SYSOP)
    xhide = 1;
  else
    xhide = 0;

  /* Thor.990111: �b�i�H��X�e�A�n�ˬd���S����X���v�O? */
  if (rc == 's' && (!HAS_PERM(PERM_INTERNET) || (xbattr & BRD_NOTRAN)))
    rc = 'l';

  /* itoc.030325: ����I�s ve_header�A�|�ϥΨ� currboard�Bcurrbattr�A���ƥ��_�� */
  strcpy(srcboard, currboard);
  srcbattr = currbattr;

  rec_at_src = !(currbattr & BRD_CROSSNOREC);
  strcpy(currboard, xboard);
  currbattr = xbattr;

  userid[0] = '\0';

  locus = 0;
  do	/* lkchu.981201: ������ */
  {
    if (tag)
    {
      EnumTag(hdr, dir, locus, sizeof(HDR));

      if (method)
	sprintf(ve_title, "Fw: %.68s", str_ttl(hdr->title));	/* �w�� Re:/Fw: �r�˴N�u�n�@�� Fw: */
      else
	strcpy(ve_title, hdr->title);
    }

    if (hdr->xmode & GEM_FOLDER)	/* �D plain text ������ */
      continue;

#ifdef HAVE_REFUSEMARK
    if (hdr->xmode & POST_RESTRICT)
      continue;
#endif

    hdr_fpath(fpath, dir, hdr);

#ifdef HAVE_DETECT_CROSSPOST
    if (check_crosspost(fpath, xbno))
      break;
#endif

    brd_fpath(xfolder, xboard, fn_dir);
    fpw = fdopen(hdr_stamp(xfolder, 'A', &xpost, buf), "w");

    if (method)		/* �@����� */
    {
      int finish = 0;

      ve_header(fpw);

      /* itoc.040228: �p�G�O�q��ذ�����X�Ӫ��ܡA�|�������� [currboard] �ݪO�A
	  �M�� currboard �����O�Ӻ�ذϪ��ݪO�C���L���O�ܭ��n�����D�A�ҥH�N���ޤF :p */
      fprintf(fpw, "�� ��������� [%s] %s\n\n",
	*dir == 'u' ? cuser.userid : method == 2 ? "���K" : srcboard,
	*dir == 'u' ? "�H�c" : "�ݪO");

      /* Kyo.051117: �Y�O�q���K�ݪO��X���峹�A�R���峹�Ĥ@��ҰO�����ݪO�W�� */
      if (method == 2 && (fpr = fopen(fpath, "r")))
      {
	if (fgets(buf, sizeof(buf), fpr) &&
	  ((ptr = strstr(buf, str_post1)) || (ptr = strstr(buf, str_post2))) && (ptr > buf))
	{
	  ptr[-1] = '\n';
	  *ptr = '\0';

	  do
	  {
	    fputs(buf, fpw);
	  } while (fgets(buf, sizeof(buf), fpr));
	  finish = 1;
	}
	fclose(fpr);
      }

      if (!finish)
	f_suck(fpw, fpath);

      /* ��ؼХ[�W������`�� */
      if(*dir == 'u')	/* dust.100321: �q�H�c��X�Ӫ��令�[�W��ñ */
	ve_banner(fpw, 0);
      else
      {
	int len;

	if(method == 2)
	  strcpy(bname, "(Unknown)");
	else
	  sprintf(bname, "[%s]", srcboard);
	len = 15 - strlen(bname);
	memset(buf, ' ', len);

	sprintf(buf + len, FORWARD_BANNER_F, bname, cuser.userid, Now());
	fputs("\n--\n", fpw);
	fputs(buf, fpw);
      }

      fclose(fpw);

      strcpy(xpost.owner, cuser.userid);
      strcpy(xpost.nick, cuser.username);

      /* �q�ݪO��X�ɡA�O����X��T���� */
      if (*dir == 'b' && rec_at_src)
      {			/* dust.100331: �l�[����X�h�����d�U������� */
	int len;

	fpw = fopen(fpath, "a");

	if(xhide)
	  strcpy(bname, "(Unknown)");
	else
	  sprintf(bname, "[%s]", xboard);
	len = 17 - strlen(bname);
	memset(buf, ' ', len);
	sprintf(buf + len, FORWARD_BANNER_T, bname, cuser.userid, Now());
	fputs(buf, fpw);

	fclose(fpw);
      }
    }
    else		/* ������ */
    {
      if(!(fpr = fopen(fpath, "r")))
      {
	fclose(fpw);
	continue;
      }

      /* ���ȭ������L�H�峹 */
      if(HAS_PERM(PERM_ALLBOARD) && strcmp(hdr->owner, cuser.userid))
      {
	if(!userid[0])	/* �ƥ� ID �M�ʺ� */
	{
	  strcpy(userid, cuser.userid);
	  strcpy(username, cuser.username);
	}

	strcpy(cuser.userid, hdr->owner);
	strcpy(cuser.username, hdr->nick);
      }

      ve_header(fpw);

      if(!fgets(buf, sizeof(buf), fpr))
	goto OFW_CloseFile;
      if(!memcmp(buf, str_author1, LEN_AUTHOR1))
      {
	if(!fgets(buf, sizeof(buf), fpr))
	  goto OFW_CloseFile;
	if(!memcmp(buf, "���D:", 5))
	{
	  if(!fgets(buf, sizeof(buf), fpr))
	    goto OFW_CloseFile;
	  if(!memcmp(buf, "�ɶ�:", 5))
	  {
	    if(!fgets(buf, sizeof(buf), fpr))
	      goto OFW_CloseFile;
	    if(buf[0] == '\n')
	    {
	      if(!fgets(buf, sizeof(buf), fpr))
		goto OFW_CloseFile;
	    }
	  }
	}
      }

      do
      {
	fputs(buf, fpw);
      } while(fgets(buf, sizeof(buf), fpr));

OFW_CloseFile:
      fclose(fpr);
      fclose(fpw);

      strcpy(xpost.owner, hdr->owner);
      strcpy(xpost.nick, hdr->nick);
    }

    strcpy(xpost.title, ve_title);

    if (rc == 's')
      xpost.xmode = POST_OUTGO;
#ifdef HAVE_REFUSEMARK
    else if (rc == 'x')
      xpost.xmode = POST_RESTRICT;
#endif

    rec_bot(xfolder, &xpost, sizeof(HDR));

    if (rc == 's')
      outgo_post(&xpost, xboard);
  } while (++locus < tag);

  btime_update(xbno);

  /* Thor.981205: check �Q�઺�O���S���C�J����? */
  if (!(xbattr & BRD_NOCOUNT))
    cuser.numposts += tag ? tag : 1;	/* lkchu.981201: �n�� tag */

  /* �_�� currboard�Bcurrbattr */
  strcpy(currboard, srcboard);
  currbattr = srcbattr;

  if(userid[0])
  {
    strcpy(cuser.userid, userid);
    strcpy(cuser.username, username);
  }

  vmsg("�������");
  return XO_HEAD;
}


int
post_forward(xo)
  XO *xo;
{
  ACCT muser;
  HDR *hdr;

  if (!HAS_PERM(PERM_LOCAL))
    return XO_NONE;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);

  if (hdr->xmode & GEM_FOLDER)	/* �D plain text ������ */
    return XO_NONE;

#ifdef HAVE_REFUSEMARK
  if (!chkrestrict(hdr))
    return XO_NONE;
#endif

  if (acct_get("��F�H�󵹡G", &muser) > 0)
  {
    strcpy(quote_user, hdr->owner);
    strcpy(quote_nick, hdr->nick);
    hdr_fpath(quote_file, xo->dir, hdr);
    sprintf(ve_title, "%.64s (fwd)", hdr->title);
    move(1, 0);
    clrtobot();
    prints("��F��: %s (%s)\n��  �D: %s\n", muser.userid, muser.username, ve_title);

    mail_send(muser.userid);
    *quote_file = '\0';
  }
  return XO_HEAD;
}


/* ----------------------------------------------------- */
/* �O�D�\��Gmark / delete / label			 */
/* ----------------------------------------------------- */


static int
post_mark(xo)
  XO *xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.090321:�޲z�v�O���� */
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

#ifdef HAVE_LABELMARK
    if (xmode & POST_DELETE)	/* �ݬ媺�峹���� mark */
      return XO_NONE;
#endif

    hdr->xmode = xmode ^ POST_MARKED;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);

    return xo->pos + 1 + XO_MOVE;	/* lkchu.981201: ���ܤU�@�� */
  }
  return XO_NONE;
}



static int          /* qazq.030815: ����B�z�����аO��*/
post_done_mark(xo)
  XO *xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if ((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.091226: �}�񵹤@��O�D */
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

#ifdef HAVE_LABELMARK
    if (xmode & POST_DELETE)    /* �ݬ媺�峹����S */
      return XO_NONE;
#endif
    if (xmode & POST_SPMS)      /* �tSPMS���峹����S */
      return XO_NONE;
    if(xmode & POST_RESERVED)   /* �Q��w���峹 */
      return XO_NONE;

    hdr->xmode = xmode ^ POST_DONE;
    currchrono = hdr->chrono;

    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ?
      hdr->xid : pos, cmpchrono);
    move(3 + cur, 0);
    post_item(pos + 1, hdr);

    return xo->pos + 1 + XO_MOVE;   /* lkchu.981201: ���ܤU�@�� */
  }
  return XO_NONE;
}



static int
post_spms(xo)
  XO* xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.090321:�޲z�v�O���� */
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

    if(xmode & POST_DONE)
      return XO_NONE;
    if(xmode & POST_RESERVED)   /* �Q��w���峹 */
      return XO_NONE;

    hdr->xmode = xmode ^ POST_SPMS;
    currchrono = hdr->chrono;

    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ?
      hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);

    return xo->pos + 1 + XO_MOVE;   /* lkchu.981201: ���ܤU�@�� */
  }
  return XO_NONE;
}



static int
post_reserve(xo)    /* dust.080212: ��w�峹 */
  XO* xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.090321:�޲z�v�O���� */
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif

    if((xmode & POST_LOCKCLEAR) && !(xmode & POST_RESERVED))
      xmode &= (~POST_LOCKCLEAR);

    hdr->xmode = xmode ^ POST_RESERVED;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);
  }

  return XO_NONE;
}



static int
post_bottom(xo)
  XO *xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.090321:�޲z�v�O���� */
  {
    HDR *hdr, post;
    char fpath[64];
    int fd, fsize;
    struct stat st;

    if ((fd = open(xo->dir, O_RDONLY)) >= 0)
    {
      if (!fstat(fd, &st))
      {
        fsize = st.st_size;
        while ((fsize -= sizeof(HDR)) >= 0)
        {
          lseek(fd, fsize, SEEK_SET);
          read(fd, &post, sizeof(HDR));
          if (!(post.xmode & POST_BOTTOM))
            break;
        }
      }
      close(fd);
      if ((st.st_size - fsize) / sizeof(HDR) > 20)
      {
        vmsg("�w�F�m����ƶq�W��");
        return XO_FOOT;
      }
    }

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

    hdr_fpath(fpath, xo->dir, hdr);
    hdr_stamp(xo->dir, HDR_LINK | 'A', &post, fpath);
#ifdef HAVE_REFUSEMARK
    post.xmode = POST_BOTTOM | (hdr->xmode & POST_RESTRICT);
#else
    post.xmode = POST_BOTTOM;
#endif
    strcpy(post.owner, hdr->owner);
    strcpy(post.nick, hdr->nick);
    strcpy(post.title, hdr->title);

    rec_add(xo->dir, &post, sizeof(HDR));
    /* btime_update(currbno); */	/* ���ݭn�A�]���m���峹���C�J��Ū */

    return post_load(xo);	/* �ߨ���ܸm���峹 */
  }
  return XO_NONE;
}


#ifdef HAVE_REFUSEMARK
static int
post_refuse(xo)		/* itoc.010602: �峹�[�K */
  XO *xo;
{
  HDR *hdr;
  int pos, cur;

  if (!cuser.userlevel)	/* itoc.020114: guest ������L guest ���峹�[�K */
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if (is_author(hdr) || (bbstate & STAT_BM))
  {
    hdr->xmode ^= POST_RESTRICT;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    if((hdr->xmode & POST_RESTRICT))
    {
      if(pos + 1 >= xo->max || (hdr + 1)->xmode & POST_BOTTOM)
	btime_update(currbno);
    }

    move(3 + cur, 0);
    post_item(pos + 1, hdr);
  }

  return XO_NONE;
}
#endif


#ifdef HAVE_LABELMARK
static int
post_label(xo)
  XO *xo;
{
  if(currbattr & BRD_LOG)
    return XO_NONE;

  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))	/* dust.090321:�޲z�v�O���� */
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

    if (xmode & (POST_MARKED | POST_DONE))	/* mark�M����B�z���峹����ݬ� */
      return XO_NONE;
    if(xmode & POST_RESERVED)   /* �Q��w���峹 */
      return XO_NONE;

    hdr->xmode = xmode ^ POST_DELETE;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);

    return pos + 1 + XO_MOVE;	/* ���ܤU�@�� */
  }

  return XO_NONE;
}


static int
post_delabel(xo)
  XO *xo;
{
  int fdr, fsize, xmode;
  char fnew[64], fold[64], *folder;
  HDR *hdr;
  FILE *fpw;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  if (vans("�T�w�n�R���ݬ�峹��(Y/N)�H[N] ") != 'y')
    return XO_FOOT;

  folder = xo->dir;
  if ((fdr = open(folder, O_RDONLY)) < 0)
    return XO_FOOT;

  if (!(fpw = f_new(folder, fnew)))
  {
    close(fdr);
    return XO_FOOT;
  }

  fsize = 0;
  mgets(-1);
  while (hdr = mread(fdr, sizeof(HDR)))
  {
    xmode = hdr->xmode;

    if (!(xmode & POST_DELETE))
    {
      if ((fwrite(hdr, sizeof(HDR), 1, fpw) != 1))
      {
	close(fdr);
	fclose(fpw);
	unlink(fnew);
	return XO_FOOT;
      }
      fsize++;
    }
    else
    {
      /* �s�u��H */
      cancel_post(hdr);

      hdr_fpath(fold, folder, hdr);
      unlink(fold);
    }
  }
  close(fdr);
  fclose(fpw);

  sprintf(fold, "%s.o", folder);
  rename(folder, fold);
  if (fsize)
    rename(fnew, folder);
  else
    unlink(fnew);

  btime_update(currbno);

  return post_load(xo);
}
#endif


static int
post_delete(xo)
  XO *xo;
{
  int pos, cur, by_BM;
  HDR *hdr;
  char buf[80];

  if (!cuser.userlevel ||
    !strcmp(currboard, BN_DELETED) ||
    !strcmp(currboard, BN_JUNK))
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if ((hdr->xmode & POST_MARKED) || (!(bbstate & STAT_BOARD) && !is_author(hdr)))
    return XO_NONE;

  by_BM = bbstate & STAT_BOARD;

  if (vans(msg_del_ny) == 'y')
  {
    currchrono = hdr->chrono;

    if (!rec_del(xo->dir, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono))
    {
     /* pos = */move_post(hdr, xo->dir, by_BM);

      if (!by_BM && !(currbattr & BRD_NOCOUNT) && !(hdr->xmode & POST_BOTTOM))
      {
	/* itoc.010711: ��峹�n�����A���ɮפj�p */
	pos = pos >> 3;	/* �۹�� post �� wordsnum / 10 */
#if 0
	/* itoc.010830.����: �|�}: �Y multi-login �夣��t�@������ */
	if (cuser.money > pos)
	  cuser.money -= pos;
	else
	  cuser.money = 0;
#endif
	if (cuser.numposts > 0)
	  cuser.numposts--;
	sprintf(buf, "%s�A�z���峹� %d �g", MSG_DEL_OK, cuser.numposts);
	vmsg(buf);
      }

      if (xo->key == XZ_XPOST)
      {
	vmsg("��C��g�R����V�áA�Э��i�걵�Ҧ��I");
	return XO_QUIT;
      }
      return XO_LOAD;
    }
  }
  return XO_FOOT;
}


static int
chkpost(hdr)
  HDR *hdr;
{
  return (hdr->xmode & POST_MARKED);
}


static int
vfypost(hdr, pos)
  HDR *hdr;
  int pos;
{
  return (Tagger(hdr->chrono, pos, TAG_NIN) || chkpost(hdr));
}


static void
delpost(xo, hdr)
  XO *xo;
  HDR *hdr;
{
  char fpath[64];

  cancel_post(hdr);
  hdr_fpath(fpath, xo->dir, hdr);
  unlink(fpath);
}


static int
post_rangedel(xo)
  XO *xo;
{
  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  btime_update(currbno);

  return xo_rangedel(xo, sizeof(HDR), chkpost, delpost);
}


static int
post_prune(xo)
  XO *xo;
{
  int ret;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  ret = xo_prune(xo, sizeof(HDR), vfypost, delpost);

  btime_update(currbno);

  if (xo->key == XZ_XPOST && ret == XO_LOAD)
  {
    vmsg("��C��g�妸�R����V�áA�Э��i�걵�Ҧ��I");
    return XO_QUIT;
  }

  return ret;
}


static int
post_copy(xo)	   /* itoc.010924: ���N gem_gather */
  XO *xo;
{
  int tag;

  tag = AskTag("�ݪO�峹����");

  if (tag < 0)
    return XO_FOOT;

#ifdef HAVE_REFUSEMARK
  gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), chkrestrict);
#else
  gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), NULL);
#endif

  if (bbstate & STAT_BOARD)
  {
#ifdef XZ_XPOST
    if (xo->key == XZ_XPOST)
    {
      zmsg("�ɮ׼аO�����C[�`�N] �z���������}�걵�Ҧ��~��i�J��ذϡC");
      return XO_FOOT;
    }
    else
#endif
    {
      zmsg("���������C[�`�N] �K�W��~��R�����I");
      return post_gem(xo);	/* �����������i��ذ� */
    }
  }

  zmsg("�ɮ׼аO�����C[�`�N] �z�u��b���(�p)�O�D�Ҧb�έӤH��ذ϶K�W�C");
  return XO_FOOT;
}


static int
post_tbf(xo)
  XO *xo;
{
  if (!cuser.userlevel)
    return XO_NONE;

  xo_tbf(xo, chkrestrict);

  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* �����\��Gedit / title				 */
/* ----------------------------------------------------- */


int
post_edit(xo)
  XO *xo;
{
  char fpath[64];
  HDR *hdr;
  FILE *fp;
  int modified = 0;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);

  hdr_fpath(fpath, xo->dir, hdr);

  if (hdr->xmode & POST_RESERVED)	/* �Q��w(!)���峹 */
  {
    vmsg("����w�Q��w�A�L�k�ק�峹");
    return XO_NONE;
  }

  if(currbattr & BRD_LOG)
    return XO_NONE;

  if (HAS_PERM(PERM_ALLBOARD))
  {
#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif
    curredit |= EDIT_MODIFYPOST;
    if(!vedit(fpath, 0) && vans("�p��n�d�U�ק�����ܡH (Y/N) [N] ") == 'y')
    {
      if (fp = fopen(fpath, "a"))
      {
        ve_banner(fp, 1);
        fclose(fp);
        modified = 1;
      }
    }
  }
  else if(cuser.userlevel && is_author(hdr))	/* ��@�̭ק� */
  {
    curredit |= EDIT_MODIFYPOST;
    if(!vedit(fpath, 0))	/* �Y�D�����h�[�W�ק��T */
    {
      if (fp = fopen(fpath, "a"))
      {
        ve_banner(fp, 1);
        fclose(fp);
        modified = 1;
      }
    }
  }
  else		/* itoc.010301: ���ѨϥΪ̭ק�(�������x�s)��L�H�o���峹 */
  {
#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif
    vedit(fpath, -1);
  }

  if(modified && hdr->stamp > 0)
  {	/* dust.100905: �����n��ְO���W�v��? */
    change_stamp(xo->dir, hdr);

    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : xo->pos, cmpchrono);

    /* �[�J�ۤv���\Ū�O�� */
    if(!(hdr->xmode & POST_BOTTOM)) 
      crh_add(currchrono, hdr->stamp);

    btime_update(currbno);
  }

  /* return post_head(xo); */
  return XO_HEAD;	/* itoc.021226: XZ_POST �M XZ_XPOST �@�� post_edit() */
}


void
header_replace(xo, hdr)		/* itoc.010709: �ק�峹���D���K�ק鷺�媺���D */
  XO *xo;
  HDR *hdr;
{
  FILE *fpr, *fpw;
  char srcfile[64], tmpfile[64], buf[ANSILINELEN];
  
  hdr_fpath(srcfile, xo->dir, hdr);
  strcpy(tmpfile, "tmp/");
  strcat(tmpfile, hdr->xname);
  f_cp(srcfile, tmpfile, O_TRUNC);

  if (!(fpr = fopen(tmpfile, "r")))
    return;

  if (!(fpw = fopen(srcfile, "w")))
  {
    fclose(fpr);
    return;
  }

  fgets(buf, sizeof(buf), fpr);		/* �[�J�@�� */
  fputs(buf, fpw);

  fgets(buf, sizeof(buf), fpr);		/* �[�J���D */
  if (!str_ncmp(buf, "��", 2))		/* �p�G�� header �~�� */
  {
    strcpy(buf, buf[2] == ' ' ? "��  �D: " : "���D: ");
    strcat(buf, hdr->title);
    strcat(buf, "\n");
  }
  fputs(buf, fpw);

  while(fgets(buf, sizeof(buf), fpr))	/* �[�J��L */
    fputs(buf, fpw);

  fclose(fpr);
  fclose(fpw);
  f_rm(tmpfile);
}


static int
post_title(xo)
  XO *xo;
{
  HDR *fhdr, mhdr;
  int i, pos, cur;
  int modify = 0, bm_done;
  char *title, ans[3];
  char *menu[] = {"Y��\306\356��\306\356", "N�ڤ��޿ߦժ�", "C�ߤ]�i�H", NULL};
  screen_backup_t old_screen;

  if (!cuser.userlevel)	/* itoc.000213: �קK guest �b sysop �O����D */
    return XO_NONE;

  if(currbattr & BRD_LOG)
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  fhdr = (HDR *) xo_pool + cur;

  if(is_author(fhdr))
    bm_done = 0;
  else
  {
    if(HAS_PERM(PERM_ALLBOARD) && chkrestrict(fhdr))
      bm_done = 0;
    else if(bbstate & STAT_BOARD)
      bm_done = 1;
    else
      return XO_NONE;
  }

  memcpy(&mhdr, fhdr, sizeof(HDR));

  vget(b_lines, 0, "���D�G", mhdr.title, TTLEN + 1, GCARRY);

  if(HAS_PERM(PERM_ALLBOARD))
  {
    scr_dump(&old_screen);
    grayout(0, b_lines - 2, GRAYOUT_DARK);
    title = str_ttl(mhdr.title);
    i = (title == mhdr.title)? 5 : (mhdr.title[0] == 'R') ? 3 : 4;
    move(b_lines, 0);
    prints("%s %s\033[m\n", title_type[i], title);

    switch(krans(b_lines - 1, "nn", menu, "�p��H "))
    {
    case 'c':
      vget(b_lines, 0, "�@�̡G", mhdr.owner, 73 /* sizeof(mhdr.owner) */, GCARRY);
                /* Thor.980727: sizeof(mhdr.owner) = 80 �|�W�L�@�� */
      vget(b_lines, 0, "�ʺ١G", mhdr.nick, sizeof(mhdr.nick), GCARRY);
      vget(b_lines, 0, "����G", mhdr.date, sizeof(mhdr.date), GCARRY);

    case 'y':
      modify = 1;
    }

    scr_restore(&old_screen);
  }
  else if(memcmp(fhdr, &mhdr, sizeof(HDR)))
  {
    scr_dump(&old_screen);
    grayout(0, b_lines - 2, GRAYOUT_DARK);
    title = str_ttl(mhdr.title);
    i = (title == mhdr.title)? 5 : (mhdr.title[0] == 'R') ? 3 : 4;
    move(b_lines, 0);
    prints("%s %s\033[m\n", title_type[i], title);

    if(vget(b_lines - 1, 0, msg_sure_ny, ans, sizeof(ans), LCECHO) == 'y')
      modify = 1;

    scr_restore(&old_screen);
  }
  else
    return XO_FOOT;

  if(modify)
  {
    memcpy(fhdr, &mhdr, sizeof(HDR));
    currchrono = fhdr->chrono;
    rec_put(xo->dir, fhdr, sizeof(HDR), xo->key == XZ_XPOST ? fhdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(++pos, fhdr);

    /* itoc.010709: �ק�峹���D���K�ק鷺�媺���D */
    /* dust.100922: �l�[�p�G�O�O�D�諸���D�N���鷺����D */
    if(!bm_done)
      header_replace(xo, fhdr);
  }

  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* �B�~�\��Gwrite / score				 */
/* ----------------------------------------------------- */


int
post_write(xo)			/* itoc.010328: ��u�W�@�̤��y */
  XO *xo;
{
  if (HAS_PERM(PERM_PAGE))
  {
    HDR *hdr;
    UTMP *up;

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

    if (!(hdr->xmode & POST_INCOME) && (up = utmp_seek(hdr)))
      do_write(up);
  }
  return XO_NONE;
}


#ifdef HAVE_SCORE

static int curraddscore;

static void
addscore(hdd, ram)
  HDR *hdd, *ram;
{
  /* itoc.030618: �y�@�ӷs�� chrono �� xname */
  hdd->stamp = ram->stamp;
  hdd->xmode |= POST_SCORE;
  hdd->score += curraddscore;
}


static int
post_resetscore(xo)
  XO *xo;
{
  if (bbstate & STAT_BOARD) /* bm only */
  {
    HDR *hdr;
    int pos, cur, score;
    char ans[3];

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    switch (vans("�������]�w 1)�ۭq 2)�M�� 3)�z 4)�� [Q] "))
		/* �o�̷|���۵��{�H...... */
    {
    case '1':
#if 1
      if (!vget(b_lines, 0, "�D�H��A�n�令�h�֩O�H(�j���L�L�a�n�ۧA��) ", ans, 5, DOECHO))
#else
      if (!vget(b_lines, 0, "��J����(-100 ~ 100)�G", ans, 5, DOECHO))
#endif
        return XO_FOOT;
      score = atoi(ans);
      if (score>100) score = 100;
      else if (score<-100) score = -100;
      hdr->xmode |= POST_SCORE;		/* ���i��L���� */
      hdr->score = score;
      break;

    case '2':
      hdr->xmode &= ~POST_SCORE; 	/* �M���N���ݭnPOST_SCORE�F */
      hdr->score = 0;
      break;

    case '3':
      hdr->xmode |= POST_SCORE;		/* ���i��L���� */
      hdr->score = 100;
      break;

    case '4':
      hdr->xmode |= POST_SCORE;		/* ���i��L���� */
      hdr->score = -100;
      break;
    case '5':
      vmsg("ker ker");			/* just for a stupid test XD */
      break;

    default:
      return XO_FOOT;
    }

    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);
  }
  return XO_LOAD;
}


int
post_score(xo)
  XO *xo;
{
  HDR *hdr;
  FILE *fp;

  int pos, cur, ans, vtlen, maxlen, idlen;
  char fpath[64], reason[80], vtbuf[32], prompt[48];
  char *dir, *userid, *verb;
  time_t now;

#ifdef HAVE_ANONYMOUS
  char uid[IDLEN + 2];
#endif

  /* �������v���M�o��峹�ۦP */
  if ((currbattr & BRD_NOSCORE) || !(bbstate & STAT_POST))
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

#ifdef HAVE_REFUSEMARK
  if (!chkrestrict(hdr))	/* �ݤ��쪺�夣����� */
    return XO_NONE;
#endif

  if (hdr->xmode & POST_RESERVED)	/* �Q��w(!)���峹 */
  {
    vmsg("����w�Q��w�A�L�k����");
    return XO_NONE;
  }

  ans = vans("�� ����  0/5)���� 1)���� 2)��� 3)�ۭq�� 4)�ۭq�A�H[Q] ");
  if(ans == '0' || ans == '5' || ans == '%')
  {
    verb = "3m��";
    vtlen = 2;

    curraddscore = 0;
  }
  else if(ans == '1' || ans == '!')
  {
    verb = "1m��";
    vtlen = 2;

    curraddscore = 1;
  }
  else if(ans == '2' || ans == '@')
  {
    verb = "2m�A";
    vtlen = 2;

    curraddscore = -1;
  }
  else if(ans == '3' || ans == '#')
  {
    if(!vget(b_lines, 0, "�п�J�ʵ��G", &vtbuf[2], 5, DOECHO))
      return XO_FOOT;

    memcpy(vtbuf, "1m", sizeof(char) * 2);
    verb = vtbuf;
    vtlen = strlen(vtbuf+2);

    curraddscore = 1;
  }
  else if(ans == '4' || ans == '$')
  {
    if(!vget(b_lines, 0, "�п�J�ʵ��G", &vtbuf[2], 5, DOECHO))
      return XO_FOOT;

    memcpy(vtbuf, "2m", sizeof(char) * 2);
    verb = vtbuf;
    vtlen = strlen(vtbuf+2);

    curraddscore = -1;
  }
  else 
    return XO_FOOT;

#ifdef HAVE_ANONYMOUS
  /* �ΦW����\�� */
  if (currbattr & BRD_ANONYMOUS)
  {
    userid = uid;
    if (!vget(b_lines, 0, "�п�J�Q�Ϊ�ID�A�ο�J\033[1mr\033[m�ϥίu�W (����J�h�ϥιw�]ID)�G", userid, IDLEN + 1, DOECHO))
      userid = STR_ANONYMOUS;
    else if (userid[0] == 'r' && userid[1] == '\0')
      userid = cuser.userid;
    else
      strcat(userid, ".");		/* �۩w���ܡA�̫�[ '.' */
  }
  else
#endif
    userid = cuser.userid;

  idlen = strlen(userid);
  if(idlen <= 9)
  {
    maxlen = 62 - vtlen - idlen;
    sprintf(prompt, "�� \033[36m%s \033[3%s\033[1;30m:\033[m", userid, verb);
  }
  else	/* ID����10�H�W���S��ƪ� */
  {
    const char *prefix[] = {"��", " ", ""};
    int i = idlen - 10;

    if(i > 2) i = 2;

    if(idlen <= 12)
      maxlen = 53 - vtlen;
    else
      maxlen = 65 - vtlen - idlen;
    sprintf(prompt, "%s\033[36m%s \033[3%s\033[1;30m:\033[m", prefix[i], userid, verb);
  }

  if(!vget(b_lines, 0, prompt, reason, maxlen, DOECHO))
    return XO_FOOT;

  time(&now);

  dir = xo->dir;
  hdr_fpath(fpath, dir, hdr);

  if (fp = fopen(fpath, "a"))
  {
    struct tm *ptime = localtime(&now);

    fprintf(fp, "%s%-*s\033[1;30m%02d/%02d\033[;30m%c\033[1m%02d:%02d\033[m\n",
      prompt, maxlen, reason, ptime->tm_mon + 1, ptime->tm_mday,
      (ptime->tm_year + 49) % 50 + ((ptime->tm_year + 49) % 50 < 25 ? 'A' : 'a' - 25),
      ptime->tm_hour, ptime->tm_min);

    fclose(fp);
  }

#ifdef HAVE_ANONYMOUS
  if(userid != cuser.userid)	/* �O���ΦW�o�� */
    unanony_score(hdr, userid, &now, verb, reason);
#endif

  change_stamp(dir, hdr);

  currchrono = hdr->chrono;
  rec_ref(dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono, addscore);

  /* �[�J�ۤv���\Ū���� */
  if(!(hdr->xmode & POST_BOTTOM))	/* �m����S���\Ū�����A����[ */
    crh_add(currchrono, hdr->stamp);

  btime_update(currbno);

  return XO_LOAD;
}
#endif	/* HAVE_SCORE */


#ifdef MOVE_CAPABLE
static int
post_move(xo)
  XO *xo;
{
  HDR *hdr, mhdr;
  char buf[40];
  int pos, new_pos;
  int supermode = HAS_PERM(PERM_ALLBOARD);

  if(!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  hdr = (HDR *) xo_pool + (pos - xo->top);

  if(!supermode && !(hdr->xmode & POST_BOTTOM))	/* �@��H���ಾ�ʫD�m���� */
    return XO_NONE;

  switch(vget(b_lines, 0, "�п�J�s��m(�������w/+����/-���e)�G", buf, 6, DOECHO))
  {
  case '\0':
    return XO_FOOT;

  case '+':
    new_pos = pos + abs(atoi(buf + 1));
    break;

  case '-':
    new_pos = pos - abs(atoi(buf + 1));
    break;

  default:
    new_pos = atoi(buf) - 1;
  }

  if(new_pos < 0)
    new_pos = 0;
  else if(new_pos >= xo->max)
    new_pos = xo->max - 1;

  if(!supermode)	/* �@��H���ಾ�ʨ�m����ϥ~ */
  {
    rec_get(xo->dir, &mhdr, sizeof(HDR), new_pos);
    if(!(mhdr.xmode & POST_BOTTOM))
      return XO_FOOT;
  }

  if(new_pos != pos)
  {
    currchrono = hdr->chrono;
    if(!rec_del(xo->dir, sizeof(HDR), pos, cmpchrono))
    {
      rec_ins(xo->dir, hdr, sizeof(HDR), new_pos, 1);
      xo->pos = new_pos;
      return XO_LOAD;
    }
  }

  return XO_FOOT;
}
#endif			/* MOVE_CAPABLE */


static int
post_query(xo)		/* dust.091213: ��ܤ峹��T */
  XO *xo;
{
  HDR *ghdr;
  char owner[80], aid[16];
  char xmodes[64] = "";
  int cx, cy, y, i, mode;

  ghdr = (HDR *) xo_pool + (xo->pos - xo->top);
  
  /* �B�z�@�̸�T */
  strcpy(owner, ghdr->owner);
  strtok(owner, "@");
  if(strlen(owner) > 24)
    owner[24] = '\0';

  if(strtok(NULL, "\n"))
    strcat(owner, " \033[1;30m(���~�o�H)\033[m");
  else
    strcat(owner, " \033[1;30m(�����o��)\033[m");

  /* �B�z�峹�ݩʸ�T */
  mode = ghdr->xmode;
  if(mode & POST_RESERVED)
    strcat(xmodes, "!  ");
  if(mode & POST_DONE)
    strcat(xmodes, "S  ");
  if(mode & POST_RESTRICT)
    strcat(xmodes, "X  ");
  if(mode & POST_SPMS)
    strcat(xmodes, "#  ");
  if(mode & POST_MARKED)
    strcat(xmodes, "M  ");
  if(mode & POST_DELETE)
    strcat(xmodes, "T  ");
  
  if(strlen(xmodes) > 25)
    xmodes[25] = '\0';
  else if(xmodes[0] == '\0')
    sprintf(xmodes, "\033[1;30m(�L)\033[m");

  aid_encode(ghdr->chrono, aid);


  /* dust.111111: �令���e�ئA��r�A�����x�Ϋ� */
  getyx(&cy, &cx);
  if(cy <= b_lines / 2 + 1)
    y = cy + 1;
  else
    y = cy - 5;

  clrregion(y, y + 4);
  grayout(0, y - 1, GRAYOUT_DARK);
  grayout(y + 5, b_lines, GRAYOUT_DARK);

  /* �e�� */
  move(y, 0);
  outs("�z");
  for(i = 2; i < b_cols - 3; i += 2) outs("�w");
  outs("�{");

  for(i = 1; i < 4; i++)
  {
    move(y + i, 0);
    outs("�x");
    move(y + i, b_cols - (b_cols % 2? 3 : 2));
    outs("�x");
  }

  move(y + 4, 0);
  outs("�|");
  for(i = 2; i < b_cols - 3; i += 2) outs("�w");
  outs("�}");

  /* �L�X�峹��T */
  move(y + 1, 3);
  prints("�峹�N�X(AID): \033[1m#%s (%s)   \033[36m[" BBSNAME "]\033m", aid, currboard);

  move(y + 2, 3);
  if(chkrestrict(ghdr) || (currbattr & BRD_XRTITLE))
  {
    if(strlen(ghdr->title) > 67 + d_cols)
      outs(ghdr->title);
    else
      prints("\033[1m���D:\033[m %s", ghdr->title);
  }
  else
    outs("\033[1m���D:\033[m " RESTRICTED_TITLE);

  move(y + 3, 3);
  prints("\033[1m�@��:\033[m %s", owner);
  move(y + 3, 45);
  prints("\033[1m�ݩ�:\033[m %s", xmodes);

  move(cy, cx);
  vkey();

  return post_head(xo);
}


static int
post_state(xo)		/* dust.091213: ��ܤ峹��T(���ȱM�Ϊ�) */
  XO *xo;
{
  HDR *ghdr;
  char fpath[64];
  struct stat st;
  int mode;
  char xmodes[64] = "";

  if(!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  ghdr = (HDR *) xo_pool + (xo->pos - xo->top);

  hdr_fpath(fpath, xo->dir, ghdr);
  mode = ghdr->xmode;

  /* dust.071116 :���ӷs�W�ݩʮɭn��ʭק�o��...... */
  if(mode & POST_RESERVED)
    strcat(xmodes, "!  ");
  if(mode & POST_DONE)
    strcat(xmodes, "S  ");
  if(mode & POST_RESTRICT)
    strcat(xmodes, "X  ");
  if(mode & POST_SPMS)
    strcat(xmodes, "#  ");
  if(mode & POST_MARKED)
    strcat(xmodes, "M  ");
  if(mode & POST_DELETE)
    strcat(xmodes, "T  ");

  if(*xmodes == '\0')
    strcpy(xmodes,"\033[1;30m(�L)\033[m");

  move(8, 0);
  clrtobot();

  /* floatJ.080308: �ק��ı�˦� */

  /* dust.071126: �o�̪��Ÿ����ӳQvim�䴩�A�ק�ɽФp��"�۵�"�{�H */
  outs("\033[1;35m�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b\033[m\n");
  outs("\033[1;33;45m��\033[37m   �H�U�O�z�Ҭd�ߪ��峹��T�@�@�@�@                                         \033[m\n");

  prints("\n\033[1m�s��: \033[m%d", xo->pos+1);

  outs("\n\033[1m���D: \033[m");
  if (chkrestrict(ghdr) || (currbattr & BRD_XRTITLE))
    outs(ghdr->title);
  else
    outs(RESTRICTED_TITLE);

  prints("\n\033[1m�@��: \033[m%s", ghdr->owner);
  prints("\n\033[1m�ʺ�: \033[m%s", ghdr->nick);
  prints("\n\033[1m���: \033[m%s", ghdr->date);
  prints("\n\033[1m�ݩ�: \033[m%s", xmodes);

  outs("\n\033[1m���|: \033[m");
  if(chkrestrict(ghdr))
    outs(fpath);
  else
    outs("\033[31m(Sorry, but you don\'t have the permission.)\033[m");

  mode = stat(fpath, &st);
  outs("\n\033[1m�j�p: \033[m");
  if(!mode)
    prints("%d byte%s", st.st_size, st.st_size > 1 ? "s" : "");
  else
    outs("\033[1;30m(Ū������)\033m");

  outs("\n\033[1mMDT : \033[m");
  if(!mode)
    outs(Btime(&st.st_mtime));
  else
    outs("\033[1;30m(Ū������)\033m");

  prints("\n\033[1mCHT : \033[m%s (%d)", Btime(&ghdr->chrono), ghdr->chrono);
  prints("\n\033[1mSTT : \033[m%s (%d)", Btime(&ghdr->stamp), ghdr->stamp);

  outs("\n\033[35m�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w\033[m");
  vmsg(NULL);

  return post_body(xo);
}


static int
post_aidjump(xo)	/* dust.091215: AID���D�\��(ver 1.1) */
  XO *xo;
{
  char aid[32], *brdname, fpath[64];
  int pos, max, szbuf, chrono, i, match, newbno;
  usint ufo_backup;
  HDR *buffer;
  FILE *fp;


  if (!vget(b_lines, 0, "�п�J�峹�N�X(AID)�G #", aid, sizeof(aid), DOECHO))
    return XO_FOOT;

  /* AID�ѽX */
  chrono = aid_decode(aid);
  if(chrono == -1)	/* �L�k�ѪR��chrono�A�N��ҿ�J��AID�榡���~ */
  {
    vmsg("�o���O���T�榡��AID�A�Э��s��J");
    return XO_FOOT;
  }

  /* �O�W�ѪR (AID jump ver 1.1 �S���\��) */
  brdname = strchr(aid, '(');
  if(brdname)
  {
    strtok(brdname++, ")");	/* �зǮ榡�|�a���A���A�h�� */
    if(valid_brdname(brdname))
    {
      newbno = brd_bno(brdname);
      if(newbno >= 0 && (brd_bits[newbno] & BRD_L_BIT))
      {
	if(newbno == currbno)	/* �O��e�ݪO�A���ݭn��xo_pool[] */
	  brdname = NULL;
	else if(!(brd_bits[newbno] & BRD_R_BIT))
	{
	  vmsg("�L�k�\\Ū���ݪO");
	  return XO_FOOT;
	}
      }
      else
      {
	vmsg("���ݪO���s�b");
	return XO_FOOT;
      }
    }
    else
    {
      vmsg("�o���O���T�榡��AID�A�Э��s��J");
      return XO_FOOT;
    }
  }

  if(brdname)
    brd_fpath(fpath, brdname, fn_dir);
  else
    brd_fpath(fpath, currboard, fn_dir);

  max = szbuf = rec_num(fpath, sizeof(HDR));

  if(max <= 0)		/* �ӬݪO�S���峹�N�������䤣��峹 */
  {
    vmsg("�䤣��峹");
    return XO_FOOT;
  }

  if(!(fp = fopen(fpath, "rb")))
  {
    move(b_lines, 0);
    clrtoeol();
    outs("AID jump error: �L�k�}�Ҧ��ݪO�������� ");
    vkey();
    return XO_FOOT;
  }

  /* �M�w I/O buffer ���j�p */
  do
  {
    buffer = (HDR*) malloc(sizeof(HDR) * szbuf);
    szbuf = szbuf/2 + 1;
  }while(!buffer && szbuf > 2);

  if(!buffer)
  {
    fclose(fp);
    move(b_lines, 0);
    clrtoeol();
    outs("AID jump error: �j�M�ΪŶ��t�m���ѡC�ЦA�դ@���εy��A�� ");
    vkey();
    return XO_FOOT;
  }

  /* �j�M�{�� */
  match = 0;
  i = szbuf;
  for(pos = 0; pos < max; pos++)
  {
    if(i >= szbuf)
    {
      fread(buffer, szbuf, sizeof(HDR), fp);
      i = 0;
    }

    if(buffer[i].chrono == chrono)
    {
      match = 1;
      break;
    }

    i++;
  }

  free(buffer);
  fclose(fp);

  if(!match)	/* �S���峹 */
  {
    vmsg("�䤣��峹");
    return XO_FOOT;
  }

  if(brdname)		/* AID jump v1.1: ��ݪO��AID */
  {
    brd_bits[newbno] |= BRD_V_BIT;
    ufo_backup = cuser.ufo;
    cuser.ufo &= ~UFO_BRDNOTE;
    XoPost(newbno);
    cuser.ufo = ufo_backup;
  }

  xz[XZ_POST - XO_ZONE].xo->pos = pos;	/* ���ܸӽg�峹 */
  return XZ_POST;
}


int
post_thelastNBpost(xo)
  XO *xo;
{
  int pos, cur;
  FILE *fp;
  HDR hdr;

  cur = xo->max - 1;
  pos = xo->pos;

  if(cur > pos && (fp = fopen(xo->dir, "rb")))
  {
    fseeko(fp, -sizeof(HDR), SEEK_END);

    do
    {
      if(fread(&hdr, 1, sizeof(HDR), fp) <= 0)
	break;

      if(!(hdr.xmode & POST_BOTTOM))
	break;

      fseeko(fp, -(sizeof(HDR) * 2), SEEK_CUR);
    }while(--cur > pos);

    fclose(fp);
  }

  return cur <= pos? xo->max - 1 : cur;
}



static int
post_help(xo)
  XO *xo;
{
  xo_help("post");
  /* return post_head(xo); */
  return XO_HEAD;		/* itoc.001029: �P xpost_help �@�� */
}


KeyFunc post_cb[] =
{
  XO_INIT, post_init,
  XO_LOAD, post_load,
  XO_HEAD, post_head,
  XO_BODY, post_body,

  'r', post_browse,
  's', post_switch,
  KEY_TAB, post_gem,
  'z', post_gem,

  'y', post_reply,
  'd', post_delete,
  'v', post_visit,
  'x', post_cross,		/* �b post/mbox �����O�p�g x ��ݪO�A�j�g X ��ϥΪ� */
  'X', post_forward,
  't', post_tag,
  'E', post_edit,
  'T', post_title,
  'm', post_mark,
  '_', post_bottom,
  'D', post_rangedel,
  '#', post_aidjump,
  '@', post_spms,
  '!', post_reserve,

#ifdef HAVE_SCORE
  '%', post_score,
  'i', post_resetscore,
#endif

  'w', post_write,

  'b', post_memo,
  'c', post_copy,
  'C', post_tbf,
  'g', gem_gather,
  'Q', post_query,
  Ctrl('B'), post_state,
  
  Ctrl('P'), post_add,
  Ctrl('D'), post_prune,
  Ctrl('Q'), xo_uquery,
  Ctrl('O'), xo_usetup,
  Ctrl('S'), post_done_mark,    /* qazq.030815: ����B�z�����аO��*/
#ifdef HAVE_REFUSEMARK
  Ctrl('Y'), post_refuse,
#endif

#ifdef HAVE_LABELMARK
  'n', post_label,
  Ctrl('N'), post_delabel,
#endif

  'B' | XO_DL, (void *) "bin/manage.so:post_manage",
  'R' | XO_DL, (void *) "bin/vote.so:vote_result",
  'V' | XO_DL, (void *) "bin/vote.so:XoVote",
  'I' | XO_DL, (void *) "bin/manage.so:post_binfo",

#ifdef HAVE_TERMINATOR
  Ctrl('X') | XO_DL, (void *) "bin/manage.so:post_terminator",
#endif

  '~', XoXselect,		/* itoc.001220: �j�M�@��/���D */
  'S', XoXsearch,		/* itoc.001220: �j�M�ۦP���D�峹 */
  'a', XoXauthor,		/* itoc.001220: �j�M�@�� */
  '/', XoXtitle,		/* itoc.001220: �j�M���D */
  'f', XoXfull,			/* itoc.030608: ����j�M */
  'G', XoXmark,			/* itoc.010325: �j�M mark �峹 */
  Ctrl('G'), XoXuni,
  'L', XoXlocal,		/* itoc.010822: �j�M���a�峹 */

#ifdef HAVE_XYNEWS
  'u', XoNews,			/* itoc.010822: �s�D�\Ū�Ҧ� */
#endif

#ifdef MOVE_CAPABLE
  'M', post_move,
#endif

  'h', post_help
};


KeyFunc xpost_cb[] =
{
  XO_INIT, xpost_init,
  XO_LOAD, xpost_load,
  XO_HEAD, xpost_head,
  XO_BODY, post_body,		/* Thor.980911: �@�ΧY�i */

  'r', xpost_browse,
  'y', post_reply,
  't', post_tag,
  'x', post_cross,
  'X', post_forward,
  'c', post_copy,
  'C', post_tbf,
  'g', gem_gather,
  'm', post_mark,
  '@', post_spms,
  '!', post_reserve,
  'd', post_delete,		/* Thor.980911: ��K�O�D */
  'E', post_edit,		/* itoc.010716: ���� XPOST ���i�H�s����D�B�峹�A�[�K */
  'T', post_title,
#ifdef HAVE_SCORE
  '%', post_score,
  'i', post_resetscore, 
#endif
  'w', post_write,
  'b', post_memo,
  'Q', post_query,
  Ctrl('B'), post_state,
#ifdef HAVE_REFUSEMARK
  Ctrl('Y'), post_refuse,
#endif
#ifdef HAVE_LABELMARK
  'n', post_label,
#endif

  '~', XoXselect,
  'S', XoXsearch,
  'a', XoXauthor,
  '/', XoXtitle,
  'f', XoXfull,
  'G', XoXmark,
  'L', XoXlocal,
  Ctrl('G'), XoXuni,
  'I' | XO_DL, (void *) "bin/manage.so:post_binfo",

  Ctrl('P'), post_add,
  Ctrl('D'), post_prune,
  Ctrl('Q'), xo_uquery,
  Ctrl('O'), xo_usetup,
  Ctrl('S'), post_done_mark,    /* qazq.030815: ����B�z�����аO��*/

  'h', post_help		/* itoc.030511: �@�ΧY�i */
};


#ifdef HAVE_XYNEWS
KeyFunc news_cb[] =
{
  XO_INIT, news_init,
  XO_LOAD, news_load,
  XO_HEAD, news_head,
  XO_BODY, post_body,

  'r', XoXsearch,

  'h', post_help		/* itoc.030511: �@�ΧY�i */
};
#endif	/* HAVE_XYNEWS */

