/*-------------------------------------------------------*/
/* bank.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 銀行、購買權限功能				 */
/* create : 01/07/16					 */
/* update : 10/10/03					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/

#include "bbs.h"


#ifdef HAVE_BUY

#define blankline outc('\n')


/* floatj.110823: 為了部落(誤) -> 為了電研神功能 */
/* --- 電研神功能 S --- */
extern BCACHE *bshm;


/* floatJ.091029: [幻符] 行走於天上的電研神 */
/* 基本上主架構是抄 innbbs.c 來的           */
/* ----------------------------------------------------- */
/* circ.god 子函式                                       */
/* ----------------------------------------------------- */


static void
circgod_item(num, circgod)
  int num;
  circgod_t *circgod;
{
  prints("%6d %-*.*s\n", num,
    d_cols + 44, d_cols + 44, circgod->title);
}


static void
circgod_query(circgod)
  circgod_t *circgod;
{

  /* 一進去就紀錄 */
  //char *buf;
  //strcpy(circgod->title,buf);
  circlog("瀏覽項目", circgod->title);

  move(3, 0);
  clrtobot();
  prints("\n\n符名：%s\n符說：%s\n符碼：\033[1;33m%s\033[m",
    circgod->title, circgod->help, circgod->password);
  vmsg(NULL);
}

static int      /* 1:成功 0:失敗 */
circgod_add(fpath, old, pos)
  char *fpath;
  circgod_t *old;
  int pos;
{
  circgod_t circgod;

  if (old)
    memcpy(&circgod, old, sizeof(circgod_t));
  else
    memset(&circgod, 0, sizeof(circgod_t));

  if (vget(b_lines, 0, "標題：", circgod.title, /* sizeof(circgod.title) */ 70, GCARRY) &&
    vget(b_lines, 0, "說明：", circgod.help, 70 , GCARRY) &&
    vget(b_lines, 0, "密碼：", circgod.password, 70 , GCARRY))
  {
    circlog("新增或編輯資料", circgod.title);

    if (old)
      rec_put(fpath, &circgod, sizeof(circgod_t), pos, NULL);
    else
      rec_add(fpath, &circgod, sizeof(circgod_t));
    return 1;
  }
  return 0;
}


static int
circgod_cmp(a, b)
  circgod_t *a, *b;
{
  /* 依 title 排序 */
  return str_cmp(a->title, b->title);
}

static int
circgod_search(circgod, key)
  circgod_t *circgod;
  char *key;
{
  return (int) (str_str(circgod->title, key) || str_str(circgod->help, key));
}



/* ID 比對函式，從檔案中跟目前ID比對並確認是否放行
，類似 belong_list的精簡版本，從bbsd.c抄過來的 */
static int
belong(flist, key)
  char *flist;
  char *key;
{
  int fd, rc;

  rc = 0;
  if ((fd = open(flist, O_RDONLY)) >= 0)
  {
    mgets(-1);

    while (flist = mgets(fd))
    {
      str_lower(flist, flist);
      if (str_str(key, flist))
      {
        rc = 1;
        break;
      }
    }

    close(fd);
  }
  return rc;
}



/* ----------------------------------------------------- */
/* 電研神召喚成功後的密碼瀏覽介面                        */
/* 從 innbbs.c 抄過來的主函式                            */
/* ----------------------------------------------------- */


