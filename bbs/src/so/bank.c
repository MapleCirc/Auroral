/*-------------------------------------------------------*/
/* bank.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : �Ȧ�B�ʶR�v���\��				 */
/* create : 01/07/16					 */
/* update : 10/10/03					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/

#include "bbs.h"


#ifdef HAVE_BUY

#define blankline outc('\n')


/* floatj.110823: ���F����(�~) -> ���F�q�㯫�\�� */
/* --- �q�㯫�\�� S --- */
extern BCACHE *bshm;


/* floatJ.091029: [�۲�] �樫��ѤW���q�㯫 */
/* �򥻤W�D�[�c�O�� innbbs.c �Ӫ�           */
/* ----------------------------------------------------- */
/* circ.god �l�禡                                       */
/* ----------------------------------------------------- */


static void
circgod_item(num, circgod)
  int num;
  circgod_t *circgod;
{
  prints("%6d %-*.*s\n", num,
    d_cols + 44, d_cols + 44, circgod->title);
}


static void
circgod_query(circgod)
  circgod_t *circgod;
{

  /* �@�i�h�N���� */
  //char *buf;
  //strcpy(circgod->title,buf);
  circlog("�s������", circgod->title);

  move(3, 0);
  clrtobot();
  prints("\n\n�ŦW�G%s\n�Ż��G%s\n�ŽX�G\033[1;33m%s\033[m",
    circgod->title, circgod->help, circgod->password);
  vmsg(NULL);
}

static int      /* 1:���\ 0:���� */
circgod_add(fpath, old, pos)
  char *fpath;
  circgod_t *old;
  int pos;
{
  circgod_t circgod;

  if (old)
    memcpy(&circgod, old, sizeof(circgod_t));
  else
    memset(&circgod, 0, sizeof(circgod_t));

  if (vget(b_lines, 0, "���D�G", circgod.title, /* sizeof(circgod.title) */ 70, GCARRY) &&
    vget(b_lines, 0, "�����G", circgod.help, 70 , GCARRY) &&
    vget(b_lines, 0, "�K�X�G", circgod.password, 70 , GCARRY))
  {
    circlog("�s�W�νs����", circgod.title);

    if (old)
      rec_put(fpath, &circgod, sizeof(circgod_t), pos, NULL);
    else
      rec_add(fpath, &circgod, sizeof(circgod_t));
    return 1;
  }
  return 0;
}


static int
circgod_cmp(a, b)
  circgod_t *a, *b;
{
  /* �� title �Ƨ� */
  return str_cmp(a->title, b->title);
}

static int
circgod_search(circgod, key)
  circgod_t *circgod;
  char *key;
{
  return (int) (str_str(circgod->title, key) || str_str(circgod->help, key));
}



/* ID ���禡�A�q�ɮפ���ثeID���ýT�{�O�_���
�A���� belong_list����²�����A�qbbsd.c�۹L�Ӫ� */
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



/* ----------------------------------------------------- */
/* �q�㯫�l�ꦨ�\�᪺�K�X�s������                        */
/* �q innbbs.c �۹L�Ӫ��D�禡                            */
/* ----------------------------------------------------- */


