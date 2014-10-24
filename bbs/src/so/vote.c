/*-------------------------------------------------------*/
/* vote.c	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : boards' vote routines		 	 */
/* create : 95/03/29				 	 */
/* update : 09/05/16				 	 */
/*-------------------------------------------------------*/
/* brd/_/.VCH  : Vote Control Header    �ثe�Ҧ��벼���� */
/* brd/_/@vote : vote history           �L�h���벼���v	 */
/* brd/_/@/@__ : vote description       �벼����	 */
/* brd/_/@/I__ : vote selection Items   �벼�ﶵ	 */
/* brd/_/@/O__ : user's Opinion(comment) �ϥΪ̦��ܭn��	 */
/* brd/_/@/L__ : can vote List          �i�벼�W��	 */
/* brd/_/@/G__ : voted id loG file      �w�Ⲽ�W��	 */
/* brd/_/@/Z__ : final/temporary result	�벼���G	 */
/* brd/_/@/B__ : ��L�U�`����(�ڭ^�夣�n���n�s�ڼg�^��)	 */
/*-------------------------------------------------------*/
/* dust.090426: 1.������L�t�ΩҨϥΪ����		 */
/*		2.��}��L������			 */
/*-------------------------------------------------------*/


#include "bbs.h"



extern BCACHE *bshm;
extern XZ xz[];
extern char xo_pool[];


static char *
vch_fpath(fpath, folder, vch)
  char *fpath, *folder;
  VCH *vch;
{
  /* VCH �M HDR �� xname ���ǰt�A�ҥH�����ɥ� hdr_fpath() */
  hdr_fpath(fpath, folder, (HDR *) vch);
  return strrchr(fpath, '@');
}


static int vote_add();


int
vote_result(xo)
  XO *xo;
{
  char fpath[64];

  setdirpath(fpath, xo->dir, "@/@vote");
  /* Thor.990204: ���Ҽ{more �Ǧ^�� */   
  if (more(fpath, MFST_NORMAL) >= 0)
    return XO_HEAD;	/* XZ_POST �M XZ_VOTE �@�� vote_result() */

  vmsg("�ثe�S������}�������G");
  return XO_FOOT;
}


static void
vote_item(num, vch)
  int num;
  VCH *vch;
{
  prints("%6d%c%c%c%c%c %-9.8s%-12s %.44s\n",
    num, tag_char(vch->chrono), vch->vgamble, vch->vsort, vch->vpercent, vch->vprivate, 
    vch->cdate, vch->owner, vch->title);
}


static int
vote_body(xo)
  XO *xo;
{
  VCH *vch;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    if (bbstate & STAT_BOARD)
    {
      if (vans("�n�|��벼��(Y/N)�H[N] ") == 'y')
	return vote_add(xo);
    }
    else
    {
      vmsg("�ثe�õL�벼�|��");
    }
    return XO_QUIT;
  }

  vch = (VCH *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    vote_item(++num, vch++);
  } while (num < max);
  clrtobot();

  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: �� b_lines ��W feeter */
}


static int
vote_head(xo)
  XO *xo;
{
  vs_head("�벼/��L�C��", "�벼��");
  prints(NECKER_VOTE, d_cols, "");
  return vote_body(xo);
}


static int
vote_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(VCH));
  return vote_head(xo);
}


static int
vote_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(VCH));
  return vote_body(xo);
}


static void
vch_edit(vch, item, echo)
  VCH *vch;
  int item;		/* vlist ���X�� */
  int echo;		/* DOECHO:�s�W  GCARRY:�ק� */
{
  int num, row, days;
  char ans[8], buf[80];

  clear();

  row = 4;


  sprintf(buf, "�аݨC�H�̦h�i��X���H(1~%d) ", item);
  if(echo == GCARRY)
    sprintf(ans, "%d", vch->maxblt);
  vget(++row, 0, buf, ans, 3, echo);
  num = atoi(ans);
  if (num < 1)
    num = 1;
  else if (num > item)
    num = item;
  vch->maxblt = num;

  if(echo == DOECHO)
  {
    move(++row, 0);
    outs("�����벼�i��h�[(�ܤ֤@�p��)�H ");
    vget(row, 31, "�X�� [0] ", ans, 3, DOECHO);
    days = atoi(ans);
    if (days < 0)
      days = 0;

    sprintf(buf, "�X�p�� [%d] ", (days == 0)? 1:0);
    vget(row, 46, buf, ans, 5, DOECHO);
    num = atoi(ans);
    if (num < 1 && days == 0)
      num = 1;
    else if(num < 0)
      num = 0;

    vch->vclose = vch->chrono + (days * 24 + num) * 3600;
    str_stamp(vch->cdate, &vch->vclose);
  }

  vch->vsort = (vget(++row, 0, "�}�����G�O�_�Ƨ�(Y/N)�H[N] ", ans, 3, LCECHO) == 'y') ? 's' : ' ';
  vch->vpercent = (vget(++row, 0, "�}�����G�O�_��ܦʤ����(Y/N)�H[N] ", ans, 3, LCECHO) == 'y') ? '%' : ' ';
  vch->vprivate = (vget(++row, 0, "�O�_����벼�W��(Y/N)�H[N] ", ans, 3, LCECHO) == 'y') ? ')' : ' ';

  if (vget(++row, 0, "�O�_����벼���(Y/N)�H[N] ", ans, 3, LCECHO) == 'y')
  {
    vget(++row, 0, "�аݭn�n�J�X���H�W�~�i�H�ѥ[�����벼�H([0]��9999)�G", ans, 5, DOECHO);
    num = atoi(ans);
    if (num < 0)
      num = 0;
    vch->limitlogins = num;

    vget(++row, 0, "�аݭn�o��X���H�W�~�i�H�ѥ[�����벼�H([0]��9999)�G", ans, 5, DOECHO);
    num = atoi(ans);
    if (num < 0)
      num = 0;
    vch->limitposts = num;
  }
}


