/*-------------------------------------------------------*/
/* newbrd.c   ( YZU_CSE WindTop BBS )                    */
/*-------------------------------------------------------*/
/* target : �s�p�\��    			 	 */
/* create : 00/01/02				 	 */
/* update : 10/10/03				 	 */
/*-------------------------------------------------------*/
/* run/newbrd/_/.DIR - newbrd control header		 */
/* run/newbrd/_/@/@_ - newbrd description file		 */
/* run/newbrd/_/@/G_ - newbrd voted id loG file		 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef HAVE_COSIGN

extern XZ xz[];
extern char xo_pool[];
extern BCACHE *bshm;		/* itoc.010805: �}�s�O�� */

static int nbrd_add();

static char *split_line = "\033[m\033[33m�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w\033[m\n";


typedef struct
{
  char userid[IDLEN + 1];
  char email[60];
}      LOG;



static int
cmpbtime(nbrd)
  NBRD *nbrd;
{
  return nbrd->btime == currchrono;
}



static void
nbrd_fpath(fpath, folder, nbrd)
  char *fpath;
  char *folder;
  NBRD *nbrd;
{
  char *str;
  int cc;

  while (cc = *folder++)
  {
    *fpath++ = cc;
    if (cc == '/')
      str = fpath;
  }
  strcpy(str, nbrd->xname);
}



static int
nbrd_body(xo)
  XO *xo;
{
  NBRD *nbrd;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    if (vans("�n�s�W�s�p���ض�(Y/N)�H[N] ") == 'y')
      return nbrd_add(xo);
    return XO_QUIT;
  }

  nbrd = (NBRD *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;

  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    prints("%6d ", ++num);

    if(nbrd->mode & NBRD_FINISH)
      outc(' ');
    else if(nbrd->mode & NBRD_END)	/* �s�p���� */
      outc('-');
    else if(nbrd->mode & NBRD_EXPIRE)	/* �s�p�L�� */
      outc('x');
    else				/* �s�p�� */
      outc('+');

    prints(" %-5s %-13s [%s] %.*s\n", nbrd->date + 3, nbrd->owner, nbrd->brdname, d_cols + 20, nbrd->title);
    nbrd++;

  }while(num < max);
  clrtobot();

  return XO_FOOT;
}


static int
nbrd_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(NBRD));
  return nbrd_body(xo);
}



static int
nbrd_head(xo)
  XO *xo;
{
  vs_head("�ݪO�s�p", str_site);
  prints(NECKER_COSIGN, d_cols, "");
  return nbrd_body(xo);
}


static int
nbrd_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(NBRD));
  return nbrd_head(xo);
}



static int
nbrd_find(fpath, brdname)
  char *fpath, *brdname;
{
  NBRD old;
  int fd;
  int rc = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(NBRD)) == sizeof(NBRD))
    {
      if (!str_cmp(old.brdname, brdname) && !(old.mode & NBRD_FINISH))
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
nbrd_stamp(folder, nbrd, fpath)
  char *folder;
  NBRD *nbrd;
  char *fpath;
{
  char *fname;
  char *family = NULL;
  int rc;
  time_t token;

  fname = fpath;
  while (rc = *folder++)
  {
    *fname++ = rc;
    if (rc == '/')
      family = fname;
  }

  fname = family;
  *family++ = '@';

  token = time(0);

  archiv32(token, family);

  nbrd->btime = token;
  str_stamp(nbrd->date, &nbrd->btime);
  strcpy(nbrd->xname, fname);

  rc = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600);
  if(rc >= 0)
  {
    close(rc);
    return 1;
  }
  else
    return 0;
}


