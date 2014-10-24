/*-------------------------------------------------------*/
/* util/camera.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : �إ� [�ʺA�ݪO] cache			 */
/* create : 95/03/29				 	 */
/* update : 10/10/02			 		 */
/*-------------------------------------------------------*/


#include "bbs.h"


static char *list[] = 		/* src/include/struct.h */
{
  "opening.0",			/* FILM_OPENING0 */
  "opening.1",			/* FILM_OPENING1 */
  "opening.2",			/* FILM_OPENING2 */
  NULL,
  NULL,
  NULL,
  NULL,			/* dust.101002: �s���������ϥ�NULL�@���� */
  NULL,
  NULL,
  NULL,
  "goodbye", 			/* FILM_GOODBYE  */
  "notify",			/* FILM_NOTIFY   */
  "mquota",			/* FILM_MQUOTA   */
  "mailover",			/* FILM_MAILOVER */
  "mgemover",			/* FILM_MGEMOVER */
  "birthday",			/* FILM_BIRTHDAY */
  "justify",			/* FILM_JUSTIFY  */
  "re-reg",			/* FILM_REREG    */
  "e-mail",			/* FILM_EMAIL    */
  "newuser",			/* FILM_NEWUSER  */
  "tryout",			/* FILM_TRYOUT   */
  "post",			/* FILM_POST     */
  "loginmsg",			/* FILM_LOGINMSG */
  "adminnote",			/* FILM_0ADMIN	 */
};



static FCACHE image;
static int number = 0;	/* �ثe�w mirror �X�g�F */
static int total = 0;	/* �ثe�w mirror �X byte �F */


static int		/* 1:���\ 0:�w�W�L�g�Ʃήe�q */
mirror(fpath, line)
  char *fpath;
  int line;		/* 0:�t�Τ��A������C��  !=0:�ʺA�ݪO�Aline �C */
{
  static char film_buf[FILM_SIZ];
  int size;
  FILE *fp;

  /* �Y�w�g�W�L�̤j�g�ơA�h���A�~�� mirror */
  if (number >= MOVIE_MAX - 1)
    return 0;

  if(fpath)
  {
    if(fp = fopen(fpath, "r"))
    {
      char buf[512];
      int y = size = 0;

  /* dust.101002: �]���L�k�I�sEScode()�����Y�A�ҥH�S��k�T�w�@����ܰ_�Ө쩳�|���h�� */
  /* �u�n���]����|��@�檺��ܪ��ױ���b 79char �H��...... */
      while(fgets(buf, sizeof(buf), fp))
      {
	int i = size;

	size += strlen(buf);
	if(size >= FILM_SIZ)
	  break;
	else
	  strcpy(film_buf + i, buf);

	if(line && strchr(buf, '\n'))
	{
	  /* �ʺA�ݪO�A�̦h line �C */
	  if (++y >= line)
	    break;
	}
      }
      fclose(fp);

      if(line)
      {
	/* �ʺA�ݪO�A�Y���� line �C�A�n�� line �C */
	for (; y < line; y++)
	{
	  if(size >= FILM_SIZ) break;

	  film_buf[size++] = '\n';
	}
      }

      if(size >= FILM_SIZ)
      {
	if(line)	/* �ʺA�ݪO�L�j�N�� mirror �F */
	  return 1;

	sprintf(film_buf, "�Чi�D�����ɮ� %s �L�j", fpath);
	size = strlen(film_buf);
      }
    }
    else
    {
      if(line)		/* �p�G�O �ʺA�ݪO/�I�q ���ɮסA�N�� mirror */
	return 1;
      else	/* �p�G�O�t�Τ��X�����ܡA�n�ɤW�h */
      {
	sprintf(film_buf, "�Чi�D�����ɮ� %s �L�k�}��", fpath);
	size = strlen(film_buf);
      }
    }

    if (total + size >= MOVIE_SIZE)	/* �Y�[�J�o�g�|�W�L�����e�q�A�h�� mirror */
      return 0;

    memcpy(image.film + total, film_buf, size);
    total += size;
  }

  image.shot[++number] = total;

  return 1;
}


static void
do_gem(folder)		/* itoc.011105: ��ݪO/��ذϪ��峹���i movie */
  char *folder;		/* index ���| */
{
  char fpath[64];
  FILE *fp;
  HDR hdr;

  if (fp = fopen(folder, "r"))
  {
    while (fread(&hdr, sizeof(HDR), 1, fp) == 1)
    {
      if (hdr.xmode & (GEM_RESTRICT | GEM_RESERVED | GEM_BOARD | GEM_LINE))	/* ����šB�ݪO�B���j�u ����J movie �� */
	continue;

      hdr_fpath(fpath, folder, &hdr);

      if (hdr.xmode & GEM_FOLDER)	/* �J����v�h�j��i�h�� movie */
      {
	do_gem(fpath);
      }
      else				/* plain text */
      {
	if (!mirror(fpath, MOVIE_LINES))
	  break;
      }
    }
    fclose(fp);
  }
}


