/*-------------------------------------------------------*/
/* config.h	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : site-configurable settings		 	 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


/*--------------------------------------------------------------------*/
/* MapleBridge Bulletin Board System  Version: 2.36                   */
/* Copyright (C) 1994-1995            Jeng-Hermes Lin, Hung-Pin Chen, */
/*                                    SoC, Xshadow                    */
/*--------------------------------------------------------------------*/
/* MapleBridge Bulletin Board System  Version: 3.10                   */
/* Copyright (C) 1997-1998            Jeng-Hermes Lin                 */
/*--------------------------------------------------------------------*/


#ifndef	_CONFIG_H_
#define	_CONFIG_H_


/* ----------------------------------------------------- */
/* �w�q BBS ���W��}					 */
/* ------------------------------------------------------*/

#define SCHOOLNAME	"�@��q��"		/* ��´�W�� */
#define BBSNAME		"�����H��"		/* ���寸�W */
#define BBSNAME2	"TCIRC"	  		/* �^�寸�W */
#define SYSOPNICK	"�ȪA��"		/* sysop ���ʺ� */
#define TAG_VALID       "["BBSNAME2"]To"	/* �����{�Ҩ� token */

#define MYIPADDR	"210.60.107.213"	/* IP address */
#define MYHOSTNAME	"tcirc.twbbs.org"	/* �����a�} FQDN */

#define HOST_ALIASES	{MYHOSTNAME, MYIPADDR, \
			 "tcirc.org", \
			 NULL}

#define MYCHARSET	"big5"			/* BBS �ҨϥΪ��r�� */

#define BBSHOME		"/home/bbs"		/* BBS ���a */
#define BAKPATH		"/backup2"		/* �ƥ��ɪ����| */

#define BBSUID		1001
#define BBSGID		1001			/* �Ѧҥثe��BBS�b����UID */


/* ----------------------------------------------------- */
/* �պA�W��						 */
/* ----------------------------------------------------- */

  /* ------------------------------------------------- */
  /* �պA�W�����~�ӼҲ�                                */
  /* ------------------------------------------------- */

#define M3_USE_PMORE            /* dust.091029: �ϥ�piaip's more */
#define M3_USE_PFTERM           /* dust.091029: �ϥ�piaip's flat terminal system, the perfect terminal! */

#ifdef M3_USE_PFTERM
# define USE_PFTERM
# define HAVE_GRAYOUT
#endif

#define USE_DUST_KRANS          /* davy.20120623: �ϥΦǹФj�j�� krans */
#define USE_DAVY_REGISTER       /* davy.20120624: �ϥηs�E���U��f�֨t�� */

#ifdef USE_DAVY_REGISTER
# define HAVE_REGISTER_FORM     /* �j��}�ҵ��U��{�� */
#endif

  /* ------------------------------------------------- */
  /* �պA�W�����t�ΰl��                                */
  /* ------------------------------------------------- */

#define	HAVE_SEM		/* �ϥ� semaphore */

#define	HAVE_MMAP		/* �ĥ� mmap(): memory mapped I/O */

#ifndef CYGWIN
#define	HAVE_RLIMIT		/* �ĥ� resource limit�ACygwin ����� */
#endif

#undef	MODE_STAT               /* �[��βέp user ���ͺA�A�H�����g���w */

#undef	SYSOP_CHECK_MAIL	/* itoc.001029: �����i�HŪ���ϥΪ̫H�c */

#undef	SYSOP_SU		/* itoc.001102: �H sysop �n�J�i�H�ܧ�ϥΪ̨��� */

#define	HAVE_MULTI_BYTE		/* hightman.060504: �䴩���r�`�~�r�B�z */

  /* ------------------------------------------------- */
  /* �պA�W�������U�{��                                */
  /* ------------------------------------------------- */

#define LOGINASNEW		/* �ĥΤW���ӽбb����� */

#ifdef LOGINASNEW
#undef	HAVE_GUARANTOR		/* itoc.000319: �ĥΫO�ҤH��� */
#endif

