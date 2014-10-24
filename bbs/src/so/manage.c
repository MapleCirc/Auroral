/*-------------------------------------------------------*/
/* manage.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 看板管理				 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"

extern BCACHE *bshm;

#ifdef HAVE_TERMINATOR
/* ----------------------------------------------------- */
/* 站長功能 : 拂楓落葉斬				 */
/* ----------------------------------------------------- */


extern char xo_pool[];


#define MSG_TERMINATOR	"《拂楓落葉斬》"

int
post_terminator(xo)		/* Thor.980521: 終極文章刪除大法 */
  XO *xo;
{
  int mode, type;
  HDR *hdr;
  char keyOwner[80], keyTitle[TTLEN + 1], buf[80];

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_FOOT;

  if(!vget(b_lines, 0, "請輸入密碼，以繼續執行"MSG_TERMINATOR"(正確才會繼續)：",
         buf, PSWDLEN + 1, NOECHO))
    return XO_FOOT;

  if(chkpasswd(cuser.passwd, buf))
  {
    vmsg(ERR_PASSWD);
    return XO_FOOT;
  }

  mode = vans(MSG_TERMINATOR "刪除 (1)本文作者 (2)本文標題 (3)自定？[Q] ") - '0';

  if (mode == 1)
  {
    hdr = (HDR *) xo_pool + (xo->pos - xo->top);
    strcpy(keyOwner, hdr->owner);
  }
  else if (mode == 2)
  {
    hdr = (HDR *) xo_pool + (xo->pos - xo->top);
    strcpy(keyTitle, str_ttl(hdr->title));		/* 拿掉 Re: */
  }
  else if (mode == 3)
  {
    if (!vget(b_lines, 0, "作者：", keyOwner, 73, DOECHO))
      mode ^= 1;
    if (!vget(b_lines, 0, "標題：", keyTitle, TTLEN + 1, DOECHO))
      mode ^= 2;
  }
  else
  {
    return XO_FOOT;
  }

  type = vans(MSG_TERMINATOR "刪除 (1)轉信板 (2)非轉信板 (3)所有看板？[Q] ");
  if (type < '1' || type > '3')
    return XO_FOOT;

  sprintf(buf, "刪除%s：%.35s 於%s板，確定嗎(Y/N)？[N] ", 
    mode == 1 ? "作者" : mode == 2 ? "標題" : "條件", 
    mode == 1 ? keyOwner : mode == 2 ? keyTitle : "自定", 
    type == '1' ? "轉信" : type == '2' ? "非轉信" : "所有看");

  if (vans(buf) == 'y')
  {
    buf[strlen(buf)-17] = '\0' ;
    strcat(buf, "非常確定嗎(Y/N)~ [N] ");
    if (vans(buf) == 'y')
    {
      BRD *bhdr, *head, *tail;
      char tmpboard[BNLEN + 1];

      /* Thor.980616: 記下 currboard，以便復原 */
      strcpy(tmpboard, currboard);

      head = bhdr = bshm->bcache;
      tail = bhdr + bshm->number;
      do				/* 至少有 note 一板 */
      {
        int fdr, fsize, xmode;
        FILE *fpw;
        char fpath[64], fnew[64], fold[64];
        HDR *hdr;

        xmode = head->battr;
        if ((type == '1' && (xmode & BRD_NOTRAN)) || (type == '2' && !(xmode & BRD_NOTRAN)))
	  continue;

      /* Thor.980616: 更改 currboard，以 cancel post */
        strcpy(currboard, head->brdname);

        sprintf(buf, MSG_TERMINATOR "看板：%s \033[5m...\033[m", currboard);
        outz(buf);
        refresh();

        brd_fpath(fpath, currboard, fn_dir);

        if ((fdr = open(fpath, O_RDONLY)) < 0)
	  continue;

        if (!(fpw = f_new(fpath, fnew)))
        {
	  close(fdr);
	  continue;
        }

        fsize = 0;
        mgets(-1);
        while (hdr = mread(fdr, sizeof(HDR)))
        {
  	  xmode = hdr->xmode;

	  if ((xmode & POST_MARKED) || 
	    ((mode & 1) && strcmp(keyOwner, hdr->owner)) ||
	    ((mode & 2) && strcmp(keyTitle, str_ttl(hdr->title))))
	  {
	    if ((fwrite(hdr, sizeof(HDR), 1, fpw) != 1))
	    {
	      fclose(fpw);
	      close(fdr);
	      goto contWhileOuter;
	    }
	    fsize++;
	  }
	  else
  	  {
	    /* 砍文並連線砍信 */

	    cancel_post(hdr);
	    hdr_fpath(fold, fpath, hdr);
	    unlink(fold);
	  }
        }
        close(fdr);
        fclose(fpw);

        sprintf(fold, "%s.o", fpath);
        rename(fpath, fold);
        if (fsize)
	  rename(fnew, fpath);
        else
  contWhileOuter:
	  unlink(fnew);

        btime_update(brd_bno(currboard));
      } while (++head < tail);

      /* 還原 currboard */
      strcpy(currboard, tmpboard);
      return XO_LOAD;
    }
  }

  return XO_FOOT;
}
#endif	/* HAVE_TERMINATOR */


