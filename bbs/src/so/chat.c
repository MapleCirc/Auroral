/*-------------------------------------------------------*/
/* chat.c	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : chat client for xchatd			 */
/* create : 95/03/29					 */
/* update : 10/11/25					 */
/*-------------------------------------------------------*/


#include "bbs.h"


#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


static char chatroom[IDLEN];	/* Chat-Room Name */
static int chatline;		/* Where to display message now */
static char chatopic[48];
static FILE *frec;


#define	stop_line	(b_lines - 4)


extern char *bmode();


static void
chat_topic()
{
  move(0, 0);
  prints("\033[1;37;46m %s：%-14s\033[45m 話題：%-48s\033[m",
    (frec ? "(錄音)" : "聊天室"), chatroom, chatopic);
}


static void
printchatline(msg)
  char *msg;
{
  if (frec)
    fprintf(frec, "%s\n", msg);

  if (chatline >= stop_line)
  {
    region_scroll_up(2, stop_line);
  }
  else
    chatline++;

  move(chatline, 0);
  outs(msg);
  clrtoeol();
}


static void
chat_record()
{
  FILE *fp;
  time_t now;
  char buf[80];

  time(&now);

  if (fp = frec)
  {
    fprintf(fp, "%s\n結束：%s\n", msg_seperator, Btime(&now));
    fclose(fp);
    frec = NULL;
    printchatline("◆ 錄音完畢！");
  }
  else
  {
#ifdef EVERY_Z
    /* Thor.980602: 由於 tbf_ask() 需問檔名，此時會用到 igetch()，
       為了防止 I_OTHERDATA 造成當住，在此用 every_Z() 的方式，
       先保存 vio_fd，待問完後再還原 */

    vio_save();		/* Thor.980602: 暫存 vio_fd */
#endif

    char *ftbf = tbf_ask();
    if (ftbf == NULL)
    {
#ifdef EVERY_Z
      vio_restore();	/* Thor.980602: 還原 vio_fd */
#endif
      move(b_lines, 0);
      clrtoeol();
      return ;
    }

    usr_fpath(buf, cuser.userid, ftbf);

#ifdef EVERY_Z
    vio_restore();	/* Thor.980602: 還原 vio_fd */
#endif

    move(b_lines, 0);
    clrtoeol();

    fp = fopen(buf, "a");
    if (fp)
    {
      fprintf(fp, "主題: %s\n包廂: %s\n錄音: %s (%s)\n開始: %s\n%s\n",
	chatopic, chatroom, cuser.userid, cuser.username,
	Btime(&now), msg_seperator);
      printchatline("◆ 開始錄音囉！");
      frec = fp;
    }
    else
    {
      printchatline("◆ 錄音機啟動失敗......");
    }
  }

  bell();
  chat_topic();
}


static void
chat_clear()
{
  clrregion(2, stop_line);
  chatline = 1;
}


static void
print_chatid(chatid)
  char *chatid;
{
  move(b_lines - 1, 0);
  prints("%-8s>", chatid);
}


static inline int
chat_send(fd, buf)
  int fd;
  char *buf;
{
  int len;

  len = strlen(buf);
  return (send(fd, buf, len, 0) == len);
}


static inline int
chat_recv(fd, chatid)
  int fd;
  char *chatid;
{
  static char buf[512];
  static int bufstart = 0;
  int cc, len;
  char *bptr, *str;

  bptr = buf;
  cc = bufstart;
  len = sizeof(buf) - cc - 1;
  if ((len = recv(fd, bptr + cc, len, 0)) <= 0)
    return -1;
  cc += len;

  for (;;)
  {
    len = strlen(bptr);

    if (len >= cc)
    {				/* wait for trailing data */
      memcpy(buf, bptr, len);
      bufstart = len;
      break;
    }
    if (*bptr == '/')
    {
      str = bptr + 1;
      fd = *str++;

      if (fd == 'c')
      {
	chat_clear();
      }
      else if (fd == 'n')
      {
	str_ncpy(chatid, str, 9);
         
	/* Thor.980819: 順便換一下 mateid 好了... */
	str_ncpy(cutmp->mateid, str, sizeof(cutmp->mateid));

	print_chatid(chatid);
	clrtoeol();
      }
      else if (fd == 'r')
      {
	str_ncpy(chatroom, str, sizeof(chatroom));
	chat_topic();
      }
      else if (fd == 't')
      {
	str_ncpy(chatopic, str, sizeof(chatopic));
	chat_topic();
      }
    }
    else
    {
      printchatline(bptr);
    }

    cc -= ++len;
    if (cc <= 0)
    {
      bufstart = 0;
      break;
    }
    bptr += len;
  }

  return 0;
}