#undef	HAVE_LOGIN_DENIED	/* itoc.000319: �ױ��Y�Ǩӷ����s���A�ѷ� etc/bbs.acl */

#undef	NEWUSER_LIMIT		/* �s��W�����T�ѭ��� */

#define	JUSTIFY_PERIODICAL	/* �w�������{�� */

#undef	EMAIL_JUSTIFY		/* �o�X Internet Email �����{�ҫH�� */

#ifdef EMAIL_JUSTIFY
#define	HAVE_POP3_CHECK		/* itoc.000315: POP3 �t�λ{�� */
#define	HAVE_REGKEY_CHECK	/* itoc.010112: �{�ҽX���� */
#endif

#define	HAVE_REGISTER_FORM	/* ���U��{�� */

  /* ------------------------------------------------- */
  /* �պA�W��������B��                                */
  /* ------------------------------------------------- */

#define	HAVE_MODERATED_BOARD	/* ���Ѧn�ͯ��K�O */	/* ���K�ݪO���\Ū�v���n�O PERM_SYSOP �~�|�Q�������K�ݪO */
							/* �n�ͬݪO���\Ū�v���n�O PERM_BOARD �~�|�Q�����n�ͬݪO */
#define	CHECK_ONLINE		/* itoc.010306: �峹�C�����i�H��ܨϥΪ̬O�_�b���W */

#define	HAVE_BADPAL		/* itoc.010302: �����a�H���\�� */

#define	HAVE_LIST		/* itoc.010923: �s�զW�� */

#define HAVE_ALOHA              /* itoc.001202: �W���q�� */

#undef	LOGIN_NOTIFY            /* �t�Ψ�M���� */

#if (defined(HAVE_ALOHA) || defined(LOGIN_NOTIFY))
#define	HAVE_NOALOHA		/* itoc.010716: �W�����q��/��M */
#endif

#define	LOG_BMW                 /* lkchu.981201: ���y�O���B�z */

#ifdef LOG_BMW
#define	RETAIN_BMW		/* itoc.021102: ���y�s�� */
#endif

#define	LOG_TALK		/* lkchu.981201: ��ѰO���B�z */

#define	HAVE_NOBROAD		/* itoc.010716: �ڦ��s�� */

#define	BMW_COUNT		/* itoc.010312: �p�⤤�F�X�Ӥ��y */

#define	BMW_DISPLAY		/* itoc.010313: ��ܤ��e�����y */

#define	HAVE_CHANGE_NICK	/* �ϥΪ̦W�� ^N �ä[���ʺ� */

#define	HAVE_CHANGE_FROM        /* �ϥΪ̦W�� ^F �Ȯɧ��G�m */

#define	HAVE_CHANGE_ID		/* �ϥΪ̦W�� ^D �Ȯɧ�� ID */

#define	HAVE_WHERE		/* itoc.001102: �ϥΪ̦W��G�m���ѡA�ѷ� etc/host fqdn */

#ifdef HAVE_WHERE
#define GUEST_WHERE		/* itoc.010208: guest �üƨ��G�m */
#endif

#define	GUEST_NICK		/* itoc.000319: guest �üƨ��ʺ� */

#define	DETAIL_IDLETIME		/* itoc.020316: �ɱ`��s���m�ɶ� */

#define	TIME_KICKER		/* itoc.030514: �O�_�۰�ñ�h idle �L�[���ϥΪ� */

#define	HAVE_BRDMATE		/* itoc.020602: �O�� (�\Ū�P�@�ݪO) */

#define	HAVE_SUPERCLOAK		/* itoc.020602: �����A�����W������ */

  /* ------------------------------------------------- */
  /* �պA�W�����ݪO�H�c                                */
  /* ------------------------------------------------- */
      
#define	HAVE_ANONYMOUS		/* ���� anonymous �O */


#ifdef HAVE_ANONYMOUS
#define	HAVE_UNANONYMOUS_BOARD	/* itoc.020602: �ϰΦW�O�A�������} BN_UNANONYMOUS */
#endif

