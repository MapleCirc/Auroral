/*-------------------------------------------------------*/
/* more.c						 */
/*-------------------------------------------------------*/
/* target : simple & beautiful ANSI/Chinese browser	 */
/* create : 95/03/29				 	 */
/* update : 10/09/25				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


/* ----------------------------------------------------- */
/* buffered file read					 */
/* ----------------------------------------------------- */


#define MORE_BUFSIZE	4096


#if 0
static int more_width;	/* more screen 的寬度 */
#endif

static uschar more_pool[MORE_BUFSIZE];
static int more_base;		/* more_pool[more_base ~ more_base+more_size] 有值 */
static int more_size;


/* ----------------------------------------------------- */
/* mget 閱讀文字檔；mread 閱讀二進位檔			 */
/* ----------------------------------------------------- */


/* itoc.041226.註解: mgets() 和 more_line() 不一樣的部分有
   1. mgets 直接用 more_pool 的空間；more_line 則是會把值寫入一塊 buffer
   2. mgets 不會自動斷行；more_line 則是會自動斷行在 more_width
   所以 mgets 是拿用在一些系統檔處理或是 edit.c，而 more_line 只用在 more()
 */

char *
mgets(fd)
  int fd;
{
  char *pool, *base, *head, *tail;
  int ch;

  if (fd < 0)
  {
    more_base = more_size = 0;
    return NULL;
  }

  pool = more_pool;
  base = pool + more_base;
  tail = pool + more_size;
  head = base;

  for (;;)
  {
    if (head >= tail)
    {
      if (ch = head - base)
	memcpy(pool, base, ch);

      head = pool + ch;
      ch = read(fd, head, MORE_BUFSIZE - ch);

      if (ch <= 0)
	return NULL;

      base = pool;
      tail = head + ch;
      more_size = tail - pool;
    }

    ch = *head;

    if (ch == '\n')
    {
      *head++ = '\0';
      more_base = head - pool;
      return base;
    }

    head++;
  }
}



/* use mgets(-1) to reset */

void *
mread(fd, len)
  int fd, len;
{
  char *pool;
  int base, size;

  base = more_base;
  size = more_size;
  pool = more_pool;

  if (size < len)
  {
    if (size)
      memcpy(pool, pool + base, size);

    base = read(fd, pool + size, MORE_BUFSIZE - size);

    if (base <= 0)
      return NULL;

    size += base;
    base = 0;
  }

  more_base = base + len;
  more_size = size - len;

  return pool + base;
}



/* dust.100925: 由於在pmore第四回適應性修正中拿掉了more_line()，
  因此需要一個新的顯示核心取代原本show_postlaw()的演算法 */

#define FTCMD_MAXLEN	(63)	/* 這數值要跟pfterm.c裡面所定義的一樣 */
#define ANSI_IS_PARAM(c) (c == ';' || (c >= '0' && c <= '9'))	/* from pfterm.c */
#define CAT_BUFFERSIZE	2048	/* 一次讀2KB就好，不需要塞滿more_pool */


