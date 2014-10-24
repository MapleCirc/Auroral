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


#define BRD_PERSONAL	0x00000001	/* �ӤH�O */
#define BRD_NOTRAN	0x00000002	/* ����H */
#define BRD_NOCOUNT	0x00000004	/* ���p�峹�o��g�� */
#define BRD_NOSTAT	0x00000008	/* ���ǤJ�������D�έp */
#define BRD_NOVOTE	0x00000010	/* �����G�벼���G�� [record] �O */
#define BRD_ANONYMOUS	0x00000020	/* �ΦW�ݪO */
#define BRD_NOSCORE	0x00000040	/* �������ݪO */
#define BRD_XRTITLE	0x00000080	/* �[�K�峹��ܼ��D */
#define BRD_LOCALSAVE	0x00000100	/* ��H�O�w�]�s������ */
#define BRD_NOEXPIRE	0x00000200	/* ���Hexpire�R���L�[���峹 */
#define BRD_LOG		0x00000400	/* �����άݪO */
#define BRD_BATTR11	0x00000800
#define BRD_CROSSNOREC	0x00001000	/* ��X�峹���d�U��T */



/* ----------------------------------------------------- */
/* �U�غX�Ъ�����N�q					 */
/* ----------------------------------------------------- */


#define NUMBATTRS	13

#define STR_BATTR	"zTcsvA%xLelrC"		/* itoc: �s�W�X�Ъ��ɭԧO�ѤF��o�̰� */


#ifdef _ADMIN_C_
static char *battr_tbl[NUMBATTRS] =
{
  "�ӤH�O",			/* BRD_PERSONAL */
  "����H�X�h",			/* BRD_NOTRAN */
  "���O���g��",			/* BRD_NOCOUNT */
  "�����������D�έp",		/* BRD_NOSTAT */
  "�����}�벼���G",		/* BRD_NOVOTE */
  "�ΦW�ݪO",			/* BRD_ANONYMOUS */
  "�������ݪO",			/* BRD_NOSCORE */
  "�[�K�峹��ܼ��D",		/* BRD_XRTITLE */
  "��H�O�w�]�s������",		/* BRD_LOCALSAVE */
  "���Hexpire�R���L�[���峹",	/* BRD_NOEXPIRE */
  "�����άݪO",			/* BRD_LOG */
  "(�O�d)",
  "��X�峹���d�U��T", 	/* BRD_CROSSNOREC */
};

#endif

#endif				/* _BATTR_H_ */

