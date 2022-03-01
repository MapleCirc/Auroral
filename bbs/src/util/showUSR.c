/*-------------------------------------------------------*/
/* util/showUSR.c       ( NTHU CS MapleBBS Ver 3.00 )    */
/*-------------------------------------------------------*/
/* target : ¨q¥X FN_SCHEMA                               */
/* create :   /  /                                       */
/* update : 02/11/03                                     */
/*-------------------------------------------------------*/
/* syntax : showUSR                                      */
/*-------------------------------------------------------*/


#include "bbs.h"


int 
main (void)
{
  int fd, n;
  SCHEMA *usr;
  struct stat st;

  chdir(BBSHOME);

  if ((fd = open(FN_SCHEMA, O_RDONLY)) < 0)
  {
    printf("ERROR at open file\n");
    exit(1);
  }

  fstat(fd, &st);
  usr = (SCHEMA *) malloc(st.st_size);
  read(fd, usr, st.st_size);
  close(fd);

  printf("\n%s  ==> %d bytes\n", FN_SCHEMA, st.st_size);

  fd = st.st_size / sizeof(SCHEMA);
  for (n = 0; n < fd; n++)
  {
    printf("userno:%d  ID:%-12.12s\n", usr[n].userno, usr[n].userid);
  }

  free(usr);
  return 0;
}
