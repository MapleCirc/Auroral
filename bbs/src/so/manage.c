/*-------------------------------------------------------*/
/* manage.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : �ݪO�޲z				 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"

extern BCACHE *bshm;

#ifdef HAVE_TERMINATOR
/* ----------------------------------------------------- */
/* �����\�� : �ط�������				 */
/* ----------------------------------------------------- */


extern char xo_pool[];


#define MSG_TERMINATOR	"�m�ط������١n"

int
post_terminator(xo)		/* Thor.980521: �׷��峹�R���j�k */
  XO *xo;
{
  int mode, type;
  HDR *hdr;
  char keyOwner[80], keyTitle[TTLEN + 1], buf[80];

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_FOOT;

  if(!vget(b_lines, 0, "�п�J�K�X�A�H�~�����"MSG_TERMINATOR"(���T�~�|�~��)�G",
         buf, PSWDLEN + 1, NOECHO))
    return XO_FOOT;

  if(chkpasswd(cuser.passwd, buf))
  {
    vmsg(ERR_PASSWD);
    return XO_FOOT;
  }

  mode = vans(MSG_TERMINATOR "�R�� (1)����@�� (2)������D (3)�۩w�H[Q] ") - '0';

  if (mode == 1)
  {
    hdr = (HDR *) xo_pool + (xo->pos - xo->top);
    strcpy(keyOwner, hdr->owner);
  }
  else if (mode == 2)
  {
    hdr = (HDR *) xo_pool + (xo->pos - xo->top);
    strcpy(keyTitle, str_ttl(hdr->title));		/* ���� Re: */
  }
  else if (mode == 3)
  {
    if (!vget(b_lines, 0, "�@�̡G", keyOwner, 73, DOECHO))
      mode ^= 1;
    if (!vget(b_lines, 0, "���D�G", keyTitle, TTLEN + 1, DOECHO))
      mode ^= 2;
  }
  else
  {
    return XO_FOOT;
  }

  type = vans(MSG_TERMINATOR "�R�� (1)��H�O (2)�D��H�O (3)�Ҧ��ݪO�H[Q] ");
  if (type < '1' || type > '3')
    return XO_FOOT;

  sprintf(buf, "�R��%s�G%.35s ��%s�O�A�T�w��(Y/N)�H[N] ", 
    mode == 1 ? "�@��" : mode == 2 ? "���D" : "����", 
    mode == 1 ? keyOwner : mode == 2 ? keyTitle : "�۩w", 
    type == '1' ? "��H" : type == '2' ? "�D��H" : "�Ҧ���");

  if (vans(buf) == 'y')
  {
    buf[strlen(buf)-17] = '\0' ;
    strcat(buf, "�D�`�T�w��(Y/N)~ [N] ");
    if (vans(buf) == 'y')
    {
      BRD *bhdr, *head, *tail;
      char tmpboard[BNLEN + 1];

      /* Thor.980616: �O�U currboard�A�H�K�_�� */
      strcpy(tmpboard, currboard);

      head = bhdr = bshm->bcache;
      tail = bhdr + bshm->number;
      do				/* �ܤ֦� note �@�O */
      {
        int fdr, fsize, xmode;
        FILE *fpw;
        char fpath[64], fnew[64], fold[64];
        HDR *hdr;

        xmode = head->battr;
        if ((type == '1' && (xmode & BRD_NOTRAN)) || (type == '2' && !(xmode & BRD_NOTRAN)))
	  continue;

      /* Thor.980616: ��� currboard�A�H cancel post */
        strcpy(currboard, head->brdname);

        sprintf(buf, MSG_TERMINATOR "�ݪO�G%s \033[5m...\033[m", currboard);
        outz(buf);
        refresh();

        brd_fpath(fpath, currboard, fn_dir);

        if ((fdr = open(fpath, O_RDONLY)) < 0)
	  continue;

        if (!(fpw = f_new(fpath, fnew)))
        {
	  close(fdr);
	  continue;
        }

        fsize = 0;
        mgets(-1);
        while (hdr = mread(fdr, sizeof(HDR)))
        {
  	  xmode = hdr->xmode;

	  if ((xmode & POST_MARKED) || 
	    ((mode & 1) && strcmp(keyOwner, hdr->owner)) ||
	    ((mode & 2) && strcmp(keyTitle, str_ttl(hdr->title))))
	  {
	    if ((fwrite(hdr, sizeof(HDR), 1, fpw) != 1))
	    {
	      fclose(fpw);
	      close(fdr);
	      goto contWhileOuter;
	    }
	    fsize++;
	  }
	  else
  	  {
	    /* ���ós�u��H */

	    cancel_post(hdr);
	    hdr_fpath(fold, fpath, hdr);
	    unlink(fold);
	  }
        }
        close(fdr);
        fclose(fpw);

        sprintf(fold, "%s.o", fpath);
        rename(fpath, fold);
        if (fsize)
	  rename(fnew, fpath);
        else
  contWhileOuter:
	  unlink(fnew);

        btime_update(brd_bno(currboard));
      } while (++head < tail);

      /* �٭� currboard */
      strcpy(currboard, tmpboard);
      return XO_LOAD;
    }
  }

  return XO_FOOT;
}
#endif	/* HAVE_TERMINATOR */


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�O�W					 */
/* ----------------------------------------------------- */


