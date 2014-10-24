/*-------------------------------------------------------*/
/* ufo.c						 */
/*-------------------------------------------------------*/
/* target : User Custom Flag Option Routine		 */
/* create : 10/02/28					 */
/* update : 10/03/30					 */
/*-------------------------------------------------------*/

#include "bbs.h"

#define SEPARATOR "\033[30;45m�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h�h\033[m"

typedef struct {
  char *desc;
  char *help;
  char **option;
  int optsz;
  usint perm;
  int (*processor)(int, ACCT*);
} UFO_ItemInfo;


typedef struct {
  char *name;
  UFO_ItemInfo *items;
} UFO_PageInfo;


typedef struct
{
  UFO_PageInfo *pinfo;
  int label_sx;		/* ø�s���_�l�I */
  int label_ex;
  int size;		/* �ӭ������h�ֶ��� */
  UFO_ItemInfo **items;
  int *curr_opt;
  int *old_opt;
  int cursor_position;
} UFOpage;


#include "ufo.rc"

/* ------------------------------------- */
/* page information			 */
/* ------------------------------------- */

static UFO_PageInfo page_infor[] = 
{
  "�~�[�]�w", ui_appearance,

  "�\\��]�w", ui_functionality,

  "�s�边", ui_editor,

  "�ۭq�D��", ui_equipment,

  "�����s��", ui_admin,

  NULL, NULL
};



/* ------------------------------------- */
/* �D�{��				 */
/* ------------------------------------- */

static UFOpage *pages;
static int page_n;

#define IDESC_LEN 36
#define OPTION_X (IDESC_LEN + 8)
#define ITEM_MAX 17


static void
outs_labeltop(len)
  int len;
{
  int x;

  getyx(NULL, &x);
  move(1, x);

  outs("\033[1;35m");
  for(; len > 1; len -= 2)
    outs("��");

  if(len > 0)
    outs("\241\033[;30m\304");

  outs("\033[m");
  move(2, x);
}