static void
lunar_calendar(key, now, ptime)	/* itoc.050528: �Ѷ����A���� */
  char *key;
  time_t *now;
  struct tm *ptime;
{
#if 0	/* Table ���N�q */

  (1) "," �u�O���j�A��K���H�\Ū�Ӥw
  (2) �e 12 �� byte ���O�N��A�� 1-12 �묰�j��άO�p��C"L":�j��T�Q�ѡA"-":�p��G�Q�E��
  (3) �� 14 �� byte �N���~�A��|��C"X":�L�|��A"123456789:;<" ���O�N��A�� 1-12 �O�|��
  (4) �� 16-20 �� bytes �N���~�A��s�~�O���ѡC�Ҧp "02/15" ��ܶ���G��Q����O�A��s�~

#endif

  #define TABLE_INITAIL_YEAR	2005
  #define TABLE_FINAL_YEAR	2016

  /* �Ѧ� http://sean.o4u.com/ap/calendar/calendar.htm �ӱo */
  static const char Table[TABLE_FINAL_YEAR - TABLE_INITAIL_YEAR + 1][21] =
  {
    "-L-L-LL-L-L-,X,02/09",	/* 2005 ���~ */
    "L-L-L-L-LL-L,7,01/29",	/* 2006 ���~ */
    "--L--L-LLL-L,X,02/18",	/* 2007 �ަ~ */
    "L--L--L-LL-L,X,02/07",	/* 2008 ���~ */
    "LL--L--L-L-L,5,01/26",	/* 2009 ���~ */
    "L-L-L--L-L-L,X,02/14",	/* 2010 ��~ */
    "L-LL-L--L-L-,X,02/03",	/* 2011 �ߦ~ */
    "L-LLL-L-L-L-,4,01/23",	/* 2012 �s�~ */
    "L-L-LL-L-L-L,X,02/10",	/* 2013 �D�~ */
    "-L-L-L-LLL-L,9,01/31",	/* 2014 ���~ */
    "-L--L-LLL-L-,X,02/19",	/* 2015 �Ϧ~ */
    "L-L--L-LL-LL,X,02/08",	/* 2016 �U�~ */
  };

  char year[21];

  time_t nyd;	/* ���~���A��s�~ */
  struct tm ntime;

  int i;
  int Mon, Day;
  int leap;	/* 0:���~�L�|�� */

  /* ����X���ѬO�A����@�~ */

  memcpy(&ntime, ptime, sizeof(ntime));

  for (i = TABLE_FINAL_YEAR - TABLE_INITAIL_YEAR; i >= 0; i--)
  {
    strcpy(year, Table[i]);
    ntime.tm_year = TABLE_INITAIL_YEAR - 1900 + i;
    ntime.tm_mday = atoi(year + 18);
    ntime.tm_mon = atoi(year + 15) - 1;
    nyd = mktime(&ntime);

    if (*now >= nyd)
      break;
  }

  /* �A�q�A�䥿���@�}�l�ƨ줵�� */

  leap = (year[13] == 'X') ? 0 : 1;

  Mon = Day = 1;
  for (i = (*now - nyd) / 86400; i > 0; i--)
  {
    if (++Day > (year[Mon - 1] == 'L' ? 30: 29))
    {
      Mon++;
      Day = 1;

      if (leap == 2)
      {
	leap = 0;
	Mon--;
	year[Mon - 1] = '-';	/* �|�륲�p�� */
      }
      else if (year[13] == Mon + '0')
      {
	if (leap == 1)		/* �U��O�|�� */
	  leap++;
      }
    }
  }

  sprintf(key, "%02d:%02d", Mon, Day);
}


static char feast[OPENING_MAX][64];

