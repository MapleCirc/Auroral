/*-------------------------------------------------------*/
/* gray2.c        ( Tcfsh CIRC 27th )                    */
/*-------------------------------------------------------*/
/* target : �¥մѹC�� ��}                              */
/* create : 10/12/11                  �Ǵ��˼� 46 ��     */
/* update :   /  /                                       */
/* author : xatier @tcirc.org                            */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME


#define B 4
#define W 2
#define N 0

/* �¤l=4  �դl=2  �Ů�=0 */

/* - structure ------------------------------------------ */

typedef struct {
  int x;
  int y;
  int state;
} chess;                /* �Ѥl */

typedef struct {
  chess b[9][9];        /* 8x8 �ѽL , �U�� 0 ���� */
} chessboard;           /* �ѽL */

typedef struct {
  int x;
  int y;
} CUR;                  /* ��� */

/* - global vars----------------------------------------- */

chessboard cur_board;   /* �{�b���ѽL       */
int b_occupation;       /* �¤l�����Ѥl��   */
int w_occupation;       /* �դl�����Ѥl��   */
CUR cur;                /* ���             */
int black_turn;         /* ����¤l(���a)�U */
int comp_pass;          /* �q�� Pass        */


                        /* �ѽL��m�� (�H�u���z��) */
double init_map_val [9][9] = 
{   /* ��Ȭ����I�P �ѽL���� ���Z���}����� ; �U�йs����*/
#if 0
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 2.2248, 2.0739, 1.9514, 1.8803, 1.8803, 1.9514, 2.0739, 2.2248,
    0.0000, 2.0739, 1.8803, 1.7075, 1.5967, 1.5967, 1.7075, 1.8803, 2.0739,
    0.0000, 1.9514, 1.7075, 1.4565, 1.2574, 1.2574, 1.4565, 1.7075, 1.9514,
    0.0000, 1.8803, 1.5967, 1.2574, 0.8409, 0.8409, 1.2574, 1.5967, 1.8803,
    0.0000, 1.8803, 1.5967, 1.2574, 0.8409, 0.8409, 1.2574, 1.5967, 1.8803,
    0.0000, 1.9514, 1.7075, 1.4565, 1.2574, 1.2574, 1.4565, 1.7075, 1.9514,
    0.0000, 2.0739, 1.8803, 1.7075, 1.5967, 1.5967, 1.7075, 1.8803, 2.0739,
    0.0000, 2.2248, 2.0739, 1.9514, 1.8803, 1.8803, 1.9514, 2.0739, 2.2248
#endif

//   magic number XD
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 
0.0000, 4.9497, 3.2127, 2.9144, 2.7464, 2.7464, 2.9144, 3.2127, 4.9497, 
0.0000, 3.2127, 1.8803, 1.7075, 1.5967, 1.5967, 1.7075, 1.8803, 3.2127, 
0.0000, 2.9144, 1.7075, 1.4565, 1.2574, 1.2574, 1.4565, 1.7075, 2.9144, 
0.0000, 2.7464, 1.5967, 1.2574, 0.8409, 0.8409, 1.2574, 1.5967, 2.7464, 
0.0000, 2.7464, 1.5967, 1.2574, 0.8409, 0.8409, 1.2574, 1.5967, 2.7464, 
0.0000, 2.9144, 1.7075, 1.4565, 1.2574, 1.2574, 1.4565, 1.7075, 2.9144, 
0.0000, 3.2127, 1.8803, 1.7075, 1.5967, 1.5967, 1.7075, 1.8803, 3.2127, 
0.0000, 4.9497, 3.2127, 2.9144, 2.7464, 2.7464, 2.9144, 3.2127, 4.9497, 

};

int dir[8][2] = {  {-1, -1}, {0, -1}, { 1, -1},
                   {-1,  0},          { 1,  0},
                   {-1,  1}, {0,  1}, { 1,  1}
                };


double map_val[9][9];

/* - function declaration ------------------------------- */

void init (void);                         /* ��l��       */
void draw_chess (chess *c);               /* �e�Ѥl       */
void draw_cur_board (void);               /* ��ø�ѽL     */
int  search (chess *c, int dx, int dy);   /* �V�~�j��     */
int  do_play (void);                      /* �i�H�U�o�B   */
void do_eat (chess *c, int dx, int dy);   /* �Y���Ѥl     */
int  check (void);                        /* �ˬd�C������ */
void computer_play(void);                 /* �H�u���z     */
void map_val_update(void);                /* ��s�ѽL�� */

/* - function definition -------------------------------- */


