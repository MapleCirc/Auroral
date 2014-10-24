/*-------------------------------------------------------*/
/* battr.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : Board Attribution				 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#ifndef	_BATTR_H_
#define	_BATTR_H_


/* ----------------------------------------------------- */
/* Board Attribution : flags in BRD.battr		 */
/* ----------------------------------------------------- */


#define BRD_PERSONAL	0x00000001	/* 個人板 */
#define BRD_NOTRAN	0x00000002	/* 不轉信 */
#define BRD_NOCOUNT	0x00000004	/* 不計文章發表篇數 */
#define BRD_NOSTAT	0x00000008	/* 不納入熱門話題統計 */
#define BRD_NOVOTE	0x00000010	/* 不公佈投票結果於 [record] 板 */
#define BRD_ANONYMOUS	0x00000020	/* 匿名看板 */
#define BRD_NOSCORE	0x00000040	/* 不評分看板 */
#define BRD_XRTITLE	0x00000080	/* 加密文章顯示標題 */
#define BRD_LOCALSAVE	0x00000100	/* 轉信板預設存為站內 */
#define BRD_NOEXPIRE	0x00000200	/* 不以expire刪除過久的文章 */
#define BRD_LOG		0x00000400	/* 紀錄用看板 */
#define BRD_BATTR11	0x00000800
#define BRD_CROSSNOREC	0x00001000	/* 轉出文章不留下資訊 */



/* ----------------------------------------------------- */
/* 各種旗標的中文意義					 */
/* ----------------------------------------------------- */


#define NUMBATTRS	13

#define STR_BATTR	"zTcsvA%xLelrC"		/* itoc: 新增旗標的時候別忘了改這裡啊 */


#ifdef _ADMIN_C_
static char *battr_tbl[NUMBATTRS] =
{
  "個人板",			/* BRD_PERSONAL */
  "不轉信出去",			/* BRD_NOTRAN */
  "不記錄篇數",			/* BRD_NOCOUNT */
  "不做熱門話題統計",		/* BRD_NOSTAT */
  "不公開投票結果",		/* BRD_NOVOTE */
  "匿名看板",			/* BRD_ANONYMOUS */
  "不評分看板",			/* BRD_NOSCORE */
  "加密文章顯示標題",		/* BRD_XRTITLE */
  "轉信板預設存為站內",		/* BRD_LOCALSAVE */
  "不以expire刪除過久的文章",	/* BRD_NOEXPIRE */
  "紀錄用看板",			/* BRD_LOG */
  "(保留)",
  "轉出文章不留下資訊", 	/* BRD_CROSSNOREC */
};

#endif

#endif				/* _BATTR_H_ */

