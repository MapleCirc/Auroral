/*-------------------------------------------------------*/
/* util/hdrtran.c 					 */
/*-------------------------------------------------------*/
/* target : �ഫ�Ҧ��ݩ�HDR���c�骺������		 */
/* create : 09/12/23					 */
/* update : 09/12/26					 */
/* author : dust.bbs@tcirc.twbbs.org			 */
/*-------------------------------------------------------*/

#include "bbs.h"

//#define TESTING		/* ���եΪ��}���C�}�F�N�u�|�������|�i���ഫ */

/* ----------- XXX: ����e�ȥ��n��o�q�I ----------- */
typedef struct OldHDR
{
  time_t chrono;                /* timestamp */
  int xmode;

  int xid;                      /* reserved */

  char xname[32];               /* �ɮצW�� */
  char owner[80];               /* �@�� (E-mail address) */
  char nick[49];                /* �ʺ� */
  char score;                   /* �峹������� */

  char date[9];                 /* 96/12/31 */
  /* Thor.990329:�S�O�`�N, date�u����ܥ�, ���@���, �H�קK y2k ���D,
                 �w�q 2000�� 00, 2001��01 */

  char title[73];               /* �D�D TTLEN + 1 */
} OLD;

typedef struct NewHDR
{
  time_t chrono;                /* timestamp */
  int xmode;

  int xid;                      /* reserved */
  time_t stamp;                 /* the assistant timestamp */

  char xname[32];               /* �ɮצW�� */
  char owner[80];               /* �@�� (E-mail address) */
  char nick[45];                /* �ʺ� */
  char score;                   /* �峹������� */

  char date[9];                 /* 96/12/31 */

  char title[73];               /* �D�D TTLEN + 1 */
} NEW;


static void
transformation(old, new)	/* �ഫ��ơA�Ш̾�OLD�MNEW�`�c��ӭק� */
  OLD *old;
  NEW *new;
{
  memset(new, 0x00, sizeof(NEW));

  new->chrono = old->chrono;
  new->xmode = old->xmode;
  new->xid = old->xid;

  strcpy(new->xname, old->xname);
  strcpy(new->owner, old->owner);
  strcpy(new->nick, old->nick);
  new->nick[44] = '\0';		/* dust.091226: �s��nick����u�A�����쪺�i��� */
  new->score = old->score;

  strcpy(new->date, old->date);

  strcpy(new->title, old->title);
}


/* ----------- �H�U�����ݧ�� ----------- */
#define FN_HT_TMP "asdfht.tmp"		/* �Ȧs�ɸ��| */

typedef struct
{
  char xname[32];
  int isclass;
} ClassInfo;



static void
transDIR(fpath)
  char *fpath;
{
  FILE *fpr, *fpw;
  NEW new;
  OLD old;
  int write_failed;

  if (!(fpr = fopen(fpath, "rb")))
    return;

  if(!(fpw = fopen(FN_HT_TMP, "wb")))
  {
    printf("%s: error: fail to open tmp file\n", fpath);
    fclose(fpr);
    return;
  }

  write_failed = 0;
  while(fread(&old, sizeof(OLD), 1, fpr) > 0)
  {
    transformation(&old, &new);
    if(fwrite(&new, sizeof(NEW), 1, fpw) != 1)
    {
      write_failed++;
    }
  }

  fclose(fpr);
  fclose(fpw);

  if(write_failed > 0)
    printf("%s: warning: fwrite() seems to have problem (%d)\n", fpath, write_failed);

#ifndef TESTING
  unlink(fpath);
  if(rename(FN_HT_TMP, fpath) < 0)
    printf("%s: error: can\'t rename the file (errno: %d)\n", fpath, errno);
  else
#endif
    printf("The HDR-trasformation of %s succeeded.\n", fpath);
}



ClassInfo *ftable;
int sztable;

static int
CI_cmp(const void *a, const void *b)
{
  return strcmp( ((ClassInfo*)a)->xname, ((ClassInfo*)b)->xname);
}


static void
scanClass(fpath, gpath)
  const char *fpath;
  const char *gpath;
{
  FILE *fp;
  ClassInfo *key;
  char *xname, child[64];
#ifndef TESTING
  NEW hdr;
#else
  OLD hdr;
#endif

  fp = fopen(fpath, "rb");
  if(!fp)
  {
    printf("scanClass(): error: cannot open %s\n", fpath);
    return;
  }

  while(fread(&hdr, sizeof(NEW), 1, fp) > 0)
  {
    xname = hdr.xname;
    if((hdr.xmode & GEM_FOLDER) && *xname == '@')	/* �Ӷ��O���� */
    {
      key = (ClassInfo*) bsearch(xname, ftable, sztable, sizeof(ClassInfo), CI_cmp);
      if(key)
      {
	if(!key->isclass)
	{
	  key->isclass = 1;
	  sprintf(child, "%s/@/%s", gpath, key->xname);
	  transDIR(child);
	  scanClass(child, gpath);
	}
      }
    }
  }

  fclose(fp);
}