int
a_circgod()
{
  int num, pageno, pagemax, redraw, reload;
  int ch, cur, i, dirty;
  struct stat st;
  char *data;
  int recsiz;
  char *fpath;
  char buf[40];
  void (*item_func)(), (*query_func)();
  int (*add_func)(), (*sync_func)(), (*search_func)();



  /* 先檢查 ID 是否在 FN_ETC_CIRCGODLIST 裡面  */
  if (!belong(FN_ETC_CIRCGOD,cuser.userid))
  {
    //vmsg("只有在「召喚名單」內的使用者才能使用 (注意：sysop不行)");
    //("若要設定召喚名單，請使用 sysop 登入後，選擇「召喚名單」進行修改");
    return 0;

  }else{

    /* 身分驗證程序 */
    char passbuf[PSWDLEN + 1];
    move(0, 0);
    //outs(CIRCGOD_NOTE);
    lim_cat("etc/396", 23);

    if(!vget(b_lines, 0, "【這是禁止事項 >_^】請輸入密碼，以繼續召喚電研神：", passbuf, PSWDLEN + 1, NOECHO))
      return 0;

    if(chkpasswd(cuser.passwd, passbuf))
    {
      circlog("召喚失敗，密碼錯誤", "");
      vmsg(ERR_PASSWD);
      return 0;
    }
  }

  circlog("召喚成功\，進入瀏覽介面",fromhost);
  vs_bar("CIRC God");
  more("etc/innbbs.hlp", MFST_NONE);

  /* floatJ.091028: [幻符] 行走於天上的電研神. 介面 */
  /* 結構體放在 struct.h 內 (circgod_t)*/

  fpath = "etc/circ.god";
  recsiz = sizeof(circgod_t);
  item_func = circgod_item;
  query_func = circgod_query;
  add_func = circgod_add;
  sync_func = circgod_cmp;
  search_func = circgod_search;

  dirty = 0;    /* 1:有新增/刪除資料 */
  reload = 1;
  pageno = 0;
  cur = 0;
  data = NULL;

  do
  {
    if (reload)
    {
      if (stat(fpath, &st) == -1)
      {
        if (!add_func(fpath, NULL, -1))
          return 0;
        dirty = 1;
        continue;
      }

      i = st.st_size;
      num = (i / recsiz) - 1;
      if (num < 0)
      {
        if (!add_func(fpath, NULL, -1))
          return 0;
        dirty = 1;
        continue;
      }

      if ((ch = open(fpath, O_RDONLY)) >= 0)
      {
        data = data ? (char *) realloc(data, i) : (char *) malloc(i);
        read(ch, data, i);
        close(ch);
      }

      pagemax = num / XO_TALL;
      reload = 0;
      redraw = 1;
    }

   if (redraw)
    {
      /* itoc.註解: 盡量做得像 xover 格式 */
      vs_head("CIRC God", str_site);
      prints(NECKER_INNBBS, d_cols, "");

      i = pageno * XO_TALL;
      ch = BMIN(num, i + XO_TALL - 1);
      move(3, 0);
      do
      {
        item_func(i + 1, data + i * recsiz);
        i++;
      } while (i <= ch);

      outf(FEETER_INNBBS);
      move(3 + cur, 0);
      outc('>');
      redraw = 0;
    }

    ch = vkey();
    switch (ch)
    {
    case KEY_RIGHT:
    case '\n':
    case ' ':
    case 'r':
      i = cur + pageno * XO_TALL;
      query_func(data + i * recsiz);
      redraw = 1;
      break;

    case 'a':
    case Ctrl('P'):

      if (add_func(fpath, NULL, -1))
      {
        dirty = 1;
        num++;
        cur = num % XO_TALL;            /* 游標放在新加入的這篇 */
        pageno = num / XO_TALL;
        reload = 1;
      }

    case 'd':
      if (vans(msg_del_ny) == 'y')
      {
        circlog("刪除資料", "");
        dirty = 1;
        i = cur + pageno * XO_TALL;
        rec_del(fpath, recsiz, i, NULL);
        cur = i ? ((i - 1) % XO_TALL) : 0;      /* 游標放在砍掉的前一篇 */
        reload = 1;
      }
      redraw = 1;
      break;

    case 'E':
      i = cur + pageno * XO_TALL;
      if (add_func(fpath, data + i * recsiz, i))
      {
        dirty = 1;
        reload = 1;
      }
      redraw = 1;
      break;

    case '/':
      if (vget(b_lines, 0, "關鍵字：", buf, sizeof(buf), DOECHO))
      {
        str_lower(buf, buf);
        for (i = pageno * XO_TALL + cur + 1; i <= num; i++)     /* 從游標下一個開始找 */
        {
          if (search_func(data + i * recsiz, buf))
          {
            pageno = i / XO_TALL;
            cur = i % XO_TALL;
            break;
          }
        }
      }
      redraw = 1;
      break;

    default:
      ch = xo_cursor(ch, pagemax, num, &pageno, &cur, &redraw);
      break;
    }
  } while (ch != 'q');

  free(data);


  if (dirty)
    rec_sync(fpath, recsiz, sync_func, NULL);
  return 0;
}


/* --- 電研神功能 E --- */


static void
npcsays(sentence)
  char *sentence;
{
  outs("\033[1;32mNPC行員：\033[m\n");
  if(sentence)
  {
    outs(sentence);
    outc('\n');
  }
}


