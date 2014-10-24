/*-------------------------------------------------------*/
/* gray.c         ( Tcfsh CIRC 27th )                    */
/*-------------------------------------------------------*/
/* target : 黑白棋遊戲 (新)                              */
/* create : 11/02/21                        學測放榜     */
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

  move(0, 0);                             /* 清螢幕       */
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
  outs("按任意鍵離開...");
  vkey();
  return 0;
}



#endif	/* HAVE_GAME */
