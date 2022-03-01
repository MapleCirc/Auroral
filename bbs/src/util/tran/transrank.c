#include"bbs.h"


#define FN_MINE_RANK    "run/minerank"


struct new_rli{		/* 排行榜結構體 */
  char id[IDLEN + 1];
  char date[19];
  usint cost;
} new_ranklist[6][5];


struct old_rli{		/* 排行榜結構體 */
  char id[IDLEN + 1];
  time_t sec;
  char date[19];
} old_ranklist[6][5];


static void 
saverank (void)
{
  FILE *fp;

  if(fp = fopen(FN_MINE_RANK, "wb"))
  {
    fwrite(new_ranklist, 30, sizeof(struct new_rli), fp);
    fclose(fp);
  }
  else
    printf("renew failed\n");
}


static void 
loadrank (void)
{
  FILE *fp;

  if(fp = fopen(FN_MINE_RANK, "rb"))
  {
    fread(old_ranklist, 30, sizeof(struct old_rli), fp);
    fclose(fp);
  }
  else
  {
    printf("corruption.\n");
    exit(0);
  }
}



int 
main (void)
{
  int i, j;

  chdir(BBSHOME);

  loadrank();

  for(i=0;i<6;i++){
    for(j=0;j<5;j++){
      new_ranklist[i][j].cost = (usint)old_ranklist[i][j].sec*100;
      strcpy(new_ranklist[i][j].id, old_ranklist[i][j].id);
      strcpy(new_ranklist[i][j].date, old_ranklist[i][j].date);
    }
  }

  saverank();
  return 0;
}


