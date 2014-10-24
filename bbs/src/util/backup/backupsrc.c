/*-------------------------------------------------------*/
/* util/backupsrc.c	( NTHU MapleBBS Ver 3.10 )       */
/*-------------------------------------------------------*/
/* target : 備份所有原始碼資料                           */
/* create : 07/12/09                                     */
/* update :   /  /                                       */
/* author : pioneerlike@gmail.com       		 */
/*-------------------------------------------------------*/


#include "bbs.h"


int
main()
{
  char bakpath[128], cmd[256];
  time_t now;
  struct tm *ptime;
  DIR *dirp;

  if (chdir(BBSHOME ) || !(dirp = opendir(".")))
    exit(-1);


  /* 建立備份路徑目錄 */
  time(&now);
  ptime = localtime(&now);
  sprintf(bakpath, "%s/src%02d%02d%02d%02d%02d%02d", BAKPATH, ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min, ptime->tm_sec);
  mkdir(bakpath, 0755);

  sprintf(cmd, "tar cfz %s/src.tgz src", bakpath);
  system(cmd);

  exit(0);
}