static void
vch_editG(vch, item, echo)	/* dust.090426: vch_edit()����L�M�Ϊ� */
  VCH *vch;
  int item;
  int echo;		/* DOECHO:�s�W  GCARRY:�ק� */
{
  int num, row, dyear;
  char ans[8];
  struct tm ntm, ftm, ctm;
  /* ntm:�{�b�ɶ�  ftm:���Ӫ��ɶ�  ctm:�ʽL���ɶ�(�ק�ɤ~�|�Ψ�) */
  time_t now;

  clear();

  row = 3;

  if (echo == DOECHO)	/* �u���s�W�ɤ~����L������ */
  {
    vch->maxblt = 1;	/* ��L�N�u���@�� */

    vget(++row, 0, "�C�i����h�ֻȹ��H(50��10000)�G", ans, 6, DOECHO);
    num = atoi(ans);
    if (num < 50)
      num = 50;
    else if (num > 10000)
      num = 10000;
    vch->price = num;
  }

  row++;
  move(++row, 0);
  outs("������L�������ɶ�(�ܤ֤@�p�ɡA�̪���~)�G");
  /* dust.090426: �ƹ�W�u���Φ~���M����ӧP�_�ӤwXD */
  row++;

  time(&now);
  ntm = *localtime(&now);
  ftm = ntm;	/* ��l��ftm */
  if(echo == GCARRY)	/* �ק�ɡA�I��ɶ��|���w�]�� */
    ctm = *localtime(&vch->vclose);
  else	/* �ٳ·Ъ��g�k */
    ctm = *localtime(&now);

  sprintf(ans, "%d", ctm.tm_year + 1900);
  vget(++row, 0, "    �~(�褸)�G", ans, 5, echo);
  num = atoi(ans);
  dyear = num - 1900 - ntm.tm_year;
  if (dyear > 2 || dyear < 0)
    dyear = 0;
  ftm.tm_year = ntm.tm_year + dyear;

  sprintf(ans, "%d", ctm.tm_mon + 1);
  vget(++row, 0, "  ���(1~12)�G", ans, 3, echo);
  num = atoi(ans);
  if (dyear > 2 && num > ntm.tm_mon + 1)
    num = ntm.tm_mon;
  else if(num > 12)
    num = 12;
  else if(num < 1)
    num = 1;
  ftm.tm_mon = num - 1;

  sprintf(ans, "%d", ctm.tm_mday);
  vget(++row, 0, "  ���(1~31)�G", ans, 3, echo);
  num = atoi(ans);
  if(num > 31)
    num = 31;
  else if(num < 1)
    num = 0;
  ftm.tm_mday = num;

  sprintf(ans, "%d", ctm.tm_hour);
  vget(++row, 0, "  �X�I(0~23)�G", ans, 3, echo);
  num = atoi(ans);
  if(num > 23)
    num = 23;
  else if(num < 0)
    num = 0;
  ftm.tm_hour = num;

  sprintf(ans, "%d", ctm.tm_min);
  vget(++row, 0, "  �X��(0~59)�G", ans, 3, echo);
  num = atoi(ans);
  if(num > 59)
    num = 59;
  else if(num < 0)
    num = 0;
  ftm.tm_min = num;

  sprintf(ans, "%d", ctm.tm_sec);
  vget(++row, 0, "  �X��(0~59)�G", ans, 3, echo);
  num = atoi(ans);
  if(num > 59)
    num = 59;
  else if(num < 0)
    num = 0;
  ftm.tm_sec = num;

  vch->vclose = mktime(&ftm);
  if(vch->vclose < vch->chrono + 3600)	/* dust.090426: �ܤ��|��@�p�� */
    vch->vclose = vch->chrono + 3600;

  /* dust.090427: ��L���ݭn�U�C�o���ݩ� */
  vch->vsort = ' ';
  vch->vpercent = ' ';
  vch->vprivate = ' ';

  str_stamp(vch->cdate, &vch->vclose);
}


static int
vlist_edit(vlist, n)
  vitem_t vlist[];
  int n;
{
  int item;
  char buf[80];

  clear();

  prints("�Ш̧ǿ�J�ﶵ (�̦h %d ��)�A�� ENTER �����G", n);

  strcpy(buf, " ) ");
  for (;;)
  {
    item = 0;
    for (;;)
    {
      buf[0] = radix32[item];
      if (!vget((item & 15) + 3, (item / 16) * 40, buf, vlist[item], sizeof(vitem_t), GCARRY)
        || (++item >= n))
	break;
    }
    if (item && vans("�O�_���s��J�ﶵ(Y/N)�H[N] ") != 'y')
      break;
  }
  return item;
}