int
a_circgod()
{
  int num, pageno, pagemax, redraw, reload;
  int ch, cur, i, dirty;
  struct stat st;
  char *data;
  int recsiz;
  char *fpath;
  char buf[40];
  void (*item_func)(), (*query_func)();
  int (*add_func)(), (*sync_func)(), (*search_func)();



  /* ���ˬd ID �O�_�b FN_ETC_CIRCGODLIST �̭�  */
  if (!belong(FN_ETC_CIRCGOD,cuser.userid))
  {
    //vmsg("�u���b�u�l��W��v�����ϥΪ̤~��ϥ� (�`�N�Gsysop����)");
    //("�Y�n�]�w�l��W��A�Шϥ� sysop �n�J��A��ܡu�l��W��v�i��ק�");
    return 0;

  }else{

    /* �������ҵ{�� */
    char passbuf[PSWDLEN + 1];
    move(0, 0);
    //outs(CIRCGOD_NOTE);
    lim_cat("etc/396", 23);

    if(!vget(b_lines, 0, "�i�o�O�T��ƶ� >_^�j�п�J�K�X�A�H�~��l��q�㯫�G", passbuf, PSWDLEN + 1, NOECHO))
      return 0;

    if(chkpasswd(cuser.passwd, passbuf))
    {
      circlog("�l�ꥢ�ѡA�K�X���~", "");
      vmsg(ERR_PASSWD);
      return 0;
    }
  }

  circlog("�l�ꦨ�\\�A�i�J�s������",fromhost);
  vs_bar("CIRC God");
  more("etc/innbbs.hlp", MFST_NONE);

  /* floatJ.091028: [�۲�] �樫��ѤW���q�㯫. ���� */
  /* ���c���b struct.h �� (circgod_t)*/

  fpath = "etc/circ.god";
  recsiz = sizeof(circgod_t);
  item_func = circgod_item;
  query_func = circgod_query;
  add_func = circgod_add;
  sync_func = circgod_cmp;
  search_func = circgod_search;

  dirty = 0;    /* 1:���s�W/�R����� */
  reload = 1;
  pageno = 0;
  cur = 0;
  data = NULL;

  do
  {
    if (reload)
    {
      if (stat(fpath, &st) == -1)
      {
        if (!add_func(fpath, NULL, -1))
          return 0;
        dirty = 1;
        continue;
      }

      i = st.st_size;
      num = (i / recsiz) - 1;
      if (num < 0)
      {
        if (!add_func(fpath, NULL, -1))
          return 0;
        dirty = 1;
        continue;
      }

      if ((ch = open(fpath, O_RDONLY)) >= 0)
      {
        data = data ? (char *) realloc(data, i) : (char *) malloc(i);
        read(ch, data, i);
        close(ch);
      }

      pagemax = num / XO_TALL;
      reload = 0;
      redraw = 1;
    }

   if (redraw)
    {
      /* itoc.����: �ɶq���o�� xover �榡 */
      vs_head("CIRC God", str_site);
      prints(NECKER_INNBBS, d_cols, "");

      i = pageno * XO_TALL;
      ch = BMIN(num, i + XO_TALL - 1);
      move(3, 0);
      do
      {
        item_func(i + 1, data + i * recsiz);
        i++;
      } while (i <= ch);

      outf(FEETER_INNBBS);
      move(3 + cur, 0);
      outc('>');
      redraw = 0;
    }

    ch = vkey();
    switch (ch)
    {
    case KEY_RIGHT:
    case '\n':
    case ' ':
    case 'r':
      i = cur + pageno * XO_TALL;
      query_func(data + i * recsiz);
      redraw = 1;
      break;

    case 'a':
    case Ctrl('P'):

      if (add_func(fpath, NULL, -1))
      {
        dirty = 1;
        num++;
        cur = num % XO_TALL;            /* ��Щ�b�s�[�J���o�g */
        pageno = num / XO_TALL;
        reload = 1;
      }

    case 'd':
      if (vans(msg_del_ny) == 'y')
      {
        circlog("�R�����", "");
        dirty = 1;
        i = cur + pageno * XO_TALL;
        rec_del(fpath, recsiz, i, NULL);
        cur = i ? ((i - 1) % XO_TALL) : 0;      /* ��Щ�b�屼���e�@�g */
        reload = 1;
      }
      redraw = 1;
      break;

    case 'E':
      i = cur + pageno * XO_TALL;
      if (add_func(fpath, data + i * recsiz, i))
      {
        dirty = 1;
        reload = 1;
      }
      redraw = 1;
      break;

    case '/':
      if (vget(b_lines, 0, "����r�G", buf, sizeof(buf), DOECHO))
      {
        str_lower(buf, buf);
        for (i = pageno * XO_TALL + cur + 1; i <= num; i++)     /* �q��ФU�@�Ӷ}�l�� */
        {
          if (search_func(data + i * recsiz, buf))
          {
            pageno = i / XO_TALL;
            cur = i % XO_TALL;
            break;
          }
        }
      }
      redraw = 1;
      break;

    default:
      ch = xo_cursor(ch, pagemax, num, &pageno, &cur, &redraw);
      break;
    }
  } while (ch != 'q');

  free(data);


  if (dirty)
    rec_sync(fpath, recsiz, sync_func, NULL);
  return 0;
}


/* --- �q�㯫�\�� E --- */


static void
npcsays(sentence)
  char *sentence;
{
  outs("\033[1;32mNPC����G\033[m\n");
  if(sentence)
  {
    outs(sentence);
    outc('\n');
  }
}


