/*-------------------------------------------------------*/
/* talk.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : talk/query routines		 		 */
/* create : 95/03/29				 	 */
/* update : 10/11/25				 	 */
/*-------------------------------------------------------*/


#define	_MODES_C_


#include "bbs.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


extern UCACHE *ushm;



/* ------------------------------------- */
/* �u��ʧ@				 */
/* ------------------------------------- */

char *
bmode(up, simple)
  UTMP *up;
  int simple;
{
  static char modestr[32];
  int mode;
  char *word, *mateid;

  word = ModeTypeTable[mode = up->mode];

  if (simple)
    return word;

#ifdef BMW_COUNT
  if (simple = up->bmw_count)	/* �ɥ� simple */
  {
    sprintf(modestr, "%d �Ӥ��y", simple);
    return modestr;
  }
#endif

#ifdef HAVE_BRDMATE
  /* itoc.020602: �����o���ϥΪ̦b�ݭ��ӪO */
  if (mode == M_READA && HAS_PERM(PERM_SYSOP))
  {
    sprintf(modestr, "�\\:%s", up->reading);
    return modestr;
  }
#endif

  if (mode < M_TALK || mode > M_IDLE)	/* M_TALK(�t) �P M_IDLE(�t) ���� mateid */
    return word;

  mateid = up->mateid;

  if (mode == M_TALK)
  {
    /* itoc.020829: up �b Talk �ɡA�Y up->mateid ���Ϋh�ݤ��� */
    if (!utmp_get(0, mateid))
      mateid = "(��)";
  }
  if (mode == M_IDLE)
  {
    sprintf(modestr, "<%s>", mateid);
    return modestr;
  }
  if (mode == M_QUERY)
    return word;

  sprintf(modestr, "%s:%s", word, mateid);
  return modestr;
}



void
do_query(acct, utmp)
  ACCT *acct;
  UTMP *utmp;
{
  UTMP *up;
  int userno, rich;
  char *userid;
  char *fortune[7] = {"�t��", "���h", "�M�H", "���q", "�p�d", "�I��", "�j�a�D"};
  int amount[6] = {0, 500, 10000, 100000, 1500000, 30000000};
  char fpath[64];
  char *source_host;
  time_t login_time;

  utmp_mode(M_QUERY);
  strcpy(cutmp->mateid, acct->userid);


  userno = acct->userno;
  userid = acct->userid;

  for(rich = 0; rich < 6; rich++)
  {
    if(acct->money + acct->gold * 15000 < amount[rich])
      break;
  }

  prints("[�b��] %-12s [�ʺ�] %-16.16s [�W������] %-6d [�峹�g��] %-6d\n",
    userid, acct->username, acct->numlogins, acct->numposts);


  if(utmp)
    up = utmp;
  else
    up = utmp_find(userno);

  prints("[�{��] %s�q�L�{�� [�ʺA] %-16.16s [�g�٪��p] %-6s [�s�H��Ū] %s\n",
    acct->userlevel & PERM_VALID ? "�w�g" : "�|��",
    (up && can_see(cutmp, up)) ? bmode(up, 1) : "���b���W",
    fortune[rich],
    (m_query(userid) & STATUS_BIFF) ? "��" : "�L");


  if(utmp)
  {
    source_host = up->fromhost;
    login_time = up->login_time;
  }
  else
  {
    source_host = acct->lasthost;
    login_time = acct->lastlogin;
  }

  /* davy.110926: ����IP�P�w */
  if (acct->ufo2 & UFO2_HIDEMYIP) /* �P�_�O�_�}������IP */
    if (HAS_PERM(PERM_IPREADER) || acct->userid == cuser.userid) /* �P�_�O�_���j��\ŪIP�v�� */
      prints("[�ӷ�] \033[1;30m%.46s\033[m (%s)\n", source_host, Btime(&login_time));
    else if (utmp) /* �W���ɶ�/�W���W�� */
      prints("[�W���ɶ�] %s\n", Btime(&login_time));
    else
      prints("[�W���W��] %s\n", Btime(&login_time));
  else
    prints("[�ӷ�] %.46s (%s)\n", source_host, Btime(&login_time));


  usr_fpath(fpath, acct->userid, fn_plans);
  lim_cat(fpath, MAXQUERYLINES);

  vmsg(NULL);
}


void
my_query(userid)
  char *userid;
{
  ACCT acct;

  if (acct_load(&acct, userid) >= 0)
    do_query(&acct, NULL);
  else
    vmsg(err_uid);
}


#ifdef HAVE_ALOHA
static int
chkfrienz(frienz)
  FRIENZ *frienz;
{
  int userno;

  userno = frienz->userno;
  return (userno > 0 && userno == acct_userno(frienz->userid));
}


static int
frienz_cmp(a, b)
  FRIENZ *a, *b;
{
  return a->userno - b->userno;
}


void
frienz_sync(fpath)
  char *fpath;
{
  rec_sync(fpath, sizeof(FRIENZ), frienz_cmp, chkfrienz);
}


void
aloha()
{
  UTMP *up;
  int fd;
  char fpath[64];
  BMW bmw;
  FRIENZ *frienz;
  int userno;

  usr_fpath(fpath, cuser.userid, FN_FRIENZ);

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    bmw.caller = cutmp - ushm->uslot;
    /* bmw.sender = cuser.userno; */	/* bmw.sender �̹��i�H call �ڡA�ݷ|�A�M�w */
    strcpy(bmw.userid, cuser.userid);
    strcpy(bmw.msg, "�� ����i"BBSNAME"���� [�W���q��] ��");

    mgets(-1);
    while (frienz = mread(fd, sizeof(FRIENZ)))
    {
      userno = frienz->userno;
      up = utmp_find(userno);

      if (up && (up->ufo & UFO_ALOHA) && !(up->status & STATUS_REJECT) && can_see(up, cutmp))	/* ���ݤ����ڤ��q�� */
      {
	/* �n�ͥB�ۤv�S���������ۤ~�i�H reply */
	bmw.sender = (is_mygood(userno) && !(cuser.ufo & UFO_QUIET)) ? cuser.userno : 0;
	bmw.recver = userno;
	bmw_send(up, &bmw);
      }
    }
    close(fd);
  }
}
#endif


#ifdef LOGIN_NOTIFY
extern LinkList *ll_head;


int
t_loginNotify()
{
  LinkList *wp;
  BENZ benz;
  char fpath[64];

  /* �]�w list ���W�� */

  vs_bar("�t�Ψ�M����");

  ll_new();

  if (pal_list(0))
  {
    wp = ll_head;
    benz.userno = cuser.userno;
    strcpy(benz.userid, cuser.userid);

    do
    {
      if (strcmp(cuser.userid, wp->data))	/* ���i��M�ۤv */
      {
	usr_fpath(fpath, wp->data, FN_BENZ);
	rec_add(fpath, &benz, sizeof(BENZ));
      }
    } while (wp = wp->next);

    vmsg("��M�]�w�����A���W���ɨt�η|�q���z");
  }
  return 0;
}