static int
post_brdtitle(xo)
  XO *xo;
{
  BRD *oldbrd, newbrd;

  oldbrd = bshm->bcache + currbno;
  memcpy(&newbrd, oldbrd, sizeof(BRD));

  /* itoc.����: ���I�s brd_title(bno) �N�i�H�F�A�S�t�A�Z�F�@�U�n�F :p */
  vget(b_lines, 0, "�ݪO���D�G", newbrd.title, BTLEN + 1, GCARRY);

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
    brd_classchange("gem/@/@"CLASS_INIFILE, oldbrd->brdname, &newbrd);
    return XO_HEAD;	/* itoc.011125: �n��ø����O�W */
  }

  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�o�����                               */
/* ----------------------------------------------------- */


static int
post_postlaw_edit(xo)       /* �O�D�۩w�峹�o����� */
  XO *xo;
{
  int mode;
  char fpath[64];

  mode = vans("�峹�o����� (D)�R�� (E)�ק� (Q)�����H[E] ");

  if (mode != 'q')
  {
    brd_fpath(fpath, currboard, FN_POSTLAW);

    if (mode == 'd')
    {
      if(vans("�A�T�w�n�R���ۭq���o������p(Y/N)�H[N] ") == 'y')        /*����~�R*/
        unlink(fpath);
      return XO_FOOT;
    }

    if (vedit(fpath, 0))      /* Thor.981020: �`�N�Qtalk�����D */
      vmsg(msg_cancel);
    return XO_HEAD;
  }
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�i�O�e��				 */
/* ----------------------------------------------------- */


static int
post_memo_edit(xo)
  XO *xo;
{
  int mode;
  char fpath[64];

  mode = vans("�i�O�e�� (D)�R�� (E)�ק� (Q)�����H[E] ");

  if (mode != 'q')
  {
    brd_fpath(fpath, currboard, fn_note);

    if (mode == 'd')
    {
      if(vans("�A�T�w�n�R���i�O�e���p~ (Y/N)�H[N] ") == 'y')	/*����~�R*/
        unlink(fpath);
      return XO_FOOT;
    }

    if (vedit(fpath, 0))	/* Thor.981020: �`�N�Qtalk�����D */
      vmsg(msg_cancel);
    return XO_HEAD;
  }
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�o�����O                               */
/* ----------------------------------------------------- */

#ifdef POST_PREFIX

static int
post_prefix_edit(xo)
  XO *xo;
{
  const char const *default_prefix[PREFIX_MAX] = DEFAULT_PREFIX;
  char *tail_parts[] = {
    "C.�������O�ƶq", "R.�^�_���w�]��", "Q.���}", NULL
  };

  char prefix[PREFIX_MAX][PREFIX_LEN + 2];	/* n.[prefix]  �ҥH�n�B�~2�r�� */
  char *menu[PREFIX_MAX + 5];
  char menu_f[] = "CQ";		/* menu���Ĥ@�� */
  char fpath[64], buf[64];

  int i = 0, prefix_num = NUM_PREFIX_DEF;
  FILE *fp;
  time_t last_modify, old_modify;


  if(!(bbstate & STAT_BOARD))
    return XO_NONE;

  brd_fpath(fpath, currboard, "prefix");

  if(fp = fopen(fpath, "r"))
  {
    int newline_num;	/* dust.100926: �ۮe�󷥥��³]�p */
    struct stat st;

    stat(fpath, &st);
    last_modify = st.st_mtime;

    fgets(buf, sizeof(buf), fp);
    sscanf(buf, "%d %d", &prefix_num, &newline_num);

    for(; i < prefix_num; i++)
    {
      if(!fgets(buf, sizeof(buf), fp))
	break;
      strtok(buf, "\n");
      sprintf(prefix[i], "%d.%s", (i + 1) % 10, buf);
    }
    fclose(fp);
  }
  else
    last_modify = 0;

  old_modify = last_modify;

  for(; i < prefix_num; i++)	/* �񺡦� prefix_num �� */
    sprintf(prefix[i], "%d.%s", (i + 1) % 10, default_prefix[i]);

  menu[0] = menu_f;

  if(prefix_num == 0)
    menu_f[0] = 'C';
  else
    menu_f[0] = '1';
  for(i = 1; i <= prefix_num; i++)
    menu[i] = prefix[i - 1];
  for(i = 1; i <= sizeof(tail_parts) / sizeof(tail_parts[0]); i++)
    menu[prefix_num + i] = tail_parts[i - 1];

  do
  {
    move(b_lines, 0);
    attrset(8);
    if(last_modify)
      prints("Last modify: %s\n", Btime(&last_modify));
    else
      outs("Last modify: (none)\n");
    attrset(7);

    i = pans((b_lines - prefix_num - 7) / 2, 20, "�峹���O", menu);

    if(i == 'c')		/* �ۭq���O�`�� */
    {
      int tmp = i = prefix_num;
      do
      {
	if(vget(b_lines, 0, "�п�J�s�����O�ƶq (0 ~ 10)�G", buf, 3, DOECHO))
	  tmp = atoi(buf);
      }while(tmp < 0 || tmp > 10);

      menu_f[0] = 'C';

      if(prefix_num == tmp)
	continue;

      sprintf(buf, "�T�w�n�N�ƶq�令 %d �ӶܡH (Y/N) [N] ", tmp);
      if(vans(buf) != 'y')
	continue;

      prefix_num = tmp;

      for(; i < prefix_num; i++)	/* �s�W�X�Ӫ��ιw�]�ȶ� */
	sprintf(prefix[i], "%d.%s", (i + 1) % 10, default_prefix[i]);

      if(prefix_num > 0)
	menu_f[0] = '1';

      /* ��sMenu */
      for(i = 1; i <= prefix_num; i++)
	menu[i] = prefix[i - 1];
      for(i = 1; i <= sizeof(tail_parts) / sizeof(tail_parts[0]); i++)
	menu[prefix_num + i] = tail_parts[i - 1];

      time(&last_modify);
    }
    else if(i == 'r')		/* �^�_���w�]�� */
    {
       menu_f[0] = 'R';
#if 1
      if(vans("�A�T�w�n�~�ե��p�H(Y/N) [N] ") != 'y')
#else
      if(vans("�T�w�n�^�_���w�]�ȶܡH(Y/N) [N] ") != 'y')
#endif
	continue;

      /* ��sprefix[] */
      for(i = 0; i < prefix_num; i++)
	strcpy(prefix[i] + 2, default_prefix[i]);

      time(&last_modify);
    }
    else if(isdigit(i))
    {
      menu_f[0] = i;
      if(i == '0')
	i = 9;
      else
	i -= '1';

      strcpy(buf, prefix[i] + 2);
      if(vget(b_lines, 0, "���O�G", buf, PREFIX_LEN, GCARRY))
      {
	if(strcmp(prefix[i] + 2, buf))
	{
	  strcpy(prefix[i] + 2, buf);
	  time(&last_modify);
	}
      }
    }

  }while (i != 'q');

  if(old_modify != last_modify)		/* �N��ʹL��prefix�ɼg�J�w�� */
  {
    if(fp = fopen(fpath, "w"))
    {
      fprintf(fp, "%d\n", prefix_num);
      for (i = 0; i < prefix_num; i++)
	fprintf(fp, "%s\n", prefix[i] + 2);

      fclose(fp);
    }
  }

  return XO_FOOT;
}

#endif      /* POST_PREFIX */


#ifdef HAVE_TEMPLATE

/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�峹�d��                               */
/* ----------------------------------------------------- */

static int
post_template_edit(xo)
  XO *xo;
{
  const char const *default_prefix[PREFIX_MAX] = DEFAULT_PREFIX;
  char *tp_menu[] = {"e�s��", "d�R��", "q����", NULL};

  char prefix[PREFIX_MAX][PREFIX_LEN + 2];	/* _.[prefix]  �ҥH�n�B�~2�r�� */
  char *menu[PREFIX_MAX + 3];
  char menu_f[] = "EE";			/* menu���Ĥ@�� */
  char fpath[64], tpName[32], buf[32];

  int i, prefix_num = NUM_PREFIX_DEF;
  FILE *fp;
  screen_backup_t old_scr;


  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  brd_fpath(fpath, currboard, "prefix");

  i = 0;

  if (fp = fopen(fpath, "r"))
  {
    int nouse;
    fscanf(fp, "%d %d\n", &prefix_num, &nouse);
    
    for (; i < prefix_num; i++)
    {
      if (!fgets(buf, PREFIX_LEN + 1, fp))
        break;
      strcat(buf, "\n");
      *(strstr(buf, "\n")) = '\0';
      sprintf(prefix[i], "%d.%s", (i+1)%10, buf);
    }
    fclose(fp);
  }
  else
  {
    prefix_num = 0;
  }

  /* �Y�ƶq����h�񺡦� prefix_num �� */
  for (; i < prefix_num; i++)
    sprintf(prefix[i], "%d.%s", (i+1)%10, default_prefix[i]);

  if(prefix_num > 0) menu_f[0] = '1';

  menu[0] = menu_f;
  for (i = 1; i <= prefix_num; i++)
    menu[i] = prefix[i - 1];
  menu[prefix_num + 1] = "E.���}";
  menu[prefix_num + 2] = NULL;

  scr_dump(&old_scr);

  do
  {
    i = pans((b_lines - prefix_num - 7) / 2, 20, "��ܭn�s�誺�d��", menu);

    if(isdigit(i))
    {
      menu[0][0] = i;

      switch(krans(b_lines, "eq", tp_menu, "�A�{�b�n..."))
      {

      case 'e':
	/* �s��d�� */
	sprintf(tpName, "template.%d", i - '0');
	brd_fpath(fpath, currboard, tpName);

	if(vedit(fpath, 0))
	  vmsg(msg_cancel);

	scr_restore(&old_scr);	/* edit�ɵe���|���� */
	scr_dump(&old_scr);

	break;

      case 'd':
	/* �R���d�� */
	if(vans(MSG_SURE_NY) == 'y')
	{
	  sprintf(tpName, "template.%d", i - '0');
	  brd_fpath(fpath, currboard, tpName);
	  unlink(fpath);
	}
	break;

      case 'q':
	break;

      }

      move(b_lines, 0);
      clrtoeol();
    }

  } while (i != 'e');


  scr_restore(&old_scr);

  return XO_FOOT;
}

#endif


/* ----------------------------------------------------- */
/* �O�D�\�� : �ݪO�ݩ�					 */
/* ----------------------------------------------------- */


#ifdef HAVE_SCORE
static int
post_battr_noscore(xo)
  XO *xo;
{
  BRD *oldbrd, newbrd;

  oldbrd = bshm->bcache + currbno;
  memcpy(&newbrd, oldbrd, sizeof(BRD));

  switch (vans("�}����� (1)���\\ (2)���\\ (Q)�����H[Q] "))
  {
  case '1':
    newbrd.battr &= ~BRD_NOSCORE;
    break;
  case '2':
    newbrd.battr |= BRD_NOSCORE;
    break;
  default:
    return XO_FOOT;
  }

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
    currbattr = newbrd.battr;	/* dust.090322: �Y�ɧ�s�{�b�ݪO���X�� */
  }

  return XO_FOOT;
}
#endif	/* HAVE_SCORE */


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�O�D�W��				 */
/* ----------------------------------------------------- */


static int
post_changeBM(xo)
  XO *xo;
{
  char buf[80], userid[IDLEN + 2], *blist;
  BRD *oldbrd, newbrd;
  ACCT acct;
  int BMlen, len;

  oldbrd = bshm->bcache + currbno;

  blist = oldbrd->BM;
  if (is_bm(blist, cuser.userid) != 1)	/* �u�����O�D�i�H�]�w�O�D�W�� */
    return XO_FOOT;

  memcpy(&newbrd, oldbrd, sizeof(BRD));

  move(3, 0);
  clrtobot();

  move(8, 0);
  prints("�ثe�O�D�� %s\n�п�J�s���O�D�W��A�Ϋ� [Return] ����", oldbrd->BM);

  strcpy(buf, oldbrd->BM);
  BMlen = strlen(buf);

  while (vget(10, 0, "�п�J�ƪO�D�A�����Ы� Enter�A�M���Ҧ��ƪO�D�Х��u�L�v�G", userid, IDLEN + 1, DOECHO))
  {
    if (!strcmp(userid, "�L"))
    {
      strcpy(buf, cuser.userid);
      BMlen = strlen(buf);
    }
    else if (is_bm(buf, userid))	/* �R���¦����O�D */
    {
      len = strlen(userid);
      if (!str_cmp(cuser.userid, userid))
      {
	vmsg("���i�H�N�ۤv���X�O�D�W��");
	continue;
      }
      else if (!str_cmp(buf + BMlen - len, userid))	/* �W��W�̫�@��AID �᭱���� '/' */
      {
	buf[BMlen - len - 1] = '\0';			/* �R�� ID �Ϋe���� '/' */
	len++;
      }
      else						/* ID �᭱�|�� '/' */
      {
	str_lower(userid, userid);
	strcat(userid, "/");
	len++;
	blist = str_str(buf, userid);
	strcpy(blist, blist + len);
      }
      BMlen -= len;
    }
    else if (acct_load(&acct, userid) >= 0 && !is_bm(buf, userid))	/* ��J�s�O�D */
    {
      len = strlen(userid) + 1;	/* '/' + userid */
      if (BMlen + len > BMLEN)
      {
	vmsg("�O�D�W��L���A����N�o ID �]���O�D");
	continue;
      }
      sprintf(buf + BMlen, "/%s", acct.userid);
      BMlen += len;

      acct_setperm(&acct, PERM_BM, 0);
    }
    else
      continue;

    move(8, 0);
    prints("�ثe�O�D�� %s", buf);
    clrtoeol();
  }
  strcpy(newbrd.BM, buf);

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);

    sprintf(currBM, "�O�D:%s", newbrd.BM);
    return XO_HEAD;	/* �n��ø���Y���O�D */
  }

  return XO_BODY;
}