void
init (void)
{

  int i, j;
  
  move(0, 0);                             /* �M�ù�       */
  clrtobot();

  move(0, 0);

  outs(Gray_INIT);                        /* �C���e��ø�s */

  for (i = 1; i <= 8; i++)                /* �ѽL�M��     */
    for (j = 1; j <= 8; j++)
    {
      cur_board.b[i][j].x = i;
      cur_board.b[i][j].y = j;
      cur_board.b[i][j].state = N;
      map_val[i][j] = init_map_val[i][j];
    }

  /* �@�H��l */
  cur_board.b[4][4].state = cur_board.b[5][5].state = B;
  cur_board.b[4][5].state = cur_board.b[5][4].state = W;

  cur.x = cur.y = 4;                      /* ����\����   */

  black_turn = 1;                         /* �¤l���U     */

  check();                                /* ��s������� */

  draw_cur_board();                       /* �e�X�ѽL     */
  move(11 + (cur.y-1), 32 + (cur.x-1) * 2);

}


void 
draw_chess (chess *c)
{
  move(11 + (c->y-1), 31 + (c->x-1) * 2);
  switch (c->state) {
    case N:
      outs("\033[33m��\033[m");
      break;
    case W:
      outs("��");
      break;
    case B:
      outs("��");
      break;
  }
}


void
draw_cur_board (void)
{
  int i, j;

  for (i = 1; i <= 8; i++)                /* �v�@�L�X�Ѥl */
    for (j = 1; j <= 8; j++)
      draw_chess(&cur_board.b[i][j]);
  
  move(12, 62);                           /* ��X������� */
  prints("%2d", b_occupation);
  move(13, 62);
  prints("%2d", w_occupation);

  map_val_update();
}


int
search (chess *c, int dx, int dy)
{
  int x = c->x, y = c->y, count = 0;
  int target = (black_turn ? B : W), another = (black_turn ? W : B);
                                          /* �I �I!       */
  x += dx;                                /* ����         */
  y += dy;
                                          /* �ѽL�d�򤺳� */
  while ((1 <= x && x <= 8) && (1 <= y && y <= 8))
  {
    if (cur_board.b[x][y].state == another)
    {
      count++;                            /* �ڥi�H�Y�o�� */
      x += dx;                            /* �~�򩹫e��   */
      y += dy;
    }
    else if (cur_board.b[x][y].state == target)
        return count;                     /* �j�M�פF     */
    else 
      break;
  }
  return 0;
}


int
do_play (void)
{
  int max = 0, i;

  if (cur_board.b[cur.x][cur.y].state != N)
    return 0;                             /* ����U�o��   */
                                          /* �V�~�j�M     */
  for (i = 0; i < 8; i++)
    max += search(&cur_board.b[cur.x][cur.y], dir[i][0], dir[i][1]);

  return max;
}


void
do_eat (chess *c, int dx, int dy)
{
  int x = c->x, y = c->y, count = search(c, dx, dy);
  int target = (black_turn ? B : W), another = (black_turn ? W : B);

  if ( count == 0 )                       /* �o�椣��Y�� */
    return;

  while (count--)                         /* �@���@���Y�� */
  {
    x += dx;
    y += dy;

    if (cur_board.b[x][y].state == another)
      cur_board.b[x][y].state = target;
    else
      break;
    
  }
}

void
computer_play (void)
{
  int i, j, k, d[8], sum;
  double val, max;
  CUR best;

  best.x = 1;
  best.y = 1;
  max = val = 0.0;
  comp_pass = 0;

  for (i = 1; i <= 8; i++)                /* �v�@�M���ѽL */
  {
    for (j = 1; j <= 8; j++)
    {

      cur.x = i;                          /* ���ʴ��     */
      cur.y = j;
      sum = 0;

      if (cur_board.b[cur.x][cur.y].state != N)
        continue;
                                          /* ��X��Y�X�� */
      for (k = 0; k < 8; k++)
        sum += search(&cur_board.b[cur.x][cur.y], dir[k][0], dir[k][1]);
      
      if (sum == 0)                       /* �S�Y��b��   */
        continue;

      val = sum * map_val[i][j];          /* ���W��m�v�� */

      if (val > max)                      /* ��̤j��     */
      {
        max = val;
        best = cur;
      }
    }
  }

  cur = best;                             

  if (do_play())                          /* �q���i�H�U */
  {
    /* �U�o�B */
    cur_board.b[cur.x][cur.y].state = black_turn ? B : W;
    /* �|���K��h�Y */
    for (i = 0; i < 8; i++)
      do_eat(&cur_board.b[cur.x][cur.y], dir[i][0], dir[i][1]);
  }
  else
  {
    vmsg(" �q���L�l�i�U ");
    comp_pass = 1;
  }

  black_turn = !black_turn;                         /* ���a���A�o   */

}


