/*-------------------------------------------------------*/
/* rhsys.c						 */
/*-------------------------------------------------------*/
/* target : article Reading History System		 */
/* create : 10/10/18					 */
/* update : 10/12/15					 */
/*-------------------------------------------------------*/



#include "bbs.h"



#define BRH_MAX 100
#define CRH_MAX 100
#define RHS_WINDOW 10		/* base一次增大這麼多筆的空間 */
#define EXPIRE_READ_DAYS 180	/* 多久以前視為已讀 */

#define	BRH_MASK 0x7fffffff	/* 請定義為time_t的最大值 */
#define	BRH_CMPR 0x80000000	/* 請定義為time_t的最高位為一，其餘為零的值 */

#define RHSYS_DEBUG	/* 除錯用 */


typedef struct
{
  time_t t1;
  time_t t2;
}time_pair;	/* 純粹美化某些sizeof()用 */


typedef struct
{
  short num;
  short allocated;
  time_t list[0][2];
}RHLIST;


typedef struct
{
  time_t bstamp;
  RHLIST *brh_list;
  RHLIST *crh_list;
}RHBASE;



extern char brd_bits[];

static RHBASE *rhs_base;
static int rhs_size;		/* base上有多少看板 */
static int rhs_allocate;	/* base已經配置的空間 */
static time_t rhs_expire;

static time_t cached_bstamp;

static time_t brh_cache[BRH_MAX][2];
static int brca_num = 0;

static time_t crh_cache[CRH_MAX][2];
static int crca_num = 0;




#define RHS_ASSERT(exp) \
{ \
  if(!(exp)) \
    abort(); \
}


static int
rhs_findbrd(time_t key, int *pos)
{
  int lower, upper, mid;

  lower = 0;
  upper = rhs_size - 1;

  while(lower <= upper)
  {
    mid = (lower + upper) / 2;
    if(key < rhs_base[mid].bstamp)
      upper = mid - 1;
    else if(key > rhs_base[mid].bstamp)
      lower = mid + 1;
    else
    {
      *pos = mid;
      return 1;
    }
  }

  *pos = lower;
  return 0;
}



static int
brh_search(time_t key, time_t list[][2], int upper)
{
  int lower = 0, mid;
  upper--;

  while(lower <= upper)
  {
    mid = (lower + upper) / 2;
    if(key >= list[mid][0] && key <= list[mid][1])
      return 1;
    else if(key < list[mid][0])
      upper = mid - 1;
    else
      lower = mid + 1;
  }

  return 0;
}



static int
crh_search(time_t key, time_t list[][2], int upper, int *pos)
{
  int lower = 0, mid;
  upper--;

  while(lower <= upper)
  {
    mid = (lower + upper) / 2;
    if(key < list[mid][0])
      upper = mid - 1;
    else if(key > list[mid][0])
      lower = mid + 1;
    else
    {
      *pos = mid;
      return 1;
    }
  }

  *pos = lower;
  return 0;
}



static RHLIST*
rhslist_copy(RHLIST *rhlist, int size, time_t src[][2])
{
  rhlist->num = size;
  if(size <= 0)
    return rhlist;

  if(rhlist->allocated < size)	/* 調整 list 的大小 */
  {
    rhlist = (RHLIST*) realloc(rhlist, sizeof(RHLIST) + sizeof(time_pair) * size);
    RHS_ASSERT(rhlist);
    rhlist->allocated = size;
  }

  memcpy(rhlist->list, src, sizeof(time_pair) * size);
  return rhlist;
}




/*---------------------------------------*/
/* unread: 查詢文章是否為未讀		 */
/*---------------------------------------*/

