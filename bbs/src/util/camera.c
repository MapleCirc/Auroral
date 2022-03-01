/*-------------------------------------------------------*/
/* util/camera.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : 建立 [動態看板] cache			 */
/* create : 95/03/29				 	 */
/* update : 10/10/02			 		 */
/*-------------------------------------------------------*/


#include "bbs.h"


static char *list[] = 		/* src/include/struct.h */
{
  "opening.0",			/* FILM_OPENING0 */
  "opening.1",			/* FILM_OPENING1 */
  "opening.2",			/* FILM_OPENING2 */
  NULL,
  NULL,
  NULL,
  NULL,			/* dust.101002: 新版本中不使用NULL作結尾 */
  NULL,
  NULL,
  NULL,
  "goodbye", 			/* FILM_GOODBYE  */
  "notify",			/* FILM_NOTIFY   */
  "mquota",			/* FILM_MQUOTA   */
  "mailover",			/* FILM_MAILOVER */
  "mgemover",			/* FILM_MGEMOVER */
  "birthday",			/* FILM_BIRTHDAY */
  "justify",			/* FILM_JUSTIFY  */
  "re-reg",			/* FILM_REREG    */
  "e-mail",			/* FILM_EMAIL    */
  "newuser",			/* FILM_NEWUSER  */
  "tryout",			/* FILM_TRYOUT   */
  "post",			/* FILM_POST     */
  "loginmsg",			/* FILM_LOGINMSG */
  "adminnote",			/* FILM_0ADMIN	 */
};



static FCACHE image;
static int number = 0;	/* 目前已 mirror 幾篇了 */
static int total = 0;	/* 目前已 mirror 幾 byte 了 */


static int 
mirror (
    char *fpath,
    int line		/* 0:系統文件，不限制列數  !=0:動態看板，line 列 */
)
{
  static char film_buf[FILM_SIZ];
  int size;
  FILE *fp;

  /* 若已經超過最大篇數，則不再繼續 mirror */
  if (number >= MOVIE_MAX - 1)
    return 0;

  if(fpath)
  {
    if(fp = fopen(fpath, "r"))
    {
      char buf[512];
      int y = size = 0;

  /* dust.101002: 因為無法呼叫EScode()的關係，所以沒辦法確定一行顯示起來到底會有多長 */
  /* 只好假設站方會把一行的顯示長度控制在 79char 以內...... */
      while(fgets(buf, sizeof(buf), fp))
      {
	int i = size;

	size += strlen(buf);
	if(size >= FILM_SIZ)
	  break;
	else
	  strcpy(film_buf + i, buf);

	if(line && strchr(buf, '\n'))
	{
	  /* 動態看板，最多 line 列 */
	  if (++y >= line)
	    break;
	}
      }
      fclose(fp);

      if(line)
      {
	/* 動態看板，若不到 line 列，要填滿 line 列 */
	for (; y < line; y++)
	{
	  if(size >= FILM_SIZ) break;

	  film_buf[size++] = '\n';
	}
      }

      if(size >= FILM_SIZ)
      {
	if(line)	/* 動態看板過大就不 mirror 了 */
	  return 1;

	sprintf(film_buf, "請告訴站長檔案 %s 過大", fpath);
	size = strlen(film_buf);
      }
    }
    else
    {
      if(line)		/* 如果是 動態看板/點歌 缺檔案，就不 mirror */
	return 1;
      else	/* 如果是系統文件出錯的話，要補上去 */
      {
	sprintf(film_buf, "請告訴站長檔案 %s 無法開啟", fpath);
	size = strlen(film_buf);
      }
    }

    if (total + size >= MOVIE_SIZE)	/* 若加入這篇會超過全部容量，則不 mirror */
      return 0;

    memcpy(image.film + total, film_buf, size);
    total += size;
  }

  image.shot[++number] = total;

  return 1;
}


static void 
do_gem (		/* itoc.011105: 把看板/精華區的文章收進 movie */
    char *folder		/* index 路徑 */
)
{
  char fpath[64];
  FILE *fp;
  HDR hdr;

  if (fp = fopen(folder, "r"))
  {
    while (fread(&hdr, sizeof(HDR), 1, fp) == 1)
    {
      if (hdr.xmode & (GEM_RESTRICT | GEM_RESERVED | GEM_BOARD | GEM_LINE))	/* 限制級、看板、分隔線 不放入 movie 中 */
	continue;

      hdr_fpath(fpath, folder, &hdr);

      if (hdr.xmode & GEM_FOLDER)	/* 遇到卷宗則迴圈進去收 movie */
      {
	do_gem(fpath);
      }
      else				/* plain text */
      {
	if (!mirror(fpath, MOVIE_LINES))
	  break;
      }
    }
    fclose(fp);
  }
}


