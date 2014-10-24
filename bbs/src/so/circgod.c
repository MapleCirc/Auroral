/*-------------------------------------------------------*/
/* circgod.c (NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : �l��q�㯫          			 */
/* create : 04/04/25					 */
/* update :   /  /  					 */
/* author : circgod@tcirc.org           		 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;

/* floatJ.091029: [�۲�] �樫��ѤW���q�㯫 */
/* �򥻤W�D�[�c�O�� innbbs.c �Ӫ�           */
/* ----------------------------------------------------- */
/* circ.god �l�禡                                       */
/* ----------------------------------------------------- */


static void
circgod_item(num, circgod)
  int num;
  circgod_t *circgod;
{
  prints("%6d %-*.*s\n", num,
    d_cols + 44, d_cols + 44, circgod->title);
}


static void
circgod_query(circgod)
  circgod_t *circgod;
{

  /* �@�i�h�N���� */
  //char *buf;
  //strcpy(circgod->title,buf);
  circlog("�s������", circgod->title);

  move(3, 0);
  clrtobot();
  prints("\n\n�ŦW�G%s\n�Ż��G%s\n�ŽX�G\033[1;33m%s\033[m",
    circgod->title, circgod->help, circgod->password);
  vmsg(NULL);
}


static int      /* 1:���\ 0:���� */
circgod_add(fpath, old, pos)
  char *fpath;
  circgod_t *old;
  int pos;
{
  circgod_t circgod;

  if (old)
    memcpy(&circgod, old, sizeof(circgod_t));
  else
    memset(&circgod, 0, sizeof(circgod_t));

  if (vget(b_lines, 0, "���D�G", circgod.title, /* sizeof(circgod.title) */ 70, GCARRY) &&
    vget(b_lines, 0, "�����G", circgod.help, 70 , GCARRY) && 
    vget(b_lines, 0, "�K�X�G", circgod.password, 70 , GCARRY))
  {
    circlog("�s�W�νs����", circgod.title);

    if (old)
      rec_put(fpath, &circgod, sizeof(circgod_t), pos, NULL);
    else
      rec_add(fpath, &circgod, sizeof(circgod_t));
    return 1;
  }
  return 0;
}


static int
circgod_cmp(a, b)
  circgod_t *a, *b;
{
  /* �� title �Ƨ� */
  return str_cmp(a->title, b->title);
}


static int
circgod_search(circgod, key)
  circgod_t *circgod;
  char *key;
{
  return (int) (str_str(circgod->title, key) || str_str(circgod->help, key));
}



/* ID ���禡�A�q�ɮפ���ثeID���ýT�{�O�_���
�A���� belong_list����²�����A�qbbsd.c�۹L�Ӫ� */
static int
belong(flist, key)
  char *flist;
  char *key;
{
  int fd, rc;

  rc = 0;
  if ((fd = open(flist, O_RDONLY)) >= 0)
  {
    mgets(-1);

    while (flist = mgets(fd))
    {
      str_lower(flist, flist);
      if (str_str(key, flist))
      {
        rc = 1;
        break;
      }
    }

    close(fd);
  }
  return rc;
}




/* ----------------------------------------------------- */
/* �q�㯫�l�ꦨ�\�᪺�K�X�s������			 */
/* �q innbbs.c �۹L�Ӫ��D�禡	         		 */
/* ----------------------------------------------------- */