int	/* 0:已讀  1:BRH未讀  2:CRH未讀 */
rhs_unread(time_t bstamp, time_t chrono, time_t stamp)
{
  time_t (*blist)[2], (*clist)[2];
  int bsz, csz, pos, list_full;

  if(chrono < rhs_expire)
    return 0;

  if(bstamp < 0 || bstamp == cached_bstamp)
  {
    blist = brh_cache;
    bsz = brca_num;
    clist = crh_cache;
    csz = crca_num;
  }
  else
  {
    if(!rhs_findbrd(bstamp, &pos))	/* 不在base中直接視為BRH unread */
      return 1;

    blist = rhs_base[pos].brh_list->list;
    bsz = rhs_base[pos].brh_list->num;
    clist = rhs_base[pos].crh_list->list;
    csz = rhs_base[pos].crh_list->num;
  }


  if(!brh_search(chrono, blist, bsz))
    return 1;

  if(stamp == 0)		/* stamp 為零(也就是未有推文過)當成已讀 */
    return 0;

  list_full = csz >= CRH_MAX;
  if(clist[0][0] < 0)
  {
    if(stamp <= clist[0][1])	/* visit的特殊判斷 */
      return 0;

    clist++;
    csz--;
  }

  /* list全滿時，若chrono比最小項還小就當成已讀 */
  if(list_full && chrono < clist[0][0])
    return 0;

  if(crh_search(chrono, clist, csz, &pos))
    return clist[pos][1] == stamp? 0 : 2;
  else
    return 2;
}




/*---------------------------------------*/
/* add: 加入閱讀紀錄			 */
/*---------------------------------------*/

void
brh_add(time_t prev, time_t chrono, time_t next)
{
  int i;
  time_t final, begin;

  /* try to merge */
  for(i = brca_num - 1; i >= 0; i--)
  {
    final = brh_cache[i][1];
    if(chrono > final)
    {
      if(prev <= final)
      {
	if(next >= begin && i < brca_num - 1)
	{
	  brh_cache[i+1][0] = brh_cache[i][0];
	  brca_num--;
	  memcpy(brh_cache[i], brh_cache[i+1], sizeof(time_pair) * (brca_num - i));
	}
	else
	  brh_cache[i][1] = chrono;

	return;
      }

      if(next >= begin && i < brca_num - 1)
      {
	brh_cache[i+1][0] = chrono;
	return;
      }

      break;
    }

    begin = brh_cache[i][0];
  }

  if(i < 0 && next >= begin && brca_num > 0)
  {
    brh_cache[0][0] = chrono;
    return;
  }


  /* add the new record in */
  /* ex: (p=21, c=22, n=23) add to list=([4, 9], [10, 15], [30, 32]) */

  if(brca_num >= BRH_MAX)
  {
    /* if brh list is full, merge with existed inteval */
    /* => [4, 9] [10, 22] [30, 32] */
    /* if args are (p=1, c=2, n=3), list will be ([2, 9], [10, 22], [30, 32]) */

    if(i < 0)
      brh_cache[0][0] = chrono;
    else
      brh_cache[i][1] = chrono;
  }
  else
  {
    /* if brh list is not full, insert [chrono, chrono] in */
    /* => [4, 9] [10, 15] [22, 22] [30, 32] */

    i++;
    if(brca_num > i)
      memmove(brh_cache[i+1], brh_cache[i], sizeof(time_pair) * (brca_num - i));
    brh_cache[i][0] = brh_cache[i][1] = chrono;
    brca_num++;
  }
}



void
crh_add(time_t chrono, time_t stamp)
{
  int pos, num;
  time_t (*list)[2];

  if(stamp == 0 || chrono < rhs_expire)
    return;

  list = crh_cache;
  num = crca_num;
  if(num > 0 && crh_cache[0][0] < 0)
  {
    list++;
    num--;
  }

  if(crh_search(chrono, list, num, &pos))
  {
    list[pos][1] = stamp;
  }
  else
  {
    if(crca_num < CRH_MAX)	/* 列表還沒滿向後挪出空間 */
    {
      if(num - pos > 0)
	memmove(list[pos+1], list[pos], sizeof(time_pair) * (num - pos));

      crca_num++;
    }
    else		/* 列表已滿時改成往前挪出空格 */
    {
      if(--pos < 0)	/* 若加進來的比每一項的chrono都小的話就不加入 */
	return;

      memcpy(list[0], list[1], sizeof(time_pair) * pos);
    }

    list[pos][0] = chrono;
    list[pos][1] = stamp;
  }
}




