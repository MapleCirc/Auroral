/*-------------------------------------------------------*/
/* window.c						 */
/*-------------------------------------------------------*/
/* target : popup window menu				 */
/* create : 03/02/12					 */
/* update : 10/09/25					 */
/* author : verit.bbs@bbs.yzu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/* (pfterm基底化)dust.bbs@tcirc.twbbs.org		 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_POPUPMENU

#ifdef POPUP_ANSWER

/* ----------------------------------------------------- */
/* 選項繪製						 */
/* ----------------------------------------------------- */

static void 
draw_item (char *desc, int is_cur)
{
  if(is_cur)
    prints(COLOR4 "> [%c]%-25s  \033[m", *desc, desc + 1);
  else
    prints("\033[30;47m  (%c)%-25s  \033[m", *desc, desc + 1);
}



/* ----------------------------------------------------- */
/* 找選項						 */
/* ----------------------------------------------------- */

static int 
find_cur (		/* 找 ch 這個按鍵是第幾個選項 */
    int ch,
    int max,
    char *desc[]
)
{
  int i, cc;

  if (ch >= 'A' && ch <= 'Z')
    ch |= 0x20;		/* 換小寫 */

  for (i = 1; i <= max; i++)
  {
    cc = desc[i][0];
    if (cc >= 'A' && cc <= 'Z')
      cc |= 0x20;	/* 換小寫 */

    if (ch == cc)
      return i;
  }

  return -1;
}


/*------------------------------------------------------ */
/* 詢問選項，可用來取代 vans()				 */
/*------------------------------------------------------ */
/* y, x  是蹦出視窗左上角的 (y, x) 位置			 */
/* title 是視窗的標題					 */
/* desc  是選項的敘述：					 */
/*       第一個字串必須為兩個 char			 */
/*         第一個字元代表一開始游標停的位置		 */
/*         第二個字元代表按下 KEY_LEFT 的預設回傳值	 */
/*       中間的字串是每個選項的敘述 			 */
/*       最後一個字串必須為 NULL，否則會當機		 */
/* 傳回選項的第一個字，大寫字母會被轉為小寫字母		 */
/* desc[]長度不能超過26, title不能超過28，否則排版會亂掉 */
/*------------------------------------------------------ */

int 
pans (int y, int x, char *title, char *desc[])
{
  int cur = 0, old_cur, max = y, ch, i;
  char buf[128];
  screen_backup_t old_screen;

  scr_dump(&old_screen);

  /* 畫出整個選單 */
  move(y++, x);
  outstr(" ╭────────────────╮ ");

  sprintf(buf, " │" COLOR4 "  %-28s  \033[m│ ", title);
  move(y++, x);
  outstr(buf);

  move(y++, x);
  outstr(" ├────────────────┤ ");

  for (i = 1; desc[i]; i++)
  {
    move(y, x);
    outstr(" │                                │ ");

    move(y++, x + 3);
    draw_item(desc[i], desc[i][0] == desc[0][0]);

    if(desc[i][0] == desc[0][0])
      cur = i;
  }

  move(y, x);
  outstr(" ╰────────────────╯ ");

  y = max;
  max = i - 1;

  if(cur < 1)	/* 除錯用 */
  {
    vmsg("pans() fatal error: 找不到預設選項，請檢查參數desc");
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
	ch |= 0x20;		/* 回傳小寫 */
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

    default:		/* 去找所按鍵是哪一個選項 */
      if ((ch = find_cur(ch, max, desc)) > 0)
	cur = ch;
      break;
    }

    if(old_cur != cur)		/* 游標變動位置才需要重繪 */
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
draw_line (int y, int x, uschar *str)
{
  move(y, x);
  outstr(str);
}

/*------------------------------------------------------ */
/* 蹦出式視窗訊息，可用來取代 vmsg()			 */
/*------------------------------------------------------ */

int 
pmsg (
    char *msg		/* 不可為 NULL */
)
{
  int len, y, x, i;
  char buf[80];
  screen_backup_t old_screen;

  scr_dump(&old_screen);

  len = strlen(msg);
  if (len < 16)		/* 取 msg title 其中較長者為 len */
    len = 16;
  if (len % 2)		/* 變成偶數 */
    len++;
  y = (b_lines - 4) >> 1;	/* 置中 */
  x = (b_cols - 8 - len) >> 1;

  strcpy(buf, "╭");
  for (i = -4; i < len; i += 2)
    strcat(buf, "─");
  strcat(buf, "╮");
  draw_line(y++, x, buf);

  sprintf(buf, "│" COLOR4 "  %-*s  \033[m│", len, "請按任意鍵繼續..");
  draw_line(y++, x, buf);

  strcpy(buf, "├");
  for (i = -4; i < len; i += 2)
    strcat(buf, "─");
  strcat(buf, "┤");
  draw_line(y++, x, buf);

  sprintf(buf, "│\033[30;47m  %-*s  \033[m│", len, msg);
  draw_line(y++, x, buf);

  strcpy(buf, "╰");
  for (i = -4; i < len; i += 2)
    strcat(buf, "─");
  strcat(buf, "╯");
  draw_line(y++, x, buf);

  move(b_lines, 0);
  i = vkey();

  scr_restore(&old_screen);
  return i;
}
#endif	/* POPUP_MESSAGE */

#endif	/* HAVE_POPUPMENU */

