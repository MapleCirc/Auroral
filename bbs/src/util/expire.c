/*-------------------------------------------------------*/
/* util/expire.c					 */
/*-------------------------------------------------------*/
/* target : 自動砍信工具程式				 */
/* create : 95/03/29				 	 */
/* remake : 12/10/05					 */
/* update : 12/10/13				 	 */
/*-------------------------------------------------------*/


#if 0

新版 util/expire 的工作：

  1."expire": 清理過舊的文章與過多的文章

  2."sync": 同步索引與硬碟中的檔案，刪掉那些不在索引裡的檔案

  3.清理看板閱讀紀錄(只在expire沒有加上參數時執行)

  4.檢查在"個人"分類中的看板有無"個人板"屬性

#endif



#include "bbs.h"


/* 多少天前的文章會被清除 */
#define	DEF_DAYS	2556

/* 看板文章數上限，高於此數值不論時間一律清除 */
#define	DEF_MAXP	65536

/* 看板文章數下限，低於此數值不須清除文章 */
#define	DEF_MINP	9999

/* 多久以內生成的檔案不需要sync (單位秒) */
#define NO_SYNC_TIME	3600

/* 看板閱讀紀錄 */
#define BRD_USIES_LIMIT	1048576


#define	EXPIRE_LOG	"run/expire.log"
#define expire_log(...) fprintf(flog, __VA_ARGS__)



/* ------------------------------------- */
/* Life 結構體：看板文章存活時間設定	 */
/* ------------------------------------- */

typedef struct
{
  char bname[BNLEN + 1];	/* board name */
  int days;
  int maxp;
  int minp;
  int exist;
} Life;

static int
Life_cmp(a, b)
  Life *a, *b;
{
  return str_cmp(a->bname, b->bname);
}


/* ------------------------------------- */
/* SyncData 結構體：sync用		 */
/* ------------------------------------- */

typedef struct
{
  time_t chrono;
  char prefix;		/* 檔案的 family */
  char exotic;		/* 1:不在.DIR中  0:在.DIR中 */
} SyncData;


static int
SyncData_cmp(a, b)
  SyncData *a, *b;
{
  return a->chrono - b->chrono;
}



static int enforce_sync = 0;
static int no_expire_post = 0;
static FILE *flog;
static Life table[MAXBOARD];

static SyncData *sync_pool;
static int sync_size, sync_head;
static time_t startime, synctime;



/* sync 說明：
  sync_init() 會把 brd/<brd name>/_/ 下所有檔案丟進 sync_pool[]，
  然後在 expire() 檢查該檔案是否在 .DIR 中，若在 .DIR 中就把 exotic 設回 0
  最後在 sync_check() 中把 sync_pool[] 裡 exotic 還是 1 的檔案都刪除 */

static int	/* -1: init失敗，取消sync */
sync_init(fname)
  char *fname;
{
  int ch, prefix;
  time_t chrono;
  char *str, fpath[80];
  struct dirent *de;
  DIR *dirp;


  if(!sync_pool)
  {
    sync_size = 16384;
    sync_pool = (SyncData *) malloc(sync_size * sizeof(SyncData));

    if(!sync_pool)
    {
      expire_log("\tsync pool alloc fail\n");
      return -1;
    }
  }

  sync_head = 0;

  ch = strlen(fname);
  memcpy(fpath, fname, ch);
  fname = fpath + ch;
  *fname++ = '/';
  fname[1] = '\0';

  ch = '0';
  for (;;)
  {
    *fname = ch++;

    if (dirp = opendir(fpath))
    {
      while (de = readdir(dirp))
      {
	str = de->d_name;
	prefix = *str;
	if (prefix == '.')
	  continue;

	chrono = chrono32(str);

	if (chrono > synctime)	/* 這是最近剛發表的文章，不需要加入 sync_pool[] */
	  continue;

	if (sync_head >= sync_size)
	{
	  sync_size += (sync_size >> 1);
	  sync_pool = (SyncData *) realloc(sync_pool, sync_size * sizeof(SyncData));

	  if(!sync_pool)	/* 記憶體重配置失敗，取消本次sync */
	  {
	    expire_log("\tsync pool realloc fail (%d bytes)\n", sync_size);
	    return -1;
	  }
	}

	sync_pool[sync_head].chrono = chrono;
	sync_pool[sync_head].prefix = prefix;
	sync_pool[sync_head].exotic = 1;	/* 先全部令為 1，於 expire() 中再改回 0 */
	sync_head++;
      }

      closedir(dirp);
    }

    if (ch == 'W')
      break;

    if (ch == '9' + 1)
      ch = 'A';
  }

  if (sync_head > 1)
    qsort(sync_pool, sync_head, sizeof(SyncData), SyncData_cmp);

  return 0;
}


