/*-------------------------------------------------------*/
/* util/reaper.c					 */
/*-------------------------------------------------------*/
/* target : �ϥΪ̱b���w���M�z				 */
/* create : 95/03/29				 	 */
/* remake : 12/10/08				 	 */
/* update : 12/10/19				 	 */
/*-------------------------------------------------------*/



#include "bbs.h"



/* �@��ϥΪ̦ܤ֫O�d�T�~�b */
#define LIVETIME_BASE	42

/* �O�d�ɶ��̪�20�~ */
#define LIVETIME_LIMIT	240

/* �ݭn�Q�έp���S���v�� */
#define SPECIAL_PERM_LIST	{PERM_ALLADMIN, PERM_XEMPT, PERM_MBOX}
#define SPECIAL_PERM_NAME	{"����", "�ä[�O�d�b��", "�H��L�W��"}
#define SPECIAL_PERM_NUM	3


#define REAPER_LOG      "run/reaper.log"
#define reaper_log(...) fprintf(flog, __VA_ARGS__)
#define MONTH_TO_SEC(mon)	((time_t)((double) (mon) / 12 * 31556926))



static time_t startime;

static FILE *flog;
static FILE *f_usr_expired, *f_special_perm, *f_changed_bm;
static int fd_schema;
static BCACHE *bshm;
static char date_postfix[16];

static int prune = 0;
static int visit = 0;

static char BMpool[MAXBOARD * BMLEN / 3][IDLEN+1];	/* �C�O�ܦh�� (BMLEN / 3) �ӪO�D */
static int BMpool_sz = 0;

static SCHEMA *expired_users;
static int expired_users_allocnum;
static int expusr_size;

static usint perm_special;
static const usint spcperm_list[SPECIAL_PERM_NUM] = SPECIAL_PERM_LIST;
static const char *spcperm_name[SPECIAL_PERM_NUM] = SPECIAL_PERM_NAME;
static int spcperm_sum[SPECIAL_PERM_NUM];



static void
unexcepted_abort()
{
  const char *pattern_message = "===== unexcepted abort =====\n";

  fputs(pattern_message, f_usr_expired);
  fputs(pattern_message, f_special_perm);
  fputs(pattern_message, f_changed_bm);

  exit(0);
}


static int	/* 0: SHM�|���]�w�n */
init_bshm()
{
  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

  return bshm->uptime > 0;
}


static int
uid_cmp(a, b)
  char *a, *b;
{
  return str_cmp(a, b);
}


static int
explst_uno_cmp(a, b)	/* ��userno�Ѥp��j�Ƨ� */
  SCHEMA *a, *b;
{
  return a->userno - b->userno;
}


static int
explst_uid_cmp(a, b)	/* ��userid�Ƨ� */
  SCHEMA *a, *b;
{
  return str_ncmp(a->userid, b->userid, sizeof(b->userid));
}


static int
explst_uid_find(a, b)	/* ��userid�Ƨ�(�j�M��) */
  char *a;
  SCHEMA *b;
{
  return str_ncmp(a, b->userid, sizeof(b->userid));
}



static void
collect_allBM()
{
  BRD *head, *tail;
  char *bm, buf[BMLEN + 1];

  head = bshm->bcache;
  tail = head + bshm->number;
  do
  {
    strcpy(buf, head->BM);
    bm = strtok(buf, "/");

    while(bm)
    {
      strcpy(BMpool[BMpool_sz], bm);
      BMpool_sz++;

      bm = strtok(NULL, "/");
    }
  } while(++head < tail);
}



static char *		/* 1: �b�����O�D�W�椺 */
check_allBM(userid)
  const char *userid;
{
  return bsearch(userid, BMpool, BMpool_sz, IDLEN + 1, uid_cmp);
}


static void
enum_special_perm(ulevel)
  usint ulevel;
{
  int i;

  for(i = 0; i < SPECIAL_PERM_NUM; i++)
  {
    if(ulevel & spcperm_list[i])
    {
      fprintf(f_special_perm, "  %s", spcperm_name[i]);
      spcperm_sum[i]++;
    }
    else
      fprintf(f_special_perm, "  %*s", strlen(spcperm_name[i]), "");
  }
}


