/*-------------------------------------------------------*/
/* visio.c						 */
/*-------------------------------------------------------*/
/* target : VIrtual Screen Input/Output routines 	 */
/* create : 95/03/29				 	 */
/* update : 10/09/23				 	 */
/*-------------------------------------------------------*/


#include <stdarg.h>
#include <arpa/telnet.h>

#include "bbs.h"


#define VOSZ_NORMAL	3072	/* output buffer 常駐大小 */
#define VOSZ_STEP	2048	/* output buffer 每次跳的大小 */
#define VOSZ_LIM	7168	/* output buffer 上限 */
#define VI_MAX		256	/* input buffer 大小 */

#define INPUT_ACTIVE	0
#define INPUT_IDLE	1



/* ----------------------------------------------------- */
/* 漢字 (zh-char) 判斷					 */
/* ----------------------------------------------------- */

int			/* 1:是 0:不是 */
is_zhc_low(str, n)	/* hightman.060504: 判斷字串中的第 n 個字符是否為漢字的後半字 */
  char *str;
  int n;
{
  char *end;

  end = str + n;
  while (str < end)
  {
    if (!*str)
      return 0;
    if (IS_ZHC_HI(*str))
      str++;
    str++;
  }

  return (str - end);
}


/* ----------------------------------------------------- */
/* output routines					 */
/* ----------------------------------------------------- */

static uschar *vo_pool;
static int vo_limit;
static int vo_size;


#ifdef VERBOSE		/* dust.090909: VERBOSE沒開... */
static void
telnet_flush(data, size)
  char *data;
  int size;
{
  int oset;

  oset = 1;

  if (select(1, NULL, &oset, NULL, NULL) <= 0)
  {
    abort_bbs();
  }

  xwrite(0, data, size);
}

#else
# define telnet_flush(data, size)	send(0, data, size, 0)
#endif


void
oflush()
{
  int size;

  if (size = vo_size)
  {
    telnet_flush(vo_pool, size);
    if(vo_limit != VOSZ_NORMAL)
    {
      vo_pool = realloc(vo_pool, VOSZ_NORMAL);
      vo_limit = VOSZ_NORMAL;	/* 輸出後把大小調回NORMAL */
    }
    vo_size = 0;
  }
}


void
vo_init()
{
  if(!(vo_pool = malloc(VOSZ_NORMAL)))
    abort_bbs();

  vo_limit = VOSZ_NORMAL;
}


void
ochar(ch)
  int ch;
{
  int size;

  size = vo_size;
  if (size > vo_limit - 2)
  {
    if(vo_limit >= VOSZ_LIM)
    {
      telnet_flush(vo_pool, size);
      size = 0;
      vo_limit = VOSZ_NORMAL;
    }
    else
      vo_limit += VOSZ_STEP;

    vo_pool = realloc(vo_pool, vo_limit);
  }

  vo_pool[size++] = ch;
  vo_size = size;
}


void
bell()
{
  static char sound[1] = {Ctrl('G')};

  telnet_flush(sound, sizeof(sound));
}



/* ----------------------------------------------------- */
/* virtual screen 的碎片				 */
/* ----------------------------------------------------- */

static int save_y, save_x;


void
cursor_save()
{
  getyx(&save_y, &save_x);
}


void
cursor_restore()
{
  move(save_y, save_x);
}



#ifdef SHOW_USER_IN_TEXT

/* 注意：如果要修改控碼格式時，請注意該控碼的長度是否會超過 ESCODE_MAXLEN */
/*       若 ESCODE_MAXLEN 所定義的長度不足可能會無法正常顯示 */

int		/* 回傳ESC-Star控制碼的長度 */
EScode(buf, src, szbuf, submsg)
  char *buf;
  const char *src;
  int szbuf;
  int *submsg;
{
  int year, mon, day, left, warn, base, short_fmt = 0, XDBBS_fmt = 0;
  const char *itr;

  switch (src[2])
  {
  case 's':		/* **s 顯示 ID */
    strcpy(buf, cuser.userid);
    *submsg = 1;
    return 3;

  case 'n':		/* **n 顯示暱稱 */
    strcpy(buf, cuser.username);
    *submsg = 1;
    return 3;

  case 'l':		/* **l 顯示上站次數 */
    sprintf(buf, "%d", cuser.numlogins);
    *submsg = 1;
    return 3;

  case 'p':		/* **p 顯示發文次數 */
    sprintf(buf, "%d", cuser.numposts);
    *submsg = 1;
    return 3;

  case 'g':		/* **g 顯示金幣 */
    sprintf(buf, "%d", cuser.gold);
    *submsg = 1;
    return 3;

  case 'm':		/* **m 顯示銀幣 */
    sprintf(buf, "%d", cuser.money);
    *submsg = 1;
    return 3;

  case 't':		/* **t 顯示日期 */
    strcpy(buf, Now());
    *submsg = 1;
    return 3;

  case 'u':		/* **u 顯示線上人數 */
    sprintf(buf, "%d", total_user);
    *submsg = 1;
    return 3;

  case 'b':		/* **b 顯示生日 */
    sprintf(buf, "%d/%d", cuser.month, cuser.day);
    *submsg = 1;
    return 3;

  case 'T':		/* **T 倒數計時-年份兩位數(相容於XDBBS) */
    XDBBS_fmt = 1;
    year = 20;
    mon = day = 0;
    base = 10;
    itr = src + 3;
    goto Get__Year_Mon_Day;

  case 'c':		/* **c, **C 倒數計時-年四兩位數(極光) */
    short_fmt = 1;
  case 'C':
    year = mon = day = 0;
    base = 1000;
    itr = src + (src[3] == 'x'? 4 : 3);	/* 另一種顯示"已經截止"的格式 */

Get__Year_Mon_Day:
    /* 年 */
    for(; base > 0; base /= 10, itr++)
    {
      year *= 10;
      if(isdigit(*itr))
	year += *itr - '0';
      else
	goto DateCountdown_CommonRoutine;
    }

    /* 月 */
    for(base = 10; base > 0; base /= 10, itr++)
    {
      mon *= 10;
      if(isdigit(*itr))
	mon += *itr - '0';
      else
	goto DateCountdown_CommonRoutine;
    }

    /* 日 */
    for(base = 10; base > 0; base /= 10, itr++)
    {
      day *= 10;
      if(isdigit(*itr))
	day += *itr - '0';
      else
	goto DateCountdown_CommonRoutine;
    }

    /* 天數警告 */
    if(*itr == 'w' && !XDBBS_fmt)
    {
      itr++;
      if(isdigit(*itr))
      {
	warn = *itr - '0';
	itr++;
	if(isdigit(*itr))
	{
	  warn = warn * 10 + *itr - '0';
	  itr++;
	}
      }
    }
    else
      warn = 0;


DateCountdown_CommonRoutine:
    left = date_countdown(year, mon, day, buf, short_fmt);

    *submsg = 1;
    if(left <= 0)
    {
      if(src[3] == 'x')
      {
	strcpy(buf, short_fmt? "0" : "0 天 0 小時 0 分 0 秒");
      }
      else
      {
	*submsg = -2;	/* *[1;30m */
	strcpy(buf, "已經截止");
      }
    }
    else if(left <= warn)
      *submsg = -1;	/* *[1;31m */

    return itr - src;

  default:
    strlcpy(buf, src + 1, 3);	/* unknown type, copy it */
    *submsg = 0;
    return 3;
  }
}