void
loginNotify()
{
  UTMP *up;
  int fd;
  char fpath[64];
  BMW bmw;
  BENZ *benz;
  int userno;
  int row, col;		/* �p��L��� */

  usr_fpath(fpath, cuser.userid, FN_BENZ);

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    vs_bar("�t�Ψ�M����");

    bmw.caller = cutmp;
    /* bmw.sender = cuser.userno; */	/* bmw.sender �̹��i�H call �ڡA�ݷ|�A�M�w */
    strcpy(bmw.userid, cuser.userid);
    strcpy(bmw.msg, "�� ����i"BBSNAME"���� [�t�Ψ�M] ��");

    row = 1;
    col = 0;
    mgets(-1);
    while (benz = mread(fd, sizeof(BENZ)))
    {
      /* �L�X���ǤH�b��� */
      if (row < b_lines)
      {
	move(row, col);
	outs(benz->userid);
	col += IDLEN + 1;
	if (col > b_cols + 1 - IDLEN - 1)	/* �`�@�i�H�� (b_cols + 1) / (IDLEN + 1) �� */
	{
	  row++;
	  col = 0;
	}
      }

      userno = benz->userno;
      up = utmp_find(userno);

      if (up && !(up->status & STATUS_REJECT) && can_see(up, cutmp))	/* ���ݤ����ڤ��q�� */
      {
	/* �n�ͥB�ۤv�S���������ۤ~�i�H reply */
	bmw.sender = (is_mygood(userno) && !(cuser.ufo & UFO_QUIET)) ? cuser.userno : 0;
	bmw.recver = userno;
	bmw_send(up, &bmw);
	outc('*');	/* Thor.980707: ���q���쪺���Ҥ��P */
      }
    }
    close(fd);
    unlink(fpath);
    vmsg("�o�ǨϥΪ̳]�z���W����M�A�� * ��ܥثe�b���W");
  }
}
#endif



/* ----------------------------------------------------- */
/* talk sub-routines					 */
/* ----------------------------------------------------- */

#define TALK_EXIT_KEY Ctrl('C')
#define TSL_MAXAVAILINE 11

/* TALKCMD_* �����p��s */
#define TALKCMD_BWBOARD -11
#define TALKCMD_BELL -12
#define TALKCMD_BWLOCK -13

/* TSLCMD_* �����p��s */
#define TSLCMD_DRAWBOARD -1

/* TDBCMD_* �����p��s */
#define TDBCMD_SLBACK -1
#define TDBCMD_DRAW -2
#define TDBCMD_CLEAR -3


/* ------------------------------------- */
/* part of Draw Board Talk		 */
/* ------------------------------------- */

static void
tdb_foot(biff, Alice_return, Bob_return)
{
  move(b_lines, 0);

  outs(COLOR1 " ø�ϼҦ� " COLOR2 " ");

  if(Alice_return && Bob_return)
  {	/* �ǳƪ�^�F�ҥH����]���� */
    int x; getyx(NULL, &x);
    move(b_lines, x + 16);
  }
  else if(Alice_return)
    outs("(^B) ������^   ");
  else if(Bob_return)
    outs("(^B) �P�N��^   ");
  else
    outs("(^B)��^��ͼҦ�");

  prints(" (^G)�e�X�_�� (^T)%s�I�s�� (^Q)��ܻ��� (^C)���� \033[m\n", (cuser.ufo & UFO_PAGER)? "�}��" : "����");
}