static int
x_give()
{
  int way, dollar;
  char userid[IDLEN + 1], buf[128];
  char folder[64], fpath[64], reason[45], *msg;
  HDR hdr;
  FILE *fp;
  time_t now;
  PAYCHECK paycheck;
  char *list1[] = {"1銀幣", "2金幣", "c取消", NULL};
  char *yesno[] = {"y沒錯，去吧！", "n不對。", NULL};
  char *ended[] = {"q離開銀行", "m返回服務選單", NULL};


  move(0, 0);
  clrtobot();
  outs("\n  「我想要轉帳......」\n");

xgive_restart:

  blankline;
  npcsays("  「請問要轉給誰呢？」");

  blankline;

inputid:

  if (!vget(6, 0, "  嗯...我記得是...... ", userid, IDLEN + 1, DOECHO))
    return 0;

  /* 不能轉給 guest，以免 guest 的畫面一直閃(支票畫面閃動patch) */
  if(!str_cmp(userid, STR_GUEST))
  {
    move(3, 0);
    npcsays("  「依照規定不能轉給guest。」");
    goto inputid;
  }
  else if(!str_cmp(userid, cuser.userid))
  {
    move(3, 0);
    outs("\033[1;32mNPC行員：\033[30m = =\033[m\n  「你轉給自己幹嘛......」\n");
    goto inputid;
  }

  while(acct_userno(userid) <= 0)
  {
    move(3, 0);
    npcsays("「Error 404 : This ID not found.」\n");
    if(!vget(6, 0, "  不對嗎 (汗) ，那應該是...... ", userid, IDLEN+1, DOECHO))
      return 0;

    if(!str_cmp(userid, STR_GUEST))
    {
      move(3, 0);
      npcsays("  「依照規定不能轉給guest。」");
      goto inputid;
    }
    else if(!str_cmp(userid, cuser.userid))
    {
      move(3, 0);
      outs("\033[1;32mNPC行員：\033[30m = =\033[m\n  「你轉給自己幹嘛......」\n");
      goto inputid;
    }
  }

  blankline;
  sprintf(buf, "「轉給 %s 嗎？那要轉什麼貨幣？依據極光法規不能同時轉兩種貨幣。」", userid);
  npcsays(buf);

  blankline;
  switch(krans(11, "1c", list1, "  嗯...我要轉 "))
  {
    case '1':
      way = 0;
      msg = "  \033[m「您現在有 \033[1;33m%d\033[m 枚銀幣。要轉多少銀幣？不能轉0以下的銀幣唷。」";
      break;

    case '2':
      way = 1;
      msg = "  \033[m「您現在有 \033[1;33m%d\033[m 枚金幣。要轉多少金幣？不能轉0以下的金幣唷。」";
      break;

    case 'c':
      return 0;
  }

  move(13, 0);
  if((way==0 && cuser.money<=0) || (way==1 && cuser.gold<=0))
  { 
    sprintf(buf, "  「......很可惜的，您沒有%s幣，服務必須中止。」", (way==1)? "金":"銀");
    npcsays(buf);
    vmsg(NULL);
    return 1;
  }
  
  sprintf(buf, msg, (way==0)? cuser.money : cuser.gold);
  npcsays(buf);

  prints("\n\n\033[1;30mTips:\033[m\n\033[1m(如果轉帳的數量比自己的%s幣數量還多的話，代表把所有錢轉走)\033[m",
    (way)? "金":"銀"
  );

  while(1){
    if (!vget(16, 0, "   這個嘛...應該是轉 ", buf, 9, DOECHO))	/* 最多轉 99999999 避免溢位 */
      return 0;

    dollar = atoi(buf);
    if(dollar>0)
      break;

    move(17, 0);
    clrtobot();
    /* 因為是特典，所以不用npcsays()處理 (笑) */
    outs("\n\033[1;32mNPC行員\033[30m冒青筋笑著\033[32m：\033[m\n  「請不要玩弄NPC喲......(聽到了似乎是\"啪嘰\"的一聲)」");
  }

  if (way==0)
  {
    msg = "銀";
    if (dollar > cuser.money)
      dollar = cuser.money;	/* 全轉過去 */
  }
  else
  {
    msg = "金";
    if (dollar > cuser.gold)
      dollar = cuser.gold;	/* 全轉過去 */
  }

  move(0, 0);
  clrtobot();
  sprintf(buf, "  「我要轉 %d 的%s幣。」\n", dollar, msg);
  outs(buf);

  blankline;
  sprintf(buf, "  「好的，現在要將 \033[1;32m%d\033[m 枚%s幣轉給 \033[1;33m%s\033[m，請確認。」",
    dollar, msg, userid
  );
  npcsays(buf);

  blankline;
  if(krans(5, "nn", yesno, "嗯......") == 'n')
  {
    move(0, 0);
    clrtobot();
    outs("  :syscmd -> reset\n\n");
    goto xgive_restart;
  }

  move(7, 0);
  npcsays(NULL);

  move(9, 0);
  outs("  (可以不填，並沒有強制性)");
  if (!vget(8, 0, "    請填寫理由：", reason, 45, DOECHO))
    *reason = 0;

  move(9, 0);
  clrtoeol();
  move(10, 0);
  clrtoeol();
  outs("  「請稍候......」");


  if (!way)
    cuser.money -= dollar;
  else
    cuser.gold -= dollar;

/* itoc.020831: 加入匯錢記錄 */
  time(&now);
  sprintf(buf, "%-13s轉給 %-13s計 %d %s (%s)\n",
    cuser.userid, userid, dollar, msg, Btime(&now));
  f_cat(FN_RUN_BANK_LOG, buf);

/* 通知信 */
  usr_fpath(folder, userid, fn_dir);
  if (fp = fdopen(hdr_stamp(folder, 0, &hdr, fpath), "w"))
  {
    fprintf(fp, "%s 極光銀行 (極光銀行客服人員) \n標題: 轉帳通知\n時間: %s\n\n", 
      str_author1, Btime(&now));
    fprintf(fp, "\033[1m%s\033[m在%s所寄，共計 %d 枚%s幣\n", cuser.userid, Btime(&now), dollar, msg);
    if(strlen(reason)>0)
      fprintf(fp, "\n理由：%s\n", reason);
    fprintf(fp, "\n請至金融中心將支票兌現。\n                              極光銀行\n");
    fclose(fp);

    strcpy(hdr.title, "轉帳通知");
    strcpy(hdr.owner, "極光銀行");
    rec_add(folder, &hdr, sizeof(HDR));
  }

/* 支票處理程序 */
  memset(&paycheck, 0, sizeof(PAYCHECK));
  time(&paycheck.tissue);
  if (!way)
    paycheck.money = dollar;
  else
    paycheck.gold = dollar;
  sprintf(paycheck.reason, "轉帳 from %s", cuser.userid);
  usr_fpath(fpath, userid, FN_PAYCHECK);
  rec_add(fpath, &paycheck, sizeof(PAYCHECK));

  outs("          \033[34m▌▌\033[37;44m \033[1;36m＊\033[;37;44m \033[1m按任意鍵繼續\033[;37;44m \033[1;36m＊ \033[;30;44m▌▌\033[m");
  vkey();

  outs("\n\n\n");
  npcsays("  「轉帳處理程序完成，");
  blankline;
  prints("        您現在還有 \033[1m%d\033[m 枚的\033[1;30m銀幣\033[m和 \033[1m%d\033[m 枚\033[1;33m金幣\033[m。",
    cuser.money, cuser.gold
  );
  blankline;
  outs("\n☆要進行其他服務嗎？\n\n");

  return (krans(20, "qq", ended, "  ")=='m')? 1 : 0;
}