int                            /* 0: �٥����ӭt  1: ���a��   2:�q���� */
check (void)
{
  int i, j;
  int b = 0, w = 0;

  for (i = 1; i <= 8; i++)                /* �p�����Ѽ� */
    for (j = 1; j <= 8; j++)
      (cur_board.b[i][j].state == B ? b++: cur_board.b[i][j].state == W ? w++: 0);

  b_occupation = b;
  w_occupation = w;

  draw_cur_board();			/* �γo�̨ӧ�s */

  return (w == 0 && b > 0) ? 1 : (b == 0 && w > 0) ? 2 : ((b+w) == 64) ? (b > w ? 1: 2): 0;
}


#if 0
//     �p�G�ݤ����W�� code ���ܳo�O����
//     for ()
//       for ()
      switch (cur_board.b[i][j].state)
      {
        case B:
          b++;
          break;
        case W:
          w++;
          break;
      }
  
//	return ����
if (w == 0 && b > 0)                    /* ��Ĺ�P�w     */
    return 1;
  else if (b == 0 && w > 0)
    return 2;
  else if ((b+w) == 64)
    return (b > w ? 1 : 2);
  else
    return 0;
#endif


void
map_val_update(void)
{
  int x, y, nx, ny, i;
  double new[9][9];
  for (x = 1; x <= 8; x++)
  {
    for (y = 1; y <= 8; y++)
    {
      new[x][y] = map_val[x][y];
      for (i = 0; i < 8; i++)
      {
        nx = x + dir[i][0];
        ny = y + dir[i][1];
        if (0 < nx && nx <= 8 && 0 < ny && ny <= 8)
          new[x][y] *= (cur_board.b[nx][ny].state == W ? 1.2: 0.8);
      }
    }
  }

  for (x = 1; x <= 8; x++)
    for (y = 1; y <= 8; y++)
      map_val[x][y] = new[x][y];

}


int 
main_gray2 (void)
{
  
  char key, msg[20];
  int pass, done, i, j;
  CUR cur_temp;


game_start:  
  init();

  for (;;)
  {
    pass = done = 0;


                                          /* �ˬd���a�i�_�U�� */
    cur_temp = cur;
    for (i = 1; i <= 8; i++)
    {
      for (j = 1; j <= 8; j++)
      {
        cur.x = i;
        cur.y = j;
        if (!do_play())
          pass++;
      }
    }
    cur = cur_temp;


    if (pass == 64)
    {
      if (comp_pass)
      {
        vmsg("����L�l�i�U �M��!");
        goto game_over;
      }
      else
      {
        vmsg(" ʨ... �L�l�i�U ");
        black_turn = 0;
        done = 1;
      }

      goto comp;                          /* ����q���U��   */
    }




    switch (key = vkey())
    {

      /* - ���}�P���s --------------------------------------*/
      case 'q':                           /* ���}�C��       */
        vmsg("�T�T�o~~");
        goto game_over;

      case 'r':
        vmsg("���s�}�l!");
        goto game_start;                  /* ����           */

      case 'A':
        cur.x = cur.y = 1;
        break;

      /* - ��V��B�z ------------------------------------- */
      case KEY_UP:
        cur.y--;
        if (cur.y < 1)
          cur.y = 8;
        break;

      case KEY_DOWN:
        cur.y++;
        if (cur.y > 8)
          cur.y = 1;
        break;

      case KEY_LEFT:
        cur.x--;
        if (cur.x < 1)
          cur.x = 8;
        break;

      case KEY_RIGHT:
        cur.x++;
        if (cur.x > 8)
          cur.x = 1;
        break;


      /* - �U�o�� ----------------------------------------- */
      case ' ': case '\n':
        if (do_play())                    /* �i�H�U�o�Ӧ�m */
        {

          /* �U�o�B */
          cur_board.b[cur.x][cur.y].state = black_turn ? B : W;
          /* �|���K��h�Y */
          for (i = 0; i < 8; i++)
            do_eat(&cur_board.b[cur.x][cur.y], dir[i][0], dir[i][1]);
          
          black_turn = 0;
          done = 1;

        }
        break;

    }                                     /* end of switch  */


    draw_cur_board();

    switch (check())
    {
      case 1:
        sprintf(msg, "���aĹ��[%d : %d]", b_occupation, w_occupation);
        vmsg(msg);
        goto game_over;
        break;
      case 2:
        sprintf(msg, "���a���  [%d : %d]", b_occupation, w_occupation);
        vmsg(msg);
        goto game_over;
        break;
    }

    draw_cur_board();

comp:

    if (done)
      computer_play();


    switch (check())
    {
      case 1:
        sprintf(msg, "���aĹ�X  [%d : %d]", b_occupation, w_occupation);
        vmsg(msg);
        goto game_over;
        break;
      case 2:
        sprintf(msg, "���a���  [%d : %d]", b_occupation, w_occupation);
        vmsg(msg);
        goto game_over;
        break;
    }

    draw_cur_board();

    move(11 + (cur.y-1), 32 + (cur.x-1) * 2);

  } 


game_over:
  move(b_lines, 0);
  outs("�����N�����}...");
  vkey();
  return 0;
}

#endif