#ifdef HAVE_MODERATED_BOARD
/* ----------------------------------------------------- */
/* �O�D�\�� : �ݪO�v��					 */
/* ----------------------------------------------------- */


static int
post_brdlevel(xo)
  XO *xo;
{
  BRD *oldbrd, newbrd;

  oldbrd = bshm->bcache + currbno;
  memcpy(&newbrd, oldbrd, sizeof(BRD));

  switch (vans("1)���}�ݪO 2)���K�ݪO 3)�n�ͬݪO�H[Q] "))
  {
  case '1':				/* ���}�ݪO */
    newbrd.readlevel = 0;
    newbrd.postlevel = PERM_POST;
    newbrd.battr &= ~(BRD_NOSTAT | BRD_NOVOTE);
    break;

  case '2':				/* ���K�ݪO */
    newbrd.readlevel = PERM_SYSOP;
    newbrd.postlevel = 0;
    newbrd.battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;

  case '3':				/* �n�ͬݪO */
    newbrd.readlevel = PERM_BOARD;
    newbrd.postlevel = 0;
    newbrd.battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;

  default:
    return XO_FOOT;
  }

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
  }

  return XO_FOOT;
}
#endif	/* HAVE_MODERATED_BOARD */


#ifdef HAVE_MODERATED_BOARD
/* ----------------------------------------------------- */
/* �O�ͦW��Gmoderated board				 */
/* ----------------------------------------------------- */