static int	/* 1: exit talk  0: go back */
talk_drawboard(sock, itsuid, itsnick, lock_board)
  int sock;
  char *itsuid;
  char *itsnick;
  int *lock_board;
{
  char textline[11][80];
  char matid[IDLEN + 1];
  int linept[11][2];	//[begin, end)
  int redraw_area[2];	//[begin, end)
  int tab_counter, row, col, redraw_row, input_chain;
  screen_backup_t old_screen;
  time_t bell_time;
  int Alice_return;	//�ڤ�w�o�X��^�n�D
  int Bob_return;	//���w�o�X��^�n�D


  int ch, i;
  char *p, len;

  const int talkcmd_bwlock = TALKCMD_BWLOCK;
  const int talkcmd_bell = TALKCMD_BELL;
  const int tdbcmd_slback = TDBCMD_SLBACK;
  const int tdbcmd_draw = TDBCMD_DRAW;
  const int tdbcmd_clear = TDBCMD_CLEAR;

#ifdef EVERY_BIFF
  int biff = HAS_STATUS(STATUS_BIFF);
  time_t next_check = time(NULL) + 10;
#else
  int biff = 0;
#endif


  memset(textline, ' ', sizeof(textline));
  memset(linept, 0, sizeof(linept));
  tab_counter = 0;
  row = 0;
  col = 0;
  redraw_row = -1;
  redraw_area[0] = redraw_area[1] = 0;
  input_chain = 0;
  bell_time = 0;
  Alice_return = 0;
  Bob_return = 0;


  clear();

  move(11, 0);
  prints("\033[1;46m �ͤѻ��a \033[45m %s (%s)%*s", itsuid, itsnick, 45 - strlen(itsuid) - strlen(itsnick), "");
  prints("(^X)����r�����G%s \033[m", (cuser.ufo & UFO_ZHC)? "�}" : "��");

  tdb_foot(biff, Alice_return, Bob_return);

  do
  {
    /* return to serial line mode */
    if(Alice_return && Bob_return)
      return 0;

#ifdef EVERY_BIFF
    if(time(NULL) >= next_check)
    {
      if(biff != HAS_STATUS(STATUS_BIFF))
      {
	biff = HAS_STATUS(STATUS_BIFF);
	tdb_foot();
      }
      next_check = time(NULL) + 10;
    }
#endif


    if(!(input_chain && num_in_buf() > 0) && tab_counter <= 0 && redraw_row >= 0)
    {
      move(redraw_row, redraw_area[0]);

      for(i = redraw_area[0]; i < redraw_area[1]; i++)
	outc(textline[redraw_row][i]);

      if(send(sock, &tdbcmd_draw, sizeof(int), 0) != sizeof(int))
	return 1;
      if(send(sock, &redraw_row, sizeof(short), 0) != sizeof(short))
	return 1;
      if(send(sock, &redraw_area[0], sizeof(short), 0) != sizeof(short))
	return 1;
      len = redraw_area[1] - redraw_area[0];
      if(send(sock, &len, sizeof(char), 0) != sizeof(char))
	return 1;
      if(send(sock, &textline[redraw_row][redraw_area[0]], len, 0) != len)
	return 1;

      redraw_row = -1;
    }


    if(tab_counter-- > 0)
      ch = ' ';
    else
    {
      move(row, col);
      ch = vkey();
    }

    input_chain = 0;


    if(ch == I_OTHERDATA)
    {
      int cmd_type;
      short x, y;
      char buf[80];

      if(recv(sock, &cmd_type, sizeof(int), 0) != sizeof(int))
	return 1;

      switch(cmd_type)
      {
      case TALKCMD_BELL:
	bell();
	break;

      case TALKCMD_BWLOCK:
	(*lock_board) = !(*lock_board);
	break;

      case TDBCMD_SLBACK:
	Bob_return = !Bob_return;
	tdb_foot(biff, Alice_return, Bob_return);
	break;

      case TDBCMD_CLEAR:
	clrregion(12, 22);
	break;

      case TDBCMD_DRAW:
	if(recv(sock, &y, sizeof(short), 0) != sizeof(short))
	  return 1;
	if(recv(sock, &x, sizeof(short), 0) != sizeof(short))
	  return 1;
	if(recv(sock, &len, sizeof(char), 0) != sizeof(char))
	  return 1;
	if(recv(sock, buf, len, 0) != len)
	  return 1;

	buf[len] = '\0';
	move(y + 12, x);
	outs(buf);
	break;

      default:
	sprintf(buf, "���~�G�������R�O���� (%d)", cmd_type);
	vmsg(buf);
      }
    }
    else if(ch == ' ')
    {
process_space:
      if(col >= linept[row][1])	//append
      {
	if(col >= 79)
	  continue;
      }
      else if(col <= linept[row][0])	//insert at front
      {
	if(linept[row][1] >= 79)
	  continue;

	p = &textline[row][linept[row][0]];
	memmove(p + 1, p, linept[row][1] - linept[row][0]);
	textline[row][linept[row][0]] = ' ';

	if(redraw_row < 0)
	{
	  redraw_row = row;
	  redraw_area[0] = linept[row][0];
	}
	linept[row][0]++;
	linept[row][1]++;
	redraw_area[1] = linept[row][1];
      }
      else
	goto insert_in_middle;

      col++;
      input_chain = 1;
    }
    else if(isprint2(ch))
    {
      if(linept[row][0] >= linept[row][1])	//empty
      {
	if(col >= 79)
	  continue;

	linept[row][0] = col;
	linept[row][1] = col + 1;
      }
      else if(col >= linept[row][1])	//append
      {
	if(col >= 79)
	  continue;

	linept[row][1] = col + 1;
      }
      else if(col <= linept[row][0])	//insert at front
      {
	if(linept[row][1] >= 79)
	  continue;

	p = &textline[row][linept[row][0]];
	memmove(p + 1, p, linept[row][1] - linept[row][0]);

	linept[row][0] = col;
	linept[row][1]++;
      }
      else
      {
insert_in_middle:
	if(linept[row][1] >= 79)
	  continue;

	memmove(&textline[row][col+1], &textline[row][col], linept[row][1] - col);

	linept[row][1]++;
      }

      textline[row][col] = ch;
      if(redraw_row < 0 || col < redraw_area[0])
      {
	redraw_row = row;
	redraw_area[0] = col;
      }
      redraw_area[1] = linept[row][1];
      col++;

      input_chain = 1;
    }
    else if(ch == '\n')
    {
      if(row < 10)
      {
	col = linept[row][0];
	row++;
      }
    }
    else if(ch == KEY_UP)
    {
      if(row > 0)
	row--;
    }
    else if(ch == KEY_DOWN)
    {
      if(row < 10)
	row++;
    }
    else if(ch == KEY_PGUP)
    {
      row -= 6;
      if(row < 0)
	row = 0;
    }
    else if(ch == KEY_PGDN)
    {
      row += 6;
      if(row > 10)
	row = 10;
    }
    else if(ch == KEY_LEFT)
    {
      if(col > 0)
      {
	col--;
	if((cuser.ufo & UFO_ZHC) && IS_BIG5_LOW(textline[row], col))
	  col--;
      }
    }
    else if(ch == KEY_RIGHT)
    {
      if(col < 79)
      {
	col++;
	if((cuser.ufo & UFO_ZHC) && IS_BIG5_LOW(textline[row], col))
	  col++;
      }
    }
    else if(ch == Ctrl('S'))
    {
      col -= 8;
      if(col < 0)
	col = 0;
    }
    else if(ch == Ctrl('F'))
    {
      col += 8;
      if(col > 79)
	col = 79;
    }
    else if(ch == KEY_TAB)
    {
      tab_counter = 3;
      ch = ' ';
      goto process_space;
    }
    else if(ch == KEY_HOME || ch == Ctrl('A'))
    {
      if(col <= linept[row][0])
	col = 0;
      else
	col = linept[row][0];
    }
    else if(ch == KEY_END || ch == Ctrl('E'))
    {
      if(col >= linept[row][1])
	col = 79;
      else
	col = linept[row][1];
    }
    else if(ch == KEY_DEL || ch == Ctrl('D'))
    {
      int start_point;

      if(col >= linept[row][1])
	continue;

      i = ((cuser.ufo & UFO_ZHC) && IS_BIG5_LOW(textline[row], col + 1))? 2 : 1;
delete_char:
      p = textline[row] + col;
      memcpy(p, p + i, linept[row][1] - col - i);

      if(linept[row][0] - i >= col)
	linept[row][0] -= i;

      start_point = (col >= linept[row][0])? col : linept[row][0];
      if(redraw_row < 0)
      {
	redraw_row = row;
	redraw_area[0] = start_point;
	redraw_area[1] = linept[row][1];
      }
      else
      {
	if(start_point < redraw_area[0]) redraw_area[0] = start_point;
	if(linept[row][1] > redraw_area[1]) redraw_area[1] = linept[row][1];
      }

      linept[row][1] -= i;
      memset(&textline[row][linept[row][1]], ' ', i);
      for(i = linept[row][1]; i >= linept[row][0]; i--)
      {
	if(textline[row][i] != ' ')
	  break;
      }

      linept[row][1] = i + 1;

      if(linept[row][1] <= linept[row][0])
	linept[row][1] = linept[row][0] = 0;

      input_chain = 1;
    }
    else if(ch == KEY_BKSP)
    {
      if(col <= 0)
	continue;

      i = ((cuser.ufo & UFO_ZHC) && IS_BIG5_LOW(textline[row], col - 1))? 2 : 1;
      col -= i;
      if(col <= linept[row][1] - i)
	goto delete_char;
    }
    else if(ch == Ctrl('K'))
    {
      if(col < linept[row][1])
      {
	memset(&textline[row][col], ' ', linept[row][1] - col);
	for(i = col; i >= linept[row][0]; i--)
	{
	  if(textline[row][i] != ' ')
	    break;
	}
	linept[row][1] = i + 1;
	if(linept[row][1] <= linept[row][0])
	  linept[row][1] = linept[row][0] = 0;

	if(redraw_row < 0 || col < redraw_area[0])
	  redraw_area[0] = col;
	redraw_area[1] = 79;
	redraw_row = row;
      }
    }
    else if(ch == Ctrl('Y'))
    {
      memset(textline[row], ' ', sizeof(textline[0]));
      redraw_row = row;
      redraw_area[0] = 0;
      redraw_area[1] = 79;
      linept[row][0] = 0;
      linept[row][1] = 0;
    }
    else if(ch == Ctrl('G'))
    {
      if(time(0) > bell_time)
      {
	if(send(sock, &talkcmd_bell, sizeof(int), 0) != sizeof(int))
	  return 1;

	bell_time = time(0);
      }
    }
    else if(ch == Ctrl('W'))
    {
      if(send(sock, &tdbcmd_clear, sizeof(int), 0) != sizeof(int))
	return 1;

      clrregion(0, 10);
      memset(textline, ' ', sizeof(textline));
      memset(linept, 0, sizeof(linept));

      redraw_row = -1;
    }
    else if(ch == Ctrl('Q'))
    {
      if(send(sock, &talkcmd_bwlock, sizeof(int), 0) != sizeof(int))
	break;
      add_io(0, 60);
      scr_dump(&old_screen);

      more("etc/help/talk_db.hlp", MFST_NORMAL);

      scr_restore(&old_screen);
      add_io(sock, 60);
      if(send(sock, &talkcmd_bwlock, sizeof(int), 0) != sizeof(int))
	break;
    }
    else if(ch == Ctrl('T'))
    {
      if (cuser.userlevel)	/* guest ���i��Q�����ܽ� Talk */
      {
	cuser.ufo ^= UFO_PAGER;
	cutmp->ufo = cuser.ufo;

	tdb_foot(biff, Alice_return, Bob_return);
      }
    }
    else if(ch == Ctrl('B'))
    {
      if(send(sock, &tdbcmd_slback, sizeof(int), 0) != sizeof(int))
	return 1;

      Alice_return = !Alice_return;
      tdb_foot(biff, Alice_return, Bob_return);
    }
    else if(ch == Ctrl('X'))
    {
      cuser.ufo ^= UFO_ZHC;
      move(11, 75);
      prints("\033[1;45m%s\033[m", (cuser.ufo & UFO_ZHC)? "�}" : "��");
    }
#ifdef EVERY_Z
    else if(ch == Ctrl('Z'))
    {
      if(send(sock, &talkcmd_bwlock, sizeof(int), 0) != sizeof(int))
	break;
      strcpy(matid, cutmp->mateid);
      vio_save();
      scr_dump(&old_screen);

      every_Z(0);

      scr_restore(&old_screen);
      vio_restore();
      strcpy(cutmp->mateid, matid);
      if(send(sock, &talkcmd_bwlock, sizeof(int), 0) != sizeof(int))
	break;
    }
#endif

  }while(ch != TALK_EXIT_KEY);

  return 1;
}