/* ----------------------------------------------------- */
/* 板主功能 : 修改板名					 */
/* ----------------------------------------------------- */


static int
post_brdtitle(xo)
  XO *xo;
{
  BRD *oldbrd, newbrd;

  oldbrd = bshm->bcache + currbno;
  memcpy(&newbrd, oldbrd, sizeof(BRD));

  /* itoc.註解: 其實呼叫 brd_title(bno) 就可以了，沒差，蠻幹一下好了 :p */
  vget(b_lines, 0, "看板標題：", newbrd.title, BTLEN + 1, GCARRY);

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
    brd_classchange("gem/@/@"CLASS_INIFILE, oldbrd->brdname, &newbrd);
    return XO_HEAD;	/* itoc.011125: 要重繪中文板名 */
  }

  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 板主功能 : 修改發文綱領                               */
/* ----------------------------------------------------- */


static int
post_postlaw_edit(xo)       /* 板主自定文章發表綱領 */
  XO *xo;
{
  int mode;
  char fpath[64];

  mode = vans("文章發表綱領 (D)刪除 (E)修改 (Q)取消？[E] ");

  if (mode != 'q')
  {
    brd_fpath(fpath, currboard, FN_POSTLAW);

    if (mode == 'd')
    {
      if(vans("你確定要刪除自訂的發文綱領喵(Y/N)？[N] ") == 'y')        /*防止誤刪*/
        unlink(fpath);
      return XO_FOOT;
    }

    if (vedit(fpath, 0))      /* Thor.981020: 注意被talk的問題 */
      vmsg(msg_cancel);
    return XO_HEAD;
  }
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 板主功能 : 修改進板畫面				 */
/* ----------------------------------------------------- */


static int
post_memo_edit(xo)
  XO *xo;
{
  int mode;
  char fpath[64];

  mode = vans("進板畫面 (D)刪除 (E)修改 (Q)取消？[E] ");

  if (mode != 'q')
  {
    brd_fpath(fpath, currboard, fn_note);

    if (mode == 'd')
    {
      if(vans("你確定要刪除進板畫面喵~ (Y/N)？[N] ") == 'y')	/*防止誤刪*/
        unlink(fpath);
      return XO_FOOT;
    }

    if (vedit(fpath, 0))	/* Thor.981020: 注意被talk的問題 */
      vmsg(msg_cancel);
    return XO_HEAD;
  }
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 板主功能 : 修改發文類別                               */
/* ----------------------------------------------------- */

#ifdef POST_PREFIX

static int
post_prefix_edit(xo)
  XO *xo;
{
  const char const *default_prefix[PREFIX_MAX] = DEFAULT_PREFIX;
  char *tail_parts[] = {
    "C.改變類別數量", "R.回復成預設值", "Q.離開", NULL
  };

  char prefix[PREFIX_MAX][PREFIX_LEN + 2];	/* n.[prefix]  所以要額外2字元 */
  char *menu[PREFIX_MAX + 5];
  char menu_f[] = "CQ";		/* menu的第一項 */
  char fpath[64], buf[64];

  int i = 0, prefix_num = NUM_PREFIX_DEF;
  FILE *fp;
  time_t last_modify, old_modify;


  if(!(bbstate & STAT_BOARD))
    return XO_NONE;

  brd_fpath(fpath, currboard, "prefix");

  if(fp = fopen(fpath, "r"))
  {
    int newline_num;	/* dust.100926: 相容於極光舊設計 */
    struct stat st;

    stat(fpath, &st);
    last_modify = st.st_mtime;

    fgets(buf, sizeof(buf), fp);
    sscanf(buf, "%d %d", &prefix_num, &newline_num);

    for(; i < prefix_num; i++)
    {
      if(!fgets(buf, sizeof(buf), fp))
	break;
      strtok(buf, "\n");
      sprintf(prefix[i], "%d.%s", (i + 1) % 10, buf);
    }
    fclose(fp);
  }
  else
    last_modify = 0;

  old_modify = last_modify;

  for(; i < prefix_num; i++)	/* 填滿至 prefix_num 個 */
    sprintf(prefix[i], "%d.%s", (i + 1) % 10, default_prefix[i]);

  menu[0] = menu_f;

  if(prefix_num == 0)
    menu_f[0] = 'C';
  else
    menu_f[0] = '1';
  for(i = 1; i <= prefix_num; i++)
    menu[i] = prefix[i - 1];
  for(i = 1; i <= sizeof(tail_parts) / sizeof(tail_parts[0]); i++)
    menu[prefix_num + i] = tail_parts[i - 1];

  do
  {
    move(b_lines, 0);
    attrset(8);
    if(last_modify)
      prints("Last modify: %s\n", Btime(&last_modify));
    else
      outs("Last modify: (none)\n");
    attrset(7);

    i = pans((b_lines - prefix_num - 7) / 2, 20, "文章類別", menu);

    if(i == 'c')		/* 自訂類別總數 */
    {
      int tmp = i = prefix_num;
      do
      {
	if(vget(b_lines, 0, "請輸入新的類別數量 (0 ~ 10)：", buf, 3, DOECHO))
	  tmp = atoi(buf);
      }while(tmp < 0 || tmp > 10);

      menu_f[0] = 'C';

      if(prefix_num == tmp)
	continue;

      sprintf(buf, "確定要將數量改成 %d 個嗎？ (Y/N) [N] ", tmp);
      if(vans(buf) != 'y')
	continue;

      prefix_num = tmp;

      for(; i < prefix_num; i++)	/* 新增出來的用預設值填滿 */
	sprintf(prefix[i], "%d.%s", (i + 1) % 10, default_prefix[i]);

      if(prefix_num > 0)
	menu_f[0] = '1';

      /* 更新Menu */
      for(i = 1; i <= prefix_num; i++)
	menu[i] = prefix[i - 1];
      for(i = 1; i <= sizeof(tail_parts) / sizeof(tail_parts[0]); i++)
	menu[prefix_num + i] = tail_parts[i - 1];

      time(&last_modify);
    }
    else if(i == 'r')		/* 回復成預設值 */
    {
       menu_f[0] = 'R';
#if 1
      if(vans("你確定要洗白白喵？(Y/N) [N] ") != 'y')
#else
      if(vans("確定要回復成預設值嗎？(Y/N) [N] ") != 'y')
#endif
	continue;

      /* 更新prefix[] */
      for(i = 0; i < prefix_num; i++)
	strcpy(prefix[i] + 2, default_prefix[i]);

      time(&last_modify);
    }
    else if(isdigit(i))
    {
      menu_f[0] = i;
      if(i == '0')
	i = 9;
      else
	i -= '1';

      strcpy(buf, prefix[i] + 2);
      if(vget(b_lines, 0, "類別：", buf, PREFIX_LEN, GCARRY))
      {
	if(strcmp(prefix[i] + 2, buf))
	{
	  strcpy(prefix[i] + 2, buf);
	  time(&last_modify);
	}
      }
    }

  }while (i != 'q');

  if(old_modify != last_modify)		/* 將更動過的prefix檔寫入硬碟 */
  {
    if(fp = fopen(fpath, "w"))
    {
      fprintf(fp, "%d\n", prefix_num);
      for (i = 0; i < prefix_num; i++)
	fprintf(fp, "%s\n", prefix[i] + 2);

      fclose(fp);
    }
  }

  return XO_FOOT;
}

#endif      /* POST_PREFIX */


#ifdef HAVE_TEMPLATE

/* ----------------------------------------------------- */
/* 板主功能 : 修改文章範本                               */
/* ----------------------------------------------------- */

static int
post_template_edit(xo)
  XO *xo;
{
  const char const *default_prefix[PREFIX_MAX] = DEFAULT_PREFIX;
  char *tp_menu[] = {"e編輯", "d刪除", "q取消", NULL};

  char prefix[PREFIX_MAX][PREFIX_LEN + 2];	/* _.[prefix]  所以要額外2字元 */
  char *menu[PREFIX_MAX + 3];
  char menu_f[] = "EE";			/* menu的第一項 */
  char fpath[64], tpName[32], buf[32];

  int i, prefix_num = NUM_PREFIX_DEF;
  FILE *fp;
  screen_backup_t old_scr;


  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  brd_fpath(fpath, currboard, "prefix");

  i = 0;

  if (fp = fopen(fpath, "r"))
  {
    int nouse;
    fscanf(fp, "%d %d\n", &prefix_num, &nouse);
    
    for (; i < prefix_num; i++)
    {
      if (!fgets(buf, PREFIX_LEN + 1, fp))
        break;
      strcat(buf, "\n");
      *(strstr(buf, "\n")) = '\0';
      sprintf(prefix[i], "%d.%s", (i+1)%10, buf);
    }
    fclose(fp);
  }
  else
  {
    prefix_num = 0;
  }

  /* 若數量不對則填滿至 prefix_num 個 */
  for (; i < prefix_num; i++)
    sprintf(prefix[i], "%d.%s", (i+1)%10, default_prefix[i]);

  if(prefix_num > 0) menu_f[0] = '1';

  menu[0] = menu_f;
  for (i = 1; i <= prefix_num; i++)
    menu[i] = prefix[i - 1];
  menu[prefix_num + 1] = "E.離開";
  menu[prefix_num + 2] = NULL;

  scr_dump(&old_scr);

  do
  {
    i = pans((b_lines - prefix_num - 7) / 2, 20, "選擇要編輯的範本", menu);

    if(isdigit(i))
    {
      menu[0][0] = i;

      switch(krans(b_lines, "eq", tp_menu, "你現在要..."))
      {

      case 'e':
	/* 編輯範本 */
	sprintf(tpName, "template.%d", i - '0');
	brd_fpath(fpath, currboard, tpName);

	if(vedit(fpath, 0))
	  vmsg(msg_cancel);

	scr_restore(&old_scr);	/* edit時畫面會改變 */
	scr_dump(&old_scr);

	break;

      case 'd':
	/* 刪除範本 */
	if(vans(MSG_SURE_NY) == 'y')
	{
	  sprintf(tpName, "template.%d", i - '0');
	  brd_fpath(fpath, currboard, tpName);
	  unlink(fpath);
	}
	break;

      case 'q':
	break;

      }

      move(b_lines, 0);
      clrtoeol();
    }

  } while (i != 'e');


  scr_restore(&old_scr);

  return XO_FOOT;
}

#endif


/* ----------------------------------------------------- */
/* 板主功能 : 看板屬性					 */
/* ----------------------------------------------------- */


#ifdef HAVE_SCORE
static int
post_battr_noscore(xo)
  XO *xo;
{
  BRD *oldbrd, newbrd;

  oldbrd = bshm->bcache + currbno;
  memcpy(&newbrd, oldbrd, sizeof(BRD));

  switch (vans("開放評分 (1)允許\ (2)不許\ (Q)取消？[Q] "))
  {
  case '1':
    newbrd.battr &= ~BRD_NOSCORE;
    break;
  case '2':
    newbrd.battr |= BRD_NOSCORE;
    break;
  default:
    return XO_FOOT;
  }

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
    currbattr = newbrd.battr;	/* dust.090322: 即時更新現在看板的旗標 */
  }

  return XO_FOOT;
}
#endif	/* HAVE_SCORE */


/* ----------------------------------------------------- */
/* 板主功能 : 修改板主名單				 */
/* ----------------------------------------------------- */


static int
post_changeBM(xo)
  XO *xo;
{
  char buf[80], userid[IDLEN + 2], *blist;
  BRD *oldbrd, newbrd;
  ACCT acct;
  int BMlen, len;

  oldbrd = bshm->bcache + currbno;

  blist = oldbrd->BM;
  if (is_bm(blist, cuser.userid) != 1)	/* 只有正板主可以設定板主名單 */
    return XO_FOOT;

  memcpy(&newbrd, oldbrd, sizeof(BRD));

  move(3, 0);
  clrtobot();

  move(8, 0);
  prints("目前板主為 %s\n請輸入新的板主名單，或按 [Return] 不改", oldbrd->BM);

  strcpy(buf, oldbrd->BM);
  BMlen = strlen(buf);

  while (vget(10, 0, "請輸入副板主，結束請按 Enter，清掉所有副板主請打「無」：", userid, IDLEN + 1, DOECHO))
  {
    if (!strcmp(userid, "無"))
    {
      strcpy(buf, cuser.userid);
      BMlen = strlen(buf);
    }
    else if (is_bm(buf, userid))	/* 刪除舊有的板主 */
    {
      len = strlen(userid);
      if (!str_cmp(cuser.userid, userid))
      {
	vmsg("不可以將自己移出板主名單");
	continue;
      }
      else if (!str_cmp(buf + BMlen - len, userid))	/* 名單上最後一位，ID 後面不接 '/' */
      {
	buf[BMlen - len - 1] = '\0';			/* 刪除 ID 及前面的 '/' */
	len++;
      }
      else						/* ID 後面會接 '/' */
      {
	str_lower(userid, userid);
	strcat(userid, "/");
	len++;
	blist = str_str(buf, userid);
	strcpy(blist, blist + len);
      }
      BMlen -= len;
    }
    else if (acct_load(&acct, userid) >= 0 && !is_bm(buf, userid))	/* 輸入新板主 */
    {
      len = strlen(userid) + 1;	/* '/' + userid */
      if (BMlen + len > BMLEN)
      {
	vmsg("板主名單過長，不能將這 ID 設為板主");
	continue;
      }
      sprintf(buf + BMlen, "/%s", acct.userid);
      BMlen += len;

      acct_setperm(&acct, PERM_BM, 0);
    }
    else
      continue;

    move(8, 0);
    prints("目前板主為 %s", buf);
    clrtoeol();
  }
  strcpy(newbrd.BM, buf);

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);

    sprintf(currBM, "板主:%s", newbrd.BM);
    return XO_HEAD;	/* 要重繪檔頭的板主 */
  }

  return XO_BODY;
}


