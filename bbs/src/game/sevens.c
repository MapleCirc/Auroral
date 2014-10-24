/******************************************************************************/
/*                                                                            */
/*  Name: Sevens �P�C�C��                                                     */
/*  Copyright: TCFSH CIRC 27th                                                */
/*  Author: xatier @tcirc.org                                                 */
/*  Create: 2010/07/31                                                        */
/*  Update:                                                                   */
/*  Description: �����H��(telnet://tcirc.org)�W�Y�N���X���s�C��               */
/*               �� Windows ���x�W���g�X�Ӫ����չ���                          */
/*               [ �����@�̤ɰ��T�������V�����ѵL�ᤧ�@ ]                     */
/*                                                                            */
/******************************************************************************/


/* - include ---------------------------------------------------------------- */

#include "bbs.h"
#ifdef HAVE_GAME

/* - macro ------------------------------------------------------------------ */

#define XX 100              /* empty card */
#define F_COLOR "\033[1;36;40m"
#define E_COLOR "\033[1;32;40m"
#define END_COLOR "\033[m"

/* - pointer arrays --------------------------------------------------------- */

char *NUMBER[] = {"��", "��", "��", "��", "��", "��", "��", "��", "��", "10",
                  "��", "��", "��"};
char *FLOWER[] = {"��", "��", "��", "��"};


char *ASDF[] = {"0123", "0132", "0213", "0231", "0312", "0321", "1023", "1032",
        "1203", "1230", "1302", "1320", "2013", "2031", "2103", "2130",
        "2301", "2310", "3012", "3021", "3102", "3120", "3201", "3210"};
        /*  "0123"�����ƦC�A�X�P���ǥ� */
char *NAME[] = {" you    ", " xatier ", "  zeta  ", "raincole"};
                 /* ���a���W�r(?) */


/* - Structures ------------------------------------------------------------- */
typedef struct {
  int num;          /* �I��    1, 2, 3 ... 13 */
  int flo;          /* ���    1 ��, 2 ��, 3 ��, 4 �� */
} CARD;

typedef struct {
  CARD hand[13];        /* ��P */
  int pass;         /* �w�� */
  char *name;           /* �W�r */
} PLAYER;           /* ���a */

typedef struct {
  int n;            /* ����ĴX�i�P */
  int front;            /* �̫e�����@�i */
  int back;         /* �̫᭱���@�i */
} CUR;              /* ��� */

/* - �����ܼ� --------------------------------------------------------------- */

CARD set[52];           /* ��ƵP */
CARD copy[52], temp;        /* �o�P�Ϊ�  */
CARD table[4][13];      /* ��l�W���P */
int fb[4][2];           /* �e�@�i�P��@�i */
PLAYER user, p1, p2, p3;    /* ���a�M�T�ӹq�� */
CUR cur;            /* ��� */
int light;          /* �}�O */
int i, j;           /* �U�� i j */

/* - ��ƫŧi --------------------------------------------------------------- */
void init (void);           /* ��l�� */
void draw_card (CARD);          /* �e�P */
void sort_card (CARD *);        /* �Ƨ� */
void show_all (void);           /* �ù���X */
int go (PLAYER *);              /* �۰ʥX�P */
int throw_card (PLAYER *);      /* ��ʥX�P */
void new_cur (int n);           /* ��s��� */
int check (PLAYER *);           /* �ˬd�ӧQ */
void cls (void);            /* �M�� */
void win (void);            /* �ӧQ */
void lose (void);           /* ���� */