static int
vlog_seek(fpath)
  char *fpath;
{
  VLOG old;
  int fd;
  int rc = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(VLOG)) == sizeof(VLOG))
    {
      if (!strcmp(old.userid, cuser.userid))
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
vote_add(xo)
  XO *xo;
{
  VCH vch;
  int fd, item, vgamble;
  char *dir, *str, fpath[64], title[TTLEN + 1];
  vitem_t vlist[MAX_CHOICES];
  BRD *brd;
  char *menu[] = {"v��벼", "g�}��L", "q����", NULL};

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;
    
  switch(krans(b_lines, "vq", menu, NULL))
  {
  case 'v':
    vgamble = ' ';
    break;
  case 'g':
    vgamble = '$';
    break;
  default:
    return xo->max ? XO_FOOT : vote_body(xo);	/* �p�G�S������벼�A�n�^�� vote_body() */
  }

  if (!vget(b_lines, 0, "�D�D�G", title, TTLEN + 1, DOECHO))
    return xo->max ? XO_FOOT : vote_body(xo);	/* itoc.011125: �p�G�S������벼�A�n�^�� vote_body() */

  dir = xo->dir;
  if ((fd = hdr_stamp(dir, 0, (HDR *) &vch, fpath)) < 0)
  {
    vmsg("�L�k�إ߻�����");
    return XO_FOOT;
  }
  close(fd);

  if(vgamble == ' ')
    vmsg("�}�l�s�� [�벼����]");
  else
    vmsg("�}�l�s�� [��L����]");

  fd = vedit(fpath, 0); /* Thor.981020: �`�N�Qtalk�����D */
  if (fd)
  {
    unlink(fpath);
    if(vgamble == ' ')
      vmsg("�����벼");
    else
      vmsg("������L");
    return vote_head(xo);
  }

  strcpy(vch.title, title);
  str = strrchr(fpath, '@');

  /* --------------------------------------------------- */
  /* �벼�ﶵ�� : Item					 */
  /* --------------------------------------------------- */

  memset(vlist, 0, sizeof(vlist));
  item = vlist_edit(vlist, (vgamble == ' ')? 32 : 22);

  *str = 'I';
  if ((fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
  {
    vmsg("�L�k�إ߿ﶵ�ɡI");
    return vote_head(xo);
  }
  write(fd, vlist, item * sizeof(vitem_t));
  close(fd);

  if(vgamble == ' ')
    vch_edit(&vch, item, DOECHO);
  else
    vch_editG(&vch, item, DOECHO);

  vch.vgamble = vgamble;

  strcpy(vch.owner, cuser.userid);

  brd = bshm->bcache + currbno;

  brd->bvote |= (vgamble == ' ')? 1 : 2;
  vch.bstamp = brd->bstamp;

  rec_add(dir, &vch, sizeof(VCH));

  if(vgamble == ' ')
    vmsg("�}�l�벼�F�I");
  else
    vmsg("��L�}�l�I");

  return vote_init(xo);
}


static int
vote_edit(xo)
  XO *xo;
{
  int pos;
  VCH *vch, vxx;
  char *dir, fpath[64], *msg;

  /* Thor: for �ק�벼�ﶵ */
  int fd, item;
  vitem_t vlist[MAX_CHOICES];
  char *fname;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  dir = xo->dir;
  vch = (VCH *) xo_pool + (pos - xo->top);

  vxx = *vch;

  /* �ק�D�D */
  if (!vget(b_lines, 0, "�D�D�G", vxx.title, TTLEN + 1, GCARRY))
    return XO_FOOT;

  /* �קﻡ�� */
  fname = vch_fpath(fpath, dir, vch);
  vedit(fpath, 0);	/* Thor.981020: �`�N�Qtalk�����D  */

  /* �ק�ﶵ */
  memset(vlist, 0, sizeof(vlist));
  *fname = 'I';
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    read(fd, vlist, sizeof(vlist));
    close(fd);
  }

  item = vlist_edit(vlist, (vch->vgamble == ' ')? 32 : 20);

  if ((fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
  {
    vmsg("�L�k�إߧ벼�ﶵ��");
    return vote_head(xo);
  }
  write(fd, vlist, item * sizeof(vitem_t));
  close(fd);

  if(vxx.vgamble == ' ')
    vch_edit(&vxx, item, GCARRY);
  else
    vch_editG(&vxx, item, GCARRY);

  if (memcmp(&vxx, vch, sizeof(VCH)))
  {
    if(vxx.vgamble == ' ')
      msg = "�T�w�n�ק�o���벼��(Y/N)�H[N] ";
    else
      msg = "�T�w�n�ק�o����L��(Y/N)�H[N] ";

    if (vans(msg) == 'y')
    {
      *vch = vxx;
      currchrono = vch->chrono;
      rec_put(dir, vch, sizeof(VCH), pos, cmpchrono);
    }
  }

  return vote_head(xo);
}


static int
vote_query(xo)
  XO *xo;
{
  char *dir, *fname, fpath[64], buf[80];
  VCH *vch;
  int cc, pos;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  dir = xo->dir;
  vch = (VCH *) xo_pool + (pos - xo->top);

  fname = vch_fpath(fpath, dir, vch);

  /* more(fpath, (char *) -1); */
  /* dust.090321: ���ӨS�����n�A��ܤ@���벼���� */

  *fname = 'G';
  sprintf(buf, "�w�g�� %d �H�ѥ[�A�T�w�n���ܵ����ɶ��ܡH(Y/N) [N] ", rec_num(fpath, sizeof(VLOG)));
  if (vans(buf) == 'y')
  {
    vget(b_lines, 0, "�Ч�ﵲ���ɶ�(-n���en�p��/+m����m�p��/0����)�G", buf, 5, DOECHO);
    if (cc = atoi(buf))
    {
      vch->vclose += cc * 3600;
      currchrono = vch->chrono;
      /* dust.090321: �令�����ɶ�����Z���}�l�ɶ��W�L�Q�~(315569260��) */
      if(vch->vclose > currchrono + 315569260)
        vch->vclose = currchrono + 315569260;
      else if(vch->vclose < currchrono)
	vch->vclose = currchrono;	/* dust.090321: �H�������ɶ���}�l�ɶ��� */
      str_stamp(vch->cdate, &vch->vclose);
      rec_put(dir, vch, sizeof(VCH), pos, cmpchrono);
    }
  }

  return vote_head(xo); 
}


static int
vfyvch(vch, pos)
  VCH *vch;
  int pos;
{
  return Tagger(vch->chrono, pos, TAG_NIN);
}


static void
delvch(xo, vch)
  XO *xo;
  VCH *vch;
{
  int fd;
  char fpath[64], buf[64], *fname;
  char *list = "@IOLGZ";	/* itoc.����: �Mvote file */
  VLOG vlog;
  PAYCHECK paycheck;

  fname = vch_fpath(fpath, xo->dir, vch);

  if (vch->vgamble == '$')	/* itoc.050313: �p�G�O��L�Q�R���A����n�h��� */
  {
    *fname = 'G';

    if ((fd = open(fpath, O_RDONLY)) >= 0)
    {
      memset(&paycheck, 0, sizeof(PAYCHECK));
      time(&paycheck.tissue);
      sprintf(paycheck.reason, "[��L�h��] %s�O", currboard);
	/* dust.090426: ��]�������[��(�ϥ��{�breason���X�j�L�F) */

      while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
      {
	paycheck.money = vlog.numvotes * vch->price;
	usr_fpath(buf, vlog.userid, FN_PAYCHECK);
	rec_add(buf, &paycheck, sizeof(PAYCHECK));
      }
    }
    close(fd);
  }

  while (*fname = *list++)
    unlink(fpath); /* Thor: �T�w�W�r�N�� */
}



static int
vote_delete(xo)
  XO *xo;
{
  int pos;
  VCH *vch;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  vch = (VCH *) xo_pool + (pos - xo->top);

  if (vans(msg_del_ny) == 'y')
  {
    delvch(xo, vch);

    currchrono = vch->chrono;
    rec_del(xo->dir, sizeof(VCH), pos, cmpchrono);    
    return vote_load(xo);
  }

  return XO_FOOT;
}


static int
vote_rangedel(xo)
  XO *xo;
{
  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  return xo_rangedel(xo, sizeof(VCH), NULL, delvch);
}


static int
vote_prune(xo)
  XO *xo;
{
  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  return xo_prune(xo, sizeof(VCH), vfyvch, delvch);
}


static int
vote_pal(xo)		/* itoc.020117: �s�譭��벼�W�� */
  XO *xo;
{
  char *fname, fpath[64];
  VCH *vch;
  XO *xt;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  vch = (VCH *) xo_pool + (xo->pos - xo->top);

  if (vch->vprivate != ')')
    return XO_NONE;

  fname = vch_fpath(fpath, xo->dir, vch);
  *fname = 'L';

  xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
  xt->key = PALTYPE_VOTE;
  xover(XZ_PAL);		/* Thor: �ixover�e, pal_xo �@�w�n ready */

  free(xt);
  return vote_init(xo);
}


static int
vote_chos(vch, xo)	/* dust.090426: �ѻP�벼���D�n�B�z��� */
  VCH *vch;
  XO *xo;
{
  VCH vbuf;
  VLOG vlog;
  int count, fd;
  usint choice;
  char *dir, *fname, fpath[64], buf[80], *slist[MAX_CHOICES];
  vitem_t vlist[MAX_CHOICES];


  /* --------------------------------------------------- */
  /* �]�w�ɮ׸��|					 */
  /* --------------------------------------------------- */

  dir = xo->dir;
  fname = vch_fpath(fpath, dir, vch);

  /* --------------------------------------------------- */
  /* �ˬd�O�_�w�g��L��					 */
  /* --------------------------------------------------- */

  *fname = 'G';
  if (vlog_seek(fpath))
  {
    vmsg("�z�w�g��L���F�I");
    return XO_FOOT;
  }

  /* --------------------------------------------------- */
  /* �ˬd�벼����					 */
  /* --------------------------------------------------- */

  if (vch->vprivate == ' ')
  {
    if (cuser.numlogins < vch->limitlogins || cuser.numposts < vch->limitposts)
    {
      vmsg("�z������`��I");
      return XO_FOOT;
    }
  }
  else		/* itoc.020117: �p�H�벼�ˬd�O�_�b�벼�W�椤 */
  {
    *fname = 'L';

    /* �ѩ�ä����ۤv�[�J�B�ͦW��A�ҥH�n�h�ˬd�O�_���O�D */
    if (!pal_find(fpath, cuser.userno) && !(bbstate & STAT_BOARD))
    {
      vmsg("�z�S�����ܥ����p�H�벼�I");
      return XO_FOOT;
    }
  }

  /* --------------------------------------------------- */
  /* �T�{�i�J�벼					 */
  /* --------------------------------------------------- */

  if (vans("�O�_�ѥ[�벼(Y/N)�H[N] ") != 'y')
    return XO_FOOT;

  /* --------------------------------------------------- */
  /* �}�l�벼�A��ܧ벼����				 */
  /* --------------------------------------------------- */

  *fname = '@';
  more(fpath, MFST_NORMAL);

  /* --------------------------------------------------- */
  /* ���J�벼�ﶵ��					 */
  /* --------------------------------------------------- */

  *fname = 'I';
  if ((fd = open(fpath, O_RDONLY)) < 0)
  {
    vmsg("�L�kŪ���벼�ﶵ��");
    return vote_head(xo);
  }
  count = read(fd, vlist, sizeof(vlist)) / sizeof(vitem_t);
  close(fd);

  for (fd = 0; fd < count; fd++)
    slist[fd] = (char *) &vlist[fd];

  /* --------------------------------------------------- */
  /* �i��벼						 */
  /* --------------------------------------------------- */

  choice = 0;
  sprintf(buf, "��U���t�� %d ��", vch->maxblt); /* Thor: ��̦ܳh�X�� */
  vs_bar(buf);

  for (;;)
  {
    choice = bitset(choice, count, vch->maxblt, vch->title, slist);

    vget(b_lines - 1, 0, "�ڦ��ܭn���G", buf, 60, DOECHO);

    fd = vans("�벼 (Y)�T�w (N)���� (Q)�����H[N] ");

    if (fd == 'q')
      return vote_head(xo);

    if (fd == 'y')
      break;
  }

  /* --------------------------------------------------- */
  /* �O�����G�G�@���]���몺���p ==> �۷���o��	 */
  /* --------------------------------------------------- */

  /* �T�w�벼�|���I�� */
  /* itoc.050514: �]���O�D�i�H���ܶ}���ɶ��A���F�קK�ϥΪ̷|�t�b vget() �άO
     �Q�� xo_pool[] ���P�B�ӳW�� time(0) > vclose ���ˬd�A�ҥH�N�o���s���J VCH */
  if (rec_get(dir, &vbuf, sizeof(VCH), xo->pos) || vch->chrono != vch->chrono || time(0) > vbuf.vclose)
  {
    vmsg("�벼�w�g�I��F�A���R�Զ}��");
    return vote_init(xo);
  }

  if (*buf)	/* �g�J�ϥΪ̷N�� */
  {
    FILE *fp;

    *fname = 'O';
    if (fp = fopen(fpath, "a"))
    {
      fprintf(fp, "�E%-12s�G%s\n", cuser.userid, buf);
      fclose(fp);
    }
  }

  /* �[�J�O���� */
  memset(&vlog, 0, sizeof(VLOG));
  strcpy(vlog.userid, cuser.userid);
  vlog.numvotes = 1;
  vlog.choice = choice;
  *fname = 'G';
  rec_add(fpath, &vlog, sizeof(VLOG));

  vmsg("�벼�����I");

  return vote_head(xo);
}


static int
gamble_chos(vch, xo)	/* dust.090426: �ѻP��L���D�n�B�z��� */
  VCH *vch;
  XO *xo;
{
  VCH vbuf;
  VLOG vlog;
  int count, fd, i, k;
  usint choice;
  char *dir, *fname, fpath[64], buf[80], ans[4], *slist[MAX_CHOICES];
  vitem_t vlist[MAX_CHOICES];


  /* --------------------------------------------------- */
  /* �ˬd�O�_��������					 */
  /* --------------------------------------------------- */

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XO_FOOT;
  }

  if (vch->vgamble == '$')
  {
    if (cuser.money < vch->price)
    {
      vmsg("�z���������ѥ[����L");
      return XO_FOOT;
    }
  }

  /* --------------------------------------------------- */
  /* �]�w�ɮ׸��|					 */
  /* --------------------------------------------------- */

  dir = xo->dir;
  fname = vch_fpath(fpath, dir, vch);

  /* --------------------------------------------------- */
  /* �T�{�i�J��L					 */
  /* --------------------------------------------------- */

  if (vans("�O�_�ѥ[��L(Y/N)�H[N] ") != 'y')
    return XO_FOOT;

  /* --------------------------------------------------- */
  /* �}�l�벼�A��ܧ벼����				 */
  /* --------------------------------------------------- */

  *fname = '@';
  more(fpath, MFST_NORMAL);

  /* --------------------------------------------------- */
  /* ���J��L�ﶵ��					 */
  /* --------------------------------------------------- */

  *fname = 'I';
  if ((fd = open(fpath, O_RDONLY)) < 0)
  {
    vmsg("�L�kŪ����L�ﶵ��");
    return vote_head(xo);
  }
  count = read(fd, vlist, sizeof(vlist)) / sizeof(vitem_t);
  close(fd);

  for (fd = 0; fd < count; fd++)
    slist[fd] = (char *) &vlist[fd];

  /* --------------------------------------------------- */
  /* �i���L						 */
  /* --------------------------------------------------- */
  
  vs_bar("��L�U�`");

  move(1, 0);
  outs("    1.��L�ѸӪO�O�D�t�d�|��A�èM�w�}���ɶ��P�}�����G�C\n");
  outs("    2.�t�η|�q�`�������� 5% �|���A�䤤�� 20% �����|��̡A���̦h 800 �ȹ��C\n");
  outs("    3.�}���ɥu�|���@�رm�餤���C�����̸̨Ӷ����`�ʶR�i�Ƨ�����|�᪺�`����C\n");
  outs("      �C�i����L����˥h�p���I�᪺���B�A��t���k���t�Ω�|�C\n");
  outs("    4.�U�`�̽Цۦ�Ӿ�t�β��`�ɭP���l���ұa�Ӫ����I�C��h�W���褣�����v�C\n\n");

  for (i = 0; i < count; i++)
  {
    move(i % 11 + 7, (i < 11)? 0:40);
    prints("%s %c %s", "�@", radix32[i], slist[i]);
  }

  move(20, 0);
  prints("�U�`�I��ɶ��G%s", Btime(&vch->vclose));

  for (;;)
  {
    choice = 0;

    k = vans("�n�U�`���@��(����J�h����)�H ") - '0';
    if (k >= 10)
      k -= 'a' - '0' - 10;

    if (k >= 0 && k < count)
    {
      choice ^= (1 << k);
      move(k % 11 + 7, (k < 11)? 0:40);
      outs("��");
    }
    else
      return vote_head(xo);

    sprintf(buf, "�C�i %d �ȹ��A�n�R�X�i�H[1] ", vch->price);
    vget(b_lines, 0, buf, ans, 3, DOECHO);	/* �̦h�R 99 �i�A�קK���� */

    i = atoi(ans);
    if (i < 1)
      i = 1;
    if (vch->price * i > cuser.money)
      i = cuser.money / vch->price;

    fd = i * vch->price;	/* fd �O�n�I����� */

    sprintf(buf, "�n�R %d �i�� [%c] �ܡH (Y)�T�w (N)���� (Q)�����H[N] ", i, radix32[k]);
    k = i;
    i = vans(buf);

    if (i == 'q')
      return vote_head(xo);

    if (i == 'y')
    {
      if (time(0) > vch->vclose)	/* �]����vget()�A�ҥH�٭n�A�ˬd�@�� */
      {
        vmsg("��L�w����U�`�A���R�Զ}��");
        return vote_head(xo);
      }
      break;
    }

    for (i = 0; i < count; i++)
    {
      move(i % 11 + 7, (i < 11)? 0:40);
      outs("�@");
    }
  }

  /* --------------------------------------------------- */
  /* �O�����G						 */
  /* --------------------------------------------------- */

  /* �T�w�벼�|���I�� */
  /* itoc.050514: �]���O�D�i�H���ܶ}���ɶ��A���F�קK�ϥΪ̷|�t�b vget() �άO
     �Q�� xo_pool[] ���P�B�ӳW�� time(0) > vclose ���ˬd�A�ҥH�N�o���s���J VCH */
  if (rec_get(dir, &vbuf, sizeof(VCH), xo->pos) || vch->chrono != vch->chrono || time(0) > vbuf.vclose)
  {
    vmsg("��L�w����U�`�A���R�Զ}��");
    return vote_init(xo);
  }

  cuser.money -= fd;	/* fd �O�n�I����� */

  /* �[�J�O���� */
  memset(&vlog, 0, sizeof(VLOG));
  strcpy(vlog.userid, cuser.userid);
  vlog.numvotes = k;
  vlog.choice = choice;
  *fname = 'G';
  rec_add(fpath, &vlog, sizeof(VLOG));

  vmsg("�U�`�����I");

  return vote_head(xo);
}


static int
vote_join(xo)
  XO *xo;
{
  VCH *vch;

  vch = (VCH *) xo_pool + (xo->pos - xo->top);


  /* dust.090430: �ѥ[�벼�̥����n���o���v�� */
  if(vch->vgamble == ' ' && !(bbstate & STAT_POST))
  {
    vmsg("�z�b���O�L�k�o���A����ѥ[�벼");
    return XO_FOOT;
  }

  /* --------------------------------------------------- */
  /* �ˬd�O�_�w�g����					 */
  /* --------------------------------------------------- */

  if (time(0) > vch->vclose)
  {
    if(vch->vgamble == ' ')
      vmsg("�벼�w�g�I��F�A���R�Զ}��");
    else
      vmsg("��L�w����U�`�A���R�Զ}��");
    return XO_FOOT;
  }

  /* dust.090426: �����ѻP�벼�P�ѻP��L���B�z��� */

  if(vch->vgamble == ' ')
    return vote_chos(vch, xo);
  else
    return gamble_chos(vch, xo);
}



struct Tchoice
{
  int count;
  vitem_t vitem;
};

static int
TchoiceCompare(i, j)
  struct Tchoice *i, *j;
{
  return j->count - i->count;
}


static char *		/* NULL:�إ��ɮץ���(�٨S���H�벼) */
draw_vote(fpath, folder, vch, preview)	/* itoc.030906: �벼���G (�P account.c:draw_vote() �榡�ۦP) */
  char *fpath;
  char *folder;
  VCH *vch;
  int preview;		/* 1:�w��  0:�}�� */
{
  struct Tchoice choice[MAX_CHOICES];
  FILE *fp;
  char *fname;
  int total, items, num, fd, ticket, bollt;
  VLOG vlog;

  fname = vch_fpath(fpath, folder, vch);

  /* vote item */

  *fname = 'I';

  items = 0;
  if (fp = fopen(fpath, "r"))
  {
    while (fread(&choice[items].vitem, sizeof(vitem_t), 1, fp) == 1)
    {
      choice[items].count = 0;
      items++;
    }
    fclose(fp);
  }

  if (items == 0)
    return NULL;

  /* �֭p�벼���G */

  *fname = 'G';
  bollt = 0;		/* Thor: �`�����k�s */
  total = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
    {
      for (ticket = vlog.choice, num = 0; ticket && num < items; ticket >>= 1, num++)
      {
	if (ticket & 1)
	{
	  choice[num].count += vlog.numvotes;
	  bollt += vlog.numvotes;
	}
      }
      total++;
    }
    close(fd);
  }

  /* ���Ͷ}�����G */

  *fname = 'Z';
  if (!(fp = fopen(fpath, "w")))
    return NULL;

  fprintf(fp, "\033[1;32m��[%s] �ݪO�벼�G%s\033[m\n\n�|��H�G%s\n\n", currboard, vch->title, vch->owner);
  fprintf(fp, "�벼�|�����G%s\n\n", Btime(&vch->chrono));
  fprintf(fp, "�벼�����ɶ��G%s\n\n", Btime(&vch->vclose));
  if(!preview)
    fprintf(fp, "�벼�}���ɶ��G%s\n\n", Now());

  fputs("\033[1;32m�� �벼�����G\033[m\n\n", fp);

  *fname = '@';
  f_suck(fp, fpath);

  fputs("\033[m", fp);	/* dust.090426: �[�JANSI�٭�X */

  fprintf(fp, "\n\n\033[1;32m�� �벼���G�G�C�H�i�� %d ���A�@ %d �H�ѥ[�A��X %d ��\033[m\n\n",
    vch->maxblt, total, bollt);

  if (vch->vsort == 's')
    qsort(choice, items, sizeof(struct Tchoice), TchoiceCompare);

  if (vch->vpercent == '%')
    fd = BMAX(1, bollt);	/* �קK���v�ɡA�������s */
  else
    fd = 0;

  for (num = 0; num < items; num++)
  {
    ticket = choice[num].count;
    if (fd)
      fprintf(fp, "    %-36s%5d �� (%4.1f%%)\n", &choice[num].vitem, ticket, 100.0 * ticket / fd);
    else
      fprintf(fp, "    %-36s%5d ��\n", &choice[num].vitem, ticket);
  }


  /* User's Comments */

  *fname = 'O';
  fputs("\n\n\033[1;32m�� �ڦ��ܭn���G\033[m\n\n", fp);
  f_suck(fp, fpath);
  fputs("\n\n\n\n", fp);
  fclose(fp);

  /* �̫�Ǧ^�� fpath �Y���벼���G�� */
  *fname = 'Z';
  return fname;
}


static char *		/* NULL:�إ��ɮץ���(�٨S���H�벼) */
draw_gamble(fpath, folder, vch, preview)	/* dust.090427: ��L���Gø�s */
  char *fpath;
  char *folder;
  VCH *vch;
  int preview;		/* 1:�w��  0:�}�L */
{
  struct Tchoice choice[MAX_CHOICES];
  FILE *fp;
  char *fname;
  int total, items, num, fd, ticket, bollt;
  VLOG vlog;

  fname = vch_fpath(fpath, folder, vch);

  /* �p�⦳�X�ӿﶵ */
  *fname = 'I';

  items = 0;
  if (fp = fopen(fpath, "r"))
  {
    while (fread(&choice[items].vitem, sizeof(vitem_t), 1, fp) == 1)
    {
      choice[items].count = 0;
      items++;
    }
    fclose(fp);
  }

  if (items == 0)
    return NULL;

  /* �֭p�벼���G */

  *fname = 'G';
  bollt = 0;		/* Thor: �`�����k�s */
  total = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
    {
      for (ticket = vlog.choice, num = 0; ticket && num < items; ticket >>= 1, num++)
      {
	if (ticket & 1)
	{
	  choice[num].count += vlog.numvotes;
	  bollt += vlog.numvotes;
	}
      }
      total++;
    }
    close(fd);
  }

  /* ���Ͷ}�����G�� */

  *fname = 'Z';
  if (!(fp = fopen(fpath, "w")))
    return NULL;


  if(preview)
  {
    fprintf(fp, "�� ��L�D�D�G%s\n\n\033[1m�� �U�`�I��ɶ��G%s\033[m\n\n", vch->title, Btime(&vch->vclose));
    fputs("\033[1;32m�� ��L�����G\033[m\n\n", fp);
  }
  else
  {
    fprintf(fp, "��L�D�D�G%s\n\n�|��ɶ��G%s\n", vch->title, Btime(&vch->chrono));
    fprintf(fp, "�I��ɶ��G%s\n\n��L�����G\n\n", Btime(&vch->vclose));
  }

  *fname = '@';
  f_suck(fp, fpath);

  fputs("\033[m", fp);	/* dust.090426: �[�JANSI�٭�X */

  if(preview)
    fprintf(fp, "\n\n\033[1;33m�� �U�`���p�G\033[m\n\n");
  else
    fprintf(fp, "\n\n�U�`���p�G\n\n");


  fd = BMAX(1, bollt);	/* �קK���v�ɡA�������s */

  for (num = 0; num < items; num++)
  {
    ticket = choice[num].count;
    if(preview)
      fprintf(fp, "    %-36s%5d �i (%4.1f%%) �߲v 1:%.3f\n", &choice[num].vitem, ticket, 100.0 * ticket / fd, 0.95 * (bollt + 1) / (ticket + 1));
    else
      fprintf(fp, "    %-36s%5d �i (%5.2f%%)\n", &choice[num].vitem, ticket, 100.0 * ticket / fd);
  }

  if(preview)
    fprintf(fp, "\n%s\n   �`�U�`�ƶq�G\033[1m%d\033[m �i�A�C�i \033[1m%d\033[m �ȹ��C\n\n\n\n", msg_seperator, bollt, vch->price);

  fclose(fp);

  /* �̫�Ǧ^�� fpath �Y���벼���G�� */
  *fname = 'Z';
  return fname;
}


static int
vote_view(xo)
  XO *xo;
{
  char fpath[64];
  VCH *vch;

  vch = (VCH *) xo_pool + (xo->pos - xo->top);

  if ((bbstate & STAT_BOARD) && vch->vgamble == ' ')
  {
    if (draw_vote(fpath, xo->dir, vch, 1))
    {
      more(fpath, MFST_NORMAL);
      unlink(fpath);
      return vote_head(xo);
    }

    vmsg("�ثe�|�����H�벼");
    return XO_FOOT;
  }
  else if(vch->vgamble == '$')
  {
    if (draw_gamble(fpath, xo->dir, vch, 1))
    {
      more(fpath, MFST_NORMAL);
      unlink(fpath);
      return vote_head(xo);
    }

    vmsg("�ثe�|�����H�U�`");
    return XO_FOOT;
  }

  return XO_NONE;
}



static void
keeplogV(fnlog, board, title, eb)	/* ��L�M�Ϊ�keeplog() */
  char *fnlog;
  char *board;
  char *title;
  int eb;		/* 0:���[�W��ñ  1:�[�W��ñ */
{
  HDR hdr;
  char folder[64], fpath[64];
  FILE *fp;

  if (!dashf(fnlog))	/* Kudo.010804: �ɮ׬O�Ū��N�� keeplog */
    return;

  brd_fpath(folder, board, fn_dir);

  if (fp = fdopen(hdr_stamp(folder, 'A', &hdr, fpath), "w"))
  {
    extern char *ebs[EDIT_BANNER_NUM];

    fprintf(fp, "�@��: %s (%s)\n���D: %s\n�ɶ�: %s\n\n",
      str_sysop, "�۰ʼҦ�", title, Btime(&hdr.chrono));
    f_suck(fp, fnlog);
    if(eb)
      fprintf(fp, ebs[time(0) % EDIT_BANNER_NUM], "sysop (�۰ʼҦ�)", "��������");
    fclose(fp);

    strcpy(hdr.title, title);
    strcpy(hdr.owner, str_sysop);
    rec_bot(folder, &hdr, sizeof(HDR));

    btime_update(brd_bno(board));
  }
}


static int	/* 0:���\  1:���`���� */
vlog_pay(fpath, choice, fp, vch, comment, answer)	/* �I������諸�ϥΪ� */
  char *fpath;			/* �O���ɸ��| */
  usint choice;			/* ���T������ */
  FILE *fp;			/* �g�J���ɮ� */
  VCH *vch;
  char *comment;		/* �O�D���� */
  char *answer;			/* ���G�ﶵ(�r��) */
{
  int correct, bollt;		/* ���/���� ������ */
  int single;			/* �C�i�������m��i��o���ȹ� */
  int commission, tax;		/* �O�D�⦨���B/�t�Ω�|���B */
  int fd, money;
  char buf[64];
  VLOG vlog;
  PAYCHECK paycheck;
  FILE *fpl;			/* dust.090508: �O���Ҧ��U�`�̥� */
#define GAMBLE_LOG "tmp/gamble_log.tmp"


/* dust.090426: �o�Ӽg�k�����S���Ҽ{��int���쪺���D......
                �N���]���|�o�ͧa(?) */

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    /* �Ĥ@���j��(��X�߲v���έp���) */
    correct = bollt = 0;
    while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
    {
      bollt += vlog.numvotes;
      if (vlog.choice == choice)
	correct += vlog.numvotes;
    }

    /* ���|��̩��Y 1%(�W��800��) */
    commission = (int) (vch->price / (double)100 * bollt);
    if(commission > 800)
      commission = 800;

    if(correct > 0)
      single = (double)vch->price * 0.95 * bollt / correct;
    else
      single = 0;

    tax = bollt * vch->price - correct * single - commission;

    memset(&paycheck, 0, sizeof(PAYCHECK));
    if(commission > 0)		/* ���Y�j��s�ɤ~�����n�H�䲼 */
    {
      time(&paycheck.tissue);
      paycheck.money = commission;
      sprintf(paycheck.reason, "[���Y] %s�O", currboard);
      usr_fpath(buf, vch->owner, FN_PAYCHECK);
      rec_add(buf, &paycheck, sizeof(PAYCHECK));
    }

    /* �L�X�έp��� */
    fputs("\n\n", fp);
    fprintf(fp, "�}���ɶ��G%s�A�� %s �}���C\n\n", Now(), cuser.userid);
    fprintf(fp, "�}�����G�G%s\n\n", answer);

    if(comment[0])	/* ���g���Ѥ~��X */
      fprintf(fp, "�O�D���ѡG%s\n\n", comment);

    fprintf(fp, "������ҡG%d�i/%d�i (%.2f%%)\n", correct, bollt, correct * 100.0 / bollt);
    fprintf(fp, "�`�U�`���B�� %d �ȹ��A�t�Ω�| %d �ȹ� (%.2f%%)\n", bollt * vch->price, tax, (double)tax * 100 / vch->price / bollt);
    if(commission > 0)
      fprintf(fp, "�|��� %s ���Y�A�o�� %d �ȹ� (%.2f%%)\n", vch->owner, commission, (double)commission * 100 / vch->price / bollt);

    if(correct > 0)	/* ���H�����~�g�J */
    {
      fprintf(fp, "\n�C�i�����m��i�o %d �ȹ��A�߲v 1:%.3f\n", single, 0.95 * bollt / correct);
      fputs("\n\n�H�U�O�����W��G\n\n", fp);
    }

    fpl = fopen(GAMBLE_LOG, "w");
    if(fpl)
    {
      fprintf(fpl, "��L�D�D�G%s\n", vch->title);
      fprintf(fpl, "�}���ɶ��G%s�A�� %s �}���C\n\n", Now(), cuser.userid);
    }

    /* �ĤG���j��(�i��o���B�O�������̻P�O���U�`����) */
    lseek(fd, (off_t) 0, SEEK_SET);
    time(&paycheck.tissue);
    while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
    {
      if(fpl)
	fprintf(fpl, "%s %08X %d\n", vlog.userid, vlog.choice, vlog.numvotes);
      /* dust.090508: �H�Q���i���ƪ��choice�A�o�ˤ���٨�(?) */

      if(correct > 0)	/* ���H�����~�ˬd */
      {
	if (vlog.choice == choice)
	{
	  money = single * vlog.numvotes;
	  fprintf(fp, "    ���� %12s �R�F %2d �i�A�@�i��o %6d �ȹ�\n", vlog.userid, vlog.numvotes, money);

	  paycheck.money = money;
	  sprintf(paycheck.reason, "[��L����] %s�O", currboard);
	  usr_fpath(buf, vlog.userid, FN_PAYCHECK);
	  rec_add(buf, &paycheck, sizeof(PAYCHECK));
	}
      }
    }

    if(fpl)
    {
      fclose(fpl);
      sprintf(buf, "[����] %s�O-��L�U�`�̲M��", currboard);
      keeplogV(GAMBLE_LOG, BN_SECURITY, buf, 0);
      unlink(GAMBLE_LOG);
    }

    close(fd);
  }
  else
    return 1;

  return 0;
}