static void
bpal_cache(fpath, op)
  char *fpath;
  int op;
{
  BPAL *bpal;
  bpal = bshm->pcache + currbno;

  switch(op)
  {
  case PALTYPE_BPAL:
    bpal->pal_max = image_pal(fpath, bpal->pal_spool);
    break;
  case PALTYPE_BWLIST:
    bpal->bwlist_max = image_pal(fpath, bpal->bwlist_pool);
    break;
  case PALTYPE_BVLIST:
    bpal->bvlist_max = image_pal(fpath, bpal->bvlist_pool);
    break;
  }
}


extern XZ xz[];


static int
XoBM(xo)
  XO *xo;
{
  XO *xt;
  char fpath[64];

  brd_fpath(fpath, currboard, fn_pal);
  xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
  xt->key = PALTYPE_BPAL;
  xover(XZ_PAL);		/* Thor: �ixover�e, pal_xo �@�w�n ready */

  /* build userno image to speed up, maybe upgreade to shm */

  bpal_cache(fpath, PALTYPE_BPAL);

  free(xt);

  return XO_INIT;
}


/* dust.090315: ����W��\�� */
static int
XoBW(xo)
  XO *xo;
{
  XO *xt;
  char fpath[64];

  brd_fpath(fpath, currboard, FN_BWLIST);
  xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
  xt->key = PALTYPE_BWLIST;
  xover(XZ_PAL);

  /* build userno image to speed up */
  bpal_cache(fpath, PALTYPE_BWLIST);

  free(xt);

  return XO_INIT;
}


