/*-------------------------------------------------------*/
/* circgod.c (NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 召喚電研神          			 */
/* create : 04/04/25					 */
/* update :   /  /  					 */
/* author : circgod@tcirc.org           		 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;

/* floatJ.091029: [幻符] 行走於天上的電研神 */
/* 基本上主架構是抄 innbbs.c 來的           */
/* ----------------------------------------------------- */
/* circ.god 子函式                                       */
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

  /* 一進去就紀錄 */
  //char *buf;
  //strcpy(circgod->title,buf);
  circlog("瀏覽項目", circgod->title);

  move(3, 0);
  clrtobot();
  prints("\n\n符名：%s\n符說：%s\n符碼：\033[1;33m%s\033[m",
    circgod->title, circgod->help, circgod->password);
  vmsg(NULL);
}


static int      /* 1:成功 0:失敗 */
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

  if (vget(b_lines, 0, "標題：", circgod.title, /* sizeof(circgod.title) */ 70, GCARRY) &&
    vget(b_lines, 0, "說明：", circgod.help, 70 , GCARRY) && 
    vget(b_lines, 0, "密碼：", circgod.password, 70 , GCARRY))
  {
    circlog("新增或編輯資料", circgod.title);

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
  /* 依 title 排序 */
  return str_cmp(a->title, b->title);
}


static int
circgod_search(circgod, key)
  circgod_t *circgod;
  char *key;
{
  return (int) (str_str(circgod->title, key) || str_str(circgod->help, key));
}



/* ID 比對函式，從檔案中跟目前ID比對並確認是否放行
，類似 belong_list的精簡版本，從bbsd.c抄過來的 */
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
/* 電研神召喚成功後的密碼瀏覽介面			 */
/* 從 innbbs.c 抄過來的主函式	         		 */
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



  /* 先檢查 ID 是否在 FN_ETC_CIRCGODLIST 裡面  */
  if (!belong(FN_ETC_CIRCGOD,cuser.userid))
  {
    vmsg("只有在「召喚名單」內的使用者才能使用 (注意：sysop不行)");
    //("若要設定召喚名單，請使用 sysop 登入後，選擇「召喚名單」進行修改");
    return 0;

  }else{

    /* 身分驗證程序 */
    char passbuf[20];
    if(!vget(b_lines, 0, "【幻符】請輸入密碼，以繼續召喚電研神：", passbuf, PSWDLEN + 1, NOECHO))
      return 0;

    if(chkpasswd(cuser.passwd, passbuf))
    {
      circlog("召喚失敗，密碼錯誤", "");
      vmsg(ERR_PASSWD);
      return 0;
    }
  }

  circlog("召喚成功\，進入瀏覽介面",fromhost);
  vs_bar("CIRC God");
  more("etc/innbbs.hlp", MFST_NONE);

  /* floatJ.091028: [幻符] 行走於天上的電研神. 介面 */
  /* 結構體放在 struct.h 內 (circgod_t)*/

  
  fpath = "etc/circ.god";
  recsiz = sizeof(circgod_t);
  item_func = circgod_item;
  query_func = circgod_query;
  add_func = circgod_add;
  sync_func = circgod_cmp;
  search_func = circgod_search;

  dirty = 0;	/* 1:有新增/刪除資料 */
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
      /* itoc.註解: 盡量做得像 xover 格式 */
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
	cur = num % XO_TALL;		/* 游標放在新加入的這篇 */
	pageno = num / XO_TALL;
	reload = 1;
      }
      redraw = 1;
      break;

    case 'd':
      if (vans(msg_del_ny) == 'y')
      {
        circlog("刪除資料", "");
	dirty = 1;
	i = cur + pageno * XO_TALL;
	rec_del(fpath, recsiz, i, NULL);
	cur = i ? ((i - 1) % XO_TALL) : 0;	/* 游標放在砍掉的前一篇 */
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
      if (vget(b_lines, 0, "關鍵字：", buf, sizeof(buf), DOECHO))
      {
	str_lower(buf, buf);
	for (i = pageno * XO_TALL + cur + 1; i <= num; i++)	/* 從游標下一個開始找 */
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
