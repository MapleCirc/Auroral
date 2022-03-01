/*-------------------------------------------------------*/
/* util/backupsrc.c	( NTHU MapleBBS Ver 3.10 )       */
/*-------------------------------------------------------*/
/* target : �ƥ��Ҧ���l�X���                           */
/* create : 07/12/09                                     */
/* update :   /  /                                       */
/* author : pioneerlike@gmail.com       		 */
/*-------------------------------------------------------*/


#include "bbs.h"


int 
main (void)
{
  char bakpath[128], cmd[256];
  time_t now;
  struct tm *ptime;
  DIR *dirp;

  if (chdir(BBSHOME ) || !(dirp = opendir(".")))
    exit(-1);


  /* �إ߳ƥ����|�ؿ� */
  time(&now);
  ptime = localtime(&now);
  sprintf(bakpath, "%s/src%02d%02d%02d%02d%02d%02d", BAKPATH, ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min, ptime->tm_sec);
  mkdir(bakpath, 0755);

  sprintf(cmd, "tar cfz %s/src.tgz src", bakpath);
  system(cmd);

  exit(0);
}