static void
push_expired_usr(userno, userid)
  int userno;
  char *userid;
{
  if(expusr_size >= expired_users_allocnum)
  {
    expired_users_allocnum += expired_users_allocnum >> 1;

    expired_users = realloc(expired_users, sizeof(SCHEMA) * expired_users_allocnum);
    if(!expired_users)
    {
      reaper_log("expired_users[] realloc fail\n");
      unexcepted_abort();
    }
  }

  expired_users[expusr_size].userno = userno;
  memcpy(expired_users[expusr_size].userid, userid, sizeof(expired_users[0].userid));

  expusr_size++;
}


static void
reaper(fpath, lower_id)
  char *fpath;
  char *lower_id;	/* �ϥΪ�ID�b�w�Ф�����Ƨ��W�A���p�g */
{
  FILE *fp;
  ACCT acct;
  int keep;
  char usr_home[64], new_home[64];
  time_t livetime;


  visit++;

  strcpy(usr_home, fpath);

  strcat(fpath, FN_ACCT);
  fp = fopen(fpath, "rb+");
  if(!fp)
  {
    int n = errno;
    reaper_log("open %s fail: %s\n", fpath, strerror(n));
    return;
  }

  if(fread(&acct, sizeof(ACCT), 1, fp) != 1)
  {
    int n = errno;
    reaper_log("read %s fail: %s\n", fpath, strerror(n));
    fclose(fp);
    return;
  }

  /* �� PERM_BM �����b�����O�D�W�椺�A������ PERM_BM */
  if((acct.userlevel & PERM_BM) && !check_allBM(lower_id))
  {
    acct.userlevel &= ~PERM_BM;
    rewind(fp);
    fwrite(&acct, sizeof(ACCT), 1, fp);
  }

  fclose(fp);

  if(acct.userno <= 0)
    reaper_log("abnormal userno(%d): %d\n", acct.userid);

  /* �έp�S���v�� */
  if(acct.userlevel & perm_special)
  {
    fprintf(f_special_perm, "%10d) %-12s", acct.userno, acct.userid);
    enum_special_perm(acct.userlevel);
    fprintf(f_special_perm, "\n");
  }
  else if(!acct.userlevel)	/* guest */
  {
    fprintf(f_special_perm, "%10d) %-12s  <guest>\n", acct.userno, acct.userid);
  }


  /* �b���M���P�w */
  if(acct.userlevel & (PERM_ALLADMIN | PERM_XEMPT))
  {
    keep = 1;	/* ���ȡB�֦�"�ä[�O�d�b��"�̪����O�d */
  }
  else if(!str_cmp(acct.userid, STR_GUEST))
  {
    keep = 1;	/* guest�b�� */
  }
  else if(acct.userlevel & PERM_PURGE)
  {
    keep = 0;	/* �Q��"�M���b��"�̪����M�� */
  }
  else
  {
    livetime = MONTH_TO_SEC(LIVETIME_BASE);

    livetime += (time_t)(315569260 * ((double)acct.numposts/(acct.numposts + 1500)));

    if(acct.numlogins < 7500)
    {
      int x = 8000 - acct.numlogins;
      livetime += (time_t)(190084072 - 2.97006362 * (x*x));
    }
    else
      livetime += 189341556;

    if(acct.userlevel & PERM_BM)
      livetime += MONTH_TO_SEC(42);

    if(livetime > MONTH_TO_SEC(LIVETIME_LIMIT))
      livetime = MONTH_TO_SEC(LIVETIME_LIMIT);

    if(acct.lastlogin > startime - livetime)
      keep = 1;
    else
      keep = 0;
  }

  if(!keep)
  {
    fprintf(f_usr_expired, "%10d) %-12s  %s  ", acct.userno, acct.userid, Btime(&acct.lastlogin));
    if(acct.userlevel & PERM_PURGE)
      fprintf(f_usr_expired, "PURGE\n");
    else
      fprintf(f_usr_expired, "%d days\n", livetime / 86400);

    sprintf(new_home, "usr/@/%s_%s", lower_id, date_postfix);
    if(rename(usr_home, new_home) < 0)
    {
      int n = errno;
      fprintf(f_usr_expired, "\tmove user data failed: %s\n", strerror(n));
    }
    else
    {
      push_expired_usr(acct.userno, acct.userid);
      prune++;
    }
  }
}


