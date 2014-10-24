#include "bbs.h"


#define FN_MINE_PREC    "mine.rec"

/* ----------------------------------------------------- */
/* 新的 profile						 */
/* ----------------------------------------------------- */
#define MINE_NITEM 6
struct new_psr{		/* 個人紀錄結構體 */
  int ptimes;
  time_t playtime;
  usint bestime[6];
  int wtimes[6];
  int item[MINE_NITEM];
  int storage[MINE_NITEM];
  short disable_stand;		/* 是否關閉替身人型的使用 */
  short pt_centisec;
};

/* ----------------------------------------------------- */
/* 舊的 profile						 */
/* ----------------------------------------------------- */
#define OLD_MINE_NITEM 5
struct old_psr{		/* 個人紀錄結構體 */
  int ptimes;
  time_t playtime;
  time_t bestime[6];
  int wtimes[6];
  int item[OLD_MINE_NITEM];
  int storage[OLD_MINE_NITEM];
};


/* ----------------------------------------------------- */
/* 轉換主程式						 */
/* ----------------------------------------------------- */


static void
trans_profile(old, new)
  struct old_psr *old;
  struct new_psr *new;
{
  int i;

  memset(new, 0, sizeof(struct new_psr));

  new->ptimes = old->ptimes;
  new->playtime = old->playtime;

  for(i=0;i<6;i++){
    if(old->bestime[i]<0)
      new->bestime[i] = 0;
    else
      new->bestime[i] = (usint)old->bestime[i] * 100;
    new->wtimes[i] = old->wtimes[i];
  }

#define INO_TEPA 2
  for(i=0;i<2;i++){
    new->item[i] = old->item[i];
    new->storage[i] = old->storage[i];
  }
  new->item[i] = new->storage[i] = 0;
  for(i=2;i<5;i++){
    new->item[i+1] = old->item[i];
    new->storage[i+1] = old->storage[i];
  }
}



int
main(argc, argv)
  int argc;
  char *argv[];
{
  struct new_psr new;
  char c;

  if (argc > 2)
  {
    printf("Usage: %s [userid]\n", argv[0]);
    return -1;
  }

  for (c = 'a'; c <= 'z'; c++)
  {
    char buf[64];
    struct dirent *de;
    DIR *dirp;

    sprintf(buf, BBSHOME "/usr/%c", c);
    chdir(buf);

    if (!(dirp = opendir(".")))
      continue;

    while (de = readdir(dirp))
    {
      struct old_psr old;
      int fd;
      char *str;

      str = de->d_name;
      if (*str <= ' ' || *str == '.')
	continue;

      if ((argc == 2) && str_cmp(str, argv[1]))
	continue;

      sprintf(buf, "%s/" FN_MINE_PREC, str);
      if ((fd = open(buf, O_RDONLY)) < 0)
	continue;

      read(fd, &old, sizeof(struct old_psr));
      close(fd);

      trans_profile(&old, &new);

      unlink(buf);
      fd = open(buf, O_WRONLY | O_CREAT, 0600);	/* 重建新的 profile */
      write(fd, &new, sizeof(struct new_psr));
      close(fd);
    }

    closedir(dirp);
  }

  return 0;
}