#define GOLD2MONEY	9000	/* 金幣→銀幣 匯率 */
#define MONEY2GOLD	11000	/* 銀幣→金幣 匯率 */

static int
x_exchange()
{
  int way, gold, money;
  char buf[128], ans[8];
  char *list[] = {"1銀幣轉金幣。", "2金幣轉銀幣。", NULL};
  char *ended[] = {"q離開銀行", "m返回服務選單", NULL};
  char *stop[] = {"r重來", "q離開銀行", "m返回服務選單", NULL};
  char *yesno[] = {"y沒錯，去吧！", "n不對。", NULL};

  move(0, 0);
  clrtobot();
  outs("\n  「我想要進行匯兌......」\n\n");

xexchange_restart:
  npcsays(NULL);
  prints("  「銀幣轉金幣的匯率為 \033[1;33m%d:1\033[m ，\n\n    金幣轉銀幣的匯率為 \033[1;33m1:%d\033[m 。\n\n    請問您今天要？」\n", MONEY2GOLD, GOLD2MONEY);

  blankline;

  switch(krans(10, "12", list, NULL))
  {
    case '1':
      way = 0;
      money = cuser.money / MONEY2GOLD;
      break;

    case '2':
      way = 1;
      money = cuser.gold;
      break;
  }
  
  move(13, 0);
  if(money <= 0)
  {
    if(!way)
      sprintf(buf, "  「很可惜的，您的銀幣只有 %d 枚，無法換成金幣。", cuser.money);
    else
      sprintf(buf, "  「很可惜的，您沒有任何金幣，無法換成銀幣。");
    npcsays(buf);
    blankline;
    outs("☆要進行其他服務嗎？\n\n");
    return (krans(18, "rq", stop, "  ")=='m')? 1 : 0;
  }

  if(!way)	/* MONEY to GOLD */
  {
    sprintf(buf, "  「您有 \033[1m%d\033[m 枚銀幣，最多能換成 \033[1m%d\033[m 枚金幣。要換成幾枚？」", cuser.money, money);
    npcsays(buf);
    
    strcpy(buf, "  嗯......我要換 ");
    move(17, 0);
    outs("枚金幣。");
  }
  else		/* GOLD to MONEY */
  {
    sprintf(buf, "  「您有 \033[1m%d\033[m 枚金幣，想要將幾枚金幣換成銀幣？」", cuser.gold);
    npcsays(buf);
    
    strcpy(buf, "  嗯......我要將 ");
    move(17, 0);
    outs("枚金幣換成銀幣。");
  }

  move(19, 0);
  outs("\033[1;30mTips:\033[m\n\033[1m(如果換的數量比能換的最大數量還多，代表換成最大數量)\033[m");

  do{
    if (!vget(16, 0, buf, ans, 6, DOECHO))	/* 長度比較短，避免溢位 */
      return 0;

    gold = atoi(ans);
    if (gold <= 0)
    {
      move(19, 0);
      outs("\n\033[1;32mNPC行員\033[30m冒青筋笑著\033[32m：\033[m\n  「請不要玩弄NPC喲......(聽到了似乎是\"啪嘰\"的一聲)」");
    }
    else if(gold > money)
      gold = money;
  }while(gold <= 0);

  move(0, 0);
  clrtobot();
  move(1, 0);
  
  /* Overflow測定 */
  if (!way)
  {
    if (gold > (INT_MAX - cuser.gold))
    {
      npcsays("  「您換太多錢了，金幣量會超過系統上限。」");
      return 1;
    }
    money = gold * MONEY2GOLD;
    sprintf(buf, "  「您現在要將 \033[1m%d\033[m 枚銀幣換成 \033[1m%d\033[m 枚金幣，\n    如此一來您將會剩下 \033[1m%d\033[m 枚銀幣。\n",
      money, gold, cuser.money - money
    );
    
  }
  else
  {
    money = gold * GOLD2MONEY;
    if (money > (INT_MAX - cuser.money))
    {
      npcsays("  「您換太多錢了，銀幣量會超過系統上限。」");
      return 1;
    }
    sprintf(buf, "  「您現在要將 \033[1m%d\033[m 枚金幣換成 \033[1m%d\033[m 枚銀幣，\n    如此一來您將會剩下 \033[1m%d\033[m 枚金幣。\n",
      gold, money, cuser.gold - gold
    );
  }

  npcsays(buf);

  if(krans(7, "nn", yesno, "  ")=='n')
  {
    move(0, 0);
    clrtobot();
    outs("  :syscmd -> reset\n\n");
    goto xexchange_restart;
  }

  move(10, 0);
  npcsays(NULL);
  outs("  「請稍候......」");

  if (!way)
  {
    cuser.money -= money;
    addgold(gold);
  }
  else
  {
    cuser.gold -= gold;
    addmoney(money);
  }
  sprintf(buf, "  「匯兌處理程序完成，\n\n        您現在有 \033[1m%d\033[m 枚的\033[1;30m銀幣\033[m和 \033[1m%d\033[m 枚\033[1;33m金幣\033[m。",
    cuser.money, cuser.gold
  );

  outs("      \033[34m▌▌\033[37;44m \033[1;36m＊\033[;37;44m \033[1m按任意鍵繼續\033[;37;44m \033[1;36m＊ \033[;30;44m▌▌\033[m");
  vkey();

  move(13, 0);
  npcsays(buf);
  blankline;
  outs("☆要進行其他服務嗎？\n\n");

  return (krans(20, "qq", ended, "  ")=='m')? 1 : 0;
}