/*---------------------------------------*/
/* visit: 設定看板閱讀狀態		 */
/*---------------------------------------*/

void
rhs_visit(time_t mode)
{
  switch(mode)
  {
  case 0:	/* visit */
    time(&mode);
    brca_num = crca_num = 1;
    brh_cache[0][0] = 0;
    brh_cache[0][1] = mode;
    crh_cache[0][0] = -1;
    crh_cache[0][1] = mode;
    break;

  case 1:	/* unvisit */
    brca_num = crca_num = 0;
    break;

  default:	/* partial visit */
    brca_num = 1;
    brh_cache[0][0] = 0;
    brh_cache[0][1] = mode;
  }
}




/*---------------------------------------*/
/* get/put: cache<->base移動處理	 */
/*---------------------------------------*/

static void
rhs_putback(void)	/* 把 cache 中的閱讀紀錄放回 base 中 */
{
  int pos;

  if(cached_bstamp == -1)
    return;

  /* base中沒有對應的點。加進去 */
  if(!rhs_findbrd(cached_bstamp, &pos))
  {
    int bno;

    if(brca_num <= 0)	/* 若沒有BRH紀錄就別新增了 */
      return;

    if(rhs_allocate <= rhs_size)
    {
      rhs_allocate += RHS_WINDOW;
      RHS_ASSERT(rhs_base = (RHBASE*) realloc(rhs_base, rhs_allocate * sizeof(RHBASE)));
    }

    if(rhs_size - pos > 0)
      memmove(rhs_base + pos + 1, rhs_base + pos, sizeof(RHBASE) * (rhs_size - pos));

    rhs_base[pos].bstamp = cached_bstamp;

    rhs_base[pos].brh_list = (RHLIST*) malloc(sizeof(RHLIST) + sizeof(time_pair) * brca_num);
    RHS_ASSERT(rhs_base[pos].brh_list);
    rhs_base[pos].brh_list->allocated = brca_num;

    rhs_base[pos].crh_list = (RHLIST*) malloc(sizeof(RHLIST) + sizeof(time_pair) * crca_num);
    RHS_ASSERT(rhs_base[pos].crh_list);
    rhs_base[pos].crh_list->allocated = crca_num;

    rhs_size++;
    bno = bstamp2bno(cached_bstamp);
    if(bno >= 0)
      brd_bits[bno] |= BRD_H_BIT;
  }

  rhs_base[pos].brh_list = rhslist_copy(rhs_base[pos].brh_list, brca_num, brh_cache);
  rhs_base[pos].crh_list = rhslist_copy(rhs_base[pos].crh_list, crca_num, crh_cache);
}



void
rhs_get(time_t bstamp, int bno)	/* 把base中的閱讀紀錄放入快取 */
{
  int pos;

  if(cached_bstamp == bstamp)	/* 該看板已經在cache中 */
    return;

  rhs_putback();

  cached_bstamp = bstamp;

  if((brd_bits[bno] & BRD_H_BIT) && rhs_findbrd(bstamp, &pos))
  {
    brca_num = rhs_base[pos].brh_list->num;
    memcpy(brh_cache, rhs_base[pos].brh_list->list, sizeof(time_pair) * brca_num);

    crca_num = rhs_base[pos].crh_list->num;
    memcpy(crh_cache, rhs_base[pos].crh_list->list, sizeof(time_pair) * crca_num);
  }
  else
  {
    brca_num = 0;
    crca_num = 0;
  }
}




/*---------------------------------------*/
/* load/save: 讀取與儲存		 */
/*---------------------------------------*/

