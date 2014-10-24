/*-------------------------------------------------------*/
/* tetravex.c                                            */
/*-------------------------------------------------------*/
/* target : ������ϹC��                                 */
/* create : 10/03/12                                     */
/* author : xatier @tcirc.org                            */
/* Ubuntu �W���g��C�� tetravex                          */
/*-------------------------------------------------------*/



#include "bbs.h"
#ifdef HAVE_GAME


#define _1_ "\033[m\033[30;47m��\033[m"		/* �Ʀr */
#define _2_ "\033[1;32;41m��\033[m"
#define _3_ "\033[1;32;42m��\033[m"
#define _4_ "\033[1;36;44m��\033[m"
#define _5_ "\033[1;33;45m��\033[m"
#define _6_ "\033[1;36;46m��\033[m"
#define _7_ "\033[1;33;44m��\033[m"
#define _8_ "\033[1;32;45m��\033[m"
#define _9_ "\033[1;33;46m��\033[m"
#define _0_ "\033[1;44m��\033[m"

#define _B_ "\033[m\033[1;30m�i\033[m"		/* ��� */
#define _C_ "\033[m\033[1;33m��\033[m"		/* ��� */
#define _F_ "\033[m\033[1;36m��\033[m"		/* �X�m */





enum
{
  SIDE_L = 1,                        /* �b���� */
  SIDE_R = 0                         /* �b�k�� */
};

int SIDE;

int cheat[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};	/* �@���Ϊ��m�J */

int i, j;		/* �U�� i, j */

time_t init_time;       /* �C���}�l���ɶ� */
time_t t1, t2;          /* �ɶ��Ȱ��Ϊ� */
time_t dif_t;           /* �}�l�ɶ��P�����ɶ����t���A�Y��F�h�֮ɶ��}�� */
int bonus;		/* ���� */
int light;		/* �O�w */


typedef struct          /* ������c�� */
{
  int up;               /* �W */
  int down;             /* �U */
  int left;             /* �� */
  int right;            /* �k */
  int x;                /* X �y�� */
  int y;                /* Y �y�� */
  int no;               /* �s�� */
} Block;

#if 0
  �j�����o��:

        --------
        |  �W  |
        |��  �k|
        |  �U  |
        --------

  �䤤
    no = x + 3*y
    x = no % 3
    y = no / 3
#endif


Block user[9], puzzle[9], ans[9], copy_user[9];
/* �k��ݫ��� ������@�b�� �ѵ� �ƻs�ϥΪ̪����� */
Block null;	/* �Ū���� */

typedef struct			/* ��е��c�� */
{
  int x;			/* X �y�� */
  int y;			/* Y �y�� */
  int flag;			/* �O�_�Q�аO */
  int catch;			/* ����@�� */
  int side;			/* ����@�� */
} CUR;

CUR cur;			/* �ڬO��� */
int cur_map[3][6];		/* ��Юy�� */

#if 0
  ��Ъ��y�Шt�O3*6�������

	  0 1 2 3 4 5
	  __________
	0|          |
	1|          |
	2|__________|

 6, 12   6, 20   6, 28 		�k�� X  +36
10, 12  10, 20  10, 28
14, 12  14, 20  14, 28


  val:
    1 = ����l
    2 = �X�m
    0 = �Ů�

#endif


#if 0
   �a�ϡB�k��ݫ����B�ѵ�

   �E�c����o��

        X 0   1   2
        -------------
     Y  |   |   |   |  5, 10   5, 18   5, 26   �k�� X  +36
     0  | 0 | 1 | 2 |  6, 10   6, 18   6, 26
        |   |   |   |  7, 10   7, 18   7, 26
        |------------
        |   |   |   |  9, 10   9, 18   9, 26
     1  | 3 | 4 | 5 | 10, 10  10, 18  10, 26
        |   |   |   | 11, 10  11, 18  11, 26
        |------------
        |   |   |   | 13, 10  13, 18  13, 26
     2  | 6 | 7 | 8 | 14, 10  14, 18  14, 26
        |   |   |   | 15, 10  15, 18  15, 26
        -------------


#endif

void init (void);
void draw_all (void);
void out_foot (void);
void draw_num (int);
void swap_block (Block *, Block *);
void draw_block (int , Block);
void draw_cur (int);
void x_cur (void);
int check_ans (void);
void move_block (void);