#ifdef HAVE_MODERATED_BOARD
/* ----------------------------------------------------- */
/* 板主功能 : 看板權限					 */
/* ----------------------------------------------------- */


static int
post_brdlevel(xo)
  XO *xo;
{
  BRD *oldbrd, newbrd;

  oldbrd = bshm->bcache + currbno;
  memcpy(&newbrd, oldbrd, sizeof(BRD));

  switch (vans("1)公開看板 2)秘密看板 3)好友看板？[Q] "))
  {
  case '1':				/* 公開看板 */
    newbrd.readlevel = 0;
    newbrd.postlevel = PERM_POST;
    newbrd.battr &= ~(BRD_NOSTAT | BRD_NOVOTE);
    break;

  case '2':				/* 秘密看板 */
    newbrd.readlevel = PERM_SYSOP;
    newbrd.postlevel = 0;
    newbrd.battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;

  case '3':				/* 好友看板 */
    newbrd.readlevel = PERM_BOARD;
    newbrd.postlevel = 0;
    newbrd.battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;

  default:
    return XO_FOOT;
  }

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
  }

  return XO_FOOT;
}
#endif	/* HAVE_MODERATED_BOARD */


#ifdef HAVE_MODERATED_BOARD
/* ----------------------------------------------------- */
/* 板友名單：moderated board				 */
/* ----------------------------------------------------- */