/* ------------------------------------- */
/* part of Serial Line Talk		 */
/* ------------------------------------- */

static void
talk_save()
{
  char fpath[64];
  char *menu[] = {"m�Ƨѿ�", "c�M��", NULL};

  usr_fpath(fpath, cuser.userid, FN_TALK_LOG);

#ifdef LOG_TALK
  if (!(cuser.ufo & UFO_NTLOG) && krans(b_lines, "mm", menu, "������Ѭ����B�z ") != 'c')
    mail_self(fpath, cuser.userid, "[�� �� ��] ��Ѭ���", MAIL_READ | MAIL_NOREPLY);
#endif

  unlink(fpath);
}


static void
draw_seperator(availines, paste_mode)
  int availines;
  int paste_mode;
{
  move(b_lines - availines - 1, 0);
  outs("�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w \033[1m");
  prints("\033[3%cm(^P)�K�W�Ҧ� \033[3%cm(^X)����r����\033m\n", paste_mode? '7' : '0', (cuser.ufo & UFO_ZHC)? '7' : '0');
}


static void
draw_dialog(biff, availines, textline, paste_mode)
  int biff;
  int availines;
  char **textline;
  int paste_mode;
{
  int i;
  char indent[IDLEN + 1];

  if(availines > 1)
    sprintf(indent, "%*s", strlen(cuser.userid), "");

  draw_seperator(availines, paste_mode);

  for(i = 0; i < availines; i++)
    prints("%s: %s\n", (i>0)? indent : cuser.userid, textline[i]);
}


static void
tsl_foot(biff, A_ms, B_ms)
  int biff;
  int A_ms, B_ms;
{
  move(b_lines, 0);
#ifdef EVERY_BIFF
  if(biff)
    outs(COLOR1 " �l�t�ӤF ");
  else
#endif
    outs(COLOR1 " ��ͼҦ� ");

  outs(COLOR2 " (PageUp)�^�U�T�� ");

  if(A_ms && B_ms)
  {	/* �ǳƶi�Jø�ϪO�F�ҥH����]���� */
    int x; getyx(NULL, &x);
    move(b_lines, x + 14);
  }
  else if(A_ms)
    outs("(^B) �����ܽ� ");
  else if(B_ms)
    outs("(^B) �����ܽ� ");
  else
    outs("(^B)ø�ϪO�ܽ�");

  outs(" (^G)�e�X�_�� (^Q)��ܻ��� (^C)���� \033[m\n");
}


static void
chat_printline(color, prefix, msg, pCurln, availines)
  uschar *color;
  char *prefix;	/* �Ymsg��NULL�Aprefix�]�i�H��NULL */
  char *msg;
  int *pCurln;
  int availines;
{
  if(*pCurln >= b_lines - availines - 2)
  {
    region_scroll_up(2, b_lines - availines - 2);
    (*pCurln)--;
  }

  if(msg)
  {
    move(*pCurln, 0);
    attrset(color);
    outs(prefix);
    outs(msg);
    attrset(7);

    (*pCurln)++;
  }
}


