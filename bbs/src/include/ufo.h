/*-------------------------------------------------------*/
/* ufo.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : User Flag Option				 */
/* create : 95/03/29				 	 */
/* update : 10/03/31				 	 */
/*-------------------------------------------------------*/


#ifndef	_UFO_H_
#define	_UFO_H_


/* ----------------------------------------------------- */
/* User Flag Option : flags in ACCT.ufo			 */
/* ----------------------------------------------------- */


#define BFLAG(n)	(1 << n)	/* 32 bit-wise flag */


#define UFO_NOUSE00	BFLAG(0)	/* 沒用到 */
#define UFO_MOVIE	BFLAG(1)	/* 動態看板顯示 */
#define UFO_BRDPOST	BFLAG(2)	/* 1: 看板列表顯示篇數  0: 看板列表顯示號碼 itoc.010912: 恆為新文章模式 */
#define UFO_BRDNAME	BFLAG(3)	/* itoc.010413: 看板列表依 1:brdname 0:class+title 排序 */
#define UFO_BRDNOTE	BFLAG(4)	/* 顯示進板畫面 */
#define UFO_VEDIT	BFLAG(5)	/* 簡化編輯器 */
#define UFO_MOTD	BFLAG(6)	/* 簡化進/離站畫面 */

#define UFO_PAGER	BFLAG(7)	/* 關閉呼叫器 */
#define UFO_RCVER	BFLAG(8)	/* itoc.010716: 拒收廣播 */
#define UFO_QUIET	BFLAG(9)	/* 結廬在人境，而無車馬喧 */
#define UFO_PAL		BFLAG(10)	/* 使用者名單只顯示好友 */
#define UFO_ALOHA	BFLAG(11)	/* 接受上站通知 */
#define UFO_NOALOHA	BFLAG(12)	/* itoc.010716: 上站不通知/協尋 */

#define UFO_BMWDISPLAY	BFLAG(13)	/* itoc.010315: 水球回顧介面 */
#define UFO_NWLOG       BFLAG(14)	/* lkchu.990510: 不存對話紀錄 */
#define UFO_NTLOG       BFLAG(15)	/* lkchu.990510: 不存聊天紀錄 */

#define UFO_NOSIGN	BFLAG(16)	/* itoc.000320: 不使用簽名檔 */
#define UFO_SHOWSIGN	BFLAG(17)	/* itoc.000319: 存檔前顯示簽名檔 */

#define UFO_ZHC		BFLAG(18)	/* hightman.060504: 全型字偵測 */
#define UFO_JUMPBRD	BFLAG(19)	/* itoc.020122: 自動跳去下一個未讀看板 */
#define UFO_IDLEONLY	BFLAG(20)	/* dust.100306: 使用者名單只顯示閒置時間 */
#define UFO_NODCBUM	BFLAG(21)	/* dust.100312: 關閉雙色看板未讀燈 */
#define UFO_UNIUNREADM	BFLAG(22)	/* dust.100312: 文章未讀記號僅使用一種顏色 */
#define UFO_VEKCP       BFLAG(23)	/* dust.100713: 維持游標位置(for 新VE) */

#define UFO_CLOAK	BFLAG(24)	/* 1: 進入隱形 */
#define UFO_SUPERCLOAK	BFLAG(25)	/* 1: 超級隱身 */
#define UFO_ACL		BFLAG(26)	/* 1: 使用 ACL */
#define UFO_ARLKNQ	BFLAG(27)	/* dust.100306: 站務踢除重複Login 詢問 */
#define UFO_KQDEFYES	BFLAG(28)	/* dust.100306: 站務踢除重複Login 詢問預設Y */
#define UFO_MFMGCTD	BFLAG(29)	/* dust.100312: 選單狀態列模式 -官方倒數 */
#define UFO_MFMCOMER	BFLAG(30)	/* dust.100312: 選單狀態列模式 -金融模式 */
#define UFO_MFMUCTD	BFLAG(31)	/* dust.100312: 選單狀態列模式 -使用者自訂倒數 */

/* dust.100713: UFO擴充欄位 */
#define UFO2_PSRENDER   BFLAG(0)	/* dust.101108: pmore語法上色(for 新VE) */
#define UFO2_VEMONOESC  BFLAG(1)	/* dust.100713: 不彩色顯示ESC字元(for 新VE) */
#define UFO2_MONOTONE   BFLAG(2)	/* dust.100713: 單色顯示一般模式(for 新VE) */
#define UFO2_CVMNOFIX   BFLAG(3)	/* dust.100713: 單色顯示一般模式(for 新VE) */
#define UFO2_SNRON      BFLAG(4)	/* dust.100906: 選擇簽名檔時不記住選項「不加」 */
#define UFO2_ENDJTNBP	BFLAG(5)	/* dust.100923: 文章列表End鍵跳至最後一篇非置底文章 */
#define UFO2_BRDNOTE	BFLAG(6)	/* dust.100924: 進板畫面顯示選項擴充用 */
#define UFO2_QUOTEMOVE  BFLAG(7)	/* dust.101108: 回文時游標移動到引言結束點 */