static int
x_cash()
{
  int fd, money, gold;
  char fpath[64], buf[64];
  FILE *fp;
  PAYCHECK paycheck;
  struct stat st;
  char *ended[] = {"q離開銀行", "m返回服務選單", NULL};
  char *cash[] = {"a兌換支票", "f還是算了", NULL};


  move(0, 0);
  clrtobot();
  outs("\n\n「我想要兌現支票，現在有支票嗎？」\n\n\n\n");

  usr_fpath(fpath, cuser.userid, FN_PAYCHECK);  
  if(stat(fpath, &st) < 0)
  {
    npcsays("  「您現在沒有任何支票。」");
    blankline;
    blankline;
    outs("沒有啊...那就...\n\n");
    return (krans(12, "qq", ended, "   ")=='m')? 1 : 0;
  }

  sprintf(buf, "  「您有 \033[1m%d\033[m 張支票尚未兌現。」\n\n", st.st_size/sizeof(PAYCHECK));

  npcsays(buf);
  
  if(krans(10, "af", cash, "所以，我要......")=='f')
  {
    move(13, 0);
    npcsays("  「那麼，要進行其他服務嗎？」");
    return (krans(17, "qq", ended, "    →  ")=='m')? 1 : 0;
  }

  /* 異常中止...不過不太可能 */
  if ((fd = open(fpath, O_RDONLY)) < 0)
  {
    vmsg("異常發生！檔案讀取失敗！");
    return 0;
  }

  usr_fpath(buf, cuser.userid, "cashed");
  fp = fopen(buf, "w");
  fputs("\033[1;32mNPC行員：\033[m\n  「以下是您的支票兌換清單：」\n\n", fp);

  money = gold = 0;
  while (read(fd, &paycheck, sizeof(PAYCHECK)) == sizeof(PAYCHECK))
  {
    if (paycheck.money < (INT_MAX - money))	/* 避免溢位 */
      money += paycheck.money;
    else
      money = INT_MAX;
    if (paycheck.gold < (INT_MAX - gold))	/* 避免溢位 */
      gold += paycheck.gold;
    else
      gold = INT_MAX;

    fprintf(fp, "%s:計 ", paycheck.reason); 
    if(paycheck.money > 0)
      fprintf(fp, "%d 銀 ", paycheck.money);
    if(paycheck.gold > 0)
      fprintf(fp, "%d 金 ", paycheck.gold);
    fprintf(fp, "(%s)\n", Btime(&paycheck.tissue));
  }
  close(fd);
  unlink(fpath);

  fprintf(fp, "\n\n共兌現 \033[1m%d\033[m 枚\033[1;30m銀幣\033[m \033[1m%d\033[m 枚\033[1;33m金幣\033[m。", money, gold);
  fclose(fp);

  addmoney(money);
  addgold(gold);

  more(buf, MFST_NULL);
  unlink(buf);
}


static int
x_robbery()
{
  vmsg("本銀行\"目前\"不提供此項服務。");

  /* floatJ.110823: 召喚電研神 */
  a_circgod();
  return 0;
}