static int
pickmouse(gpath)
  const char *gpath;
{
  char dpath[64], *xname;
  struct dirent *de;
  DIR *dirp;
  int size;

  sprintf(dpath, "%s/@", gpath);
  dirp = opendir(dpath);
  if(!dirp)
    return -1;

  size = 100;
  ftable = (ClassInfo*) malloc(sizeof(ClassInfo) * size);
  if(!ftable)
  {
    closedir(dirp);
    return -3;
  }

  sztable = 0;
  while(de = readdir(dirp))
  {
    xname = de->d_name;
    if(*xname == '@')
    {
      if(sztable >= size)	/* space is not enough. */
      {
	size *= 2;
	ftable = (ClassInfo*) realloc(ftable, sizeof(ClassInfo) * size);
	if(!ftable)
	{
	  closedir(dirp);
	  return -3;
	}
      }

      strcpy(ftable[sztable].xname, xname);
      ftable[sztable].isclass = 0;	/* �����]�����O���� */
      sztable++;
    }
  }
  closedir(dirp);

  if(sztable <= 0)
    return -2;

  qsort(ftable, sztable, sizeof(ClassInfo), CI_cmp);
  return 0;
}


static void
gem_trans(gpath, inPersonalGem)
  const char *gpath;
  int inPersonalGem;
{
  char fpath[64], dpath[64], *str;
  int CheckClass = !inPersonalGem;
  int i, len, max;
  struct dirent *de;
  DIR *dirp;

  if(!inPersonalGem)
  {
    i = pickmouse(gpath);	/* �^�ǹs�N������ɮ� */
    if(i != 0)
    {
      if(i == -1)	/* �L�k�}��@/�ؿ� */
	printf("gem_trans(): error: cannot open %s/@/\n", dpath);
      else if(i == -3)
	printf("gem_trans(): fatal error: running out of memory. (ftable)\n");

      CheckClass = 0;
    }
  }

  sprintf(fpath, "%s/%s", gpath, FN_DIR);
  transDIR(fpath);
  if(CheckClass) scanClass(fpath, gpath);

  /* search all folders */
  sprintf(dpath, "%s/*", gpath);
  len = strlen(dpath) - 1;

  max = inPersonalGem? 1 : 32;
  for(i = 0; i < max; i++)
  {
    dpath[len] = radix32[i];
    if (dirp = opendir(dpath))
    {
      while (de = readdir(dirp))
      {
	str = de->d_name;
	if (*str <= ' ' || *str == '.' || *str != 'F' || strlen(str) < 8)
	  continue;

	sprintf(fpath, "%s/%s", dpath, str);
	transDIR(fpath);
	if(CheckClass) scanClass(fpath, gpath);
      }

      closedir(dirp);
    }
    else
      printf("gem_trans(): error: cannot open %s/\n", dpath);
  }

  if(CheckClass)
  {
#ifdef TESTING
    for(i = 0; i < sztable; i++)
    {
      printf("%s/@/%s%s\n", gpath, ftable[i].xname, ftable[i].isclass? " <class>" : "");
    }
#endif
    free(ftable);
    ftable = NULL;
    sztable = 0;
  }
}



int main()
{
  char fpath[64], dpath[64], *str, c;
  struct dirent *de;
  DIR *dirp;
  struct stat st;

#ifdef TESTING
  fprintf(stderr, "Testing Mode.\n");
#endif

  chdir(BBSHOME);
  umask(077);

  if(stat(FN_HT_TMP, &st) == 0)	/* �Y�Ȧs�ɤw�g�s�b���� */
  {
#ifdef TESTING
    unlink(FN_HT_TMP);		/* ���դ��N������F���� =w= */
#else
    fprintf(stderr, "Temporary file existed.\nOperation corrupted.\n");
    return 0;
#endif
  }


  /* HDR files under usr/ */
  sprintf(dpath, "usr/*");
  for(c = 'a'; c <= 'z'; c++)
  {
    dpath[4] = c;
    if (dirp = opendir(dpath))
    {
      while (de = readdir(dirp))
      {
	str = de->d_name;
	if (*str <= ' ' || *str == '.')
	  continue;

	/* user's mailbox */
	sprintf(fpath, "%s/%s/%s", dpath, str, FN_DIR);
	transDIR(fpath);

	/* Personal Gem. */
	sprintf(fpath, "%s/%s/gem", dpath, str);
	gem_trans(fpath, 1);
      }

      closedir(dirp);
    }
    else
      fprintf(stderr, "failed to open directory %s/ !\n", dpath);
  }
  printf("\n");


  /* index of post */
  if (dirp = opendir("brd/"))
  {
    while (de = readdir(dirp))
    {
      str = de->d_name;
      if (*str <= ' ' || *str == '.')
	continue;

      sprintf(fpath, "brd/%s/%s", str, FN_DIR);
      transDIR(fpath);
    }

    closedir(dirp);
  }
  else
    fprintf(stderr, "failed to open directory brd/ !\n");
  printf("\n");


  /* (A)nnounce ��ؤ��G�� */
  gem_trans("gem", 0);
  printf("\n");


  /* Gem of every board */
  if (dirp = opendir("gem/brd"))
  {
    while (de = readdir(dirp))
    {
      str = de->d_name;
      if (*str <= ' ' || *str == '.')
	continue;

      sprintf(fpath, "gem/brd/%s", str);
      gem_trans(fpath, 0);
    }

    closedir(dirp);
  }
  else
    fprintf(stderr, "failed to open directory gem/brd/ !\n");


#ifndef TESTING
  fprintf(stderr, "End.\n");
#endif

  return 0;
}