static int
voting_open(xo, vch, pos)	/* dust.090427: �}���M�Ψ�� */
  XO *xo;
  VCH *vch;
  int pos;
{
  char *dir, *fname, fpath[64], tmpfpath[64], buf[80];
  FILE *fp;

  dir = xo->dir;

  /* �إߧ벼���G�� */

  if (!(fname = draw_vote(fpath, dir, vch, 0)))
  {
    vmsg("�ثe�|�����H�벼");
    return XO_FOOT;
  }

  /* �N�}�����G post �� [BN_RECORD] �P ���ݪO */

  if (!(currbattr & BRD_NOVOTE))
  {
    sprintf(buf, "[�O��] %s <<�ݪO�ﱡ����>>", currboard);
    keeplogV(fpath, BN_RECORD, buf, 0);
  }

  keeplogV(fpath, currboard, "[�O��] �ﱡ����", 1);


  /* �벼���G���[�� @vote */

  setdirpath(tmpfpath, dir, "@/@vote.tmp");
  setdirpath(buf, dir, "@/@vote");
  if (fp = fopen(tmpfpath, "w"))
  {
    fprintf(fp, "\n\033[1;34m%s\033[m\n\n", msg_seperator);
    f_suck(fp, fpath);	/* ���W���G�� */
    f_suck(fp, buf);	/* ���W@vote */
    fclose(fp);
    rename(tmpfpath, buf);
  }

  delvch(xo, vch);	/* �}�����N�R�� */

  currchrono = vch->chrono;
  rec_del(dir, sizeof(VCH), pos, cmpchrono);

  vmsg("�}������");
  return vote_init(xo);
}


