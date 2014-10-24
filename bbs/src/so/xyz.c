/*-------------------------------------------------------*/
/* xyz.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : ���C���K���~��				 */
/* create : 01/03/01					 */
/* update : 09/03/20					 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef SP_SEND_MONEY

/* ----------------------------------------------------- */
/* '08�~�����S�� - �����x���\��				 */
/* ----------------------------------------------------- */

/* --------------------------------------------- */
/* Link List routines				 */
/* --------------------------------------------- */

struct NOD{
  char id[16];
  int money,gold;
  struct NOD* next;
};

#define LinkList	struct NOD

LinkList *mohead = NULL;	/* head of link list */
LinkList *motail = NULL;	/* tail of link list */

static void
mo_free()
{
  LinkList *next;
  
  while (mohead)
  {
    next = mohead->next;
    free(mohead);
    mohead = next;
  }
  
  mohead = motail = NULL;
}

static LinkList*
mo_has(name)		/* check sth. if it exits in list */
  char *name;
{
  LinkList *list;

  for (list = mohead; list; list = list->next)
  {
    if (!strcmp(list->id, name))
      return list;
  }
  return NULL;
}

static void
mo_add(id, money, gold)		/* add node */
  char *id;
  int money;
  int gold;
{
  if (mohead)
  {
    motail->next = (LinkList*) malloc(sizeof(struct NOD));
    motail = motail->next;
    
    motail->next = NULL;
    strcpy(motail->id, id);
    motail->money = money;
    motail->gold = gold;
  }
  else
  {
    mohead = (LinkList*) malloc(sizeof(struct NOD));
    
    mohead->next = NULL;
    strcpy(mohead->id, id);
    mohead->money = money;
    mohead->gold = gold;
    
    motail = mohead;
  }
}

static int
mo_del(name)		/* delete node (���\�^��1,���Ѧ^��0) */
  char *name;
{
  LinkList *list, *prev, *next;

  prev = NULL;
  for (list = mohead; list; list = next)
  {
    next = list->next;
    if (!strcmp(list->id, name))
    {
      if (prev == NULL)
	mohead = next;
      else
	prev->next = next;

      if (list == motail)
	motail = prev;

      free(list);
      return 1;
    }
    prev = list;
  }
  return 0;
}

static void
mo_out(row, column, msg)
/* row, col�N��ø�ϰ_�l��m�Amsg���@�}�l��ܪ��T�� */
  int row, column;
  char *msg;
{
  int len;
  LinkList *list;
  char buf[64];

  move(row, column);
  outs(msg);

  column = 80;		/* ���F�j����...�� */
  for (list = mohead; list; list = list->next)
  {
    sprintf(buf, "%s-%d+%d", list->id, list->money, list->gold);
    len = strlen(buf) + 1;
    if (column + len > 78)
    {
      if (++row > b_lines - 2)
      {
	move(b_lines, 60);
	outs("\033[1m(�U��...)\033[m");
	break;
      }
      else
      {
	column = len;
	outc('\n');
      }
    }
    else
    {
      column += len;
      outc(' ');
    }
    outs(buf);
  }
}

/* -------- End of Linked List routines -------- */


/* ���o�ӥ\��g��vans�A����w��m */
static int
setans(row, col, msg)
  int row;
  int col;
  char *msg;
{
  char ans[2];
  return tolower(vget(row, col, msg, ans, 2, DOECHO));
}


