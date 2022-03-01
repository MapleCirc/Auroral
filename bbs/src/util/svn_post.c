//dust.110924 XXX: NEED TO MODIFY


#include "bbs.h"
extern BCACHE *bshm;

/*copied from post.c*/

void 
add_post (  /* 發文到看板 */
    char *brdname,        /* 欲 post 的看板 */
    char *fpath,          /* 檔案路徑 */
    char *title,          /* 文章標題 */
    char *authurid,       /* 作者帳號 */
    char *authurnick,     /* 作者暱稱 */
    int xmode
)
{
  HDR hdr;
  char folder[64];

  brd_fpath(folder, brdname, ".DIR");
  hdr_stamp(folder, HDR_COPY | 'A', &hdr, fpath);
  /*獨立建檔*/

  printf("%s\n",folder);
  strcpy(hdr.owner, authurid);

  strcpy(hdr.nick, authurnick);

  strcpy(hdr.title, title);

  hdr.xmode = xmode;

  rec_bot(folder, &hdr, sizeof(HDR));
}

int 
main (void) {
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

    /* 文章檔頭 */

    fprintf(fp, "作者: %s (%s)\n",
     "[SVN_Logger]", "");
    fprintf(fp, "標題: %s\n時間: %s\n\n", title, Now());

    /* 文章內容 */

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
