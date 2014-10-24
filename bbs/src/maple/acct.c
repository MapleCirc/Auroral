/*-------------------------------------------------------*/
/* acct.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : account / administration routines	 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#define	_ADMIN_C_


#include "bbs.h"

extern BCACHE *bshm;


/* ----------------------------------------------------- */
/* (.ACCT) �ϥΪ̱b�� (account) subroutines		 */
/* ----------------------------------------------------- */


int
acct_load(acct, userid)
  ACCT *acct;
  char *userid;
{
  int fd;

  usr_fpath((char *) acct, userid, fn_acct);
  fd = open((char *) acct, O_RDONLY);
  if (fd >= 0)
  {
    /* Thor.990416: �S�O�`�N, ���� .ACCT�����׷|�O0 */
    read(fd, acct, sizeof(ACCT));
    close(fd);
  }
  return fd;
}


/* static */	/* itoc.010408: ����L�{���� */
void
acct_save(acct)
  ACCT *acct;
{
  int fd;
  char fpath[64];

  /* itoc.010811: �Y�Q������w�A�N����g�^�ۤv���ɮ� */
  if ((acct->userno == cuser.userno) && HAS_STATUS(STATUS_DATALOCK) && !HAS_PERM(PERM_ALLACCT))
    return;

  usr_fpath(fpath, acct->userid, fn_acct);
  fd = open(fpath, O_WRONLY, 0600);	/* fpath �����w�g�s�b */
  if (fd >= 0)
  {
    write(fd, acct, sizeof(ACCT));
    close(fd);
  }
}


int
acct_userno(userid)
  char *userid;
{
  int fd;
  int userno;
  char fpath[64];

  usr_fpath(fpath, userid, fn_acct);
  fd = open(fpath, O_RDONLY);
  if (fd >= 0)
  {
    read(fd, &userno, sizeof(userno));
    close(fd);
    return userno;
  }
  return 0;
}


/* ----------------------------------------------------- */
/* name complete for user ID				 */
/* ----------------------------------------------------- */
/* return value :					 */
/* 0 : �ϥΪ����� enter ==> cancel			 */
/* -1 : bad user id					 */
/* ow.: �Ǧ^�� userid �� userno				 */
/* ----------------------------------------------------- */


int
acct_get(msg, acct)
  char *msg;
  ACCT *acct;
{
  outz("�� ��J���r����A�i�H���ť���۰ʷj�M");
  
  if (!vget(1, 0, msg, acct->userid, IDLEN + 1, GET_USER))
    return 0;

  if (acct_load(acct, acct->userid) >= 0)
    return acct->userno;

  vmsg(err_uid);
  return -1;
}


/* ----------------------------------------------------- */
/* bit-wise display and setup				 */
/* ----------------------------------------------------- */


#define BIT_ON		"��"
#define BIT_OFF		"�@"

static void
print_bits(str, level)
  char *str;
  usint level;
{
  while(*str)
  {
    outc((level & 1) ? *str : '-');
    level >>= 1;
    str++;
  }
}


static void
bitmsg(msg, str, level)
  char *msg, *str;
  int level;
{
  outs(msg);
  print_bits(str, level);
  outc('\n');
}


usint
bitset(pbits, count, maxon, msg, perms)
  usint pbits;
  int count;			/* �@���X�ӿﶵ */
  int maxon;			/* �̦h�i�H enable �X�� */
  char *msg;
  char *perms[];
{
  int i, j, on;

  move(1, 0);
  clrtobot();
  move(3, 0);
  outs(msg);

  for (i = on = 0, j = 1; i < count; i++)
  {
    msg = BIT_OFF;
    if (pbits & j)
    {
      on++;
      msg = BIT_ON;
    }
    move(5 + (i & 15), (i < 16 ? 0 : 40));
    prints("%c %s %s", radix32[i], msg, perms[i]);
    j <<= 1;
  }

  while (i = vans("�Ы�������]�w�A�Ϋ� [Return] �����G"))
  {
    i -= '0';
    if (i >= 10)
      i -= 'a' - '0' - 10;

    if (i >= 0 && i < count)
    {
      j = 1 << i;
      if (pbits & j)
      {
	on--;
	msg = BIT_OFF;
      }
      else
      {
	if (on >= maxon)
	  continue;
	on++;
	msg = BIT_ON;
      }

      pbits ^= j;
      move(5 + (i & 15), (i < 16 ? 2 : 42));
      outs(msg);
    }
  }
  return (pbits);
}


static usint
setperm(level)
  usint level;
{
  if (HAS_PERM(PERM_SYSOP))
    return bitset(level, NUMPERMS, NUMPERMS, MSG_USERPERM, perm_tbl);

  /* [�b���޲z��] ����� PERM_SYSOP */
  if (level & PERM_SYSOP)
    return level;

  /* [�b���޲z��] �������v�� PERM_ACCOUNTS CHATROOM BOARD SYSOP */
  return bitset(level, NUMPERMS - 4, NUMPERMS - 4, MSG_USERPERM, perm_tbl);
}


/* ----------------------------------------------------- */
/* �b���޲z						 */
/* ----------------------------------------------------- */