static void
talk_speak(sock, itsuid, itsnick)
  int sock;
  char *itsuid;
  char *itsnick;
{
  char buf[80], matid[IDLEN + 1], rec_path[64];
  int msg_curln, start_x, row, col, redraw_point, lock_board, m_lines;
  screen_backup_t old_screen;
  FILE *frec;
  time_t bell_time;
  char line_space[11][80];
  char *textline[TSL_MAXAVAILINE];
  int lnsp_inuse[TSL_MAXAVAILINE];	/* �ϥΤ���line_space */
  int line_len[TSL_MAXAVAILINE];
  int Alice_modeswitch;	/* �ڤ�w�o�Xø�ϪO�n�D */
  int Bob_modeswitch;	/* ���w�o�Xø�ϪO�n�D */
  int availines;	/* �i�Φ�� */
  int paste_mode, key_sendline, key_newline;
  char prefix_cuid[IDLEN+3], blank_cuid[IDLEN+3];
  char prefix_itsuid[IDLEN+3], blank_itsuid[IDLEN+3];

  int i, ch;

#ifdef HAVE_GAME
  const int talkcmd_bwboard = TALKCMD_BWBOARD;
#endif
  const int talkcmd_bwlock = TALKCMD_BWLOCK;
  const int talkcmd_bell = TALKCMD_BELL;
  const int tslcmd_drawboard = TSLCMD_DRAWBOARD;

#ifdef EVERY_BIFF
  int biff = HAS_STATUS(STATUS_BIFF);
#else
  int biff = 0;
#endif


  memset(lnsp_inuse, 0, sizeof(lnsp_inuse));
  lnsp_inuse[0] = 1;
  textline[0] = line_space[0];
  textline[0][0] = '\0';
  line_len[0] = 0;
  row = 0;
  col = 0;
  availines = 1;
  redraw_point = -1;
  Alice_modeswitch = 0;
  Bob_modeswitch = 0;
  paste_mode = 0;
  m_lines = b_lines;
  lock_board = 0;
  msg_curln = 2;
  bell_time = 0;
  key_sendline = '\n';
  key_newline = Ctrl('N');
  start_x = sprintf(prefix_cuid, "%s: ", cuser.userid);
  sprintf(blank_cuid, "%*s", start_x, "");
  i = sprintf(prefix_itsuid, "%s: ", itsuid);
  sprintf(blank_itsuid, "%*s", i, "");

  clear();
  prints("\033[1;46m �ͤѻ��a \033[45m %s (%s)%*s\033[m\n", itsuid, itsnick, 64 - strlen(itsuid) - strlen(itsnick), "");
  outs(msg_seperator);
  draw_dialog(biff, availines, textline, paste_mode);
  tsl_foot(biff, Alice_modeswitch, Bob_modeswitch);

  utmp_mode(M_TALK);

  add_io(sock, 60);

  usr_fpath(rec_path, cuser.userid, FN_TALK_LOG);
  if(frec = fopen(rec_path, "w"))
  {
    fprintf(frec, "�i %s �P %s ����ѰO�� �j\n", cuser.userid, itsuid);
    fprintf(frec, "�}�l��Ѯɶ� [%s]\n\n", Now());
  }

  do
  {
    /* switch to drawboard */
    if(Alice_modeswitch && Bob_modeswitch)
    {
      screen_backup_t old_screen;

      scr_dump(&old_screen);
      if(talk_drawboard(sock, itsuid, itsnick, &lock_board))
      {
	scr_restore(&old_screen);
	break;
      }
      scr_restore(&old_screen);

      Alice_modeswitch = 0;
      Bob_modeswitch = 0;
      tsl_foot(biff, 0, 0);
    }

    /* redraw input line */
    if(redraw_point >= 0)
    {
      move(b_lines - availines + row, start_x + redraw_point);
      outs(textline[row] + redraw_point);
      clrtoeol();
      redraw_point = -1;
    }

    move(b_lines - availines + row, start_x + col);
    ch = vkey();

    /* screen size adjustion */
    if(m_lines != b_lines)
    {
      if(m_lines > b_lines)	/* �Y�p */
      {
	msg_curln = 2;
	clrregion(2, b_lines);
      }
      else
	clrregion(msg_curln + 1, b_lines);

      draw_dialog(biff, availines, textline, paste_mode);
      tsl_foot(biff, Alice_modeswitch, Bob_modeswitch);
      m_lines = b_lines;
    }


    if(ch == I_OTHERDATA)	/* mate's reaction */
    {
      int cmd_type;
      char *prefix;

      if(recv(sock, &cmd_type, sizeof(int), 0) != sizeof(int))
	goto disconnection;

      switch(cmd_type)
      {
#ifdef HAVE_GAME
      case TALKCMD_BWBOARD:
	i = 1;
	goto call_bwboard_plug;
#endif
      case TALKCMD_BWLOCK:
	lock_board = !lock_board;
	break;

      case TALKCMD_BELL:
	bell();
	break;

      case TSLCMD_DRAWBOARD:
	Bob_modeswitch = !Bob_modeswitch;
	tsl_foot(biff, Alice_modeswitch, Bob_modeswitch);
	break;

      default:
	prefix = prefix_itsuid;

	for(i = 0; i < cmd_type; i++)
	{
	  char len;

	  if(recv(sock, &len, 1, 0) != 1)
	    goto disconnection;

	  if(len > 0 && recv(sock, buf, len, 0) != len)
	    goto disconnection;

	  buf[len] = '\0';
	  chat_printline(15, prefix, buf, &msg_curln, availines);
	  if(frec)
	    fprintf(frec, "\033[32m%s%s\033[m\n", prefix, buf);

	  prefix = blank_itsuid;
	}
      }
    }
    else if(isprint2(ch))
    {
      if(line_len[row] >= 79 - start_x)
	continue;

      memmove(&textline[row][col + 1], &textline[row][col], line_len[row] - col);
      textline[row][col] = ch;

      redraw_point = col;
      col++;
      line_len[row]++;
      textline[row][line_len[row]] = '\0';
    }
    else if(ch == KEY_TAB)	/* insert 4 space */
    {
      if(line_len[row] >= 76 - start_x)
	continue;

      memmove(&textline[row][col + 4], &textline[row][col], line_len[row] - col);
      memset(textline[row] + col, ' ', 4);

      redraw_point = col;
      col += 4;
      line_len[row] += 4;
      textline[row][line_len[row]] = '\0';
    }
    else if(ch == key_sendline)
    {
      int sum_of_len = 0;
      char *prefix;

      for(i = 0; i < availines; i++)
	sum_of_len += line_len[i];

      if(sum_of_len <= 0)
	continue;

      if(availines > 1)
	clrregion(b_lines - availines - 1, b_lines - 3);

      if(send(sock, &availines, sizeof(int), 0) != sizeof(int))
	break;

      prefix = prefix_cuid;
      for(i = 0; i < availines; i++)
      {
	if(send(sock, &line_len[i], 1, 0) != 1)
	  goto disconnection;

	if(send(sock, textline[i], line_len[i], 0) != line_len[i])
	  goto disconnection;

	chat_printline(7, prefix, textline[i], &msg_curln, 1);
	if(frec)
	  fprintf(frec, "%s%s\n", prefix, textline[i]);

	prefix = blank_cuid;
      }

      textline[0][0] = '\0';
      line_len[0] = 0;
      col = row = 0;

      if(availines > 1)	/* reset dialog */
      {
	memset(lnsp_inuse, 0, sizeof(lnsp_inuse));
	lnsp_inuse[(char (*)[80])textline[0] - line_space] = 1;

	availines = 1;
	draw_dialog(biff, availines, textline, paste_mode);
      }
      else
	redraw_point = 0;

#ifdef EVERY_BIFF
      if(biff != HAS_STATUS(STATUS_BIFF))
      {
	biff = HAS_STATUS(STATUS_BIFF);
	tsl_foot(biff, Alice_modeswitch, Bob_modeswitch);
      }
#endif
    }
    else if(ch == KEY_LEFT)
    {
      if(col > 0)
      {
	col--;
	if((cuser.ufo & UFO_ZHC) && IS_BIG5_LOW(textline[row], col))
	  col--;
      }
      else if(row > 0)
      {
	row--;
	col = line_len[row];
      }
    }
    else if(ch == KEY_RIGHT)
    {
      if(col < line_len[row])
      {
	col++;
	if((cuser.ufo & UFO_ZHC) && IS_BIG5_LOW(textline[row], col))
	  col++;
      }
      else if(row < availines - 1)
      {
	row++;
	col = 0;
      }
    }
    else if(ch == KEY_UP)
    {
      if(row > 0)
      {
	row--;
	if(col > line_len[row])
	  col = line_len[row];
	if((cuser.ufo & UFO_ZHC) && IS_BIG5_LOW(textline[row], col))
	  col--;
      }
    }
    else if(ch == KEY_DOWN)
    {
      if(row < availines - 1)
      {
	row++;
	if(col > line_len[row])
	  col = line_len[row];
	if((cuser.ufo & UFO_ZHC) && IS_BIG5_LOW(textline[row], col))
	  col--;
      }
    }
    else if(ch == key_newline)
    {
      if(availines < TSL_MAXAVAILINE)
      {
	row++;
	memmove(&textline[row+1], &textline[row], sizeof(char*) * (availines - row));
	memmove(&line_len[row+1], &line_len[row], sizeof(int) * (availines - row));

	for(i = 0; i < TSL_MAXAVAILINE; i++)
	{
	  if(!lnsp_inuse[i])
	  {
	    textline[row] = line_space[i];
	    lnsp_inuse[i] = 1;
	    break;
	  }
	}
	if(i == TSL_MAXAVAILINE)
	  vmsg("debug: no available line space.");

	strcpy(textline[row], textline[row-1] + col);
	line_len[row] = strlen(textline[row]);
	textline[row-1][col] = '\0';
	line_len[row-1] = col;

	availines++;
	chat_printline(0, NULL, NULL, &msg_curln, availines);
	draw_dialog(biff, availines, textline, paste_mode);

	col = 0;
      }
    }
    else if(ch == KEY_HOME || ch == Ctrl('A'))
    {
      col = 0;
    }
    else if(ch == KEY_END || ch == Ctrl('E'))
    {
      col = line_len[row];
    }
    else if(ch == KEY_BKSP)
    {
      if(col > 0)
      {
	int x = ((cuser.ufo & UFO_ZHC) && IS_BIG5_LOW(textline[row], col - 1))? 2 : 1;
	char *data = textline[row] + col;
	memcpy(data - x, data, line_len[row] - col);
	col -= x;
	line_len[row] -= x;
	textline[row][line_len[row]] = '\0';
	redraw_point = col;
      }
    }
    else if(ch == KEY_DEL || ch == Ctrl('D'))
    {
      if(col < line_len[row])
      {
	int x = ((cuser.ufo & UFO_ZHC) && IS_BIG5_LOW(textline[row], col + 1))? 2 : 1;
	char *data = textline[row] + col;
	memcpy(data, data + x, line_len[row] - col - x);
	line_len[row] -= x;
	textline[row][line_len[row]] = '\0';
	redraw_point = col;
      }
    }
    else if(ch == Ctrl('K'))
    {
cutting_line:
      line_len[row] = col;
      textline[row][col] = '\0';
      redraw_point = col;
    }
    else if(ch == Ctrl('Y'))
    {
      col = 0;
      if(availines == 1)
	goto cutting_line;
      else
      {
	size_t displace = (char (*)[80])textline[row] - line_space;
	lnsp_inuse[displace] = 0;

	availines--;
	memcpy(&textline[row], &textline[row+1], sizeof(char*) * (availines - row));
	memcpy(&line_len[row], &line_len[row+1], sizeof(int) * (availines - row));

	clrregion(b_lines - availines - 2, b_lines - availines - 2);
	draw_dialog(biff, availines, textline, paste_mode);
      }
    }
    else if(ch == KEY_PGUP)
    {
      if(!frec)
      {
	vmsg("���~�G�L�k�إ߹�ܬ���");
	continue;
      }

      if(send(sock, &talkcmd_bwlock, sizeof(int), 0) != sizeof(int))
	break;
      add_io(0, 60);
      scr_dump(&old_screen);

      fflush(frec);
      more(rec_path, MFST_NORMAL);

      scr_restore(&old_screen);
      add_io(sock, 60);
      if(send(sock, &talkcmd_bwlock, sizeof(int), 0) != sizeof(int))
	break;

    }
    else if(ch == Ctrl('G'))
    {
      if(time(0) > bell_time)
      {
	if(send(sock, &talkcmd_bell, sizeof(int), 0) != sizeof(int))
	  goto disconnection;

	bell_time = time(0);
      }
    }
    else if(ch == Ctrl('W'))
    {
      msg_curln = 2;
      clrregion(2, b_lines - availines - 3);
    }
#ifdef HAVE_GAME
    else if(ch == Ctrl('O'))
    {
      if(lock_board)
	chat_printline(8, "", "���{�b�L�k�i��﫳�C", &msg_curln, availines);
      else
      {
	if(send(sock, &talkcmd_bwboard, sizeof(int), 0) != sizeof(int))
	  goto disconnection;
	else
	{
	  i = 0;
call_bwboard_plug:
	  if(DL_func("bin/bwboard.so:vaBWboard", sock, i) == -2)
	    break;
	}
      }
    }
#endif
    else if(ch == Ctrl('Q'))
    {
      if(send(sock, &talkcmd_bwlock, sizeof(int), 0) != sizeof(int))
	break;
      add_io(0, 60);
      scr_dump(&old_screen);

      more("etc/help/talk.hlp", MFST_NORMAL);

      scr_restore(&old_screen);
      add_io(sock, 60);
      if(send(sock, &talkcmd_bwlock, sizeof(int), 0) != sizeof(int))
	break;
    }
    else if(ch == Ctrl('T'))
    {
      if (cuser.userlevel)	/* guest ���i��Q�����ܽ� Talk */
      {
	cuser.ufo ^= UFO_PAGER;
	cutmp->ufo = cuser.ufo;

	chat_printline(2, "", (cuser.ufo & UFO_PAGER) ? "�� �����I�s��" : "�� ���}�I�s��", &msg_curln, availines);
      }
    }
    else if(ch == Ctrl('B'))
    {
      if(send(sock, &tslcmd_drawboard, sizeof(int), 0) != sizeof(int))
	break;

      Alice_modeswitch = !Alice_modeswitch;
      tsl_foot(biff, Alice_modeswitch, Bob_modeswitch);
    }
    else if(ch == Ctrl('P'))
    {
      int tmp = key_sendline;
      key_sendline = key_newline;
      key_newline = tmp;

      paste_mode = !paste_mode;
      draw_seperator(availines, paste_mode);
    }
    else if(ch == Ctrl('X'))
    {
      cuser.ufo ^= UFO_ZHC;
      draw_seperator(availines, paste_mode);
    }
#ifdef EVERY_Z
    else if(ch == Ctrl('Z'))
    {
      if(send(sock, &talkcmd_bwlock, sizeof(int), 0) != sizeof(int))
	break;
      strcpy(matid, cutmp->mateid);
      vio_save();
      scr_dump(&old_screen);

      every_Z(0);

      scr_restore(&old_screen);
      vio_restore();
      strcpy(cutmp->mateid, matid);
      if(send(sock, &talkcmd_bwlock, sizeof(int), 0) != sizeof(int))
	break;
    }
#endif

  }while(ch != TALK_EXIT_KEY);

disconnection:
  if(frec)
    fclose(frec);

  add_io(0, 60);
}


