/*-------------------------------------------------------*/
/* pset.c						 */
/*-------------------------------------------------------*/
/* target : some personal setting routines		 */
/* create : 10/03/30					 */
/* update : 10/03/30					 */
/*-------------------------------------------------------*/


#include "bbs.h"



/* --------------------------------------------- */
/* �˼ƭp�ɾ��]�w				 */
/* --------------------------------------------- */

static int 
hm_edit(source, echo)
  MCD_info *source;
  int echo;
{
  char buf[16], desc[16];
  int currline;
  MCD_info meter = {0};

  if(echo == GCARRY)
    meter = *source;

  getyx(&currline, NULL);
  if(!vget(currline++, 0, "�˼ƭp�ɾ����D�G", meter.name, 9, echo))
    return 0;
  currline++;

  if(echo == GCARRY)
    sprintf(buf, "%d", meter.year);
  if(!vget(currline, 0, "�~(�褸)�G", buf, 5, echo))
    return 0;
  meter.year = atoi(buf);

  if(echo == GCARRY)
    sprintf(buf, "%d", meter.mon);
  vget(currline, 20, "��(1-12)�G", buf, 3, echo);
  meter.mon = (atoi(buf) + 11) % 12 + 1;

  if(echo == GCARRY)
    sprintf(buf, "%d", meter.day);
  desc[0] = MonthToDay(meter.mon, meter.year);	/* ��desc[0]�ӥ� */
  sprintf(desc + 1, "��(1-%d)�G", desc[0]);
  vget(currline++, 38, desc + 1, buf, 3, echo);
  meter.day = (atoi(buf) - 1 + desc[0]) % desc[0] + 1;

  if(echo == GCARRY)
    sprintf(buf, "%d", meter.hr);
  vget(currline, 0, "�X��(0-23)�G", buf, 3, echo);
  meter.hr = atoi(buf) % 24;

  if(echo == GCARRY)
    sprintf(buf, "%d", meter.min);
  vget(currline++, 21, "�X��(0-59)�G", buf, 3, echo);
  meter.min = atoi(buf) % 60;

  if(echo == GCARRY)
    sprintf(buf, "%d", meter.warning);
  vget(currline, 0, "ĵ�ܤѼ�(�Ѿl�֩󦹤ѼƮɭp�ɾ��N�|�ܦ�C����J��0��ܤ�ĵ��)�G", buf, 3, 

echo);
  meter.warning = atoi(buf);

  if(echo == GCARRY && !memcmp(source, &meter, sizeof(MCD_info)))
    return 0;

  *source = meter;

  return 1;
}


int
hm_admin()
{
  char *menu[] = {"e�s��", "n�s�W", "q���}", NULL};
  FILE *fp;
  MCD_info meter;
  int ch;

  ch = krans(b_lines, "nq", menu, "���˼ƭp�ɾ��G");
  if(ch != 'q')
  {
    move(MENU_XPOS, 0);
    clrtobot();

    switch(ch)
    {
    case 'e':
      if(fp = fopen(FN_ETC_MCD, "rb"))
      {
	fread(&meter, sizeof(MCD_info), 1, fp);
	fclose(fp);

	if(hm_edit(&meter, GCARRY))
	{
	  if(fp = fopen(FN_ETC_MCD, "wb"))
	  {
	    fwrite(&meter, sizeof(MCD_info), 1, fp);
	    fclose(fp);
	    vmsg("�ק粒��");
	  }
	}
	break;
      }

    case 'n':
      if(!hm_edit(&meter, DOECHO))
	break;

      if(fp = fopen(FN_ETC_MCD, "wb"))
      {
	fwrite(&meter, sizeof(MCD_info), 1, fp);
	fclose(fp);
	vmsg("�إߧ���");
      }
      break;
    }
  }

  return 0;
}



