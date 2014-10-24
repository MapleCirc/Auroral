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
static int more_width;	/* more screen ���e�� */
#endif

static uschar more_pool[MORE_BUFSIZE];
static int more_base;		/* more_pool[more_base ~ more_base+more_size] ���� */
static int more_size;


/* ----------------------------------------------------- */
/* mget �\Ū��r�ɡFmread �\Ū�G�i����			 */
/* ----------------------------------------------------- */


/* itoc.041226.����: mgets() �M more_line() ���@�˪�������
   1. mgets ������ more_pool ���Ŷ��Fmore_line �h�O�|��ȼg�J�@�� buffer
   2. mgets ���|�۰��_��Fmore_line �h�O�|�۰��_��b more_width
   �ҥH mgets �O���Φb�@�Ǩt���ɳB�z�άO edit.c�A�� more_line �u�Φb more()
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



/* dust.100925: �ѩ�bpmore�ĥ|�^�A���ʭץ��������Fmore_line()�A
  �]���ݭn�@�ӷs����ܮ֤ߨ��N�쥻show_postlaw()���t��k */

#define FTCMD_MAXLEN	(63)	/* �o�ƭȭn��pfterm.c�̭��ҩw�q���@�� */
#define ANSI_IS_PARAM(c) (c == ';' || (c >= '0' && c <= '9'))	/* from pfterm.c */
#define CAT_BUFFERSIZE	2048	/* �@��Ū2KB�N�n�A���ݭn�뺡more_pool */


int	/* 0:���`�q�X   -1:�ɮ׶}�ҥ��� */
lim_cat(fpath, lines)
  char *fpath;
  int lines;	/* �̦h�L�X�X��A�d�� [1, b_lines] */
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

	  if(submsg)	/* �w�������X */
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
	  else	/* ���������X�A�������^queue�� */
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
/* more �\Ū��r��					 */
/* ----------------------------------------------------- */

int	/* -1: �ɮ׵L�k�}��  0:���q�a����  1:��L����X�A�ݭn�B�z���� */
more(fpath, ft_style)
  char *fpath;
  enum MFST ft_style;	/* dust.100925: [pmore�ĥ|�^�A���ʭץ�]�Henum MFST���N�ª�char* */
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

    if(vmsg(NULL) == 'C')       /* MFST_NULL �ɥi�H�ϥ�TBF */
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
static uschar *foff;		/* �ثeŪ����� */

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

    /* weiyu.040802: �p�G�o�X�O����r�����X�A���O�u�ѤU�@�X���Ŷ��i�H�L�A���򤣭n�L�o�X */
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
      ch = ' ';		/* �L�X���Ӫ��������ť� */
      len++;
    }

    *buf++ = ch;

    /* �Y���t����X�����פw�F more_width �r�A�Χt����X�����פw�F ANSILINELEN-1�A�������}�j�� */
#ifndef M3_USE_PFTERM
    if (len >= more_width || bytes >= ANSILINELEN - 1)
#else
    if (len >= more_width)
