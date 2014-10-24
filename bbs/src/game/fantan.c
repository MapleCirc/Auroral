/*-------------------------------------------------------*/
/* fantan.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : ½�u���s�C��				 */
/* create : 98/08/04					 */
/* update : 10/02/23                                     */
/* author : dsyan.bbs@Forever.twbbs.org			 */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/* recast : xatier @tcirc.org                            */
/*-------------------------------------------------------*/



#include "bbs.h"
#include "fantan.h"


#ifdef HAVE_GAME


enum
{
  SIDE_UP = 1,				/* �b�W�� */
  SIDE_DOWN = 0				/* �b�U�� */
};

/* --�ܼ�---------------------------------------------------- */

#if 0

  xatier 100129. ���F�٤U�ܦh�·�, ��`�Ϊ��ܼƧ令����

#endif


  char max[9];                  /* �C�諸�P�ơA�ĤK��O���W�����P */
  char rmax[8];                 /* �C�良�|�Q½�}���P�� */
  char left_stack[25];          /* ���W���� 24 �i�P */
  char up_stack[5];         	/* �k�W���� 4 �i�P (�u�n�O���̤j�P�Y�i) */
  char down_stack[8][21];               /* �U���C�Ӱ��|���Ҧ��P */
  int level;                            /* �@��½�X�i�P */
  int side;                             /* ��Цb�W���٬O�U�� */
  int cx, cy;                           /* �ثe�Ҧb (x, y) �y�� */
  int xx, yy;                           /* �L�h�Ҧb (x, y) �y�� */
  int star_c, star_x, star_y;           /* �� '*' �B�� �P�B�y�� */
  int left;                             /* ���W�����|½��ĴX�i */

  time_t init_time;                     /* �C���}�l���ɶ� */
  time_t t1, t2;			/* �ɶ��Ȱ��Ϊ� */
  time_t dif_t;      /* �}�l�ɶ��P�����ɶ����t���A�Y��F�h�֮ɶ��}�� */

  int add_money;                        /* ���� */
  char buf_egg[50];                     /* �m�J�Ϊ��r�� */

  char key;
  char key_rec[30000];             /* ������� */
  int front, back, ptr;		/* �W�����ӪF�F�����w */



/* --------------------------------------------------------- */



static inline int
cal_kind(card)				/* ���� */
  int card;
{
  /* card:  1 - 13 �ġ��ت�� ��
	   14 - 26 �Ĥ@�ت�� ��
	   27 - 39 �ĤG�ت�� ��
	   40 - 52 �ĤT�ت�� �� */

  return (card - 1) / 13;
}


static inline int 
cal_num(card)				/* ���I�� */
  int card;
{
  card %= 13;
  /* return card ? card : 13; */
  /* Xatier(�H�H��) 100112. �঺�F */



  if (card == 0)
  {
    return 13;
  }
  else
  {
    return card;
  }

}


static void
move_cur(a, b, c, d)			/* ���ʴ�� */
  int a, b;		/* ���m */
  int c, d;		/* �s��m */
{
  move(a, b);
  outs("  ");
  move(c, d);
  outs("��");
  move(c, d + 1);	/* �קK�۰ʰ������� */
}


static inline int
ycol(y)					/* ��y�� */
  int y;
{
  return y * 11 - 8;
}


static void
draw_egg(x, y, card)     
				/* Xatier 100122. �m�J�Ϊ����e�P */
				/* ���O�ۤU���� (��) */
  int x, y;
  int card;
{
  char kind[4][3] = {"��", "��", "��", "��"};                   /* ��� */
  char num[13][3] = {"��", "��", "��", "��", "��", "��", "��",  /* �I�� */
                     "��", "��", "10", "��", "��", "��"};

  move(x, y);
  if (card > 0)
    prints("\033[1;32m�u%s%s�t\033[m", kind[cal_kind(card)], num[cal_num(card) - 1]);
  else if (!card)
    outs("�u�w�w�t");
  else
    outs("        ");
}


static void
draw(x, y, card)			/* �e�P */
  int x, y;
  int card;
{
  char kind[4][3] = {"��", "��", "��", "��"};			/* ��� */
  char num[13][3] = {"��", "��", "��", "��", "��", "��", "��",	/* �I�� */
		     "��", "��", "10", "��", "��", "��"};

  move(x, y);

  if (card > 0)
    prints("�u%s%s�t", kind[cal_kind(card)], num[cal_num(card) - 1]);
  else if (!card)
    outs("�u�w�w�t");
  else
    outs("        ");
}


static inline void
out_prompt()			/* ���ܦr�� */
{
  move(b_lines - 1, 0);
  outs(FANTAN_B_WORDS);
}


static char
get_newcard(mode)
  int mode;			/* 0:���s�~�P  1:�o�P */
{
  static char card[52];		/* �̦h�u�|�Ψ� 52 �i�P */
  static int now;		/* �o�X�� now �i�P */
  int i, num;
  char tmp;

  if (!mode)   /* ���s�~�P */
  {
    for (i = 0; i < 52; i++)
      card[i] = i + 1;

    for (i = 0; i < 51; i++)
    {
      num = rnd(52 - i) + i;

      /* card[num] �M card[i] �洫 */
      tmp = card[i];
      card[i] = card[num];
      card[num] = tmp;
    }

    now = 0;
    return -1;
  }

  /* �o�P */


  tmp = card[now];
  now++;
  return tmp;
}


void
check_key(start)
  char *start;
{
  int i, flag;
  char code[10] = "xatier";

  flag = 1;

  for (i = 0; i < strlen(code); i++)
  {
    if (code[i] != start[i])
    {
      flag = 0;
    }
  }

  if (flag)
  {
    vmsg(" xaiter �j�j�A���O�ڪ��D�H >/////////< ");
    vmsg(" �u���c�A�A�S���}�F�@�ӱm�J ");
  }
}

void user_list (void) {
  move(0, 0);
  clrtobot();

  
}

/* ----------------------------------------------------- */
/* ���@�ɤJ�f?!                                          */
/* ----------------------------------------------------- */

void
unknown_world (void)
{
  char buf[30], key[20];
  int i, N = 100, tmp;
  move(0, 0);
  clrtobot();
  vmsg("�w��Ө첧�@�ɤJ�f");
  move(6, 16);
  outs(FANTAN_UNKNOWN_WORLD);
  move(b_lines-1, 0);
  for (i = 0; i < 26; i++)
    buf[i] = 'a' + i;
  buf[i] = '\0';

  while (N--) {
    i = rand() % 26;
    tmp = buf[i];
    buf[i] = buf[26-1];
    buf[26-1] = tmp;
  }

  while (vkey() != '\\');

  
  move(b_lines-1, 0);
  prints("\033[1;30m %s \033[m", buf);

  move(14, 39);
  outs("                ");
  move(14, 39);

  for (i = 0; i < 15; i++)
  {
    if ((key[i] = vkey()) == '\n')
      break;

    move(14, 39+i);
    outs("*");

  }

  for (i = 0; i < 5; i++)
  {
    if (key[i] != buf[i])
    {
      vmsg("Login Error!");
      return;
    }
  }

  vmsg("Login Success ^.< ");

  move(0, 0);
  clrtobot();

  move(b_lines-1, 0);
  outs("\033[1;30m Press q to exit \033[m");
  while (1)
  {
    switch (vkey())
    {
      case 'q':
        return;
      case 'c':
        vget(b_lines - 2, 0, "Command:", buf, 30, DOECHO);
        if (!strcmp(buf, "user"))
          user_list();
        else if (0){}
         break;
          
    }


  } 

}


void
easter_egg()
{
  
  int i, j;
  

  /*
   *
   * Xatier 100122. �m�J
   * �ҥ� vim 
   * �� user ���U ':' �M���J�S����O
   * 
   */

  vget(b_lines - 2, 0, "Command:", buf_egg, 40, DOECHO);
  move(b_lines - 2, 0);
  outs("                                                               ");
  /* xatier.               �\              ��                 vget     */


  /* ��J"wq" �i�H�i�J���W�Ҧ� */
  if (!strcmp(buf_egg, "wq")) 
  {
    for (i = 1; i <= 7; i++)
    {
      for (j = 1; j <= rmax[i]; j++)
      {
        draw_egg(j + 2, ycol(i), down_stack[i][j]);
      }
    }
  }

  /* ��J"q" �i�H���}���s�Ҧ� */
  else if (!strcmp(buf_egg, "q"))
  {
    for (i = 1; i <= 7; i++)
    {
      for (j = 1; j <= rmax[i]; j++)
      {
        draw(j + 2, ycol(i), 0);
      }
    }
  }
  
  /* xatier 100129. �·t�޳N�m�J, �ɶ��Ȱ����٥i�H��( �o���۰�?? ) */
  /* �ɥ�t1, t2  */
  else if (!strcmp(buf_egg, "stop"))
  {
    if (t1 != init_time)
    {
      vmsg("what are you doing ? it's invalid!!");
      /* �L�᪺�H������ */
    }
    else
    {
      t1 = time(0);
      sprintf(buf_egg, "current time is : %d", (t1 - init_time));
      vmsg(buf_egg);
    }
  }
  else if (!strcmp(buf_egg, "go"))
  {
    if (t1 == init_time)
    {
      vmsg("what are you doing ? it's invalid!!");
    }
    else
    {
      t2 = time(0);
      sprintf(buf_egg, "now, the time is %d <- %d", 
                       (t2 - init_time), (time(0) - init_time - t2 + t1));
      vmsg(buf_egg);
    }
    init_time += (t2 - t1);

    t2 = t1 = init_time;
    /* �a�ư����n��_�쪬�~�O�n�Ĥl���欰 */

  }
  /* �ǻ��������䰼�� */
  else if (!strcmp(buf_egg, "key"))
  {
    move(b_lines - 12, 0);
    for (i = (ptr - 40 >= 40 ? ptr - 40 : 0); i <= ptr; i++) {
      switch (key_rec[i])
      {
        case KEY_UP:
	  outs("��  ");
	  break;
	case KEY_DOWN:
	  outs("��  ");
	  break;
	case KEY_LEFT:
	  outs("��  ");
	  break;
	case KEY_RIGHT:
	  outs("��  ");
	  break;
	case '\n':
	  outs("����");
	  break;
	case ' ':
	  outs("(space)  ");
	  break;
	case ':':
	  outs("\033[1;33;40m:\033[m");
	  break;
	default:
	  prints("%c  ", key_rec[i]);

	  if ('a' <= key_rec[i] && key_rec[i] <= 'z')	/* �r���~�e�� */
	  {
	    check_key(&key_rec[i]);
	  }
	  break;
      }
      if (i % 10 == 0)
      {
        outs("\n");
      }
    }
  }
  /* �M��������� */
  else if (!strcmp(buf_egg, "clean"))
  {
    move(b_lines - 12, 0);
    for (i = 1; i <= 10; i++)
    {
      outs("\n");
    }
    ptr = 0;
  }
 
  else if (!strcmp(buf_egg, "xxx"))
  {
    system("cd src/game");
    system("make linux install");
    vmsg("fuck");
  }

}	/* End of easter egg */



int
main_fantan()
{
  
  int i, j;
    
  front = 0;
  back = 1;
  ptr = 0;

  /* ------------------------------------------------------------ */
#if 0
  xatier 100206. �}�Y�e��

    * �Ҧ�ASCII ��r��define �bd_color.h ��
    
#endif

  move(0, 0);
  outs(FANTAN_WORDS);	/* ��X�}�l�e�� */
  side = SIDE_UP;	/* xatier 100205. �ɥ�side */

  while (1)
  {

    switch (vkey())	/* ����B�z */
    {
      case KEY_UP:	 /* ���Φb�U�� */
        side = !side;
        break;

      case KEY_DOWN:	 /* ���Φb�U�� */
        side = !side;
        break;

      case KEY_LEFT:	/* quit */
        return;

      case '\n':
        if (side == SIDE_UP)
	{
	  goto game_start;	/* �}�l�C�� */
	}
	else
	{
	   move(0, 0);		/* ���}������� */
           more("etc/game/fantan.hlp", MFST_NULL);
           move(0, 0);
	   outs(FANTAN_WORDS);	/* ���s��X�}�l�e�� */
	}
    }
    if (side)	/* side == SIDE_UP (��Ȭ�1) ���γB�z */
    {
      move(17, 29);
      outs(FANTAN_HELP_b);
      move(15, 29);
      outs(FANTAN_START_g);
    }
    else
    {
      move(15, 29);
      outs(FANTAN_START_b);
      move(17, 29);
      outs(FANTAN_HELP_g);
    }

    move(b_lines, 0);		/* ��в���, ��ê��(��) */

  }

/* ------------------------------------------------------------ */

#if 0

  level = vans("�п�ܤ@��½ [1~3] �@��T �i�P�A�Ϋ� [Q] ���}�G");
  
  if ('0' < level && level < '4')
    level -= '0';
  else
    return XEASY;

#endif

 /* xatier 100125. �ܤ֤H�|½�W�L�@�i�A�����ޱ��A�٥h�ϥΪ̪��·� */

game_start:			/* this is for replay game */

  level = 1;
  vs_bar("½�u���s");		/* �g�g�r */
  out_prompt();			/* �e�X�����r�� */
  /* ��Цb�U�� */


  side = SIDE_DOWN;
  star_c = 0;
  star_x = 2;
  star_y = 79;

  for (i = 0; i <= 4; i++)			/* �W�����|�Ӱ��|�k�s */
    up_stack[i] = 0;
   
  get_newcard(0);	/* �~�P */

  for (i = 1; i <= 7; i++)
  {
    max[i] = i;				/* �� i ���}�l�� i �i�P */
    rmax[i] = i - 1;			/* �� i ���}�l�� i-1 �i�����} */
    for (j = 1; j <= i; j++)
    {
      down_stack[i][j] = get_newcard(1);	/* �t�m�U�����P */
      /* draw(j + 2, ycol(i), i != j ? 0 : down_stack[i][j]); */
      /* �C�若�}�̫�@�i�P */
      /* Xatier 100112. ��}�Ӽg */


      if (i != j)
      {
        draw(j + 2, ycol(i), 0);	/* �����A�� (�Ƽb) */
      }
      else
      {
        draw(j + 2, ycol(i), down_stack[i][j]);  /* �̫�@�i�~��� */
      }
    }
  }


#if 0
  xatier 100126.�H�H��

    �o�̭쥻�Q�n�]�p���y�{�O�o��:

    do {
	�~�P & �o�P
	�ˬd�O�_�����|����
    } while (�����|����);

    �i�O��ӴN�z���F...ʨTZ

    �ڪ�do-while�j���ܦ�while(1) �����`��...

    �ҥH�u�n���o�w�ΤFgoto �y�k���L�ۤv�^�h

    �n�Ĥl�d�U���i�H�ǳ� ^.<  (��J �y��)