void
outx(str)
  uschar *str;
{
  int ch, submsg;
  char buf[80];

  while (ch = *str)
  {
    /* itoc.020301: ESC + * + s 等控制碼 */
    if (ch == KEY_ESC && str[1] == '*')
    {
      str += EScode(buf, str, sizeof(buf), &submsg);
      if(submsg < 0) attrset(10 + submsg);
      outs(buf);
      if(submsg < 0) attrset(7);
      continue;
    }
    outc(ch);
    str++;
  }
}
#endif



/* ----------------------------------------------------- */
/* clear the bottom line and show the message		 */
/* ----------------------------------------------------- */

void
outz(str)
  uschar *str;
{
  move(b_lines, 0);
  clrtoeol();
  outs(str);
}


void
outf(str)
  uschar *str;
{
  outz(str);
  prints("%*s\033[m", d_cols, "");
}


void
prints(char *fmt, ...)
{
  va_list args;
  uschar buf[512];	/* 最長只能印 511 字 */

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);

  outs(buf);
}


#ifdef POPUP_MESSAGE

int
vmsg(msg)
  char *msg;			/* length <= 54 */
{
  if (msg)
    return pmsg(msg);

  move(b_lines, 0);
  outs(VMSG_NULL);
  move(b_lines, 0);	/* itoc.010127: 修正在偵測左右鍵全形下，按左鍵會跳離二層選單的問題 */
  return vkey();
}


int
vmsg_bar(msg)		/* dust.091210: 強制顯示在底端的vmsg() */
  char *msg;
{
  if (msg)
  {
    move(b_lines, 0);
    clrtoeol();
    prints(COLOR1 " ◆ %-55s " COLOR2 " [請按任意鍵繼續] \033[m", msg);
  }
  else
  {
    move(b_lines, 0);
    outs(VMSG_NULL);
    move(b_lines, 0);
  }
  return vkey();
}

#else

int
vmsg(msg)
  char *msg;			/* length <= 54 */
{
  if (msg)
  {
    move(b_lines, 0);
    clrtoeol();
    prints(COLOR1 " ◆ %-55s " COLOR2 " [請按任意鍵繼續] \033[m", msg);
  }
  else
  {
    move(b_lines, 0);
    outs(VMSG_NULL);
    move(b_lines, 0);	/* itoc.010127: 修正在偵測左右鍵全形下，按左鍵會跳離二層選單的問題 */
  }
  return vkey();
}


int
vmsg_bar(msg)
  char *msg;
{
  return vmsg(msg);
}

#endif


static inline void
zkey()				/* press any key or timeout (1 sec) */
{
  struct timeval tv = {1, 100};
  int rset;

  rset = 1;
  select(1, (fd_set *) &rset, NULL, NULL, &tv);
}


void
zmsg(msg)			/* easy message */
  char *msg;
{
  outz(msg);
  move(b_lines, 0);	/* itoc.031029: 修正在偵測左右鍵全形下，按左鍵會跳離二層選單的問題 */

  refresh();
  zkey();
}


void
vs_bar(title)
  char *title;
{
  clear();
  prints("\033[1;33;44m【 %s 】\033[m\n", title);
}


static void
vs_line(msg)
  char *msg;
{
  int head, tail;

  if (msg)
    head = (strlen(msg) + 1) >> 1;
  else
    head = 0;

  tail = head;

  while (head++ < 38)
    outc('-');

  if (tail)
  {
    outc(' ');
    outs(msg);
    outc(' ');
  }

  while (tail++ < 38)
    outc('-');
  outc('\n');
}



/* ----------------------------------------------------- */
/* input routines					 */
/* ----------------------------------------------------- */

static uschar vi_pool[VI_MAX];
static int vi_size;
static int vi_head;

static int vio_fd;


#ifdef EVERY_Z

static int holdon_fd;		 /* Thor.980727: 跳出chat&talk暫存vio_fd用 */


void
vio_save()
{
  holdon_fd = vio_fd;
  vio_fd = 0;
}


void
vio_restore()
{
  vio_fd = holdon_fd;
  holdon_fd = 0;
}


int
vio_holdon()
{
  return holdon_fd;
}
#endif



static struct timeval vio_to = {60, 0};

void
add_io(fd, timeout)
  int fd;
  int timeout;
{
  vio_fd = fd;
  vio_to.tv_sec = timeout;
}