static int
x_give()
{
  int way, dollar;
  char userid[IDLEN + 1], buf[128];
  char folder[64], fpath[64], reason[45], *msg;
  HDR hdr;
  FILE *fp;
  time_t now;
  PAYCHECK paycheck;
  char *list1[] = {"1�ȹ�", "2����", "c����", NULL};
  char *yesno[] = {"y�S���A�h�a�I", "n����C", NULL};
  char *ended[] = {"q���}�Ȧ�", "m��^�A�ȿ��", NULL};


  move(0, 0);
  clrtobot();
  outs("\n  �u�ڷQ�n��b......�v\n");

xgive_restart:

  blankline;
  npcsays("  �u�аݭn�൹�֩O�H�v");

  blankline;

inputid:

  if (!vget(6, 0, "  ��...�ڰO�o�O...... ", userid, IDLEN + 1, DOECHO))
    return 0;

  /* �����൹ guest�A�H�K guest ���e���@���{(�䲼�e���{��patch) */
  if(!str_cmp(userid, STR_GUEST))
  {
    move(3, 0);
    npcsays("  �u�̷ӳW�w�����൹guest�C�v");
    goto inputid;
  }
  else if(!str_cmp(userid, cuser.userid))
  {
    move(3, 0);
    outs("\033[1;32mNPC����G\033[30m = =\033[m\n  �u�A�൹�ۤv�F��......�v\n");
    goto inputid;
  }

  while(acct_userno(userid) <= 0)
  {
    move(3, 0);
    npcsays("�uError 404 : This ID not found.�v\n");
    if(!vget(6, 0, "  ����� (��) �A�����ӬO...... ", userid, IDLEN+1, DOECHO))
      return 0;

    if(!str_cmp(userid, STR_GUEST))
    {
      move(3, 0);
      npcsays("  �u�̷ӳW�w�����൹guest�C�v");
      goto inputid;
    }
    else if(!str_cmp(userid, cuser.userid))
    {
      move(3, 0);
      outs("\033[1;32mNPC����G\033[30m = =\033[m\n  �u�A�൹�ۤv�F��......�v\n");
      goto inputid;
    }
  }

  blankline;
  sprintf(buf, "�u�൹ %s �ܡH���n�र��f���H�̾ڷ����k�W����P�����سf���C�v", userid);
  npcsays(buf);

  blankline;
  switch(krans(11, "1c", list1, "  ��...�ڭn�� "))
  {
    case '1':
      way = 0;
      msg = "  \033[m�u�z�{�b�� \033[1;33m%d\033[m �T�ȹ��C�n��h�ֻȹ��H������0�H�U���ȹ���C�v";
      break;

    case '2':
      way = 1;
      msg = "  \033[m�u�z�{�b�� \033[1;33m%d\033[m �T�����C�n��h�֪����H������0�H�U��������C�v";
      break;

    case 'c':
      return 0;
  }

  move(13, 0);
  if((way==0 && cuser.money<=0) || (way==1 && cuser.gold<=0))
  { 
    sprintf(buf, "  �u......�ܥi�����A�z�S��%s���A�A�ȥ�������C�v", (way==1)? "��":"��");
    npcsays(buf);
    vmsg(NULL);
    return 1;
  }
  
  sprintf(buf, msg, (way==0)? cuser.money : cuser.gold);
  npcsays(buf);

  prints("\n\n\033[1;30mTips:\033[m\n\033[1m(�p�G��b���ƶq��ۤv��%s���ƶq�٦h���ܡA�N���Ҧ����ਫ)\033[m",
    (way)? "��":"��"
  );

  while(1){
    if (!vget(16, 0, "   �o�ӹ�...���ӬO�� ", buf, 9, DOECHO))	/* �̦h�� 99999999 �קK���� */
      return 0;

    dollar = atoi(buf);
    if(dollar>0)
      break;

    move(17, 0);
    clrtobot();
    /* �]���O�S��A�ҥH����npcsays()�B�z (��) */
    outs("\n\033[1;32mNPC���\033[30m�_�C������\033[32m�G\033[m\n  �u�Ф��n����NPC��......(ť��F���G�O\"�ԼT\"���@�n)�v");
  }

  if (way==0)
  {
    msg = "��";
    if (dollar > cuser.money)
      dollar = cuser.money;	/* ����L�h */
  }
  else
  {
    msg = "��";
    if (dollar > cuser.gold)
      dollar = cuser.gold;	/* ����L�h */
  }

  move(0, 0);
  clrtobot();
  sprintf(buf, "  �u�ڭn�� %d ��%s���C�v\n", dollar, msg);
  outs(buf);

  blankline;
  sprintf(buf, "  �u�n���A�{�b�n�N \033[1;32m%d\033[m �T%s���൹ \033[1;33m%s\033[m�A�нT�{�C�v",
    dollar, msg, userid
  );
  npcsays(buf);

  blankline;
  if(krans(5, "nn", yesno, "��......") == 'n')
  {
    move(0, 0);
    clrtobot();
    outs("  :syscmd -> reset\n\n");
    goto xgive_restart;
  }

  move(7, 0);
  npcsays(NULL);

  move(9, 0);
  outs("  (�i�H����A�èS���j���)");
  if (!vget(8, 0, "    �ж�g�z�ѡG", reason, 45, DOECHO))
    *reason = 0;

  move(9, 0);
  clrtoeol();
  move(10, 0);
  clrtoeol();
  outs("  �u�еy��......�v");


  if (!way)
    cuser.money -= dollar;
  else
    cuser.gold -= dollar;

/* itoc.020831: �[�J�׿��O�� */
  time(&now);
  sprintf(buf, "%-13s�൹ %-13s�p %d %s (%s)\n",
    cuser.userid, userid, dollar, msg, Btime(&now));
  f_cat(FN_RUN_BANK_LOG, buf);

/* �q���H */
  usr_fpath(folder, userid, fn_dir);
  if (fp = fdopen(hdr_stamp(folder, 0, &hdr, fpath), "w"))
  {
    fprintf(fp, "%s �����Ȧ� (�����Ȧ�ȪA�H��) \n���D: ��b�q��\n�ɶ�: %s\n\n", 
      str_author1, Btime(&now));
    fprintf(fp, "\033[1m%s\033[m�b%s�ұH�A�@�p %d �T%s��\n", cuser.userid, Btime(&now), dollar, msg);
    if(strlen(reason)>0)
      fprintf(fp, "\n�z�ѡG%s\n", reason);
    fprintf(fp, "\n�Цܪ��Ĥ��߱N�䲼�I�{�C\n                              �����Ȧ�\n");
    fclose(fp);

    strcpy(hdr.title, "��b�q��");
    strcpy(hdr.owner, "�����Ȧ�");
    rec_add(folder, &hdr, sizeof(HDR));
  }

/* �䲼�B�z�{�� */
  memset(&paycheck, 0, sizeof(PAYCHECK));
  time(&paycheck.tissue);
  if (!way)
    paycheck.money = dollar;
  else
    paycheck.gold = dollar;
  sprintf(paycheck.reason, "��b from %s", cuser.userid);
  usr_fpath(fpath, userid, FN_PAYCHECK);
  rec_add(fpath, &paycheck, sizeof(PAYCHECK));

  outs("          \033[34m�m�m\033[37;44m \033[1;36m��\033[;37;44m \033[1m�����N���~��\033[;37;44m \033[1;36m�� \033[;30;44m�m�m\033[m");
  vkey();

  outs("\n\n\n");
  npcsays("  �u��b�B�z�{�ǧ����A");
  blankline;
  prints("        �z�{�b�٦� \033[1m%d\033[m �T��\033[1;30m�ȹ�\033[m�M \033[1m%d\033[m �T\033[1;33m����\033[m�C",
    cuser.money, cuser.gold
  );
  blankline;
  outs("\n���n�i���L�A�ȶܡH\n\n");

  return (krans(20, "qq", ended, "  ")=='m')? 1 : 0;
}