void
init (void)
{
  int N = 50;		/* N is for swap user[] */

  /* init the NULL block */
  null.up = -1;
  null.down = -1;
  null.left = -1;
  null.right = -1;
  null.x = -1;
  null.y = -1;
  null.no = -1;

  int fill[12];         /* �񪺼Ʀr */

  for (i = 0; i < 9; i++)
  {
    puzzle[i] = null;		/* �M��puzzle[] */

    /* �]����b�|�䪺�Ʀr�i�H��0~9�A�ҥH�� -1�Ӫ�l�� */
    ans[i].up = ans[i].down = ans[i].left = ans[i].right = -1;

    /* ��l�� puzzle[] �M user[] ���s���P�y�� */
    puzzle[i].no = ans[i].no = i;
    puzzle[i].x  = ans[i].x  = ans[i].no % 3;
    puzzle[i].y  = ans[i].y  = ans[i].no / 3;

    cheat[i] = i + 1;		/* �@�����m�J */
  }

  
  /* init the cur_map[][] */
  for (i = 0; i < 3; i++)
    for (j = 0; j < 6; j++)
      cur_map[i][j] = 0;


  for (i = 0; i < 12; i++)
    fill[i] = rand() % 10;      /* ����0~9 ��12 �Ӷü� */
  

  /* ��l�Ƹѵ��A�۾F���I�ƭn�@�� */
  ans[0].right = ans[1].left = fill[0];
  ans[1].right = ans[2].left = fill[1];

  ans[3].right = ans[4].left = fill[2];
  ans[4].right = ans[5].left = fill[3];

  ans[6].right = ans[7].left = fill[4];
  ans[7].right = ans[8].left = fill[5];

  ans[0].down = ans[3].up = fill[6];
  ans[1].down = ans[4].up = fill[7];
  ans[2].down = ans[5].up = fill[8];

  ans[3].down = ans[6].up = fill[9];
  ans[4].down = ans[7].up = fill[10];
  ans[5].down = ans[8].up = fill[11];


  /* �ɺ���L��l */
  for (i = 0; i < 9; i++)
  {
    if (ans[i].up == -1)
      ans[i].up = rand() % 10;

    if (ans[i].down == -1)
      ans[i].down = rand() % 10;

    if (ans[i].right == -1)
      ans[i].right = rand() % 10;

    if (ans[i].left == -1)
      ans[i].left = rand() % 10;
  }

  /* �N�ѵ��ƻs��user[] */
  for (i = 0; i < 9; i++)
  {
    user[i] = ans[i];
  }
  

  /* �H���洫user[] */
  while (N--)
  {
    int t;
    i = rand() % 9;
    j = ((2 * i) % 9);
    swap_block(&user[i], &user[j]);

    t = cheat[i];
    cheat[i] = cheat[j];
    cheat[j] = t;
  }


  /* �e�X�خ� */
  move(4, 0);
  outs(TetraVex_BORDER);

  for (i = 0; i < 9; i++)
  {
    draw_block(0, user[i]);	/* �e�Xuser[] */
    draw_block(1, puzzle[i]);
    
    /* ���K�]�w cur_map[][]�̪��� (�k���) */
    cur_map[i / 3][i % 3 + 3] = 1;
    cur_map[i / 3][i % 3] = 0;

    /* ��user[] �ƻs�� copy_user[] */
    copy_user[i] = user[i];

  }

  /* init our cursor */
  cur.x = 3;
  cur.y = 0;
  cur.flag = 0;

  light = 0;			/* turn off the light */

  SIDE = SIDE_R;		/* we are in the right side ^.< */
  t2 = t1 = init_time = time(0);                /* �}�l�O���ɶ� */
}

void
draw_all (void)			/* ø�X�Ҧ�����l */
{
  for (i = 0; i < 9; i++)
  {
    draw_block(0, user[i]);
    draw_block(1, puzzle[i]);
  }
}



void
out_foot (void)                    /* �������ܦr�� */
{
  move(b_lines - 1, 0);
  outs(TetraVex_FOOT);
}



void
draw_num (n)			/* �e�X�Ʀr */
int n;
{
  switch (n)
  {
    case 1:
      outs(_1_);
      break;
    case 2:
      outs(_2_);
      break;
    case 3:
      outs(_3_);
      break;
    case 4:
      outs(_4_);
      break;
    case 5:
      outs(_5_);
      break;
    case 6:
      outs(_6_);
      break;
    case 7:
      outs(_7_);
      break;
    case 8:
      outs(_8_);
      break;
    case 9:
      outs(_9_);
      break;
    case 0:
      outs(_0_);
      break;
    case -1:
      outs("  ");
      break;
  }
}