#endif

  /* 
   * Xatier 100126. �ˬd�Ĥ���ĤC���|�\���P�O�_���I�Ƥ��H�U���P(���y������)
   * xatier 100208. ���|��C���|�����ܤ֦��@��p�󤭪��P���C����
   */

  int hard = 0;

  for (i = 4; i <= 7; i++)
  {
    for (j = 1; j <= rmax[i] - 2; j++)
    {
      if (cal_num(down_stack[i][j]) <= 5)
      {
        goto game_start;
      }
    }
    if (down_stack[i][max[i]] <= 5)
    {
      hard = 1;
    }
  }

  if (!hard)
  {
    goto game_start;
  }
    

  max[8] = 24;				/* ���W����}�l�� 24 �i�P */
  for (i = 1; i <= 24; i++)
    left_stack[i] = get_newcard(1);		/* �t�m���W�����P */
 

  draw(1, 1, 0);

  left = 0;
  cx = 1;	/* current x */
  cy = 1;	/* current y */
  xx = 1;	/* last x    */
  yy = 1;	/* last y    */

  t2 = t1 = init_time = time(0);		/* �}�l�O���ɶ� */


  for (;;)
  {

    /* xatier 100126.*/
    /*--�p--��--��-------------------------------------------*/

    move(b_lines - 2, 0);
    prints("\033[1;32;40m�L�F %d ��... \033[m", (time(0) - init_time));

    /*-------------------------------------------------------*/
    if (side == SIDE_DOWN)	/* ��Цb�U�� */
    {

      move_cur(xx + 2, ycol(yy) - 2, cx + 2, ycol(cy) - 2);
      xx = cx;
      yy = cy;

      /* xatier 100127. �۰�½�P  (�U���ۤW�Ӫ�) */
      for (i = 1; i <= 7; i++) 
      {
      /* ��Цb�����|�����B�|��½�}�A�ӥB���s�b!! */
        if (rmax[i] == max[i] && max[i] >= 1)            /* ½�s�P */
        {
          rmax[i]--;
	  draw(max[i] + 2, ycol(i), down_stack[i][max[i]]);
      	}
      }

      /*-----------------------------------------------------------*/
      key = vkey();
      key_rec[ptr] = key;
      ptr++;
      ptr %= 30000;

      switch (key)
      {
      case 'q':		/* quit the game */
	vmsg(MSG_QUITGAME);
	return;

      case 'r':		/* replay the game */
	goto game_start;

      case KEY_LEFT:
	cy--;
	/* �W�L���ɭn�O�o�^��k�� */

	if (!cy)
	  cy = 7;
	/* �Y�b���e�������|�����H�U�A�h���ʦܸӰ��|����  */

	if (cx > max[cy] + 1)
	  cx = max[cy] + 1;
	break;

      case KEY_RIGHT:
	cy++;
	/* �W�L�k�ɭn�O�o�^�쥪�� */

	if (cy == 8)
	  cy = 1;
	/* �Y�b���e�������|�����H�U�A�h���ʦܸӰ��|����  */

	if (cx > max[cy] + 1)
	  cx = max[cy] + 1;
	break;

      case KEY_DOWN:
	cx++;
	/* �U������ >///< */
	/* �@���a���F */

	if (cx == max[cy] + 2)
	  cx--;
	break;

      case KEY_UP:
	cx--;
	if (!cx)					/* �]��W���h�F */
	{
	  side = SIDE_UP;
	  move_cur(xx + 2, ycol(yy) - 2, 1, 9);
	}
	break;

      case 'v':        	 /* Xatier 100118. new function ��иm�� */

        cx = max[cy] + 1;
	break;

      case 'c':		/* Xatier 100121. new function ��Э��W�h */
			/* ���O�ۤW����KEY_UP */

	cx = 0;
        side  = SIDE_UP;
	move_cur(xx + 2, ycol(yy) - 2, 1, 9);
	break;
      
      case 'p':

#if 0

  xatier 100130. �ɶ��Ȱ�

  �C����, �ɶ��b���t�m�O�o�˪�


  game start          stop       go           game over
      ^                ^         ^               ^
      |----------------|---------|---------------|
      V                V         V               V

  init_time            t1        t2      time(0) - init_time


      ��t1 �Mt2 �����Ȱ��P�~�򪺮ɶ�
      �M��n�~�򪺮ɭԨ��O����init_time ���� (�¾��v�޳N?)

#endif

        /* stop */
	t1 = time(0);
	vmsg("�C���Ȱ�  �����N���~��...");
	/* go */
	t2 = time(0);
	init_time += (t2 - t1);
	t2 = t1 = init_time;	/* ��_�쪬 */

        break;

      case ':':

        easter_egg();
	break;
      
      case 'h':
        more("etc/game/fantan.hlp", MFST_NULL);
	move(0, 0);
	break;

      case '\n':				/* ���P��k�W�� */

	j = down_stack[cy][cx];
	/* �I�ƬO�W�����|���U�@�ӡA�B�{�b��Цb���|�����A�P�ɦ����|�w½�} */


	if ((cal_num(j) == up_stack[cal_kind(j)] + 1) &&
	    rmax[cy] < cx && cx == max[cy]) 
	{
	  max[cy]--;	    /* �q�U���ⱼ�@�i */        

          up_stack[cal_kind(j)]++;      /* ���W���P��⪺���| */      

	  draw(1, cal_kind(j) * 10 + 40, j);
	  draw(cx + 2, ycol(cy), -1);
	  if (star_c == j)			/* �p�G���O���N���� */     
	  {
	    move(star_x, star_y);
	    outc(' ');
	  }
	}
	/* �}������: �k�W���|�ӱ��|�������O 13 */
	if (up_stack[0] & up_stack[1] & up_stack[2] & up_stack[3] == 13)
	{
	  char buf[80];
	  
	  dif_t = time(0) - init_time;
	  add_money = level * 1000 + 10 * (500 - (int)dif_t);
	  sprintf(buf, " �z! %s �A��F %d �� �}�� �n�F�` ^O^"
	                , cuser.userid, dif_t);
	  vmsg(buf);
	  sprintf(buf, " ��o���� %d ��", add_money);
	  vmsg(buf);
	  addmoney(add_money);
	  return;	  
	}
	break;

      case KEY_PGDN:
        vmsg("Hey!");
        unknown_world();
        return 0;

      case ' ':

	/* ��Цb�����|�����B�w�g½�} */   

        if (rmax[cy] < cx && cx <= max[cy])	/* �ŤU */  
	{
	  move(star_x, star_y);
	  outc(' ');
	  star_c = down_stack[cy][cx];
	  star_x = cx + 2;
	  star_y = cy * 11;
	  move(star_x, star_y);
	  outc('*');
	  break;
	}
	else if (cx != max[cy] + 1)	 /* Xatier 100112. �o�Oԣ... */   
	  break;				/* �K�W */

	if ((max[cy] && (cal_num(down_stack[cy][max[cy]]) == cal_num(star_c) + 1) && 
	  (cal_kind(down_stack[cy][max[cy]]) + cal_kind(star_c)) % 2) ||
	  (max[cy] == 0 && cal_num(star_c) == 13))
	{
	  if (star_x == 1)		/* �q�W���K�U�Ӫ� */
	  {
	    max[cy]++;
	    max[8]--;
	    star_x = 2;
	    left--;
	    for (i = left + 1; i <= max[8]; i++)
	      left_stack[i] = left_stack[i + 1];
	    down_stack[cy][max[cy]] = star_c;
	    draw(max[cy] + 2, ycol(cy), star_c);
	    move(1, 19);
	    outc(' ');
	    draw(1, 11, left ? left_stack[left] : -1);
	  }
	  else if (star_x > 2)		/* �b�U���K�ӶK�h�� */	//ԣ��??
	  {
	    int tmp;;
	    j = star_y / 11;
	    tmp = max[j];	    
	    for (i = star_x - 2; i <= tmp; i++)
	    {
	      max[cy]++;
	      max[j]--;
	      down_stack[cy][max[cy]] = down_stack[j][i];
	      draw(max[cy] + 2, ycol(cy), down_stack[cy][max[cy]]);
	      draw(i + 2, ycol(j), -1);
	    }
	    move(star_x, star_y);
	    outc(' ');
	    star_x = 2;
	  }
	}
	break;
      }

    }
    else /* side == SIDE_UP */	/* ��Цb�W�� */
    {
      
      /* draw(1, 11, left ? left_stack[left] : -1); */
      if (left)
      {
        draw(1, 11, left_stack[left]);
      }
      else
      {
        draw(1, 11, -1);
      }

      key = vkey();
      key_rec[ptr] = key;
      ptr++;
      ptr %= 30000;

      switch (key) 
      {
      case 'q':		/* quit the game */

	vmsg(MSG_QUITGAME);
	return;

      case 'r':		/* replay the game */
      
	goto game_start;
      
      case 'p':
        /* stop */
	t1 = time(0);
	vmsg("�C���Ȱ�  �����N���~��...");
	/* go */
	t2 = time(0);
	init_time += (t2 - t1);
	t2 = t1 = init_time;	/* ��_�쪬 */
	break;

      case '\n':				/* ���P��k�W�� */
	j = left_stack[left];
	if (cal_num(j) == up_stack[cal_kind(j)] + 1)
	{
	  up_stack[cal_kind(j)]++;
	  max[8]--;
	  left--;
	  draw(1, cal_kind(j) * 10 + 40, j);

	  for (i = left + 1; i <= max[8]; i++)
	    left_stack[i] = left_stack[i + 1];

	  draw(1, 11, left ? left_stack[left] : -1);

	  if (star_x == 1)	/* �p�G���O���N�M�� */
	  {
	    star_x = 2;
	    move(1, 19);
	    outc(' ');
	  }
	  /* �}������: �k�W���|�ӱ��|�������O 13 */
	  if (up_stack[0] & up_stack[1] & up_stack[2] & up_stack[3] == 13)
	  {
	    char buf[80];
	    
	    dif_t = time(0) - init_time;
	    add_money = level * 1000 + 10 * (500 - (int)dif_t);
	    sprintf(buf, " �z! %s �A��F %d �� �}�� �n�F�` ^O^"
                          , cuser.userid, dif_t);
	    vmsg(buf);
	    sprintf(buf, " ��o���� %d ��", add_money);
            vmsg(buf);
	    addmoney(add_money);
            return;
	  }
	}
	break;

      case KEY_DOWN:		 /* ��в��U�h */
	side = SIDE_DOWN;
	cx = 1;
	move_cur(1, 9, cx + 2, ycol(cy) - 2);
	break; 
      
      case KEY_UP:
	if (left == max[8])
	  left = 0;
	else
	  left += level;	/* �@���o level �i�P */
	if (left > max[8])
	  left = max[8];

	if (star_x == 1)
	{
	  star_x = 2;
	  move(1, 19);
	  outc(' ');
	}

	draw(1, 1, left == max[8] ? -1 : 0);
	break;
      case ':':

        easter_egg();
	break;

      case ' ':
	if (left > 0)
	{
	  move(star_x, star_y);
	  outc(' ');
	  star_c = left_stack[left];
	  star_x = 1;
	  star_y = 19;
	  move(1, 19);
	  outc('*');
	}
	break;
      }					/* end of switch */
    }				/* end of side_down */
  }			/* end of for(;;) */
}		/* end of main */
#endif	/* HAVE_GAME */