static void
traverse(fpath)	/* visit secondary user hierarchy */
  char *fpath;
{
  DIR *dirp;
  struct dirent *de;
  char *fname, *root;


  if(!(dirp = opendir(fpath)))
  {
    reaper_log("unable to enter hierarchy [%s]\n", fpath);
    return;
  }

  root = fpath + strlen(fpath);

  while(de = readdir(dirp))
  {
    fname = de->d_name;
    if(fname[0] <= ' ' || fname[0] == '.')
      continue;

    sprintf(root, "%s/", fname);
    reaper(fpath, fname);
  }
  closedir(dirp);
}



static void
schema_cleaner()	/* ����.USR���o���Q�M�����b���P�ť���� */
{
  SCHEMA schema;
  int i = 0, last_uno = 1, max_uno = 0;
  off_t reader_head = 0, writer_head = 0;


  fprintf(f_usr_expired, "\n");

  lseek(fd_schema, 0, SEEK_SET);

  while(read(fd_schema, &schema, sizeof(SCHEMA)) == sizeof(SCHEMA))
  {
    reader_head += sizeof(SCHEMA);

    if(schema.userno > max_uno)
      max_uno = schema.userno;

    while(i < expusr_size && expired_users[i].userno < schema.userno)
    {
      i++;
    }

    if(i >= expusr_size || expired_users[i].userno != schema.userno)
    {
      if(schema.userid[0] != '\0')
      {
	lseek(fd_schema, writer_head, SEEK_SET);
	write(fd_schema, &schema, sizeof(SCHEMA));
	writer_head += sizeof(SCHEMA);
	last_uno = schema.userno;

	lseek(fd_schema, reader_head, SEEK_SET);
      }
    }
  }

  /* �Y�s��.USR���̫�@�����O�ثe�o�X��UNO���̤j���A�[�J�@��dummy record */
  if(max_uno > last_uno)
  {
    schema.userno = max_uno;
    memset(schema.userid, 0, sizeof(schema.userid));

    lseek(fd_schema, writer_head, SEEK_SET);
    write(fd_schema, &schema, sizeof(SCHEMA));
    writer_head += sizeof(SCHEMA);
  }

  ftruncate(fd_schema, writer_head);
  f_unlock(fd_schema);
  close(fd_schema);

  fprintf(f_usr_expired, "�̤juserno�G%d\n", max_uno);
}



static int
in_expired(userid)
  const char *userid;
{
  return bsearch(userid, expired_users, expusr_size, sizeof(SCHEMA), explst_uid_find) != NULL;
}


static void
remove_expiredBM()	/* �ޱ��Q�M�����O�D */
{
  BRD *brd;
  int bno, i, modified;
  char buf[BMLEN + 1], *bm;
  char newBM[BMLEN + 1];
  int changed_board = 0, all_remove = 0, no_bm = 0;


  brd = bshm->bcache;

  for(bno = 0; bno < bshm->number; bno++)
  {
    if(brd[bno].brdname[0] == '\0')
      continue;

    strcpy(buf, brd[bno].BM);
    bm = strtok(buf, "/");
    modified = 0;
    newBM[0] = '\0';

    i = 0;
    while(bm)
    {
      if(in_expired(bm))
      {
	modified = 1;
	if(i == 0)	/* ���O�D�Q�M���N��������O�D */
	{
	  all_remove++;
	  break;
	}
      }
      else
      {
	if(newBM[0])
	  strcat(newBM, "/");
	strcat(newBM, bm);
      }

      bm = strtok(NULL, "/");
      i++;
    }

    if(i == 0 && strcmp(brd[bno].class, "����"))	/* dust.121013: ���ȪO���C�J�L�D�ݪO�έp */
      no_bm++;

    if(modified)
    {
      fprintf(f_changed_bm, "%s:\n", brd[bno].brdname);
      fprintf(f_changed_bm, "\t%s\n", brd[bno].BM);
      fprintf(f_changed_bm, "->\t%s\n\n", newBM);

      strcpy(brd[bno].BM, newBM);
      rec_put(FN_BRD, &(brd[bno]), sizeof(BRD), bno, NULL);

      changed_board++;
    }
  }

  fprintf(f_changed_bm, "��ʼƶq�G%d\n", changed_board);
  fprintf(f_changed_bm, "��s�M���G%d\n", all_remove);
  fprintf(f_changed_bm, "�L�D�ݪO�G%d\n", no_bm);
}