int
hm_user()
{
  char def_desc[] = "Nq";
  char *menu[13] = {def_desc, "N �إ߷s���˼ƭp�ɾ�"};
  char *leave = "q ��^";
  char cmd_def[] = " q";
  char *cmd[] = {"a�ĥ�", "e�ק�", "d�R��", "q��^", NULL};

  FILE *fp;
  char fpath[64], ans[3];
  MCD_info pool[9];
  int size, ch, i = 0, applied = -1;

  usr_fpath(fpath, cuser.userid, FN_MCD);
  if(fp = fopen(fpath, "rb"))
  {
    for(; i < 9; i++)
    {
      if(fread(pool+i, sizeof(MCD_info), 1, fp) < 1)
	break;

      menu[i+2] = (char*) malloc(16 * sizeof(char));
      sprintf(menu[i+2], "%d    %s", i + 1, pool[i].name);
      if(pool[i].warning & DEFMETER_BIT)
      {
	applied = i;
	pool[i].warning &= ~DEFMETER_BIT;	/* �٭�^�쥻���Ʀr */
	strcpy(menu[i+2] + 2, "��");
	menu[i+2][4] = ' ';
      }
    }
    fclose(fp);
  }
  menu[i + 2] = leave;
  size = i;
  if(applied >= 0)
    def_desc[0] = applied + '1';

  clear();
  vs_bar("�ӤH�˼ƭp�ɾ��]�w");

  do
  {
    ch = pans((18-size)/2-1, 20, "�n�ĥέ��ӭ˼ƭp�ɾ��O�H", menu);

    if(ch == 'n')
    {
      def_desc[0] = 'N';
      if(size < 9)
      {
	move(8, 0);
	clrtobot();
	outs("\033[1;36m�� �إ߷s���˼ƭp�ɾ�\033[m\n");
	outs("\033[1;33m�񧴥H�U��T�A�z�N�i�H�s�y�X�s���˼ƭp�ɾ��� :D\033[m\n\n");
	if(hm_edit(&pool[size], DOECHO))
	{
	  menu[size+2] = (char*) malloc(16 * sizeof(char));
	  sprintf(menu[size+2], "%d    %s", size + 1, pool[size].name);
	  size++;
	  menu[size+2] = leave;
	  def_desc[0] = size + '0';
	}
	clrregion(8, 15);
      }
      else
	outz(" �˼ƭp�ɾ����ƶq�w�F�W���A�L�k�A�s�W�C");
    }
    else if(ch >= '1' && ch <= '9')
    {
      def_desc[0] = ch;
      ch -= '1';

      sprintf(fpath, "�z��ܤF %s  ", menu[ch+2] + 5);
      if(ch == applied)
	cmd_def[0] = 'e';
      else
	cmd_def[0] = 'a';

      switch(krans(b_lines, cmd_def, cmd, fpath))
      {
      case 'a':
	strcpy(menu[ch+2] + 2, "��");
	menu[ch+2][4] = ' ';
	if(applied >= 0)
	  memset(menu[applied+2] + 2, ' ', 2);
	applied = ch;
	break;

      case 'e':
	clrcurln();
	move(9, 0);
	if(hm_edit(&pool[ch], GCARRY))
	  strcpy(menu[ch+2]+5, pool[ch].name);

	clrregion(9, 13);
	continue;	/* �w�g���ݭn clear current line �F */

      case 'd':
	if(vget(b_lines - 1, 0, MSG_SURE_NY, ans, sizeof(ans), DOECHO) != 'y')
	  break;

	free(menu[ch+2]);
	for(i = ch + 1; i < size; i++)
	{
	  pool[i-1] = pool[i];
	  menu[i+1] = menu[i+2];
	  menu[i+1][0]--;
	}

	if(ch == size - 1)
	  def_desc[0]--;
	size--;
	if(size == 0)
	  def_desc[0] = 'N';

	menu[size+2] = leave;
	menu[size+3] = NULL;

	if(applied == ch)
	  applied = -1;

	clrregion(b_lines - 1, b_lines);
	continue;
      }

      clrcurln();
    }

  }while(ch != 'q');

  if(applied >= 0)
    pool[applied].warning |= DEFMETER_BIT;

  usr_fpath(fpath, cuser.userid, FN_MCD);
  if(fp = fopen(fpath, "wb"))
  {
    for(i = 0; i < size; i++)
    {
      fwrite(pool+i, sizeof(MCD_info), 1, fp);
      free(menu[i+2]);
    }
    fclose(fp);
  }

  hm_init();
  return 0;
}



/* --------------------------------------------- */
/* ñ�W�ɽs�褶��				 */
/* --------------------------------------------- */

#define REDRAW_SIGN	0x01
#define REDRAW_LFIELD	0x02
#define REDRAW_RFIELD	0x04
#define REDRAW_CROSS	0x08
#define REDRAW_NECK	0x10
#define CLEAR_PREV	0x20
#define LONG_HIGHESTBYTE_MASK 0x00FFFFFF
#define ITEM_SX 44
#define TMP_SIG ('x' - '1')
#define MS_NECK1 "\033[1;33m[Enter]\033[m��ܡB\033[1;33m[�Ʀr��]\033[m���ܹ��������ءC\n"
#define MS_NECK2 "\033[1;33m[C]\033[m�����B\033[1;33m[Enter]\033[m��ܡB\033[1;33m[Q]\033[m���}\n"

#define SETOLDPOSITION(a) ((a) + 1 << 24)
#define SET_SIGPATH(path, n) path[signame_start] = (n) + '1'
#define SET_ITMCOLOR(fst) (fst)? attrset(7) : attrset(8)