void
rhs_load(void)
{
  rhs_expire = time(0) - EXPIRE_READ_DAYS * 86400;
  rhs_size = rhs_allocate = 0;

  if(cuser.userlevel)	/* guest不需載入.BRH與.CRH */
  {
    char fpath[64];
    struct stat st;
    FILE *fp;

    /* load .BRH */
    usr_fpath(fpath, cuser.userid, FN_BRH);
    if(stat(fpath, &st) >= 0 && st.st_size > 0 && (fp = fopen(fpath, "rb")))
    {
      uschar *tmp_space, *head;
      time_t bstamp;
      int i, num;

      //first pass: read into memory, and filter dummy/expire record
      RHS_ASSERT(tmp_space = (uschar*) malloc(st.st_size));

      head = tmp_space;
      while(fread(head, sizeof(time_t), 1, fp) > 0)
      {
	short *brh_count;
	int bno;

	bstamp = *((time_t *)head);
	head += sizeof(time_t);

	fread(head, sizeof(short), 1, fp);
	brh_count = (short*) head;
	head += sizeof(short);
	num = *brh_count;

	bno = bstamp2bno(bstamp);
	if(bno >= 0)
	{
	  time_t t1, t2;
	  time_t *list = (time_t*)head;

	  while(num > 0)
	  {
	    fread(&t1, sizeof(time_t), 1, fp);
	    num--;

	    if((t1 & BRH_MASK) >= rhs_expire)
	    {
	      *list++ = t1;
	      goto there_are_BRHs;
	    }

	    if(!(t1 & BRH_CMPR))
	    {
	      fread(&t2, sizeof(time_t), 1, fp);
	      num--;

	      if(t2 >= rhs_expire)
	      {
		*list++ = t1;
		*list++ = t2;
		goto there_are_BRHs;
	      }

	      --*brh_count;
	    }

	    --*brh_count;
	  }

	  if(1)
	  {
	    head -= sizeof(short);
	    head -= sizeof(time_t);
	  }
	  else
	  {
there_are_BRHs:
	    rhs_size++;
	    if(num > 0)
	    {
	      fread(list, sizeof(time_t), num, fp);
	      list += num;
	    }
	    head = (uschar *)list;
	    brd_bits[bno] |= BRD_H_BIT;
	  }
	}
	else	/* dummy record, not read in */
	{
	  fseeko(fp, num * sizeof(time_t), SEEK_CUR);
	}
      }

      head = tmp_space;
      rhs_allocate = rhs_size + RHS_WINDOW;
      RHS_ASSERT(rhs_base = (RHBASE*) malloc(sizeof(RHBASE) * rhs_allocate));

      //second pass: move records from temp space to base, and expand compressed time pair
      for(i = 0; i < rhs_size; i++)
      {
	time_t *list, *tail;
	int j;

	rhs_base[i].bstamp = *((time_t*)head);
	head += sizeof(time_t);
	num = *((short *)head);
	head += sizeof(short);

	list = (time_t*)head;
	tail = list + num;
	for(j = 0; list < tail; j++)
	{
	  if(j >= BRH_MAX)
	    break;

	  brh_cache[j][0] = *list;
	  if(*list & BRH_CMPR)
	  {
	    brh_cache[j][0] &= BRH_MASK;
	    brh_cache[j][1] = brh_cache[j][0];
	  }
	  else
	    brh_cache[j][1] = *++list;
	  list++;
	}

	rhs_base[i].brh_list = (RHLIST*) malloc(sizeof(RHLIST) + sizeof(time_pair) * j);
	RHS_ASSERT(rhs_base[i].brh_list);

	rhs_base[i].brh_list->num = j;
	rhs_base[i].brh_list->allocated = j;
	memcpy(rhs_base[i].brh_list->list, brh_cache, sizeof(time_pair) * j);

	head = (uschar*)tail;
      }

      free(tmp_space);
      fclose(fp);
    }

    /* load .CRH */
    if(rhs_size > 0)
    {
      int i;

      usr_fpath(fpath, cuser.userid, FN_CRH);
      if(stat(fpath, &st) >= 0 && st.st_size > 0 && (fp = fopen(fpath, "rb")))
      {
	time_t bstamp;
	short num;

	fread(&bstamp, sizeof(time_t), 1, fp);
	fread(&num, sizeof(short), 1, fp);
	if(feof(fp))
	  bstamp = BRH_MASK;

	for(i = 0; i < rhs_size; i++)
	{
	  if(bstamp < rhs_base[i].bstamp)
	  {
	    do
	    {
	      fseeko(fp, sizeof(time_pair) * num, SEEK_CUR);
	      if(fread(&bstamp, sizeof(time_t), 1, fp) <= 0 || fread(&num, sizeof(short), 1, fp) <= 0)
		bstamp = BRH_MASK;
	    }while(bstamp < rhs_base[i].bstamp);
	  }

	  if(bstamp > rhs_base[i].bstamp)
	  {
	    RHS_ASSERT(rhs_base[i].crh_list = (RHLIST*) malloc(sizeof(RHLIST)));
	    rhs_base[i].crh_list->allocated = rhs_base[i].crh_list->num = 0;
	  }
	  else
	  {
	    time_t pair[2];
	    int j, k;

	    k = 0;
	    for(j = 0; j < num; j++)
	    {
	      fread(pair, sizeof(time_t), 2, fp);

	      if(pair[0] < 0)
	      {
		/* 刪掉過舊的CRH visit紀錄 */
		if(pair[1] < rhs_expire) continue;
	      }
	      else if(pair[0] < rhs_expire)
		continue;

	      if(k < CRH_MAX)
	      {
		crh_cache[k][0] = pair[0];
		crh_cache[k][1] = pair[1];
		k++;
	      }
	    }

	    rhs_base[i].crh_list = (RHLIST*) malloc(sizeof(RHLIST) + sizeof(time_pair) * k);
	    RHS_ASSERT(rhs_base[i].crh_list);

	    rhs_base[i].crh_list->allocated = rhs_base[i].crh_list->num = k;
	    memcpy(rhs_base[i].crh_list->list, crh_cache, sizeof(time_pair) * k);

	    if(fread(&bstamp, sizeof(time_t), 1, fp) <= 0 || fread(&num, sizeof(short), 1, fp) <= 0)
	      bstamp = BRH_MASK;
	  }
	}
	fclose(fp);
      }
      else
      {
	for(i = 0; i < rhs_size; i++)
	{
	  RHS_ASSERT(rhs_base[i].crh_list = (RHLIST*) malloc(sizeof(RHLIST)));
	  rhs_base[i].crh_list->allocated = rhs_base[i].crh_list->num = 0;
	}
      }
    }
  }

  brca_num = crca_num = 0;
  cached_bstamp = -1;

  /* 若沒有配置rhs_base的話就在這邊補配置 */
  if(rhs_allocate == 0)
  {
    rhs_allocate = RHS_WINDOW;
    RHS_ASSERT(rhs_base = (RHBASE*) malloc(RHS_WINDOW * sizeof(RHBASE)));
  }
}