static void
ufopage_drawlabel(currpage, moving_direction, scope_sx)
  int currpage;
  int moving_direction;
  int *scope_sx;
{
  int i, phase, delta, namelen, in_zhc, curr_x;

  curr_x = *scope_sx;

  /* caculating the proper starting position */
  if(moving_direction > 0)
  {
    i = currpage+1 < page_n? currpage+1 : currpage;
    if(pages[i].label_ex > curr_x + 78)
      curr_x = pages[i].label_ex - 78;
  }
  else if(moving_direction < 0)
  {
    i = currpage > 0? currpage-1 : currpage;
    if(pages[i].label_sx < curr_x)
      curr_x = pages[i].label_sx;
  }
  else	/* �u���b�Ĥ@���i�Jufo_pageproc()��moving_direction�|�O�s */
  {
    curr_x = 0;
  }

  /* ��e���������Ҥ@�w�n���b�i���d��� */
  if(pages[currpage].label_sx < curr_x || pages[currpage].label_ex > curr_x + 78)
    curr_x = pages[currpage].label_sx;

  for(i = 1; i < page_n; i++)	/* ��X�_�l�I�O���b���Ӽ��Ҹ� */
  {
    if(curr_x < pages[i].label_sx)
      break;
  }
  i--;


  /* �B�z�̫e�Y������ */
  move(1, 0);
  clrtoln(3);
  move(2, 0);

  delta = curr_x - pages[i].label_sx;
  *scope_sx = curr_x;
  curr_x = 0;
  phase = 1;
  if(delta > 0)	/* �Y�_�l�I�N�O�Y���Ҫ��_�l�I���ܴN���ѳo�q�{���X�B�z�A�浹�U�@�q */
  {
    namelen = strlen(pages[i].pinfo->name);

    if(delta < 4)		/* the first part of label */
    {
      char fwdigit_buf[8];

      sprintf(fwdigit_buf, " \242%c ", 176+i);
      if(delta == 2)
	fwdigit_buf[2] = ' ';

      if(delta < 2) attrsetfg(3);
      outs(fwdigit_buf + delta);
      if(delta < 2) attrsetfg(7);

      curr_x = 4 - delta;
      phase = 2;
    }
    else if(delta < namelen + 6)	/* the second part of label */
    {
      delta -= 4;
      if(delta < namelen)
      {
	if(IS_ZHC_LO(pages[i].pinfo->name, delta))
	{
	  outc(' ');
	  outs(pages[i].pinfo->name + delta+1);
	}
	else
	  outs(pages[i].pinfo->name + delta);
	curr_x += namelen - delta;
      }

      if(delta <= namelen)
      {
	outc(' ');
	curr_x++;
      }
      outc(' ');
      curr_x++;
      phase = 3;
    }
    else			/* the third part of label */
    {
      if(delta - namelen - 6 == 0)
      {
	outc(' ');
	curr_x++;
      }
      outc(' ');
      curr_x++;
      phase = 1;
      i++;		/* assigned to next label */
    }
  }	/* if(delta > 0) */


  /* �B�z���U�Ӫ����� */
  in_zhc = 0;
  for(delta = 0; curr_x < 78 && i < page_n; curr_x++, delta++)
  {
    if(phase == 1)
    {
      switch(delta)
      {
      case 0:
	namelen = strlen(pages[i].pinfo->name);

	if(i == currpage)
	{
	  outs_labeltop(namelen + 6);
	  attrset(0x5B);
	}
	else
	  attrsetfg(3);
	outc(' ');

	break;

      case 1:
	outc('\242');
	in_zhc = 176 + i;
	break;

      case 2:
	outc(176 + i);
	in_zhc = 0;
	break;

      case 3:
	outc(' ');
	attrsetfg(7);
	phase++;
	delta = -1;
      }
    }
    else if(phase == 2)
    {
      if(delta < namelen)
      {
	outc(pages[i].pinfo->name[delta]);

	if(in_zhc)
	  in_zhc = 0;
	else if(IS_ZHC_HI(pages[i].pinfo->name[delta]))
	  in_zhc = pages[i].pinfo->name[delta+1];
      }
      else if(delta == namelen)
      {
	outc(' ');
	in_zhc = 0;	/* ������ҦW�٥����O�����㪺DBCS�r��ɱN�����n���r�t�±��F */
      }
      else
      {
	outc(' ');
	phase++;
	delta = -1;
	if(i == currpage)
	  attrset(7);
      }
    }
    else if(phase == 3)
    {
      if(i == page_n - 1)
	break;		/* �̫᪺���Ҥ��ݭn�� phase 3�A�j����j�� */

      outc(' ');
      if(delta)
      {
	phase = 1;
	delta = -1;
	i++;		/* assigned to next label */
      }
    }
  }

  if(in_zhc)	/* �I��(�±�)����r��b�q */
  {
    attrset(0);
    outc(in_zhc);
  }

  outs("\033[m");
}


static void
option_switch(page, no, var)	/* change the current option */
  UFOpage *page;
  int no;
  int var;
{
  int i, size, x, oldp;
  char **option;

  oldp = page->curr_opt[no];
  size = page->items[no]->optsz;
  while(var < 0)
    var += size;

  var = (oldp + var) % size;
  option = page->items[no]->option;

  /* redraw option */
  x = OPTION_X;
  for(i = 0; i < size; i++)
  {
    if(i == oldp || i == var)
    {
      move(no + 4, x);
      attrset(i == var? 0x0E:0x08);	/* 1;36m �M 1;30m */
      outs(option[i]);
      attrset(7);
    }

    x += strlen(option[i]) + 1;
  }

  page->curr_opt[no] = var;
}