/* �D�{��(����: Ver 1.0 release) */
int
year_bonus()
{
  char folder[64], fpath[64], passbuf[16];
  char reason[64], id[16], buf[128], ans[4], *str;
  LinkList *now;
  int modes;
/* 1:�C�H���B�]�����P  2:�C�H���B�]���ۦP  3:������H�o�� */
  int money, gold;
  HDR hdr;
  FILE *fp;
  DIR *dire;
  struct dirent *de;
  PAYCHECK paycheck;
  time_t nowtime;
  char *menu[] = {"a�W�[", "d�R��", "s����", "e�ק�", "q����", NULL};
  char *main_menu[] = {"1q", "1�C�H���B�]�����P", "2�C�H���B�]���ۦP", "3������H�o��", "Q���}", NULL};

  if(!strcmp(cuser.userid, str_sysop))
  {
    vmsg("�����sysop�o��");
    return 0;
  }

  /* debug function(check if head is null) */
  modes = 0;		/* modes�ɨӧ@�������� */
  if(mohead)
  {
    vmsg("���~�I��C�����ONULL");
    modes = 1;
  }
  if(motail)
  {
    vmsg("���~�I��C�����ONULL");
    modes = 1;
  }

  if(modes)
    return 0;
  /* End of debug */


  /* �������ҵ{�� */
  if(!vget(b_lines, 0, "��J�K�X�H�~�����G", passbuf, PSWDLEN + 1, NOECHO))
    return 0;

  if(chkpasswd(cuser.passwd, passbuf))
  {
    vmsg(ERR_PASSWD);
    return 0;
  }

  move(0, 0);
  clrtobot();

  /* draw header */
  outs("\033[1;33;45m ��\033[37m �����o���\\�� Ver 1.0(release) \033[m");

  ans[0] = pans(5, 20, "�����o���\\��", main_menu);	/* ��ans[]�ӥ� */
  switch(ans[0])
  {
  case 'q':
    return 0;

  default:
    modes = ans[0]-'0';
  }

  if(modes==2 || modes==3)
  {
    do
    {
      if (!vget(4, 0, "�o���C�H�ȹ�[0]�G", buf, 9, DOECHO))
        strcpy(buf, "0");
      money = atoi(buf);
      if (money < 0) money = 0;

      buf[0] = '\0';

      if (!vget(4, 28, "����[0]�G", buf, 9, DOECHO))
        strcpy(buf, "0");
      gold = atoi(buf);
      if (gold < 0) gold = 0;

      if (money==0 && gold==0) continue;

    }while(setans(4, 55, "�T�{�H(y/N) ")!='y');
  }

  /* ��J�o�����z�� */
  do
  {
    if(!vget(4, 0, "�o���z��(����J�h��������)�G", reason, 40, DOECHO))
      return 0;

    ans[0] = setans(5, 0, "�T�{�C(y/N) ");

    if (ans[0] == 'q')
      return 0;

  }while(ans[0] != 'y');

  move(5, 0);
  clrtoeol();

  /* �o�������� */
  if(modes == 3)
  {
    move(8, 0);
    prints("�{�b�Y�N�o�������ϥΪ� \033[1m%d \033[m�ȹ� �P \033[1m%d \033[m�����A", money, gold);

    if(setans(10, 4, "�нT�{�C(Y/N) [N] ")!='y')
    {
      vmsg("�����ʧ@");
      return 0;
    }

    /* �o���̬����{�� */ 
    time(&nowtime);
    sprintf(buf, "�����o���{������(�� %s �ϥ�)\n�ɶ��G%s\n", cuser.userid, Btime(&nowtime));
    f_cat(FN_RUN_BONUS_LOG, buf);
    sprintf(buf, "��]�G%s\n\n�W��G�o�� �����ϥΪ� �p[%d �ȹ� + %d ����]", reason, money, gold);
    f_cat(FN_RUN_BONUS_LOG, buf);
    f_cat(FN_RUN_BONUS_LOG, "\n-----end of this turn-----\n\n");

    /* �o�䲼�B�H�q���H���B�z�{�� */
    strcpy(folder, "usr/*");
    for(folder[4]='a';folder[4]<='z';folder[4]++)
    {
      dire = opendir(folder);

      if(dire)
      {
        while(de = readdir(dire))
        {
          str = de->d_name;

          /* �ϥΪ̦s�b�Ϊ̤��Osysop�Bguest�����p�U�~�ݭn�e�� */
          if(acct_userno(str)<=0 || !strcmp(str, STR_GUEST) || !strcmp(str, str_sysop))
            continue;

          /* �q���H */
          usr_fpath(fpath, str, fn_dir);
          if (fp = fdopen(hdr_stamp(fpath, 0, &hdr, buf), "w"))
          {
            fprintf(fp, "�@��: sysop (%s)\n", SYSOPNICK);
            fprintf(fp, "���D: ����o���q��\n");
            fprintf(fp, "�ɶ�: %s\n\n", Btime(&nowtime));

            fprintf(fp, "��ѡG%s\n\n", reason);
            fprintf(fp, "�в��r�� Xyz -> Market -> �Ȧ� �N�䲼�I�{�C\n\n"
                        "                                  �����H�ů��� (ps:���H���|�۰ʾP��)\n");
            fclose(fp);

            strcpy(hdr.title, "����o���q��");
            strcpy(hdr.owner, "sysop");
            rec_add(fpath, &hdr, sizeof(HDR));
          }

          /* �o�e�䲼 */
          memset(&paycheck, 0, sizeof(PAYCHECK));
          time(&paycheck.tissue);
          paycheck.money = now -> money;
          paycheck.gold = now -> gold;
          strcpy(paycheck.reason, "[����o��]");
          usr_fpath(fpath, str, FN_PAYCHECK);
          rec_add(fpath, &paycheck, sizeof(PAYCHECK));
        }

        closedir(dire);
      }
    }

    vmsg("�o�e����");
    return 0;
  }



/* ----------------------------------------------------- */
/*							 */
/*  Caution : �U���������b return �e�@�w�n�i�� mo_free() */
/*							 */
/* ----------------------------------------------------- */



  do		/* command line process  */
  {
    move(8, 0);
    clrtobot();
    mo_out(11, 0, "\033[1;33m[�W��-�ȹ�+����]\033[m");

    ans[0] = krans(6, "aq", menu, "sysop>");

    switch(ans[0]){
      case 'd':
	if(!vget(8, 0, "�Q�n�R����ID�G", id, IDLEN + 1, DOECHO))
	  break;

	mo_del(id);
	break;

      case 'e':
	if(modes == 2)
	  break;
	if(!vget(8, 0, "�Q�n�ק諸ID�G", id, IDLEN + 1, DOECHO))
	  break;
	if((now = mo_has(id)) == NULL)
	{
	  move(6, 0);
	  clrtoeol();
	  outs("�䤣��o��ID");
	  break;
	}

	if(modes == 1)
        {
          if(!vget(9, 0, "�o�X�ȹ�[0] ", buf, 9, DOECHO))
            strcpy(buf, "0");
          money = atoi(buf);
          if(money < 0) money = 0;

          if(!vget(9, 21, " +����[0] ", buf, 9, DOECHO))
            strcpy(buf, "0");
          gold = atoi(buf);
          if(gold < 0) gold = 0;

          if(money ==0 && gold ==0)
            break;
        }
	now->money = money;
	now->gold = gold;
	break;

      case 's':
        if (mohead == NULL)
	{
	  mo_free();
	  return 0;
	}else
	  break;

      case 'q':
	mo_free();
	vmsg("�����ʧ@");
	return 0;

      /* case 'a': */
      default:
        if(!vget(8, 0, "ID�G", id, IDLEN + 1, DOECHO))
          break;

        if(acct_userno(id) <= 0 || !strcmp(id, STR_GUEST))
          break;

        if(mo_has(id) != NULL)
          break;

        if(modes == 1)
        {
          if(!vget(9, 0, "�o�X�ȹ�[0] ", buf, 9, DOECHO))
            strcpy(buf, "0");
          money = atoi(buf);
          if(money < 0) money = 0;

          if(!vget(9, 21, " +����[0] ", buf, 9, DOECHO))
            strcpy(buf, "0");
          gold = atoi(buf);
          if(gold < 0) gold = 0;

          if(money ==0 && gold ==0)
            break;
        }

        if(setans((modes == 1)? 9:8, 45, "�T�{�C(Y/n) ") == 'n')
          break;

        mo_add(id, money, gold);

        break;
    }
    /* End of key switch */

  }while(ans[0]!='s');


  time(&nowtime);


  /* �o���̰O���{�� */
  sprintf(buf, "�����o���{������(�� %s �ϥ�)\n�ɶ��G%s\n", cuser.userid, Btime(&nowtime));
  f_cat(FN_RUN_BONUS_LOG, buf);
  sprintf(buf, "��]�G%s\n\n�W��G\n", reason);
  f_cat(FN_RUN_BONUS_LOG, buf);

  /* �o���B�z�j�� */
  for(now = mohead; now ; now = now->next)
  {
    /* �q���H */
    usr_fpath(folder, now->id, fn_dir);
    if (fp = fdopen(hdr_stamp(folder, 0, &hdr, fpath), "w"))
    {
      fprintf(fp, "�@��: sysop (%s)\n", SYSOPNICK);
      fprintf(fp, "���D: ����o���q��\n");
      fprintf(fp, "�ɶ�: %s\n\n", Btime(&nowtime));

      fprintf(fp, "��ѡG%s\n\n", reason);
      fprintf(fp, "�в��r�� Xyz -> Market -> �Ȧ� �N�䲼�I�{�C\n\n"
                  "                                  �����H�ů��� (ps:���H���|�۰ʾP��)\n");

      fclose(fp);

      strcpy(hdr.title, "����o���q��");
      strcpy(hdr.owner, "sysop");
      rec_add(folder, &hdr, sizeof(HDR));
    }

    /* �o�e�䲼 */
    memset(&paycheck, 0, sizeof(PAYCHECK));
    time(&paycheck.tissue);
    paycheck.money = now -> money;
    paycheck.gold = now -> gold;
    strcpy(paycheck.reason, "[����o��]");
    usr_fpath(fpath, now->id, FN_PAYCHECK);
    rec_add(fpath, &paycheck, sizeof(PAYCHECK));

    sprintf(buf, "�o�� %-13s �p [%d �� + %d ��]\n", now->id, now->money, now->gold);
    f_cat(FN_RUN_BONUS_LOG, buf);

  }
  f_cat(FN_RUN_BONUS_LOG, "\n-----end of this turn-----\n\n");


  mo_free();

  vmsg("�o�e����");
  return 0;
}