static void
sync_check(flog, fname)
  FILE *flog;
  char *fname;
{
  char *str, fpath[80];
  SyncData *xpool, *xtail;
  time_t cc;

  if(sync_head <= 0)
    return;

  xpool = sync_pool;
  xtail = xpool + sync_head;

  sprintf(fpath, "%s/ /", fname);
  str = strchr(fpath, ' ');
  fname = str + 3;

  do
  {
    if (xtail->exotic)
    {
      cc = xtail->chrono;
      fname[-1] = xtail->prefix;
      *str = radix32[cc & 31];
      archiv32(cc, fname);
      unlink(fpath);

      expire_log("-\t%s\n", fpath);
    }
  } while (--xtail >= xpool);
}


static void
expire(life, battr, sync)
  Life *life;
  usint battr;
  int sync;
{
  HDR hdr;
  struct stat st;
  char fpath[128], fnew[128], index[128], *fname, *bname, *str;
  int done, keep, total, origin_total;
  FILE *fpr, *fpw;

  int days, maxp, minp;
  time_t duetime;

  SyncData *xpool, *xsync;
  int xhead;


  days = life->days;
  maxp = life->maxp;
  minp = life->minp;
  bname = life->bname;

  sprintf(index, "%s/.DIR", bname);
  if(!(fpr = fopen(index, "r")))
  {
    expire_log("\topen DIR fail: %s\n", index);
    return;
  }

  fpw = f_new(index, fnew);
  if(!fpw)
  {
    expire_log("\tExclusive lock: %s\n", fnew);
    fclose(fpr);
    return;
  }

  if(sync)
  {
    if(sync_init(bname) < 0)
      sync = 0;		/* 初始化失敗，不sync */
    else
    {
      xpool = sync_pool;
      xhead = sync_head;
      if (xhead <= 0)
	sync = 0;
      else
	expire_log("\t%d files to sync\n", xhead);
    }
  }

  strcpy(fpath, index);
  str = strrchr(fpath, '.');
  fname = str + 1;
  *fname++ = '/';

  done = 1;
  duetime = startime - days * 86400;

  if(fstat(fileno(fpr), &st) != 0)
  {
    expire_log("\tstate .DIR fail, errno=%d\n", errno);
    return;
  }

  origin_total = total = st.st_size / sizeof(hdr);

  while (fread(&hdr, sizeof(hdr), 1, fpr) == 1)
  {
    if(battr & BRD_NOEXPIRE || no_expire_post)
      keep = 1;
    else if(total <= minp)
      keep = 1;
    else if(hdr.xmode & (POST_MARKED | POST_BOTTOM))
      keep = 1;
    else if(total > maxp)
      keep = 0;
    else if(hdr.chrono < duetime && !(battr & BRD_PERSONAL))
      keep = 0;
    else
      keep = 1;

    if(sync && (hdr.chrono < synctime))
    {
      if(xsync = (SyncData *) bsearch(&hdr.chrono, xpool, xhead, sizeof(SyncData), SyncData_cmp))
      {
	xsync->exotic = 0;	/* 這篇在 .DIR 中，不 sync */
      }
      else
	keep = 0;		/* 硬碟中沒有對應(石頭文)，不保留 */
    }

    if(keep)
    {
      if(fwrite(&hdr, sizeof(hdr), 1, fpw) != 1)
      {
	expire_log("\tError in write DIR.n: %s\n", hdr.xname);
	done = 0;
	sync = 0;
	break;
      }
    }
    else
    {
      *str = hdr.xname[7];
      strcpy(fname, hdr.xname);
      unlink(fpath);
      expire_log("\t%s\n", fname);
      total--;
    }
  }

  fclose(fpr);
  fclose(fpw);

  /* expire後仍超出maxp，進入強制expire模式 */
  if(total > maxp && !(battr & BRD_NOEXPIRE || no_expire_post))
  {
    expire_log("\tstill over maxp (%d)", total);

    if(fpw = fopen(fnew, "r+b"))
    {
      int disp = 0;

      expire_log(", start enforce expire mode\n");

      do	/* 從頭開始一篇篇砍到符合maxp */
      {
	if(fread(&hdr, sizeof(hdr), 1, fpw) != 1)
	{
	  expire_log("\tUnexpected error interrupted enforce expire\n");
	  fseek(fpw, -sizeof(HDR), SEEK_CUR);
	  break;
	}

	*str = hdr.xname[7];
	strcpy(fname, hdr.xname);
	unlink(fpath);
	expire_log("\t%s\n", fname);
	disp++;
      }while(--total > maxp);

      disp *= sizeof(HDR);

      /* record向前移 */
      while(fread(&hdr, sizeof(hdr), 1, fpw) == 1)
      {
	fseek(fpw, -disp - sizeof(HDR), SEEK_CUR);
	if(fwrite(&hdr, sizeof(hdr), 1, fpw) != 1)
	{
	  expire_log("\t[enforce mode]error in write DIR.n: %s\n", hdr.xname);
	  done = 0;
	  sync = 0;
	  break;
	}

	fseek(fpw, disp, SEEK_CUR);
      }

      ftruncate(fileno(fpw), total * sizeof(HDR));
      fclose(fpw);
    }
    else
      expire_log(", but error to open: %s\n", fnew);
  }

  if (done)	/* 正式寫入.DIR */
  {
    sprintf(fpath, "%s.o", index);
    if (!rename(index, fpath))
    {
      if (rename(fnew, index))
	rename(fpath, index);	/* .DIR.n -> .DIR 更名失敗，換回來 */
    }
  }
  unlink(fnew);

  expire_log("\texpire %d files\n", origin_total - total);

  if (sync)
    sync_check(flog, bname);
}