static void
bpal_cache(fpath, op)
  char *fpath;
  int op;
{
  BPAL *bpal;
  bpal = bshm->pcache + currbno;

  switch(op)
  {
  case PALTYPE_BPAL:
    bpal->pal_max = image_pal(fpath, bpal->pal_spool);
    break;
  case PALTYPE_BWLIST:
    bpal->bwlist_max = image_pal(fpath, bpal->bwlist_pool);
    break;
  case PALTYPE_BVLIST:
    bpal->bvlist_max = image_pal(fpath, bpal->bvlist_pool);
    break;
  }
}


extern XZ xz[];


static int
XoBM(xo)
  XO *xo;
{
  XO *xt;
  char fpath[64];

  brd_fpath(fpath, currboard, fn_pal);
  xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
  xt->key = PALTYPE_BPAL;
  xover(XZ_PAL);		/* Thor: 進xover前, pal_xo 一定要 ready */

  /* build userno image to speed up, maybe upgreade to shm */

  bpal_cache(fpath, PALTYPE_BPAL);

  free(xt);

  return XO_INIT;
}


/* dust.090315: 水桶名單功能 */
static int
XoBW(xo)
  XO *xo;
{
  XO *xt;
  char fpath[64];

  brd_fpath(fpath, currboard, FN_BWLIST);
  xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
  xt->key = PALTYPE_BWLIST;
  xover(XZ_PAL);

  /* build userno image to speed up */
  bpal_cache(fpath, PALTYPE_BWLIST);

  free(xt);

  return XO_INIT;
}