static int
nbrd_add(xo)
  XO *xo;
{
  char fpath[64], path[64];
  FILE *fp;
  NBRD nbrd;
  const int rv_noact = xo->max > 0? XO_FOOT : XO_QUIT;


  if(!HAS_PERM(PERM_POST))
  {
    vmsg("�A�S���v���o�_�s�p");
    return rv_noact;
  }

  memset(&nbrd, 0, sizeof(NBRD));

  if (!vget(b_lines, 0, "�^��O�W�G", nbrd.brdname, sizeof(nbrd.brdname), DOECHO))
    return XO_FOOT;

  if(!valid_brdname(nbrd.brdname))
  {
    vmsg("�O�W���X�k");
    return rv_noact;
  }
  else if(brd_bno(nbrd.brdname) >= 0)
  {
    vmsg("�w�����O");
    return rv_noact;
  }
  else if(nbrd_find(xo->dir, nbrd.brdname))
  {
    vmsg("���O�W���b�s�p��");
    return XO_FOOT;
  }

  if(!vget(b_lines, 0, "�ݪO�D�D�G", nbrd.title, sizeof(nbrd.title), DOECHO) ||
    !vget(b_lines, 0, "�ݪO�����G", nbrd.class, sizeof(nbrd.class), DOECHO))
    return rv_noact;

#ifdef SYSOP_START_COSIGN
  nbrd.mode = NBRD_NEWBOARD;
#else
  nbrd.mode = NBRD_NEWBOARD | NBRD_START;
#endif

  vmsg("�}�l�s��s�p����");
  sprintf(path, "tmp/%s.nbrd", cuser.userid);	/* �s�p��]���Ȧs�ɮ� */
  if(vedit(path, 0) < 0)
  {
    unlink(path);
    vmsg(msg_cancel);
    return nbrd_head(xo);
  }

  if(!nbrd_stamp(xo->dir, &nbrd, fpath))
    return nbrd_head(xo);

  nbrd.etime = nbrd.btime + NBRD_DAY_BRD * 86400;
  nbrd.total = NBRD_NUM_BRD;
  strcpy(nbrd.owner, cuser.userid);

  fp = fopen(fpath, "a");
  fprintf(fp, "�@��: %s (%s) ����: �s�p�t��\n", cuser.userid, cuser.username);
  fprintf(fp, "���D: [%s] %s\n", nbrd.brdname, nbrd.title);
  fprintf(fp, "�ɶ�: %s\n\n", Btime(&nbrd.btime));

  fprintf(fp, "�^��O�W�G%s\n", nbrd.brdname);
  fprintf(fp, "�ݪO�����G%s\n", nbrd.class);
  fprintf(fp, "�ݪO�D�D�G%s\n", nbrd.title);
  fprintf(fp, "�O�D�W�١G%s\n", cuser.userid);
  fprintf(fp, "�|�����G%s\n", nbrd.date);
  fprintf(fp, "����ѼơG%d\n", NBRD_DAY_BRD);
  fprintf(fp, "�ݳs�p�H�G%d\n", NBRD_NUM_BRD);
  fprintf(fp, split_line);
  fprintf(fp, "�s�p�����G\n");
  f_suck(fp, path);
  unlink(path);
  fprintf(fp, split_line);
  fclose(fp);

  rec_add(xo->dir, &nbrd, sizeof(NBRD));

  vmsg("�s�p�}�l�F�I");
  return nbrd_init(xo);
}



static int
nbrd_seek(fpath)
  char *fpath;
{
  LOG old;
  int fd;
  int rc = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(LOG)) == sizeof(LOG))
    {
      if (!strcmp(old.userid, cuser.userid) || !str_cmp(old.email, cuser.email))
      {
	rc = 1;
	break;
      }
    }
    close(fd);
  }
  return rc;
}


static void
addreply(hdd, ram)
  NBRD *hdd, *ram;
{
  if (--hdd->total <= 0)
    hdd->mode |= NBRD_END;

  if(hdd->total < 0)
    ram->total = 0;
  else
    ram->total = hdd->total + 1;
}