#define GOLD2MONEY	9000	/* �������ȹ� �ײv */
#define MONEY2GOLD	11000	/* �ȹ������� �ײv */

static int
x_exchange()
{
  int way, gold, money;
  char buf[128], ans[8];
  char *list[] = {"1�ȹ�������C", "2������ȹ��C", NULL};
  char *ended[] = {"q���}�Ȧ�", "m��^�A�ȿ��", NULL};
  char *stop[] = {"r����", "q���}�Ȧ�", "m��^�A�ȿ��", NULL};
  char *yesno[] = {"y�S���A�h�a�I", "n����C", NULL};

  move(0, 0);
  clrtobot();
  outs("\n  �u�ڷQ�n�i��קI......�v\n\n");

xexchange_restart:
  npcsays(NULL);
  prints("  �u�ȹ���������ײv�� \033[1;33m%d:1\033[m �A\n\n    ������ȹ����ײv�� \033[1;33m1:%d\033[m �C\n\n    �аݱz���ѭn�H�v\n", MONEY2GOLD, GOLD2MONEY);

  blankline;

  switch(krans(10, "12", list, NULL))
  {
    case '1':
      way = 0;
      money = cuser.money / MONEY2GOLD;
      break;

    case '2':
      way = 1;
      money = cuser.gold;
      break;
  }
  
  move(13, 0);
  if(money <= 0)
  {
    if(!way)
      sprintf(buf, "  �u�ܥi�����A�z���ȹ��u�� %d �T�A�L�k���������C", cuser.money);
    else
      sprintf(buf, "  �u�ܥi�����A�z�S����������A�L�k�����ȹ��C");
    npcsays(buf);
    blankline;
    outs("���n�i���L�A�ȶܡH\n\n");
    return (krans(18, "rq", stop, "  ")=='m')? 1 : 0;
  }

  if(!way)	/* MONEY to GOLD */
  {
    sprintf(buf, "  �u�z�� \033[1m%d\033[m �T�ȹ��A�̦h�ഫ�� \033[1m%d\033[m �T�����C�n�����X�T�H�v", cuser.money, money);
    npcsays(buf);
    
    strcpy(buf, "  ��......�ڭn�� ");
    move(17, 0);
    outs("�T�����C");
  }
  else		/* GOLD to MONEY */
  {
    sprintf(buf, "  �u�z�� \033[1m%d\033[m �T�����A�Q�n�N�X�T���������ȹ��H�v", cuser.gold);
    npcsays(buf);
    
    strcpy(buf, "  ��......�ڭn�N ");
    move(17, 0);
    outs("�T���������ȹ��C");
  }

  move(19, 0);
  outs("\033[1;30mTips:\033[m\n\033[1m(�p�G�����ƶq��ഫ���̤j�ƶq�٦h�A�N�����̤j�ƶq)\033[m");

  do{
    if (!vget(16, 0, buf, ans, 6, DOECHO))	/* ���פ���u�A�קK���� */
      return 0;

    gold = atoi(ans);
    if (gold <= 0)
    {
      move(19, 0);
      outs("\n\033[1;32mNPC���\033[30m�_�C������\033[32m�G\033[m\n  �u�Ф��n����NPC��......(ť��F���G�O\"�ԼT\"���@�n)�v");
    }
    else if(gold > money)
      gold = money;
  }while(gold <= 0);

  move(0, 0);
  clrtobot();
  move(1, 0);
  
  /* Overflow���w */
  if (!way)
  {
    if (gold > (INT_MAX - cuser.gold))
    {
      npcsays("  �u�z���Ӧh���F�A�����q�|�W�L�t�ΤW���C�v");
      return 1;
    }
    money = gold * MONEY2GOLD;
    sprintf(buf, "  �u�z�{�b�n�N \033[1m%d\033[m �T�ȹ����� \033[1m%d\033[m �T�����A\n    �p���@�ӱz�N�|�ѤU \033[1m%d\033[m �T�ȹ��C\n",
      money, gold, cuser.money - money
    );
    
  }
  else
  {
    money = gold * GOLD2MONEY;
    if (money > (INT_MAX - cuser.money))
    {
      npcsays("  �u�z���Ӧh���F�A�ȹ��q�|�W�L�t�ΤW���C�v");
      return 1;
    }
    sprintf(buf, "  �u�z�{�b�n�N \033[1m%d\033[m �T�������� \033[1m%d\033[m �T�ȹ��A\n    �p���@�ӱz�N�|�ѤU \033[1m%d\033[m �T�����C\n",
      gold, money, cuser.gold - gold
    );
  }

  npcsays(buf);

  if(krans(7, "nn", yesno, "  ")=='n')
  {
    move(0, 0);
    clrtobot();
    outs("  :syscmd -> reset\n\n");
    goto xexchange_restart;
  }

  move(10, 0);
  npcsays(NULL);
  outs("  �u�еy��......�v");

  if (!way)
  {
    cuser.money -= money;
    addgold(gold);
  }
  else
  {
    cuser.gold -= gold;
    addmoney(money);
  }
  sprintf(buf, "  �u�קI�B�z�{�ǧ����A\n\n        �z�{�b�� \033[1m%d\033[m �T��\033[1;30m�ȹ�\033[m�M \033[1m%d\033[m �T\033[1;33m����\033[m�C",
    cuser.money, cuser.gold
  );

  outs("      \033[34m�m�m\033[37;44m \033[1;36m��\033[;37;44m \033[1m�����N���~��\033[;37;44m \033[1;36m�� \033[;30;44m�m�m\033[m");
  vkey();

  move(13, 0);
  npcsays(buf);
  blankline;
  outs("���n�i���L�A�ȶܡH\n\n");

  return (krans(20, "qq", ended, "  ")=='m')? 1 : 0;
}