void
rhs_save(void)
{
  char fpath[64];
  FILE *fp_brh, *fp_crh;

  if(!rhs_base)
    return;


  rhs_putback();

  usr_fpath(fpath, cuser.userid, FN_BRH);
  if(!(fp_brh = fopen(fpath, "wb")))
    return;

  usr_fpath(fpath, cuser.userid, FN_CRH);
  if(fp_crh = fopen(fpath, "wb"))
  {
    int i, j, num;

    for(i = 0; i < rhs_size; i++)
    {
      num = rhs_base[i].brh_list->num;
      if(num > 0)
      {
	time_t (*list)[2] = rhs_base[i].brh_list->list;
	int count = num;

	for(j = 0; j < num; j++)
	{
	  if(list[j][0] != list[j][1])
	    count++;
	}

	fwrite(&rhs_base[i].bstamp, sizeof(time_t), 1, fp_brh);
	fwrite(&count, sizeof(short), 1, fp_brh);
	for(j = 0; j < num; j++)
	{
	  if(list[j][0] == list[j][1])
	  {
	    time_t tmp = list[j][0] | BRH_CMPR;
	    fwrite(&tmp , sizeof(time_t), 1, fp_brh);
	  }
	  else
	    fwrite(list[j], sizeof(time_t), 2, fp_brh);
	}
      }

      num = rhs_base[i].crh_list->num;
      if(num > 0)
      {
	fwrite(&rhs_base[i].bstamp, sizeof(time_t), 1, fp_crh);
	fwrite(&rhs_base[i].crh_list->num, sizeof(short), 1, fp_crh);
	fwrite(rhs_base[i].crh_list->list, sizeof(time_t), num * 2, fp_crh);
      }

      free(rhs_base[i].brh_list);
      free(rhs_base[i].crh_list);
    }

    fclose(fp_brh);
    fclose(fp_crh);
  }
  else
    fclose(fp_brh);

  free(rhs_base);
  rhs_base = NULL;
}


