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


#define UFO_NOUSE00	BFLAG(0)	/* �S�Ψ� */
#define UFO_MOVIE	BFLAG(1)	/* �ʺA�ݪO��� */
#define UFO_BRDPOST	BFLAG(2)	/* 1: �ݪO�C����ܽg��  0: �ݪO�C����ܸ��X itoc.010912: ���s�峹�Ҧ� */
#define UFO_BRDNAME	BFLAG(3)	/* itoc.010413: �ݪO�C��� 1:brdname 0:class+title �Ƨ� */
#define UFO_BRDNOTE	BFLAG(4)	/* ��ܶi�O�e�� */
#define UFO_VEDIT	BFLAG(5)	/* ²�ƽs�边 */
#define UFO_MOTD	BFLAG(6)	/* ²�ƶi/�����e�� */

#define UFO_PAGER	BFLAG(7)	/* �����I�s�� */
#define UFO_RCVER	BFLAG(8)	/* itoc.010716: �ڦ��s�� */
#define UFO_QUIET	BFLAG(9)	/* ���f�b�H�ҡA�ӵL������ */
#define UFO_PAL		BFLAG(10)	/* �ϥΪ̦W��u��ܦn�� */
#define UFO_ALOHA	BFLAG(11)	/* �����W���q�� */
#define UFO_NOALOHA	BFLAG(12)	/* itoc.010716: �W�����q��/��M */

#define UFO_BMWDISPLAY	BFLAG(13)	/* itoc.010315: ���y�^�U���� */
#define UFO_NWLOG       BFLAG(14)	/* lkchu.990510: ���s��ܬ��� */
#define UFO_NTLOG       BFLAG(15)	/* lkchu.990510: ���s��Ѭ��� */

#define UFO_NOSIGN	BFLAG(16)	/* itoc.000320: ���ϥ�ñ�W�� */
#define UFO_SHOWSIGN	BFLAG(17)	/* itoc.000319: �s�ɫe���ñ�W�� */

#define UFO_ZHC		BFLAG(18)	/* hightman.060504: �����r���� */
#define UFO_JUMPBRD	BFLAG(19)	/* itoc.020122: �۰ʸ��h�U�@�ӥ�Ū�ݪO */
#define UFO_IDLEONLY	BFLAG(20)	/* dust.100306: �ϥΪ̦W��u��ܶ��m�ɶ� */
#define UFO_NODCBUM	BFLAG(21)	/* dust.100312: ��������ݪO��Ū�O */
#define UFO_UNIUNREADM	BFLAG(22)	/* dust.100312: �峹��Ū�O���ȨϥΤ@���C�� */
#define UFO_VEKCP       BFLAG(23)	/* dust.100713: ������Ц�m(for �sVE) */

#define UFO_CLOAK	BFLAG(24)	/* 1: �i�J���� */
#define UFO_SUPERCLOAK	BFLAG(25)	/* 1: �W������ */
#define UFO_ACL		BFLAG(26)	/* 1: �ϥ� ACL */
#define UFO_ARLKNQ	BFLAG(27)	/* dust.100306: ���Ƚ𰣭���Login �߰� */
#define UFO_KQDEFYES	BFLAG(28)	/* dust.100306: ���Ƚ𰣭���Login �߰ݹw�]Y */
#define UFO_MFMGCTD	BFLAG(29)	/* dust.100312: ��檬�A�C�Ҧ� -�x��˼� */
#define UFO_MFMCOMER	BFLAG(30)	/* dust.100312: ��檬�A�C�Ҧ� -���ļҦ� */
#define UFO_MFMUCTD	BFLAG(31)	/* dust.100312: ��檬�A�C�Ҧ� -�ϥΪ̦ۭq�˼� */

/* dust.100713: UFO�X�R��� */
#define UFO2_PSRENDER   BFLAG(0)	/* dust.101108: pmore�y�k�W��(for �sVE) */
#define UFO2_VEMONOESC  BFLAG(1)	/* dust.100713: ���m�����ESC�r��(for �sVE) */
#define UFO2_MONOTONE   BFLAG(2)	/* dust.100713: �����ܤ@��Ҧ�(for �sVE) */
#define UFO2_CVMNOFIX   BFLAG(3)	/* dust.100713: �����ܤ@��Ҧ�(for �sVE) */
#define UFO2_SNRON      BFLAG(4)	/* dust.100906: ���ñ�W�ɮɤ��O��ﶵ�u���[�v */
#define UFO2_ENDJTNBP	BFLAG(5)	/* dust.100923: �峹�C��End����̫ܳ�@�g�D�m���峹 */
#define UFO2_BRDNOTE	BFLAG(6)	/* dust.100924: �i�O�e����ܿﶵ�X�R�� */
#define UFO2_QUOTEMOVE  BFLAG(7)	/* dust.101108: �^��ɴ�в��ʨ�ި������I */

#define UFO2_LIMITVISIT BFLAG(8)	/* dust.101114: �]�w�ݪO���wŪ�\�઺�@�νd�� */
#define UFO2_NOINTRESC	BFLAG(9)	/* dust.110923: �h���@�r���� */

#define UFO2_HIDEMYIP	BFLAG(10)	/* davy.110926: ����IP */