static void
chat_pager()
{
  char buf[64];

  cuser.ufo ^= UFO_PAGER;
  cutmp->ufo ^= UFO_PAGER;

  sprintf(buf, "◆ 呼叫器已%s", cuser.ufo & UFO_PAGER ? "關閉" : "打開");

  printchatline(buf);
}


struct chat_command
{
  char *cmdname;		/* Char-room command length */
  void (*cmdfunc) ();		/* Pointer to function */
};


struct chat_command chat_cmdtbl[] = 
{
  {"pager", chat_pager},
  {"tape", chat_record},
  {NULL, NULL}
};



static void
chat_drawfoot(is_biff)
{
  move(b_lines, 0);
  if(is_biff)
    outs(COLOR1 " 郵差來了 ");
  else
    outs(COLOR1 "【聊天室】");

  outs(COLOR2 " (^C)清除一整行  (^K)清除至行尾  (^X)");

  if(cuser.ufo & UFO_ZHC)
    outs("關閉");
  else
    outs("開啟");

  outs("中文字偵測  (^D)離開聊天室 ");
}


static void
chat_drawframe(is_biff, chatid)
  int is_biff;
  char *chatid;
{
  clear();

  chat_topic();
  move(1, 0);
  outs(msg_seperator);
  move(b_lines - 2, 0);
  outs("─────────────────────────────── /h 說明  /b 離開\n");
  print_chatid(chatid);
  chat_drawfoot(is_biff);

  chatline = 1;
}



static inline int
chat_cmd_match(buf, str)
  char *buf;
  char *str;
{
  int c1, c2;

  for (;;)
  {
    c1 = *str++;
    if (!c1)
      break;

    c2 = *buf++;
    if (!c2 || c2 == ' ' || c2 == '\n')
      break;

    if (c2 >= 'A' && c2 <= 'Z')
      c2 |= 0x20;

    if (c1 != c2)
      return 0;
  }

  return 1;
}


static inline int
chat_cmd(fd, buf)
  int fd;
  char *buf;
{
  struct chat_command *cmd;
  char *key;

  buf++;
  for (cmd = chat_cmdtbl; key = cmd->cmdname; cmd++)
  {
    if (chat_cmd_match(buf, key))
    {
      cmd->cmdfunc();
      return '/';
    }
  }

  return 0;
}



#define CHAT_YPOS	10

