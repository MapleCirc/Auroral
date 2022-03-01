#include <sys/stat.h>


int 
rec_num (char *fpath, int size)
{
  struct stat st;

  if (stat(fpath, &st))
    return 0;
  return (st.st_size / size);
}