/* - ��Ʃw�q --------------------------------------------------------------- */
void init (void)
{
  cur.n = p1.pass = p2.pass = p3.pass = user.pass = light = 0;

  user.name = NAME[0];
  p1.name = NAME[1];
  p2.name = NAME[2];
  p3.name = NAME[3];

  int N = 100;

  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 13; j++)
    {
      /* set[] �O��ƵP */
      set[i*13 + j].num = j+1;
      set[i*13 + j].flo = i+1;
      /* copy[] �O�o�P�Ϊ�, �ݷ|�|�~�P */
      copy[i*13 + j] = set[i*13 + j];
      /* table[][] �O�P��W���P */
      table[i][j].num = XX;
      table[i][j].flo = i;
    }
  }

  /* �~�P, �ϥΥ洫�k */
  srand(time(NULL));
  while (N--)
  {
    i = rand() % 52;
    j = rand() % 52;
    temp    = copy[i];
    copy[i] = copy[j];
    copy[j] = temp;
  }

  /* �o�P */
  for (i = 0; i < 13; i++) user.hand[i] = copy[i];
  for (; i < 26; i++)   p1.hand[i%13] = copy[i];
  for (; i < 39; i++)   p2.hand[i%13] = copy[i];
  for (; i < 52; i++)   p3.hand[i%13] = copy[i];


  /* �Ƨ�, ��z��P */
  sort_card (user.hand);
  sort_card (p1.hand);
  sort_card (p2.hand);
  sort_card (p3.hand);

  /* �� 7�I���n���X */
  for (i = 0; i < 13; i++)
  {
    if (user.hand[i].num == 7)
    {
      table[user.hand[i].flo-1][6] = user.hand[i];
      user.hand[i].flo = user.hand[i].num = XX;
    }
    if (p1.hand[i].num == 7)
    {
      table[p1.hand[i].flo-1][6] = p1.hand[i];
      p1.hand[i].flo = p1.hand[i].num = XX;
    }
    if (p2.hand[i].num == 7)
    {
      table[p2.hand[i].flo-1][6] = p2.hand[i];
      p2.hand[i].flo = p2.hand[i].num = XX;
    }
    if (p3.hand[i].num == 7)
    {
      table[p3.hand[i].flo-1][6] = p3.hand[i];
      p3.hand[i].flo = p3.hand[i].num = XX;
    }
  }

  /* 7�I��X�h��e�@�i�P��@�i���O�O 6�I�P 8�I */
  fb[0][0] = fb[1][0] = fb[2][0] = fb[3][0] = 6;
  fb[0][1] = fb[1][1] = fb[2][1] = fb[3][1] = 8;

  cls();

}

void cls (void)
{
  move(0, 0);
  clrtobot();
  move(0, 0);
}


void draw_card (CARD c)
{
  if (c.num == XX)
    prints("�x    �x");
  else
    prints("%s�x%s%s�x" END_COLOR, (c.flo-1)&1 ? F_COLOR : E_COLOR, FLOWER[c.flo-1], NUMBER[c.num-1]);
}