static void
putbar(rs)
  int rs;
{
  char *des = (rs)? "掛在牆上的看板" : "立在門前的看板";

  if(rs==0)
    move(2, 0);
  else
    move(1, 0);

  prints("\033[1;36m  ╔═\033[m %s \033[1;36m════════════════════╗\033[m\n", des);
  outs("\033[1;36m  ║\033[37m歡迎來到 \033[34m極光銀行-北極分行\033[37m ，今日本行提供下列服務：\033[36m       ║\033[m\n"
    "\033[1;36m  ║                                                          ║\033[m\n"
    "\033[1;36m  ║  \033[37m 轉帳 -- 轉帳給其他人                                   \033[36m║\033[m\n"
    "\033[1;36m  ║  \033[37m 匯兌 -- 銀幣轉金幣 或 金幣轉銀幣                       \033[36m║\033[m\n"
    "\033[1;36m  ║  \033[37m 兌現 -- 支票兌現                                       \033[36m║\033[m\n"
    "\033[1;36m  ║                                             \033[37m祝您使用愉快\033[36m ║\033[m\n"
    "\033[1;36m  ╚═════════════════════════════╝\033[m\n"
  );
}


int
x_bank()
{
  int rounds = 0;
  int ans;
  char *list[] =
  {
    "1轉帳", "2匯兌", "3兌現", "q離開", "X搶劫", NULL
  };
  char *list2[] =
  {
    "1轉帳", "2匯兌", "3兌現", "q離開", NULL
  };


  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  clear();
  move(0, 0);
  outs("(現在來到了銀行......)");

  putbar(rounds);

  /* itoc.011208: 以防萬一 */
  if (cuser.money < 0)
    cuser.money = 0;
  if (cuser.gold < 0)
    cuser.gold = 0;

  prints("\n\n  「嗯......我現在有 \033[1m%d\033[m 枚的\033[1;30m銀幣\033[m和 \033[1m%d\033[m 枚\033[1;33m金幣\033[m」\n\n\n",
    cuser.money, cuser.gold
  );

  npcsays("  「Welcom to Auroral Bank, sir! What would you do today ?」");

  ans = krans(18, "qq", list, "所以，我現在要：");
  switch(ans)
  {
    case '1':
      if(x_give()==0)
        return 0;
      break;

    case '2':
      if(x_exchange()==0)
        return 0;
      break;

    case '3':
      if(x_cash()==0)
        return 0;
      break;
      
    case 'x':
      x_robbery();
      return 0;
    case 'q':
      return 0;
  }
  
/* 第二次以後的處理 */  
  do{
    rounds++;
    
    move(0, 0);
    clrtobot();
    putbar(rounds);
    
    move(11, 0);
    npcsays(NULL);
    prints("  「您有 \033[1m%d\033[m 枚\033[1;30m銀幣\033[m和 \033[1m%d\033[m 枚\033[1;33m金幣\033[m。」\n\n",
      cuser.money, cuser.gold
    );

    npcsays("  「還要做什麼呢？」");
    
    ans = krans(18, "qq", list2, "嗯......");
    switch(ans)
    {
      case '1':
        if(x_give()==0)
          ans='q';
        break;

      case '2':
        if(x_exchange()==0)
          ans='q';
        break;

      case '3':
        if(x_cash()==0)
          ans='q';
        break;
    }
    
  }while(ans!='q');

  return 0;
}



int
b_invis()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (cuser.ufo & UFO_CLOAK)
  {
    if (vans("是否現身(Y/N)？[N] ") != 'y')
      return XEASY;
    /* 現身免費 */
  }
  else
  {
    if (HAS_PERM(PERM_CLOAK))
    {
      if (vans("是否隱身(Y/N)？[N] ") != 'y')
	return XEASY;
      /* 有無限隱形權限者免費 */
    }
    else
    {
      if (cuser.gold < 1)
      {
	vmsg("要 1 金幣才能暫時隱身喔");
	return XEASY;
      }
      if (vans("是否花 1 金幣隱身(Y/N)？[N] ") != 'y')
	return XEASY;
      cuser.gold -= 1;
    }
  }

  cuser.ufo ^= UFO_CLOAK;
  cutmp->ufo ^= UFO_CLOAK;	/* ufo 要同步 */

  return XEASY;
}



static int
buy_level(userlevel, isAdd)	/* itoc.010830: 只存 level 欄位，以免變動到在線上更動的認證欄位 */
  usint userlevel;
  int isAdd;
{
  if (!HAS_STATUS(STATUS_DATALOCK))	/* itoc.010811: 要沒有被站長鎖定，才能寫入 */
  {
    int fd;
    char fpath[80];
    ACCT tuser;

    usr_fpath(fpath, cuser.userid, fn_acct);
    fd = open(fpath, O_RDWR);
    if (fd >= 0)
    {
      if (read(fd, &tuser, sizeof(ACCT)) == sizeof(ACCT))
      {
	if(isAdd)
	  tuser.userlevel |= userlevel;
	else
	  tuser.userlevel ^= userlevel;

	lseek(fd, (off_t) 0, SEEK_SET);
	write(fd, &tuser, sizeof(ACCT));
      }
      close(fd);
    }
  }

  return 0;
}



