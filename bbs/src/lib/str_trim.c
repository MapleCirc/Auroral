void 
str_trim (			/* remove trailing space */
    char *buf
)
{
  char *p = buf;

  while (*p)
    p++;
  while (--p >= buf)
  {
    if (*p == ' ')
      *p = '\0';
    else
      break;
  }
}