#define UFO2_LIMITVISIT BFLAG(8)	/* dust.101114: 設定看板為已讀功能的作用範圍 */
#define UFO2_NOINTRESC	BFLAG(9)	/* dust.110923: 去除一字雙色 */

#define UFO2_HIDEMYIP	BFLAG(10)	/* davy.110926: 隱藏IP */

/* 新註冊帳號、guest 的預設 ufo */

#define UFO_DEFAULT_NEW		(UFO_MOVIE | UFO_BRDNOTE | UFO_MOTD | UFO_BMWDISPLAY | UFO_NOSIGN)
#define UFO_DEFAULT_GUEST	(UFO_MOVIE | UFO_BRDNOTE | UFO_QUIET | UFO_NOALOHA | UFO_NWLOG | UFO_NTLOG | UFO_NOSIGN)


/* ----------------------------------------------------- */
/* Status : flags in UTMP.status			 */
/* ----------------------------------------------------- */


#define STATUS_BIFF	BFLAG(0)	/* 有新信件 */
#define STATUS_REJECT	BFLAG(1)	/* true if reject any body */
#define STATUS_BIRTHDAY	BFLAG(2)	/* 今天生日 */
#define STATUS_COINLOCK	BFLAG(3)	/* 錢幣鎖定 */
#define STATUS_DATALOCK	BFLAG(4)	/* 資料鎖定 */
#define STATUS_MQUOTA	BFLAG(5)	/* 信箱中有過期之信件 */
#define STATUS_MAILOVER	BFLAG(6)	/* 信箱過多信件 */
#define STATUS_MGEMOVER	BFLAG(7)	/* 個人精華區過多 */
#define STATUS_PALDIRTY BFLAG(8)	/* 有人在他的朋友名單新增或移除了我 */


#define	HAS_STATUS(x)	(cutmp->status&(x))


/* ----------------------------------------------------- */
/* 各種習慣的中文意義					 */
/* ----------------------------------------------------- */


/* itoc.000320: 增減項目要更改 NUMUFOS_* 大小, 也別忘了改 STR_UFO */

#define NUMUFOS		32
#define NUMUFOS_GUEST	5	/* guest 可以用前 5 個 ufo */
#define NUMUFOS_USER	20	/* 一般使用者 可以用前 20 個 ufo */

#define STR_UFO		"-mpsnemPBQFANbwtSHZJidUKCHANygEc"	/* itoc: 新增習慣的時候別忘了改這裡啊 */
#define STR_UFO2        "RvmcsebqLIh---------------------"


#ifdef _ADMIN_C_

#if 0	/* dust.100306: 裝了新介面後這個暫時用不到了 */
char *ufo_tbl[NUMUFOS] =
{
  "保留",				/* UFO_NOUSE */
  "顯示動態看板    (顯示/關掉)",	/* UFO_MOVIE */

  "看板列表顯示  (文章數/編號)",	/* UFO_BRDPOST */
  "看板列表排序依  (字母/分類)",	/* UFO_BRDNAME */	/* itoc.010413: 看板依照字母/分類排序 */
  "進板畫面        (顯示/跳過)",	/* UFO_BRDNOTE */
  "文章編輯器      (簡化/完整)",	/* UFO_VEDIT */
  "進/離站畫面     (簡化/完整)",	/* UFO_MOTD */

  "誰能丟你水球(只有好友/全部)",	/* UFO_PAGER */
#ifdef HAVE_NOBROAD
  "拒收別人廣播    (拒收/接收)",	/* UFO_RCVER */
#else
  "保留",
#endif
  "拒收任何人水球  (拒收/接收)",	/* UFO_QUITE */

  "使用者名單顯示  (好友/全部)",	/* UFO_PAL */

#ifdef HAVE_ALOHA
  "別人上站時通知我(通知/取消)",	/* UFO_ALOHA */
#else
  "保留",
#endif
#ifdef HAVE_NOALOHA
  "上站不通知別人(不通知/通知)",	/* UFO_NOALOHA */
#else
  "保留",
#endif

#ifdef BMW_DISPLAY
  "水球回顧介面    (完整/上次)",	/* UFO_BMWDISPLAY */
#else
  "保留",
#endif
  "離站時刪除水球？(刪除/問我)",	/* UFO_NWLOG */
  "離站刪除聊天紀錄(刪除/問我)",	/* UFO_NTLOG */

  "不使用簽名檔？  (不用/問我)",	/* UFO_NOSIGN */
  "顯示簽名檔供選擇(顯示/不看)",	/* UFO_SHOWSIGN */

#ifdef HAVE_MULTI_BYTE
  "全型字偵測      (偵測/不用)",	/* UFO_ZHC */
#else
  "保留",
#endif

#ifdef AUTO_JUMPBRD
  "自動跳去未讀看板(跳去/不跳)",	/* UFO_JUMPBRD */
#else
  "保留",
#endif

  "保留",
  "保留",
  "保留",
  "保留",

  "隱身術          (隱身/現身)",	/* UFO_CLOAK */
#ifdef HAVE_SUPERCLOAK
  "紫色隱身術      (紫隱/現身)",	/* UFO_SUPERCLOAK */
#else
  "保留",
#endif

  "站長上站來源    (限制/不限)"		/* UFO_ACL */
};
#endif

#endif

#endif				/* _UFO_H_ */