static int
nbrd_reply(xo)
  XO *xo;
{
  NBRD *nbrd;
  char *fname, fpath[64], reason[80];
  LOG rec;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
  fname = NULL;

  if (nbrd->mode & (NBRD_FINISH | NBRD_END))
    return XO_NONE;

  if (time(0) >= nbrd->etime)
  {
    /* ��s�w�и̪����� */
    if(!(nbrd->mode & NBRD_END))
    {
      nbrd->mode |= NBRD_END;
      currchrono = nbrd->btime;
      rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);
    }

    vmsg("�s�p�w�g�I��F");
    return XO_FOOT;
  }


  /* --------------------------------------------------- */
  /* �ˬd�O�_�w�g�s�p�L					 */
  /* --------------------------------------------------- */

  nbrd_fpath(fpath, xo->dir, nbrd);
  fname = strrchr(fpath, '@');
  *fname = 'G';

  if (nbrd_seek(fpath))
  {
    vmsg("�z�w�g�s�p�L�F�I");
    return XO_FOOT;
  }

  /* --------------------------------------------------- */
  /* �}�l�s�p						 */
  /* --------------------------------------------------- */

  *fname = '@';

  if (vans("�n�[�J�s�p��(Y/N)�H[N] ") == 'y')
  {
    FILE *fp;

    if(!vget(b_lines, 0, "�ڦ��ܭn���G", reason, 65, DOECHO))
      return XO_FOOT;

    currchrono = nbrd->btime;
    rec_ref(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime, addreply);

    if (fp = fopen(fpath, "a"))
    {
      fprintf(fp, "%3d -> %s (%s)\n    %s\n", nbrd->total, cuser.userid, fromhost, reason);
      fclose(fp);
    }

    memset(&rec, 0, sizeof(LOG));
    strcpy(rec.userid, cuser.userid);
    strcpy(rec.email, cuser.email);
    *fname = 'G';
    rec_add(fpath, &rec, sizeof(LOG));

    vmsg("�[�J�s�p����");
    return nbrd_init(xo);
  }

  return XO_FOOT;
}



static int
nbrd_finish(xo)
  XO *xo;
{
  NBRD *nbrd;
  char fpath[64], path[64];
  FILE *fp;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);

  if (nbrd->mode & NBRD_FINISH)
    return XO_NONE;

  if (vans("�нT�w�����s�p(Y/N)�H[N] ") != 'y')
    return XO_FOOT;

  vmsg("�нs�赲���s�p��]");
  sprintf(path, "tmp/%s", cuser.userid);	/* �s�p��]���Ȧs�ɮ� */
  if(vedit(path, 0) < 0)
  {
    unlink(path);
    vmsg(msg_cancel);
    return nbrd_head(xo);
  }

  nbrd_fpath(fpath, xo->dir, nbrd);

  fp = fopen(fpath, "a");
  fprintf(fp, split_line);
  fprintf(fp, "�� %s �� %s �����s�p\n\n", HAS_PERM(PERM_SpSYSOP)? SysopLoginID_query(NULL) : cuser.userid, Now());
  fprintf(fp, "�����s�p��]�G\n\n");
  f_suck(fp, path);
  fclose(fp);
  unlink(path);

  nbrd->mode |= NBRD_FINISH;
  currchrono = nbrd->btime;
  rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);

  return nbrd_head(xo);
}



static int			/* 1:�}�O���\ */
nbrd_newbrd(nbrd)
  NBRD *nbrd;
{
  BRD newboard;
  ACCT acct;
  /* itoc.030519: �קK���ж}�O */
  if (brd_bno(nbrd->brdname) >= 0)
  {
    vmsg("�w�����O");
    return 1;
  }

  memset(&newboard, 0, sizeof(BRD));

  /* itoc.010805: �s�ݪO�w�] battr = ����H, postlevel = PERM_POST, �ݪO�O�D�����_�s�p�� */
  newboard.battr = BRD_NOTRAN;
  newboard.postlevel = PERM_POST;
  strcpy(newboard.brdname, nbrd->brdname);
  strcpy(newboard.class, nbrd->class);
  strcpy(newboard.title, nbrd->title);
  strcpy(newboard.BM, nbrd->owner);

  if (acct_load(&acct, nbrd->owner) >= 0)
    acct_setperm(&acct, PERM_BM, 0);

  if (brd_new(&newboard) < 0)
    return 0;

  vmsg("�s�O�w���ߡC�O�o�n��ʥ[�J�����s�դ�");
  return 1;
}