#undef LinkList			/* ����ѧO�r�ٵ��O�H */

#endif



#ifdef HAVE_TIP

/* ----------------------------------------------------- */
/* �C��p���Z						 */
/* ----------------------------------------------------- */

int
x_tip()
{
  int i, j;
  char msg[128];
  FILE *fp;

  if (!(fp = fopen(FN_ETC_TIP, "r")))
    return XEASY;

  fgets(msg, 128, fp);
  j = atoi(msg);		/* �Ĥ@��O���`�g�� */
  i = time(0) % j + 1;
  j = 0;

  while (j < i)			/* ���� i �� tip */
  {
    fgets(msg, 128, fp);
    if (msg[0] == '#')
      j++;
  }

  move(12, 0);
  clrtobot();
  fgets(msg, 128, fp);
  prints("\033[1;36m�C��p���Z�G\033[m\n");
  prints("            %s", msg);
  fgets(msg, 128, fp);
  prints("            %s", msg);
  vmsg(NULL);
  fclose(fp);
  return 0;
}
#endif	/* HAVE_TIP */


#ifdef HAVE_LOVELETTER 

/* ----------------------------------------------------- */
/* ���Ѳ��;�						 */
/* ----------------------------------------------------- */

int
x_loveletter()
{
  FILE *fp;
  int start_show;	/* 1:�}�l�q */
  int style;		/* 0:�}�Y 1:���� 2:���� */
  int line;
  char buf[128];
  char header[3][5] = {"head", "body", "foot"};	/* �}�Y�B����B���� */
  int num[3];

  /* etc/loveletter �e�q�O#head ���q�O#body ��q�O#foot */
  /* ��ƤW���G#head����  #body�K��  #foot���� */

  if (!(fp = fopen(FN_ETC_LOVELETTER, "r")))
    return XEASY;

  /* �e�T��O���g�� */
  fgets(buf, 128, fp);
  num[0] = atoi(buf + 5);
  num[1] = atoi(buf + 5);
  num[2] = atoi(buf + 5);

  /* �M�w�n��ĴX�g */
  line = time(0);
  num[0] = line % num[0];
  num[1] = (line >> 1) % num[1];
  num[2] = (line >> 2) % num[2];

  vs_bar("���Ѳ��;�");

  start_show = style = line = 0;

  while (fgets(buf, 128, fp))
  {
    if (*buf == '#')
    {
      if (!strncmp(buf + 1, header[style], 4))  /* header[] ���׳��O 5 bytes */
	num[style]--;

      if (num[style] < 0)	/* �w�g fget ��n�諸�o�g�F */
      {
	outc('\n');
	start_show = 1;
	style++;
      }
      else
      {
	start_show = 0;
      }
      continue;
    }

    if (start_show)
    {
      if (line >= (b_lines - 5))	/* �W�L�ù��j�p�F */
	break;

      outs(buf);
      line++;
    }
  }

  fclose(fp);
  vmsg(NULL);

  return 0;
}
#endif	/* HAVE_LOVELETTER */