void show_all (void)
{

  char buf[10];

/*************************************************************************
 * �T���B��l�O���F����@�����ӬO���|�o��(Debug �ɱ`�|�J�� XD)�� BUG :
 *  *FLOWER[]�M *NUMBER[] ���� NULL�ɷ|�L�X "(NULL)" �r��
 *
 *************************************************************************/

  if (user.hand[0].num != XX)
    sprintf(buf, "%s%s",
            FLOWER[user.hand[cur.n].flo-1] ? FLOWER[user.hand[cur.n].flo-1] : "  ",                    NUMBER[user.hand[cur.n].num-1] ? NUMBER[user.hand[cur.n].num-1] : "  ");

  else sprintf(buf, "    ");

  cls();

  outs("�ݢ�����������������������������������������������������������������"
       "����������\n");
  /* �٥��}�O */
  for (i = 0; i < 13 && !light; i++)
  {
    outs("��");
    /* - ���a���P ------------------------------------------------------- */
    if (i == cur.n)
    {
      move(cur.n+1, 0);
      outs("\033[1;35;40m��\033[m");            /* �b�Y */
    }
    if (user.hand[i].num != XX)
      prints("%s�u%s%s�t" END_COLOR, ((user.hand[i].flo-1)&1 ? F_COLOR : E_COLOR), FLOWER[user.hand[i].flo-1],   NUMBER[user.hand[i].num-1]);
    else outs("        ");              /* empty card */

    if (p1.hand[i].num != XX) outs("\033[1;34;40m" "�u�����t" END_COLOR); /* �S�}�O�ɬݤ����⪺�P */
    else outs("        ");
    if (p2.hand[i].num != XX) outs("\033[0;31;40m" "�u�����t" END_COLOR);
    else outs("        ");
    if (p3.hand[i].num != XX) outs("\033[1;34;40m" "�u�����t" END_COLOR);
    else outs("        ");

    outs(" �� ");
    /* - �P�઺�P ------------------------------------------------------- */
    draw_card(table[0][i]);
    outs("  ");
    draw_card(table[1][i]);
    outs("  ");
    draw_card(table[2][i]);
    outs("  ");
    draw_card(table[3][i]);

    outs("��\n");
  }
  /* �}�O�F */
  for (i = 0; i < 13 && light; i++)
  {
    outs("��");
    /* - ���a���P ------------------------------------------------------- */
    if (i == cur.n) 
    {
      move(cur.n+1, 0);
      outs("\b\b��");
    }

    draw_card(user.hand[i]);
    draw_card(p1.hand[i]);
    draw_card(p2.hand[i]);
    draw_card(p3.hand[i]);

    outs(" �� ");
    /* - �P�઺�P ------------------------------------------------------- */
    draw_card(table[0][i]);
    outs("  ");
    draw_card(table[1][i]);
    outs("  ");
    draw_card(table[2][i]);
    outs("  ");
    draw_card(table[3][i]);
    outs("��\n");
  }
  while (user.hand[cur.n].num == XX) cur.n++;

  /*********************************************************************************
   * ê�󪩭�, �U������X���o���o�˼g = ="
   * �ק�ɥ������޸��᭱������ delet���ݰ_�ӷ|������
   *********************************************************************************/

  prints("�� %s%s%s%s  �������������������������������������� ��\n",   NAME[0], NAME[1],
         NAME[2], NAME[3]);
  prints("��   %d       %d       %d       %d     [�ѤUPASS����]             "
         "           �� ��\n", 4-user.pass, 4-p1.pass, 4-p2.pass, 4-p3.pass);
  outs("�㢤�������������������������������ޢ�������������������������������"
       "����������\n");
  outs("                 �~�w�w�w�w��     ��   ���仡��:                    "
       "        ��\n");
  outs("  �o�i�P�O ...   �x        �x     ��                                "
       "        ��\n");
  prints("                 �x %s   �x     ��      �� [�W�@�i]  R [���s�}�l] "
         "        ��\n", buf);
  outs("                 �x        �x     ��      �� [�U�@�i]  Q [�����C��] "
       "        ��\n");
  outs("                 �x        �x     ��      �� [ PASS ]               "
       "        ��\n");
  outs("  [�ť���X�P]   ���w�w�w�w��     �㢤������������������������������"
       "����������\n");

  outs("\n");
}


void sort_card (CARD *c)
{
  /* ��w�ƧǪk */
  for (i = 0; i < 13; i++)
  {
    for (j = 0; j < 13-1; j++)
    {   /* ���� */
      if (c[i].flo < c[j].flo)
      {
        temp = c[i];
        c[i] = c[j];
    c[j] = temp;
      }
      else if (c[i].flo == c[j].flo)
      { /*  ���I�� */
        /* �Y�P���s�b�ɨ�Ȭ����� XX(�]�N�O 100)�ҥH�|�Q����̫� */
        if (c[i].num < c[j].num)
        {
          temp = c[i];
          c[i] = c[j];
          c[j] = temp;
        }
      }
    }
  }
}