static void
bm_list(userid)		/* ��� userid �O���ǪO���O�D */
  char *userid;
{
  int i, num, uidlen, x, y, sp;
  BRD *bcache;

  bcache = bshm->bcache;
  num = bshm->number;
  uidlen = strlen(userid);

  getyx(&y, &x);
  sp = x;
  for(i = 0; i < num; i++)
  {
    if(str_has(bcache[i].BM, userid, uidlen))
    {
      int len = strlen(bcache[i].brdname);

      if(x + len > b_cols - 1)
      {
	if(++y >= b_lines)
	  break;
	x = sp;
	move(y, x);
      }
      outs(bcache[i].brdname);
      outc(' ');

      x += len + 1;
    }
  }
}


static void
adm_log(old, new)
  ACCT *old, *new;
{
  int i;
  usint bit, oldl, newl;
  char *userid, buf[80];

  userid = new->userid;
  alog("���ʸ��", userid);

  if (strcmp(old->passwd, new->passwd))
    alog("���ʱK�X", userid);

  if ((old->money != new->money) || (old->gold != new->gold))
  {
    sprintf(buf, "%-13s��%d��%d ��%d��%d", userid, old->money, new->money, old->gold, new->gold);
    alog("���ʪ���", buf);
  }

  /* Thor.990405: log permission modify */
  oldl = old->userlevel;
  newl = new->userlevel;
  for (i = 0, bit = 1; i < NUMPERMS; i++, bit <<= 1)
  {
    if ((newl & bit) != (oldl & bit))
    {
      sprintf(buf, "%-13s%s %s", userid, (newl & bit) ? BIT_ON : BIT_OFF, perm_tbl[i]);
      alog("�����v��", buf);
    }
  }
}


static void
acct_ADMINshow(u, type)
  ACCT *u;
  int type;
{
  int diff;
  usint ulevel = u->userlevel;
  char *uid = u->userid, buf[80];

  outs("\n\033[1m");

  if (type != 2)	/* type == 2: �f���U��ݹ���� */
    prints("  \033[32m�^��N���G\033[37m%-35s\033[32m�Τ�s���G\033[37m%d\n", uid, u->userno);

  prints("  \033[32m�u��m�W�G\033[37m%-35s\033[32m�֦��ȹ��G\033[37m%d\n", u->realname, u->money);

  prints("  \033[32m�ϥμʺ١G\033[37m%-35s\033[32m�֦������G\033[37m%d\n", u->username, u->gold);

  prints("  \033[32m�ʧO���G\033[37m%-35.2s", "�H�k�k" + u->sex * 2);
  prints("\033[32m�X�ͤ���G\033[37m%d/%02d/%02d\n", u->year, u->month, u->day);

  prints("  \033[32m�W�����ơG\033[37m%-35d\033[32m�峹�g�ơG\033[37m%d\n", u->numlogins, u->numposts);

  if(ulevel & PERM_LABEL)
  {
    outs("  \033[32m�S����ҡG");
    attrset(u->label_attr);
    outs(u->label);
    outs("\033[m\n\033[1m");
  }

  prints("  \033[32m�l��H�c�G\033[37m%s\n", u->email);

  prints("  \033[32m���U����G\033[37m%s\n", Btime(&u->firstlogin));

  prints("  \033[32m���{����G\033[37m%s\n", Btime(&u->lastlogin));

  if (ulevel & PERM_ALLDENY)
  {
    outs("  \033[32m���v�ѼơG\033[37m");
    if ((diff = u->tvalid - time(0)) < 0)
      outs("���v�����w��\n");
    else
      prints("�٦� %d �� %d �p��\n", diff / 86400, (diff % 86400) / 3600);
  }
  else
    prints("  \033[32m�����{�ҡG\033[37m%s\n", (ulevel & PERM_VALID) ? Btime(&u->tvalid) : "����o�����{��");

  usr_fpath(buf, uid, fn_dir);
  prints("  \033[32m�ӤH�H��G\033[37m%d ��\n", rec_num(buf, sizeof(HDR)));

  prints("  \033[32m�W���a�I�G\033[37m%-35s\033[32m�o�H���ơG\033[37m%d\n", u->lasthost, u->numemails);

  bitmsg("  \033[32m�v�����šG\033[37m", STR_PERM, ulevel);

  outs("  \033[32m�ߺD�X�СG\033[37m");
  print_bits(STR_UFO, u->ufo);
  outs(", ");
  print_bits(STR_UFO2, u->ufo2);
  outc('\n');

  if(type != 2 && (ulevel & PERM_BM))
  {
    outs("  \033[32m����O�D�G\033[37m");
    bm_list(uid);
  }

  attrset(7);
}