int	/* 0:正常秀出   -1:檔案開啟失敗 */
lim_cat(fpath, lines)
  char *fpath;
  int lines;	/* 最多印出幾行，範圍 [1, b_lines] */
{
  int fd, end_of_file = 0;

  fd = open(fpath, O_RDONLY);
  if(fd < 0) return -1;

  if(lines > b_lines) lines = b_lines - 1;

  more_base = more_size = 0;
  do
  {
    uschar ch, buf[64], queue[32], esbuf[ESCODE_MAXLEN + 1];
    int qsize = 0, qhead = 0, in_seq = 0, ftcmd_len = 0;
    int no_output = 0, col = 0, newline = 0;

    while(1)
    {
      if(qsize > qhead)
	ch = queue[qhead++];
      else if(!newline)
      {
	if(more_base >= more_size)
	{
	  more_base = 0;
	  if((more_size = read(fd, more_pool, CAT_BUFFERSIZE)) <= 0)
	  {
	    newline = end_of_file = 1;
	    continue;
	  }
	}

	ch = more_pool[more_base++];

	if(ch == '\n')
	{
	  newline = 1;
	  continue;
	}
      }
      else if(in_seq >= 2)
	goto flush_EScode;
      else
	break;


      if(ftcmd_len >= 2)
      {
	outc(ch);

	if(ANSI_IS_PARAM(ch) && ftcmd_len < FTCMD_MAXLEN - 1)
	  ftcmd_len++;
	else
	  ftcmd_len = 0;
      }
      else if(in_seq == 1)
      {
	if(ch == '*')
	{
	  esbuf[in_seq++] = ch;
	  ftcmd_len = 0;
	}
	else
	{
	  in_seq = 0;
	  outc(KEY_ESC);
	  outc(ch);
	  if(ch == '[')
	    ftcmd_len = 2;
	  else
	    ftcmd_len = 0;
	}
      }
      else if(in_seq)
      {
	esbuf[in_seq++] = ch;

	if(in_seq >= sizeof(esbuf) - 1)
	{
	  int len, submsg;

flush_EScode:
	  esbuf[in_seq] = '\0';

	  len = EScode(buf, esbuf, sizeof(buf), &submsg);

	  if(submsg)	/* 已知的控碼 */
	  {
	    int i, phylen = strlen(buf);

	    qsize = in_seq - len;
	    if(qsize > 0)
	      memcpy(queue, esbuf + len, qsize);
	    qhead = 0;

	    if(!no_output)
	    {
	      if(col + phylen > b_cols)
		buf[b_cols - col + 1] = '\0';

	      for(i = 0; i < phylen; i++)
	      {
		if(col > b_cols)
		  break;
	      }

	      if(submsg < 0) attrset(10 + submsg);
	      outs(buf);
	      if(submsg < 0) attrset(7);
	    }

	  }
	  else	/* 未知的控碼，全部扔回queue裡 */
	  {
	    qsize = in_seq - 1;
	    memcpy(queue, esbuf + 1, qsize);
	    qhead = 0;
	  }

	  in_seq = 0;
	}
      }
      else if(ch == KEY_ESC)
      {
	esbuf[0] = KEY_ESC;
	in_seq = 1;
	ftcmd_len = 1;
      }
      else if(!no_output)
      {
	outc(ch);
	col++;
      }

      if(col > b_cols)
	no_output = 1;
    }

    if(end_of_file)
      break;

    if(!no_output)
      outc('\n');

  }while(--lines > 0);

  close(fd);
  pfterm_resetcmd();
  attrset(7);
  return 0;
}



/* ----------------------------------------------------- */
/* more 閱讀文字檔					 */
/* ----------------------------------------------------- */

int	/* -1: 檔案無法開啟  0:普通地結束  1:其他鍵跳出，需要處理按鍵 */
more(fpath, ft_style)
  char *fpath;
  enum MFST ft_style;	/* dust.100925: [pmore第四回適應性修正]以enum MFST取代舊的char* */
{
  int cmd, has_foot;

  if(ft_style == MFST_NONE || ft_style == MFST_NULL)
    has_foot = 0;
  else
  {
    switch(ft_style)
    {
    case MFST_MINESWEEPER:
      pmore_setfooterstyle(MFST_MINESWEEPER);
      break;

    case MFST_NORMAL:
      pmore_setfooterstyle(MFST_NORMAL);
      break;

    default:
      pmore_setfooterstyle(MFST_AUTO);
    }

    has_foot = 1;
  }

  cmd = pmore(fpath, has_foot);

  if(cmd == 0 && ft_style == MFST_NULL)
  {
    move(b_lines, 0);
    clrtoeol();

    if(vmsg(NULL) == 'C')       /* MFST_NULL 時可以使用TBF */
    {
      FILE *fp;

      if (fp = tbf_open())
      {
	f_suck(fp, fpath);
	fclose(fp);
      }
    }
  }

