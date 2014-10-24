#include<time.h>

#if 0
在2106年大約2月還3月時，time(0)得出的數字換成十六進位後會超過8位，

屆時這個系統將會出現類似Y2k的錯誤。我將其稱為Y2.1k問題。

所以那個時候來臨時(提早是無意義的)請記得做出能相容舊編碼規格的新系統



預祝 telnet BBS system 永恆不朽！（笑）

				By 活在20與21世紀交替之時的先行者  2009/11/28
#endif

/* 未採用版本
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
/* davy.130601: 是誰寫出這麼腦抽的矩陣乘法的... */
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

/* davy.130601: 這才是正確的吧....? */
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
aid_encode(chrono, aid)		/* 將 chrono 轉成固定8碼的 AID */
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
aid_decode(aid)		/* 將 AID 解回 chrono */
  char *aid;
{
  char src[4][2], dst[4][2], c;
  time_t chrono;
  int i;

  if(*aid == '#')	/* 避開以'#'起頭的AID字串 */
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



#if 0		//此為測試AID編碼系統用
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