void
acct_show(u, adm)
  ACCT *u;
  int adm;		/* 0: user info  1: admin  2: reg-form */
{
  char buf[128], fpath[64], address[80] = "\033[30m(none)\n", phone[80] = "\033[30m(none)\n";
  time_t diff;
  FILE *fp;

  clrtobot();
  if(adm)
  {
    acct_ADMINshow(u, adm);
    return;
  }

  outs(INFO_BANNER);
  outs("\n\033[1m");

  prints("  \033[32m�ϥΪ̥N���G\033[37m%-35s", u->userid);
  prints("\033[30m�֦��ȹ�\033[37m %d\n", u->money);

  prints("        \033[32m�ʺ١G\033[37m%-39s", u->username);
  prints("\033[30m����\033[37m %d\n", u->gold);

  prints("    \033[32m�u��m�W�G\033[37m%-39s", u->realname);
  prints("\033[30m�W��\033[37m %d \033[30m��\n", u->numlogins);

  prints("        \033[32m�ʧO�G\033[37m%.2s\033[37C", "�H�k�k" + u->sex * 2);
  prints("\033[30m�峹\033[37m %d \033[30m�g\n", u->numposts);

  prints("    \033[32m�X�ͤ���G\033[37m%d �~ %d �� %d ��\n", u->year, u->month, u->day);

  usr_fpath(fpath, u->userid, FN_RFINFO);
  if(fp = fopen(fpath, "r"))
  {
    fgets(phone, sizeof(phone), fp);
    fgets(address, sizeof(address), fp);
    fclose(fp);
  }
  prints("    \033[32m�s���q�ܡG\033[37m%s", phone);
  prints("    \033[32m���U�a�}�G\033[37m%s", address);

  prints("  \033[32mE-mail�H�c�G\033[37m%s\n", u->email);

  prints("    \033[32m���U����G\033[37m%s\n", Btime(&u->firstlogin));

  prints("    \033[32m�n�J�ɶ��G\033[37m%-33s", Btime(&u->lastlogin));
  if(u->userlevel & PERM_ALLDENY)
  {
    diff = u->tvalid - time(0);

    if(diff <= 0)
      outs("\033[33m���v�����w��A�i�ۦ�ӽд_�v");
    else
    {
      char *unit[] = {"��", "�p��", "����", "��"};
      int i, scale[] = {86400, 3600, 60, 1};

      for(i = 0; diff / scale[i] == 0; i++) ;

      prints("\033[31m�Z�����v����٦� %d %s", diff / scale[i], unit[i]);
    }
  }
  else
  {
    outs("  \033[30m�{�Үɶ�");
    if(u->userlevel & PERM_VALID)
      prints("\033[37m %s", strtok(Btime(&u->tvalid), " "));
    else
      outs("\033[36m <�|�����������{��>");
  }
  outc('\n');

  usr_fpath(buf, u->userid, fn_dir);
  prints("\033[32m�ӤH�H��ƶq�G\033[37m%d ��\n", rec_num(buf, sizeof(HDR)));

  outs("    \033[32m�������d�G\033[37m");
  diff = (time(0) - ap_start) / 60;
  if(diff >= 60)
    prints("%d �p�� ", diff / 60);
  prints("%d ����\n", diff % 60);

#ifdef NEWUSER_LIMIT
  if (u->lastlogin - u->firstlogin < 3 * 86400)
    outs("    \033[35m�s��W���G�T�ѫ�}���v��\n");
#endif

  if(u->userlevel & PERM_BM)
  {
    outs("    \033[32m����O�D�G\033[37m");
    bm_list(u->userid);
  }

  attrset(7);
}


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


int
acct_editlabel(acct, is_adm)
  ACCT *acct;
  int is_adm;
{
  char label[LBLEN + 1], c;
  uschar label_attr;
  char buf[8], *color = "brgylpcw", *s;

  move(13, 0);
  clrtobot();

  outs("\n����� ");
  attrset(acct->label_attr);
  outs(acct->label);
  attrset(7);

  if(!vget(16, 0, "�]�w���Ҭ��G", label, sizeof(label), DOECHO))
    return 0;

  /* �ư��W�� */
  if(!is_adm && belong(FN_ETC_BADLABEL, label))
  {
    vmsg("�L�k�����o�Ӽ��ҡA�ФŨϥί��ȱM�μ��ҩΤ����r��");
    return 0;
  }

  if(!vget(18, 0, "�e���� \033[1;33mB\033[41mR\033[42mG\033[43mY\033[44mL\033[45mP\033[46mC\033[47mW\033[m [W] ", buf, 2, DOECHO))
    c = 'x';
  else
    c = tolower(*buf);
  if(s = strchr(color, c))
    label_attr = s - color;
  else
    label_attr = 7;

  if(!vget(19, 0, "�I���� \033[1;33mB\033[41mR\033[42mG\033[43mY\033[44mL\033[45mP\033[46mC\033[47mW\033[m [B] ", buf, 2, DOECHO))
    c = 'x';
  else
    c = tolower(*buf);
  if(s = strchr(color, c))
    label_attr |= (s - color) << 4;
  else
    label_attr |= 0 << 4;	/* �ӼзǨӼg�A��K�H��ק�w�]�C�� */

  if(vget(20, 0, "�O�_�[�G (Y/N) [Y] ", buf, 2, LCECHO) != 'n')
    label_attr |= 0x08;

  if(is_adm)	/* dust.101005: ��ų��User list��b�ӹL�{�G�A�ҥH����u�����ȯ൹�{�{ */
  {
    if(vget(21, 0, "�O�_�{�{ (Y/N) [N] ", buf, 2, LCECHO) == 'y')
      label_attr |= 0x80;
  }

  if(vans(msg_sure_ny) == 'y')	/* �g�iacct�� */
  {
    acct->label_attr = label_attr;
    strcpy(acct->label, label);
    return 1;
  }
}