  return cmd;
}



#if 0

#define	STR_ANSICODE	"[0123456789;"


static uschar *fimage;		/* file image begin */
static uschar *fend;		/* file image end */
static uschar *foff;		/* 目前讀到哪裡 */

static int
more_line(buf)
  char *buf;
{
  int ch, len, bytes, in_ansi, in_chi;

  len = bytes = in_ansi = in_chi = 0;

  for (;;)
  {
    if (foff >= fend)
      break;

    ch = *foff;

    /* weiyu.040802: 如果這碼是中文字的首碼，但是只剩下一碼的空間可以印，那麼不要印這碼 */
    if (in_chi || IS_ZHC_HI(ch))
      in_chi ^= 1;
#ifndef M3_USE_PFTERM
    if (in_chi && (len >= more_width - 1 || bytes >= ANSILINELEN - 2))
#else
    if (in_chi && len >= more_width - 1)
#endif
      break;

    foff++;
    bytes++;

    if (ch == '\n')
      break;

    if (ch == KEY_ESC)
    {
      in_ansi = 1;
    }
    else if (in_ansi)
    {
      if (!strchr(STR_ANSICODE, ch))
	in_ansi = 0;
    }
    else if (isprint2(ch))
    {
      len++;
    }
    else
    {
      ch = ' ';		/* 印出不來的都換成空白 */
      len++;
    }

    *buf++ = ch;

    /* 若不含控制碼的長度已達 more_width 字，或含控制碼的長度已達 ANSILINELEN-1，那麼離開迴圈 */
#ifndef M3_USE_PFTERM
    if (len >= more_width || bytes >= ANSILINELEN - 1)
#else
    if (len >= more_width)
#endif
    {
      /* itoc.031123: 如果是控制碼，即使不含控制碼的長度已達 more_width 了，還可以繼續吃 */
      if ((in_ansi || (foff < fend && *foff == KEY_ESC)) && bytes < ANSILINELEN - 1)
	continue;

      /* itoc.031123: 再檢查下一個字是不是 '\n'，避免恰好是 more_width 或 ANSILINELEN-1 時，會多跳一列空白 */
      if (foff < fend && *foff == '\n')
      {
	foff++;
	bytes++;
      }
      break;
    }
  }

  *buf = '\0';

  return bytes;
}


static void
outs_line(str)			/* 印出一般內容 */
  char *str;
{
  int ch1, ch2, ansi;

  /* ※處理引用者 & 引言 */

  ch1 = str[0];
  ch2 = str[1];

  if (ch2 == ' ' && (ch1 == QUOTE_CHAR1 || ch1 == QUOTE_CHAR2))	/* 引言 */
  {
    ansi = 1;
    ch1 = str[2];
    outs((ch1 == QUOTE_CHAR1 || ch1 == QUOTE_CHAR2) ? "\033[33m" : "\033[36m");	/* 引用一層/二層不同顏色 */
  }
  else if (ch1 == '\241' && ch2 == '\260')	/* ※ 引言者 */
  {
    ansi = 1;
    outs("\033[1;36m");
  }
  else
    ansi = 0;

  /* 印出內容 */

  if (!hunt[0])
  {
    outx(str);
  }
  else
  {
    int len;
    char buf[ANSILINELEN];
    char *ptr1, *ptr2;

    len = strlen(hunt);
    ptr2 = buf;
    while (1)
    {
      if (!(ptr1 = str_sub(str, hunt)))
      {
	str_ncpy(ptr2, str, buf + ANSILINELEN - ptr2 - 1);
	break;
      }

      if (buf + ANSILINELEN - 1 <= ptr2 + (ptr1 - str) + (len + 7))	/* buf 空間不夠 */
	break;

      str_ncpy(ptr2, str, ptr1 - str + 1);
      ptr2 += ptr1 - str;

#if 0
 /* floatJ: 改搜尋反白顏色, 記得ptr2也要跟著改 */
      sprintf(ptr2, "\033[1;33;5;45m%.*s\033[m", len, ptr1);
      ptr2 += len + 15;
#else
      sprintf(ptr2, "\033[7m%.*s\033[m", len, ptr1);
      ptr2 += len + 7;
#endif

      str = ptr1 + len;
    }

    outx(buf);
  }

  if (ansi)
    outs(str_ransi);
}