#define	SHOW_USER_IN_TEXT       /* �b��� Ctrl+Q �i��� User ���W�r */

#undef	ANTI_PHONETIC		/* itoc.030503: �T�Ϊ`���� */

#define	ENHANCED_VISIT		/* itoc.010407: �wŪ/��Ū�ˬd�O�ӬݪO���̫�@�g�M�w */

#define	SLIDE_SHOW		/* itoc.030411: �۰ʼ���峹 */

#undef	COLOR_HEADER            /* lkchu.981201: �ܴ��m����Y */

#define	CURSOR_BAR		/* itoc.010113: �����ΡA�Y�}�ҿ����ΡA���N���঳�C�ⱱ��X */

#define	HAVE_DECLARE		/* �� title ���� [] �����A�B����W�� */

#define	HAVE_POPUPMENU          /* �ۥX����� */

#ifdef HAVE_POPUPMENU
#define	POPUP_ANSWER		/* �ۥX����� -- �߰ݿﶵ */
#define	POPUP_MESSAGE		/* �ۥX����� -- �T������ */
#endif

#define	AUTHOR_EXTRACTION	/* �M��P�@�@�̤峹 */

#define	HAVE_TERMINATOR		/* �׵��峹�j�k --- �ط������� */

#define	HAVE_XYNEWS		/* itoc.010822: �s�D�\Ū�Ҧ� */

#define	HAVE_SCORE		/* itoc.020114: �峹�����\�� */

#define	HAVE_DETECT_CROSSPOST	/* cross-post �۰ʰ��� */

#define	AUTO_JUMPBRD		/* itoc.010910: �ݪO�C���۰ʸ��h�U�@�ӥ�Ū�ݪO */

#undef	AUTO_JUMPPOST		/* itoc.010910: �峹�C���۰ʸ��h�̫�@�g��Ū */

#define	ENHANCED_BSHM_UPDATE	/* itoc.021101: �ݪO�C���R��/�аO�峹���C�J��Ū���O */

#define	EVERY_Z			/* ctrl-z Everywhere (���F�g�峹) */

#define	EVERY_BIFF		/* Thor.980805: �l�t��B�ӫ��a */

#undef	HAVE_SIGNED_MAIL	/* Thor.990409: �~�e�H��[ñ�W */

#define	INPUT_TOOLS		/* itoc.000319: �Ÿ���J�u�� */

#define	HAVE_FORCE_BOARD	/* itoc.000319: �j�� user login �ɭ�Ū���Y���i�ݪO */
				/* itoc.010726: �Ӥ��i�ݪO�n���i zap�A�_�h zap �ᤣ�|�O�� brh */

#define	MY_FAVORITE		/* itoc.001202: ���ѧڪ��̷R�ݪO */

#define	HAVE_COSIGN		/* itoc.010108: ���ѬݪO�s�p */

#ifdef HAVE_COSIGN
#undef	SYSOP_START_COSIGN	/* itoc.030613: �s�O�s�p�n���g�����f�֤~��}�l */
#endif

#define	HAVE_REFUSEMARK		/* itoc.010602: ���ѬݪO�峹�[�K */

#define	HAVE_LABELMARK		/* itoc.020307: ���ѬݪO�峹�[�ݬ�аO */

#define	POST_PREFIX		/* itoc.020113: �o���峹�ɼ��D�i��ܺ��� */

#define HAVE_TEMPLATE		/* floatj.080908: �峹�d�� */
				/* �n����POST_PREFIX�ɰO�o�s�o�Ӥ@�_��(���M���ӥi�ள��POST_PREFIX�N�O...) */

#define	MULTI_MAIL		/* �s�ձH�H�\�� */

#define	HAVE_MAIL_ZIP		/* itoc.010228: ���ѧ�H��/��ذ����Y��H���\�� */

#undef	OVERDUE_MAILDEL		/* itoc.020217: �M���L���H�� */

#define	MOVE_CAPABLE		/* dust.070817: ���ʤ峹���� */ 

