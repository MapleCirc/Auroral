#include "bbs.h"


static void
transformation(HDR *old, HDR *new)
{
  memcpy(new, old, sizeof(HDR));

  if(new->xmode & POST_RESERVED)
    new->xmode &= ~POST_MARKED;
}


#define FN_HT_TMP ".postdir.tmp"		/* 暫存檔路徑 */

static void 
transDIR (char *fpath)
{
  FILE *fpr, *fpw;
  HDR new;
  HDR old;
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
  while(fread(&old, sizeof(HDR), 1, fpr) > 0)
  {
    transformation(&old, &new);
    if(fwrite(&new, sizeof(HDR), 1, fpw) != 1)
    {
      write_failed++;
    }
  }

  fclose(fpr);
  fclose(fpw);

  if(write_failed > 0)
    printf("%s: warning: fwrite() seems to have problem (%d)\n", fpath, write_failed);

  unlink(fpath);
  if(rename(FN_HT_TMP, fpath) < 0)
    printf("%s: error: can\'t rename the file (errno: %d)\n", fpath, errno);
}



int 
main (void)
{
  char fpath[64], *str;
  struct dirent *de;
  DIR *dirp;
  struct stat st;


  chdir(BBSHOME);
  umask(077);

  if(stat(FN_HT_TMP, &st) == 0)	/* 若暫存檔已經存在的話 */
  {
    fprintf(stderr, "Temporary file existed.\nOperation corrupted.\n");
    return 0;
  }

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


  return 0;
}