static void
outs_header(str, header_len)	/* 印出檔頭 */
  char *str;
  int header_len;
{
  static char header1[LINE_HEADER][LEN_AUTHOR1] = {"作者",   "標題",   "時間"};
  static char header2[LINE_HEADER][LEN_AUTHOR2] = {"發信人", "標  題", "發信站"};
  int i;
  char *ptr, *word;

  /* 處理檔頭 */

  if ((header_len == LEN_AUTHOR1 && !memcmp(str, header1[0], LEN_AUTHOR1 - 1)) ||
    (header_len == LEN_AUTHOR2 && !memcmp(str, header2[0], LEN_AUTHOR2 - 1)))
  {
    /* 作者/看板 檔頭有二欄，特別處理 */
    word = str + header_len;
    if ((ptr = strstr(word, str_post1)) || (ptr = strstr(word, str_post2)))
    {
      ptr[-1] = ptr[4] = '\0';
      prints(COLOR5 " %s " COLOR6 "%-*.*s" COLOR5 " %s " COLOR6 "%-13s\033[m", 
	header1[0], d_cols + 53, d_cols + 53, word, ptr, ptr + 5);
    }
    else
    {
      /* 少看板這欄 */
      prints(COLOR5 " %s " COLOR6 "%-*.*s\033[m", 
	header1[0], d_cols + 72, d_cols + 72, word);
    }
    return;
  }

  for (i = 1; i < LINE_HEADER; i++)
  {
    if ((header_len == LEN_AUTHOR1 && !memcmp(str, header1[i], LEN_AUTHOR1 - 1)) ||
      (header_len == LEN_AUTHOR2 && !memcmp(str, header2[i], LEN_AUTHOR2 - 1)))
    {
      /* 其他檔頭都只有一欄 */
      word = str + header_len;
      prints(COLOR5 " %s " COLOR6 "%-*.*s\033[m", 
	header1[i], d_cols + 72, d_cols + 72, word);
      return;
    }
  }

  /* 如果不是檔頭，就當一般文字印出 */
  outs_line(str);
}


static inline void
outs_footer(buf, lino, fsize)
  char *buf;
  int lino;
  int fsize;
{
  int i;

  /* P.1 有 (PAGE_SCROLL + 1) 列，其他 Page 都是 PAGE_SCROLL 列 */

  /* prints(FOOTER_MORE, (lino - 2) / PAGE_SCROLL + 1, ((foff - fimage) * 100) / fsize); */

  /* itoc.010821: 為了和 FOOTER 對齊 */
  sprintf(buf, FOOTER_MORE, (lino - 2) / PAGE_SCROLL + 1, ((foff - fimage) * 100) / fsize);
  outs(buf);

  for (i = b_cols + sizeof(COLOR1) + sizeof(COLOR2) - strlen(buf); i > 3; i--)
  {
    /* 填滿最後的空白顏色，最後留一個空白 */
    outc(' ');
  }
  outs(str_ransi);
}


#ifdef SLIDE_SHOW
static int slideshow;		/* !=0: 播放 movie 的速度 */