static int
gambling_open(xo, vch, pos)	/* dust.090427: ��L�}���M�Ψ�� */
  XO *xo;
  VCH *vch;
  int pos;
{
  int fd, count, i;
  char *dir, *fname, fpath[64], buf[80];
  usint choice;
  char *slist[MAX_CHOICES];
  vitem_t vlist[MAX_CHOICES];
  FILE *fp;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  vch = (VCH *) xo_pool + (pos - xo->top);

  if (time(NULL) < vch->vclose)
  {
    /* dust.090426: ��L���ണ���} */
    if(vch->vgamble == '$')
      return XO_NONE;

    if (vans("�|�����w�}���ɶ��A�T�w�n�����}��(Y/N)�H[N] ") != 'y')
      return XO_FOOT;
  }

  dir = xo->dir;

  /* �إߧ벼���G�� */

  if (!(fname = draw_gamble(fpath, dir, vch, 0)))
  {
    vmsg("�|�����H�ѥ[��L");
    return XO_FOOT;
  }


  /* ���J��L�ﶵ�� */

  *fname = 'I';
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    count = read(fd, vlist, sizeof(vlist)) / sizeof(vitem_t);
    close(fd);

    for (fd = 0; fd < count; fd++)
    {
      slist[fd] = (char *) &vlist[fd];
    }

    /* �O�D���G��L���G */
    choice = 0;
    vs_bar("��L�}��");

    for (;;)
    {
      choice = bitset(choice, count, 1, "�п�ܽ�L���}�����G�G", slist);
      if(choice == 0)
      {				/* ����J���ת��ܫh�������} */
	*fname = 'Z';
	unlink(fpath);
	return vote_head(xo);
      }

      vget(b_lines, 0, "�п�J���ѡG", buf, 60, DOECHO);

      fd = vans("��L�}�� (Y)�T�w (N)���� (Q)�����H[N] ");

      if (fd == 'q')
      {
	*fname = 'Z';
	unlink(fpath);
	return vote_head(xo);
      }

      if (fd == 'y')
	break;
    }

    *fname = 'Z';
    if (fp = fopen(fpath, "a"))
    {
      /* �}�l�o�� & �������G�C�Y return 1 �h�N��o�Ͳ��` */

      for(i=0 ; i < count ; i++)
      {
	if(choice & (1 << i))
	  break;
      }
      if(i >= count) i = count - 1;

      *fname = 'G';
      if(vlog_pay(fpath, choice, fp, vch, buf, slist[i]))
      {
	vmsg("�L�k���J�U�`�����I");
	*fname = 'Z';
	unlink(fpath);
	return vote_head(xo);
      }

      fputs("\n\n\n", fp);
      fclose(fp);
    }

    /* �}�����G */
    *fname = 'Z';
  }
  else
  {
    vmsg("�L�k���J��L�ﶵ��");
    *fname = 'Z';
    unlink(fpath);
    return vote_head(xo);
  }


  /* �N�}�����G post �� [BN_RECORD] �P ���ݪO */

  if (!(currbattr & BRD_NOVOTE))
  {
    sprintf(buf, "[�O��] %s <<�ݪO��L����>>", currboard);
    keeplogV(fpath, BN_RECORD, buf, 0);
  }

  keeplogV(fpath, currboard, "[����] ��L�}��", 1);


  /* �}�����N�R�� */
  vch->vgamble = ' ';	/* �O���D��L�A�p���b delvch �̭��N���|�h��� */
  delvch(xo, vch);

  currchrono = vch->chrono;
  rec_del(dir, sizeof(VCH), pos, cmpchrono);

  vmsg("�}������");
  return vote_init(xo);
}