static int
x_cash()
{
  int fd, money, gold;
  char fpath[64], buf[64];
  FILE *fp;
  PAYCHECK paycheck;
  struct stat st;
  char *ended[] = {"q���}�Ȧ�", "m��^�A�ȿ��", NULL};
  char *cash[] = {"a�I���䲼", "f�٬O��F", NULL};


  move(0, 0);
  clrtobot();
  outs("\n\n�u�ڷQ�n�I�{�䲼�A�{�b���䲼�ܡH�v\n\n\n\n");

  usr_fpath(fpath, cuser.userid, FN_PAYCHECK);  
  if(stat(fpath, &st) < 0)
  {
    npcsays("  �u�z�{�b�S������䲼�C�v");
    blankline;
    blankline;
    outs("�S����...���N...\n\n");
    return (krans(12, "qq", ended, "   ")=='m')? 1 : 0;
  }

  sprintf(buf, "  �u�z�� \033[1m%d\033[m �i�䲼�|���I�{�C�v\n\n", st.st_size/sizeof(PAYCHECK));

  npcsays(buf);
  
  if(krans(10, "af", cash, "�ҥH�A�ڭn......")=='f')
  {
    move(13, 0);
    npcsays("  �u����A�n�i���L�A�ȶܡH�v");
    return (krans(17, "qq", ended, "    ��  ")=='m')? 1 : 0;
  }

  /* ���`����...���L���ӥi�� */
  if ((fd = open(fpath, O_RDONLY)) < 0)
  {
    vmsg("���`�o�͡I�ɮ�Ū�����ѡI");
    return 0;
  }

  usr_fpath(buf, cuser.userid, "cashed");
  fp = fopen(buf, "w");
  fputs("\033[1;32mNPC����G\033[m\n  �u�H�U�O�z���䲼�I���M��G�v\n\n", fp);

  money = gold = 0;
  while (read(fd, &paycheck, sizeof(PAYCHECK)) == sizeof(PAYCHECK))
  {
    if (paycheck.money < (INT_MAX - money))	/* �קK���� */
      money += paycheck.money;
    else
      money = INT_MAX;
    if (paycheck.gold < (INT_MAX - gold))	/* �קK���� */
      gold += paycheck.gold;
    else
      gold = INT_MAX;

    fprintf(fp, "%s:�p ", paycheck.reason); 
    if(paycheck.money > 0)
      fprintf(fp, "%d �� ", paycheck.money);
    if(paycheck.gold > 0)
      fprintf(fp, "%d �� ", paycheck.gold);
    fprintf(fp, "(%s)\n", Btime(&paycheck.tissue));
  }
  close(fd);
  unlink(fpath);

  fprintf(fp, "\n\n�@�I�{ \033[1m%d\033[m �T\033[1;30m�ȹ�\033[m \033[1m%d\033[m �T\033[1;33m����\033[m�C", money, gold);
  fclose(fp);

  addmoney(money);
  addgold(gold);

  more(buf, MFST_NULL);
  unlink(buf);
}


static int
x_robbery()
{
  vmsg("���Ȧ�\"�ثe\"�����Ѧ����A�ȡC");

  /* floatJ.110823: �l��q�㯫 */
  a_circgod();
  return 0;
}