int go (PLAYER *p)
{
  int over = 0;     /* �X���F�S */

  /* - �W�j CIRC �~�P��� �q���X�P�H�u���z���� ---------------------------- */
  /*                                                                        */
  /* *asdf  : �T�|�U�إX�P�u������("0123"�����ƦC)�ܨ�@�ӥΤ�              */
  /* fb_01_ : �H���q�j�Τp�����W�h                                          */
  /*                                                                        */
  /* ---------------------------------------------------------------------- */
  char *asdf = ASDF[rand()%24];     /* �u���X�P��ⶶ�� */
  int fb_01_ = rand() % 2;      /* �q�j�Τp�}�l�X�P */


  for (i = 0; i < 4 && !over; i++)  /* �̧ǬݦU�ت��O�_���P�X */
  {
    for (j = 0; j < 13; j++)        /* �˵��⤤���P */
    {
      if (p->hand[j].flo == asdf[i]-'0'+1)      /* ��⥿�T */
      {
        if (p->hand[j].num == fb[asdf[i]-'0'][fb_01_])  /* �I�ƥ��T */
    {
      /* ��s��W�P�� */
          table[p->hand[j].flo-1][p->hand[j].num-1].num = p->hand[j].num;
      table[p->hand[j].flo-1][p->hand[j].num-1].flo = p->hand[j].flo;

      /* ��s�e�@����@�i */
      fb[asdf[i]-'0'][fb_01_] = p->hand[j].num + (fb_01_? 1: -1);

      /* ��⤤���P�ᱼ */
      p->hand[j].flo = p->hand[j].num = XX;

      over = 1;                 /* �X���F */
      break;
    }
    if (p->hand[j].num == fb[asdf[i]-'0'][!fb_01_]) /* �I�ƥ��T */
    {
          /* ��s��W�P�� */
          table[p->hand[j].flo-1][p->hand[j].num-1].num = p->hand[j].num;
      table[p->hand[j].flo-1][p->hand[j].num-1].flo = p->hand[j].flo;

      /* ��s�e�@����@�i */
      fb[asdf[i]-'0'][!fb_01_] = p->hand[j].num - (fb_01_? 1: -1);

      /* ��⤤���P�ᱼ */
      p->hand[j].flo = p->hand[j].num = XX;

          over = 1;                 /* �X���F */
      break;
    }
      }
    }
  }
  if (!over) p->pass++;                 /* �S�X�P�� */
  sort_card(p->hand);
  return over;
}


int throw_card (PLAYER *p)
{
  int over = 0;
  for (i = 0; i < 4; i++)
  {
    if (p->hand[cur.n].flo == i+1 && !over)     /* ��⥿�T */
    {
      if (p->hand[cur.n].num == fb[i][0])       /* �I�ƥ��T */
      {
    /* ��s��W�P�� */
        table[p->hand[cur.n].flo-1][p->hand[cur.n].num-1].num = p->hand[cur.n].num;
    table[p->hand[cur.n].flo-1][p->hand[cur.n].num-1].flo = p->hand[cur.n].flo;

    /* ��s�e�@����@�i */
    fb[i][0] = p->hand[cur.n].num - 1;

    /* ��⤤���P�ᱼ */
    p->hand[cur.n].flo = p->hand[cur.n].num = XX;

    over = 1;                   /* �X���F */
    break;
      }
      if (p->hand[cur.n].num == fb[i][1])       /* �I�ƥ��T */
      {
        /* ��s��W�P�� */
        table[p->hand[cur.n].flo-1][p->hand[cur.n].num-1].num = p->hand[cur.n].num;
    table[p->hand[cur.n].flo-1][p->hand[cur.n].num-1].flo = p->hand[cur.n].flo;

    /* ��s�e�@����@�i */
    fb[i][1] = p->hand[cur.n].num + 1;

    /* ��⤤���P�ᱼ */
    p->hand[cur.n].flo = p->hand[cur.n].num = XX;
    over = 1;                   /* �X���F */
    break;
      }
    }
  }
  if (!over) p->pass++;     /* �S�X�P�� */
  sort_card(p->hand);
  return over;
}


void new_cur (int n)
{
  /* ���s�p�� cur.front�P cur.back����m */
  for (i = 0, cur.front = 0; i < 13; i++)
  {
    if (user.hand[i].num == XX) cur.front++;
    else break;
  }

  for (i = 12, cur.back = 12; i >= 0; i--)
  {
    if (user.hand[i].num == XX) cur.back--;
    else break;
  }
  /* n�ȨM�w��Ъ�����: n = 1 �U��, n = 0����, n = -1�W�� */
  if (n > 0)
  {
    do {cur.n++;} while (cur.n < cur.back && user.hand[cur.n].num == XX);
    if (cur.n > cur.back) cur.n = cur.front;
  }
  else if (n == 0)
  {
    while (cur.n < cur.back && user.hand[cur.n].num == XX) cur.n++;
    if (cur.n > cur.back) cur.n = cur.front;
  }
  else if (n < 0)
  {
    do {cur.n--;} while (cur.n > cur.front && user.hand[cur.n].num == XX);
    if (cur.n < cur.front) cur.n = cur.back;
  }

  if (cur.front == cur.back) cur.n = cur.front;
}