static int
nbrd_open(xo)		/* itoc.010805: �}�s�O�s�p�A�s�p�����}�s�ݪO */
  XO *xo;
{
  NBRD *nbrd;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);

  if (nbrd->mode & NBRD_FINISH || !(nbrd->mode & NBRD_NEWBOARD))
    return XO_NONE;

  if (vans("�T�w�n�}�ҬݪO��(Y/N)�H[N] ") == 'y')
  {
    if (nbrd_newbrd(nbrd))
    {
      FILE *fp;
      char fpath[64];

      nbrd_fpath(fpath, xo->dir, nbrd);
      fp = fopen(fpath, "a");
      fprintf(fp, "\n�� %s �� %s �}�ҬݪO [%s]\n", HAS_PERM(PERM_SpSYSOP)? SysopLoginID_query(NULL) : cuser.userid, Now(), nbrd->brdname);

      nbrd->mode |= NBRD_FINISH;
      currchrono = nbrd->btime;
      rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);
    }
    return nbrd_head(xo);
  }

  return XO_FOOT;
}


static int
nbrd_browse(xo)
  XO *xo;
{
  int key;
  NBRD *nbrd;
  char fpath[80];

  /* itoc.010304: ���F���\Ū��@�b�]�i�H�[�J�s�p�A�Ҽ{ more �Ǧ^�� */
  for (;;)
  {
    nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
    nbrd_fpath(fpath, xo->dir, nbrd);

    if ((key = more(fpath, MFST_AUTO)) < 0)
      break;

    if (!key)
      key = vkey();

    switch (key)
    {
    case KEY_UP:
    case KEY_PGUP:
    case 'k':
      key = xo->pos - 1;

      if (key < 0)
	break;

      xo->pos = key;

      if (key <= xo->top)
      {
	xo->top = (key / XO_TALL) * XO_TALL;
	nbrd_load(xo);
      }
      continue;

    case KEY_DOWN:
    case KEY_PGDN:
    case 'j':
    case ' ':
      key = xo->pos + 1;

      if (key >= xo->max)
	break;

      xo->pos = key;

      if (key >= xo->top + XO_TALL)
      {
	xo->top = (key / XO_TALL) * XO_TALL;
	nbrd_load(xo);
      }
      continue;

    case 'y':
      nbrd_reply(xo);
      break;
    }
    break;
  }

  return nbrd_head(xo);
}


static int
nbrd_delete(xo)
  XO *xo;
{
  NBRD *nbrd;
  char *fname, fpath[80];
  char *list = "@G";		/* itoc.����: �M newbrd file */

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
  if (strcmp(cuser.userid, nbrd->owner) && !HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  if (vans(msg_del_ny) != 'y')
    return XO_FOOT;

  nbrd_fpath(fpath, xo->dir, nbrd);
  fname = strrchr(fpath, '@');
  while (*fname = *list++)
  {
    unlink(fpath);	/* Thor: �T�w�W�r�N�� */
  }

  currchrono = nbrd->btime;
  rec_del(xo->dir, sizeof(NBRD), xo->pos, cmpbtime);
  return nbrd_init(xo);
}


static int
nbrd_edit(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_ALLBOARD))
  {
    char fpath[64];
    NBRD *nbrd;

    nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
    nbrd_fpath(fpath, xo->dir, nbrd);
    vedit(fpath, 0);
    return nbrd_head(xo);
  }

  return XO_NONE;
}