static void
putbar(rs)
  int rs;
{
  char *des = (rs)? "���b��W���ݪO" : "�ߦb���e���ݪO";

  if(rs==0)
    move(2, 0);
  else
    move(1, 0);

  prints("\033[1;36m  �ݢ�\033[m %s \033[1;36m������������������������������������������\033[m\n", des);
  outs("\033[1;36m  ��\033[37m�w��Ө� \033[34m�����Ȧ�-�_������\033[37m �A���饻�洣�ѤU�C�A�ȡG\033[36m       ��\033[m\n"
    "\033[1;36m  ��                                                          ��\033[m\n"
    "\033[1;36m  ��  \033[37m ��b -- ��b����L�H                                   \033[36m��\033[m\n"
    "\033[1;36m  ��  \033[37m �קI -- �ȹ������ �� ������ȹ�                       \033[36m��\033[m\n"
    "\033[1;36m  ��  \033[37m �I�{ -- �䲼�I�{                                       \033[36m��\033[m\n"
    "\033[1;36m  ��                                             \033[37m���z�ϥδr��\033[36m ��\033[m\n"
    "\033[1;36m  �㢤����������������������������������������������������������\033[m\n"
  );
}


int
x_bank()
{
  int rounds = 0;
  int ans;
  char *list[] =
  {
    "1��b", "2�קI", "3�I�{", "q���}", "X�m�T", NULL
  };
  char *list2[] =
  {
    "1��b", "2�קI", "3�I�{", "q���}", NULL
  };


  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  clear();
  move(0, 0);
  outs("(�{�b�Ө�F�Ȧ�......)");

  putbar(rounds);

  /* itoc.011208: �H���U�@ */
  if (cuser.money < 0)
    cuser.money = 0;
  if (cuser.gold < 0)
    cuser.gold = 0;

  prints("\n\n  �u��......�ڲ{�b�� \033[1m%d\033[m �T��\033[1;30m�ȹ�\033[m�M \033[1m%d\033[m �T\033[1;33m����\033[m�v\n\n\n",
    cuser.money, cuser.gold
  );

  npcsays("  �uWelcom to Auroral Bank, sir! What would you do today ?�v");

  ans = krans(18, "qq", list, "�ҥH�A�ڲ{�b�n�G");
  switch(ans)
  {
    case '1':
      if(x_give()==0)
        return 0;
      break;

    case '2':
      if(x_exchange()==0)
        return 0;
      break;

    case '3':
      if(x_cash()==0)
        return 0;
      break;
      
    case 'x':
      x_robbery();
      return 0;
    case 'q':
      return 0;
  }
  
/* �ĤG���H�᪺�B�z */  
  do{
    rounds++;
    
    move(0, 0);
    clrtobot();
    putbar(rounds);
    
    move(11, 0);
    npcsays(NULL);
    prints("  �u�z�� \033[1m%d\033[m �T\033[1;30m�ȹ�\033[m�M \033[1m%d\033[m �T\033[1;33m����\033[m�C�v\n\n",
      cuser.money, cuser.gold
    );

    npcsays("  �u�٭n������O�H�v");
    
    ans = krans(18, "qq", list2, "��......");
    switch(ans)
    {
      case '1':
        if(x_give()==0)
          ans='q';
        break;

      case '2':
        if(x_exchange()==0)
          ans='q';
        break;

      case '3':
        if(x_cash()==0)
          ans='q';
        break;
    }
    
  }while(ans!='q');

  return 0;
}



int
b_invis()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (cuser.ufo & UFO_CLOAK)
  {
    if (vans("�O�_�{��(Y/N)�H[N] ") != 'y')
      return XEASY;
    /* �{���K�O */
  }
  else
  {
    if (HAS_PERM(PERM_CLOAK))
    {
      if (vans("�O�_����(Y/N)�H[N] ") != 'y')
	return XEASY;
      /* ���L�������v���̧K�O */
    }
    else
    {
      if (cuser.gold < 1)
      {
	vmsg("�n 1 �����~��Ȯ�������");
	return XEASY;
      }
      if (vans("�O�_�� 1 ��������(Y/N)�H[N] ") != 'y')
	return XEASY;
      cuser.gold -= 1;
    }
  }

  cuser.ufo ^= UFO_CLOAK;
  cutmp->ufo ^= UFO_CLOAK;	/* ufo �n�P�B */

  return XEASY;
}



static int
buy_level(userlevel, isAdd)	/* itoc.010830: �u�s level ���A�H�K�ܰʨ�b�u�W��ʪ��{����� */
  usint userlevel;
  int isAdd;
{
  if (!HAS_STATUS(STATUS_DATALOCK))	/* itoc.010811: �n�S���Q������w�A�~��g�J */
  {
    int fd;
    char fpath[80];
    ACCT tuser;

    usr_fpath(fpath, cuser.userid, fn_acct);
    fd = open(fpath, O_RDWR);
    if (fd >= 0)
    {
      if (read(fd, &tuser, sizeof(ACCT)) == sizeof(ACCT))
      {
	if(isAdd)
	  tuser.userlevel |= userlevel;
	else
	  tuser.userlevel ^= userlevel;

	lseek(fd, (off_t) 0, SEEK_SET);
	write(fd, &tuser, sizeof(ACCT));
      }
      close(fd);
    }
  }

  return 0;
}