static void
lunar_calendar(char *key, time_t *now, struct tm *ptime)	/* itoc.050528: 由陽曆算農曆日期 */
{
#if 0	/* Table 的意義 */

  (1) "," 只是分隔，方便給人閱讀而已
  (2) 前 12 個 byte 分別代表農曆 1-12 月為大月或是小月。"L":大月三十天，"-":小月二十九天
  (3) 第 14 個 byte 代表今年農曆閏月。"X":無閏月，"123456789:;<" 分別代表農曆 1-12 是閏月
  (4) 第 16-20 個 bytes 代表今年農曆新年是哪天。例如 "02/15" 表示陽曆二月十五日是農曆新年

#endif

  #define TABLE_INITAIL_YEAR	2005
  #define TABLE_FINAL_YEAR	2016

  /* 參考 http://sean.o4u.com/ap/calendar/calendar.htm 而得 */
  static const char Table[TABLE_FINAL_YEAR - TABLE_INITAIL_YEAR + 1][21] =
  {
    "-L-L-LL-L-L-,X,02/09",	/* 2005 雞年 */
    "L-L-L-L-LL-L,7,01/29",	/* 2006 狗年 */
    "--L--L-LLL-L,X,02/18",	/* 2007 豬年 */
    "L--L--L-LL-L,X,02/07",	/* 2008 鼠年 */
    "LL--L--L-L-L,5,01/26",	/* 2009 牛年 */
    "L-L-L--L-L-L,X,02/14",	/* 2010 虎年 */
    "L-LL-L--L-L-,X,02/03",	/* 2011 兔年 */
    "L-LLL-L-L-L-,4,01/23",	/* 2012 龍年 */
    "L-L-LL-L-L-L,X,02/10",	/* 2013 蛇年 */
    "-L-L-L-LLL-L,9,01/31",	/* 2014 馬年 */
    "-L--L-LLL-L-,X,02/19",	/* 2015 羊年 */
    "L-L--L-LL-LL,X,02/08",	/* 2016 猴年 */
  };

  char year[21];

  time_t nyd;	/* 今年的農曆新年 */
  struct tm ntime;

  int i;
  int Mon, Day;
  int leap;	/* 0:本年無閏月 */

  /* 先找出今天是農曆哪一年 */

  memcpy(&ntime, ptime, sizeof(ntime));

  for (i = TABLE_FINAL_YEAR - TABLE_INITAIL_YEAR; i >= 0; i--)
  {
    strcpy(year, Table[i]);
    ntime.tm_year = TABLE_INITAIL_YEAR - 1900 + i;
    ntime.tm_mday = atoi(year + 18);
    ntime.tm_mon = atoi(year + 15) - 1;
    nyd = mktime(&ntime);

    if (*now >= nyd)
      break;
  }

  /* 再從農曆正月初一開始數到今天 */

  leap = (year[13] == 'X') ? 0 : 1;

  Mon = Day = 1;
  for (i = (*now - nyd) / 86400; i > 0; i--)
  {
    if (++Day > (year[Mon - 1] == 'L' ? 30: 29))
    {
      Mon++;
      Day = 1;

      if (leap == 2)
      {
	leap = 0;
	Mon--;
	year[Mon - 1] = '-';	/* 閏月必小月 */
      }
      else if (year[13] == Mon + '0')
      {
	if (leap == 1)		/* 下月是閏月 */
	  leap++;
      }
    }
  }

  sprintf(key, "%02d:%02d", Mon, Day);
}


static char feast[OPENING_MAX][64];