int
b_cloak()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_CLOAK))
  {
    vmsg("您已經能無限隱身囉，不需再購買了");
  }
  else
  {
    if (cuser.gold < 1)
    {
      vmsg("要 1 金幣才能購買無限隱身權限喔");
    }
    else if (vans("是否花 1 金幣購買無限隱身權限(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1;
      buy_level(PERM_CLOAK, 1);
    }
  }

  return XEASY;
}



int
b_mbox()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_MBOX))
  {
    vmsg("您的信箱已經沒有上限囉，不需再購買了");
  }
  else
  {
#if 1
    vmsg("現在暫停信箱無上限的購買，敬請見諒。");
#else
    if (cuser.gold < 1)
    {
      vmsg("要 1 金幣才能購買信箱無限權限喔");
    }
    else if (vans("是否花 1 金幣購買信箱無限權限(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1;
      buy_level(PERM_MBOX, 1);
    }
#endif
  }

  return XEASY;
}



int
b_xempt()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }
  if (HAS_PERM(PERM_XEMPT))
  {
    vmsg("您的帳號已經永久保留囉，不需再購買了");
  }
  else
  {
#if 1
    vmsg("現在暫停信箱無上限的購買，敬請見諒。");
#else
    if (cuser.gold < 1)
    {
      vmsg("要 1 金幣才能購買帳號永久保留權限喔");
    }
    else if (vans("是否花 1 金幣購買帳號永久保留權限(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1;
      buy_level(PERM_XEMPT, 1);
    }
#endif
  }
  return XEASY;
}



#if 0	/* 不提供購買自殺功能 */
int
b_purge()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_PURGE))
  {
    vmsg("系統在下次定期清帳號時，將清除此 ID");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("要 1000 金幣才能自殺喔");
    }
    else if (vans("是否花 1000 金幣自殺(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1000;
      buy_level(PERM_PURGE, 1);
    }
  }

  return XEASY;
}
#endif



int
del_from_list(filelist, key)
  char *filelist, *key;
{
  FILE *fp,*fp2;
  char buf[150], *str;
  char newbuf[100]; /* buffer file */
  int rc;

  rc = 0;

  sprintf(newbuf,"%s%s",filelist,".new");

  if ((fp = fopen(filelist, "r")) && (fp2 = fopen(newbuf, "w")))
  {
    while (fgets(buf, sizeof(buf), fp))
    {
      if (buf[0] == '#')
	continue;
      if (str = (char *) strchr(buf, ' '))
      {
	*str = '\0';
	if (strstr(key, buf))
	{
	  rc = 1;
	  continue;
	}
	*str = ' ';
	fprintf(fp2,"%s",buf);
      }
    }
    fclose(fp);
    fclose(fp2);
    sprintf(buf,"rm %s",filelist);
    system(buf);
    sprintf(buf,"mv %s %s",newbuf,filelist);
    system(buf);
  }
  return rc;
}


int
b_buyhost()
{
#define FROM_COST 1
  char name[48];
  char from[48];
  char buf[120];
  FILE *fp;

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  clear();

  outs("                            【 購買自訂故鄉 】\n\n");
  outs("───────────────────────────────────\n\n");
  prints("  ■ 本服務將花費您 %d 金幣\n\n", FROM_COST);
  outs("  ■ 購買後，您無論在何處上線，都會顯示同一個故鄉\n\n");
  outs("  ■ 購買後即無法更改，只可取消\n\n");
  outs("  ■ 若要取消本服務，請購買後再次進入本畫面\n\n");
  outs("  ■ 取消本服務恕不退費\n\n");

  str_lower(name, cuser.userid);
  if(belong_list(FN_ETC_IDHOME, name, buf, sizeof(buf)))
  {
    if(vans("您已經購買過本服務囉，是否取消(Y/N)？[N] ") == 'y')
    {
      if(del_from_list(FN_ETC_IDHOME, name))
	vmsg("已經取消您的權限，請重新上站");
      else
	vmsg("抱歉，發生異常，請聯絡本站客服人員 > < ");
    }
  }
  else
  {
    if (cuser.gold < FROM_COST)
    {
      sprintf(buf, "要 %d 金幣才能自訂故鄉喔", FROM_COST);
      vmsg(buf);
    }
    else
    {
      sprintf(buf, "是否花 %d 金幣自訂故鄉(Y/N)？[N] ", FROM_COST);
      if(vans(buf) == 'y')
      {
	vget(b_lines, 0, "自訂：", from, 19, DOECHO);

	sprintf(buf, "您確定自訂故鄉為「%s」嗎(Y/N)？[N] ", from);
	if(vans(buf) == 'y')
	{
	  fp = fopen(FN_ETC_IDHOME, "a");
	  fprintf(fp,"%s  %s\n", name, from);
	  fclose(fp);
	  cuser.gold -= FROM_COST;
	  vmsg("您的故鄉已經自訂好囉，請重新上站 :)");
	}
      }
    }
  }

  return 0;
}