static void
talk_hangup(sock)
  int sock;
{
  cutmp->sockport = 0;
  cutmp->talker = -1;
  add_io(0, 60);
  close(sock);
}

static char *talk_reason[] =
{
  "�@�L�����A�N�u�O�ӨϥΪ̡C",
  "�藍�_�A�ڦ��Ƥ����A talk",
  "�ڲ{�b�ܦ��A�е��@�|��A call ��",
  "�ڲ{�b�����L�ӡA���@�U�ڷ| call �A",
  "�ڲ{�b���Q talk ��",
  "�A�ܷЭC�A�ڴN�O���Q talk �աI",
};


/* return 0: �S�� talk, 1: �� talk, -1: ��L */
int
talk_page(up)
  UTMP *up;
{
  int sock, msgsock;
  struct sockaddr_in sin;
  pid_t pid;
  int ans, length;
  char buf[80];
#if     defined(__OpenBSD__)
  struct hostent *h;
#endif

#ifdef EVERY_Z
  /* Thor.980725: �� talk & chat �i�� ^z �@�ǳ� */
  if (vio_holdon())
  {
    vmsg("�A�٦b��Y�H��Ѥ��I");
    return 0;
  }
#endif

  if (up->mode >= M_SYSTEM && up->mode <= M_CHAT)
  {
    vmsg("���S��k��A���");
    return 0;
  }

#ifdef EVERY_Z
  if(up->talker >= 0)
  {
    vmsg("��西�b���L�H��Ѥ�");
    return 0;
  }
#endif

  if (!(pid = up->pid) || kill(pid, 0))
  {
    vmsg(MSG_USR_LEFT);
    return 0;
  }

  sprintf(buf, "�T�w�n�M \033[1m%s\033m ��Ѷ�(Y/N)�H[N] ", up->userid);
  if (vans(buf) != 'y')
    return 0;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return 0;

#ifdef __OpenBSD__
  if (!(h = gethostbyname(str_host)))
    return -1;  
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = 0;
  memcpy(&sin.sin_addr, h->h_addr, h->h_length);  
#else
  sin.sin_family = AF_INET;
  sin.sin_port = 0;
  sin.sin_addr.s_addr = INADDR_ANY;
  memset(sin.sin_zero, 0, sizeof(sin.sin_zero));
#endif

  length = sizeof(sin);
  if (bind(sock, (struct sockaddr *) &sin, length) < 0 || 
    getsockname(sock, (struct sockaddr *) &sin, &length) < 0)
  {
    close(sock);
    vmsg("���~�Gsocket��l�ƥ���");
    return 0;
  }

  cutmp->sockport = sin.sin_port;
  strcpy(cutmp->mateid, up->userid);
  up->talker = cutmp - ushm->uslot;
  cutmp->talker = up - ushm->uslot;	/* dust.101206: �@���O�_���_�I�s���P�_ */
  utmp_mode(M_PAGE);
  kill(pid, SIGUSR1);

  clear();
  prints("�I�s %s ��... (�i�� Ctrl+C ���_�I�s)", up->userid);
  listen(sock, 1);
  add_io(sock, 30);
  do
  {
    msgsock = vkey();

    if (msgsock == Ctrl('C'))
    {
      talk_hangup(sock);
      return -1;
    }

    if (msgsock == I_TIMEOUT)
    {
      if (kill(pid, 0) < 0)	/* �ˬd���O�_�٦b�u�W */ 
      {
	talk_hangup(sock);
	vmsg(MSG_USR_LEFT);
	return -1;
      }
    }
  } while (msgsock != I_OTHERDATA);

  msgsock = accept(sock, NULL, NULL);
  talk_hangup(sock);
  if (msgsock == -1)
    return -1;

  length = read(msgsock, buf, sizeof(buf));
  ans = buf[0];
  if (ans == 'y')
  {
#ifdef EVERY_Z
    cutmp->talker = 1;	/* dust.101206: ��every_Z�X�h��@���ˬd�Ϊ��A�Ȥ����n */
    talk_speak(msgsock, up->userid, up->username);
    cutmp->talker = -1;
#else
    talk_speak(msgsock, up->userid, up->username);
#endif
  }
  else
  {
    move(4, 0);
    outs("�i�^���j");

    if (ans == ' ')
    {
      buf[length] = '\0';
      outs(buf);
    }
    else
      outs(talk_reason[ans - '0']);
  }

  close(msgsock);

  if(ans == 'y')
  {
    talk_save();
    vmsg("��ѵ���");
  }
  else
    vmsg(NULL);

  return 1;
}


