/*-------------------------------------------------------*/
/* util/expire.c					 */
/*-------------------------------------------------------*/
/* target : �۰ʬ�H�u��{��				 */
/* create : 95/03/29				 	 */
/* remake : 12/10/05					 */
/* update : 12/10/13				 	 */
/*-------------------------------------------------------*/


#if 0

�s�� util/expire ���u�@�G

  1."expire": �M�z�L�ª��峹�P�L�h���峹

  2."sync": �P�B���޻P�w�Ф����ɮסA�R�����Ǥ��b���޸̪��ɮ�

  3.�M�z�ݪO�\Ū����(�u�bexpire�S���[�W�ѼƮɰ���)

  4.�ˬd�b"�ӤH"���������ݪO���L"�ӤH�O"�ݩ�

#endif



#include "bbs.h"


/* �h�֤ѫe���峹�|�Q�M�� */
#define	DEF_DAYS	2556

/* �ݪO�峹�ƤW���A���󦹼ƭȤ��׮ɶ��@�߲M�� */
#define	DEF_MAXP	65536

/* �ݪO�峹�ƤU���A�C�󦹼ƭȤ����M���峹 */
#define	DEF_MINP	9999

/* �h�[�H���ͦ����ɮפ��ݭnsync (����) */
#define NO_SYNC_TIME	3600

/* �ݪO�\Ū���� */
#define BRD_USIES_LIMIT	1048576


#define	EXPIRE_LOG	"run/expire.log"
#define expire_log(...) fprintf(flog, __VA_ARGS__)



/* ------------------------------------- */
/* Life ���c��G�ݪO�峹�s���ɶ��]�w	 */
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
Life_cmp (Life *a, Life *b)
{
  return str_cmp(a->bname, b->bname);
}


/* ------------------------------------- */
/* SyncData ���c��Gsync��		 */
/* ------------------------------------- */

typedef struct
{
  time_t chrono;
  char prefix;		/* �ɮת� family */
  char exotic;		/* 1:���b.DIR��  0:�b.DIR�� */
} SyncData;


static int 
SyncData_cmp (SyncData *a, SyncData *b)
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



/* sync �����G
  sync_init() �|�� brd/<brd name>/_/ �U�Ҧ��ɮץ�i sync_pool[]�A
  �M��b expire() �ˬd���ɮ׬O�_�b .DIR ���A�Y�b .DIR ���N�� exotic �]�^ 0
  �̫�b sync_check() ���� sync_pool[] �� exotic �٬O 1 ���ɮ׳��R�� */

static int 
sync_init (char *fname)
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

	if (chrono > synctime)	/* �o�O�̪��o���峹�A���ݭn�[�J sync_pool[] */
	  continue;

	if (sync_head >= sync_size)
	{
	  sync_size += (sync_size >> 1);
	  sync_pool = (SyncData *) realloc(sync_pool, sync_size * sizeof(SyncData));

	  if(!sync_pool)	/* �O���魫�t�m���ѡA��������sync */
	  {
	    expire_log("\tsync pool realloc fail (%d bytes)\n", sync_size);
	    return -1;
	  }
	}

	sync_pool[sync_head].chrono = chrono;
	sync_pool[sync_head].prefix = prefix;
	sync_pool[sync_head].exotic = 1;	/* �������O�� 1�A�� expire() ���A��^ 0 */
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
sync_check(FILE *flog, char *fname)
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
expire (Life *life, usint battr, int sync)
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
      sync = 0;		/* ��l�ƥ��ѡA��sync */
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
	xsync->exotic = 0;	/* �o�g�b .DIR ���A�� sync */
      }
      else
	keep = 0;		/* �w�Ф��S������(���Y��)�A���O�d */
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

  /* expire�ᤴ�W�Xmaxp�A�i�J�j��expire�Ҧ� */
  if(total > maxp && !(battr & BRD_NOEXPIRE || no_expire_post))
  {
    expire_log("\tstill over maxp (%d)", total);

    if(fpw = fopen(fnew, "r+b"))
    {
      int disp = 0;

      expire_log(", start enforce expire mode\n");

      do	/* �q�Y�}�l�@�g�g���ŦXmaxp */
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

      /* record�V�e�� */
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

  if (done)	/* �����g�J.DIR */
  {
    sprintf(fpath, "%s.o", index);
    if (!rename(index, fpath))
    {
      if (rename(fnew, index))
	rename(fpath, index);	/* .DIR.n -> .DIR ��W���ѡA���^�� */
    }
  }
  unlink(fnew);

  expire_log("\texpire %d files\n", origin_total - total);

  if (sync)
    sync_check(flog, bname);
}



static void 
expire_board_usies (char *bname)
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
    /* ����U�Ӵ���A�O�Ҿ\Ū�������}�Y�O���㪺�@�� */
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
dummy_config (	/* �ˬd�O�_�����s�b���]�w */
    Life *conf_ptr,
    Life *conf_end
)
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
main (int argc, char *argv[])
{
  int ch, bno, sync_counter, conf_size = 0;
  Life *key, db = {"", DEF_DAYS, DEF_MAXP, DEF_MINP};
  BCACHE *bshm;
  FILE *fp;
  char buf[256];
  BRD *brd;


  /* �ѼƤ��R */
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
    flog = stdout;	/* �Ylog�ɫإߥ��ѴN�L��stdout�h */

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


  /* ���Jexpire.conf */

  if(fp = fopen(FN_ETC_EXPIRE, "r"))
  {
    char *bname, *str;
    int n;

    while(fgets(buf, sizeof(buf), fp))
    {
      if(buf[0] == '#')
	continue;

      bname = strtok(buf, " \t\n");
      if(bname && *bname)		/* �إ߭ӧO�ݪO���]�w */
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


    /* �ˬd�ݩ�"�ӤH"�������ݪO���LBRD_PERSONAL */
    if(!(brd[bno].battr & BRD_PERSONAL) && !strcmp(brd[bno].class, "�ӤH"))
    {
      brd[bno].battr |= BRD_PERSONAL;
      expire_log("\tmissing BRD_PERSONAL\n");
      rec_put(BBSHOME "/" FN_BRD, &(brd[bno]), sizeof(BRD), bno, NULL);
    }

    /* expire+sync */
    strcpy(key->bname, brd[bno].brdname);
    expire(key, brd[bno].battr, !(sync_counter % 16) || enforce_sync);

    /* �ݪO�\Ū�����M�z */
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


