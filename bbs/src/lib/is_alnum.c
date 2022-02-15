int 
is_alnum (int ch)
{
  return ((ch >= '0' && ch <= '9') ||
    (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'));
}