void
acct_setup(u, adm)
  ACCT *u;
  int adm;
{
  ACCT x;
  int i, num;
  char *str, buf[80], pass[PSWDLEN + 1];

  acct_show(u, adm);
  memcpy(&x, u, sizeof(ACCT));

  if (adm)
  {
    adm = vans("�]�w 1)��� 2)�v�� 3)���� Q)���� [Q] ");
    if (adm == '2')
      goto set_perm;

    if(adm == '3')
    {
      if(acct_editlabel(&x, 1))
	goto update_acct;
      else
	return;
    }

    if (adm != '1')
      return;
  }
  else
  {
    if (vans("�ק���(Y/N)�H[N] ") != 'y')
      return;
  }

  move(i = 3, 0);
  clrtobot();

  if (adm)
  {
    str = x.userid;
    for (;;)
    {
      /* itoc.010804.����: ��ϥΪ̥N���ɽнT�w�� user ���b���W */
      vget(i, 0, "�ϥΪ̥N��(����Ы� Enter)�G", str, IDLEN + 1, GCARRY);
      if (!str_cmp(str, u->userid) || !acct_userno(str))
	break;
      vmsg("���~�I�w���ۦP ID ���ϥΪ�");
    }
  }
  else
  {
    vget(i, 0, "�нT�{�K�X�G", buf, PSWDLEN + 1, NOECHO);
    if (chkpasswd(u->passwd, buf))
    {
      vmsg("�K�X���~");
      return;
    }
  }

  /* itoc.030223: �u�� PERM_SYSOP ���ܧ��L���Ȫ��K�X */
  if (!adm || !(u->userlevel & PERM_ALLADMIN) || HAS_PERM(PERM_SYSOP))
  {
    i++;
    for (;;)
    {
      if (!vget(i, 0, "�]�w�s�K�X(����Ы� Enter)�G", buf, PSWDLEN + 1, NOECHO))
	break;

      strcpy(pass, buf);
      vget(i + 1, 0, "�ˬd�s�K�X�G", buf, PSWDLEN + 1, NOECHO);
      if (!strcmp(buf, pass))
      {
	str_ncpy(x.passwd, genpasswd(buf), sizeof(x.passwd));
	break;
      }
    }
  }

  i++;
  str = x.username;
  while (1)
  {
    if (vget(i, 0, "��    �١G", str, UNLEN + 1, GCARRY))
      break;
  };

  i++;
  do
  {
    sprintf(buf, "%d", u->year);
    vget(i, 0, "�X�ͦ~�G", buf, 5, GCARRY);
    x.year = atoi(buf);
  } while (x.year < 1);
  do
  {
    sprintf(buf, "%d", u->month);
    vget(i, 22, "����G", buf, 3, GCARRY);
    x.month = atoi(buf);
  } while (x.month < 0 || x.month > 12);
  do
  {
    sprintf(buf, "%d", u->day);
    vget(i, 37, "��G", buf, 3, GCARRY);
    x.day = atoi(buf);
  } while (x.day < 0 || x.day > 31);

  i++;
  sprintf(buf, "�ʧO (0)���� (1)�k�� (2)�k�ʡG[%d] ", u->sex);
  if (vget(i, 0, buf, buf, 3, DOECHO))
    x.sex = (*buf - '0') & 3;

  /* dust.110125: �Y user �O��E-mail�{�ҡA��M�����H�c�C
     ���O���i�o�g�P�_�A�[�W�s��/�����S���A�N���o��workround�F...... */
#ifndef EMAIL_JUSTIFY
  vget(++i, 0, "�q�l�H�c�G", x.email, sizeof(x.email), GCARRY);
#endif

  if (adm)
  {
    i++;
    do
    {
      vget(i, 0, "�u��m�W�G", x.realname, RNLEN + 1, GCARRY);
    } while (strlen(x.realname) < 2);

    sprintf(buf, "%d", u->userno);
    vget(++i, 0, "�Τ�s���G", buf, 11, GCARRY);
    if ((num = atoi(buf)) > 0)
      x.userno = num;

    sprintf(buf, "%d", u->numlogins);
    vget(++i, 0, "�W�u���ơG", buf, 11, GCARRY);
    if ((num = atoi(buf)) >= 0)
      x.numlogins = num;

    sprintf(buf, "%d", u->numposts);
    vget(++i, 0, "�峹�g�ơG", buf, 11, GCARRY);
    if ((num = atoi(buf)) >= 0)
      x.numposts = num;

    if(cuser.userlevel & PERM_SpSYSOP)
    {
      sprintf(buf, "%d", u->money);
      vget(++i, 0, "��    ���G", buf, 11, GCARRY);
      if ((num = atoi(buf)) >= 0)
        x.money = num;

      sprintf(buf, "%d", u->gold);
      vget(++i, 0, "��    ���G", buf, 11, GCARRY);
      if ((num = atoi(buf)) >= 0)
        x.gold = num;
    }

    sprintf(buf, "%d", u->numemails);
    vget(++i, 0, "�o�H���ơG", buf, 11, GCARRY);
    if ((num = atoi(buf)) >= 0)
      x.numemails = num;

    vget(++i, 0, "�W���a�I�G", x.lasthost, sizeof(x.lasthost), GCARRY);
#ifdef EMAIL_JUSTIFY
    vget(++i, 0, "�q�l�H�c�G", x.email, sizeof(x.email), GCARRY);
#endif

    if (vans("�]�w�ߺD(Y/N)�H[N] ") == 'y')
    {
#if 0
      x.ufo = bitset(x.ufo, NUMUFOS, NUMUFOS, MSG_USERUFO, ufo_tbl);
#else
      void *p = DL_get("bin/ufo.so:ufo_main");

      if (!p)
      {
	p = dlerror();
	if(p) vmsg((char *)p);
      }

      ((int (*)(ACCT*, UTMP*))p) (&x, NULL);
#endif
    }

    if (vans("�]�w�v��(Y/N)�H[N] ") == 'y')
    {
set_perm:

      i = setperm(num = x.userlevel);

      if (i == num)
      {
	vmsg("�����ק�");
	if (adm == '2')
	  return;
      }
      else
      {
	x.userlevel = i;

	/* itoc.011120: ��������[�W�{�ҳq�L�v���A�n���[��{�Үɶ� */
	if ((i & PERM_VALID) && !(num & PERM_VALID))
	  time(&x.tvalid);

	/* itoc.050413: �p�G������ʰ��v�A�N�n�ѯ����~��Ӵ_�v */
	if ((i & PERM_ALLDENY) && (i & PERM_ALLDENY) != (num & PERM_ALLDENY))
	  x.tvalid = INT_MAX;
      }
    }
  }

  if (!memcmp(&x, u, sizeof(ACCT)) || vans(msg_sure_ny) != 'y')
    return;

  if (adm)
  {
update_acct:

    if (str_cmp(u->userid, x.userid))
    { /* Thor: 980806: �S�O�`�N�p�G usr�C�Ӧr�����b�P�@partition���ܷ|�����D */
      char dst[80];

      usr_fpath(buf, u->userid, NULL);
      usr_fpath(dst, x.userid, NULL);
      rename(buf, dst);
      /* Thor.990416: �S�O�`�N! .USR�å��@�֧�s, �i�঳�������D */
    }

    /* itoc.010811: �ʺA�]�w�u�W�ϥΪ� */
    /* �Q������L��ƪ��u�W�ϥΪ�(�]�A�����ۤv)�A�� cutmp->status �|�Q�[�W STATUS_DATALOCK
       �o�ӺX�СA�N�L�k acct_save()�A��O�����K�i�H�ק�u�W�ϥΪ̸�� */
    /* �b�����ק�L�~�W�u�� ID �]���� cutmp->status �S�� STATUS_DATALOCK ���X�СA
       �ҥH�N�i�H�~��s���A�ҥH�u�W�p�G�P�ɦ��ק�e�B�ק�᪺�P�@�� ID multi-login�A�]�O�L���C */
    utmp_admset(x.userno, STATUS_DATALOCK | STATUS_COINLOCK);

    /* lkchu.981201: security log */
    adm_log(u, &x);
  }
  else
  {
    /* itoc.010804.����: �u�W�� userlevel/tvalid �O�ª��A.ACCT �̤~�O�s�� */
    if (acct_load(u, x.userid) >= 0)
    {
      x.userlevel = u->userlevel;
      x.tvalid = u->tvalid;
    }
  }

  memcpy(u, &x, sizeof(ACCT));
  acct_save(u);
}