void
swap_block (a, b)		/* �洫Block���Ʀr�A���O��m���洫 */
Block *a;
Block *b;
{
  int temp;
  Block Temp;

  Temp = *a;			/* ���������L�� */
  *a = *b;
  *b = Temp;

  temp = a->x;			/* ��L���A���^�� */
  a->x = b->x;
  b->x = temp;

  temp = a->y;
  a->y = b->y;
  b->y = temp;

  temp = a->no;
  a->no = b->no;
  b->no = temp;

}


void 
draw_block (side, B)		/* side = 1 ����Aside = 0 �k�� */
int side;
Block B;
{
  int px, py;			/* �e���� X Y �y�� */
  py = 5 + 4 * B.y;
  px = 10 + 8 * B.x;

  if (!side)			/* �b�k�䪺�� X �n�[36 */
    px += 36;

#if 0

�@�@���:

�W      �i���i
��	���i��
�U	�i���i

#endif

  /* �X�F�|�䳣�O�s�����p�ҥ~�B�z */
  if (B.up == B.down && B.down == B.left)
    if (B.left == B.right && B.right == 0)
      vmsg("����! �{���X�F���D�A�o��NULL����ƥ�A�Ь��}�o��xatier�B�z");


  move(py, px);			/* �W */
  outs("  ");
  draw_num(B.up);
  outs("  ");

  py++;				/* �� */
  move(py, px);
  draw_num(B.left);
  move(py, px+4);		/* �����ŵ� */
  draw_num(B.right);

  py++;				/* �U */
  move(py, px);
  outs("  ");
  draw_num(B.down);
  outs("  ");

}



void
draw_cur (kind)			/* ��ܴ�� kind: 0 = ���; 1 = �X�m */
int kind;
{
  /* ��ж]��F�_�Ǫ��a�� */
  if (cur.x > 5 || cur.x < 0)
    vmsg("����! �{���X�F���D�A��в���@�ɺ��Y���~�A�Ь��}�o�H��xatier�B�z");

  if (SIDE)
  {
    move(6 + 4 * cur.y, 12 + 8 * cur.x);
  }
  else
  {
    move(6 + 4 * cur.y, 12 + 36 + 8 * (cur.x - 3));
  }

  if (kind == 1)
    outs(_F_);
  else if (kind == 0)
    outs(_C_);

  move(b_lines - 1, 0);		/* ���в��� */
}



void
x_cur (void)
{
  int foo = 0;
  /* foo�ΨӰO���X�m���ƥءA�p�G�j��@��N�O�����D */

  for (i = 0; i < 3; i++)		/* ���� */
  {
    for (j = 0; j < 3; j++)
    {
      move(6 + 4 * i, 12 + 8 * j);
      if (!cur_map[i][j] || cur_map[i][j] == 1)
      {
        outs("  ");
      }
      else if (cur_map[i][j] == 2)	/* �o�欰�ɺX���A */
      {
        outs(_F_);
        foo++;
      }
    }
  }

  for (i = 0; i < 3; i++)		/* �k�� */
  {
    for (j = 3; j < 6; j++)
    {
      move(6 + 4 * i, 12 + 36 + 8 * (j-3));
      if (!cur_map[i][j] || cur_map[i][j] == 1)
      {
        outs("  ");
      }
      else if (cur_map[i][j] == 2)
      {
        outs(_F_);
        foo++;
      }
    }
  }
  
  /* �X�F���X�m���ҥ~�B�z */
  if (foo > 1)
    vmsg("����! �{���X�F���D�A�o�ͺX�m�ݼv�ƥ�A�Ь��}�o��xatier�B�z");


  /* �{�b��Ц�m */
  draw_cur(0);
}


int
check_ans (void)
{

  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
      if (cur_map[i][j] == 0)		/* �p�G���Ů�A�N�D���� */
        return 0;
    }
  }


  int ker = 1;		/* �P�_�O�_�U�ƭȳ��ۦP */
  for (i = 0; i < 9; i++)
  {
    /* �ϥ��N�O�����@�˪��N���O���� */
    if (puzzle[i].up    != ans[i].up)
      ker = 0;
    if (puzzle[i].down  != ans[i].down)
      ker = 0;
    if (puzzle[i].left  != ans[i].left)
      ker = 0;
    if (puzzle[i].right != ans[i].right)
      ker = 0;
  }

  if (!ker)
    return 0;

  if (						/* �h�����קK -> ��ʵw�F */
	  puzzle[0].right == puzzle[1].left &&
	  puzzle[1].right == puzzle[2].left &&

	  puzzle[3].right == puzzle[4].left &&
	  puzzle[4].right == puzzle[5].left &&

	  puzzle[6].right == puzzle[7].left &&
	  puzzle[7].right == puzzle[8].left &&

	  puzzle[0].down == puzzle[3].up &&
	  puzzle[1].down == puzzle[4].up &&
	  puzzle[2].down == puzzle[5].up &&

	  puzzle[3].down == puzzle[6].up &&
	  puzzle[4].down == puzzle[7].up &&
	  puzzle[5].down == puzzle[8].up 
  )
  {
    return 1;					/* �ݰ_�ӫܤT�p���g�k XD */
  }

}

