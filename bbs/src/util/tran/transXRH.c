/*-------------------------------------------------------*/
/* util/transXRH.c	( MapleBBS-itoc-Auroral2 )	 */
/*-------------------------------------------------------*/
/* target : BRH & CRH 轉換程式				 */
/* create : 10/10/31					 */
/* update :   /  /  					 */
/*-------------------------------------------------------*/
/* syntax : transXRH [userid]				 */
/*-------------------------------------------------------*/



#include "bbs.h"


#define FN_TMPXRH "xrh.tmp"

#define BRH_MAX 200
#define CRH_MAX 100

#define	BRH_SIGN 0x80000000


typedef struct BoardReadingHistory
{
  time_t bstamp;		/* 建立看板的時間, unique */
  time_t bvisit;		/* 上次閱讀時間 */
  int bcount;
}BRH;

typedef struct
{
  time_t bstamp;
  int size;
  time_t *list;
}HBR;


typedef struct Comment_Reading_History
{
  time_t chrono;
  time_t stamp;
} CRH;		/* CRH紀錄的本體 */

typedef struct Hearder_of_Comment_Reading_history
{
  time_t bstamp;
  int cnum;
} HCR_old;		/* CRH表頭 */

typedef struct
{
  time_t bstamp;
  int size;
  CRH *list;
} HCR;



static HBR brh_cache[MAXBOARD];
static HCR crh_cache[MAXBOARD];



static int
bstamp_cmp(a, b)
  const time_t *a;
  const time_t *b;
{
  return *a - *b;
}


static int
trans_brh(fpath)
  char *fpath;
{
  FILE *fp;
  BRH header;
  int i, total = 0;
  time_t t;
  short num;

  if(!(fp = fopen(fpath, "rb")))
    return 0;

  printf("  BRH:\n");

  while(fread(&header, sizeof(BRH), 1, fp) > 0)
  {
    if(header.bcount <= 0)
      continue;

    brh_cache[total].bstamp = header.bstamp & ~BRH_SIGN;
    brh_cache[total].size = 0;
    brh_cache[total].list = (time_t*)malloc(sizeof(time_t) * header.bcount);

    while(header.bcount-- > 0)
    {
      if(fread(&t, sizeof(time_t), 1, fp) <= 0)
	break;

      brh_cache[total].list[header.bcount] = t;
      brh_cache[total].size++;
    }

    total++;
  }

  fclose(fp);

  qsort(brh_cache, total, sizeof(HBR), bstamp_cmp);

  if(!(fp = fopen(fpath, "wb")))
  {
    printf("  write BRH... YOOOOOOOOOOOOO!!!!\n");
    return 0;
  }

  for(i = 0; i < total; i++)
  {
    num = brh_cache[i].size;

    fwrite(&brh_cache[i].bstamp, sizeof(time_t), 1, fp);
    fwrite(&num, sizeof(short), 1, fp);
    fwrite(brh_cache[i].list, sizeof(time_t), num, fp);

    printf("    %d success\n", num);
  }

  fclose(fp);

  printf("  total %d boards end.\n", total);
  return 1;
}



static int
trans_crh(char *fpath)
{
  FILE *fp;
  HCR_old header;
  int i, total = 0;
  short num;

  if(!(fp = fopen(fpath, "rb")))
    return 0;

  printf("  CRH:\n");

  while(fread(&header, sizeof(HCR_old), 1, fp) > 0)
  {
    crh_cache[total].bstamp = header.bstamp;
    crh_cache[total].list = (CRH*) malloc(sizeof(CRH) * header.cnum);
    crh_cache[total].size = fread(crh_cache[total].list, sizeof(CRH), header.cnum, fp);

    total++;
  }

  fclose(fp);

  qsort(crh_cache, total, sizeof(HCR), bstamp_cmp);

  if(!(fp = fopen(fpath, "wb")))
  {
    printf("  write CRH... YOOOOOOOOOOOOO!!!!\n");
    return 0;
  }

  for(i = 0; i < total; i++)
  {
    num = crh_cache[i].size;

    fwrite(&crh_cache[i].bstamp, sizeof(time_t), 1, fp);
    fwrite(&num, sizeof(short), 1, fp);
    fwrite(crh_cache[i].list, sizeof(CRH), num, fp);

    printf("    %d success\n", num);
  }

  fclose(fp);

  printf("  total %d boards end.\n\n", total);
  return 1;
}



int
main(argc, argv)
  int argc;
  char *argv[];
{
  char c, *str, fpath[64];
  struct dirent *de;
  DIR *dirp;

  if (argc > 2)
  {
    printf("Usage: %s [userid]\n", argv[0]);
    return -1;
  }

  if(chdir(BBSHOME "/usr/") < 0)
  {
    printf("chdir() failed\n");
    exit(0);
  }

  for (c = 'a'; c <= 'z'; c++)
  {
    sprintf(fpath, "%c/", c);

    if (!(dirp = opendir(fpath)))
      continue;

    while (de = readdir(dirp))
    {
      str = de->d_name;
      if (*str <= ' ' || *str == '.')
	continue;

      if (argc == 2 && str_cmp(str, argv[1]))
	continue;

      printf("%s%s/\n", fpath, str);

      sprintf(fpath + 2, "%s/" FN_BRH, str);
      trans_brh(fpath);

      sprintf(fpath + 2, "%s/" FN_CRH, str);
      trans_crh(fpath);
    }

    closedir(dirp);
  }

  return 0;
}



