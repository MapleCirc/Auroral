int
m3_tolower(ch)
  int ch;
{
  return (ch >= 'A' && ch <= 'Z')? ch | 0x20 : ch;
}