void				/* ���}�ѵ�XD */
wow (void)
{
  int px, py;                   /* �e���� X Y �y�� */


  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
      py =  5 + 4 * j;
      px = 10 + 8 * i + 36;
      move(py, px);                 /* �W */
      outs("      ");
      move(py+1, px);
      outs("      ");
      move(py+2, px);
      outs("      ");
      usleep(200);
      draw_block(1, ans[j * 3 + i]);

    }
  }
}


void
move_block (void)
{
  
  if (cur.flag)			/* �w�ɺX */
  {
    if (!SIDE)		/* �k�� */
    {
      /* �o��O�Ū� */
      if (cur_map[cur.y][cur.x] == 0)
      {
        if (cur.side) 				/* �쪺�����b���� */
        {
          /* �洫 */
          swap_block(&puzzle[cur.catch], &user[3 * cur.y + (cur.x - 3)]);

          /* cur_map[][] ��s */
          cur_map[cur.y][cur.x] = 1;
          cur_map[cur.catch / 3][cur.catch % 3] = 0;

          /* ���X */
          cur.flag = 0;
        }
        else					/* �쪺�����b�k�� */
        {
          /* �洫 */
          swap_block(&user[cur.catch], &user[3 * cur.y + (cur.x - 3)]);

          /* cur_map[][] ��s */
          cur_map[cur.y][cur.x] = 1;
          cur_map[cur.catch / 3][cur.catch % 3 + 3] = 0;

          /* ���X */
          cur.flag = 0;
        }
      }
      else			/* �o�椣�O�Ū��A�b�o�̤ɺX */
      {
        /* ����b�P�@�� */
        if (! ( (cur.catch / 3 == cur.y) && 
              ( (cur.catch % 3 + 3 * !cur.side) == cur.x) ) )
        {
          /* cur_map[][] ��s */
          cur_map[cur.catch / 3][cur.catch % 3 + 3 * !cur.side] = 1;
          cur_map[cur.y][cur.x] = 2;

          /* �ɺX */
          cur.flag = 1;

          /* ���������� */
          cur.side = SIDE;
          cur.catch = 3 * cur.y + (cur.x - 3);
        }
      }
    }
    else			/* ���� */
    {
      /* �o��O�Ū� */
      if (cur_map[cur.y][cur.x] == 0)
      {
        if (cur.side)                           /* �쪺�����b���� */
        {
          /* �洫 */
          swap_block(&puzzle[cur.catch], &puzzle[3 * cur.y + cur.x]);

          /* cur_map[][] ��s */
          cur_map[cur.y][cur.x] = 1;
          cur_map[cur.catch / 3][cur.catch % 3] = 0;

          /* ���X */
          cur.flag = 0;
        }
        else                                    /* �쪺�����b�k�� */
        {
          /* �洫 */
          swap_block(&user[cur.catch], &puzzle[3 * cur.y + cur.x]);

          /* cur_map[][] ��s */
          cur_map[cur.y][cur.x] = 1;
          cur_map[cur.catch / 3][cur.catch % 3 + 3] = 0;

          /* ���X */
          cur.flag = 0;
        }
      }
      else                                      /* �o�椣�O�Ū��A���N�b�o�̤ɺX */
      {
        /* ����A�P�@��B�z */
        if (!( (cur.catch / 3 == cur.y) &&
               ((cur.catch % 3 + 3 * !cur.side) == cur.x) ) )
        {
          /* cur_map[][] ��s */
          cur_map[cur.catch / 3][cur.catch % 3 + 3 * !cur.side] = 1;
          cur_map[cur.y][cur.x] = 2;

          /* �ɺX */
          cur.flag = 1;

          /* ���������� */
          cur.side = 1;
          cur.catch = 3 * cur.y + cur.x;
        }
      }
    }
  }
  else		/* �٨S�ɺX */
  {
    /* �o��n���F���!! */
    if (cur_map[cur.y][cur.x] == 1)
    {
      /* �ɺX */
      cur.flag = 1;

      /* ��@�U������� */
      cur.side = SIDE;
      if (!SIDE)
      {
        cur.catch = 3 * cur.y + (cur.x - 3);
      }
      else
      {
        cur.catch = 3 * cur.y + cur.x;
      }

      /* cur_map[][] ��s */
      cur_map[cur.catch / 3][cur.catch % 3 + 3 * !cur.side] = 0;
      cur_map[cur.y][cur.x] = 2;
    }
  }
}




