/*-------------------------------------------------------*/
/* gray.c         ( Tcfsh CIRC 27th )                    */
/*-------------------------------------------------------*/
/* target : �¥մѹC�� (�s)                              */
/* create : 11/02/21                        �Ǵ���]     */
/* update :   /  /                                       */
/* author : xatier @tcirc.org                            */
/*-------------------------------------------------------*/



#include "bbs.h"
#include "gray.h"

#ifdef HAVE_GAME

/* - structure ------------------------------------------ */

/* - global vars----------------------------------------- */

/* - function declaration ------------------------------- */

/* - function definition -------------------------------- */

void
init (void)
{

  int i, j;

  move(0, 0);                             /* �M�ù�       */
  clrtobot();
  move(0, 0);

  outs(GRAY_WORDS);
  move(b_lines-1, 0);
  outs("this is an uncompleted game, press 'q' to EXIT");
}

int
main_gray (void)
{

  char key, msg[20];
  int pass, done, i, j;


game_start:
  init();

  
  for (;;)
  {
    switch (vkey())
    {
      case 'q':
        goto game_over;
    }
  }


game_over:
  move(b_lines, 0);
  outs("�����N�����}...");
  vkey();
  return 0;
}



#endif	/* HAVE_GAME */