/* ----------------------------------------------------- */
/* �K�X�ѰO�A���]�K�X					 */
/* ----------------------------------------------------- */


int
x_password()
{
  int i;
  ACCT acct;
  FILE *fp;
  char fpath[80], email[60], passwd[PSWDLEN + 1];
  time_t now;

  vmsg("���L�ϥΪ̧ѰO�K�X�ɡA���e�s�K�X�ܸӨϥΪ̪��H�c");

  if (acct_get(msg_uid, &acct) > 0)
  {
    time(&now);

    if (acct.lastlogin > now - 86400 * 10)
    {
      vmsg("�ӨϥΪ̥����Q�ѥH�W���W����i���e�K�X");
      return 0;
    }

    vget(b_lines - 2, 0, "�п�J�{�Үɪ� Email�G", email, 40, DOECHO);

    if (str_cmp(acct.email, email))
    {
      vmsg("�o���O�ӨϥΪ̻{�ҮɥΪ� Email");
      return 0;
    }

    if (not_addr(email) || !mail_external(email))
    {
      vmsg(err_email);
      return 0;
    }

    vget(b_lines - 1, 0, "�п�J�u��m�W�G", fpath, RNLEN + 1, DOECHO);
    if (strcmp(acct.realname, fpath))
    {
      vmsg("�o���O�ӨϥΪ̪��u��m�W");
      return 0;
    }

    if (vans("��ƥ��T�A�нT�{�O�_���ͷs�K�X(Y/N)�H[N] ") != 'y')
      return 0;

    sprintf(fpath, "%s ��F %s ���K�X", cuser.userid, acct.userid);
    blog("PASSWD", fpath);

    /* �üƲ��� A~Z �զX���K�X�K�X */
    for (i = 0; i < PSWDLEN; i++)
      passwd[i] = rnd(26) + 'A';
    passwd[PSWDLEN] = '\0';

    /* ���s acct_load ���J�@���A�קK���b vans() �ɵn�J�|���~�����ĪG */
    if (acct_load(&acct, acct.userid) >= 0)
    {
      str_ncpy(acct.passwd, genpasswd(passwd), PASSLEN + 1);
      acct_save(&acct);
    }

    sprintf(fpath, "tmp/sendpass.%s", cuser.userid);
    if (fp = fopen(fpath, "w"))
    {
      fprintf(fp, "%s ���z�ӽФF�s�K�X\n\n", cuser.userid);
      fprintf(fp, BBSNAME "ID : %s\n\n", acct.userid);
      fprintf(fp, BBSNAME "�s�K�X : %s\n", passwd);
      fclose(fp);

      bsmtp(fpath, BBSNAME "�s�K�X�q���H", email, MQ_NOATTACH);
    }
  }

  return 0;
}


int
pushdoll_maker()	/* dust.100401: �����榡�ͦ��� */
{
#define PUSH_LEN 50
  char buf[80];
  int i;
  FILE *fp;

  if (!(fp = tbf_open()))
  {
    vmsg("�Ȧs�ɶ}�ҥ���");
    return XEASY;
  }

  clear();
  for(i = 0; i <= b_lines; i++)
  {
    sprintf(buf, "%2d) ", i + 1);
    if(!vget(i, 0, buf, buf + 9, PUSH_LEN, DOECHO))
      break;

    fprintf(fp, "%%%d\n%s\n", !i, buf + 9);
  }
  fclose(fp);

  return 0;
}



/* dust:���ȱM�Ϊ��������ե\�� */
/* xatier: �{�b���O���������FXD */
/* dust.reply: �o�O�b�A�J�ǥH�e�N�s�b�۪��]�p�F... */
int
admin_test()
{
#if 1
  vmsg("���ᰨ����");
  return XEASY;
#else

#endif
}