static inline int
iac_count(current)
  uschar *current;
{
  switch (*(current + 1))
  {
  case DO:
  case DONT:
  case WILL:
  case WONT:
    return 3;

  case SB:			/* loop forever looking for the SE */
    {
      uschar *look = current + 2;

      /* fuse.030518: 線上調整畫面大小，重抓 b_lines */
      if ((*look) == TELOPT_NAWS)
      {
	b_lines = ntohs(* (short *) (look + 3)) - 1;
	b_cols = ntohs(* (short *) (look + 1)) - 1;
	if (b_lines >= T_LINES)
	  b_lines = T_LINES - 1;
	else if (b_lines < 23)
	  b_lines = 23;
	if (b_cols >= T_COLS)
	  b_cols = T_COLS - 1;
	else if (b_cols < 79)
	  b_cols = 79;
	d_cols = b_cols - 79;
	resizeterm(b_lines + 1, b_cols + 1);
      }

      for (;;)
      {
	if ((*look++) == IAC)
	{
	  if ((*look++) == SE)
	  {
	    return look - current;
	  }
	}
      }
    }
  }
  return 1;
}



#define	IM_TRAIL	0x01	/* CR... */
#define	IM_REPLY	0x02	/* ^R */
#define	IM_TALK		0x04

static int imode = 0;

static int
igetch()
{
  static int idle = 0;

  int cc, fd, nfds, rset;
  uschar *data;

  data = vi_pool;
  nfds = 0;

  for (;;)
  {
    if (vi_size <= vi_head)
    {
      if (nfds == 0)
      {
	refresh();
	fd = (imode & IM_REPLY) ? 0 : vio_fd;
	nfds = fd + 1;
	if (fd)
	  fd = 1 << fd;
      }

      for (;;)
      {
	struct timeval tv = vio_to;

	rset = 1 | fd;
	cc = select(nfds, (fd_set *) &rset, NULL, NULL, &tv);

	if (cc > 0)
	{
	  if (fd & rset)
	    return I_OTHERDATA;

	  cc = recv(0, data, VI_MAX, 0);
	  if (cc > 0)
	  {
	    vi_head = (*data) == IAC ? iac_count(data) : 0;
	    if (vi_head >= cc)
	      continue;
	    vi_size = cc;

#ifdef DETAIL_IDLETIME
	    if (cutmp)
#else
	    if (idle && cutmp)
#endif
	    {
	      idle = 0;

#ifdef DETAIL_IDLETIME
	      time(&cutmp->idle_time);	/* 若 #define DETAIL_IDLETIME，則 idle_time 表示開始閒置的時間(秒) */
#else
	      cutmp->idle_time = 0;	/* 若 #undef DETAIL_IDLETIME，則 idle_time 表示已經閒置了多久(分) */
#endif

#ifdef BMW_COUNT
	      /* itoc.010421: 按任一鍵後接收水球數回歸 0 */
	      cutmp->bmw_count = 0;
#endif
	    }
	    break;
	  }

	  if ((cc == 0) || (errno != EINTR))
	    abort_bbs();
	}
	else if (cc == 0)	/* paging timeout : 每 60 秒更新一次 idle */
	{
	  if (vio_to.tv_sec < 60)
	    return I_TIMEOUT;

	  idle += vio_to.tv_sec / 60;

#ifdef TIME_KICKER
	  if (idle > IDLE_TIMEOUT)
	  {
	    move(b_lines, 0);
	    outstr("\033[1;33;46m 自動斷線 \033[m 您閒置太久而未登入，系統已自動將您斷線 .__./~\\");
	    refresh();
	    abort_bbs();
	  }
	  else if (idle >= IDLE_TIMEOUT - IDLE_WARNOUT)	/* itoc.001222: 閒置過久警告 */
	  {
	    move(b_lines, 0);
	    outstr("\033[1;33;46m 提醒 \033[m 您在登入畫面閒置過久，請盡速登入，否則系統自動斷線。");

	    /* floatj.110317: 有人抗議鈴聲很吵 (老實說我也這樣覺得)，所以就拔掉了 */
	    //bell();
	    
	    refresh();
	  }
#endif

#ifndef DETAIL_IDLETIME
	  cutmp->idle_time = idle;
#endif

	  if (bbsmode < M_XMENU)	/* 在 menu 裡面要換 movie */
	  {
	    note_setrand();
	    movie();
	    refresh();
	  }
	}
	else
	{
	  if (errno != EINTR)
	    abort_bbs();
	}
      }
    }

    cc = data[vi_head++];

    /* CR/LF mapping */
    if (imode & IM_TRAIL)
    {
      imode &= ~IM_TRAIL;
      if (cc == 0 || cc == '\n')
	continue;
    }
    if (cc == '\r')
    {
      imode |= IM_TRAIL;
      return '\n';
    }

    return cc;
  }
}


/* dust.101031: 引入vtkbd */
/* 目前尚不支援F1~F12、shift TAB */
/* 處理CR/LF的部份沿用原本M3的設計 */