/* floatJ.091017: �O�U�W��\�� */
static int
XoBV(xo)
  XO *xo;
{
  XO *xt;
  char fpath[64];

  char *blist;
  BRD *oldbrd;

  oldbrd = bshm->bcache + currbno;
  blist = oldbrd->BM;

  if (is_bm(blist, cuser.userid) != 1)  /* �u�����O�D�i�H�]�w�O�U�W�� */
    return XO_FOOT;

  brd_fpath(fpath, currboard, FN_BVLIST);
  xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
  xt->key = PALTYPE_BVLIST;
  xover(XZ_PAL);

  /* build userno image to speed up */
  bpal_cache(fpath, PALTYPE_BVLIST);

  free(xt);

  return XO_INIT;
}
#endif	/* HAVE_MODERATED_BOARD */


static int
post_usies(xo)
  XO *xo;
{
  char fpath[64];

  brd_fpath(fpath, currboard, "usies");
  if (more(fpath, MFST_NORMAL) >= 0 && vans("�аݭn�R���o�ǬݪO�\\Ū�O����(Y/N)�H[N] ") == 'y')
    unlink(fpath);

  return XO_HEAD;
}



int
post_binfo(xo)		/* dust.090321: ��ܬݪO��T */
  XO *xo;
{
  BRD *currbrd, newbrd;
  BPAL *bpal;
  int quit, ret;

  char *blist;
  BRD *oldbrd;

  oldbrd = bshm->bcache + currbno;
  blist = oldbrd->BM;


  currbrd = bshm->bcache + currbno;
  bpal = bshm->pcache + currbno;
  memcpy(&newbrd, currbrd, sizeof(BRD));

  currbattr = currbrd->battr;		/* �j����scurrbattr */


  move(8, 0);
  clrtobot();

  outs("\033[m\033[1;34m�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b�b\033[m\n");

  if(!(bbstate & STAT_BOARD))
    outs("\033[1;33;44m��\033[37m   �H�U�O�z�d�ߪ��ݪO��T                                                   \033[m\n");
  else
    outs("\033[1;33;44m��\033[37m   �H�U�O�D�H�z�d�ߪ��ݪO��T >/////<                                       \033[m\n");

  prints("\n\033[1m�ݪO�W��\033[m�G%-16s\033[1m�ݪO�����G\033[m[%s]", currboard, currbrd->class);
  if(HAS_PERM(PERM_ALLBOARD) && (currbattr & BRD_PERSONAL))
    outs(" \033[1;30m*\033[m");

  prints("\n\033[1m�ݪO���D\033[m�G%s", currbrd->title);
  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))
  {
    move(12, 59);
    outs("\033[1;30mb)�ݪO�\\Ū����\033[m");
  }

  prints("\n\033[1m�O�D�W��\033[m�G%s", currbrd->BM);
  if(bbstate & STAT_BOARD)
  {
    move(13, 59);
    outs("\033[1;30mo)�O�ͦW��\033[m");
  }

  prints("\n\033[1m�}�O�ɶ�\033[m�G%s", Btime(&(currbrd->bstamp)));
  if(bbstate & STAT_BOARD)
  {
    move(14, 59);
    outs("\033[1;30mw)����W��\033[m");
  }

  if(bbstate & STAT_POST)
    prints("\n\033[%sm�o�孭��G�z%s\033[m", "1", "�i�H�o��");
  else if(is_bwater(bpal))
    prints("\n\033[%sm�o�孭��G�z%s\033[m", "1;31", "���Q����");
  else
    prints("\n\033[%sm�o�孭��G�z%s\033[m", "31", "�L�k�o��");
  if(is_bm(blist, cuser.userid)== 1)	/* �u�����O�D�i�H�]�w�O�U�W�� */
  {
    move(15, 59);
    outs("\033[1;30mv)�O�U�W��\033[m");
  }

  prints("\n��H�]�w�G���ݪO\033[1;36m%s�O\033[m��H�O", (currbattr & BRD_NOTRAN)? "��":"");
  if(!(currbattr & BRD_NOTRAN))		/* �D��H�O���|�X�{�o���]�w */
  {
    prints("�A�w�]�s��\033[1m%s\033[m", (currbattr & BRD_LOCALSAVE)? "����":"��X");
    if(bbstate & STAT_BOARD)
      outs("                     \033[1;30mi)�վ㦹���]�w\033[m");
  }

  prints("\n�έp�����G���ݪO�峹\033[1;32m%s�|\033[m�C�J�������D�έp", (currbattr & BRD_NOSTAT)? "��":"");

  prints("\n�ΦW�ݪO�G���ݪO\033[1;33m%s�O\033[m�ΦW�O", (currbattr & BRD_ANONYMOUS)? "":"��");

  prints("\n�����T�G�q���ݪO��X�峹\033[1;35m%s�|\033[m�b���d�U����", (currbattr & BRD_CROSSNOREC)? "��":"");
  if(bbstate & STAT_BOARD)
  {
    move(19, 59);
    outs("\033[1;30mc)�վ㦹���]�w\033[m");
  }

  prints("\n�^�����šG���ݪO\033[1;32m%s\033[m�����峹(����)", (currbattr & BRD_NOSCORE)? "����":"�i�H");
  if(bbstate & STAT_BOARD)
  {
    move(20, 59);
    outs("\033[1;30ms)�վ㦹���]�w\033[m");
  }

  prints("\n�[�K�峹�G�L�k�\\Ū���ݪO�K��(X)���H\033[1;36m��%s��\033[m�Ӥ峹�����D", (currbattr & BRD_XRTITLE)? "�o":"��");
  if(bbstate & STAT_BM)
  {
    move(21, 59);
    outs("\033[1;30mx)�վ㦹���]�w\033[m");
  }

  outs("\n\033[34m�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w\033[m\n");


  move(b_lines, 0);
  if(bbstate & STAT_BOARD)
    outs("\033[1;36m�� �Ы��۹���������ק�]�w���[�ݦW��A����L�����}:          \033[37;44m [�����N���~��] \033[m");
  else
    outs(VMSG_NULL);

  quit = 0;
  ret = XO_BODY;

  do
  {
    move(b_lines, 0);
    switch(vkey())
    {
    case 'b':
      if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))
        ret = post_usies(xo);
      quit = 1;
      break;

    case 'o':
      if(bbstate & STAT_BOARD)
        ret = XoBM(xo);
      quit = 1;
      break;

    case 'w':
      if(bbstate & STAT_BOARD)
        ret = XoBW(xo);
      quit = 1;
      break;

    case 'v':
      /* floatJ.091018: ��Ϊ����P�w�O�_�����O�D�A�קK�qXoBV���X�٭n�B�z��ø���D */
      if (is_bm(blist, cuser.userid)== 1)  /* �u�����O�D�i�H�]�w�O�U�W�� */
        ret = XoBV(xo);
      quit = 1;
      break;

    case 'i':
      /* ����H���ݪO�~�|���o�� */
      if((bbstate & STAT_BOARD) && !(currbattr & BRD_NOTRAN))
      {
        newbrd.battr ^= BRD_LOCALSAVE;
	move(16, 34);
	outs((newbrd.battr & BRD_LOCALSAVE)? "\033[1m����":"\033[1m��X");
      }
      else
        quit = 1;
      break;

    case 'c':
      if(bbstate & STAT_BOARD)
      {
        newbrd.battr ^= BRD_CROSSNOREC;
	move(19, 26);
	attrset(13);
	outs((newbrd.battr & BRD_CROSSNOREC)? "���|":"�|");
	attrset(7);
	outs("�b���d�U����  ");
      }
      else
        quit = 1;
      break;

    case 's':
      if(bbstate & STAT_BOARD)
      {
        newbrd.battr ^= BRD_NOSCORE;
	move(20, 16);
	attrset(10);
	outs((newbrd.battr & BRD_NOSCORE)? "\033[1;32m����":"\033[1;32m�i�H");
	attrset(7);
      }
      else
        quit = 1;
      break;

    case 'x':
      if(bbstate & STAT_BM)
      {
        newbrd.battr ^= BRD_XRTITLE;
	move(21, 37);
	attrset(14);
	outs((newbrd.battr & BRD_XRTITLE)? "�o":"��");
	attrset(7);
      }
      else
        quit = 1;
      break;

    default:
      quit = 1;
    }

  }while(!quit);

  if (memcmp(&newbrd, currbrd, sizeof(BRD)))
  {
    memcpy(currbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
    currbattr = newbrd.battr;	/* dust.090322: �Y�ɧ�s�{�b�ݪO���X�� */
  }

  return ret;
}



