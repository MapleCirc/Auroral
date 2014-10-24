/*-------------------------------------------------------*/
/* help.c						 */
/*-------------------------------------------------------*/
/* target : help 說明文件				 */
/* create : 03/05/10				 	 */
/* update : 12/10/28				 	 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/



#include "bbs.h"



static void
do_help(catagory)
  char *catagory;
{
  char fpath[64], dir_path[64], *fname;
  int hmax, pageno, pagemax;
  int cur, redraw, reload, redraw_foot, old_blines;
  int ch, i, found;
  struct stat st;
  HLP *list;
  char hunt[32];


  sprintf(dir_path, "etc/help/%s/%s", catagory, fn_dir);
  fname = fpath + sprintf(fpath, "etc/help/%s/", catagory);

  reload = 1;
  pageno = 0;
  cur = 0;
  list = NULL;
  old_blines = b_lines;

  do
  {
    if(reload)
    {
      int fd;

      if(stat(dir_path, &st) < 0)
	break;

      hmax = st.st_size / sizeof(HLP) - 1;
      if(hmax < 0)
	break;

      if(cur > hmax) cur = hmax;

      if((fd = open(dir_path, O_RDONLY)) >= 0)
      {
	list = (HLP *) realloc(list, st.st_size);
	if(!list)
	  return;
	read(fd, list, st.st_size);
	close(fd);
      }

      pagemax = hmax / XO_TALL;
      reload = 0;
      redraw = 1;
    }

    if(redraw)
    {
      int end;

      vs_head("說明文件", str_site);
      prints(NECKER_HELP, d_cols, "");

      i = cur - cur % XO_TALL;
      end = BMIN(hmax, i + XO_TALL - 1);
      move(3, 0);
      do
      {
	int in_dbc = 0;
	uschar *s = list[i].title;

	prints("%6d   %-15s ", i + 1, list[i].fname);
	if(*s == '[')
	{
	  outc('[');
	  s++;
	  if(*s >= 'A' && *s <= 'H')
	  {
	    attrset(010 + (*s - 'A'));
	    s++;
	  }
	}

	while(*s)
	{
	  if(!in_dbc && (*s == ']' || *s == ')'))
	    attrset(7);

	  outc(*s);

	  if(in_dbc)
	    in_dbc = 0;
	  else
	  {
	    if(IS_ZHC_HI(*s))
	      in_dbc = 1;
	    else if(*s == '(')
	      attrset(016);
	  }

	  s++;
	}

	outc('\n');

      } while(++i <= end);

      redraw = 0;
      goto RedrawHelpFoot;
    }
    else if(redraw_foot)
    {
RedrawHelpFoot:
      move(b_lines, 0);
      outf(FEETER_HELP);
      move(cur % XO_TALL + 3, 0);
      outc('>');

      redraw_foot = 0;
    }

    ch = vkey();

    if(old_blines != b_lines)	/* 偵測螢幕大小變化 */
    {
      old_blines = b_lines;
      pagemax = hmax / XO_TALL;
      redraw = 1;
    }

re_key:
    switch (ch)
    {
    case KEY_RIGHT:
    case '\n':
    case 'r':
      while(1)
      {
	strcpy(fname, list[cur].fname);
	switch(more(fpath, MFST_NORMAL))
	{
	case ' ':
	case KEY_RIGHT:
	case KEY_PGDN:
	case KEY_DOWN:
	case 'j':
	  if(cur < hmax)
	  {
	    cur++;
	    continue;
	  }
	  break;

	case KEY_UP:
	case KEY_PGUP:
	case 'k':
	  if(cur > 0)
	  {
	    cur--;
	    continue;
	  }
	  break;

	case 'E':
	  ch = 'E';
	  redraw = 1;
	  goto re_key;
	}

	break;
      }

      redraw = 1;
      break;

    case '/':
      redraw_foot = 1;
      if(!vget(b_lines, 0, "標題關鍵字：", hunt, sizeof(hunt), DOECHO))
	break;

      i = cur;
      found = 0;

      do
      {
	if(strstr(list[i].title, hunt))
	{
	  found = 1;
	  break;
	}

	if(i >= hmax)
	  i = 0;
	else
	  i++;

      } while(i != cur);

      if(found)
      {
	cur = i;
	redraw = 1;
      }
      else
	vmsg(MSG_XY_NONE);

      break;

    case 't':
    case 'T':
      if(HAS_PERM(PERM_ALLADMIN))
      {
	if(vget(b_lines, 0, "標題：", list[cur].title, sizeof(list[0].title), GCARRY))
	{
	  rec_put(dir_path, &list[cur], sizeof(HLP), cur, NULL);
	  redraw = 1;
	}
	else
	  redraw_foot = 1;
      }
      break;

    case 'E':
      if(HAS_PERM(PERM_ALLADMIN))
      {
	strcpy(fname, list[cur].fname);
	if(vedit(fpath, 0) < 0)
	  vmsg("取消");
	redraw = 1;
      }
      break;

    case 'm':
      if(HAS_PERM(PERM_ALLADMIN))
      {
	char buf[40], ans[5];
	int new_pos;

	sprintf(buf, "請輸入第 %d 項的新位置：", cur + 1);
	if(vget(b_lines, 0, buf, ans, 5, DOECHO))
	{
	  new_pos = atoi(ans) - 1;
	  if(new_pos < 0)
	    new_pos = 0;
	  else if(new_pos > hmax)
	    new_pos = hmax;

	  if(new_pos != cur)
	  {
	    if(!rec_del(dir_path, sizeof(HLP), cur, NULL))
	    {
	      rec_ins(dir_path, &list[cur], sizeof(HLP), new_pos, 1);
	      //cur = new_pos;	//dust.121027:暫時disable

	      reload = 1;
	    }
	  }
	}

	redraw_foot = 1;
      }
      break;

    case Ctrl('P'):
      if(HAS_PERM(PERM_ALLADMIN))
      {
	HLP new;
	memset(&new, 0, sizeof(HLP));

	if(vget(b_lines, 0, "標題：", new.title, sizeof(new.title), DOECHO) &&
	  vget(b_lines, 0, "檔案：", new.fname, sizeof(new.fname), DOECHO))
	{
	  strcpy(fname, new.fname);
	  if(!vedit(fpath, 0))
	  {
	    rec_add(dir_path, &new, sizeof(HLP));
	    hmax++;
	    cur = hmax;
	    reload = 1;
	  }
	}
	redraw = 1;
      }
      break;

    case 'd':
      if(HAS_PERM(PERM_ALLADMIN))
      {
	if(vans(msg_del_ny) == 'y')
	{
	  strcpy(fname, list[cur].fname);
	  unlink(fpath);
	  rec_del(dir_path, sizeof(HLP), cur, NULL);
	  if(cur < 0)
	    cur = 0;
	  reload = 1;
	}
	else
	  redraw_foot = 1;
      }
      break;

    default:
      pageno = cur / XO_TALL;
      cur %= XO_TALL;
      ch = xo_cursor(ch, pagemax, hmax, &pageno, &cur, &redraw);
      cur += pageno * XO_TALL;
      break;
    }

  } while(ch != 'q');

  free(list);
}


#include <stdarg.h>

int
vaHelp(pvar)
  va_list pvar;
{
  char *path;
  path = va_arg(pvar, char *);
  do_help(path);
  return 0;
}