/* Virtual Terminal Keyboard (vtkbd)
*
* piaip's new re-implementation of xterm/VT100/220/ANSI key input
* escape sequence parser for BBS
*
* Author: Hung-Te Lin (piaip)
* Create: Wed Sep 23 15:06:43 CST 2009
* ---------------------------------------------------------------------------
* Copyright (c) 2009 Hung-Te Lin <piaip@csie.org>
* All rights reserved.
* Distributed under BSD license (GPL compatible).
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
* ---------------------------------------------------------------------------
* References:
*  http://support.dell.com/support/edocs/systems/pe2650/en/ug/5g387ad0.htm
*  http://aperiodic.net/phil/archives/Geekery/term-function-keys.html
*  http://publib.boulder.ibm.com/infocenter/iseries/v5r3/index.jsp?topic=/rzaiw/rzaiwvt220opmode.htm
*  http://www.rebol.com/docs/core23/rebolcore-18.html
*  http://ascii-table.com/ansi-escape-sequences-vt-100.php
*  http://web.mit.edu/gnu/doc/html/screen_10.html
*  http://vt100.net/docs/vt220-rm/chapter3.html
*  http://inwap.com/pdp10/ansicode.txt
*  http://www.connectrf.com/Documents/vt220.html
*  http://www.ibb.net/~anne/keyboard.html
*  http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
*  http://invisible-island.net/xterm/xterm.faq.html
*  http://www.vim.org/htmldoc/term.html
*  http://wiki.archlinux.org/index.php/Why_don%27t_my_Home_and_End_keys_work_in_terminals%3F
*  http://yz.kiev.ua/www/etc/putty/Section3.5.html
*  PuTTY Source < terminal.c, term_key() >
*  Termcap
* ---------------------------------------------------------------------------
* * BS/DEL Rules
*   The BackSpace, Erase(<X]), and Delete keys are special due to history...
*    - on vt220,       BS=0x7F, Delete=ESC[3~  (screen/xterm/PuTTY/PCMan)
*    - on vt100,       BS=0x7F
*    - on vt100/xterm, BS=0x08, Delete=0x7F    (VMX/Windows/DOS telnet/KKMan)
*   So we define
*      KEY_BS  = BACKSPACE/ERASE = 0x08, 0x7F
*      KEY_DEL = DELETE          = ESC[3~
*
* * Editing Keys (Home/End/Ins/Del/PgUp/PgDn, Find/Select/Ins/Prev/Remove/Next):
*   Some old terminals use location mapping instead of mnemonic mapping:
*     http://invisible-island.net/xterm/xterm.faq.html#xterm_keypad
*   Well.... I decide to follow the mnemonic mapping. Unfortunately some
*   terminals (may include gnome-terminal, as I've heard) may get into trouble.
* ---------------------------------------------------------------------------
* * The complete list to support:
*   - Up/Down/Right/Left:          <Esc> [ <A-D>       | <Esc> O <A-D> (app)
*   - Home/Ins/Del/End/PgUp/PgDn:  <Esc> [ <1~6> ~
*   - Shift-TAB:                   <Esc> [ Z           | <Esc> [ 0 Z
*   - F1~F4:                       <Esc> [ 1 <1234> ~  | <Esc> O <PQRS>
*   - F5:                          <Esc> [ 1 <5> ~
*   - F6-F8:                       <Esc> [ 1 <789> ~
*   - F9-F12:                      <Esc> [ 2 <0134> ~
*   - Num 0-9 *+,-./=ENTER:        <Esc> O <pqrstuvwxyjklmnoXM>
*   - (SCO) End/PgDn/Home/PgUp/Ins <Esc> [ <FGHIL>
*   - (SCO) Del                    <0x7F>
*   - (Xterm) HOME/END             <Esc> <[O> <HF>
    - (rxvt)  HOME/END             <Esc> [ <78> ~
*   - (putty-rxvt) HOME            <Esc> [ H
*   - (putty-rxvt) END             <Esc> O w
*   - (Old Term?) Home/Ins/Del/End/PgUp/PgDn:  <Esc> [ <214536> ~  // not supported
*
*   Note: we don't support some rare terms like <Esc> O <TUVWXYZA> described
*   in Dell 2650 in order to prevent confusion.
*   Num pad is also always converted to digits.
*/

typedef enum
{
  VKSTATE_NORMAL = 0,
  VKSTATE_ESC,		// <Esc>
  VKSTATE_ESC_APP,	// <Esc> O
  VKSTATE_ESC_QUOTE,	// <Esc> [
  VKSTATE_ZERO,		// <Esc> [ 0    (wait Z)
  VKSTATE_ONE,		// <Esc> [ <1>
  VKSTATE_TWO,		// <Esc> [ <2>
  VKSTATE_TLIDE		// <Esc> [ *    (wait ~, return esc_arg)
}VKSTATES;

#define VKRAW_BS    0x08	// \b = Ctrl('H')
#define VKRAW_ERASE 0x7F	// <X]

static int
vtkbd()
{
  int cc, esc_arg;
  VKSTATES state = VKSTATE_NORMAL;

  while(1)
  {
    cc = igetch();

    switch (state)
    {
    case VKSTATE_NORMAL:	// original state
      if (cc == KEY_ESC)
      {
	state = VKSTATE_ESC;
	continue;
      }

      switch (cc)	// simple mappings
      {
      // BS/ERASE/DEL Rules
      case VKRAW_BS:
      case VKRAW_ERASE:
	return KEY_BKSP;
      }

      return cc;

    case VKSTATE_ESC:		// <Esc>
      switch (cc)
      {
      case '[':
	state = VKSTATE_ESC_QUOTE;
	continue;

      case 'O':
	state = VKSTATE_ESC_APP;
	continue;

      case '0':
	state = VKSTATE_ZERO;
	continue;
      }

      return Esc(cc);

#if 0	/* dust.101101: 目前M3-極光2.0暫定不支援Shift tab */
    case VKSTATE_ZERO:  // <Esc> 0
      if (cc != 'Z')
	break;

      return KEY_STAB;
#endif

    case VKSTATE_ESC_APP:	// <Esc> O

      switch (cc)
      {
      case 'A':
      case 'B':
      case 'C':
      case 'D':
	return KEY_UP - (cc - 'A');

	// SCO
      case 'H':
	return KEY_HOME;
      case 'F':
	return KEY_END;
      case 'G':
	return KEY_PGDN;
      case 'I':
	return KEY_PGUP;
      case 'L':
	return KEY_INS;

#if 0	/* dust.101101: 目前M3-極光2.0暫定不支援F1~F12 */
      case 'P':
      case 'Q':
      case 'R':
      case 'S':
	return KEY_F1 + (cc - 'P');
#endif
	// rxvt style DELETE
      case 'w':
	return KEY_DEL;

	// Num pads: was always converted to NumLock=ON
	// However we let 'w' map to DEL..
      case 'p': case 'q': case 'r': case 's': case 't':
      case 'u': case 'v': case 'x': case 'y':
	return '0' + (cc - 'p');

      case 'M':
	return KEY_ENTER;

      case 'X':
	return '=';

      case 'j': case 'k': case 'l': case 'm':
      case 'n': case 'o':
	return "*+,-./"[cc - 'j'];
      }
      break;

    case VKSTATE_ESC_QUOTE:	// <Esc> [

      switch(cc)
      {
      case 'A':
      case 'B':
      case 'C':
      case 'D':
	return KEY_UP - (cc - 'A');

      case '1':
	state = VKSTATE_ONE;
	continue;
      case '2':
	state = VKSTATE_TWO;
	continue;

      case '3':
      case '4':
      case '5':
      case '6':
	state = VKSTATE_TLIDE;
	esc_arg = KEY_DEL - (cc - '3');
	continue;

      case '7':
	state = VKSTATE_TLIDE;
	esc_arg = KEY_HOME;
	continue;
      case '8':
	state = VKSTATE_TLIDE;
	esc_arg = KEY_END;
	continue;

#if 0	/* dust.101101: 目前M3-極光2.0暫定不支援F1~F12 */
      case 'Z':
	return KEY_STAB;
#endif
	// SCO
      case 'H':
	state = VKSTATE_NORMAL;
	return KEY_HOME;
      case 'F':
	state = VKSTATE_NORMAL;
	return KEY_END;
      case 'G':
	state = VKSTATE_NORMAL;
	return KEY_PGDN;
      case 'I':
	state = VKSTATE_NORMAL;
	return KEY_PGUP;
      case 'L':
	state = VKSTATE_NORMAL;
	return KEY_INS;
      }
      break;

    case VKSTATE_ONE:		// <Esc> [ 1
      if (cc == '~')
      {
	state = VKSTATE_NORMAL;
	return KEY_HOME;
      }
#if 0	/* dust.101101: 目前M3-極光2.0暫定不支援F1~F12 */
      switch(cc)
      {
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
	state = VKSTATE_TLIDE;
	ctx->esc_arg = KEY_F1 + cc - '1'; // F1 .. F5
	continue;

      case '7':
      case '8':
      case '9':
	state = VKSTATE_TLIDE;
	ctx->esc_arg = KEY_F6 + cc - '7'; // F6 .. F8
	continue;
      }
#endif
      break;

    case VKSTATE_TWO:		// <Esc> [ 2
      if (cc == '~')
      {
	state = VKSTATE_NORMAL;
	return KEY_INS;
      }
#if 0
      switch(c)
      {
      case '0':
      case '1':
	state = VKSTATE_TLIDE;
	ctx->esc_arg = KEY_F9 + cc - '0'; // F9 .. F10
	continue;

      case '3':
      case '4':
	state = VKSTATE_TLIDE;
	ctx->esc_arg = KEY_F11 + cc - '3'; // F11 .. F12
	continue;
      }
#endif
      break;

    case VKSTATE_TLIDE:		// Esc [ <12> <0-9> ~
      if (cc != '~')
	break;

      return esc_arg;
    }

    //unknown key. just return
    return cc;
  }
}