#define BOTTOM "\033[35m��\033[m"
/* dust.070820: �m���媺�лx �t�C�� */


  /* ------------------------------------------------- */
  /* �պA�W�����~���{��                                */
  /* ------------------------------------------------- */

#define	HAVE_EXTERNAL		/* Xyz ��� */

#ifdef	HAVE_EXTERNAL
#  define HAVE_SONG		/* itoc.010205: �����I�q�\�� */
#  define HAVE_NETTOOL		/* itoc.010209: ���Ѻ����A�Ȥu�� */
#  define HAVE_GAME		/* itoc.010208: ���ѹC�� */
#  define HAVE_BUY		/* itoc.010716: �����ʶR�v�� */
#  undef HAVE_TIP		/* itoc.010301: ���ѨC��p���Z */
#  define HAVE_CLASSTABLE	/* itoc.010907: ���ѥ\�Ҫ� */
#  define HAVE_CREDIT		/* itoc.020125: ���ѰO�b�� */
#  undef HAVE_LOVELETTER	/* itoc.020602: ���ѱ��Ѳ��;� */
#  define HAVE_CALENDAR		/* itoc.020831: ���ѸU�~�� */
#  define SP_SEND_MONEY		/* dust.080104: �����o���\�� */
#endif

#ifdef HAVE_SONG
#  define HAVE_SONG_CAMERA	/* itoc.010207: �����I�q��ʺA�ݪO�\�� */
#  define LOG_SONG_USIES	/* itoc.010928: �I�q�O�� */
#endif


/* ----------------------------------------------------- */
/* �պA�]�w                                              */
/* ------------------------------------------------------*/

#ifdef	HAVE_MMAP
#include <sys/mman.h>
#endif

#ifdef HAVE_SIGNED_MAIL
#define PRIVATE_KEY_PERIOD 0	/* �����X�Ѵ��@��key�A0 ���ܤ���key�A�۰ʲ��� */
#endif

#ifndef	SHOW_USER_IN_TEXT
#define outx	outs
#endif


/* ----------------------------------------------------- */
/* �t�ΰѼƣ��H BBS ���W�Ҧ������X�W		         */
/* ----------------------------------------------------- */

#define MAXBOARD	1024		/* �̤j�}�O�Ӽ� */

#define MAXACTIVE	512		/* �̦h�P�ɤW���H�� */

#define MAXAPPLY	100000		/* �̤j�b�����U�H�� */

/* ----------------------------------------------------- */
/* �t�ΰѼƣ���L�`�ΰѼ�                                */
/* ----------------------------------------------------- */


/* bmw.c NPC��ID*/
#define NPC_ID		"yukinagato"

/* bbsd.c �W���n�J */

#define LOGINATTEMPTS	3		/* �̤j�i���K�X�������� */
#define MULTI_MAX	2		/* �@��ϥΪ̳̤j multi-login �Ӽ� */
#define GUEST_MULTI_MAX	10		/* guest�H�ƤW�� */

/* bbsd.c guest�b�����]�w */

#define GUEST_NICK_LIST_MAX 9
#define GUEST_NICK_LIST {"����", "�ڽ�", "�U�l", "��y", "����", "�s��", "����", "���H", "��K"}

#define GUEST_FROM_LIST_MAX 16
#define GUEST_FROM_LIST {"�_���I", "�J�w����", "�q���\\�U", "�氮���D", "�R�A�s��", "�^�Ф���", "�V���k����", "�����A��", "�ƤH�B", "�̶�˪L�C�ְ�", "���Ƽ�", "���N���ߥ���U", "�����Ŭu�|�]", "���߿ߪ���", "�q�~���l", "�h�h�����{"}


/* bbsd.c user.c admutil.c �w�������{�ҡA�O�o�P�B�ק� gem/@/@re-reg */

#ifdef JUSTIFY_PERIODICAL
#define VALID_PERIOD		(86400 * 365)	/* �����{�Ҧ��Ĵ�(��) */
#define INVALID_NOTICE_PERIOD	(86400 * 10)	/* �����{�ҥ��īe�h�[�ɶ������ϥΪ�(��) */
#endif