/* ----------------------------------------------------- */
/* �O�D���						 */
/* ----------------------------------------------------- */


int
post_manage(xo)
  XO *xo;
{
#ifdef POPUP_ANSWER
  char *menu[] = 
  {
    "BQ",
    "BTitle  �ק�ݪO���D",
    "WMemo   �s��i�O�e��",
#  ifdef POST_PREFIX
    "RPrefix �s��峹���O",
#    ifdef HAVE_TEMPLATE
    "Example �s��峹�d��",
#    endif
#  endif
    "PostLaw �s��o�����",
    "Manager �W��ƪO�D",
    "Usies   �ݪO�\\Ū�O��",
#  ifdef HAVE_SCORE
    "Score   �]�w�i�_����",
#  endif
#  ifdef HAVE_MODERATED_BOARD
    "Level   ���}/�n��/���K",
    "OPal    �O�ͦW��",
    "XBWlist ����W��",
/*  "VBVlist �O�U�W��",*/
#  endif
    NULL
  };
#else
   char *menu = "�� �O�D��� (B)���D (W)�i�O  (M)�ƪO (U)����"
#  ifdef POST_PREFIX
    " (R)���O" 
#  endif
    " (P)����"
#  ifdef HAVE_SCORE
    " (S)����"
#  endif
#  ifdef HAVE_MODERATED_BOARD
    " (L)�v�� (O)�O��" "(X)����"
#  endif
    "�H[Q] ";
#endif

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

#ifdef POPUP_ANSWER
  switch (pans(3, 20, "�O�D���", menu))
#else
  switch (vans(menu))
#endif
  {
  case 'b':
    return post_brdtitle(xo);

  case 'w':
    return post_memo_edit(xo);

#ifdef POST_PREFIX
  case 'r':
    return post_prefix_edit(xo);
#ifdef HAVE_TEMPLATE
  case 'e':
    return post_template_edit(xo);
#endif

#endif

  case 'p':
    return post_postlaw_edit(xo);

  case 'm':
    return post_changeBM(xo);

  case 'u':
   return post_usies(xo);

#ifdef HAVE_SCORE
  case 's':
    return post_battr_noscore(xo);
#endif

#ifdef HAVE_MODERATED_BOARD
  case 'l':
    return post_brdlevel(xo);

  case 'o':
    return XoBM(xo);

  case 'x':
    return XoBW(xo);
  
  case 'v':
    return XoBV(xo);
#endif
  }

  return XO_FOOT;
}