int
vkey()
{
  int key;

  while(1)
  {
    key = vtkbd();

    switch(key)
    {
    case Ctrl('R'):      /* lkchu.990513: 鎖定時不可回訊 */
      if((imode & IM_REPLY) || (bbstate & STAT_LOCK) || !(bbstate & STAT_STARTED))
	break;

/* Thor.980307: 在 ^R 時 talk 會因沒有 vio_fd 看不到 I_OTHERDATA，而看不到對方打的字，所以在 ^R 時禁止 talk */
      signal(SIGUSR1, SIG_IGN);

      imode |= IM_REPLY;
      bmw_reply();
      imode &= ~IM_REPLY;

      signal(SIGUSR1, (void *) talk_rqst);

#ifdef BMW_COUNT
      /* itoc.010907: 按任一鍵後接收水球數回歸 0 */
      cutmp->bmw_count = 0;
#endif
      continue;

    case Ctrl('L'):
      redrawwin();
      refresh();
      continue;;
    }

    return key;
  }
}



int
num_in_buf()	/* dust.100815: for new VE and pmore */
{
  return vi_size - vi_head;
}



/* ----------------------------------------------------- */
/* some basic tools					 */
/* ----------------------------------------------------- */

#define	MATCH_END	0x8000
/* Thor.990204.註解: 代表MATCH完結, 要嘛就補足, 要嘛就維持原狀, 不秀出可能的值了 */

static void
match_title()
{
  move(2, 0);
  clrtobot();
  vs_line("相關資訊一覽表");
}


static int
match_getch()
{
  int ch;

  outs("\n★ 列表(C)繼續 (Q)結束？[C] ");
  ch = vkey();
  if (ch == 'q' || ch == 'Q')
    return ch;

  move(3, 0);
  clrtobot();
  return 0;
}



static BRD *xbrd;


BRD *
ask_board(board, perm, msg)
  char *board;
  int perm;
  char *msg;
{
  if (msg)
  {
    move(2, 0);
    outs(msg);
  }

  if (vget(1, 0, "請輸入看板名稱(按空白鍵自動搜尋)：", board, BNLEN + 1, GET_BRD | perm))
    return xbrd;

  return NULL;
}