int
main_tetra (void)
{

  
  game_start:		/* �C�����s�}�l */

  move(0, 0);		/* �M�� */
  clrtobot();

  out_foot();		/* �}�}>////< */

  char key;		/* ���� */
  
  init();
  draw_all();
  move(b_lines - 1, 0);
  x_cur();

  
  while (1)
  {
    /*--�p--��--��-------------------------------------------*/

    move(b_lines - 2, 0);
    prints("\033[1;32;40m�L�F %d ��... \033[m", (time(0) - init_time));

    /*-------------------------------------------------------*/

    key = vkey();
    switch (key)
    {

      /* ���}�P���s */
      case 'q':                 /* quit the game */
        vmsg("�T�T�o~~");
        return 0;

      case 'r':
        vmsg("���s�}�l!");
        goto game_start;                /* restart the game */


/* - �� �V �� �B �z --------------------------------------------- */
      case KEY_UP:
        cur.y--;
        if (cur.y < 0)
          cur.y = 2;
        draw_cur(0);
        break;

      case KEY_DOWN:
        cur.y++;
        if (cur.y > 2)
          cur.y = 0;
        draw_cur(0);
        break;

      case KEY_LEFT:
        cur.x--;
        if (cur.x < 0)
          cur.x = 5;
        if (cur.x > 2)
          SIDE = SIDE_R;
        else
          SIDE = SIDE_L;
        draw_cur(0);
        break;

      case KEY_RIGHT:
        cur.x++;
        if (cur.x > 5)
          cur.x = 0;
        if (cur.x > 2)
          SIDE = SIDE_R;
        else
          SIDE = SIDE_L;
        draw_cur(0);
        break;
/* -------------------------------------------------------------- */


      /* ���� */
      case ' ':
        move_block();
        break;
      
      /* �@�����m�J */
      case '\\':
        light = !light;		/* �} / ���O */

        if (light)		/* �}�O */
        {
	  move(b_lines - 5 , 10);
          outs("�ǻ������ѵ�(�ѯ�) --> ");
          move(b_lines - 7, 35);
          outs("+-----+");
          for (i = 0; i < 3; i++)
          {
            move(b_lines - (6-i), 35);
            outs("|");
            for (j = 0; j < 3; j++)
            {
              prints("%d|", cheat[i*3 + j]);
            }
          }
          move(b_lines - 3, 35);
          outs("+-----+");
        }
        else 			/* ���O */
        {
          move(b_lines - 7, 35);
          outs("        ");
          move(b_lines - 6, 10);
          outs("\n\n\n\n");
        } 

        break;

      /* �Ȱ� */
      case 'p':
        /* stop */
        t1 = time(0);
        vmsg("�C���Ȱ�  �����N���~��...");
        /* go */
        t2 = time(0);
        init_time += (t2 - t1);
        t2 = t1 = init_time;    /* ��_�쪬 */
        break;

      case 'w':
        wow();			/* �i�D�ϥΪ̸ѵ� */
        char buff[5];
        vget(b_lines - 2, 0, "�n�~���?y/[N]", buff, 2, DOECHO);
 
        if (buff[0] == 'y' || buff[0] == 'Y')
          goto game_start;
        else 
          goto game_over;

        break;

      case 'h':
        t1 = time(0);		/* �n��ɶ��Ȱ� */
        more("etc/game/tetravex.hlp", MFST_NULL);
        move(0, 0);
        clrtobot();
        /* �e�X�خ� */
        move(4, 0);
        outs(TetraVex_BORDER);

        out_foot();           /* �}�} >////< */

        /* �ɶ��~��] */
        t2 = time(0);
        init_time += (t2 - t1);
        t2 = t1 = init_time;    /* ��_�쪬 */

        break;
    }

    draw_all();
    move(b_lines - 1, 0);
    x_cur();

    if (check_ans())
    {

      char buf[80];

      dif_t = time(0) - init_time;
      
      bonus = (int)(50  + 0.5 * (80 - (int)dif_t));
      if (bonus < 0)
        bonus = 0;
      sprintf(buf, " Wow! %s Win the game in %d secs!!"
                    , cuser.userid, dif_t);
      vmsg(buf);
      sprintf(buf, " Bonus =  %d ", bonus);
      vmsg(buf);
      addmoney(bonus);

      goto game_over;
    }
  }

  game_over:
  return 0;
}

#endif

