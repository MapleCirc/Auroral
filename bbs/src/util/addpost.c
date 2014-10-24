/*-------------------------------------------------------*/
/* util/addpost.c        ( Maple-itoc tcirc bbs )        */
/*-------------------------------------------------------*/
/* target : 文章發表呼叫用API                            */
/* author : pioneerLike (Circ25) , modified by floatJ    */
/* create : 2009/02/27                                   */
/* update : 2009/02/27                                   */
/*-------------------------------------------------------*/


#include "bbs.h"
#define TMP_PATH "/home/bbs/tmp/addpost_buf.tmp"


/* 保留加上站簽的可能性 */
//#include "/home/bbs/src/include/d_ansi.h"

extern BCACHE *bshm;

/*copied from post.c*/

void
add_post(brdname, fpath, title, authurid, authurnick, xmode)  /* 發文到看板 */
  char *brdname;        /* 欲 post 的看板 */
  char *fpath;          /* 檔案路徑 */
  char *title;          /* 文章標題 */
  char *authurid;       /* 作者帳號 */
  char *authurnick;     /* 作者暱稱 */
  int xmode;
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



int main(int argc, char * argv[]) {

        FILE *fp,*fp2;
        char fpath[500];
        char buf[1000];

        /* 傳入參數

            1. 板名
            2. 檔名
            3. 標題
            4. 作者
            5. 暱稱

        */

strcpy(fpath,argv[2]);

        if (argc<6)
        {
          /* 參數錯誤，顯示說明 */
          printf("Add-Post: 參數傳入錯誤，或沒有傳入參數。\n");
          printf("用法：addpost [參數]\n");
          printf("參數： 1-板名  2-檔名  3-標題  4-作者  5-暱稱  [6-選項]\n");
          printf(" (選項： -i 安靜模式(不輸出訊息至stdout) -h 自動加上檔頭) -ih 或 -hi 兩者並用\n\n");
          printf("Add-Post: 終止\n\n");
          exit(0);
        }


        /* 選擇性引數 */
        if (argv[6]!=NULL)
        {
          printf("@ 選項=%s\n",argv[6]);
          if (!strcmp(argv[6],"-h")||!strcmp(argv[6],"-i")||!strcmp(argv[6],"-hi")||!strcmp(argv[6],"-ih"))
          {
            if (!strcmp(argv[6],"-h")||!strcmp(argv[6],"-hi")||!strcmp(argv[6],"-ih"))
            {
                /* 自動加上檔頭，即BBS文章開頭那三行  */
                printf("@ h(自動加上檔頭)=ON\n");

/* 暫存檔路徑 */

if (fp = fopen(TMP_PATH, "w"))
  {

    /* 文章檔頭 */
    /* 請注意：下面有中文字串，所以不要把 addpost.c 轉換為 utf8，因為目前 Maple BBS 只吃 Big5 */
    fprintf(fp, "作者: %s (%s)\n",
     argv[4], argv[5]);
    fprintf(fp, "標題: %s\n時間: %s\n\n", argv[3], Now());


    /* 文章內容 */

    fp2 = fopen(fpath,"r");

    while (fgets(buf,1000,fp2)!=NULL) {
        fprintf(fp,"%s\n",buf);
    }

    /* 站簽 */
    fprintf(fp,"\n");

//    fprintf(fp,ebs[0],"[SVN_Logger]","localhost");

    fclose(fp);
    fclose(fp2);
  }


            }
            if (!strcmp(argv[6],"-i")||!strcmp(argv[6],"-hi")||!strcmp(argv[6],"-ih"))
            {
                /* Slient Mode */
                /* floatJ.090302: 查一下 silent tty 怎麼設定 */
                printf("@ i(Silent)=ON\n");
            }

          }
        }


        /* 印出基本資料 */
        printf("- 路徑=%s\n",argv[0]);
        printf("# 板名=%s\n",argv[1]);
        printf("# 檔名=%s\n",argv[2]);
        printf("# 標題=%s\n",argv[3]);
        printf("# 作者=%s\n",argv[4]);
        printf("# 暱稱=%s\n",argv[5]);
        printf("$ 暫存=%s\n",TMP_PATH);

        /* 開始發表文章 */
        chdir("/home/bbs/");


        add_post(argv[1],TMP_PATH, argv[3], argv[4], argv[5], 0);

        printf("Add-Post: 文章發表完成.\n\n");

   return;
}