#if 0	/* itoc.010805.���� */

  �{�Ҧ��\�u�[�W PERM_VALID�A�� user �b�U���i���~�۰ʱo�� PERM_POST | PERM_PAGE | PERM_CHAT
  �H�K�s��W���B���v���\�ॢ��

  ������ email �����{�Ҫ̻ݮ��� PERM_VALID | PERM_POST | PERM_PAGE | PERM_CHAT
  �_�h user �i�H�b�U���i���e���N�ϥ� bbs_post

#endif

#if 0	/* itoc.010831.���� */

  �]���u�W cuser.userlevel �ä��O�̷s���A�ϥΪ̦p�G�b�u�W�{�ҩάO�Q���v�A
  �w�Ф��� .ACCT �g���~�O���T�� userlevel�A
  �ҥH�n��Ū�X .ACCT�A�[�J level ��A�\�^�h�C

  �ϥ� acct_seperm(&acct, adm) ���e�n�� acct_load(&acct, userid)�A
  �䤤 &acct ����O &cuser�C
  �ϥΪ̭n���s�W���~�|�����s���v���C

#endif

void
acct_setperm(u, levelup, leveldown)	/* itoc.000219: �[/���v���{�� */
  ACCT *u;
  usint levelup;		/* �[�v�� */
  usint leveldown;		/* ���v�� */
{
  u->userlevel |= levelup;
  u->userlevel &= ~leveldown;

  acct_save(u);
}


/* ----------------------------------------------------- */
/* �W�[���ȹ�						 */
/* ----------------------------------------------------- */


void
addmoney(addend)
  int addend;
{
  if (addend < (INT_MAX - cuser.money))	/* �קK���� */
    cuser.money += addend;
  else
    cuser.money = INT_MAX;
}


void
addgold(addend)
  int addend;
{
  if (addend < (INT_MAX - cuser.gold))	/* �קK���� */
    cuser.gold += addend;
  else
    cuser.gold = INT_MAX;
}


/* ----------------------------------------------------- */
/* �ݪO�޲z						 */
/* ----------------------------------------------------- */


#ifndef HAVE_COSIGN
static
#endif
int			/* 1:�X�k���O�W */
valid_brdname(brd)
  char *brd;
{
  int ch;

  if (!is_alnum(*brd))
    return 0;

  while (ch = *++brd)
  {
    if (!is_alnum(ch) && ch != '.' && ch != '-' && ch != '_')
      return 0;
  }
  return 1;
}


static int
brd_set(brd, row)
  BRD *brd;
  int row;
{
  int i, BMlen, len;
  char *brdname, buf[80], userid[IDLEN + 2];
  ACCT acct;

  i = row;
  brdname = brd->brdname;
  strcpy(buf, brdname);

  for (;;)
  {
    if (!vget(i, 0, MSG_BID, brdname, BNLEN + 1, GCARRY))
    {
      if (i == 1)	/* �}�s�O�Y�L��J�O�W������} */
	return -1;

      strcpy(brdname, buf);	/* Thor: �Y�O�M�ūh�]����W�� */
      continue;
    }

    if (!valid_brdname(brdname))
      continue;

    if (!str_cmp(buf, brdname))	/* Thor: �P�ªO��W�ۦP�h���L */
      break;

    if (brd_bno(brdname) >= 0)
      outs("\n���~�I�O�W�p�P");
    else
      break;
  }

  vget(++i, 0, "�ݪO�����G", brd->class, BCLEN + 1, GCARRY);
  vget(++i, 0, "�ݪO�D�D�G", brd->title, BTLEN + 1, GCARRY);

  /* vget(++i, 0, "�O�D�W��G", brd->BM, BMLEN + 1, GCARRY); */

  /* itoc.010212: �}�s�O/�ק�ݪO�۰ʥ[�W�O�D�v��. */
  /* �ثe���@�k�O�@��J�� id �N�[�J�O�D�v���A�Y�ϳ̫��ܤ��ܰʡA
     �p�G�]���h�[�F�O�D�v���A�b reaper.c �����U */

  i += 4;
  move(i - 2, 0);
  prints("�ثe�O�D�� %s\n�п�J�s���O�D�W��A�Ϋ� [Return] ����", brd->BM);

  strcpy(buf, brd->BM);
  BMlen = strlen(buf);

  while (vget(i, 0, "�п�J�O�D�A�����Ы� Enter�A�M���Ҧ��O�D�Х��u�L�v�G", userid, IDLEN + 1, DOECHO))
  {
    if (!strcmp(userid, "�L"))
    {
      buf[0] = '\0';
      BMlen = 0;
    }
    else if (is_bm(buf, userid))	/* �R���¦����O�D */
    {
      len = strlen(userid);
      if (BMlen == len)
      {
	buf[0] = '\0';
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
	brdname = str_str(buf, userid);
        strcpy(brdname, brdname + len);
      }
      BMlen -= len;
    }
    else if (acct_load(&acct, userid) >= 0 && !is_bm(buf, userid))	/* ��J�s�O�D */
    {
      len = strlen(userid);
      if (BMlen)
      {
	len++;		/* '/' + userid */
	if (BMlen + len > BMLEN)
	{
	  vmsg("�O�D�W��L���A�L�k�N�o ID �]���O�D");
	  continue;
	}
	sprintf(buf + BMlen, "/%s", acct.userid);
	BMlen += len;
      }
      else
      {
	strcpy(buf, acct.userid);
	BMlen = len;
      }      

      acct_setperm(&acct, PERM_BM, 0);
    }
    else
      continue;

    move(i - 2, 0);
    prints("�ثe�O�D�� %s", buf);
    clrtoeol();
  }
  strcpy(brd->BM, buf);


#ifdef HAVE_MODERATED_BOARD
  /* itoc.011208: ��θ��K�Q���ݪO�v���]�w */
  switch (vget(++i, 0, "�ݪO�v�� A)�@�� B)�۩w C)���K D)�n�͡H[Q] ", buf, 3, LCECHO))
  {
  case 'c':
    brd->readlevel = PERM_SYSOP;	/* ���K�ݪO */
    brd->postlevel = 0;
    brd->battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;

  case 'd':
    brd->readlevel = PERM_BOARD;	/* �n�ͬݪO */
    brd->postlevel = 0;
    brd->battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;
#else
  switch (vget(++i, 0, "�ݪO�v�� A)�@�� B)�۩w�H[Q] ", buf, 3, LCECHO))
  {
#endif

  case 'a':
    brd->readlevel = 0;
    brd->postlevel = PERM_POST;		/* �@��ݪO�o���v���� PERM_POST */
    brd->battr &= ~(BRD_NOSTAT | BRD_NOVOTE);	/* �����n�͡����K�O�ݩ� */
    break;

  case 'b':
    if (vget(++i, 0, "�\\Ū�v��(Y/N)�H[N] ", buf, 3, LCECHO) == 'y')
    {
      brd->readlevel = bitset(brd->readlevel, NUMPERMS, NUMPERMS, MSG_READPERM, perm_tbl);
      move(2, 0);
      clrtobot();
      i = 1;
    }

    if (vget(++i, 0, "�o���v��(Y/N)�H[N] ", buf, 3, LCECHO) == 'y')
    {
      brd->postlevel = bitset(brd->postlevel, NUMPERMS, NUMPERMS, MSG_POSTPERM, perm_tbl);
      move(2, 0);
      clrtobot();
      i = 1;
    }
    break;

  default:	/* �w�]���ܰ� */
    break;
  }

  if (vget(++i, 0, "�]�w�ݩ�(Y/N)�H[N] ", buf, 3, LCECHO) == 'y')
    brd->battr = bitset(brd->battr, NUMBATTRS, NUMBATTRS, MSG_BRDATTR, battr_tbl);

  return 0;
}


int			/* 0:�}�O���\ -1:�}�O���� */
brd_new(brd)
  BRD *brd;
{
  int bno;
  char fpath[64];
  char title[TTLEN + 1];
  FILE *fp;
  int xmode = 0;


  vs_bar("�إ߷s�O");

  if (brd_set(brd, 1))
    return -1;

  if (vans(msg_sure_ny) != 'y')
    return -1;

  time(&brd->bstamp);

  /* dust.091010:�Y�b"�ӤH"�������N�[�WBRD_PERSONAL */
  if(!strcmp(brd->class, "�ӤH"))
    brd->battr |= BRD_PERSONAL;

  if ((bno = brd_bno("")) >= 0)
  {
    rec_put(FN_BRD, brd, sizeof(BRD), bno, NULL);
  }
  /* Thor.981102: ����W�Lshm�ݪO�Ӽ� */
  else if (bshm->number >= MAXBOARD)
  {
    vmsg("�W�L�t�Ωү�e�ǬݪO�ӼơA�нվ�t�ΰѼ�");
    return -1;
  }
  else if (rec_add(FN_BRD, brd, sizeof(BRD)) < 0)
  {
    vmsg("�L�k�إ߷s�O");
    return -1;
  }

  gem_fpath(fpath, brd->brdname, NULL);
  mak_dirs(fpath);
  mak_dirs(fpath + 4);

  bshm_reload();		/* force reload of bcache */

  strcpy(fpath, "run/newbrdlog");   /* �Ȧs�� */

  alog("�إ߬ݪO", brd->brdname);	/* dust.090320:�}�O�n�O���C */

  if (fp = fopen(fpath, "w"))
  {
    sprintf(title, "�s�B�� [%s] ����",  brd->brdname);

    /* �峹���Y */

    fprintf(fp, "%s %s (%s)\n",
    str_author1, "[�B�κ޲z��]", "�t�Τ��i");
    fprintf(fp, "���D: %s\n�ɶ�: %s\n\n", title, Now());

    /* �峹���e */

    fprintf(fp, "\033[38m�B�ΦW�١G\033[m\033[1m%s\033[m\n", brd->brdname);
    fprintf(fp, "\033[38m�B�Ϊ��P�G\033[m\033[1m%s\033[m\n\n", brd->title);
    fprintf(fp, "\033[38m�B�ΥD�H�G\033[m\033[1m%s\033[m\n", brd->BM);
    fprintf(fp, "\033[38m�B�Τ��ϡG\033[m\033[1m%s\033[m\n\n", brd->class);

    if (brd->readlevel == PERM_SYSOP)         /* ���K�ݪO */
      fprintf(fp, "\033[38m�B���ݩʡG\033[m\033[1;31m���K�B��\033[m\n\n");
    else if (brd->readlevel == PERM_BOARD)        /* �n�ͬݪO */
      fprintf(fp, "\033[38m�B���ݩʡG\033[m\033[1;33m�n�ͦB��\033[m\n\n");


    fprintf(fp, "\033[38m�����ɶ��G\033[m\033[1;36m%s\033[m\n\n", Now());


    fprintf(fp, "���B�Υѥ����ȪA \033[1;33m%s\033[m �إߡA\n", cuser.userid);
    fprintf(fp, "�w��j�a�h�h���{��  :) \n");
    fclose(fp);

    xmode |= POST_DONE;

    /* �O���K�ݪO�Φn�ͬݪO���ܭn�[�K */
    if (brd->readlevel == PERM_SYSOP || brd->readlevel == PERM_BOARD)
      xmode |= POST_RESTRICT;

    add_post("NewIceHouse", fpath, title, "[�B�κ޲z��]", "[�B�κ޲z��]", xmode);
    unlink(fpath);
  }    

  rhs_save();
  board_main();			/* reload brd_bits[] */

  return 0;
}


void
brd_classchange(folder, oldname, newbrd)	/* itoc.020117: ���� @Class �����ݪO */
  char *folder;
  char *oldname;
  BRD *newbrd;		/* �Y�� NULL�A��ܭn�R���ݪO */
{
  int pos, xmode;
  char fpath[64];
  HDR hdr;

  pos = 0;
  while (!rec_get(folder, &hdr, sizeof(HDR), pos))
  {
    xmode = hdr.xmode & (GEM_BOARD | GEM_FOLDER);

    if (xmode == (GEM_BOARD | GEM_FOLDER))	/* �ݪO��ذϱ��| */
    {
      if (!strcmp(hdr.xname, oldname))
      {
	if (newbrd)	/* �ݪO��W */
	{
	  brd2gem(newbrd, &hdr);
	  rec_put(folder, &hdr, sizeof(HDR), pos, NULL);
	}
	else		/* �ݪO�R�� */
	{
	  rec_del(folder, sizeof(HDR), pos, NULL);
	  continue;	/* rec_del �H�ᤣ�ݭn pos++ */
	}
      }
    }
    else if (xmode == GEM_FOLDER)		/* ���� recursive �i�h�� */
    {
      hdr_fpath(fpath, folder, &hdr);
      brd_classchange(fpath, oldname, newbrd);
    }
    pos++;
  }
}


static inline void
strcat_brdreadlevel(buf, rlevel)
  char *buf;
  usint rlevel;
{
  switch(rlevel)
  {
    case PERM_SYSOP:
      strcat(buf, "���K�O");
      break;
    case PERM_BOARD:
      strcat(buf, "�n�ͪO");
      break;
    case 0:
      strcat(buf, "���}�O");
      break;
    default :
      strcat(buf, "(�ۭq)");
  }
}

static void
adm_brdlog(old, new)
  BRD *old, *new;
{
  int i;
  usint bit, oldl, newl;
  char *brdname, buf[256];

  brdname = new->brdname;

  if(strcmp(old->brdname, brdname))
  {
    sprintf(buf, "%s��%s", old->brdname, brdname);
    alog("���ʬݪO�O�W", buf);
  }

  if (strcmp(old->BM, new->BM))
  {
    sprintf(buf, "%-13s�G\n   %s\n�� %s", brdname, old->BM, new->BM);
    alog("���ʬݪO�O�D", buf);
  }

  if(old->readlevel != new->readlevel)
  {
    sprintf(buf, "%-13s", brdname);

    strcat_brdreadlevel(buf, old->readlevel);	/* dust:�o�O���Fcode�n�ݤ~�g���o�ˡC*/
    strcat(buf, "��");
    strcat_brdreadlevel(buf, new->readlevel);

    alog("���ʬݪO�v��", buf);
  }

  /* �ݪO�X�Эק� */
  oldl = old->battr;
  newl = new->battr;
  for (i = 0, bit = 1; i < NUMBATTRS; i++, bit <<= 1)
  {
    if ((newl & bit) != (oldl & bit))
    {
      sprintf(buf, "%-13s%s %s", brdname, (newl & bit) ? BIT_ON : BIT_OFF, battr_tbl[i]);
      alog("���ʬݪO�X��", buf);
    }
  }
}


void
brd_edit(bno)
  int bno;
{
  BRD *bhdr, newbh;
  char *bname, src[64], dst[64];;

  vs_bar("�ݪO�]�w");
  bhdr = bshm->bcache + bno;
  memcpy(&newbh, bhdr, sizeof(BRD));
  prints("�ݪO�W�١G%s\n�ݪO�����G[%s] %s\n�O�D�W��G%s\n",
    newbh.brdname, newbh.class, newbh.title, newbh.BM);

  bitmsg(MSG_READPERM, STR_PERM, newbh.readlevel);
  bitmsg(MSG_POSTPERM, STR_PERM, newbh.postlevel);
  bitmsg(MSG_BRDATTR, STR_BATTR, newbh.battr);

  switch (vget(8, 0, "(D)�R�� (E)�]�w (A)�]�w�ݩ� (Q)�����H[Q] ", src, 3, LCECHO))
  {
  case 'd':

    if (vget(9, 0, msg_sure_ny, src, 3, LCECHO) != 'y')
    {
      vmsg(MSG_DEL_CANCEL);
    }
    else
    {
      bname = bhdr->brdname;
      if (*bname)	/* itoc.000512: �P�ɬ尣�P�@�ӬݪO�|�y����ذϡB�ݪO���� */
      {
	alog("�R���ݪO", bname);

	gem_fpath(src, bname, NULL);
	f_rm(src);
	f_rm(src + 4);
	brd_classchange("gem/@/@"CLASS_INIFILE, bname, NULL);	/* itoc.020117: �R�� @Class �����ݪO��ذϱ��| */
	memset(&newbh, 0, sizeof(BRD));
	sprintf(newbh.title, "[%s] deleted by %s", bname, cuser.userid);
	memcpy(bhdr, &newbh, sizeof(BRD));
	rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);

	/* itoc.050531: ��O�|�y���ݪO���O���r���ƧǡA�ҥH�n�ץ� numberOld */
	if (bshm->numberOld > bno)
	  bshm->numberOld = bno;

	vmsg("�R�O����");
      }
    }
    break;

  case 'e':

    move(9, 0);
    outs("������ [Return] ���ק�Ӷ��]�w");

    if (!brd_set(&newbh, 11))
    {
      if (memcmp(&newbh, bhdr, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
      {
	bname = bhdr->brdname;
	if (strcmp(bname, newbh.brdname))	/* �ݪO��W�n���ؿ� */
	{
	  /* Thor.980806: �S�O�`�N�p�G�ݪO���b�P�@partition�̪��ܷ|�����D */
	  gem_fpath(src, bname, NULL);
	  gem_fpath(dst, newbh.brdname, NULL);
	  rename(src, dst);
	  rename(src + 4, dst + 4);
	  /* brd_classchange("gem/@/@"CLASS_INIFILE, bname, &newbh); */
	  /* itoc.050329: ���� @Class �����ݪO��ذϱ��| */

	  /* itoc.050520: ��F�O�W�|�y���ݪO���O���r���ƧǡA�ҥH�n�ץ� numberOld */
	  if (bshm->numberOld > bno)
	    bshm->numberOld = bno;
	}
	adm_brdlog(bhdr, &newbh);
	memcpy(bhdr, &newbh, sizeof(BRD));
	rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);
        brd_classchange("gem/@/@"CLASS_INIFILE, bname, &newbh);
      }
    }
    vmsg("�]�w����");
    break;

  case 'a':
    newbh.battr = bitset(newbh.battr, NUMBATTRS, NUMBATTRS, MSG_BRDATTR, battr_tbl);
    if(memcmp(&newbh, bhdr, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
    {
      adm_brdlog(bhdr, &newbh);
      memcpy(bhdr, &newbh, sizeof(BRD));
      rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);
    }
    break;
  }
}


void
brd_title(bno)		/* itoc.000312: �O�D�ק襤��ԭz */
  int bno;
{
  BRD *bhdr, newbh;
  char *blist;

  bhdr = bshm->bcache + bno;
  memcpy(&newbh, bhdr, sizeof(BRD));

  blist = bhdr->BM;

  if (blist[0] > ' ' && is_bm(blist, cuser.userid))
  {
    if (vans("�O�_�ק�ݪO���D(Y/N)�H[N] ") == 'y')
    {
      vget(b_lines, 0, "�ݪO���D�G", newbh.title, BTLEN + 1, GCARRY);
      memcpy(bhdr, &newbh, sizeof(BRD));
      rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);
      brd_classchange("gem/@/@"CLASS_INIFILE, bhdr->brdname, &newbh);
    }
  }
}
