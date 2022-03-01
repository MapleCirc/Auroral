/*-------------------------------------------------------*/
/* util/rform_dis.c					 */
/*-------------------------------------------------------*/
/* target : 將rform.log拆到各個使用者的目錄下		 */
/* create : 11/01/03					 */
/* update :   /  /					 */
/*-------------------------------------------------------*/
/* notice : This file should be removed after finished	 */
/*-------------------------------------------------------*/



#include "bbs.h"


#define FN_RUN_RFORM_LOG_OLD "run/rform.log"


int 
main (void)
{
  RFORM rform;
  FILE *fp;
  int counter = 0;

  chdir(BBSHOME);

  fp = fopen(FN_RUN_RFORM_LOG_OLD, "rb");
  if(!fp)
  {
    printf("cannot open " FN_RUN_RFORM_LOG_OLD "\n");
    return 0;
  }

  while(fread(&rform, sizeof(RFORM), 1, fp) == 1)
  {
    char fpath[64];
    FILE *fpx;

    usr_fpath(fpath, rform.userid, "");
    if(!dashd(fpath))
      continue;

    strcat(fpath, FN_RFINFO);
    if(!(fpx = fopen(fpath, "w")))
      continue;

    fprintf(fpx, "%s\n", rform.phone);
    fprintf(fpx, "%s\n", rform.address);

    fclose(fpx);
    counter++;
  }

  printf("%d record done.\n", counter);

  fclose(fp);
  return 0;
}

