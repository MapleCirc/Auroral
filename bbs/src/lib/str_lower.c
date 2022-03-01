void 
str_lower (char *dst, char *src)
{
  int ch;

  do
  {
    ch = *src++;
    if (ch >= 'A' && ch <= 'Z')
      ch |= 0x20;
    *dst++ = ch;
  } while (ch);
}
