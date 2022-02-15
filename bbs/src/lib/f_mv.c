#include <fcntl.h>


int
f_mv(char *src, char *dst)
{
  int ret;

  if (ret = rename(src, dst))
  {
    ret = f_cp(src, dst, O_TRUNC);
    if (!ret)
      unlink(src);
  }
  return ret;
}
