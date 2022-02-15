/*-------------------------------------------------------*/
/* lib/mcd.h		   ( Maple Auroral BBS )	 */
/*-------------------------------------------------------*/
/* target : header file for menu countdown		 */
/* create : 2022/02/16					 */
/* update : 2022/02/16					 */
/* used   : dao.h struct.h                               */
/*-------------------------------------------------------*/

#ifndef	_MCD_H_
#define _MCD_H_

/* --------------------------------------------------- */
/* Menu foot 倒數計時的結構體				 */
/* --------------------------------------------------- */

typedef struct
{
  char name[12];
  int year, mon, day, hr, min;
  int warning;
} MCD_info;

#endif // #ifndef _MCD_H_