#ifdef RHSYS_DEBUG

#define DUMP_PATH "tmp/rhs.dump"

#define print_BRH(t) fprintf(fp, "  [%10d, %10d]%s\n", t[0], t[1], t[0]==t[1]? " (duplicated)" : "")
#define print_CRH(t) fprintf(fp, "  {%10d, %10d}\n", t[0], t[1])

static void
dump_rhlist(FILE *fp, int bnum, time_t blist[][2], int cnum, time_t clist[][2])
{
  int i;

  fputs("BRH:\n", fp);
  fprintf(fp, "  num = %d\n", bnum);
  for(i = 0; i < bnum; i++)
  {
    fprintf(fp, "  [%10d, %10d]", blist[i][0], blist[i][1]);
    if(blist[i][0] == blist[i][1])
      fputs(" \033[1;30m(duplicated)\033[m", fp);
    fputs("\n", fp);
  }
  fputs("\n", fp);

  fputs("CRH:\n", fp);
  fprintf(fp, "  num = %d\n", cnum);
  for(i = 0; i < cnum; i++)
  {
    fprintf(fp, "  {%10d, %10d}\n", clist[i][0], clist[i][1]);
  }
  fputs("\n", fp);
}


void 
rhs_dump (void)
{
  screen_backup_t old_scr;
  FILE *fp;
  int i, memory;

  fp = fopen(DUMP_PATH, "w");
  if(!fp)
  {
    vmsg("cannot dump");
    return;
  }

  fprintf(fp, "size = %d, allocate = %d\n", rhs_size, rhs_allocate);
  fputs("\n----------\n", fp);

  fprintf(fp, "[cache] bstamp = %d\n\n", cached_bstamp);
  dump_rhlist(fp, brca_num, brh_cache, crca_num, crh_cache);

  memory = rhs_allocate * (sizeof(RHBASE) + sizeof(RHLIST) * 2);
  for(i = 0; i < rhs_size; i++)
  {
    fputs("\n==========\n", fp);
    fprintf(fp, "base[%d] bstamp = %d\n\n", i, rhs_base[i].bstamp);
    dump_rhlist(fp, rhs_base[i].brh_list->num, rhs_base[i].brh_list->list, rhs_base[i].crh_list->num, rhs_base[i].crh_list->list);

    memory += (rhs_base[i].brh_list->num + rhs_base[i].crh_list->num) * sizeof(time_pair);
  }

  fputs("\n----------\n", fp);
  fprintf(fp, "memory use: %d bytes\n", memory);

  fclose(fp);

  scr_dump(&old_scr);

  more(DUMP_PATH, MFST_NORMAL);
  unlink(DUMP_PATH);

  scr_restore(&old_scr);
}

#endif