static int
ufo_pageproc(currpage, moving_direction, scope_sx)
  int currpage;
  int moving_direction;
  int *scope_sx;
{
  screen_backup_t old_screen;
  int key, cc, oc;
  int i, j, optsz;
  int size = pages[currpage].size;
  int *curr_opt = pages[currpage].curr_opt;


  /* draw header(label) */
  ufopage_drawlabel(currpage, moving_direction, scope_sx);

  /* draw body(items & options) */
  move(4, 0);
  clrtoln(21);

  oc = cc = pages[currpage].cursor_position;
  for(i = 0; i < size; i++)
  {
    if(i == cc)
      outs("\033[1m>");
    else
      outs(" ");

    prints("  %c. %-*.*s ", i + 'a', IDESC_LEN, IDESC_LEN, pages[currpage].items[i]->desc);

    if(i == cc)
      outs("\033[m");

    outc('(');

    optsz = pages[currpage].items[i]->optsz;
    for(j = 0; j < optsz; j++)
    {
      if(j > 0)
	outc('/');
      prints("\033[1;3%dm%s\033[m", j==curr_opt[i]? 6:0, pages[currpage].items[i]->option[j]);
    }
    outs(")\n");
  }

  /* draw footer */
  move(23, 0);
  clrtoeol();

  outs(" \033[1;33m�� \033[m�Ы�\033[1;33m");
  if(size <= 1)
    outs("[a]");
  else
    prints("[a-%c]", 'a' + size - 1);

  outs("\033[m�����]�w�A");
  if(page_n > 1)
    prints("\033[1;33m[1-%d]\033[m���������A", page_n);
  outs("\033[1;33m[Ctrl+H]\033[m�\\Ū�ԲӾާ@�����A\033[1;33m[Enter]\033[m���}�C");


  /* key process */
  while(1)
  {
    if(oc != cc)
    {
      move(4 + oc, 0);
      prints("   %c. %-*.*s ", oc + 'a', IDESC_LEN, IDESC_LEN, pages[currpage].items[oc]->desc);

      move(4 + cc, 0);
      prints("\033[1m>  %c. %-*.*s \033[m", cc + 'a', IDESC_LEN, IDESC_LEN, pages[currpage].items[cc]->desc);

      oc = cc;
    }

    move(4 + cc, 1);
    key = vkey();
    if(key >= 'A' && key <= 'Z')	/* ������p�g�ӧP�_ */
      key += 32;

    if(key == KEY_PGUP)
    {
      option_switch(&pages[currpage], cc, -1);
    }
    else if(key == KEY_PGDN)
    {
      option_switch(&pages[currpage], cc, 1);
    }
    else if(key == KEY_UP)
    {
      cc = (cc + size - 1) % size;
    }
    else if(key == KEY_DOWN)
    {
      cc = (cc + 1) % size;
    }
    else if(key >= 'a' && key - 'a' < size)
    {
      cc = key - 'a';
      option_switch(&pages[currpage], cc, 1);
    }
    else if(key == KEY_LEFT || key == KEY_RIGHT)
    {
      break;
    }
    else if(key == '\n' || key == Ctrl('Q'))
    {
      break;
    }
    else if(key == Ctrl('H'))
    {
      scr_dump(&old_screen);

      more("etc/help/ufo.hlp", MFST_NULL);

      scr_restore(&old_screen);
    }
    else if(key > '0' && key - '1' < page_n)
    {
      if(key - '1' != currpage)	/* �P�����N�������F */
	break;
    }
#if 0
    else if(key == Ctrl('F'))		/* dust.100302: �Ȯɤ���� */
    {
      ufo_itemhelp(&pages[currpage]->items[cc]);
    }
#endif
  }

  pages[currpage].cursor_position = cc;
  return key;
}



static int
count_nitems(items)	/* �p��b�ӭ�����ݨ�h�ֶ��� */
  UFO_ItemInfo items[];
{
  int i, n = 0;

  for(i = 0; items[i].desc; i++)
  {
    if(!items[i].perm || HAS_PERM(items[i].perm))
      n++;
  }

  return n;
}

int cloak_remember = 0;	/* davy.110928: �����������A */