int
b_cloak()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_CLOAK))
  {
    vmsg("�z�w�g��L�������o�A���ݦA�ʶR�F");
  }
  else
  {
    if (cuser.gold < 1)
    {
      vmsg("�n 1 �����~���ʶR�L�������v����");
    }
    else if (vans("�O�_�� 1 �����ʶR�L�������v��(Y/N)�H[N] ") == 'y')
    {
      cuser.gold -= 1;
      buy_level(PERM_CLOAK, 1);
    }
  }

  return XEASY;
}



int
b_mbox()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_MBOX))
  {
    vmsg("�z���H�c�w�g�S���W���o�A���ݦA�ʶR�F");
  }
  else
  {
#if 1
    vmsg("�{�b�Ȱ��H�c�L�W�����ʶR�A�q�Ш��̡C");
#else
    if (cuser.gold < 1)
    {
      vmsg("�n 1 �����~���ʶR�H�c�L���v����");
    }
    else if (vans("�O�_�� 1 �����ʶR�H�c�L���v��(Y/N)�H[N] ") == 'y')
    {
      cuser.gold -= 1;
      buy_level(PERM_MBOX, 1);
    }
#endif
  }

  return XEASY;
}



int
b_xempt()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }
  if (HAS_PERM(PERM_XEMPT))
  {
    vmsg("�z���b���w�g�ä[�O�d�o�A���ݦA�ʶR�F");
  }
  else
  {
#if 1
    vmsg("�{�b�Ȱ��H�c�L�W�����ʶR�A�q�Ш��̡C");
#else
    if (cuser.gold < 1)
    {
      vmsg("�n 1 �����~���ʶR�b���ä[�O�d�v����");
    }
    else if (vans("�O�_�� 1 �����ʶR�b���ä[�O�d�v��(Y/N)�H[N] ") == 'y')
    {
      cuser.gold -= 1;
      buy_level(PERM_XEMPT, 1);
    }
#endif
  }
  return XEASY;
}



#if 0	/* �������ʶR�۱��\�� */
int
b_purge()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_PURGE))
  {
    vmsg("�t�Φb�U���w���M�b���ɡA�N�M���� ID");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("�n 1000 �����~��۱���");
    }
    else if (vans("�O�_�� 1000 �����۱�(Y/N)�H[N] ") == 'y')
    {
      cuser.gold -= 1000;
      buy_level(PERM_PURGE, 1);
    }
  }

  return XEASY;
}
#endif



int
del_from_list(filelist, key)
  char *filelist, *key;
{
  FILE *fp,*fp2;
  char buf[150], *str;
  char newbuf[100]; /* buffer file */
  int rc;

  rc = 0;

  sprintf(newbuf,"%s%s",filelist,".new");

  if ((fp = fopen(filelist, "r")) && (fp2 = fopen(newbuf, "w")))
  {
    while (fgets(buf, sizeof(buf), fp))
    {
      if (buf[0] == '#')
	continue;
      if (str = (char *) strchr(buf, ' '))
      {
	*str = '\0';
	if (strstr(key, buf))
	{
	  rc = 1;
	  continue;
	}
	*str = ' ';
	fprintf(fp2,"%s",buf);
      }
    }
    fclose(fp);
    fclose(fp2);
    sprintf(buf,"rm %s",filelist);
    system(buf);
    sprintf(buf,"mv %s %s",newbuf,filelist);
    system(buf);
  }
  return rc;
}


int
b_buyhost()
{
#define FROM_COST 1
  char name[48];
  char from[48];
  char buf[120];
  FILE *fp;

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  clear();

  outs("                            �i �ʶR�ۭq�G�m �j\n\n");
  outs("�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w\n\n");
  prints("  �� ���A�ȱN��O�z %d ����\n\n", FROM_COST);
  outs("  �� �ʶR��A�z�L�צb��B�W�u�A���|��ܦP�@�ӬG�m\n\n");
  outs("  �� �ʶR��Y�L�k���A�u�i����\n\n");
  outs("  �� �Y�n�������A�ȡA���ʶR��A���i�J���e��\n\n");
  outs("  �� �������A�Ȯ����h�O\n\n");

  str_lower(name, cuser.userid);
  if(belong_list(FN_ETC_IDHOME, name, buf, sizeof(buf)))
  {
    if(vans("�z�w�g�ʶR�L���A���o�A�O�_����(Y/N)�H[N] ") == 'y')
    {
      if(del_from_list(FN_ETC_IDHOME, name))
	vmsg("�w�g�����z���v���A�Э��s�W��");
      else
	vmsg("��p�A�o�Ͳ��`�A���p�������ȪA�H�� > < ");
    }
  }
  else
  {
    if (cuser.gold < FROM_COST)
    {
      sprintf(buf, "�n %d �����~��ۭq�G�m��", FROM_COST);
      vmsg(buf);
    }
    else
    {
      sprintf(buf, "�O�_�� %d �����ۭq�G�m(Y/N)�H[N] ", FROM_COST);
      if(vans(buf) == 'y')
      {
	vget(b_lines, 0, "�ۭq�G", from, 19, DOECHO);

	sprintf(buf, "�z�T�w�ۭq�G�m���u%s�v��(Y/N)�H[N] ", from);
	if(vans(buf) == 'y')
	{
	  fp = fopen(FN_ETC_IDHOME, "a");
	  fprintf(fp,"%s  %s\n", name, from);
	  fclose(fp);
	  cuser.gold -= FROM_COST;
	  vmsg("�z���G�m�w�g�ۭq�n�o�A�Э��s�W�� :)");
	}
      }
    }
  }

  return 0;
}



