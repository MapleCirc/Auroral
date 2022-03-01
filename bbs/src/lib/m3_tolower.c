int 
m3_tolower (int ch)
{
  return (ch >= 'A' && ch <= 'Z')? ch | 0x20 : ch;
}