static void
expire_board_usies(bname)
  char *bname;
{
  char fpath[64], fnew[64], buf[256];
  struct stat st;
  FILE *fpw, *fpr;


  sprintf(fpath, "%s/usies", bname);

  if(stat(fpath, &st) == 0)
    expire_log("\tusies: %d bytes\n", st.st_size);
  else
  {
    expire_log("\tcannot state %s\n", fpath);
    return;
  }

  if(st.st_size > BRD_USIES_LIMIT)
  {
    fpr = fopen(fpath, "r");
    if(!fpr)
    {
      expire_log("\terror read file: %s\n", fpath);
      return;
    }

    fpw = f_new(fpath, fnew);
    if(!fpw)
    {
      expire_log("\terror write file: %s\n", fnew);
      fclose(fpr);
      return;
    }

    fseek(fpr, st.st_size - BRD_USIES_LIMIT - 1, SEEK_SET);
    /* 跳到下個換行，保證閱讀紀錄的開頭是完整的一行 */
    while(fgets(buf, sizeof(buf), fpr))
    {
      if(buf[strlen(buf) - 1] == '\n') break;
    }

    while(fgets(buf, sizeof(buf), fpr))
      fputs(buf, fpw);

    fclose(fpr);
    fclose(fpw);

    rename(fnew, fpath);
  }
}


static void
dummy_config(conf_ptr, conf_end)	/* 檢查是否有不存在的設定 */
  Life *conf_ptr;
  Life *conf_end;
{
  int n = 0;

  while(conf_ptr < conf_end)
  {
    if(!conf_ptr->exist)
      n++;

    conf_ptr++;
  }

  if(n > 0)
    expire_log("Warning: %d unused Life config\n", n);
}



