/*-------------------------------------------------------*/
/* date_countdown.c					 */
/*-------------------------------------------------------*/
/* author : pioneerLike (CIRC25th system admin)		 */
/* target : date countdowning calculation		 */
/* create : 08/02/11					 */
/* update : 10/03/12					 */
/*-------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#define BASE_YEAR 2000

#define leap_year(y) (y%4==0 && y%100!=0 || y%400==0 && y%4000!=0)

typedef struct
{
  char name[12];
  int year, mon, day, hr, min;
  int warning;
} MCD_info;

/* timeCounting */
typedef struct
{
  int y, m, d, h, min, s;
} timeC;


int 
MonthToDay (int m, int y)
{
  if (m == 2)
  {
    if (leap_year(y))
      return 29;
    else 
      return 28;
  }
  else if (m == 4 || m == 6 || m == 9 || m == 11)
    return 30;
  else
    return 31;
}


static timeC 
diff_baseDate (int y, int m, int d, int h, int min, int s)
{
  timeC a;
  int i;
	
  a.d = a.s = 0;

  for (i = BASE_YEAR; i < y; i++)
    a.d += leap_year(i)? 366 : 365;

  for (i = 1; i < m; i++)
    a.d += MonthToDay(i, y);

  a.d += d - 1;

  a.s += h * 3600;
  a.s += min * 60;
  a.s += s;

  return a;
}


/* dust.100314: 提供有小時與分的進化版本 */
static int 
countdown_plus (int year, int mon, int day, int hour, int min, char *str, int output_mode)
{
  time_t rawtime;
  struct tm *nowtime;
  timeC a, b;
  timeC c = {0, 0, 0, 0, 0, 0};

  time(&rawtime);
  nowtime = localtime(&rawtime);

  a = diff_baseDate(year, mon, day, hour, min, 0);
  b = diff_baseDate(nowtime->tm_year + 1900, nowtime->tm_mon + 1, nowtime->tm_mday,
		    nowtime->tm_hour, nowtime->tm_min, nowtime->tm_sec);

  if (a.s < b.s)
  {
    a.d--;
    a.s += 86400;
  }

  c.s = a.s - b.s;
  c.d = a.d - b.d;

  if (c.s >= 60)
  {
    c.min += c.s / 60;
    c.s %= 60;
  }

  if (c.min >= 60)
  {
    c.h += c.min / 60;
    c.min %= 60;
  }

  if (c.h >= 24)
  {
    c.d += c.h / 24;
    c.h %= 24;
  }

/* modify the string "str" directly */
/* updated by floatJ: mode 0 顯示全部 1 只顯示天數*/
/* dust.100312: 追加 mode 2 為Menu foot用格式 */

  if(str)
  {
    switch(output_mode)
    {
    case 0:
	sprintf(str, "%d 天 %d 小時 %d 分 %d 秒", c.d, c.h, c.min, c.s);
      break;

    case 1:
      sprintf(str, "%d", c.d);
      break;

    case 2:
      if(c.s > 0)	/* 將秒無條件進位，以免出現0d00h00m */
      {
	c.min++;
	if(c.min >= 60)
	{
	  c.h += c.min / 60;
	  c.min %= 60;
	  if(c.h >= 24)
	  {
	    c.d += c.d / 24;
	    c.h %= 24;
	  }
	}
      }

      sprintf(str, "%dd%02dh%02dm", c.d, c.h, c.min);
      break;
    }
  }

  if(c.d == 0)	/* dust.100314: 修正剩餘不到一天時會變成"已經截止"的bug */
  {
    c.h += c.min + c.s;
    if(c.h > 0) c.d = 1;
  }

  return c.d;
}	


/* return remaining days */
int 
date_countdown (int y, int m, int d, char *str, int output_mode)
{
  return countdown_plus(y, m, d, 0, 0, str, output_mode);
}


int
DCD_AlterVer(
  MCD_info *m,
  char *str
)
{
  return countdown_plus(m->year, m->mon, m->day, m->hr, m->min, str, 2);
}