static int
vget_match(prefix, len, op)
  char *prefix;
  int len;
  int op;
{
  char *data, *hit;
  char newprefix[BNLEN + 1];	/* 繼續補完的板名 */
  int row, col, match;
  int rlen;			/* 可補完的剩餘長度 */

  row = 3;
  col = match = rlen = 0;

  if (op & GET_BRD)
  {
    usint perm;
    int i;
    char *bits, *n, *b;
    BRD *head, *tail;

    extern BCACHE *bshm;
    extern char brd_bits[];

    perm = op & (BRD_L_BIT | BRD_R_BIT | BRD_W_BIT);
    bits = brd_bits;
    head = bshm->bcache;
    tail = head + bshm->number;

    do
    {
      if (perm & *bits++)
      {
	data = head->brdname;

	if (str_ncmp(prefix, data, len))
	  continue;

	xbrd = head;

	if ((op & MATCH_END) && !data[len])
	{
	  strcpy(prefix, data);
	  return len;
	}

	match++;
	hit = data;

	if (op & MATCH_END)
	  continue;

	if (match == 1)
	{
	  match_title();
	  if (data[len])
	  {
	    strcpy(newprefix, data);
	    rlen = strlen(data + len);
	  }
	}
	else if (rlen)	/* LHD.051014: 還有可補完的餘地 */
	{
	  n = newprefix + len;
	  b = data + len;
	  for (i = 0; i < rlen && ((*n | 0x20) == (*b | 0x20)); i++, n++, b++)
	    ;
	  *n = '\0';
	  rlen = i;
	}

	move(row, col);
	outs(data);

	col += BNLEN + 1;
	if (col > b_cols + 1 - BNLEN - 1)	/* 總共可以放 (b_cols + 1) / (BNLEN + 1) 欄 */
	{
	  col = 0;
	  if (++row >= b_lines)
	  {
	    if (match_getch() == 'q')
	      break;

	    move(row = 3, 0);
	    clrtobot();
	  }
	}
      }
    } while (++head < tail);
  }
  else if (op & GET_USER)
  {
    struct dirent *de;
    DIR *dirp;
    int cc;
    char fpath[16];

    /* Thor.981203: USER name至少打一字, 用"<="會比較好嗎? */
    if (len == 0)
      return 0;

    cc = *prefix;
    if (cc >= 'A' && cc <= 'Z')
      cc |= 0x20;
    if (cc < 'a' || cc > 'z')
      return 0;

    sprintf(fpath, "usr/%c", cc);
    dirp = opendir(fpath);
    while (de = readdir(dirp))
    {
      data = de->d_name;
      if (str_ncmp(prefix, data, len))
	continue;

      if (!match++)
      {
	match_title();
	strcpy(hit = fpath, data);	/* 第一筆符合的資料 */
      }

      move(row, col);
      outs(data);

      col += IDLEN + 1;
      if (col > b_cols + 1 - IDLEN - 1)	/* 總共可以放 (b_cols + 1) / (IDLEN + 1) 欄 */
      {
	col = 0;
	if (++row >= b_lines)
	{
	  if (match_getch())
	    break;
	  row = 3;
	}
      }
    }

    closedir(dirp);
  }
  else /* Thor.990203.註解: GET_LIST */
  {
    LinkList *list;
    extern LinkList *ll_head;

    for (list = ll_head; list; list = list->next)
    {
      data = list->data;

      if (str_ncmp(prefix, data, len))
	continue;

      if ((op & MATCH_END) && !data[len])
      {
	strcpy(prefix, data);
	return len;
      }

      match++;
      hit = data;

      if (op & MATCH_END)
	continue;

      if (match == 1)
	match_title();

      move(row, col);
      outs(data);

      col += IDLEN + 1;
      if (col > b_cols + 1 - IDLEN - 1)	/* 總共可以放 (b_cols + 1) / (IDLEN + 1) 欄 */
      {
	col = 0;
	if (++row >= b_lines)
	{
	  if (match_getch())
	    break;
	  row = 3;
	}
      }
    }
  }

  if (match == 1)
  {
    strcpy(prefix, hit);
    return strlen(hit);
  }
  else if (rlen)
  {
    strcpy(prefix, newprefix);
    return len + rlen;
  }

  return 0;
}



static char lastcmd[MAXLASTCMD][80];

int vget_BottomMode = 0;	/* dust.100324: 解決水球蓋掉vget(b_lines, ...)的問題 */


/* NOECHO  0x0000  不顯示，用於密碼取得 */
/* DOECHO  0x0100  一般顯示 */
/* LCECHO  0x0200  low case echo，換成小寫 */
/* GCARRY  0x0400  會顯示上一次/目前的值 */
/* FRCSAV  0x8000  不論長度強制寫進lastcmd[]裡 */

#define IS_BIG5LEAD(ch) (ch >= 0x81)
#define PASTE_BACK() { \
  strncpy(data, physical[currline], max); \
  if(len > max) len = max; \
  currline = 0; \
}