int
main(argc, argv)
  int argc;
  char *argv[];
{
  int ch, bno, sync_counter, conf_size = 0;
  Life *key, db = {"", DEF_DAYS, DEF_MAXP, DEF_MINP};
  BCACHE *bshm;
  FILE *fp;
  char buf[256];
  BRD *brd;


  /* 參數分析 */
  while((ch = getopt(argc, argv, "sp")) != -1)
  {
    switch(ch)
    {
    case 's':
      enforce_sync = 1;
      break;

    case 'p':
      no_expire_post = 1;
      break;

    default:
      printf("usage: expire [options]\n\n");
      printf("  -s : enforce sync all boards\n");
      printf("  -p : do not expire any post\n");
      exit(0);
    }
  }


  setgid(BBSGID);
  setuid(BBSUID);
  chdir(BBSHOME);

  flog = fopen(EXPIRE_LOG, "w");
  if(!flog)
    flog = stdout;	/* 若log檔建立失敗就印到stdout去 */

  time(&startime);
  expire_log("expire launched at %s\n", Btime(&startime));

  if(enforce_sync)
    expire_log("\twith option: enforce sync\n");
  if(no_expire_post)
    expire_log("\twith option: do not expire post\n");

  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));
  if(bshm->uptime <= 0)
  {
    expire_log("BSHM is not ready!\n");
    exit(0);
  }


  /* 載入expire.conf */

  if(fp = fopen(FN_ETC_EXPIRE, "r"))
  {
    char *bname, *str;
    int n;

    while(fgets(buf, sizeof(buf), fp))
    {
      if(buf[0] == '#')
	continue;

      bname = strtok(buf, " \t\n");
      if(bname && *bname)		/* 建立個別看板的設定 */
      {
	strcpy(table[conf_size].bname, bname);

	str = strtok(NULL, " \t\n");
	if(str && (n = atoi(str)) > 0)
	  table[conf_size].days = n;
	else
	  table[conf_size].days = DEF_DAYS;

	str = strtok(NULL, " \t\n");
	if(str && (n = atoi(str)) > 0)
	  table[conf_size].maxp = n;
	else
	  table[conf_size].maxp = DEF_MAXP;

	str = strtok(NULL, " \t\n");
	if(str && (n = atoi(str)) > 0)
	  table[conf_size].minp = n;
	else
	  table[conf_size].minp = DEF_MINP;

	table[conf_size].exist = 0;
	conf_size++;
	if(conf_size >= MAXBOARD)
	  break;
      }
    }
    fclose(fp);

    if(conf_size > 1)
      qsort(table, conf_size, sizeof(Life), Life_cmp);

    expire_log("%d Life config loaded\n", conf_size);
  }
  else
    expire_log("cannot open " FN_ETC_EXPIRE "\n");
  expire_log("\n");


  /* visit all boards */

  synctime = startime - NO_SYNC_TIME;
  sync_counter = startime / 86400;

  chdir("brd");

  brd = bshm->bcache;
  for(bno = 0; bno < bshm->number; bno++)
  {
    if(brd[bno].brdname[0] == '\0')
      continue;

    expire_log("%s\n", brd[bno].brdname);

    /* search for Life config */
    if(conf_size > 0)
    {
      key = (Life *) bsearch(brd[bno].brdname, table, conf_size, sizeof(Life), Life_cmp);

      if(!key)
	key = &db;
      else
      {
	key->exist = 1;	/* for dummy Life config check */
	expire_log("\tdays=%d  maxp=%d  minp=%d\n", key->days, key->maxp, key->minp);
      }
    }
    else
      key = &db;


    /* 檢查屬於"個人"分類的看板有無BRD_PERSONAL */
    if(!(brd[bno].battr & BRD_PERSONAL) && !strcmp(brd[bno].class, "個人"))
    {
      brd[bno].battr |= BRD_PERSONAL;
      expire_log("\tmissing BRD_PERSONAL\n");
      rec_put(BBSHOME "/" FN_BRD, &(brd[bno]), sizeof(BRD), bno, NULL);
    }

    /* expire+sync */
    strcpy(key->bname, brd[bno].brdname);
    expire(key, brd[bno].battr, !(sync_counter % 16) || enforce_sync);

    /* 看板閱讀紀錄清理 */
    if(!(sync_counter % 16) && !(enforce_sync || no_expire_post))
      expire_board_usies(brd[bno].brdname);

    sync_counter++;
    brd[bno].btime = -1;
  }


  expire_log("\n\ntime elapsed: %d sec.\n", time(NULL) - startime);
  dummy_config(table, table + conf_size);
  fclose(flog);

  return 0;
}