static int
more_slideshow()
{
  int ch;

  if (!slideshow)
  {
    ch = vkey();

    if (ch == '@')
    {
      slideshow = vans("請選擇放映的速度 1(最慢)～9(最快)？播放中按任意鍵可停止播放：") - '0';
      if (slideshow < 1 || slideshow > 9)
	slideshow = 5;

      ch = KEY_PGDN;
    }
  }  
  else
  {
    struct timeval tv[9] =
    {
      {4, 0}, {3, 0}, {2, 0}, {1, 500000}, {1, 0},
      {0, 800000}, {0, 600000}, {0, 400000}, {0, 200000}
    };

    refresh();
    ch = 1;
    if (select(1, (fd_set *) &ch, NULL, NULL, tv + slideshow - 1) > 0)
    {
      /* 若播放中按任意鍵，則停止播放 */
      slideshow = 0;
      ch = vkey();
    }
    else
    {
      ch = KEY_PGDN;
    }
  }

  return ch;
}
#endif


void
bw_display()
{
  int i;
  screenline slt[T_LINES];
  screenline bw_slt[T_LINES];

  vs_save(slt);

  for(i=0;i<T_LINES;i++)
  {
    bw_slt[i].smod = slt[i].smod;
    bw_slt[i].emod = slt[i].emod;
    bw_slt[i].sso = slt[i].sso;
    bw_slt[i].eso = slt[i].eso;
    bw_slt[i].mode = slt[i].mode;
    bw_slt[i].oldlen = slt[i].len;
    bw_slt[i].width = slt[i].width;
    bw_slt[i].len = slt[i].width + 7;

    strcpy(bw_slt[i].data, "\033[1;30m");
    str_ansi(bw_slt[i].data + 7, slt[i].data, slt[i].width + 10);
  }

  vs_restore(bw_slt);

  move(b_lines, 85);
  outs("\033[m");

  while(vkey() == '\\');

  vs_restore(slt);
}



#define END_MASK	0x200	/* 按 KEY_END 直達最後一頁 */

#define HUNT_MASK	0x400
#define HUNT_NEXT	0x001	/* 按 n 搜尋下一筆 */
#define HUNT_FOUND	0x002	/* 按 / 開始搜尋，且已經找到 match 的字串 */
#define HUNT_START	0x004	/* 按 / 開始搜尋，且尚未找到 match 的字串 */

#define MAXBLOCK	256	/* 記錄幾個 block 的 offset。可加速 MAXBLOCK*32 列以內的長文在上捲/翻時的速度 */

/* Thor.990204: 傳回值 -1 為無法show出
                        0 為全數show完
                       >0 為未全show，中斷所按的key */