static int
vote_open(xo)		/* dust.090427: �벼�}��/��L�}�� */
  XO *xo;
{
  int pos;
  VCH *vch;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  vch = (VCH *) xo_pool + (pos - xo->top);

  if (time(NULL) < vch->vclose)
  {
    /* dust.090426: ��L���ണ���} */
    if(vch->vgamble == '$')
      return XO_NONE;

    if (vans("�|�����w�}���ɶ��A�T�w�n�����}��(Y/N)�H[N] ") != 'y')
      return XO_FOOT;
  }

  /* dust.090426: �벼�}���M��L�}�����B�z��Ƥ��� */
  if(vch->vgamble == ' ')
    return voting_open(xo, vch, pos);
  else
    return gambling_open(xo, vch, pos);
}


static int
vote_tag(xo)
  XO *xo;
{
  VCH *vch;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  vch = (VCH *) xo_pool + cur;

  if (tag = Tagger(vch->chrono, pos, TAG_TOGGLE))
  {
    move(3 + cur, 6);
    outc(tag > 0 ? '*' : ' ');
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE;	/* lkchu.981201: ���ܤU�@�� */
}


static int
vote_help(xo)
  XO *xo;
{
  xo_help("vote");
  return vote_head(xo);
}


static KeyFunc vote_cb[] =
{
  XO_INIT, vote_init,
  XO_LOAD, vote_load,
  XO_HEAD, vote_head,
  XO_BODY, vote_body,

  'r', vote_join,	/* itoc.010901: ���k������K */
  'v', vote_join,
  'R', vote_result,

  'V', vote_view,
  'E', vote_edit,
  'o', vote_pal,
  'd', vote_delete,
  'D', vote_rangedel,
  't', vote_tag,
  'b', vote_open,

  Ctrl('D'), vote_prune,
  Ctrl('G'), vote_pal,
  Ctrl('P'), vote_add,
  Ctrl('Q'), vote_query,

  'h', vote_help
};


int
XoVote(xo)
  XO *xo;
{
  char fpath[64];

  /* dust.090430: guest�L�k�벼�ΰѥ[��L */
  if (!cuser.userlevel)
    return XO_NONE;

  setdirpath(fpath, xo->dir, FN_VCH);
  if (!(bbstate & STAT_BOARD) && !rec_num(fpath, sizeof(VCH)))
  {
    vmsg("�ثe�S���벼�|��");
    return XO_FOOT;
  }

  xz[XZ_VOTE - XO_ZONE].xo = xo = xo_new(fpath);
  xz[XZ_VOTE - XO_ZONE].cb = vote_cb;
  xover(XZ_VOTE);
  free(xo);

  return XO_INIT;
}


#if 0	/* dust.090429: �]���{�b���]�p�P�ª����ۮe���t�G�A�Ȯɮ��� */
int
vote_all()		/* itoc.010414: �벼���� */
{
  typedef struct
  {
    char brdname[BNLEN + 1];
    char class[BCLEN + 1];
    char title[BTLEN + 1];
    char BM[BMLEN + 1];
    char bvote;
  } vbrd_t;

  extern char brd_bits[];
  char *str;
  char fpath[64];
  int num, pageno, pagemax, redraw;
  int ch, cur;
  BRD *bhead, *btail;
  XO *xo;
  vbrd_t vbrd[MAXBOARD], *vb;

  bhead = bshm->bcache;
  btail = bhead + bshm->number;
  cur = 0;
  num = 0;

  do
  {
    str = &brd_bits[cur];
    ch = *str;
    if (bhead->bvote && (ch & BRD_W_BIT))
    {
      vb = vbrd + num;
      strcpy(vb->brdname, bhead->brdname);
      strcpy(vb->class, bhead->class);
      strcpy(vb->title, bhead->title);
      strcpy(vb->BM, bhead->BM);
      vb->bvote = bhead->bvote;
      num++;
    }
    cur++;
  } while (++bhead < btail);

  if (!num)
  {
    vmsg("�ثe�����èS������벼");
    return XEASY;
  }

  num--;
  pagemax = num / XO_TALL;
  pageno = 0;
  cur = 0;
  redraw = 1;

  do
  {
    if (redraw)
    {
      /* itoc.����: �ɶq���o�� xover �榡 */
      vs_head("�벼����", str_site);
      prints(NECKER_VOTEALL, d_cols >> 1, "", d_cols - (d_cols >> 1), "");

      redraw = pageno * XO_TALL;	/* �ɥ� redraw */
      ch = BMIN(num, redraw + XO_TALL - 1);
      move(3, 0);
      do
      {
	vb = vbrd + redraw;
	/* itoc.010909: �O�W�Ӫ����R���B�[�����C��C���] BCLEN = 4 */
	prints("%6d   %-13s\033[1;3%dm%-5s\033[m%s %-*.*s %.*s\n",
	  redraw + 1, vb->brdname,
	  vb->class[3] & 7, vb->class,
	  vb->bvote > 0 ? ICON_VOTED_BRD : ICON_GAMBLED_BRD,
	  (d_cols >> 1) + 34, (d_cols >> 1) + 33, vb->title, d_cols - (d_cols >> 1) + 13, vb->BM);

	redraw++;
      } while (redraw <= ch);

      outf(FEETER_VOTEALL);
      move(3 + cur, 0);
      outc('>');
      redraw = 0;
    }

    switch (ch = vkey())
    {
    case KEY_RIGHT:
    case '\n':
    case ' ':
    case 'r':
      vb = vbrd + (cur + pageno * XO_TALL);

      /* itoc.060324: ���P�i�J�s���ݪO�AXoPost() �������ơA�o�̴X�G���n�� */
      if (!vb->brdname[0])	/* �w�R�����ݪO */
	break;

      redraw = brd_bno(vb->brdname);	/* �ɥ� redraw */
      if (currbno != redraw)
      {
	ch = brd_bits[redraw];

	/* �B�z�v�� */
	if (ch & BRD_M_BIT)
	  bbstate |= (STAT_BM | STAT_BOARD | STAT_POST);
	else if (ch & BRD_X_BIT)
	  bbstate |= (STAT_BOARD | STAT_POST);
	else if (ch & BRD_W_BIT)
	  bbstate |= STAT_POST;

	mantime_add(currbno, redraw);

	currbno = redraw;
	bhead = bshm->bcache + currbno;
	currbattr = bhead->battr;
	strcpy(currboard, bhead->brdname);
	str = bhead->BM;
	sprintf(currBM, "�O�D�G%s", *str <= ' ' ? "�x�D��" : str);
#ifdef HAVE_BRDMATE
	strcpy(cutmp->reading, currboard);
#endif

	brd_fpath(fpath, currboard, fn_dir);
#ifdef AUTO_JUMPPOST
	xz[XZ_POST - XO_ZONE].xo = xo = xo_get_post(fpath, bhead);	/* itoc.010910: �� XoPost �q�����y�@�� xo_get() */
#else
	xz[XZ_POST - XO_ZONE].xo = xo = xo_get(fpath);
#endif
	xo->key = XZ_POST;
	xo->xyz = bhead->title;
      }

      sprintf(fpath, "brd/%s/%s", currboard, FN_VCH);
      xz[XZ_VOTE - XO_ZONE].xo = xo = xo_new(fpath);
      xz[XZ_VOTE - XO_ZONE].cb = vote_cb;
      xover(XZ_VOTE);
      free(xo);
      redraw = 1;
      break;

    default:
      ch = xo_cursor(ch, pagemax, num, &pageno, &cur, &redraw);
      break;
    }
  } while (ch != 'q');

  return 0;
}
#endif