static void
do_today()
{
  FILE *fp;
  char buf[256], *str1, *str2, *str3, *today;
  char key1[6];			/* mm/dd: ���� mm��dd�� */
  char key2[6];			/* mm/#A: ���� mm�몺��#�ӬP��A */
  char key3[6];			/* MM:DD: �A�� MM��DD�� */
  time_t now;
  struct tm *ptime;

  time(&now);
  ptime = localtime(&now);
  sprintf(key1, "%02d/%02d", ptime->tm_mon + 1, ptime->tm_mday);
  sprintf(key2, "%02d/%d%c", ptime->tm_mon + 1, (ptime->tm_mday - 1) / 7 + 1, ptime->tm_wday + 'A');
  lunar_calendar(key3, &now, ptime);

  today = image.today;
  sprintf(today, "%s %.2s", key1, "��@�G�T�|����" + (ptime->tm_wday << 1));
  strcpy(image.date, image.today);

  if (fp = fopen(FN_ETC_FEAST, "r"))
  {
    while (fgets(buf, sizeof(buf), fp))
    {
      if (buf[0] == '#')
	continue;

      if ((str1 = strtok(buf, " \t\n")) && (str2 = strtok(NULL, " \t\n")))
      {
	if (!strcmp(str1, key1) || !strcmp(str1, key2) || !strcmp(str1, key3))
	{
	  char *wd_var;
	  int feast_num = 0;
	  str_ncpy(today, str2, sizeof(image.today));

	  /* dust.120816: �g�����ܼƥN�� */
	  if(wd_var = strstr(today, "%wd"))
	    memcpy(wd_var, " �� �@ �G �T �| �� ��" + (ptime->tm_wday * 3), 3);

	  /* �ѼƸѪR */
	  while(str3 = strtok(NULL, " \t\n"))
	  {
	    if(!strcmp(str3, "-otherside"))
	      image.otherside = 1;
	    else if(feast_num < OPENING_MAX)
	    {
	      sprintf(feast[feast_num], "etc/feasts/%s", str3);

	      if(dashf(feast[feast_num]))
		feast_num++;
	      else
		feast[feast_num][0] = '\0';
	    }
	  }

          image.opening_num = feast_num;
	  break;
	}
      }
    }

    fclose(fp);
  }
}


static void
set_countdown()
{
  FILE *fp;
  MCD_info meter;

  if(fp = fopen(FN_ETC_MCD, "rb"))
  {
    fread(&meter, sizeof(MCD_info), 1, fp);
    fclose(fp);
    if(DCD_AlterVer(&meter, NULL) > 0)	/* �|���I��~���ݭn��bfshm�� */
      image.counter = meter;
  }
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int i;
  char *fname, fpath[64];
  FCACHE *fshm;

  setgid(BBSGID);
  setuid(BBSUID);
  chdir(BBSHOME);

  number = total = 0;

  /* --------------------------------------------------- */
  /* ���Ѹ`��					 	 */
  /* --------------------------------------------------- */

  do_today();
  set_countdown();

  /* --------------------------------------------------- */
  /* ���J�`�Ϊ����� help			 	 */
  /* --------------------------------------------------- */

  strcpy(fpath, "gem/@/@");
  fname = fpath + 7;

  for(i = 0; i < FILM_OPENING0; i++)
  {
    strcpy(fname, list[i]);
    mirror(fpath, 0);
  }

  /* �B�z�}�Y�e�� */
  if(feast[0][0])
  {
    int j = 0;

    do
    {
      mirror(feast[j], 0);
      j++;
    }while(feast[j][0]);

    for(; j < OPENING_MAX; j++)	/* dust.101002: �ɨ��ƶq */
      mirror(NULL, 0);

    i = FILM_OPENING0 + OPENING_MAX;
  }
  else
  {
    image.opening_num = 0;

    for(; i < FILM_OPENING0 + OPENING_MAX; i++)
    {
      if(list[i])
      {
	strcpy(fname, list[i]);
	mirror(fpath, 0);
	image.opening_num++;
      }
      else
	mirror(NULL, 0);
    }
  }

  for(; i < FILM_MOVIE; i++)
  {
    strcpy(fname, list[i]);
    mirror(fpath, 0);
  }

  /* itoc.����: ��o�̥H��A���Ӥw�� FILM_MOVIE �g */

  /* --------------------------------------------------- */
  /* ���J�ʺA�ݪO					 */
  /* --------------------------------------------------- */

  /* itoc.����: �ʺA�ݪO���I�q���X�p�u�� MOVIE_MAX - FILM_MOVIE - 1 �g�~�|�Q���i movie */

  sprintf(fpath, "gem/brd/%s/@/@note", BN_CAMERA);	/* �ʺA�ݪO���s���ɮצW�����R�W�� @note */
  do_gem(fpath);					/* �� [note] ��ذϦ��i movie */

#ifdef HAVE_SONG_CAMERA
  brd_fpath(fpath, BN_KTV, FN_DIR);
  do_gem(fpath);					/* ��Q�I���q���i movie */
#endif

  /* --------------------------------------------------- */
  /* resolve shared memory				 */
  /* --------------------------------------------------- */

  image.shot[0] = number;	/* �`�@���X�� */

  fshm = (FCACHE *) shm_new(FILMSHM_KEY, sizeof(FCACHE));
  memcpy(fshm, &image, sizeof(FCACHE));


  /* dust.120813: ���ӭnparse�ѼơA�����i�F...�`�����Ѽƪ̤@�߷����O crontab �]�� */
  if(argc == 1)
    fprintf(stderr, "[camera]\t%d/%d films, %.1f/%.0f KB\n", number, MOVIE_MAX, total / 1024.0, MOVIE_SIZE / 1024.0);

  return 0;
}