/* floatJ.091017: 板助名單功能 */
static int
XoBV(xo)
  XO *xo;
{
  XO *xt;
  char fpath[64];

  char *blist;
  BRD *oldbrd;

  oldbrd = bshm->bcache + currbno;
  blist = oldbrd->BM;

  if (is_bm(blist, cuser.userid) != 1)  /* 只有正板主可以設定板助名單 */
    return XO_FOOT;

  brd_fpath(fpath, currboard, FN_BVLIST);
  xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
  xt->key = PALTYPE_BVLIST;
  xover(XZ_PAL);

  /* build userno image to speed up */
  bpal_cache(fpath, PALTYPE_BVLIST);

  free(xt);

  return XO_INIT;
}
#endif	/* HAVE_MODERATED_BOARD */


static int
post_usies(xo)
  XO *xo;
{
  char fpath[64];

  brd_fpath(fpath, currboard, "usies");
  if (more(fpath, MFST_NORMAL) >= 0 && vans("請問要刪除這些看板閱\讀記錄嗎(Y/N)？[N] ") == 'y')
    unlink(fpath);

  return XO_HEAD;
}



int
post_binfo(xo)		/* dust.090321: 顯示看板資訊 */
  XO *xo;
{
  BRD *currbrd, newbrd;
  BPAL *bpal;
  int quit, ret;

  char *blist;
  BRD *oldbrd;

  oldbrd = bshm->bcache + currbno;
  blist = oldbrd->BM;


  currbrd = bshm->bcache + currbno;
  bpal = bshm->pcache + currbno;
  memcpy(&newbrd, currbrd, sizeof(BRD));

  currbattr = currbrd->battr;		/* 強迫更新currbattr */


  move(8, 0);
  clrtobot();

  outs("\033[m\033[1;34m▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁\033[m\n");

  if(!(bbstate & STAT_BOARD))
    outs("\033[1;33;44m★\033[37m   以下是您查詢的看板資訊                                                   \033[m\n");
  else
    outs("\033[1;33;44m★\033[37m   以下是主人您查詢的看板資訊 >/////<                                       \033[m\n");

  prints("\n\033[1m看板名稱\033[m：%-16s\033[1m看板分類：\033[m[%s]", currboard, currbrd->class);
  if(HAS_PERM(PERM_ALLBOARD) && (currbattr & BRD_PERSONAL))
    outs(" \033[1;30m*\033[m");

  prints("\n\033[1m看板標題\033[m：%s", currbrd->title);
  if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))
  {
    move(12, 59);
    outs("\033[1;30mb)看板閱\讀紀錄\033[m");
  }

  prints("\n\033[1m板主名單\033[m：%s", currbrd->BM);
  if(bbstate & STAT_BOARD)
  {
    move(13, 59);
    outs("\033[1;30mo)板友名單\033[m");
  }

  prints("\n\033[1m開板時間\033[m：%s", Btime(&(currbrd->bstamp)));
  if(bbstate & STAT_BOARD)
  {
    move(14, 59);
    outs("\033[1;30mw)水桶名單\033[m");
  }

  if(bbstate & STAT_POST)
    prints("\n\033[%sm發文限制：您%s\033[m", "1", "可以發文");
  else if(is_bwater(bpal))
    prints("\n\033[%sm發文限制：您%s\033[m", "1;31", "正被水桶中");
  else
    prints("\n\033[%sm發文限制：您%s\033[m", "31", "無法發文");
  if(is_bm(blist, cuser.userid)== 1)	/* 只有正板主可以設定板助名單 */
  {
    move(15, 59);
    outs("\033[1;30mv)板助名單\033[m");
  }

  prints("\n轉信設定：本看板\033[1;36m%s是\033[m轉信板", (currbattr & BRD_NOTRAN)? "不":"");
  if(!(currbattr & BRD_NOTRAN))		/* 非轉信板不會出現這項設定 */
  {
    prints("，預設存為\033[1m%s\033[m", (currbattr & BRD_LOCALSAVE)? "站內":"轉出");
    if(bbstate & STAT_BOARD)
      outs("                     \033[1;30mi)調整此項設定\033[m");
  }

  prints("\n統計紀錄：本看板文章\033[1;32m%s會\033[m列入熱門話題統計", (currbattr & BRD_NOSTAT)? "不":"");

  prints("\n匿名看板：本看板\033[1;33m%s是\033[m匿名板", (currbattr & BRD_ANONYMOUS)? "":"不");

  prints("\n轉錄資訊：從本看板轉出文章\033[1;35m%s會\033[m在原文留下紀錄", (currbattr & BRD_CROSSNOREC)? "不":"");
  if(bbstate & STAT_BOARD)
  {
    move(19, 59);
    outs("\033[1;30mc)調整此項設定\033[m");
  }

  prints("\n回應等級：本看板\033[1;32m%s\033[m評分文章(推文)", (currbattr & BRD_NOSCORE)? "不能":"可以");
  if(bbstate & STAT_BOARD)
  {
    move(20, 59);
    outs("\033[1;30ms)調整此項設定\033[m");
  }

  prints("\n加密文章：無法閱\讀本看板密文(X)的人\033[1;36m看%s到\033[m該文章的標題", (currbattr & BRD_XRTITLE)? "得":"不");
  if(bbstate & STAT_BM)
  {
    move(21, 59);
    outs("\033[1;30mx)調整此項設定\033[m");
  }

  outs("\n\033[34m───────────────────────────────────────\033[m\n");


  move(b_lines, 0);
  if(bbstate & STAT_BOARD)
    outs("\033[1;36m◆ 請按相對應的按鍵修改設定或觀看名單，按其他鍵離開:          \033[37;44m [按任意鍵繼續] \033[m");
  else
    outs(VMSG_NULL);

  quit = 0;
  ret = XO_BODY;

  do
  {
    move(b_lines, 0);
    switch(vkey())
    {
    case 'b':
      if((bbstate & STAT_BOARD) || HAS_PERM(PERM_ALLBOARD))
        ret = post_usies(xo);
      quit = 1;
      break;

    case 'o':
      if(bbstate & STAT_BOARD)
        ret = XoBM(xo);
      quit = 1;
      break;

    case 'w':
      if(bbstate & STAT_BOARD)
        ret = XoBW(xo);
      quit = 1;
      break;

    case 'v':
      /* floatJ.091018: 改用直接判定是否為正板主，避免從XoBV跳出還要處理重繪問題 */
      if (is_bm(blist, cuser.userid)== 1)  /* 只有正板主可以設定板助名單 */
        ret = XoBV(xo);
      quit = 1;
      break;

    case 'i':
      /* 有轉信的看板才會有這項 */
      if((bbstate & STAT_BOARD) && !(currbattr & BRD_NOTRAN))
      {
        newbrd.battr ^= BRD_LOCALSAVE;
	move(16, 34);
	outs((newbrd.battr & BRD_LOCALSAVE)? "\033[1m站內":"\033[1m轉出");
      }
      else
        quit = 1;
      break;

    case 'c':
      if(bbstate & STAT_BOARD)
      {
        newbrd.battr ^= BRD_CROSSNOREC;
	move(19, 26);
	attrset(13);
	outs((newbrd.battr & BRD_CROSSNOREC)? "不會":"會");
	attrset(7);
	outs("在原文留下紀錄  ");
      }
      else
        quit = 1;
      break;

    case 's':
      if(bbstate & STAT_BOARD)
      {
        newbrd.battr ^= BRD_NOSCORE;
	move(20, 16);
	attrset(10);
	outs((newbrd.battr & BRD_NOSCORE)? "\033[1;32m不能":"\033[1;32m可以");
	attrset(7);
      }
      else
        quit = 1;
      break;

    case 'x':
      if(bbstate & STAT_BM)
      {
        newbrd.battr ^= BRD_XRTITLE;
	move(21, 37);
	attrset(14);
	outs((newbrd.battr & BRD_XRTITLE)? "得":"不");
	attrset(7);
      }
      else
        quit = 1;
      break;

    default:
      quit = 1;
    }

  }while(!quit);

  if (memcmp(&newbrd, currbrd, sizeof(BRD)))
  {
    memcpy(currbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
    currbattr = newbrd.battr;	/* dust.090322: 即時更新現在看板的旗標 */
  }

  return ret;
}