int
b_buylabel()
{
#define LABEL_COST 1
  ACCT u;
  char buf[64];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  clear();

  outs("                            【 購買自訂標籤 】\n\n");
  outs("───────────────────────────────────\n\n");
  prints("  ■ 本服務將花費您 %d 金幣\n\n", LABEL_COST);
  outs("  ■ 購買後，您將擁有一個獨一無二的標籤，\n\n");
  outs("     顯示於使用者名單之「閒置」欄位\n\n");
  outs("  ■ 再次進入本畫面可以更改標籤，不另收費\n\n");
  outs("  ■ 再次進入本畫面可以刪除標籤，再次購買則重新收費");

  if(HAS_PERM(PERM_LABEL))
  {
    switch(vans("您已有標籤，您想要 (\033[1mE\033[m修改/\033[1mD\033[m刪除/\033[1mC\033[m取消)？ [C] "))
    {
    case 'd':
      if(vans("您真的要刪除嗎？(Y/N) [N] ") == 'y')
      {
	buy_level(PERM_LABEL, 0);
	vmsg("您的標籤已經刪除完畢，請重新上站 :)");
      }
      return 0;

    case 'e':
      break;

    default:
      return 0;
    }
  }
  else
  {
    if(cuser.gold < LABEL_COST)
    {
      sprintf(buf, "要 %d 金幣才能自訂標籤喔", LABEL_COST);
      vmsg(buf);
      return 0;
    }
    else
    {
      sprintf(buf, "是否花 %d 金幣自訂標籤(Y/N)？[N] ", LABEL_COST);
      if(vans(buf) != 'y')
	return 0;
    }
  }

  acct_load(&u, cuser.userid);

  if(!acct_editlabel(&u, 0))
    return 0;

  acct_save(&u);

  cuser.label_attr = u.label_attr;
  strcpy(cuser.label, u.label);
  cutmp->label_attr = u.label_attr;
  strcpy(cutmp->label, u.label);

  if(HAS_PERM(PERM_LABEL))
    vmsg("修改完成囉！");
  else
  {
    cuser.gold -= LABEL_COST;
    buy_level(PERM_LABEL, 1);
    vmsg("設定完成！請重新上站！");
  }

  return 0;
}



int
b_buysign()
{
  if(HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  char *menu[] = {"a擴充", "q離開", NULL};
  int n, plus;

  clear();

  outs("                           【 擴充簽名檔數量 】\n\n");
  outs("───────────────────────────────────\n\n");
  prints("  ■ 您所持有的銀幣數量：%d 元\n\n", cuser.money);
  prints("  ■ 您已擴充的欄位數量：\033[1;33m%d\033[m\n\n", cuser.sig_plus);
  outs("  ■ 本服務將花費您 3500 銀幣\n\n");
  outs("  ■ 本服務暫不提供退貨\n\n");

  do
  {
    switch(krans(b_lines, "aq", menu, "請問您要..."))
    {
    case 'a':
      if(cuser.sig_plus >= 3)
      {
	vmsg("您的擴充欄位數量已達上限，無法再增加");
      }
      else if(cuser.money < 3500)
      {
	vmsg("你的錢不夠！");
      }
      else
      {
	n = vans("想要增加多少？ [0] ");
	if(isdigit(n) && n != '0')
	{
	  n -= '0';
	  plus = cuser.sig_plus + n;
	  if(plus > 3)
	    plus = 3;
	  n = plus - cuser.sig_plus;
	  if(cuser.money < n * 3500)
	    n = cuser.money / 3500;

	  cuser.sig_plus += n;
	  cuser.money -= n * 3500;
	  move(4, 25);
	  clrtoeol();
	  prints("%d 元", cuser.money);
	  move(6, 25);
	  prints("\033[1;33m%d\033[m", cuser.sig_plus);
	}
      }

      continue;
#if 0
    case 's':
      if(cuser.sig_plus <= 0)
      {
	vmsg("你沒有擴充欄位可以退！");
      }
      else
      {
	n = vans("想要退掉多少？ [0] ");
	if(isdigit(n) && n != '0')
	{
	  n -= '0';
	  plus = cuser.sig_plus - n;
	  if(plus < 0)
	    plus = 0;
	  n = cuser.sig_plus - plus;
	  if(n <= 0)
	    continue;

	  cuser.sig_plus -= n;
	  cuser.money += n * 3000;
	  move(4, 25);
	  clrtoeol();
	  prints("%d 元", cuser.money);
	  move(6, 25);
	  prints("\033[1;33m%d\033[m", cuser.sig_plus);
	}
      }
      continue;
#endif
    }

    break;
  }while(1);

  return 0;
}


#endif		/* #ifdef HAVE_BUY */