/* �s���U�b���Bguest ���w�] ufo */

#define UFO_DEFAULT_NEW		(UFO_MOVIE | UFO_BRDNOTE | UFO_MOTD | UFO_BMWDISPLAY | UFO_NOSIGN)
#define UFO_DEFAULT_GUEST	(UFO_MOVIE | UFO_BRDNOTE | UFO_QUIET | UFO_NOALOHA | UFO_NWLOG | UFO_NTLOG | UFO_NOSIGN)


/* ----------------------------------------------------- */
/* Status : flags in UTMP.status			 */
/* ----------------------------------------------------- */


#define STATUS_BIFF	BFLAG(0)	/* ���s�H�� */
#define STATUS_REJECT	BFLAG(1)	/* true if reject any body */
#define STATUS_BIRTHDAY	BFLAG(2)	/* ���ѥͤ� */
#define STATUS_COINLOCK	BFLAG(3)	/* ������w */
#define STATUS_DATALOCK	BFLAG(4)	/* �����w */
#define STATUS_MQUOTA	BFLAG(5)	/* �H�c�����L�����H�� */
#define STATUS_MAILOVER	BFLAG(6)	/* �H�c�L�h�H�� */
#define STATUS_MGEMOVER	BFLAG(7)	/* �ӤH��ذϹL�h */
#define STATUS_PALDIRTY BFLAG(8)	/* ���H�b�L���B�ͦW��s�W�β����F�� */


#define	HAS_STATUS(x)	(cutmp->status&(x))


/* ----------------------------------------------------- */
/* �U�زߺD������N�q					 */
/* ----------------------------------------------------- */


/* itoc.000320: �W��حn��� NUMUFOS_* �j�p, �]�O�ѤF�� STR_UFO */

#define NUMUFOS		32
#define NUMUFOS_GUEST	5	/* guest �i�H�Ϋe 5 �� ufo */
#define NUMUFOS_USER	20	/* �@��ϥΪ� �i�H�Ϋe 20 �� ufo */

#define STR_UFO		"-mpsnemPBQFANbwtSHZJidUKCHANygEc"	/* itoc: �s�W�ߺD���ɭԧO�ѤF��o�̰� */
#define STR_UFO2        "RvmcsebqLIh---------------------"


#ifdef _ADMIN_C_

#if 0	/* dust.100306: �ˤF�s������o�ӼȮɥΤ���F */
char *ufo_tbl[NUMUFOS] =
{
  "�O�d",				/* UFO_NOUSE */
  "��ܰʺA�ݪO    (���/����)",	/* UFO_MOVIE */

  "�ݪO�C�����  (�峹��/�s��)",	/* UFO_BRDPOST */
  "�ݪO�C��ƧǨ�  (�r��/����)",	/* UFO_BRDNAME */	/* itoc.010413: �ݪO�̷Ӧr��/�����Ƨ� */
  "�i�O�e��        (���/���L)",	/* UFO_BRDNOTE */
  "�峹�s�边      (²��/����)",	/* UFO_VEDIT */
  "�i/�����e��     (²��/����)",	/* UFO_MOTD */

  "�֯��A���y(�u���n��/����)",	/* UFO_PAGER */
#ifdef HAVE_NOBROAD
  "�ڦ��O�H�s��    (�ڦ�/����)",	/* UFO_RCVER */
#else
  "�O�d",
#endif
  "�ڦ�����H���y  (�ڦ�/����)",	/* UFO_QUITE */

  "�ϥΪ̦W�����  (�n��/����)",	/* UFO_PAL */

#ifdef HAVE_ALOHA
  "�O�H�W���ɳq����(�q��/����)",	/* UFO_ALOHA */
#else
  "�O�d",
#endif
#ifdef HAVE_NOALOHA
  "�W�����q���O�H(���q��/�q��)",	/* UFO_NOALOHA */
#else
  "�O�d",
#endif

#ifdef BMW_DISPLAY
  "���y�^�U����    (����/�W��)",	/* UFO_BMWDISPLAY */
#else
  "�O�d",
#endif
  "�����ɧR�����y�H(�R��/�ݧ�)",	/* UFO_NWLOG */
  "�����R����Ѭ���(�R��/�ݧ�)",	/* UFO_NTLOG */

  "���ϥ�ñ�W�ɡH  (����/�ݧ�)",	/* UFO_NOSIGN */
  "���ñ�W�ɨѿ��(���/����)",	/* UFO_SHOWSIGN */

#ifdef HAVE_MULTI_BYTE
  "�����r����      (����/����)",	/* UFO_ZHC */
#else
  "�O�d",
#endif

#ifdef AUTO_JUMPBRD
  "�۰ʸ��h��Ū�ݪO(���h/����)",	/* UFO_JUMPBRD */
#else
  "�O�d",
#endif

  "�O�d",
  "�O�d",
  "�O�d",
  "�O�d",

  "�����N          (����/�{��)",	/* UFO_CLOAK */
#ifdef HAVE_SUPERCLOAK
  "���������N      (����/�{��)",	/* UFO_SUPERCLOAK */
#else
  "�O�d",
#endif

  "�����W���ӷ�    (����/����)"		/* UFO_ACL */
};
#endif

#endif

#endif				/* _UFO_H_ */