static int
nbrd_setup(xo)
  XO *xo;
{
  int numbers;
  char ans[6];
  NBRD *nbrd, newnh;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  vs_bar("�s�p�]�w");
  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
  memcpy(&newnh, nbrd, sizeof(NBRD));

  prints("�ݪO�W�١G%s\n�ݪO�����G%4.4s %s\n�s�p�o�_�G%s\n",
    newnh.brdname, newnh.class, newnh.title, newnh.owner);
  prints("�}�l�ɶ��G%s\n", Btime(&newnh.btime));
  prints("�����ɶ��G%s\n", Btime(&newnh.etime));
  prints("�ٻݤH�ơG%d\n", newnh.total);

  if (vget(8, 0, "(E)�]�w (Q)�����H[Q] ", ans, 3, LCECHO) == 'e')
  {
    vget(11, 0, MSG_BID, newnh.brdname, BNLEN + 1, GCARRY);
    vget(12, 0, "�ݪO�����G", newnh.class, sizeof(newnh.class), GCARRY);
    vget(13, 0, "�ݪO�D�D�G", newnh.title, sizeof(newnh.title), GCARRY);
    sprintf(ans, "%d", newnh.total);
    vget(14, 0, "�s�p�H�ơG", ans, 6, GCARRY);
    numbers = atoi(ans);
    if (numbers <= 500 && numbers >= 1)
      newnh.total = numbers;

    if (memcmp(&newnh, nbrd, sizeof(newnh)) && vans(msg_sure_ny) == 'y')
    {
      memcpy(nbrd, &newnh, sizeof(NBRD));
      currchrono = nbrd->btime;
      rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);
    }
  }

  return nbrd_head(xo);
}


static int
nbrd_uquery(xo)
  XO *xo;
{
  NBRD *nbrd;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);

  move(1, 0);
  clrtobot();
  my_query(nbrd->owner);
  return nbrd_head(xo);
}


static int
nbrd_note(xo)
  XO *xo;
{
  clear();
  lim_cat("etc/help/cosign/note", b_lines);
  vmsg(NULL);

  return XO_HEAD;
}


static int
nbrd_help(xo)
  XO *xo;
{
  xo_help("cosign");
  return nbrd_head(xo);
}



static int
nbrd_trans(xo)	/* dust.101003: �q�®ɥN�ഫ�L�Ӫ��\�� */
  XO *xo;
{
  if(HAS_PERM(PERM_SpSYSOP))
  {
    NBRD *nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
    NBRD newh = *nbrd;

    if(newh.mode & NBRD_EXPIRE)
    {
      if(vans("�n���� NBRD_END ��(Y/N)�H [N] ") == 'y')
      {
	newh.mode &= ~NBRD_EXPIRE;
	newh.mode |= NBRD_END;
      }
    }
    else if(newh.mode & NBRD_END)
    {
      if(vans("�n���� NBRD_EXPIRE ��(Y/N)�H [N] ") == 'y')
      {
	newh.mode |= NBRD_EXPIRE;
	newh.mode &= ~NBRD_END;
      }
    }
    else
      return XO_NONE;

    if(memcmp(&newh, nbrd, sizeof(NBRD)))
    {
      currchrono = newh.btime;
      rec_put(xo->dir, &newh, sizeof(NBRD), xo->pos, cmpbtime);
      return XO_LOAD;
    }

    return XO_FOOT;
  }

  return XO_NONE;
}



static KeyFunc nbrd_cb[] =
{
  XO_INIT, nbrd_init,
  XO_LOAD, nbrd_load,
  XO_HEAD, nbrd_head,
  XO_BODY, nbrd_body,

  'y', nbrd_reply,
  'r', nbrd_browse,
  'o', nbrd_open,
  'c', nbrd_finish,
  'd', nbrd_delete,
  'E', nbrd_edit,
  'B', nbrd_setup,
  'b', nbrd_note,

  Ctrl('E'), nbrd_load,
  Ctrl('P'), nbrd_add,
  Ctrl('Q'), nbrd_uquery,
  Ctrl('G'), nbrd_trans,

  'h', nbrd_help
};


int
XoNewBoard()
{
  XO *xo;
  char fpath[64];

  sprintf(fpath, "run/newbrd/%s", fn_dir);
  xz[XZ_COSIGN - XO_ZONE].xo = xo = xo_new(fpath);
  xz[XZ_COSIGN - XO_ZONE].cb = nbrd_cb;
  xo->key = XZ_COSIGN;
  xo->pos = rec_num(fpath, sizeof(NBRD)) - 1;
  nbrd_note(xo);
  xover(XZ_COSIGN);
  free(xo);

  return 0;
}
#endif	/* HAVE_COSIGN */