/* ----------------------------------------------------- */
/* talk main-routines					 */
/* ----------------------------------------------------- */


int
t_pager()
{
#if 0	/* itoc.010923: �קK�~���A���M���@�I */
  �z�w�w�w�w�s�w�w�w�w�w�s�w�w�w�w�w�s�w�w�w�w�w�{
  �x        �x UFO_PAGER�x UFO_RCVER�x UFO_QUIET�x
  �u�w�w�w�w�q�w�w�w�w�w�q�w�w�w�w�w�q�w�w�w�w�w�t
  �x�����}��x          �x          �x          �x
  �u�w�w�w�w�q�w�w�w�w�w�q�w�w�w�w�w�q�w�w�w�w�w�t   
  �x�n�ͱM�u�x    ��    �x          �x          �x
  �u�w�w�w�w�q�w�w�w�w�w�q�w�w�w�w�w�q�w�w�w�w�w�t
  �x�������ۢx    ��    �x    ��    �x    ��    �x
  �|�w�w�w�w�r�w�w�w�w�w�r�w�w�w�w�w�r�w�w�w�w�w�}
#endif
  char *menu[] = {"1�����}��", "2�n�ͱM�u", "3��������", "q���}", NULL};

  switch(krans(b_lines, "qq", menu, "����������"))
  {
  case '1':
#ifdef HAVE_NOBROAD
    cuser.ufo &= ~(UFO_PAGER | UFO_RCVER | UFO_QUIET);
#else
    cuser.ufo &= ~(UFO_PAGER | UFO_QUIET);
#endif
    cutmp->ufo = cuser.ufo;
    return 0;	/* itoc.010923: ���M���ݭn��ø�ù��A���O�j����ø�~���s feeter ���� pager �Ҧ� */

  case '2':
    cuser.ufo |= UFO_PAGER;
#ifdef HAVE_NOBROAD
    cuser.ufo &= ~(UFO_RCVER | UFO_QUIET);
#else
    cuser.ufo &= ~UFO_QUIET;
#endif
    cutmp->ufo = cuser.ufo;
    return 0;

  case '3':
#ifdef HAVE_NOBROAD
    cuser.ufo |= (UFO_PAGER | UFO_RCVER | UFO_QUIET);
#else
    cuser.ufo |= (UFO_PAGER | UFO_QUIET);
#endif
    cutmp->ufo = cuser.ufo;
    return 0;
  }

  return XEASY;  
}


/* floatJ: �s�W�����s���ѽu���禡 */
int t_rcver()
{
  switch (vans("�����s���ѽu�� 1)���� 2)�ڦ� [Q] "))
  {
  case '1':
    cuser.ufo &= ~UFO_RCVER;    
    cutmp->ufo = cuser.ufo;
    return 0;
  case '2':
    cuser.ufo |= UFO_RCVER;
    cutmp->ufo = cuser.ufo;
    return 0;
  }
  
  return XEASY;
}