#endif
    {
      /* itoc.031123: �p�G�O����X�A�Y�Ϥ��t����X�����פw�F more_width �F�A�٥i�H�~��Y */
      if ((in_ansi || (foff < fend && *foff == KEY_ESC)) && bytes < ANSILINELEN - 1)
	continue;

      /* itoc.031123: �A�ˬd�U�@�Ӧr�O���O '\n'�A�קK��n�O more_width �� ANSILINELEN-1 �ɡA�|�h���@�C�ť� */
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
outs_line(str)			/* �L�X�@�뤺�e */
  char *str;
{
  int ch1, ch2, ansi;

  /* ���B�z�ޥΪ� & �ި� */

  ch1 = str[0];
  ch2 = str[1];

  if (ch2 == ' ' && (ch1 == QUOTE_CHAR1 || ch1 == QUOTE_CHAR2))	/* �ި� */
  {
    ansi = 1;
    ch1 = str[2];
    outs((ch1 == QUOTE_CHAR1 || ch1 == QUOTE_CHAR2) ? "\033[33m" : "\033[36m");	/* �ޥΤ@�h/�G�h���P�C�� */
  }
  else if (ch1 == '\241' && ch2 == '\260')	/* �� �ި��� */
  {
    ansi = 1;
    outs("\033[1;36m");
  }
  else
    ansi = 0;

  /* �L�X���e */

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

      if (buf + ANSILINELEN - 1 <= ptr2 + (ptr1 - str) + (len + 7))	/* buf �Ŷ����� */
	break;

      str_ncpy(ptr2, str, ptr1 - str + 1);
      ptr2 += ptr1 - str;

#if 0
 /* floatJ: ��j�M�ϥ��C��, �O�optr2�]�n��ۧ� */
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
outs_header(str, header_len)	/* �L�X���Y */
  char *str;
  int header_len;
{
  static char header1[LINE_HEADER][LEN_AUTHOR1] = {"�@��",   "���D",   "�ɶ�"};
  static char header2[LINE_HEADER][LEN_AUTHOR2] = {"�o�H�H", "��  �D", "�o�H��"};
  int i;
  char *ptr, *word;

  /* �B�z���Y */

  if ((header_len == LEN_AUTHOR1 && !memcmp(str, header1[0], LEN_AUTHOR1 - 1)) ||
    (header_len == LEN_AUTHOR2 && !memcmp(str, header2[0], LEN_AUTHOR2 - 1)))
  {
    /* �@��/�ݪO ���Y���G��A�S�O�B�z */
    word = str + header_len;
    if ((ptr = strstr(word, str_post1)) || (ptr = strstr(word, str_post2)))
    {
      ptr[-1] = ptr[4] = '\0';
      prints(COLOR5 " %s " COLOR6 "%-*.*s" COLOR5 " %s " COLOR6 "%-13s\033[m", 
	header1[0], d_cols + 53, d_cols + 53, word, ptr, ptr + 5);
    }
    else
    {
      /* �֬ݪO�o�� */
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
      /* ��L���Y���u���@�� */
      word = str + header_len;
      prints(COLOR5 " %s " COLOR6 "%-*.*s\033[m", 
	header1[i], d_cols + 72, d_cols + 72, word);
      return;
    }
  }

  /* �p�G���O���Y�A�N��@���r�L�X */
  outs_line(str);
}


static inline void
outs_footer(buf, lino, fsize)
  char *buf;
  int lino;
  int fsize;
{
  int i;

  /* P.1 �� (PAGE_SCROLL + 1) �C�A��L Page ���O PAGE_SCROLL �C */

  /* prints(FOOTER_MORE, (lino - 2) / PAGE_SCROLL + 1, ((foff - fimage) * 100) / fsize); */

  /* itoc.010821: ���F�M FOOTER ��� */
  sprintf(buf, FOOTER_MORE, (lino - 2) / PAGE_SCROLL + 1, ((foff - fimage) * 100) / fsize);
  outs(buf);

  for (i = b_cols + sizeof(COLOR1) + sizeof(COLOR2) - strlen(buf); i > 3; i--)
  {
    /* �񺡳̫᪺�ť��C��A�̫�d�@�Ӫť� */
    outc(' ');
  }
  outs(str_ransi);
}


#ifdef SLIDE_SHOW
static int slideshow;		/* !=0: ���� movie ���t�� */

static int
more_slideshow()
{
  int ch;

  if (!slideshow)
  {
    ch = vkey();

    if (ch == '@')
    {
      slideshow = vans("�п�ܩ�M���t�� 1(�̺C)��9(�̧�)�H���񤤫����N��i�����G") - '0';
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
      /* �Y���񤤫����N��A�h����� */
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



#define END_MASK	0x200	/* �� KEY_END ���F�̫�@�� */

#define HUNT_MASK	0x400
#define HUNT_NEXT	0x001	/* �� n �j�M�U�@�� */
#define HUNT_FOUND	0x002	/* �� / �}�l�j�M�A�B�w�g��� match ���r�� */
#define HUNT_START	0x004	/* �� / �}�l�j�M�A�B�|����� match ���r�� */

#define MAXBLOCK	256	/* �O���X�� block �� offset�C�i�[�t MAXBLOCK*32 �C�H��������b�W��/½�ɪ��t�� */

/* Thor.990204: �Ǧ^�� -1 ���L�kshow�X
                        0 ������show��
                       >0 ������show�A���_�ҫ���key */
int
more(fpath, footer)
  char *fpath;
  char *footer;
{
  char buf[ANSILINELEN];
  int i;

  uschar *headend;		/* ���Y���� */

  int shift;			/* �ٻݭn���U���ʴX�C */
  int lino;			/* �ثe line number */
  int header_len;		/* ���Y�����סA�P�ɤ]�O����/���~�H���ϧO */
  int key;			/* ���� */

  int fsize;			/* �ɮפj�p */
  static off_t block[MAXBLOCK];	/* �C 32 �C���@�� block�A�O���� block �� offset */

  int cmd;			/* ���_�ɩҫ����� */


  if (!(fimage = f_img(fpath, &fsize)))
    return -1;

  foff = fimage;
  fend = fimage + fsize;

  /* more_width = b_cols - 1; */	/* itoc.070517.����: �Y�γo�ӡA�C�C�̤j�r�Ʒ|�M header �� footer ��� (�Y�|���d�դG��) */
  more_width = b_cols + 1;		/* itoc.070517.����: �Y�γo�ӡA�C�C�̤j�r�ƻP�ù��P�e */
/* dust.080310: �ק�o��ɭn�s show_postlaw() �@�_�� */

  /* �����Y�������a�� */
  for (i = 0; i < LINE_HEADER; i++)
  {
    if (!more_line(buf))
      break;

    /* Ū�X�ɮײĤ@�C�A�ӧP�_�����H�٬O���~�H */
    if (i == 0)
    {
      header_len = 
        !memcmp(buf, str_author1, LEN_AUTHOR1) ? LEN_AUTHOR1 :	/* �u�@��:�v�����峹 */
        !memcmp(buf, str_author2, LEN_AUTHOR2) ? LEN_AUTHOR2 : 	/* �u�o�H�H:�v����H�峹 */
        0;							/* �S�����Y */
    }

    if (!*buf)	/* �Ĥ@�� "\n\n" �O���Y������ */
      break;
  }
  headend = foff;

  /* �k�s */
  foff = fimage;

  lino = cmd = 0;
  block[0] = 0;

#ifdef SLIDE_SHOW
  slideshow = 0;
#endif

  if (hunt[0])		/* �b xxxx_browse() �ШD�j�M�r�� */
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
    /* �L�X�@�C����r					 */
    /* ------------------------------------------------- */

    /* �����e�X�C�~�ݭn�B�z���Y */
    if (foff <= headend)
      outs_header(buf, header_len);
    else
      outs_line(buf);

    outc('\n');

    /* ------------------------------------------------- */
    /* �� shift �ӨM�w�ʧ@				 */
    /* ------------------------------------------------- */

    /* itoc.030303.����: shift �b�����N�q
       >0: �ٻݭn���U���X�C
       <0: �ٻݭn���W���X�C
       =0: �����o���A���ݨϥΪ̫��� */

    if (shift > 0)		/* �٭n�U�� shift �C */
    {
      if (lino >= b_lines)	/* �u���b��i more�A�Ĥ@���L�Ĥ@���ɤ~�i�� lino <= b_lines */
	scroll();

      lino++;

      if ((lino % 32 == 0) && ((i = lino >> 5) < MAXBLOCK))
	block[i] = foff - fimage;


      if (!(shift & (HUNT_MASK | END_MASK)))	/* �@����Ū�� */
      {
	shift--;
      }
      else if (shift & HUNT_MASK)		/* �r��j�M */
      {
	if (shift & HUNT_NEXT)	/* �� n �j�M�U�@�� */
	{
	  /* �@���N����ӦC */
	  if (str_sub(buf, hunt))
	    shift = 0;
	}
	else			/* �� / �}�l�j�M */
	{
	  /* �Y�b�ĤG���H����A�@���N����ӦC�F
	     �Y�b�Ĥ@�����A��������Ū���Ĥ@���~�ఱ�� */
	  if (shift & HUNT_START && str_sub(buf, hunt))
	    shift ^= HUNT_START | HUNT_FOUND;		/* ���� HUNT_START �å[�W HUNT_FOUND */
	  if (shift & HUNT_FOUND && lino >= b_lines)
	    shift = 0;
	}
      }
    }
    else if (shift < 0)		/* �٭n�W�� -shift �C */
    {
      shift++;

      if (!shift)
      {
	move(b_lines, 0);
	clrtoeol();

	/* �ѤU b_lines+shift �C�O rscroll�Aoffsect �h���T��m�F�o�̪� i �O�`�@�n shift ���C�� */
	for (i += b_lines; i > 0; i--)
	  more_line(buf);
      }
    }

    if (foff >= fend)		/* �w�gŪ���������ɮ� */
    {
      /* �խY�O�� End ����̫�@���A���򰱯d�b 100% �Ӥ������F�_�h�@�ߵ��� */
      if (!(shift & END_MASK))
	break;
      shift = 0;
    }

    if (shift)			/* �ٻݭn�~��Ū��� */
      continue;

    /* ------------------------------------------------- */
    /* �즹�L���һݪ� shift �C�A���U�ӦL�X footer �õ��� */
    /* �ϥΪ̫���					 */
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
      /* itoc.010324: ��F�̶}�l�A�W��������}�A�æ^�� 'k' (keymap[] �w�q�W�@�g) */
      if (lino <= b_lines)
      {
	cmd = 'k';
	break;
      }
      /* �̦h�u��W����@�}�l */
      i = b_lines - lino;
      shift = BMAX(-PAGE_SCROLL, i);
    }

    else if (key == KEY_UP)
    {
      /* itoc.010324: ��F�̶}�l�A�W��������}�A�æ^�� 'k' (keymap[] �w�q�W�@�g) */
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
      if (lino <= b_lines)	/* �w�g�b�̶}�l�F */
	shift = 0;
      else
	shift = -PAGE_SCROLL - 1;
    }

    else if (key == '/' || key == 'n')
    {
      if (key == 'n' && hunt[0])	/* �p�G�� n �o����J�L�j�M�r��A������P�� / */
      {
	shift = HUNT_MASK | HUNT_NEXT;
      }
      else if (vget(b_lines, 0, "�j�M�G", hunt, sizeof(hunt), DOECHO))
      {
	str_lowest(hunt, hunt);
	shift = HUNT_MASK | HUNT_START;
      }
      else				/* �p�G�����j�M���ܡA��ø footer �Y�i */
      {
	shift = 0;
      }
    }

    else if (key == 'C')	/* Thor.980405: more �ɥi�s�J�Ȧs�� */
    {
      FILE *fp;
      if (fp = tbf_open())
      {
	f_suck(fp, fpath);
	fclose(fp);
      }
      shift = 0;		/* ��ø footer */
    }

    else if (key == 'h')
    {
      screenline slt[T_LINES];

      uschar *tmp_fimage;
      uschar *tmp_fend;
      uschar *tmp_foff;
      off_t tmp_block[MAXBLOCK];
      /* dust: show_postlaw()�|�Ψ�fend�Mfoff�C�ҥH�쥻���n�O���U�ӡC */

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

    else		/* ��L�䳣�O�ϥΪ̤��_ */
    {
      /* itoc.041006: �ϥΪ̤��_������n > 0 (�� KEY_LEFT �O < 0) */
      cmd = key > 0 ? key : 'q';
      break;
    }

    /* ------------------------------------------------- */
    /* �ϥΪ̤w����A�Y break �h���}�j��F�_�h�̷� shift */
    /* ������ (��Y���䪺����) �Ӱ����P���ʧ@		 */
    /* ------------------------------------------------- */

    if (shift > 0)			/* �ǳƤU�� shift �C */
    {
      if (shift < (HUNT_MASK | HUNT_START))	/* �@��U�� */
      {
	/* itoc.041114.����: �ؼЬO�q�X lino-b_lines+1+shift ~ lino+shift �C�����e�G
	   �N�u�n�M footer �Y�i�A��L���N�浹�e���`�ǦL shift �C���{�� */

	move(b_lines, 0);
	clrtoeol();

#if 1
	/* itoc.041116: End ���@�k���M�@��U���i�H�O�����@�˪��A���O�p�G�J��W���峹�ɡA
	   �|�y���e���`�ǦL shift �C���{���N�o�@��½�A������̫�@���A�o�˷|���Ӧh outs_line() �դu�A
	   �ҥH�b���S�O�ˬd�W���峹�ɡA�N���h��̫�@���Ҧb */

	if ((shift & END_MASK) && (fend - foff >= MORE_BUFSIZE))	/* �٦��@��SŪ�L�A�~�S�O�B�z */
	{
	  int totallino = lino;

	  /* ��Ū��̫�@�C�ݬݥ������X�C */
	  while (more_line(buf))
	  {
	    totallino++;
	    if ((totallino % 32 == 0) && ((i = totallino >> 5) < MAXBLOCK))
	      block[i] = foff - fimage;
	  }

	  /* ���첾��W�@�� block ������ */
	  i = (totallino - b_lines) >> 5;
	  if (i >= MAXBLOCK)
	    i = MAXBLOCK - 1;
	  foff = fimage + block[i];
	  i = i << 5;

	  /* �A�q�W�@�� block �����ݦ첾�� totallino-b_lines+1 �C */
	  for (i = totallino - b_lines - i; i > 0; i--)
	    more_line(buf);

	  lino = totallino - b_lines;
	}
#endif
      }
      else
      {
	/* '/' �q�Y�}�l�j�M */
	lino = 0;
	foff = fimage;
	clear();
      }
    }
    else if (shift < 0)			/* �ǳƤW�� -shift �C */
    {
      if (shift >= -PAGE_SCROLL)	/* �W���ƦC */
      {
	lino += shift;

	/* itoc.041114.����: �ؼЬO�q�X lino-b_lines+1 ~ lino �C�����e�G
	  1. ���q�Y�첾�� lino-b_lines+1 �C
	  2. �䤤�� b_lines+shift �C�O���ܪ����e�A�� rscroll �F��
	  3. �b�e���� outs_line() ���a��L�X -shift �C
	  4. �̫�A�첾��~ rscroll ���C��
	*/

	/* ���첾��W�@�� block ������ */
	i = (lino - b_lines) >> 5;
	if (i >= MAXBLOCK)
	  i = MAXBLOCK - 1;
	foff = fimage + block[i];
	i = i << 5;

	/* �A�q�W�@�� block �����ݦ첾�� lino-b_lines+1 �C */
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
	/* itoc.041226.����: �ؼЬO�q�X 1 ~ b_lines �C�����e�G
           �@�k�N�O�������k�s�A�q�Y�A�L b_lines �C�Y�i */

	clear();

	foff = fimage;
	lino = 0;
	shift = b_lines;
      }
    }
    else				/* ��ø footer �� re-key */
    {
      move(b_lines, 0);
      clrtoeol();
      goto re_key;
    }
  }	/* while(more_line()) �j�骺���� */

  /* --------------------------------------------------- */
  /* �ɮפw�g�q�� (cmd = 0) �� �ϥΪ̤��_ (cmd != 0)	 */
  /* --------------------------------------------------- */

  free(fimage);

  if (!cmd)	/* �ɮץ��`�q���A�n�B�z footer */
  {
    if (footer)		/* �� footer */
    {
      if (footer != (char *) -1)
	outf(footer);
      else
	outs(str_ransi);
    }
    else		/* �S�� footer �n vmsg() */
    {
      /* lkchu.981201: ���M�@���H�K���|��� */
      move(b_lines, 0);
      clrtoeol();

      if (vmsg(NULL) == 'C')	/* Thor.990204: �S�O�`�N�Y�^�� 'C' ��ܼȦs�� */
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
  else		/* �ϥΪ̤��_�A�������} */
  {
    outs(str_ransi);
  }

  hunt[0] = '\0';

  /* Thor.990204: ��key�i�^�Ǧ�more�~ */
  return cmd;
}

#endif