static void 
do_today (void)
{
  FILE *fp;
  char buf[256], *str1, *str2, *str3, *today;
  char key1[6];			/* mm/dd: 陽曆 mm月dd日 */
  char key2[6];			/* mm/#A: 陽曆 mm月的第#個星期A */
  char key3[6];			/* MM:DD: 農曆 MM月DD日 */
  time_t now;
  struct tm *ptime;

  time(&now);
  ptime = localtime(&now);
  sprintf(key1, "%02d/%02d", ptime->tm_mon + 1, ptime->tm_mday);
  sprintf(key2, "%02d/%d%c", ptime->tm_mon + 1, (ptime->tm_mday - 1) / 7 + 1, ptime->tm_wday + 'A');
  lunar_calendar(key3, &now, ptime);

  today = image.today;
  sprintf(today, "%s %.2s", key1, "日一二三四五六" + (ptime->tm_wday << 1));
  strcpy(image.date, image.today);

  if (fp = fopen(FN_ETC_FEAST, "r"))
  {
    while (fgets(buf, sizeof(buf), fp))
    {
      if (buf[0] == '#')
	continue;

      if ((str1 = strtok(buf, " \t\n")) && (str2 = strtok(NULL, " \t\n")))
      {
	if (!strcmp(str1, key1) || !strcmp(str1, key2) || !strcmp(str1, key3))
	{
	  char *wd_var;
	  int feast_num = 0;
	  str_ncpy(today, str2, sizeof(image.today));

	  /* dust.120816: 週間日變數代換 */
	  if(wd_var = strstr(today, "%wd"))
	    memcpy(wd_var, " 日 一 二 三 四 五 六" + (ptime->tm_wday * 3), 3);

	  /* 參數解析 */
	  while(str3 = strtok(NULL, " \t\n"))
	  {
	    if(!strcmp(str3, "-otherside"))
	      image.otherside = 1;
	    else if(feast_num < OPENING_MAX)
	    {
	      sprintf(feast[feast_num], "etc/feasts/%s", str3);

	      if(dashf(feast[feast_num]))
		feast_num++;
	      else
		feast[feast_num][0] = '\0';
	    }
	  }

          image.opening_num = feast_num;
	  break;
	}
      }
    }

    fclose(fp);
  }
}


static void 
set_countdown (void)
{
  FILE *fp;
  MCD_info meter;

  if(fp = fopen(FN_ETC_MCD, "rb"))
  {
    fread(&meter, sizeof(MCD_info), 1, fp);
    fclose(fp);
    if(DCD_AlterVer(&meter, NULL) > 0)	/* 尚未截止才有需要放在fshm中 */
      image.counter = meter;
  }
}


int 
main (int argc, char *argv[])
{
  int i;
  char *fname, fpath[64];
  FCACHE *fshm;

  setgid(BBSGID);
  setuid(BBSUID);
  chdir(BBSHOME);

  number = total = 0;

  /* --------------------------------------------------- */
  /* 今天節日					 	 */
  /* --------------------------------------------------- */

  do_today();
  set_countdown();

  /* --------------------------------------------------- */
  /* 載入常用的文件及 help			 	 */
  /* --------------------------------------------------- */

  strcpy(fpath, "gem/@/@");
  fname = fpath + 7;

  for(i = 0; i < FILM_OPENING0; i++)
  {
    strcpy(fname, list[i]);
    mirror(fpath, 0);
  }

  /* 處理開頭畫面 */
  if(feast[0][0])
  {
    int j = 0;

    do
    {
      mirror(feast[j], 0);
      j++;
    }while(feast[j][0]);

    for(; j < OPENING_MAX; j++)	/* dust.101002: 補足數量 */
      mirror(NULL, 0);

    i = FILM_OPENING0 + OPENING_MAX;
  }
  else
  {
    image.opening_num = 0;

    for(; i < FILM_OPENING0 + OPENING_MAX; i++)
    {
      if(list[i])
      {
	strcpy(fname, list[i]);
	mirror(fpath, 0);
	image.opening_num++;
      }
      else
	mirror(NULL, 0);
    }
  }

  for(; i < FILM_MOVIE; i++)
  {
    strcpy(fname, list[i]);
    mirror(fpath, 0);
  }

  /* itoc.註解: 到這裡以後，應該已有 FILM_MOVIE 篇 */

  /* --------------------------------------------------- */
  /* 載入動態看板					 */
  /* --------------------------------------------------- */

  /* itoc.註解: 動態看板及點歌本合計只有 MOVIE_MAX - FILM_MOVIE - 1 篇才會被收進 movie */

  sprintf(fpath, "gem/brd/%s/@/@note", BN_CAMERA);	/* 動態看板的群組檔案名稱應命名為 @note */
  do_gem(fpath);					/* 把 [note] 精華區收進 movie */

#ifdef HAVE_SONG_CAMERA
  brd_fpath(fpath, BN_KTV, FN_DIR);
  do_gem(fpath);					/* 把被點的歌收進 movie */
#endif

  /* --------------------------------------------------- */
  /* resolve shared memory				 */
  /* --------------------------------------------------- */

  image.shot[0] = number;	/* 總共有幾片 */

  fshm = (FCACHE *) shm_new(FILMSHM_KEY, sizeof(FCACHE));
  memcpy(fshm, &image, sizeof(FCACHE));


  /* dust.120813: 應該要parse參數，但我懶了...總之有參數者一律當成不是 crontab 跑的 */
  if(argc == 1)
    fprintf(stderr, "[camera]\t%d/%d films, %.1f/%.0f KB\n", number, MOVIE_MAX, total / 1024.0, MOVIE_SIZE / 1024.0);

  return 0;
}