int
t_cloak()
{
#ifdef HAVE_SUPERCLOAK
  if (HAS_PERM(PERM_ALLADMIN))
  {
    switch (vans("�����Ҧ� 1)�@������ 2)�������� 3)������ [Q] "))
    {
    case '1':
      cuser.ufo |= UFO_CLOAK;
      cuser.ufo &= ~UFO_SUPERCLOAK;
      vmsg("�K�K�A���_���o�I");
      break;

    case '2':
      cuser.ufo |= (UFO_CLOAK | UFO_SUPERCLOAK);
      vmsg("�K�K�A�ð_���o�A��L�����]�ݤ���ڡI");
      break;

    case '3':
      cuser.ufo &= ~(UFO_CLOAK | UFO_SUPERCLOAK);
      vmsg("���{����F");
      time(&cutmp->login_time);     /* davy.110928: ��s�X�{�ɶ� */
      break;
    }
  }
  else
#endif
  { /* davy.110928: �_�ǳo�̥��ӨS���o�h��... ���i�H���`�B�@Orz */
    cuser.ufo ^= UFO_CLOAK;
    vmsg(cuser.ufo & UFO_CLOAK ? "�K�K�A���_���o�I" : "���{����F");

    if (!(cuser.ufo & UFO_CLOAK))
      time(&cutmp->login_time);     /* davy.110928: ��s�X�{�ɶ� */
  }

  cutmp->ufo = cuser.ufo;
  return XEASY;
}


int
t_query()
{
  ACCT acct;

  vs_bar("�d�ߺ���");
  if (acct_get(msg_uid, &acct) > 0)
  {
    move(1, 0);
    clrtobot();
    do_query(&acct, NULL);
  }

  return 0;
}


static int
talk_choose()
{
  UTMP *up, *ubase, *uceil;
  int self;
  char userid[IDLEN + 1];

  ll_new();

  self = cuser.userno;
  ubase = up = ushm->uslot;
  /* uceil = ushm->uceil; */
  uceil = ubase + ushm->count;
  do
  {
    if (up->pid && up->userno != self && up->userlevel && can_see(cutmp, up))
      ll_add(up->userid);
  } while (++up <= uceil);

  if (!vget(1, 0, msg_uid, userid, IDLEN + 1, GET_LIST))
    return 0;

  up = ubase;
  do
  {
    if (!str_cmp(userid, up->userid))
      return up->userno;
  } while (++up <= uceil);

  return 0;
}


int
t_talk()
{
  int tuid, unum, ucount;
  UTMP *up;
  char ans[4];

  if (total_user <= 1)
  {
    zmsg("�ثe�u�W�u���z�@�H�A���ܽФj�a�ӥ��{�i" BBSNAME "�j�a�I");
    return XEASY;
  }

  tuid = talk_choose();
  if (!tuid)
    return 0;

  /* ----------------- */
  /* multi-login check */
  /* ----------------- */

  move(3, 0);
  unum = 1;

  while ((ucount = utmp_count(tuid, 0)) > 1)
  {
    outs("(0) ���Q talk �F...\n");
    utmp_count(tuid, 1);
    vget(1, 33, "�п�ܤ@�Ӳ�ѹ�H [0]�G", ans, 3, DOECHO);
    unum = atoi(ans);
    if (unum == 0)
      return 0;
    move(3, 0);
    clrtobot();
    if (unum > 0 && unum <= ucount)
      break;
  }

  if (up = utmp_search(tuid, unum))
  {
    if (can_override(up))
    {
      if (tuid != up->userno)
	vmsg(MSG_USR_LEFT);
      else
	talk_page(up);
    }
    else
    {
      vmsg("��������I�s���F");
    }
  }

  return 0;
}


/* ------------------------------------- */
/* ���H�Ӧ���l�F�A�^���I�s��		 */
/* ------------------------------------- */

void
talk_rqst()
{
  UTMP *up;
  int mode, sock, ans, len, port;
  char buf[80];
  struct sockaddr_in sin;
  screen_backup_t old_screen;

#ifdef __OpenBSD__
  struct hostent *h;
#endif

  if(cutmp->talker < 0)
    return;

  up = &ushm->uslot[cutmp->talker];
  port = up->sockport;
  if (!port)
  {
    cutmp->talker = -1;
    return;
  }

  mode = bbsmode;
  utmp_mode(M_TRQST);

  scr_dump(&old_screen);

  clear();

  bell();
  prints("�z�Q�� %s (%s) ��ѶܡH (�Ӧ� %s)", up->userid, up->username, up->from);
  for (;;)
  {
    ans = vget(1, 0, "   (Q)�d�߹�� (Y)���� (N)�ڵ�  [Y] ", buf, 3, LCECHO);

    if(!up->pid || up->talker != cutmp - ushm->uslot)
    {
      vmsg("���w���_�I�s");
      goto caller_interrupt;
    }

    if (ans == 'q')
    {
      move(2, 0);
      my_query(up->userid);
      clrregion(b_lines, b_lines);
    }
    else
      break;
  }

  if (ans == 'n')
  {
    clrregion(2, b_lines);

    move(3, 0);
    len = sizeof(talk_reason) / sizeof(char *);
    for (ans = 0; ans < len; ans++)
      prints(" (%d) %s\n", ans, talk_reason[ans]);

    outs("\n�п�ܤ@�����ЩΦۦ��J [0]�G");

    ans = vget(5 + len, 0, "  ", buf + 1, 60, DOECHO);
    if(ans == '\0')
    {
      buf[0] = '0';
      len = 1;
    }
    else if(ans >= '0' && ans < '0' + len)
    {
      buf[0] = ans;
      len = 1;
    }
    else
    {
      buf[0] = ' ';
      len = strlen(buf);
    }
  }
  else
  {
    buf[0] = ans = 'y';
    len = 1;
  }

  if(!up->pid || up->talker != cutmp - ushm->uslot)
    goto caller_interrupt;


  sock = socket(AF_INET, SOCK_STREAM, 0);

#ifdef __OpenBSD__
  if (!(h = gethostbyname(str_host)))
  {
    cutmp->talker = -1;
    return;
  }
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = port;
  memcpy(&sin.sin_addr, h->h_addr, h->h_length);  
#else
  sin.sin_family = AF_INET;
  sin.sin_port = port;
  sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  memset(sin.sin_zero, 0, sizeof(sin.sin_zero));
#endif

  if (!connect(sock, (struct sockaddr *) & sin, sizeof(sin)))
  {
    send(sock, buf, len, 0);

    if (ans == 'y')
    {
      strcpy(cutmp->mateid, up->userid);

      talk_speak(sock, up->userid, up->username);
    }
  }

  close(sock);

  if (ans == 'y')
    talk_save();

caller_interrupt:
  scr_restore(&old_screen);
  refresh();
  cutmp->talker = -1;
  utmp_mode(mode);
}