int check (PLAYER *p)
{
  int ok;

  if (p->pass >= 4)     /* �ϥΥ|�� pass�H�W�N���o */
  {
    show_all();
    prints("%s have passed 4 times!!\twant to play another game? [y/n]", p->name);

    if (!strcmp(p->name, NAME[0]))
      lose();
    else
      win();

    while (1)
      switch (vkey())
      {
        case 'n': case 'N':
      return 2;
    case 'y': case 'Y':
      return 1;
      }
  }

  for (i = 0, ok = 1; i < 13; i++)
  {
    if (p->hand[i].num != XX)
    {
      ok = 0;               /* ��P�٦��� */
      return 0;
    }
  }

  if (ok)                       /* �X���� */
  {
    show_all();
    prints("%s win the game!!\tDo you want to play another game? [y/n]", p->name);

    if (!strcmp(p->name, NAME[0]))
      win();
    else
      lose();

    while (1)
      switch (vkey())
      {
        case 'n': case 'N':
          return 2;
    case 'y': case 'Y':
          return 1;
      }
  }
}


void win (void)
{
  vmsg("Bonus: �C400");
  cuser.money += 400;
}

void lose (void)
{
  vmsg("Penalty: �C100");
  cuser.money = cuser.money >= 100 ? cuser.money - 100: 0;
}


int main_sevens ()
{

          ;;;;;;;;; ;;;;;;   ;;;;;;;;     ;;;;;;;;
         ;;          ;;     ;;    ;;     ;;
        ;;          ;;     ;;;;;;;;     ;;
       ;;          ;;     ;;;;         ;;
      ;;          ;;     ;;  ;;       ;;
     ;;          ;;     ;;    ;;     ;;
    ;;;;;;;;  ;;;;;;   ;;      ;;   ;;;;;;;;

    /* xatier. �L��Τ����e��XDD */

  game_start:
  init();
  int ok = 0;       /* OK �N�����a���X�P��PASS�F, �Y���a�ŧi"�����o�^�X" */

  for (;;)
  {
    /* �Ƨ�, ��z��P */
    sort_card (user.hand);
    sort_card (p1.hand);
    sort_card (p2.hand);
    sort_card (p3.hand);

    new_cur(0);

    show_all();

    switch (vkey())
    {
      /* -Debug ------------------------------------------------------- */
      case 'A':
        ok = go(&user);
	if (!ok) user.pass--;       /* �S�X�P */
	new_cur(0);
	break;
      case 'S':
	ok = 1;
	break;
      /* -End of Debug ------------------------------------------------ */

      case KEY_UP:
      case 'z': case 'Z':       /* �W�� */
	new_cur(-1);
	break;
      case KEY_DOWN:
      case 'x': case 'X':       /* �U�� */
	new_cur(1);
	break;
      case 'c': case 'C':       /* pass */
	ok = 1;
	user.pass++;
	break;
      case KEY_RIGHT:
      case ' ':             /* �X�P */
	ok = throw_card(&user);
	if (!ok) user.pass--;       /* �S�X�P */
	new_cur(0);
	break;
      case KEY_HOME:
        cur.n = cur.front;
        break;
      case KEY_END:
        cur.n = cur.back;
        break;
      case '\\':
	light = !light;         /* �}�O */
	break;
      case 'q':case 'Q':
	goto game_over;         /* ���} */
      case 'r':case 'R':        /* ���� */
	goto game_start;
    }

    switch (check(&user))
    {
      case 2: goto game_over;
      case 1: goto game_start;
      case 0: break;
    }

    if (ok)
    {
      /* - computer player 1 ------------------------------------------ */
      go(&p1);
      switch (check(&p1))
      {
        case 2: goto game_over;
        case 1: goto game_start;
        case 0: break;
      }
      /* - computer player 2 ------------------------------------------ */
      go(&p2);
      switch (check(&p2))
      {
        case 2: goto game_over;
    case 1: goto game_start;
    case 0: break;
      }
      /* - computer player 3 ------------------------------------------ */
      go(&p3);
      switch (check(&p3))
      {
        case 2: goto game_over;
        case 1: goto game_start;
        case 0: break;
      }
      ok = 0;
    }
  }
  game_over:
  return 0;
}

#endif