int
vget(line, col, prompt, data, max, echo)
  int line, col;
  uschar *prompt, *data;
  int max, echo;
{
  int ch, len, ox, cx, i, n, currline, quit;
  int InvalidateArea[2];	/* {起始點座標, 結束點座標} (皆以ox為原點) */
  char *physical[MAXLASTCMD + 1];
  int bottom_mode = (line == b_lines);
  uschar dbc_lead = 0;

  if(bottom_mode)
    vget_BottomMode = 1;
  else
    vget_BottomMode = 0;

  if(max > sizeof(lastcmd[0]))
    max = sizeof(lastcmd[0]);

  physical[0] = data;
  for(i = 0; i < MAXLASTCMD; i++)
    physical[i + 1] = lastcmd[i];

  move(line, col);
  clrtoeol();
  outs(prompt);

  max--;
  getyx(&line, &ox);
  InvalidateArea[0] = 0;
  InvalidateArea[1] = max;

  if(echo & GCARRY)
    len = strlen(data);
  else
    len = 0;

  cx = len;
  currline = 0;
  quit = 0;

  for(;;)
  {
    /* redraw */
    if(InvalidateArea[0] >= 0)
    {
      move(line, ox + InvalidateArea[0]);

      attrset(0x70);

      for(i = InvalidateArea[0]; i <= InvalidateArea[1]; i++)
      {
	if(i >= len)
	  outc(' ');
	else if(echo)
	  outc(physical[currline][i]);
	else
	  outc('*');
      }

      attrset(7);

      InvalidateArea[0] = -1;
    }

#ifdef CTECHO
    if (echo & CTECHO)
    {
      move(line, 74);
      prints("%02d/%02d", len, max);
    }
#endif

    if(quit)
      break;

    if(bottom_mode)
      vget_BottomMode = 1;

    move(line, ox + cx);
    ch = vkey();
    if(IS_BIG5LEAD(ch) && !dbc_lead)
    {
      dbc_lead = ch;
      continue;
    }

    if(ch == '\n')
    {
      if(currline)
	PASTE_BACK();

      data[len] = '\0';
      if(echo & (GET_BRD | GET_LIST))
      {
	/* Thor.990204:要求輸入任一字才代表自動 match, 否則算cancel */
	if(len > 0)
	{
	  ch = len;
	  len = vget_match(data, len, echo | MATCH_END);
	  if(len > ch)
	  {
	    InvalidateArea[0] = 0;
	    InvalidateArea[1] = len - 1;
	  }
	  else if(len == 0)
	    data[0] = '\0';
	}
	else
	  bbstate |= STAT_CANCEL;
      }

      quit = 1;
    }
    else if(isprint2(ch))
    {
      if(currline)
	PASTE_BACK();

      if (ch == ' ' && (echo & (GET_USER | GET_BRD | GET_LIST)))
      {
	ch = vget_match(data, len, echo);
	if(ch > len)
	{
	  InvalidateArea[0] = 0;
	  InvalidateArea[1] = ch - 1;
	  cx = len = ch;
	}
	continue;
      }

      if(len >= max || (len == max - 1 && dbc_lead))
      {
	bell();
	dbc_lead = 0;
	continue;
      }

      InvalidateArea[0] = cx;

      if(dbc_lead)
      {
	for(i = len; i > cx; i--)
	  data[i] = data[i-1];
	data[cx] = dbc_lead;

	len++;
	cx++;
	dbc_lead = 0;
      }

      for(i = len; i > cx; i--)
	data[i] = data[i-1];
      data[cx] = ch;

      InvalidateArea[1] = len;
      len++;
      cx++;
    }
    else if(ch == KEY_BKSP)
    {
      if(cx == 0) continue;

      if(currline)
	PASTE_BACK();
      InvalidateArea[1] = len;

      cx--;
      n = 1;
      /* hightman.060504: 判斷現在刪除的位置是否為漢字的後半段，若是刪二字元 */
      if((cuser.ufo & UFO_ZHC) && echo && IS_BIG5_LOW(data, cx))
      {
	cx--;
	n++;
      }

      len -= n;
      for(i = cx; i < len; i++)
	data[i] = data[i + n];

      InvalidateArea[0] = cx;
    }
    else if(ch == Ctrl('S'))	/* 重繪 */
    {
      move(line, col);
      clrtoeol();
      outs(prompt);
      InvalidateArea[0] = 0;
      InvalidateArea[1] = max;
    }
    else if(!echo)	/* 輸入 password 時只能按 BackSpace 和 重繪 */
    {
      continue;
    }
    else if(ch == KEY_LEFT || ch == Ctrl('B'))
    {
      if(cx == 0) continue;

      cx--;
      /* hightman.060504: 左移時碰到漢字移雙格 */
      if((cuser.ufo & UFO_ZHC) && IS_BIG5_LOW(physical[currline], cx))
	cx--;
    }
    else if(ch == KEY_RIGHT || ch == Ctrl('F'))
    {
      if(cx >= len) continue;

      cx++;
      /* hightman.060504: 右移時碰到漢字移雙格 */
      if((cuser.ufo & UFO_ZHC) && cx < len && IS_BIG5_LOW(physical[currline], cx))
	cx++;
    }
    else if(ch == KEY_DEL || ch == Ctrl('D'))
    {
      if(cx >= len) continue;

      if(currline)
	PASTE_BACK();
      InvalidateArea[1] = len;

      len--;
      n = 1;
      /* hightman.060504: 判斷現在刪除的位置是否為漢字的前半段，若是刪二字元 */
      if((cuser.ufo & UFO_ZHC) && cx < len && IS_BIG5_LOW(data, cx + 1))
      {
	len--;
	n++;
      }

      for(i = cx; i < len; i++)
	data[i] = data[i + n];

      InvalidateArea[0] = cx;
    }
    else if(ch == KEY_UP || ch == Ctrl('P'))
    {
      if(!currline)
	data[len] = '\0';
      do
      {
	currline = (currline + 1) % (MAXLASTCMD + 1);
      }while(currline != 0 && *physical[currline] == '\0');

      goto VisualGetsScrollCommon;
    }
    else if(ch == KEY_DOWN || ch == Ctrl('N'))
    {
      if(!currline)
	data[len] = '\0';

      do
      {
	currline = (currline + MAXLASTCMD) % (MAXLASTCMD + 1);
      }while(currline != 0 && *physical[currline] == '\0');

VisualGetsScrollCommon:
      InvalidateArea[0] = 0;
      InvalidateArea[1] = len;
      len = strlen(physical[currline]);
      if(len > InvalidateArea[1])
	InvalidateArea[1] = len;
      if(len > max)
      {
	len = max;
	InvalidateArea[1] = max;
      }
      InvalidateArea[1]--;
      cx = len;
    }
    else if((ch == Ctrl('R') || ch == Ctrl('T')) && bbsmode == M_BMW_REPLY)
    {
      if (bmw_reply_CtrlRT(ch))
      {
	if(currline)
	  PASTE_BACK();
	data[len] = '\0';
	return ch;
      }
    }
    else if(ch == KEY_HOME || ch == Ctrl('A'))
    {
      cx = 0;
    }
    else if(ch == KEY_END || ch == Ctrl('E'))
    {
      cx = len;
    }
    else if(ch == Ctrl('C') || ch == Ctrl('Q'))
    {
      InvalidateArea[0] = 0;
      InvalidateArea[1] = len - 1;
      len = 0;
      currline = 0;	/* 清空我想就不必做回貼了吧... */
      cx = 0;
      *data = '\0';
      if(ch == Ctrl('Q')) quit = 1;
    }
    else if(ch == Ctrl('K'))
    {
      if(currline)
	PASTE_BACK();

      InvalidateArea[0] = cx;
      InvalidateArea[1] = len - 1;
      len = cx;
    }
    else if(ch == KEY_TAB)	/* 插入4格空白 */
    {
      if(currline)
	PASTE_BACK();

      n = max - len;
      if(n <= 0)
	continue;
      else if(n > 4)
	n = 4;

      for(i = len - 1; i >= cx; i--)
	data[i + n] = data[i];

      memset(data + cx, ' ', n);
      InvalidateArea[0] = cx;
      len += n;
      InvalidateArea[1] = len - 1;
      cx += n;
    }
    else if(ch == Ctrl('X'))
    {
      cuser.ufo ^= UFO_ZHC;
    }
    else if(ch == Ctrl('Z'))
    {
      screen_backup_t old_screen;

      scr_dump(&old_screen);
      grayout(0, b_lines, GRAYOUT_DARK);
      i = (b_lines - 11) / 2;

      move(i++, 3);
      outstr("┌ \033[1;32mvget按鍵一覽\033[m ────────────────────────────┐");
      move(i++, 3);
      outstr("│                                                                      │");
      move(i++, 3);
      outstr("│  \033[1;36mEnter\033[m   結束輸入                 \033[1;36mCtrl+S  \033[m重繪vget                   │");
      move(i++, 3);
      outstr("│  \033[1;36mTab \033[m    插入四格空白             \033[1;36mCtrl+R  \033[m回水球                     │");
      move(i++, 3);
      outstr("│  \033[1;36mCtrl+K\033[m  清除至行尾               \033[1;36mDelete/Ctrl+D  \033[m刪除游標所在處字元  │");
      move(i++, 3);
      outstr("│  \033[1;36mCtrl+C\033[m  清空一整行               \033[1;36m上方向鍵/Ctrl+P  \033[m向上翻閱\過去紀錄  │");
      move(i++, 3);
      outstr("│  \033[1;36mCtrl+Q\033[m  清空一整行並結束輸入     \033[1;36m下方向鍵/Ctrl+N  \033[m向下翻閱\過去紀錄  │");
      move(i++, 3);
      outstr("│  \033[1;36m左方向鍵/Ctrl+B\033[m  游標左移一格    \033[1;36mHome/Ctrl+A\033[m  游標移至開頭          │");
      move(i++, 3);
      outstr("│  \033[1;36m右方向鍵/Ctrl+F\033[m  游標右移一格    \033[1;36mEnd/Ctrl+E \033[m 游標移至結尾           │");
      move(i++, 3);
      outstr("│  \033[1;36mCtrl+X\033[m  切換全型字偵測 \033[1;30m(現在狀態:");
      if(cuser.ufo & UFO_ZHC)
	outs("\033[32m開啟\033[30m");
      else
	outs("\033[33m關閉\033[30m");
      outstr(")\033[m                              │");
      move(i++, 3);
      outstr("│                                                                      │");
      move(i, 3);
      outstr("└─────────────────────────── \033[33m按任意鍵繼續...\033[m┘");

      move(i, 73);
      vkey();

      scr_restore(&old_screen);
    }

  }	/* for(;;) */


  move(line, ox + max + 1);

  if(len >= (echo & FRCSAV? 1 : 2) && echo)
  {
    for(i = MAXLASTCMD - 1; i > 0; i--)
      strcpy(lastcmd[i], lastcmd[i - 1]);
    strcpy(lastcmd[0], data);
  }

  ch = data[0];
  if ((echo & LCECHO) && (ch >= 'A' && ch <= 'Z'))
    data[0] = (ch |= 0x20);

  if(bottom_mode)
    vget_BottomMode = 0;

  return ch;
}