/* talk.c �B�ͦW��/���y�C�� */

/* itoc.010825: �`�N BMW_LOCAL_MAX >= BMW_PER_USER */

#define PAL_MAX		512		/* �B�ͦW��W��(�H) */
#define BMW_EXPIRE	60		/* ���y�B�z�ɶ�(��) */
#define BMW_PER_USER	5		/* ���y�B�z�ɶ����A���\��X�Ӥ��y */
#define BMW_MAX		128		/* UCACHE �� pool �W�� */
#define BMW_LOCAL_MAX	8		/* Ctrl+R �W�U�ү��s�����e���y�Ӽ� */

/* aloha.c �W���q�� */

#ifdef HAVE_ALOHA
#define ALOHA_MAX	64		/* �W���q���W��(�H) */
#endif

/* menu.c �d���O */

#define NOTE_MAX	100		/* �d���O�O�d�g�� */
#define NOTE_DUE	48		/* �d���O�O�d�ɶ�(��) */

/* mail.c �Ψ��ˬd�H�c�j�p�A�W�L�h���ܡA�O�o�P�B�ק� etc/justified gem/@/@mailover */

#define	MAX_BBSMAIL	1000		/* PERM_MBOX ���H�W��(��) */
#define	MAX_VALIDMAIL	500		/* �{�� user ���H�W��(��) */
#define	MAX_NOVALIDMAIL	100		/* ���{�� user ���H�W��(��) */

/* bquota.c mail.c �ΨӲM�L���ɮ�/�H�󪺮ɶ��A�O�o�P�B�ק� etc/justified */

#ifdef OVERDUE_MAILDEL
#define MARK_DUE        1080		/* �аO�O�s���H��O�d�ɶ�(��) */
#define MAIL_DUE        720		/* �@��H��O�d�ɶ�(��) */
#define FILE_DUE        360		/* ��L�ɮ׫O�d�ɶ�(��) */
#endif

/* newbrd.c �ݪO�s�p */

#ifdef HAVE_COSIGN
#define NBRD_NUM_BRD	5		/* �}�O�ݭn�s�p�H�� */
#define NBRD_DAY_BRD	10		/* �}�O�i�s�p�Ѽ� */
#endif

/* post.c ���� cross-post */

#ifdef  HAVE_DETECT_CROSSPOST
#define	MAX_CROSS_POST		5	/* cross post �̤j�ƶq(�g) */
#define CROSSPOST_DENY_DAY	30	/* cross post ���v�ɶ�(��) */
#endif

/* post.c manage.c �峹���O�ϥΪ� */

#define NUM_PREFIX_DEF	6
#define PREFIX_MAX	10
#define PREFIX_LEN	21

/* post.c manage.c �峹���O�ϥΪ� */

#define NUM_PREFIX_DEF	6
#define PREFIX_MAX	10
#define PREFIX_LEN	21
#define DEFAULT_PREFIX	{"[���i]", "[����]", "[����]", "[���]", "[�L��]", "[���]", "[����]", "[�Ч@]", "[���V]", "[�n�o]"}

/* more.c  Post Law ����ƤW�� */

#define POSTLAW_LINEMAX 18

/* visio.c bguard.c �۰ʽ�H */

#ifdef TIME_KICKER
#define IDLE_TIMEOUT	70		/* visio.c bguard.c �o�b�L�[�۰�ñ�h(��) */
#define IDLE_WARNOUT	3		/* visio.c �o�b�L�[����(��) -- �۰�ñ�h�e�T�����e */
#endif

/* more.c edit.c ½�� */
#define ESCODE_MAXLEN	15		/* dust.100831: esc-star���X�̤j���� */

#define PAGE_SCROLL	(b_lines - 1)	/* �� PgUp/PgDn �n���ʴX�C */