int
more(fpath, footer)
  char *fpath;
  char *footer;
{
  char buf[ANSILINELEN];
  int i;

  uschar *headend;		/* 檔頭結束 */

  int shift;			/* 還需要往下移動幾列 */
  int lino;			/* 目前 line number */
  int header_len;		/* 檔頭的長度，同時也是站內/站外信的區別 */
  int key;			/* 按鍵 */

  int fsize;			/* 檔案大小 */
  static off_t block[MAXBLOCK];	/* 每 32 列為一個 block，記錄此 block 的 offset */

  int cmd;			/* 中斷時所按的鍵 */


  if (!(fimage = f_img(fpath, &fsize)))
    return -1;

  foff = fimage;
  fend = fimage + fsize;

  /* more_width = b_cols - 1; */	/* itoc.070517.註解: 若用這個，每列最大字數會和 header 及 footer 對齊 (即會有留白二格) */
  more_width = b_cols + 1;		/* itoc.070517.註解: 若用這個，每列最大字數與螢幕同寬 */
/* dust.080310: 修改這邊時要連 show_postlaw() 一起改 */

  /* 找檔頭結束的地方 */
  for (i = 0; i < LINE_HEADER; i++)
  {
    if (!more_line(buf))
      break;

    /* 讀出檔案第一列，來判斷站內信還是站外信 */
    if (i == 0)
    {
      header_len = 
        !memcmp(buf, str_author1, LEN_AUTHOR1) ? LEN_AUTHOR1 :	/* 「作者:」表站內文章 */
        !memcmp(buf, str_author2, LEN_AUTHOR2) ? LEN_AUTHOR2 : 	/* 「發信人:」表轉信文章 */
        0;							/* 沒有檔頭 */
    }

    if (!*buf)	/* 第一次 "\n\n" 是檔頭的結尾 */
      break;
  }
  headend = foff;

  /* 歸零 */
  foff = fimage;

  lino = cmd = 0;
  block[0] = 0;

#ifdef SLIDE_SHOW
  slideshow = 0;
#endif

  if (hunt[0])		/* 在 xxxx_browse() 請求搜尋字串 */
  {
    str_lowest(hunt, hunt);
    shift = HUNT_MASK | HUNT_START;
  }
  else
  {
    shift = b_lines;
  }

  clear();

  while (more_line(buf))
  {
    /* ------------------------------------------------- */
    /* 印出一列的文字					 */
    /* ------------------------------------------------- */

    /* 首頁前幾列才需要處理檔頭 */
    if (foff <= headend)
      outs_header(buf, header_len);
    else
      outs_line(buf);

    outc('\n');

    /* ------------------------------------------------- */
    /* 依 shift 來決定動作				 */
    /* ------------------------------------------------- */

    /* itoc.030303.註解: shift 在此的意義
       >0: 還需要往下移幾列
       <0: 還需要往上移幾列
       =0: 結束這頁，等待使用者按鍵 */

    if (shift > 0)		/* 還要下移 shift 列 */
    {
      if (lino >= b_lines)	/* 只有在剛進 more，第一次印第一頁時才可能 lino <= b_lines */
	scroll();

      lino++;

      if ((lino % 32 == 0) && ((i = lino >> 5) < MAXBLOCK))
	block[i] = foff - fimage;


      if (!(shift & (HUNT_MASK | END_MASK)))	/* 一般資料讀取 */
      {
	shift--;
      }
      else if (shift & HUNT_MASK)		/* 字串搜尋 */
      {
	if (shift & HUNT_NEXT)	/* 按 n 搜尋下一筆 */
	{
	  /* 一找到就停於該列 */
	  if (str_sub(buf, hunt))
	    shift = 0;
	}
	else			/* 按 / 開始搜尋 */
	{
	  /* 若在第二頁以後找到，一找到就停於該列；
	     若在第一頁找到，必須等到讀完第一頁才能停止 */
	  if (shift & HUNT_START && str_sub(buf, hunt))
	    shift ^= HUNT_START | HUNT_FOUND;		/* 拿掉 HUNT_START 並加上 HUNT_FOUND */
	  if (shift & HUNT_FOUND && lino >= b_lines)
	    shift = 0;
	}
      }
    }
    else if (shift < 0)		/* 還要上移 -shift 列 */
    {
      shift++;

      if (!shift)
      {
	move(b_lines, 0);
	clrtoeol();

	/* 剩下 b_lines+shift 列是 rscroll，offsect 去正確位置；這裡的 i 是總共要 shift 的列數 */
	for (i += b_lines; i > 0; i--)
	  more_line(buf);
      }
    }

    if (foff >= fend)		/* 已經讀完全部的檔案 */
    {
      /* 倘若是按 End 移到最後一頁，那麼停留在 100% 而不結束；否則一律結束 */
      if (!(shift & END_MASK))
	break;
      shift = 0;
    }

    if (shift)			/* 還需要繼續讀資料 */
      continue;

    /* ------------------------------------------------- */
    /* 到此印完所需的 shift 列，接下來印出 footer 並等待 */
    /* 使用者按鍵					 */
    /* ------------------------------------------------- */

re_key:

    outs_footer(buf, lino, fsize);

#ifdef SLIDE_SHOW
    key = more_slideshow();
#else
    key = vkey();
#endif

    if (key == ' ' || key == KEY_PGDN || key == KEY_RIGHT || key == Ctrl('F'))
    {
      shift = PAGE_SCROLL;
    }

    else if (key == KEY_DOWN || key == '\n')
    {
      shift = 1;
    }

    else if (key == KEY_PGUP || key == Ctrl('B') || key == KEY_DEL)
    {
      /* itoc.010324: 到了最開始再上捲表示離開，並回傳 'k' (keymap[] 定義上一篇) */
      if (lino <= b_lines)
      {
	cmd = 'k';
	break;
      }
      /* 最多只能上捲到一開始 */
      i = b_lines - lino;
      shift = BMAX(-PAGE_SCROLL, i);
    }

    else if (key == KEY_UP)
    {
      /* itoc.010324: 到了最開始再上捲表示離開，並回傳 'k' (keymap[] 定義上一篇) */
      if (lino <= b_lines)
      {
	cmd = 'k';
	break;
      }
      shift = -1;
    }

    else if (key == KEY_END || key == '$')
    {
      shift = END_MASK;
    }

    else if (key == KEY_HOME || key == '0')
    {
      if (lino <= b_lines)	/* 已經在最開始了 */
	shift = 0;
      else
	shift = -PAGE_SCROLL - 1;
    }

    else if (key == '/' || key == 'n')
    {
      if (key == 'n' && hunt[0])	/* 如果按 n 卻未輸入過搜尋字串，那麼視同按 / */
      {
	shift = HUNT_MASK | HUNT_NEXT;
      }
      else if (vget(b_lines, 0, "搜尋：", hunt, sizeof(hunt), DOECHO))
      {
	str_lowest(hunt, hunt);
	shift = HUNT_MASK | HUNT_START;
      }
      else				/* 如果取消搜尋的話，重繪 footer 即可 */
      {
	shift = 0;
      }
    }

    else if (key == 'C')	/* Thor.980405: more 時可存入暫存檔 */
    {
      FILE *fp;
      if (fp = tbf_open())
      {
	f_suck(fp, fpath);
	fclose(fp);
      }
      shift = 0;		/* 重繪 footer */
    }

    else if (key == 'h')
    {
      screenline slt[T_LINES];

      uschar *tmp_fimage;
      uschar *tmp_fend;
      uschar *tmp_foff;
      off_t tmp_block[MAXBLOCK];
      /* dust: show_postlaw()會用到fend和foff。所以原本的要記錄下來。 */

      tmp_fimage = fimage;
      tmp_fend = fend;
      tmp_foff = foff;
      memcpy(tmp_block, block, sizeof(tmp_block));

      scr_dump(&old_screen);

      show_postlaw("etc/help/post/more.hlp", b_lines - 1);
      vmsg(NULL);

      vs_restore(slt);

      fimage = tmp_fimage;
      fend = tmp_fend;
      foff = tmp_foff;
      memcpy(block, tmp_block, sizeof(block));

      shift = 0;
    }

    else if (key == '\\')
    { 
      bw_display();
    }

    else		/* 其他鍵都是使用者中斷 */
    {
      /* itoc.041006: 使用者中斷的按鍵要 > 0 (而 KEY_LEFT 是 < 0) */
      cmd = key > 0 ? key : 'q';
      break;
    }

    /* ------------------------------------------------- */
    /* 使用者已按鍵，若 break 則離開迴圈；否則依照 shift */
    /* 的種類 (亦即按鍵的種類) 而做不同的動作		 */
    /* ------------------------------------------------- */

    if (shift > 0)			/* 準備下移 shift 列 */
    {
      if (shift < (HUNT_MASK | HUNT_START))	/* 一般下移 */
      {
	/* itoc.041114.註解: 目標是秀出 lino-b_lines+1+shift ~ lino+shift 列的內容：
	   就只要清 footer 即可，其他的就交給前面循序印 shift 列的程式 */

	move(b_lines, 0);
	clrtoeol();

#if 1
	/* itoc.041116: End 的作法其實和一般下移可以是完全一樣的，但是如果遇到超長文章時，
	   會造成前面循序印 shift 列的程式就得一直翻，直到找到最後一頁，這樣會做太多 outs_line() 白工，
	   所以在此特別檢查超長文章時，就先去找最後一頁所在 */

	if ((shift & END_MASK) && (fend - foff >= MORE_BUFSIZE))	/* 還有一堆沒讀過，才特別處理 */
	{
	  int totallino = lino;

	  /* 先讀到最後一列看看全部有幾列 */
	  while (more_line(buf))
	  {
	    totallino++;
	    if ((totallino % 32 == 0) && ((i = totallino >> 5) < MAXBLOCK))
	      block[i] = foff - fimage;
	  }

	  /* 先位移到上一個 block 的尾端 */
	  i = (totallino - b_lines) >> 5;
	  if (i >= MAXBLOCK)
	    i = MAXBLOCK - 1;
	  foff = fimage + block[i];
	  i = i << 5;

	  /* 再從上一個 block 的尾端位移到 totallino-b_lines+1 列 */
	  for (i = totallino - b_lines - i; i > 0; i--)
	    more_line(buf);

	  lino = totallino - b_lines;
	}
#endif
      }
      else
      {
	/* '/' 從頭開始搜尋 */
	lino = 0;
	foff = fimage;
	clear();
      }
    }
    else if (shift < 0)			/* 準備上移 -shift 列 */
    {
      if (shift >= -PAGE_SCROLL)	/* 上捲數列 */
      {
	lino += shift;

	/* itoc.041114.註解: 目標是秀出 lino-b_lines+1 ~ lino 列的內容：
	  1. 先從頭位移到 lino-b_lines+1 列
	  2. 其中有 b_lines+shift 列是不變的內容，用 rscroll 達成
	  3. 在前面的 outs_line() 的地方印出 -shift 列
	  4. 最後再位移剛才 rscroll 的列數
	*/

	/* 先位移到上一個 block 的尾端 */
	i = (lino - b_lines) >> 5;
	if (i >= MAXBLOCK)
	  i = MAXBLOCK - 1;
	foff = fimage + block[i];
	i = i << 5;

	/* 再從上一個 block 的尾端位移到 lino-b_lines+1 列 */
	for (i = lino - b_lines - i; i > 0; i--)
	  more_line(buf);

	for (i = shift; i < 0; i++) 
	{
	  rscroll();
	  move(0, 0);
	  clrtoeol();
	}

	i = shift;
      }
      else			/* Home */
      {
	/* itoc.041226.註解: 目標是秀出 1 ~ b_lines 列的內容：
           作法就是全部都歸零，從頭再印 b_lines 列即可 */

	clear();

	foff = fimage;
	lino = 0;
	shift = b_lines;
      }
    }
    else				/* 重繪 footer 並 re-key */
    {
      move(b_lines, 0);
      clrtoeol();
      goto re_key;
    }
  }	/* while(more_line()) 迴圈的結束 */

  /* --------------------------------------------------- */
  /* 檔案已經秀完 (cmd = 0) 或 使用者中斷 (cmd != 0)	 */
  /* --------------------------------------------------- */

  free(fimage);

  if (!cmd)	/* 檔案正常秀完，要處理 footer */
  {
    if (footer)		/* 有 footer */
    {
      if (footer != (char *) -1)
	outf(footer);
      else
	outs(str_ransi);
    }
    else		/* 沒有 footer 要 vmsg() */
    {
      /* lkchu.981201: 先清一次以免重疊顯示 */
      move(b_lines, 0);
      clrtoeol();

      if (vmsg(NULL) == 'C')	/* Thor.990204: 特別注意若回傳 'C' 表示暫存檔 */
      {
	FILE *fp;

	if (fp = tbf_open()) 
	{
	  f_suck(fp, fpath); 
	  fclose(fp);
	}
      }
    }
  }
  else		/* 使用者中斷，直接離開 */
  {
    outs(str_ransi);
  }

  hunt[0] = '\0';

  /* Thor.990204: 讓key可回傳至more外 */
  return cmd;
}

#endif