int
u_sign()
{
  char *menu[] = {"e�ק�", "d�R��", "s�洫", "c����", NULL};

  screen_backup_t old_screen;
  char sigpath[64], buf[64];
  usint redraw, exmode;
  int i, key, cc, currsign, file_state[9], signame_start;
  int sig_num = 6 + cuser.sig_plus;
  int midline = 9 - sig_num / 2;
  int currfield;			/* 0: left field   1: right field */


  move(1, 0);
  clrtobot();

  /* draw the frames */
  move(3, 0);
  for(i = 0; i < 39; i++)
    outs("�w");
  move(3, 36);
  outs("�s");

  move(15, 0);
  outs("�w      ");
  for(i = 0; i < 35; i++)
    outs("�w");

  move(15, 36);
  outs("�r");

  for(i = 4; i <= 14; i++)
  {
    move(i, 36);
    outs("�x");
  }


  move(5, 1);
  if(cuser.sig_avai > cuser.sig_plus + 6)
    cuser.sig_avai = cuser.sig_plus + 6;
  prints("���i��ñ�W�ɼƶq�G\033[1;36m%d\033[m", cuser.sig_avai);

  move(8, 8);
  outs("���ܥi��ñ�W�ɼƶq");

  move(10, 13);
  outs("(Q) ���}");


  usr_fpath(sigpath, cuser.userid, FN_SIGN ". ");
  signame_start = strlen(sigpath) - 1;

  for(i = 0; i < sig_num; i++)
  {
    SET_SIGPATH(sigpath, i);
    file_state[i] = dashf(sigpath);

    move(midline + i, ITEM_SX + 3);
    SET_ITMCOLOR(file_state[i]);
    prints("(%d) %d��ñ�W��", i + 1, i + 1);
    attrset(7);
  }


  exmode = cc = currsign = 0;
  redraw = REDRAW_SIGN | REDRAW_RFIELD | REDRAW_CROSS | REDRAW_NECK;
  do
  {
    /* ��ø�B�z */
    if(redraw & REDRAW_NECK)
    {
      move(2, 0);
      outs("\033[1;36m��\033[m �Ы�\033[1;33m[��V��]\033[m���ʥ��ΡB");
      if(exmode)
	outs(MS_NECK2);
      else
	outs(MS_NECK1);

      redraw &= ~REDRAW_NECK;
    }

    if(redraw & REDRAW_LFIELD)		/* ����ϰ� */
    {
      if(redraw & REDRAW_CROSS)
      {
	redraw &= ~REDRAW_CROSS;
	move(midline + currsign, ITEM_SX);
	SET_ITMCOLOR(file_state[currsign]);
	prints("   (%d) %d��ñ�W��   ", currsign + 1, currsign + 1);
	attrset(7);
	currfield = 0;
      }

      move(7 + !cc * 2, 5);	/* �ΤF logic NOT...��b���O�Ӧn��k... */
      outs("                        ");
      move(9 + !cc * 2, 5);
      outs("                        ");
      move(7 + cc * 2, 5);
      attrset(12);	/* [1;34m */
      outs("��                    ��");
      move(9 + cc * 2, 5);
      outs("��                    ��");
      attrset(7);

      redraw &= ~REDRAW_LFIELD;
    }

    if(redraw & REDRAW_RFIELD)		/* �k��ϰ� */
    {
      if(redraw & REDRAW_CROSS)
      {
	redraw &= ~REDRAW_CROSS;
	move(7 + cc * 2, 5);
	outs("                        ");
	move(9 + cc * 2, 5);
	outs("                        ");
	currfield = 1;
      }

      i = (redraw >> 24) - 1;
      if(i >= 0)
      {
	move(midline + i, ITEM_SX);
	SET_ITMCOLOR(file_state[i]);
	prints("   (%d) %d��ñ�W��   ", i + 1, i + 1);
      }
      attrset(0x4F);	/* [1;44m */
      move(midline + currsign, ITEM_SX);
      prints("�i (%d) %d��ñ�W�� �j", currsign + 1, currsign + 1);
      attrset(7);

      redraw &= LONG_HIGHESTBYTE_MASK;
      redraw &= ~REDRAW_RFIELD;
    }

    if(redraw & REDRAW_SIGN)		/* ��ø�w���� */
    {
      if(redraw & CLEAR_PREV)
      {
	clrregion(16, 21);
	redraw &= ~CLEAR_PREV;
      }

      move(15, 3);
      if(file_state[currsign])
      {
	outs("\033[1m�w��\033[m");

	SET_SIGPATH(sigpath, currsign);
	move(16, 0);
	lim_cat(sigpath, 6);
	redraw |= CLEAR_PREV;
      }
      else
	outs("\033[1;30m�w��\033[m");

      redraw &= ~REDRAW_SIGN;
    }

    /* ����B�z */
    move(b_lines, 0);
    key = vkey();

    if(key == '\n')
    {
      if(exmode)			/* �洫�Ҧ� */
      {
	i = exmode >> 1;
	if(i != currsign)
	{
	  /* �洫file state */
	  int tmp = file_state[currsign];
	  file_state[currsign] = file_state[i];
	  file_state[i] = tmp;

	  strcpy(buf, sigpath);

	  /* �洫�ɦW */
	  SET_SIGPATH(sigpath, currsign);
	  SET_SIGPATH(buf, TMP_SIG);
	  f_mv(sigpath, buf);	//mv [sig1] [tmp]

	  SET_SIGPATH(sigpath, i);
	  SET_SIGPATH(buf, currsign);
	  f_mv(sigpath, buf);	//mv [sig2] [sig1]

	  SET_SIGPATH(sigpath, TMP_SIG);
	  SET_SIGPATH(buf, i);
	  f_mv(sigpath, buf);	//mv [tmp] [sig2]

	  redraw |= REDRAW_RFIELD | REDRAW_SIGN | SETOLDPOSITION(i);
	}

	clrregion(b_lines, b_lines);
	redraw |= REDRAW_NECK;
	exmode = 0;
      }
      else if(currfield)
      {
	SET_SIGPATH(sigpath, currsign);
	switch(krans(b_lines, "ec", menu, "�аݧA�n...... "))
	{
	case 'e':
	  clrcurln();
	  scr_dump(&old_screen);
	  if(!vedit(sigpath, 0))
	  {
	    file_state[currsign] = 1;
	    redraw |= REDRAW_SIGN;
	  }
	  scr_restore(&old_screen);
	  break;

	case 'd':
	  if(vans(MSG_SURE_NY) == 'y')
	  {
	    unlink(sigpath);
	    file_state[currsign] = 0;
	    redraw |= REDRAW_SIGN;
	  }
	case 'c':
	  clrcurln();
	  break;

	case 's':
	  exmode = (currsign << 1) | 1;
	  redraw |= REDRAW_NECK;

	  move(b_lines, 0);
	  outs("�п�ܭn�M����ñ�W�ɥ洫��m�C\n");
	}
      }
      else if(cc)		/* (Q)���} */
      {
	key = 'q';
      }
      else			/* ���ܥi��ñ�W�ɼƶq */
      {
	sprintf(buf, "�Q�n�վ㦨�h�֩O�H(1~%d) ", sig_num);
	i = vans(buf) - '0';
	clrcurln();
	if(i > 0 && i <= sig_num && i != cuser.sig_avai)
	{
	  cuser.sig_avai = i;
	  move(5, 19);
	  prints("\033[1;36m%d\033[m", i);
	}
      }
    }
    else if(key == KEY_LEFT && currfield && !exmode)
    {
      redraw |= REDRAW_LFIELD | REDRAW_CROSS;
    }
    else if(key == KEY_RIGHT && !currfield && !exmode)
    {
      redraw |= REDRAW_RFIELD | REDRAW_CROSS;
    }
    else if(key == KEY_UP)
    {
      if(currfield)
      {
	if(currsign > 0)
	{
	  redraw |= REDRAW_RFIELD | REDRAW_SIGN | SETOLDPOSITION(currsign);
	  currsign--;
	}
      }
      else if(cc)
      {
	cc = 0;
	redraw |= REDRAW_LFIELD;
      }
    }
    else if(key == KEY_DOWN)
    {
      if(currfield)
      {
	if(currsign < sig_num - 1)
	{
	  redraw |= REDRAW_RFIELD | REDRAW_SIGN | SETOLDPOSITION(currsign);
	  currsign++;
	}
      }
      else if(!cc)
      {
	cc = 1;
	redraw |= REDRAW_LFIELD;
      }
    }
    else if(key == KEY_HOME && currfield)
    {
      if(currsign != 0)
      {
	redraw |= REDRAW_RFIELD | REDRAW_SIGN | SETOLDPOSITION(currsign);
	currsign = 0;
      }
    }
    else if(key == KEY_END && currfield)
    {
      if(currsign != sig_num - 1)
      {
	redraw |= REDRAW_RFIELD | REDRAW_SIGN | SETOLDPOSITION(currsign);
	currsign = sig_num - 1;
      }
    }
    else if(key > '0' && key <= sig_num + '0')
    {
      redraw |= REDRAW_RFIELD | REDRAW_SIGN | SETOLDPOSITION(currsign);
      currsign = key - '1';
      if(!currfield)
	redraw |= REDRAW_CROSS;
    }
    else if(exmode && tolower(key) == 'c')
    {
      clrregion(b_lines, b_lines);
      redraw |= REDRAW_NECK;
      exmode = 0;
    }

  }while(key != 'q' && key != 'Q');

  return 0;
}

