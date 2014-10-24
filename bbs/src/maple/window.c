/*-------------------------------------------------------*/
/* window.c						 */
/*-------------------------------------------------------*/
/* target : popup window menu				 */
/* create : 03/02/12					 */
/* update : 10/09/25					 */
/* author : verit.bbs@bbs.yzu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/* (pfterm�򩳤�)dust.bbs@tcirc.twbbs.org		 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_POPUPMENU

#ifdef POPUP_ANSWER

/* ----------------------------------------------------- */
/* �ﶵø�s						 */
/* ----------------------------------------------------- */

static void
draw_item(desc, is_cur)
  char *desc;
  int is_cur;
{
  if(is_cur)
    prints(COLOR4 "> [%c]%-25s  \033[m", *desc, desc + 1);
  else
    prints("\033[30;47m  (%c)%-25s  \033[m", *desc, desc + 1);
}



/* ----------------------------------------------------- */
/* ��ﶵ						 */
/* ----------------------------------------------------- */

static int			/* -1:�䤣�� >=0:�ĴX�ӿﶵ */
find_cur(ch, max, desc)		/* �� ch �o�ӫ���O�ĴX�ӿﶵ */
  int ch, max;
  char *desc[];
{
  int i, cc;

  if (ch >= 'A' && ch <= 'Z')
    ch |= 0x20;		/* ���p�g */

  for (i = 1; i <= max; i++)
  {
    cc = desc[i][0];
    if (cc >= 'A' && cc <= 'Z')
      cc |= 0x20;	/* ���p�g */

    if (ch == cc)
      return i;
  }

  return -1;
}


/*------------------------------------------------------ */
/* �߰ݿﶵ�A�i�ΨӨ��N vans()				 */
/*------------------------------------------------------ */
/* y, x  �O�ۥX�������W���� (y, x) ��m			 */
/* title �O���������D					 */
/* desc  �O�ﶵ���ԭz�G					 */
/*       �Ĥ@�Ӧr�ꥲ������� char			 */
/*         �Ĥ@�Ӧr���N��@�}�l��а�����m		 */
/*         �ĤG�Ӧr���N����U KEY_LEFT ���w�]�^�ǭ�	 */
/*       �������r��O�C�ӿﶵ���ԭz 			 */
/*       �̫�@�Ӧr�ꥲ���� NULL�A�_�h�|���		 */
/* �Ǧ^�ﶵ���Ĥ@�Ӧr�A�j�g�r���|�Q�ର�p�g�r��		 */
/* desc[]���פ���W�L26, title����W�L28�A�_�h�ƪ��|�ñ� */
/*------------------------------------------------------ */

int
pans(y, x, title, desc)
  int y, x;
  char *title;
  char *desc[];
{
  int cur = 0, old_cur, max = y, ch, i;
  char buf[128];
  screen_backup_t old_screen;

  scr_dump(&old_screen);

  /* �e�X��ӿ�� */
  move(y++, x);
  outstr(" �~�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�� ");

  sprintf(buf, " �x" COLOR4 "  %-28s  \033[m�x ", title);
  move(y++, x);
  outstr(buf);

  move(y++, x);
  outstr(" �u�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�t ");

  for (i = 1; desc[i]; i++)
  {
    move(y, x);
    outstr(" �x                                �x ");

    move(y++, x + 3);
    draw_item(desc[i], desc[i][0] == desc[0][0]);

    if(desc[i][0] == desc[0][0])
      cur = i;
  }

  move(y, x);
  outstr(" ���w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�� ");

  y = max;
  max = i - 1;

  if(cur < 1)	/* ������ */
  {
    vmsg("pans() fatal error: �䤣��w�]�ﶵ�A���ˬd�Ѽ�desc");
    abort_bbs();
  }

  old_cur = cur;
  y += 2;
  x += 3;

  while (1)
  {
    move(y + cur, x + 1);
    ch = vkey();

    switch(ch)
    {
    case KEY_LEFT:
    case KEY_RIGHT:
    case '\n':
      scr_restore(&old_screen);
      ch = (ch == KEY_LEFT) ? desc[0][1] : desc[cur][0];
      if (ch >= 'A' && ch <= 'Z')
	ch |= 0x20;		/* �^�Ǥp�g */
      return ch;

    case KEY_UP:
      cur = (cur == 1) ? max : cur - 1;
      break;

    case KEY_DOWN:
      cur = (cur == max) ? 1 : cur + 1;
      break;

    case KEY_HOME:
      cur = 1;
      break;

    case KEY_END:
      cur = max;
      break;

    default:		/* �h��ҫ���O���@�ӿﶵ */
      if ((ch = find_cur(ch, max, desc)) > 0)
	cur = ch;
      break;
    }

    if(old_cur != cur)		/* ����ܰʦ�m�~�ݭn��ø */
    {
      move(y + old_cur, x);
      draw_item(desc[old_cur], 0);
      move(y + cur, x);
      draw_item(desc[cur], 1);

      old_cur = cur;
    }
  }
}
#endif	/* POPUP_ANSWER */



#ifdef POPUP_MESSAGE

static void
draw_line(y, x, str)
  int y, x;
  uschar *str;
{
  move(y, x);
  outstr(str);
}

/*------------------------------------------------------ */
/* �ۥX�������T���A�i�ΨӨ��N vmsg()			 */
/*------------------------------------------------------ */

int
pmsg(msg)
  char *msg;		/* ���i�� NULL */
{
  int len, y, x, i;
  char buf[80];
  screen_backup_t old_screen;

  scr_dump(&old_screen);

  len = strlen(msg);
  if (len < 16)		/* �� msg title �䤤�����̬� len */
    len = 16;
  if (len % 2)		/* �ܦ����� */
    len++;
  y = (b_lines - 4) >> 1;	/* �m�� */
  x = (b_cols - 8 - len) >> 1;

  strcpy(buf, "�~");
  for (i = -4; i < len; i += 2)
    strcat(buf, "�w");
  strcat(buf, "��");
  draw_line(y++, x, buf);

  sprintf(buf, "�x" COLOR4 "  %-*s  \033[m�x", len, "�Ы����N���~��..");
  draw_line(y++, x, buf);

  strcpy(buf, "�u");
  for (i = -4; i < len; i += 2)
    strcat(buf, "�w");
  strcat(buf, "�t");
  draw_line(y++, x, buf);

  sprintf(buf, "�x\033[30;47m  %-*s  \033[m�x", len, msg);
  draw_line(y++, x, buf);

  strcpy(buf, "��");
  for (i = -4; i < len; i += 2)
    strcat(buf, "�w");
  strcat(buf, "��");
  draw_line(y++, x, buf);

  move(b_lines, 0);
  i = vkey();

  scr_restore(&old_screen);
  return i;
}
#endif	/* POPUP_MESSAGE */

#endif	/* HAVE_POPUPMENU */

