/*-------------------------------------------------------*/
/* regmgr.c                                              */
/*-------------------------------------------------------*/
/* target : Advanced Register Manager                    */
/* create : 12/03/09                                     */
/* update : 12/06/04 v1.0b                               */
/*-------------------------------------------------------*/

// WARNING : this system needs "pfterm"

#include "bbs.h"

/* ----------------------------------------------------- */
/* Definations                                           */
/* ----------------------------------------------------- */
#ifndef NECKER_REGMGR
#define NECKER_REGMGR   \
COLOR3 "  �s��     �� �� �� ID    ��   ��   ��   ��%*s                                   \033[m"
#endif
#ifndef FOOTER_REGMGR
#define FOOTER_REGMGR   \
COLOR1 " �f���U�� " COLOR2 " (��)��^  (��)�d��  (d)�R��  (b)�h�^  (v)�q�L  (q)�d��ID  (s)��s  "
#endif

#define REG_CACHE_SIZE 50 /* I think it won't full :P */
#define REG_CACHE_EXPAND /* expandable */

static char *week[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

/* PLEASE BECAREFUL THE SIZE OF fpath */
void
make_reg_path(fpath, user)
  char *fpath;
  char *user;
{
  int n = 0;
  fpath[n++] = 'r';
  fpath[n++] = 'e';
  fpath[n++] = 'g';
  fpath[n++] = '/';

  strcpy(fpath + n, user);
  str_lower(fpath, fpath);
}

static void
log_register(rform, pass, reason, checkbox, szbox)
  RFORM *rform;
  int pass;
  char *reason[];
  int checkbox[];
  int szbox;
{
  char fpath[64], title[TTLEN + 1];
  FILE *fp;

  sprintf(fpath, "tmp/log_register.%s", cuser.userid);
  if(fp = fopen(fpath, "w"))
  {
    sprintf(title, "[����] ���� %s %s %s �����U��", cuser.userid, pass? "�q�L" : "�h�^", rform->userid);

    /* �峹���Y */
    fprintf(fp, "%s <���F����> (Eye of Daemon)\n", str_author1);
    fprintf(fp, "���D: %s\n�ɶ�: %s\n\n", title, Now());

    /* �峹���e */
    fprintf(fp, "�ӽЮɶ�: %s\n", Btime(&rform->rtime));
    fprintf(fp, "�A�ȳ��: %s\n", rform->career);
    fprintf(fp, "�ثe��}: %s\n", rform->address);
    fprintf(fp, "�s���q��: %s\n", rform->phone);

    if(!pass)   /* �O���h�^��] */
    {
      int i;

      fprintf(fp, "\n�h�^��]�G\n");
      for(i = 1; i < szbox; i++)
      {
        if(checkbox[i])
          fprintf(fp, "    %s\n", reason[i]);
      }
      if(checkbox[0])
        fprintf(fp, "    %s\n", reason[0]);
    }

    fclose(fp);

    add_post(BN_REGLOG, fpath, title, "<���F����>", "Eye of Daemon", POST_DONE);

    unlink(fpath);
  }
}


/* ----------------------------------------------------- */
/* Variables                                             */
/* ----------------------------------------------------- */
#ifdef REG_CACHE_EXPAND
char **register_list = NULL;
int register_list_page = 0;
#else
char register_list[REG_CACHE_SIZE][IDLEN + 1];
#endif
int register_list_count = 0;

/* ----------------------------------------------------- */
/* Pre-define functions                                  */
/* ----------------------------------------------------- */
void register_list_init();
int register_list_inited();
void register_list_push(char*);
void register_list_del(int);
void register_list_expand(int);

/* ----------------------------------------------------- */
/* Sub Functions for Drawing Screen                      */
/* ----------------------------------------------------- */

void
draw_head()
{
  int orgX, orgY;
  getyx(&orgY, &orgX);
  vs_head("�t�κ��@", str_site);
  move(1, 0);
  prints(NECKER_REGMGR, d_cols, "");

  move(orgY, orgX);
}

void
draw_footer()
{
  int orgX, orgY;
  getyx(&orgY, &orgX);
  move(b_lines, 0);
  outf(FOOTER_REGMGR);

  move(orgY, orgX);
}

int
draw_page(page)
  int page;
{
  int orgX, orgY;
  int item_per_page;
  int page_first;
  int empty = 0;
  int locked = 0;
  int fd;
  char fpath[IDLEN + 10];
  RFORM rf;
  struct tm *tt;
  int i;
  getyx(&orgY, &orgX);

  item_per_page = b_lines - 2;
  page_first = item_per_page * page;
  /* fix range */
  if (register_list_count == 0) // empty
  {
    page = 0;
    empty = 1;
  }
  else if (page_first >= register_list_count)
  {
    page = (register_list_count - 1) / item_per_page;
    page_first = item_per_page * page;
  }

  /* start drawing */
  if (empty)
  {
    move(2, 0);
    prints(">%5d ", 0);
    prints("%19.19s", "");
    outs("�ثe�L���U�����");
    clrtoeol(); // clean up
  }
  else
  {
    for (i = 0; i < register_list_count - page_first && i < item_per_page; ++i)
    {
      make_reg_path(fpath, register_list[i + page_first]);

      fd = open(fpath, O_RDONLY);
      if (fd >= 0)
      {
        if (read(fd, &rf, sizeof(RFORM)) != sizeof(RFORM)) // if error
        {
          str_ncpy(rf.userid, "CRASHED!!", IDLEN);
          rf.rtime = time(NULL);
        }
        close(fd);
      }
      strcat(fpath, ".lck");
      locked = (dashf(fpath) ? 1 : 0);

      move(i + 2, 0);
      prints(" %5d ", i + page_first + 1); /* �L�X�s�� */

      attrset(011); // *[1;31m
      outc(locked ? '#' : ' '); /* print lock */
      attrset(007);
      outs("   "); // 3 space

      prints("%-15.15s", rf.userid);

      tt = localtime(&rf.rtime);
      prints("%04d/%02d/%02d %s %02d:%02d:%02d", \
        tt->tm_year + 1900, tt->tm_mon + 1, tt->tm_mday, \
        week[tt->tm_wday], \
        tt->tm_hour, tt->tm_min, tt->tm_sec);
    }
    for ( ;i < item_per_page; ++i)
    {
      move(i + 2, 0);
      clrtoeol();
    }
  }

  move(orgY, orgX);
  return page;
}

void
draw_cursor(cursor)
  int cursor;
{
  int i;
  int item_per_page = b_lines - 3;
  for (i = 0; i < item_per_page; ++i)
  {
    move(i + 2, 0);
    outc(' ');
  }
  move(cursor + 2, 0);
  outc('>');
}

void
draw_user_infor(id)
  int id;
{
  ACCT acct;
  screen_backup_t old_screen;

  if(acct_load(&acct, register_list[id]) < 0)
  {
    vmsg("���~�G�L�kŪ��.ACCT");
    return;
  }

  scr_dump(&old_screen);
  move(1, 0);
  clrtobot();

  do_query(&acct, NULL);

  utmp_mode(M_SYSTEM);
  scr_restore(&old_screen);
}


/* ----------------------------------------------------- */
/* Sub Functions for Register Files                      */
/* ----------------------------------------------------- */

void
read_register_list()
{
  DIR *dir;
  struct dirent *fptr;
  int len;

  dir = opendir(FN_REGISTER);
  
  if (!register_list_inited())
    register_list_init();

  while ((fptr = readdir(dir)) != NULL)
  {
    if (! ('a' <= fptr->d_name[0] && fptr->d_name[0] <= 'z') )
      continue;
    len = strlen(fptr->d_name);
    if (len > 4)
      if ((fptr->d_name[len - 4] == '.') && \
        (fptr->d_name[len - 3] == 'l') && \
        (fptr->d_name[len - 2] == 'c') && \
        (fptr->d_name[len - 1] == 'k'))
        continue;

    register_list_push(fptr->d_name);
  }

  closedir(dir);
}

void
del_register_file(id)
  int id;
{
  char fpath[IDLEN + 10];
  make_reg_path(fpath, register_list[id]);
  unlink(fpath);
  register_list_del(id);
}

void
adopt_register_file(id, rform, acct)
  int id;
  RFORM *rform;
  ACCT *acct;
{
  HDR hdr;
  char buf[256], folder[64];

  /* �O�����U��� */
  sprintf(buf, "REG: %s:%s:%s:by %s", rform->phone, rform->career, rform->address, cuser.userid);
  justify_log(acct->userid, buf);

  /* �����v�� */
  time(&acct->tvalid);
  /* itoc.041025: �o�� acct_setperm() �èS�����b acct_load() �᭱�A�����j�F�@�� vans()�A
    �o�i��y������ acct �h�л\�s .ACCT �����D�C���L�]���O�����~�����v���A�ҥH�N����F */
  acct_setperm(acct, PERM_VALID, 0);
  /* �H�H�q���ϥΪ� */
  usr_fpath(folder, rform->userid, fn_dir);
  hdr_stamp(folder, HDR_LINK, &hdr, FN_ETC_JUSTIFIED);
  strcpy(hdr.title, msg_reg_valid);
  strcpy(hdr.owner, str_sysop);
  rec_add(folder, &hdr, sizeof(HDR));
  m_biff(rform->userno);

  /* �O���f�� */
  log_register(rform, 1, NULL, NULL, 0);
}
int
refuse_register_file(id, rform, acct)
  int id;
  RFORM *rform;
  ACCT *acct;
{
  char prompt[64];
  char folder[64], fpath[64];
  char *reason[] = {
    prompt, "�п�J�u��m�W", "�иԶ��}���", "�иԶ�s���q��", "�иԶ�A�ȳ��(�Ǯըt��)", "�ХΤ����g�ӽг�",
  };
  int checkbox[sizeof(reason) / sizeof(reason[0])];
  int op, i, n;
  FILE *fp_note;
  HDR hdr;

  move(7, 0);
  clrtobot();
  outs("\n�д��X�h�^���U���]�G\n\n");
  n = sizeof(reason) / sizeof(char*);
  for(i = 1; i < n; i++)
    prints("    %d. %s\n", i, reason[i]);
  prints("    i. [�ۭq]��\n");
  memset(checkbox, 0, sizeof(checkbox));

  while (1)
  {
    op = vans("�̷ӽs�������ﶵ(��Js�����A�����ɥ��Ŀ�������X)�G");

    i = op - '0';
    if (i >= 1 && i <= n)
    {
      checkbox[i] = !checkbox[i];
      move(9 + i, 0);
      outs(checkbox[i]? "��" : "  "); 
    }
    else if (op == 'i')
    {
      if (checkbox[0])
      {
        move(9 + n, 0);
        outs("  ");
        move(10 + n, 0);
        clrtoeol();
      }
      else
      {
        if (!vget(b_lines, 0, "�ۭq�G", prompt, sizeof(prompt), DOECHO))
          continue;

        move(9 + n, 0);
        outs("��");

        move(10 + n, 0);
        prints("      \"%s\"", prompt);
      }

      checkbox[0] = !checkbox[0];
    }
    else if (op == 's')
    {
      int checksum = 0;
      for (i = 0; i < n; i++)
        checksum += checkbox[i];

      if (checksum > 0)
        break;
      else
      {
        vmsg("����ܭ�]�A�w�����ާ@�C");
        return 0; // jump out
      }
    }
  }

  /* �H�h�^�H */
  usr_fpath(folder, acct->userid, fn_dir);
  if(fp_note = fdopen(hdr_stamp(folder, 0, &hdr, fpath), "w"))
  {
    fprintf(fp_note, "�@��: %s (%s)\n", cuser.userid, cuser.username);
    fprintf(fp_note, "���D: [�q��] �Э��s��g���U��\n�ɶ�: %s\n\n", Now());

    fputs("�h���]�G\n", fp_note);
    for(i = 1; i < n; i++)
    {
      if(checkbox[i])
        fprintf(fp_note, "    %s\n", reason[i]);
    }
    if(checkbox[0])
      fprintf(fp_note, "    %s\n", prompt);

    fputs("\n\n�]���H�W��]�A�ϵ��U���Ƥ������A\n", fp_note);
    fputs("�̷�\033[1m�x�W�ǳN����BBS���޲z�ϥΤ���\033[m�A�L�k�@�������{�ҡC\n", fp_note);
    fputs("\n�Э��s��g���U��C\n", fp_note);
    fputs("\n                                                    " BBSNAME "����\n", fp_note);
    fclose(fp_note);

    strcpy(hdr.owner, cuser.userid);
    strcpy(hdr.title, "[�q��] �Э��s��g���U��");
    rec_add(folder, &hdr, sizeof(HDR));

    m_biff(rform->userno);
  }

  /* �O���f�� */
  log_register(rform, 0, reason, checkbox, sizeof(reason) / sizeof(char*));

  return 1;
}

int
process_register_file(id, opt)
  int id;
  char opt;
{
  char fpath[IDLEN + 10];
  char lckpath[IDLEN + 10];
  screen_backup_t old_screen;
  int fd, rtn = 0;
  ACCT acct;
  RFORM rform;
  FILE *fp;
#ifdef USE_DUST_KRANS
  char *options[] = {"y�q�L", "n�h�^", "d�R��", "q���}", NULL};
  char *del_options[] = {"n����", "y�T�w", NULL};
#endif

  if (register_list_count == 0)
    return 0;

  make_reg_path(fpath, register_list[id]);
  sprintf(lckpath, "%s.lck", fpath);

  if (dashf(lckpath))
    return 0;

  fd = open(fpath, O_RDONLY);
  if (fd < 0) // failed
    return 0;
  if (read(fd, &rform, sizeof(RFORM)) != sizeof(RFORM))
    return 0;

  fp = fopen(lckpath, "w+");
  if (fp)
  {
    fprintf(fp, "%s", cuser.userid);
    fclose(fp);
  }
  else
    return 0;

  scr_dump(&old_screen);

  move(1, 0);
  prints("�ӽХN��: %s (�ӽЮɶ��G%s)\n", rform.userid, Btime(&rform.rtime));
  prints("�A�ȳ��: %s\n", rform.career);
  prints("�ثe��}: %s\n", rform.address);
  prints("�s���q��: %s\n", rform.phone);
  outs(msg_seperator);
  outc('\n');
  clrtobot();

  if(acct_load(&acct, rform.userid) < 0 /* || acct.userno != rform.userno */)
  {
    vmsg("�d�L���H");
    opt = 'D';
  }
  else
  {
    acct_show(&acct, 2);
#ifdef JUSTIFY_PERIODICAL
    if (acct.userlevel & PERM_VALID && acct.tvalid + VALID_PERIOD - INVALID_NOTICE_PERIOD >= acct.lastlogin)
#else
    if (acct.userlevel & PERM_VALID)
#endif
    {
      vmsg("���b���w�g�������U");
      opt = 'D';
    }
    else if (acct.userlevel & PERM_ALLDENY)
    {
      /* itoc.050405: ���������v�̭��s�{�ҡA�]���|�ﱼ�L�� tvalid (���v����ɶ�) */
      vmsg("���b���ثe�Q���v��");
      opt = 'D';
    }
    else if (opt == 0)
    {
#ifdef USE_DUST_KRANS
      opt = krans(b_lines, "nq", options, "�O�_�����H");
#else
      opt = vans("�O�_����(Y/N/Quit/Del)�H[Q] ");
#endif
    }
  }

  switch (opt)
  {
    case 'd':
#ifdef USE_DUST_KRANS
      opt = krans(b_lines, "nn", del_options, "�O�_�u���R�������U��H");
#else
      opt = vans("�O�_�u���R�������U��(Y/N)�H[N]");
#endif
      if (opt == 'y')
      {
        del_register_file(id);
        rtn = 1;
      }
      break;
    case 'D':
      del_register_file(id); // force delete
      rtn = 1;
      break;
    case 'n':
      if (refuse_register_file(id, &rform, &acct))
      {
        del_register_file(id);
        rtn = 1;
      }
      break;
    case 'y':
      adopt_register_file(id, &rform, &acct);
      del_register_file(id);
      rtn = 1;
      break;
    case 'Y':
#ifdef USE_DUST_KRANS
      opt = krans(b_lines, "nn", del_options, "�O�_�u���q�L�����U��H");
#else
      opt = vans("�O�_�u���q�L�����U��(Y/N)�H[N]");
#endif
      if (opt == 'y')
      {
        adopt_register_file(id, &rform, &acct);
        del_register_file(id);
        rtn = 1;
      }
      break;
    default: // nothing
      break;
  }

  unlink(lckpath);
  scr_restore(&old_screen);
  return rtn;
}

int
fix_register_file(id)
  int id;
{
  char lckpath[IDLEN + 10];
  char admuser[IDLEN + 1];
  char buf[512];
  char opt;
  screen_backup_t old_screen;
  FILE *fp;
#ifdef USE_DUST_KRANS
  char *options[] = {"n����", "y�T�w", NULL};
#endif

  if (register_list_count == 0)
    return 0;

  make_reg_path(lckpath, register_list[id]);
  strcat(lckpath, ".lck");

  if (!dashf(lckpath)) // do not need to fix
    return 0;
  fp = fopen(lckpath, "r");
  if (!fp)
    return 0;
  fscanf(fp, "%s", admuser);
  fclose(fp);

  scr_dump(&old_screen);
#define MARK_STR "��"
  sprintf(buf, "\033[1;31m" MARK_STR "�Фp�ߨϥ�" MARK_STR "\033[m�ثe�� \033[1;36m%s\033[m �f�֡A�T�w�n�״_��", admuser);
#undef MARK_STR
#ifdef USE_DUST_KRANS
  strcat(buf, "�H");
  opt = krans(b_lines, "nn", options, buf);
#else
  strcat(buf, "(Y/N)�H[N]");
  opt = vans(buf);
#endif
  if (opt == 'y')
    unlink(lckpath);

  scr_restore(&old_screen);
  return 1;
}

/* ----------------------------------------------------- */
/* Sub Functions for List controller                     */
/* ----------------------------------------------------- */
#ifdef REG_CACHE_EXPAND
void
register_list_init()
{
  int i;
  register_list = (char**)malloc(sizeof(char*) * REG_CACHE_SIZE);

  for (i = 0; i < REG_CACHE_SIZE; ++i)
    register_list[i] = NULL;

  register_list_page = 1;
  register_list_count = 0;
}

int
register_list_inited()
{
  return (register_list != NULL);
}

void
register_list_push(reg)
  char *reg;
{
  if ((register_list_count % REG_CACHE_SIZE == 0) \
    && (register_list_count > 0))
    register_list_expand(1);

  register_list[register_list_count] = (char *)malloc(sizeof(char) * (IDLEN + 1));

  strcpy(register_list[register_list_count], reg);
  register_list_count++;
}

void
register_list_del(id)
  int id;
{
  int i;

  free(register_list[id]);
  for (i = id + 1; i < register_list_count; ++i)
    register_list[i - 1] = register_list[i];
  register_list[--register_list_count] = NULL;

  if ((register_list_count % REG_CACHE_SIZE == 0) \
    && (register_list_count > 0))
    register_list_expand(-1);
}

void
register_list_expand(page)
  int page;
{
  char **old_register_list;
  int new_size;
  int new_list_count = 0;
  int i;
  old_register_list = register_list;
  new_size = (register_list_page + page) * REG_CACHE_SIZE;

  register_list = (char**)malloc(sizeof(char*) * new_size);
  
  for (i = 0; i < register_list_count; ++i)
    if (i < new_size)
    {
      new_list_count++;
      register_list[i] = old_register_list[i];
    }
    else if (old_register_list[i] != NULL)
      free(old_register_list[i]);
  for (i = register_list_count; i < new_size; ++i)
    register_list[i] = NULL;
  free(old_register_list);

  register_list_count = new_list_count;
  register_list_page += page;
}

void
clear_cache()
{
  int i;
  for (i = 0; i < register_list_count; ++i)
    free(register_list[i]);
  free(register_list);
  register_list = NULL;
}

#else
void
register_list_init()
{
  /* Do nothing */
}

int
register_list_inited()
{
  return 1;
}
#endif


/* ----------------------------------------------------- */
/* ���U��f�֥D�{��                                      */
/* ----------------------------------------------------- */

int
move_cursor(off, page, cursor)
  int off;
  int *page;
  int *cursor;
{
  int item_per_page = b_lines - 2;
  int items = register_list_count;
  int old_page = *page;
  int new_id = old_page * item_per_page + *cursor + off;

  if (register_list_count == 0)
  {
    *page = *cursor = 0;
    return (*page != old_page);
  }

  // fix range [0, items)
  while (new_id < 0)
    new_id = items - 1;
  if (new_id >= items)
    new_id = 0;

  // calc new page & cursor
  *page = new_id / item_per_page;
  *cursor = new_id - *page * item_per_page;

  return (*page != old_page);
}

void
move_page(off, page, cursor)
  int off;
  int *page;
  int *cursor;
{
  int item_per_page = b_lines - 2;
  int items = register_list_count;
  int pages = items / item_per_page + 1;

  if (register_list_count == 0)
  {
    *page = *cursor = 0;
    return;
  }

  *page += off;

  // fix range [0, pages)
  while (*page < 0)
    *page += pages;
  if (*page >= pages)
    *page %= pages;

  // fix cursor
  if (*page == pages - 1)
    if (*cursor >= items - *page * item_per_page)
      *cursor = items - *page * item_per_page - 1;
}

int
a_regmgr()
{
  int page = 0, cursor = 0;
  int key, quit = 0;
  int redraw_page = 1;
  int item_per_page = b_lines - 2;

  read_register_list();
  draw_head();
  draw_footer();
  while (!quit) /* main loop */
  {
    if (redraw_page)
    {
      draw_page(page);
      redraw_page = 0;
    }
    draw_cursor(cursor);
    key = vkey();
    if (key == KEY_LEFT || key == 'e')
      quit = 1;
    else if (key == KEY_DOWN)
      redraw_page = move_cursor(1, &page, &cursor);
    else if (key == KEY_UP)
      redraw_page = move_cursor(-1, &page, &cursor);
    else if (key == KEY_PGDN)
    {
      redraw_page = 1;
      move_page(1, &page, &cursor);
    }
    else if (key == KEY_PGUP)
    {
      redraw_page = 1;
      move_page(-1, &page, &cursor);
    }
    else if (key == ' ')
    {
      if (register_list_count == 0)
        continue;
      if (page == register_list_count / item_per_page)
        if (cursor == register_list_count - page * item_per_page - 1)
          redraw_page = move_cursor(1, &page, &cursor);
        else
          cursor = register_list_count - page * item_per_page - 1;
      else
        if (cursor == item_per_page - 1)
          redraw_page = move_cursor(1, &page, &cursor);
        else
          cursor = item_per_page - 1;
    }
    else if (key == KEY_RIGHT)
    {
      if (register_list_count == 0)
        continue;
      if (process_register_file(page * item_per_page + cursor, 0))
      {
        move_cursor(0, &page, &cursor);
        redraw_page = 1;
      }
    }
    else if (key == 's')
    {
      clear_cache();
      read_register_list();
      redraw_page = 1;
      move_cursor(0, &page, &cursor);
    }
    else if (key == 'q')
    {
      if (register_list_count == 0)
        continue;
      draw_user_infor(page * item_per_page + cursor);
    }
    else if (key == 'd')
    {
      if (process_register_file(page * item_per_page + cursor, 'd'))
      {
        move_cursor(0, &page, &cursor);
        redraw_page = 1;
      }
    }
    else if (key == 'b')
    {
      if (process_register_file(page * item_per_page + cursor, 'n'))
      {
        move_cursor(0, &page, &cursor);
        redraw_page = 1;
      }
    }
    else if (key == 'v')
    {
      if (process_register_file(page * item_per_page + cursor, 'Y'))
      {
        move_cursor(0, &page, &cursor);
        redraw_page = 1;
      }
    }
    else if (key == 'f') // fix register file
    {
      if (fix_register_file(page * item_per_page + cursor))
        redraw_page = 1;
    }
  }
  clear_cache();
  return 0;
}