int
a_circgod()
{
  int num, pageno, pagemax, redraw, reload;
  int ch, cur, i, dirty;
  struct stat st;
  char *data;
  int recsiz;
  char *fpath;
  char buf[40];
  void (*item_func)(), (*query_func)();
  int (*add_func)(), (*sync_func)(), (*search_func)();



  /* ���ˬd ID �O�_�b FN_ETC_CIRCGODLIST �̭�  */
  if (!belong(FN_ETC_CIRCGOD,cuser.userid))
  {
    vmsg("�u���b�u�l��W��v�����ϥΪ̤~��ϥ� (�`�N�Gsysop����)");
    //("�Y�n�]�w�l��W��A�Шϥ� sysop �n�J��A��ܡu�l��W��v�i��ק�");
    return 0;

  }else{

    /* �������ҵ{�� */
    char passbuf[20];
    if(!vget(b_lines, 0, "�i�۲šj�п�J�K�X�A�H�~��l��q�㯫�G", passbuf, PSWDLEN + 1, NOECHO))
      return 0;

    if(chkpasswd(cuser.passwd, passbuf))
    {
      circlog("�l�ꥢ�ѡA�K�X���~", "");
      vmsg(ERR_PASSWD);
      return 0;
    }
  }

  circlog("�l�ꦨ�\\�A�i�J�s������",fromhost);
  vs_bar("CIRC God");
  more("etc/innbbs.hlp", MFST_NONE);

  /* floatJ.091028: [�۲�] �樫��ѤW���q�㯫. ���� */
  /* ���c���b struct.h �� (circgod_t)*/

  
  fpath = "etc/circ.god";
  recsiz = sizeof(circgod_t);
  item_func = circgod_item;
  query_func = circgod_query;
  add_func = circgod_add;
  sync_func = circgod_cmp;
  search_func = circgod_search;

  dirty = 0;	/* 1:���s�W/�R����� */
  reload = 1;
  pageno = 0;
  cur = 0;
  data = NULL;

  do
  {
    if (reload)
    {
      if (stat(fpath, &st) == -1)
      {
	if (!add_func(fpath, NULL, -1))
	  return 0;
	dirty = 1;
	continue;
      }

      i = st.st_size;
      num = (i / recsiz) - 1;
      if (num < 0)
      {
	if (!add_func(fpath, NULL, -1))
	  return 0;
	dirty = 1;
	continue;
      }

      if ((ch = open(fpath, O_RDONLY)) >= 0)
      {
	data = data ? (char *) realloc(data, i) : (char *) malloc(i);
	read(ch, data, i);
	close(ch);
      }

      pagemax = num / XO_TALL;
      reload = 0;
      redraw = 1;
    }

    if (redraw)
    {
      /* itoc.����: �ɶq���o�� xover �榡 */
      vs_head("CIRC God", str_site);
      prints(NECKER_INNBBS, d_cols, "");

      i = pageno * XO_TALL;
      ch = BMIN(num, i + XO_TALL - 1);
      move(3, 0);
      do
      {
	item_func(i + 1, data + i * recsiz);
	i++;
      } while (i <= ch);

      outf(FEETER_INNBBS);
      move(3 + cur, 0);
      outc('>');
      redraw = 0;
    }

    ch = vkey();
    switch (ch)
    {
    case KEY_RIGHT:
    case '\n':
    case ' ':
    case 'r':
      i = cur + pageno * XO_TALL;
      query_func(data + i * recsiz);
      redraw = 1;
      break;

    case 'a':
    case Ctrl('P'):

      if (add_func(fpath, NULL, -1))
      {
	dirty = 1;
	num++;
	cur = num % XO_TALL;		/* ��Щ�b�s�[�J���o�g */
	pageno = num / XO_TALL;
	reload = 1;
      }
      redraw = 1;
      break;

    case 'd':
      if (vans(msg_del_ny) == 'y')
      {
        circlog("�R�����", "");
	dirty = 1;
	i = cur + pageno * XO_TALL;
	rec_del(fpath, recsiz, i, NULL);
	cur = i ? ((i - 1) % XO_TALL) : 0;	/* ��Щ�b�屼���e�@�g */
	reload = 1;
      }
      redraw = 1;
      break;

    case 'E':
      i = cur + pageno * XO_TALL;
      if (add_func(fpath, data + i * recsiz, i))
      {
	dirty = 1;
	reload = 1;
      }
      redraw = 1;
      break;

    case '/':
      if (vget(b_lines, 0, "����r�G", buf, sizeof(buf), DOECHO))
      {
	str_lower(buf, buf);
	for (i = pageno * XO_TALL + cur + 1; i <= num; i++)	/* �q��ФU�@�Ӷ}�l�� */
	{
	  if (search_func(data + i * recsiz, buf))
	  {
	    pageno = i / XO_TALL;
	    cur = i % XO_TALL;
	    break;
	  }
	}
      }
      redraw = 1;
      break;

    default:
      ch = xo_cursor(ch, pagemax, num, &pageno, &cur, &redraw);
      break;
    }
  } while (ch != 'q');

  free(data);

  if (dirty)
    rec_sync(fpath, recsiz, sync_func, NULL);
  return 0;
}