int
main()
{
  char fpath[64], prefix;
  time_t phase2_end;
  int i;
  struct tm *tp;
  struct stat st;


  setuid(BBSUID);
  setgid(BBSGID);
  chdir(BBSHOME);

  flog = fopen(REAPER_LOG, "w");
  if(!flog)
    flog = stdout;

  umask(077);


  /* initialization */

  startime = time(NULL);
  reaper_log("reaper launched at %s\n\n", Btime(&startime));

  if(!init_bshm())
  {
    reaper_log("BSHM is not ready\n");
    exit(0);
  }

  if(mkdir("usr/@", 0700) < 0)
  {	/* �ϥΪ̳Qreaper��|�Q��� usr/@ �U�A�b�����ذ_�� */
    if(errno != EEXIST)
    {
      int n = errno;
      reaper_log("cannot create usr/@: %s\n", strerror(n));
      exit(0);
    }
  }
  fd_schema = open(FN_SCHEMA, O_RDWR);
  if(fd_schema < 0)
  {
    int n = errno;
    reaper_log("cannot open schema(%s): %s\n", FN_SCHEMA, strerror(n));
    exit(0);
  }
  f_exlock(fd_schema);	/* reaper �@����N��w.USR */

  f_usr_expired = fopen(FN_RUN_REAPER, "w");
  if(!f_usr_expired)
  {
    int n = errno;
    reaper_log("cannot create user expire log: %s\n", strerror(n));
    exit(0);
  }

  f_special_perm = fopen(FN_RUN_SPPERM, "w");
  if(!f_special_perm)
  {
    int n = errno;
    reaper_log("cannot create special permission log: %s\n", strerror(n));
    exit(0);
  }

  f_changed_bm = fopen(FN_RUN_MODBM, "w");
  if(!f_special_perm)
  {
    int n = errno;
    reaper_log("cannot create BM list modify log: %s\n", strerror(n));
    exit(0);
  }

  if(fstat(fd_schema, &st) == 0)
    expired_users_allocnum = st.st_size / sizeof(SCHEMA) / 2;
  else
    expired_users_allocnum = MAXAPPLY / 100;

  expired_users = malloc(sizeof(SCHEMA) * expired_users_allocnum);
  if(!expired_users)
  {
    reaper_log("expired_users[] malloc failed\n");
    unexcepted_abort();
  }

  perm_special = 0;
  for(i = 0; i < SPECIAL_PERM_NUM; i++)
    perm_special |= spcperm_list[i];

  tp = localtime(&startime);
  strftime(date_postfix, sizeof(date_postfix), "%Y%m%d", tp);


  /* phase1: �qBSHM����X�Ҧ��O�D */

  collect_allBM();

  qsort(BMpool, BMpool_sz, IDLEN + 1, uid_cmp);


  /* phase2: ���X usr/_/ */

  for(prefix = 'a'; prefix <= 'z'; prefix++)
  {
    sprintf(fpath, "usr/%c/", prefix);
    traverse(fpath);
  }

  qsort(expired_users, expusr_size, sizeof(SCHEMA), explst_uno_cmp);

  schema_cleaner();

  phase2_end = time(NULL);

  fprintf(f_usr_expired, "\n�M���H�ơG%d\n", prune);
  fprintf(f_usr_expired, "  �`�H�ơG%d\n", visit);
  fprintf(f_usr_expired, "\n�`�p�ӮɡG%d sec\n", phase2_end - startime);

  fprintf(f_special_perm, "\n");
  for(i = 0; i < SPECIAL_PERM_NUM; i++)
  {
    fprintf(f_special_perm, "%s�G%d\n", spcperm_name[i], spcperm_sum[i]);
  }


  /* phase3: �����Q�M�����O�D */

  qsort(expired_users, expusr_size, sizeof(SCHEMA), explst_uid_cmp);
  remove_expiredBM();

  fprintf(f_changed_bm, "\n�`�p�ӮɡG%d sec\n", time(NULL) - phase2_end);


  /* final. */

  fclose(f_usr_expired);
  fclose(f_special_perm);
  fclose(f_changed_bm);

  reaper_log("\ntime elapsed: %d sec.\n", time(NULL) - startime);
  fclose(flog);

  return 0;
}
