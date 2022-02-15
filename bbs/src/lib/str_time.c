#include <time.h>
#include <stdio.h>


static char datemsg[40];


char *
Atime(time_t *clock)	/* Thor.990125: ���� ARPANET �ɶ��榡 */
{
  /* ARPANET format: Thu, 11 Feb 1999 06:00:37 +0800 (CST) */
  /* strftime(datemsg, 40, "%a, %d %b %Y %T %Z", localtime(clock)); */
  /* Thor.990125: time zone���Ǧ^�Ȥ����MARPANET�榡�O�_�@��,���w��,�Psendmail*/
  strftime(datemsg, 40, "%a, %d %b %Y %T +0800 (CST)", localtime(clock));
  return (datemsg);
}


char *
Btime(time_t *clock)	/* BBS �ɶ��榡 */
{
  struct tm *t = localtime(clock);

  sprintf(datemsg, "%d/%02d/%02d %.3s %02d:%02d:%02d",
    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
    "SunMonTueWedThuFriSat" + (t->tm_wday * 3),
    t->tm_hour, t->tm_min, t->tm_sec);
  return (datemsg);
}


char *
Now (void)
{
  time_t now;

  time(&now);
  return Btime(&now);
}