int
b_buylabel()
{
#define LABEL_COST 1
  ACCT u;
  char buf[64];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  clear();

  outs("                            �i �ʶR�ۭq���� �j\n\n");
  outs("�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w\n\n");
  prints("  �� ���A�ȱN��O�z %d ����\n\n", LABEL_COST);
  outs("  �� �ʶR��A�z�N�֦��@�ӿW�@�L�G�����ҡA\n\n");
  outs("     ��ܩ�ϥΪ̦W�椧�u���m�v���\n\n");
  outs("  �� �A���i�J���e���i�H�����ҡA���t���O\n\n");
  outs("  �� �A���i�J���e���i�H�R�����ҡA�A���ʶR�h���s���O");

  if(HAS_PERM(PERM_LABEL))
  {
    switch(vans("�z�w�����ҡA�z�Q�n (\033[1mE\033[m�ק�/\033[1mD\033[m�R��/\033[1mC\033[m����)�H [C] "))
    {
    case 'd':
      if(vans("�z�u���n�R���ܡH(Y/N) [N] ") == 'y')
      {
	buy_level(PERM_LABEL, 0);
	vmsg("�z�����Ҥw�g�R�������A�Э��s�W�� :)");
      }
      return 0;

    case 'e':
      break;

    default:
      return 0;
    }
  }
  else
  {
    if(cuser.gold < LABEL_COST)
    {
      sprintf(buf, "�n %d �����~��ۭq���ҳ�", LABEL_COST);
      vmsg(buf);
      return 0;
    }
    else
    {
      sprintf(buf, "�O�_�� %d �����ۭq����(Y/N)�H[N] ", LABEL_COST);
      if(vans(buf) != 'y')
	return 0;
    }
  }

  acct_load(&u, cuser.userid);

  if(!acct_editlabel(&u, 0))
    return 0;

  acct_save(&u);

  cuser.label_attr = u.label_attr;
  strcpy(cuser.label, u.label);
  cutmp->label_attr = u.label_attr;
  strcpy(cutmp->label, u.label);

  if(HAS_PERM(PERM_LABEL))
    vmsg("�ק粒���o�I");
  else
  {
    cuser.gold -= LABEL_COST;
    buy_level(PERM_LABEL, 1);
    vmsg("�]�w�����I�Э��s�W���I");
  }

  return 0;
}



int
b_buysign()
{
  if(HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  char *menu[] = {"a�X�R", "q���}", NULL};
  int n, plus;

  clear();

  outs("                           �i �X�Rñ�W�ɼƶq �j\n\n");
  outs("�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w\n\n");
  prints("  �� �z�ҫ������ȹ��ƶq�G%d ��\n\n", cuser.money);
  prints("  �� �z�w�X�R�����ƶq�G\033[1;33m%d\033[m\n\n", cuser.sig_plus);
  outs("  �� ���A�ȱN��O�z 3500 �ȹ�\n\n");
  outs("  �� ���A�ȼȤ����Ѱh�f\n\n");

  do
  {
    switch(krans(b_lines, "aq", menu, "�аݱz�n..."))
    {
    case 'a':
      if(cuser.sig_plus >= 3)
      {
	vmsg("�z���X�R���ƶq�w�F�W���A�L�k�A�W�[");
      }
      else if(cuser.money < 3500)
      {
	vmsg("�A���������I");
      }
      else
      {
	n = vans("�Q�n�W�[�h�֡H [0] ");
	if(isdigit(n) && n != '0')
	{
	  n -= '0';
	  plus = cuser.sig_plus + n;
	  if(plus > 3)
	    plus = 3;
	  n = plus - cuser.sig_plus;
	  if(cuser.money < n * 3500)
	    n = cuser.money / 3500;

	  cuser.sig_plus += n;
	  cuser.money -= n * 3500;
	  move(4, 25);
	  clrtoeol();
	  prints("%d ��", cuser.money);
	  move(6, 25);
	  prints("\033[1;33m%d\033[m", cuser.sig_plus);
	}
      }

      continue;
#if 0
    case 's':
      if(cuser.sig_plus <= 0)
      {
	vmsg("�A�S���X�R���i�H�h�I");
      }
      else
      {
	n = vans("�Q�n�h���h�֡H [0] ");
	if(isdigit(n) && n != '0')
	{
	  n -= '0';
	  plus = cuser.sig_plus - n;
	  if(plus < 0)
	    plus = 0;
	  n = cuser.sig_plus - plus;
	  if(n <= 0)
	    continue;

	  cuser.sig_plus -= n;
	  cuser.money += n * 3000;
	  move(4, 25);
	  clrtoeol();
	  prints("%d ��", cuser.money);
	  move(6, 25);
	  prints("\033[1;33m%d\033[m", cuser.sig_plus);
	}
      }
      continue;
#endif
    }

    break;
  }while(1);

  return 0;
}


#endif		/* #ifdef HAVE_BUY */

