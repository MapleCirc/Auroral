//dust.110924 XXX: NEED TO MODIFY


#include "bbs.h"
extern BCACHE *bshm;

/*copied from post.c*/

void
add_post(brdname, fpath, title, authurid, authurnick, xmode)  /* �o���ݪO */
  char *brdname;        /* �� post ���ݪO */
  char *fpath;          /* �ɮ׸��| */
  char *title;          /* �峹���D */
  char *authurid;       /* �@�̱b�� */
  char *authurnick;     /* �@�̼ʺ� */
  int xmode;
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

int main() {
	FILE *fp,*fp2;
	char fpath[200],cmd[300];
	char buf[1000];
	char title[200];
	int version,last;

        chdir("/home/bbs/src");

	system("svn log -r HEAD > /home/bbs/etc/svntmp.txt");
	
	fp = fopen("/home/bbs/etc/svntmp.txt","r");

	fscanf(fp,"%*s%*c%*c%d",&version);
	printf("v:%d\n",version);
	fclose(fp);

	fp = fopen("/home/bbs/etc/svnrec.txt","r");
	fscanf(fp,"%d",&last);
	fclose(fp);

	if (version > last) {
	        fp = fopen("/home/bbs/etc/svnrec.txt","w");
	        fprintf(fp,"%d",version);
	        fclose(fp);
		sprintf(cmd,"svn diff -r %d:%d > /home/bbs/etc/svntmp.txt",version-1,version);
		system(cmd);
	

	strcpy(fpath,"/home/bbs/etc/svnlog.txt");
  if (fp = fopen(fpath, "w"))
  {
    sprintf(title, "TCIRC SVN -> Revision %d",version);

    /* �峹���Y */

    fprintf(fp, "�@��: %s (%s)\n",
     "[SVN_Logger]", "");
    fprintf(fp, "���D: %s\n�ɶ�: %s\n\n", title, Now());

    /* �峹���e */

    fp2 = fopen("/home/bbs/etc/svntmp.txt","r");

    while (fgets(buf,1000,fp2)!=NULL) {
	fprintf(fp,"%s\n",buf);
    }

    fprintf(fp,"\n");
    fprintf(fp,ebs[0],"[SVN_Logger]","localhost");
    fclose(fp);
    fclose(fp2);
  }
    chdir(BBSHOME);
    add_post("SVN_log", "/home/bbs/etc/svnlog.txt", title, "[SVN_Logger]","", 0);
  }
   return 0;
}