/* ----------------------------------------------------- */
/* 板主選單						 */
/* ----------------------------------------------------- */


int
post_manage(xo)
  XO *xo;
{
#ifdef POPUP_ANSWER
  char *menu[] = 
  {
    "BQ",
    "BTitle  修改看板標題",
    "WMemo   編輯進板畫面",
#  ifdef POST_PREFIX
    "RPrefix 編輯文章類別",
#    ifdef HAVE_TEMPLATE
    "Example 編輯文章範本",
#    endif
#  endif
    "PostLaw 編輯發文綱領",
    "Manager 增減副板主",
    "Usies   看板閱\讀記錄",
#  ifdef HAVE_SCORE
    "Score   設定可否評分",
#  endif
#  ifdef HAVE_MODERATED_BOARD
    "Level   公開/好友/秘密",
    "OPal    板友名單",
    "XBWlist 水桶名單",
/*  "VBVlist 板助名單",*/
#  endif
    NULL
  };
#else
   char *menu = "◎ 板主選單 (B)標題 (W)進板  (M)副板 (U)紀錄"
#  ifdef POST_PREFIX
    " (R)類別" 
#  endif
    " (P)綱領"
#  ifdef HAVE_SCORE
    " (S)評分"
#  endif
#  ifdef HAVE_MODERATED_BOARD
    " (L)權限 (O)板友" "(X)水桶"
#  endif
    "？[Q] ";
#endif

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

#ifdef POPUP_ANSWER
  switch (pans(3, 20, "板主選單", menu))
#else
  switch (vans(menu))
#endif
  {
  case 'b':
    return post_brdtitle(xo);

  case 'w':
    return post_memo_edit(xo);

#ifdef POST_PREFIX
  case 'r':
    return post_prefix_edit(xo);
#ifdef HAVE_TEMPLATE
  case 'e':
    return post_template_edit(xo);
#endif

#endif

  case 'p':
    return post_postlaw_edit(xo);

  case 'm':
    return post_changeBM(xo);

  case 'u':
   return post_usies(xo);

#ifdef HAVE_SCORE
  case 's':
    return post_battr_noscore(xo);
#endif

#ifdef HAVE_MODERATED_BOARD
  case 'l':
    return post_brdlevel(xo);

  case 'o':
    return XoBM(xo);

  case 'x':
    return XoBW(xo);
  
  case 'v':
    return XoBV(xo);
#endif
  }

  return XO_FOOT;
}

