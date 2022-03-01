#include "bbs.h"



int 
main (void)
{
  int fd;
  char fnew[64], fpath[64];
  SCHEMA schema;
  int userno;
  FILE *fp;

  chdir(BBSHOME);

  if ((fd = open(FN_SCHEMA, O_RDONLY)) >= 0)
  {
    strcpy(fpath, FN_SCHEMA);
    fp = f_new(fpath, fnew);
    if(!fp)
      exit(0);
    fclose(fp);

    userno = 0;

    while (read(fd, &schema, sizeof(SCHEMA)) == sizeof(SCHEMA))
    {
      userno++;
      schema.userno = userno;

      rec_add(fnew, &schema, sizeof(SCHEMA));
    }
    close(fd);

    unlink(FN_SCHEMA);
    rename(fnew, FN_SCHEMA);

    printf("max uno = %d\n", userno);
  }

  return 0;
}