int
t_chat()
{
  char lastcmd[MAXLASTCMD + 1][80];
  ACCT acct;
  int ch, cfd, cmdpos, cmdcol;
  char *ptr, buf[80], chatid[9];
  int old_wid = b_cols, old_len = b_lines;
  struct sockaddr_in sin;
#if     defined(__OpenBSD__)
  struct hostent *h;
#endif
  
#ifdef CHAT_SECURE
  extern char passbuf[];
#endif

#ifdef EVERY_BIFF
  int biff = HAS_STATUS(STATUS_BIFF);
#else
  int biff = 0;
#endif


#ifdef EVERY_Z
  /* Thor.980725: 為 talk & chat 可用 ^z 作準備 */
  if (vio_holdon())
  {
    vmsg("你還在跟其他人聊天耶！");
    return -1;
  }
#endif

#if     defined(__OpenBSD__)
  if (!(h = gethostbyname(str_host)))
    return -1;

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(CHAT_PORT);
  memcpy(&sin.sin_addr, h->h_addr, h->h_length);       
#else
  sin.sin_family = AF_INET;
  sin.sin_port = htons(CHAT_PORT);
  sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  memset(sin.sin_zero, 0, sizeof(sin.sin_zero));
#endif

  cfd = socket(AF_INET, SOCK_STREAM, 0);
  if (cfd < 0)
    return -1;

  if (connect(cfd, (struct sockaddr *) & sin, sizeof sin))
  {
    close(cfd);
    blog("CHAT ", "connect");
    return -1;
  }

  for (;;)
  {
    ch = vget(b_lines, 0, "請輸入聊天代號：", chatid, 9, DOECHO);
    if (ch == '/')
    {
      continue;
    }
    else if (!ch)
    {
      /* str_ncpy(chatid, cuser.userid, sizeof(chatid)); */
      close(cfd);	/* itoc.010322: 大部分都是誤按到 Talk->Chat 改成預設為離開 */
      return 0;
    }
    else
    {
      /* itoc.010528: 不可以用別人的 id 做為聊天代號 */
      if (acct_load(&acct, chatid) >= 0 && acct.userno != cuser.userno)
      {
	vmsg("這個代號有人註冊為ID，您不能當成聊天代號");
	continue;
      }
      /* Thor.980911: chatid中不可以空白, 防止 parse錯誤 */
      for(ch = 0; ch < 8; ch++)
      {
        if (chatid[ch] == ' ')
          break;
        else if (!chatid[ch]) /* Thor.980921: 如果0的話就結束 */
          ch = 8;
      }

      if (ch < 8)
      {
	vmsg("代號不可含有空白");
        continue;
      }
    }

#ifdef CHAT_SECURE	/* Thor.980729: secured chat room */
    sprintf(buf, "/! %s %s %s\n", cuser.userid, chatid, passbuf);
#else
    sprintf(buf, "/! %d %d %s %s\n", cuser.userno, cuser.userlevel, cuser.userid, chatid);
#endif

    chat_send(cfd, buf);
    if (recv(cfd, buf, 3, 0) != 3)
      return 0;

    if (!strcmp(buf, CHAT_LOGIN_OK))
      break;
    else if (!strcmp(buf, CHAT_LOGIN_EXISTS))
      vmsg("這個代號已經有人用了");
    else if (!strcmp(buf, CHAT_LOGIN_INVALID))
      vmsg("這個代號是錯誤的，無法使用");
    else if (!strcmp(buf, CHAT_LOGIN_BOGUS))
    {				/* Thor: 禁止相同二人進入 */
      close(cfd);
      vmsg("請勿派遣「分身」進入聊天室");
      return 0;
    }
  }

  chat_drawframe(biff, chatid);

  for(ch = 0; ch <= MAXLASTCMD; ch++)
    lastcmd[ch][0] = '\0';

  ptr = lastcmd[0];
  cmdpos = cmdcol = 0;

  add_io(cfd, 60);

  strcpy(cutmp->mateid, chatid);

  for (;;)
  {
    if(old_wid != b_cols || old_len != b_lines)
    {
      chat_drawframe(biff, chatid);
      old_wid = b_cols;
      old_len = b_lines;
    }

    move(b_lines - 1, cmdcol + CHAT_YPOS);
    ch = vkey();

    if (ch == KEY_DOWN)
    {
      do
      {
        cmdpos = (cmdpos + MAXLASTCMD) % (MAXLASTCMD + 1);
      }while(cmdpos != 0 && *lastcmd[cmdpos] == '\0');

      goto ChatRoomScrollCommon;
    }
    else if (ch == KEY_UP)
    {
      do
      {
        cmdpos = (cmdpos + 1) % (MAXLASTCMD + 1);
      }while(cmdpos != 0 && *lastcmd[cmdpos] == '\0');

ChatRoomScrollCommon:
      move(b_lines - 1, CHAT_YPOS);
      outs(lastcmd[cmdpos]);
      clrtoeol();
      cmdcol = strlen(lastcmd[cmdpos]);
      continue;
    }
    else if(cmdpos > 0)
    {
      strcpy(ptr, lastcmd[cmdpos]);
      cmdpos = 0;
    }

    if (ch == I_OTHERDATA)
    {				/* incoming */
      if (chat_recv(cfd, chatid) == -1)
	break;
    }
    else if (isprint2(ch))
    {
      if (cmdcol < 68)
      {
	if (ptr[cmdcol])
	{			/* insert */
	  int i;

	  for (i = cmdcol; ptr[i] && i < 68; i++);
	  ptr[i + 1] = '\0';
	  for (; i > cmdcol; i--)
	    ptr[i] = ptr[i - 1];
	}
	else
	{			/* append */
	  ptr[cmdcol + 1] = '\0';
	}
	ptr[cmdcol] = ch;
	move(b_lines - 1, cmdcol + CHAT_YPOS);
	outs(&ptr[cmdcol++]);
      }
    }
    else if (ch == '\n')
    {
#ifdef EVERY_BIFF
      if (HAS_STATUS(STATUS_BIFF) && !biff)
	chat_drawfoot(biff = 1);
#endif
      if (ch = *ptr)
      {
	if (ch == '/')
	  ch = chat_cmd(cfd, ptr);

	for (cmdpos = MAXLASTCMD; cmdpos > 1; cmdpos--)
	  strcpy(lastcmd[cmdpos], lastcmd[cmdpos - 1]);
	strcpy(lastcmd[1], ptr);

	if (ch != '/')
	{
	  strcat(ptr, "\n");
	  if (!chat_send(cfd, ptr))
	    break;
	}
	if (*ptr == '/' && ptr[1] == 'b')
	  break;

	*ptr = '\0';
	cmdcol = 0;
	cmdpos = 0;
	move(b_lines - 1, CHAT_YPOS);
	clrtoeol();
      }
    }
    else if (ch == KEY_BKSP)
    {
      if (cmdcol)
      {
	ch = cmdcol;
	cmdcol--;
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: 判斷現在刪除的位置是否為漢字的後半段，若是刪二字元 */
	if ((cuser.ufo & UFO_ZHC) && cmdcol && IS_ZHC_LO(ptr, cmdcol))
	  cmdcol--;
#endif
	strcpy(ptr + cmdcol, ptr + ch);
	move(b_lines - 1, cmdcol + CHAT_YPOS);
	outs(ptr + cmdcol);
	clrtoeol();
      }
    }
    else if (ch == KEY_DEL)
    {
      if (ptr[cmdcol])
      {
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: 判斷現在刪除的位置是否為漢字的前半段，若是刪二字元 */
	if ((cuser.ufo & UFO_ZHC) && ptr[cmdcol + 1] && IS_ZHC_HI(ptr[cmdcol]))
	  ch = 2;
	else
#endif
	  ch = 1;
	strcpy(ptr + cmdcol, ptr + cmdcol + ch);
	move(b_lines - 1, cmdcol + CHAT_YPOS);
	outs(ptr + cmdcol);
	clrtoeol();
      }
    }
    else if (ch == KEY_LEFT)
    {
      if (cmdcol)
      {
	cmdcol--;
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: 左移時碰到漢字移雙格 */
	if ((cuser.ufo & UFO_ZHC) && cmdcol && IS_ZHC_LO(ptr, cmdcol))
	  cmdcol--;
#endif
      }
    }
    else if (ch == KEY_RIGHT)
    {
      if (ptr[cmdcol])
      {
	cmdcol++;
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: 右移時碰到漢字移雙格 */
	if ((cuser.ufo & UFO_ZHC) && ptr[cmdcol] && IS_ZHC_HI(ptr[cmdcol - 1]))
	  cmdcol++;
#endif
      }
    }
    else if (ch == KEY_HOME)
    {
      cmdcol = 0;
    }
    else if (ch == KEY_END)
    {
      cmdcol = strlen(ptr);
    }
    else if (ch == Ctrl('D'))	/* 離開聊天室 */
    {
      chat_send(cfd, "/b\n");
      break;
    }
    else if (ch == Ctrl('C'))	/* 清除一整行 */
    {
      *ptr = '\0';
      cmdcol = 0;
      move(b_lines - 1, CHAT_YPOS);
      clrtoeol();
    }
    else if (ch == Ctrl('K'))	/* 清除至行尾 */
    {
      ptr[cmdcol] = '\0';
      clrtoeol();
    }
    else if (ch == Ctrl('X'))	/* 切換中文字偵測 */
    {
      cuser.ufo ^= UFO_ZHC;
      chat_drawfoot(biff);
    }
#ifdef EVERY_Z
    else if (ch == Ctrl('Z'))	
    {
      char matid[IDLEN + 1];
      screen_backup_t old_screen;

      /* Thor.980731: 暫存 mateid, 因為出去時可能會用掉 mateid */
      strcpy(matid, cutmp->mateid);

      vio_save();	/* Thor.980727: 暫存 vio_fd */
      scr_dump(&old_screen);

      every_Z(0);

      scr_restore(&old_screen);
      vio_restore();	/* Thor.980727: 還原 vio_fd */

      /* Thor.980731: 還原 mateid */
      strcpy(cutmp->mateid, matid);
      continue;
    }
#endif
  }

  if (frec)
    chat_record();

  close(cfd);
  add_io(0, 60);
  cutmp->mateid[0] = '\0';
  return 0;
}

