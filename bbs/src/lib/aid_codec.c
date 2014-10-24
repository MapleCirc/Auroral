#include<time.h>

#if 0
�b2106�~�j��2����3��ɡAtime(0)�o�X���Ʀr�����Q���i���|�W�L8��A

���ɳo�Өt�αN�|�X�{����Y2k�����~�C�ڱN��٬�Y2.1k���D�C

�ҥH���ӮɭԨ��{��(�����O�L�N�q��)�аO�o���X��ۮe�½s�X�W�檺�s�t��



�w�� telnet BBS system �ë����I�]���^

				By ���b20�P21�@��������ɪ������  2009/11/28
#endif

/* ���ĥΪ���
    2  0  1	 0 -1  1
    1  1  1	-1  0  1
    2  1  1	 1  2 -2	*/

/* encode matrix       decode matrix
    1  0  1  0          1  0 -1  0
    1  1  0  1         -1  2  1 -1
    0  0  1  0          0  0  1  0
    1  1  0  2          0 -1  0  1

          matirx[row][column] */
static const char encode_matrix[4][4] = {
  {1, 0, 1, 0}, {1, 1, 0, 1}, {0, 0, 1, 0}, {1, 1, 0, 2}
};

static const char decode_matrix[4][4] = {
  {1, 0, -1, 0}, {-1, 2, 1, -1}, {0, 0, 1, 0}, {0, -1, 0, 1}
};

#if 0
/* davy.130601: �O�ּg�X�o�򸣩⪺�x�}���k��... */
static void
matrix_mul(C, A, B)	/* C = A*B */
  char C[][2];		/* dst[4][2] */
  const char A[][4];	/* XXcode_matrix[4][4] */
  const char B[][2];	/* src[4][2] */
{
  int i, j, k;

  for(i = 0; i < 4; i++)
  {
    for(j = 0; j < 4; j++)
    {
      C[i][j] = 0;
      for(k = 0; k < 4; k++)
      {
	C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
}
#endif

/* davy.130601: �o�~�O���T���a....? */
static void
matrix_mul(C, A, B)	/* C = A*B */
  char C[][2];		/* dst[4][2] */
  const char A[][4];	/* XXcode_matrix[4][4] */
  const char B[][2];	/* src[4][2] */
{
  int i, j, k;

  for(i = 0; i < 4; i++)
  {
    for(j = 0; j < 2; j++)
    {
      C[i][j] = 0;
      for(k = 0; k < 4; k++)
      {
	C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
}

char *
aid_encode(chrono, aid)		/* �N chrono �ন�T�w8�X�� AID */
  time_t chrono;
  char *aid;
{
  char src[4][2], dst[4][2];
  int i, a;

  for(i = 7; i >= 0; i--)
  {  /* [i % 4]  [i / 4]    chrono % 16 */
    src[i & 0x03][i >> 2] = chrono & 0xf;
    chrono >>= 4;
 /* chrono /= 16 */
  }

  matrix_mul(dst, encode_matrix, src);

  for(i = 0; i < 8; i++)
  {      /* [i / 2] [i % 2]  */
    a = dst[i >> 1][i & 0x01];

    if(a < 1)
      aid[i] = '0';		/* '0' */
    else if(a < 26)
      aid[i] = a - 1 + 'A';	/* 'A' ~ 'X' */
    else if(a < 35)
      aid[i] = a - 26 + '1';	/* '1' ~ '9' */
    else
      aid[i] = a - 35 + 'a';	/* 'a' ~ 'z' */

  }
  aid[i] = '\0';

  return aid;
}


time_t
aid_decode(aid)		/* �N AID �Ѧ^ chrono */
  char *aid;
{
  char src[4][2], dst[4][2], c;
  time_t chrono;
  int i;

  if(*aid == '#')	/* �׶}�H'#'�_�Y��AID�r�� */
    aid++;

  for(i = 0; i < 8; i++)
  {
    c = *aid++;

    if(isupper(c) && c != 'Z')		/* 'A' ~ 'X' */
      src[i >> 1][i & 0x01] = c - 'A' + 1;
    else if(isdigit(c))			/* '0' and '1' ~ '9' */
      src[i >> 1][i & 0x01] = (c == '0')?  0 : c - '1' + 26;
    else if(islower(c))			/* 'a' ~ 'z' */
      src[i >> 1][i & 0x01] = c - 'a' + 35;
    else
      return -1;	/* invalid AID format, just return -1. */
  }

  matrix_mul(dst, decode_matrix, src);

  chrono = 0;
  for(i = 0; i < 8; i++)
  {
    chrono <<= 4;
    chrono |= dst[i & 0x03][i >> 2];
  }

  return chrono;
}



#if 0		//��������AID�s�X�t�Υ�
#include<stdio.h>

int main(){
  time_t stamp;
  char buf[32];

  printf("time stamp > ");
  scanf("%d", &stamp);

  printf("stamp  : %d\n", stamp);
  printf("AID    : #%s\n", aid_encode(stamp, buf));
  printf("inverse: %d\n", aid_decode(buf));
}
#endif