static void
page_init(cp, size, acct)	/* initializing page containers */
  UFOpage *cp;
  int size;
  ACCT *acct;
{
  int i, j;
  UFO_ItemInfo *item_infos = cp->pinfo->items;
  usint item_perm;

  cp->size = size;
  cp->items = (UFO_ItemInfo**) malloc(sizeof(UFO_ItemInfo*) * size);
  cp->curr_opt = (int*) malloc(sizeof(int) * size);
  cp->old_opt = (int*) malloc(sizeof(int) * size);
  cp->cursor_position = 0;

  j = 0;
  for(i = 0; j < size; i++)
  {
    item_perm = item_infos[i].perm;
    if(!item_perm || HAS_PERM(item_perm))
    {
      cp->items[j] = &item_infos[i];
      cp->old_opt[j] = cp->curr_opt[j] = (item_infos[i].processor)(-1, acct);
      j++;
    }
  }

  /* davy.110928: �����������A */
  cloak_remember = acct->ufo & UFO_CLOAK ? 1 : 0;
}


static int
save_settings(quit, acct)	/* save settings and release memory */
  int quit;
  ACCT *acct;
{
  int i, j;
  int changed = 0;
  int size;

  for(i = 0; i < page_n; i++)
  {
    if(quit == 1)
    {
      size = pages[i].size;
      for(j = 0; j < size; j++)
      {
	if(pages[i].curr_opt[j] != pages[i].old_opt[j])
	{
	  pages[i].items[j]->processor(pages[i].curr_opt[j], acct);
	  changed = 1;
	}
      }
    }

    free(pages[i].items);
    free(pages[i].curr_opt);
    free(pages[i].old_opt);
  }

  free(pages);
  return changed;
}



int
ufo_main(acct, utmp)
  ACCT *acct;
  UTMP *utmp;
{
  int page_no, prevpage;
  int i, j, size, key;
  int quit, scope_sx;
  const char const *separator = SEPARATOR;


  /* �p��i�������`�� */
  page_n = 0;
  for(i = 0; page_infor[i].name; i++)
  {
    if(count_nitems(page_infor[i].items) > 0)
      page_n++;
  }

  if(page_n == 0)
    return 0;
  else if(page_n > 9)
    page_n = 9;

  /* ��l�ƩҦ���������T */
  pages = (UFOpage*) malloc(sizeof(UFOpage) * page_n);

  j = 0;
  for(i = 0; j < page_n; i++)
  {
    size = count_nitems(page_infor[i].items);
    if(size != 0)
    {
      pages[j].pinfo = &page_infor[i];
      pages[j].label_sx = (j > 0)? pages[j-1].label_ex + 2 : 0;
      pages[j].label_ex = pages[j].label_sx + strlen(page_infor[i].name) + 6;
      page_init(&pages[j], (size>ITEM_MAX)? ITEM_MAX : size, acct);
      j++;
    }
  }

  /* ø�s�e�� */
  move(1, 0);
  clrtobot();
  move(3, 0);
  outs(separator);
  move(22, 0);
  outs(separator);

  page_no = 0;
  quit = 0;
  prevpage = 99;
  scope_sx = 0;

  while(!quit)
  {
    key = ufo_pageproc(page_no, page_no - prevpage, &scope_sx);
    prevpage = page_no;

    if(key == '\n')
    {
      quit = 1;		/* �x�s�]�w�����} */
    }
    else if(key == Ctrl('Q'))
    {
      quit = -1;	/* ���x�s�]�w�N���} */
    }
    else if(key == KEY_RIGHT)
    {
      page_no = (page_no + 1) % page_n;
    }
    else if(key == KEY_LEFT)
    {
      page_no = (page_no + page_n - 1) % page_n;
    }
    else if(isdigit(key))
    {
      page_no = key - '1';
    }
  }

  if(save_settings(quit, acct))
  {
    if(utmp)
    {
      utmp->ufo = acct->ufo;
      if (!(acct->ufo & UFO_CLOAK) && cloak_remember)
        time(&utmp->login_time);     /* davy.110928: ��s�X�{�ɶ� */
    }
    vmsg("�w�x�s�ܧ�");
  }

  return 0;
}