#define MAXLASTCMD	8		/* line input buffer */
#define	BLK_SIZ		4096		/* disk I/O block size */
#define MAXSIGLINES	6		/* edit.c ñ�W�ɤޤJ�̤j��� */
#define MAXQUERYLINES	17		/* talk.c xchatd.c ��� Query/Plan �T���̤j��� */
#define	MAX_CHOICES	32		/* vote.c �벼�ɳ̦h�� 32 �ؿ�� */
#define	TAG_MAX		256		/* xover.c TagList ���Ҽƥؤ��W�� */
#define LINE_HEADER	3		/* more.c bhttpd.c ���Y���T�C */

/* bbsd.c mail.c ��z�g�� */

#define	CHECK_PERIOD	(86400 * 20)	/* ��z�H�c/�B�ͦW�檺�g��(��) */

/* camera.c �ʺA�ݪO */

#define	MOVIE_MAX	200		/* �ʵe�i�� */
#define	MOVIE_SIZE	(120 * 1024)	/* �ʵe cache size */
#define FILM_SIZ	4096		/* ��i�e���̤j�j�p */
#define OPENING_MAX	10		/* �}�Y�e�����̤j�ƶq */


/* ----------------------------------------------------- */
/* chat.c & xchatd.c ���ĥΪ� protocol			 */
/* ------------------------------------------------------*/

#define CHAT_SECURE			/* �w������ѫ� */

#define EXIT_LOGOUT     0
#define EXIT_LOSTCONN   -1
#define EXIT_CLIERROR   -2
#define EXIT_TIMEDOUT   -3
#define EXIT_KICK       -4

#define CHAT_LOGIN_OK       "OK"
#define CHAT_LOGIN_EXISTS   "EX"
#define CHAT_LOGIN_INVALID  "IN"
#define CHAT_LOGIN_BOGUS    "BG"


/* ----------------------------------------------------- */
/* BBS �A�ȩҥΪ� port					 */
/* ------------------------------------------------------*/

/* itoc.030512: �p�G�n�b�P�@�x�����W�[�G�� bbs ���A����n�G

   1) �b OS �}�@�ӷs�� user (�Ҧp�s bbs2)�A�� uid �n�M�t�@�� bbs ���P
   2) �� BBSHOME�BBAKPATH�BBBSUID�BBBSGID �� bbs2 �����|��UID�BGID
   3) ��H�U�� *_PORT �� *_KEY �M�t�@�� bbs ���P (�Ҧp���[�W 10000)
*/

#define MAX_BBSDPORT	1		/* bbsd �n�}�X�� port�A�H BBSD_PORT ���� */
#define BBSD_PORT	{23 /*,3456*/}	/* bbsd   �ҥΪ� port (bbsd.c) */
#define BMTA_PORT	3026		/* SMTP   �ҥΪ� port (bmtad.c) */
#define GEMD_PORT	70		/* Gopher �ҥΪ� port (gemd.c) */
#define FINGER_PORT	79		/* Finger �ҥΪ� port (bguard.c) */
#define BHTTP_PORT	8001		/* HTTP   �ҥΪ� port (bhttpd.c) */
#define POP3_PORT	110		/* POP3   �ҥΪ� port (bpop3d.c) */
#define BNNTP_PORT	119		/* NNTP   �ҥΪ� port (bnntp.c) */
#define CHAT_PORT	3838		/* ��ѫ� �ҥΪ� port (chat.c xchatd.c) */
#define INNBBS_PORT	7777		/* ��H   �ҥΪ� port (channel.c) */


/* ----------------------------------------------------- */
/* SHM �� SEM �ҥΪ� key				 */
/* ----------------------------------------------------- */

#define BRDSHM_KEY	2997	/* �ݪO */
#define UTMPSHM_KEY	1998	/* �ϥΪ� */
#define FILMSHM_KEY	2999	/* �ʺA�ݪO */
#define PIPSHM_KEY	4998	/* �q�l����� */

#define	BSEM_KEY	2000	/* semaphore key */
#define	BSEM_FLG	0600	/* semaphore mode */
#define BSEM_ENTER      -1	/* enter semaphore */
#define BSEM_LEAVE      1	/* leave semaphore */
#define BSEM_RESET	0	/* reset semaphore */

#endif				/* _CONFIG_H_ */