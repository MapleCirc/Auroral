/*-------------------------------------------------------*/
/* util/addpost.c        ( Maple-itoc tcirc bbs )        */
/*-------------------------------------------------------*/
/* target : �峹�o��I�s��API                            */
/* author : pioneerLike (Circ25) , modified by floatJ    */
/* create : 2009/02/27                                   */
/* update : 2009/02/27                                   */
/*-------------------------------------------------------*/


#include "bbs.h"
#define TMP_PATH "/home/bbs/tmp/addpost_buf.tmp"


/* �O�d�[�W��ñ���i��� */
//#include "/home/bbs/src/include/d_ansi.h"

extern BCACHE *bshm;

/*copied from post.c*/

void 
add_post (  /* �o���ݪO */
    char *brdname,        /* �� post ���ݪO */
    char *fpath,          /* �ɮ׸��| */
    char *title,          /* �峹���D */
    char *authurid,       /* �@�̱b�� */
    char *authurnick,     /* �@�̼ʺ� */
    int xmode
)
{
  HDR hdr;
  char folder[64];

  brd_fpath(folder, brdname, ".DIR");
  hdr_stamp(folder, HDR_COPY | 'A', &hdr, fpath);
  /*�W�߫���*/

  printf("%s\n",folder);
  strcpy(hdr.owner, authurid);

  strcpy(hdr.nick, authurnick);

  strcpy(hdr.title, title);

  hdr.xmode = xmode;

  rec_bot(folder, &hdr, sizeof(HDR));
}



int main(int argc, char * argv[]) {

        FILE *fp,*fp2;
        char fpath[500];
        char buf[1000];

        /* �ǤJ�Ѽ�

            1. �O�W
            2. �ɦW
            3. ���D
            4. �@��
            5. �ʺ�

        */

strcpy(fpath,argv[2]);

        if (argc<6)
        {
          /* �Ѽƿ��~�A��ܻ��� */
          printf("Add-Post: �ѼƶǤJ���~�A�ΨS���ǤJ�ѼơC\n");
          printf("�Ϊk�Gaddpost [�Ѽ�]\n");
          printf("�ѼơG 1-�O�W  2-�ɦW  3-���D  4-�@��  5-�ʺ�  [6-�ﶵ]\n");
          printf(" (�ﶵ�G -i �w�R�Ҧ�(����X�T����stdout) -h �۰ʥ[�W���Y) -ih �� -hi ��̨å�\n\n");
          printf("Add-Post: �פ�\n\n");
          exit(0);
        }


        /* ��ܩʤ޼� */
        if (argv[6]!=NULL)
        {
          printf("@ �ﶵ=%s\n",argv[6]);
          if (!strcmp(argv[6],"-h")||!strcmp(argv[6],"-i")||!strcmp(argv[6],"-hi")||!strcmp(argv[6],"-ih"))
          {
            if (!strcmp(argv[6],"-h")||!strcmp(argv[6],"-hi")||!strcmp(argv[6],"-ih"))
            {
                /* �۰ʥ[�W���Y�A�YBBS�峹�}�Y���T��  */
                printf("@ h(�۰ʥ[�W���Y)=ON\n");

/* �Ȧs�ɸ��| */

if (fp = fopen(TMP_PATH, "w"))
  {

    /* �峹���Y */
    /* �Ъ`�N�G�U��������r��A�ҥH���n�� addpost.c �ഫ�� utf8�A�]���ثe Maple BBS �u�Y Big5 */
    fprintf(fp, "�@��: %s (%s)\n",
     argv[4], argv[5]);
    fprintf(fp, "���D: %s\n�ɶ�: %s\n\n", argv[3], Now());


    /* �峹���e */

    fp2 = fopen(fpath,"r");

    while (fgets(buf,1000,fp2)!=NULL) {
        fprintf(fp,"%s\n",buf);
    }

    /* ��ñ */
    fprintf(fp,"\n");

//    fprintf(fp,ebs[0],"[SVN_Logger]","localhost");

    fclose(fp);
    fclose(fp2);
  }


            }
            if (!strcmp(argv[6],"-i")||!strcmp(argv[6],"-hi")||!strcmp(argv[6],"-ih"))
            {
                /* Slient Mode */
                /* floatJ.090302: �d�@�U silent tty ���]�w */
                printf("@ i(Silent)=ON\n");
            }

          }
        }


        /* �L�X�򥻸�� */
        printf("- ���|=%s\n",argv[0]);
        printf("# �O�W=%s\n",argv[1]);
        printf("# �ɦW=%s\n",argv[2]);
        printf("# ���D=%s\n",argv[3]);
        printf("# �@��=%s\n",argv[4]);
        printf("# �ʺ�=%s\n",argv[5]);
        printf("$ �Ȧs=%s\n",TMP_PATH);

        /* �}�l�o��峹 */
        chdir("/home/bbs/");


        add_post(argv[1],TMP_PATH, argv[3], argv[4], argv[5], 0);

        printf("Add-Post: �峹�o����.\n\n");

   return;
}