int
vans(prompt)
  char *prompt;
{
  char ans[3];

  /* itoc.010812.註解: 會自動換成小寫的 */
  return vget(b_lines, 0, prompt, ans, sizeof(ans), LCECHO);
}



int
krans(row, def, desc, prompt)
  int row;	/* 顯示的位置 */
  char *def;	/* def[0]:一開始停在哪(要是小寫)  def[1]:按Ctrl+Q跳出時回傳的值 */
  char *desc[];	/* 選項。每個選項第一個字為hotkey，會自動變大寫。最後一項必須為NULL */
  char *prompt;	/* 顯示在最前面的字串，沒有想顯示的字的話可以為NULL */
{
  int cc, oc, ch, i, n;
  int *col;

  move(row, 0);
  clrtoeol();
  if(prompt)
  {
    outs(prompt);
    getyx(NULL, &oc);
  }
  else
    oc = 0;

  cc = 0;
  ch = oc;
  for(i = 0; desc[i]; i++)
  {
    ch += strlen(desc[i]) + 4;
    if(ch > b_cols)
      break;

    prints(" (%c)%s ", toupper(desc[i][0]), desc[i] + 1);
    if(tolower(desc[i][0]) == def[0])
      cc = i;
  }

  n = i;
  col = (int*) malloc(sizeof(int) * n);
  if(!col)
  {
    vmsg("krans(): Memory allocation failed.");
    abort_bbs();
  }

  col[0] = oc;
  for(i = 1; i < n; i++)
    col[i] = col[i - 1] + strlen(desc[i - 1]) + 4;

  oc = -1;
  for(;;)	/* main loop */
  {
    if(oc != cc)
    {
      if(oc >= 0)
      {
	move(row, col[oc]);
	prints(" (%c)%s ", toupper(desc[oc][0]), desc[oc] + 1);
      }

      move(row, col[cc]);
      attrset(0x4F);
      prints(" [%c]%s ", toupper(desc[cc][0]), desc[cc] + 1);
      attrset(0x07);
      oc = cc;
    }

    ch = vkey();
    if(ch >= 'A' && ch <= 'Z')
    {
      ch |= 0x20;	/* 轉小寫 */
    }

    if(ch == KEY_LEFT)
    {
      cc = (cc + n - 1) % n;
    }
    else if(ch == KEY_RIGHT)
    {
      cc = (cc + 1) % n;
    }
    else if(ch == KEY_HOME || ch == Ctrl('A'))
    {
      cc = 0;
    }
    else if(ch == KEY_END || ch == Ctrl('E'))
    {
      cc = n - 1;
    }
    else if(ch == Ctrl('Q'))
    {
      ch = def[1];	/* return default value(def[1]) */
      break;
    }
    else if(ch == '\n')
    {
      ch = tolower(desc[cc][0]);
      break;
    }
    else if(ch == Ctrl('S'))	/* redraw whole line */
    {
      move(row, 0);
      clrtoeol();

      if(prompt) outs(prompt);
      for(i = 0; i < n; i++)
      {
	if(cc == i)
	  prints("\033[1;44m [%c]%s \033[m", toupper(desc[cc][0]), desc[cc] + 1);
	else
	  prints(" (%c)%s ", toupper(desc[i][0]), desc[i] + 1);
      }
    }
    else if(ch == I_OTHERDATA)	/* special design for BWboard */
    {
      int discard;
      recv(vio_fd, &discard, 1, 0);

      ch = def[1];
      break;
    }
    else
    {
      for(i = 0; i < n; i++)	/* search match option */
      {
	if(ch == tolower(desc[i][0]))
	{
	  cc = i;
	  break;
	}
      }
    }
  }	/* for(;;) */

  free(col);
  return tolower(ch);
}

