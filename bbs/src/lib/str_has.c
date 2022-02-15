int 
str_has (
    char *list,
    char *tag,
    int len		/* strlen(tag) */
)
{
  int cc, priority;
  char *str;

  priority = 1;
  str = tag;
  do
  {
    cc = list[len];	/* itoc.030730.註解: 可能會指到超過 list 的長度以外去了，不過沒差 */
    if ((!cc || cc == '/') && !str_ncmp(list, str, len))
    {
      return priority;
    }
    while (cc = *list++)
    {
      if (cc == '/')
      {
	priority++;
	break;
      }
    }
  } while (cc);

  return 0;
}
