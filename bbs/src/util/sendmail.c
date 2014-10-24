/*-------------------------------------------------------*/
/* util/sendmail.c					 */
/*-------------------------------------------------------*/
/* target : BBS sendmail utility			 */
/* create : 10/12/16				 	 */
/* update :   /  /  				 	 */
/*-------------------------------------------------------*/



#include "bbs.h"


#if UNLEN > 60
#error please increase the size of sender_nick[]
#endif



#define SENDER_TIMEOUT 60


static char report[512];
static char buf[1024];
static char *remote_site;

static time_t stamp;
static int is_readonly;
static int no_attach;
static char fpath[128];
static char from[128];
static char rcpt[128];
static char sender_userid[64];
static char sender_nick[64];
static char title[256];
static char content_type[64];
static char transfer_encode[64];
static char disposition[128];
#ifdef EMAIL_JUSTIFY
static int is_justify;
#endif



static char*
strfilt(char *src, const char *filter)
{
  while(*filter)
  {
    char *ptr = strchr(src, *filter);
    if(ptr)
      *ptr = '\0';
    filter++;
  }

  return src;
}


static void
report_to_sender(int diagnositc_code)
{
  char report_path[64];
  FILE *fp;

  sprintf(report_path, "tmp/%s_mailreport.tmp", sender_userid);

  if(fp = fopen(report_path, "w"))
  {
    char folder[64];
    HDR hdr;

    fprintf(fp, "作者: Mailer-Utility@" MYHOSTNAME "\n");
    fprintf(fp, "標題: Undelivered Mail Returned to Sender\n");
    fprintf(fp, "時間: %s\n\n", Now());

    fputs("This report is from the mail sender at " MYHOSTNAME "\n\n", fp);
    fputs("I'm sorry to have to inform you that your message could not be delivered to\n", fp);
    fputs("recipients. It's attached below.\n\n", fp);
    fputs("For further assistance, please contact sysop. If you do so, please include\n", fp);
    fputs("this problem report.\n\n\n", fp);
    fputs("                             BBS sendmail utility\n\n", fp);

    fprintf(fp, "Recipient: %s\n", rcpt);
    fprintf(fp, "Remote-MTA: %s\n", remote_site);
    fputs("Diagnostic-Code: ", fp);
    if(diagnositc_code)
      fprintf(fp, "%d\n", diagnositc_code);
    else
      fputs("None\n", fp);

    fprintf(fp, "\n%s\n\n\n", report);
    if(!is_readonly && !no_attach)
    {
      fputs("----------- 轉寄訊息 -----------\n", fp);
      f_suck(fp, fpath);
      unlink(fpath);
    }

    fclose(fp);

    usr_fpath(folder, sender_userid, FN_DIR);
    hdr_stamp(folder, HDR_LINK, &hdr, report_path);
    strcpy(hdr.owner, "MAIL-UTILITY");
    strcpy(hdr.title, "退信通知");
    hdr.xmode = MAIL_NOREPLY;
    rec_add(folder, &hdr, sizeof(HDR));

    unlink(report_path);
  }
}


/* if cur_cmd is NULL, it means this error is not from remote host. */
static void
smtp_fail(char *cur_cmd)
{
  FILE *fp;

  alarm(0);

  strfilt(buf, "\n\r");
  if(cur_cmd)
  {
    sprintf(report, "remote host said: %.400s (in reply to %s command)", buf, cur_cmd);
    report_to_sender(atoi(buf));
  }
  else
  {
    sprintf(report, "sendmail: %s", buf);
    report_to_sender(0);
  }

  /* 紀錄寄信失敗紀錄到系統中 */
  if(fp = fopen(FN_RUN_MAILFAIL_LOG, "a"))
  {
#ifdef EMAIL_JUSTIFY
    if(is_justify)
      fprintf(fp, "%s %-13s=> %s\n\t%s\n\t%s\n\n", Btime(&stamp), sender_userid, rcpt, buf, fpath);
    else
#endif
      fprintf(fp, "%s %-13s-> %s\n\t%s\n\t%s\n\n", Btime(&stamp), sender_userid, rcpt, buf, fpath);
  }

  exit(0);
}


static void
common_signal(int sig)
{
  sprintf(report, "sendmail: Exit on signal [%d] (errno:%d)", sig, errno);
  report_to_sender(0);

  exit(0);
}


static void
timeout_signal(int sig)
{
  sprintf(report, "sendmail: mail-delivery timed out. (%d sec)", SENDER_TIMEOUT);
  report_to_sender(0);

  exit(0);
}


static int
get_mailinfo()
{
  FILE *fp;
  char pipe_path[64];
  int tryout = 0;
  pid_t pid = getpid();

  sprintf(pipe_path, FN_TMP_SENDMAIL, pid);
  while(!(fp = fopen(pipe_path, "r")))
  {
    if(++tryout >= 5)	/* 正常時主程式從fork()到mkfifo不會過這麼久，所以應該沒問題 */
      exit(0);

    sleep(1);
  }


  if(fread(&stamp, sizeof(time_t), 1, fp) != 1)
    return 0;

  if(fread(&is_readonly, sizeof(int), 1, fp) != 1)
    return 0;

  if(fread(&no_attach, sizeof(int), 1, fp) != 1)
    return 0;

  if(!fgets(fpath, sizeof(fpath), fp))
    return 0;

  if(!fgets(from, sizeof(from), fp))
    return 0;

  if(!fgets(rcpt, sizeof(rcpt), fp))
    return 0;

  if(!fgets(sender_userid, sizeof(sender_userid), fp))
    return 0;

  if(!fgets(sender_nick, sizeof(sender_nick), fp))
    return 0;

  if(!fgets(title, sizeof(title), fp))
    return 0;

  if(!fgets(content_type, sizeof(content_type), fp))
    return 0;

  if(!fgets(transfer_encode, sizeof(transfer_encode), fp))
    return 0;

  if(!fgets(disposition, sizeof(disposition), fp))
    return 0;
#ifdef EMAIL_JUSTIFY
  if(fread(&is_justify, sizeof(int), 1, fp) != 1)
    return 0;
#endif

  fclose(fp);

  strfilt(fpath, "\n");
  strfilt(from, "\n");
  strfilt(rcpt, "\n");
  strfilt(sender_userid, "\n");
  strfilt(sender_nick, "\n");
  strfilt(title, "\n");
  strfilt(content_type, "\n");
  strfilt(transfer_encode, "\n");
  strfilt(disposition, "\n");

  if(!is_readonly)
  {
    sprintf(buf, "%s.%d.tmp", fpath, pid);
    rename(fpath, buf);
    strcpy(fpath, buf);
  }

  return 1;
}


static void
tn_signal()
{
  struct sigaction act;

  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_ONESHOT;

  act.sa_handler = common_signal;
  sigaction(SIGSEGV, &act, NULL);
  sigaction(SIGTERM, &act, NULL);

  act.sa_handler = timeout_signal;
  sigaction(SIGALRM, &act, NULL);
}


int
main()
{
  int sock;
  FILE *fp, *fr, *fw;
  char *str, msgid[80];

  alarm(SENDER_TIMEOUT);	/* 一開始就定下自爆時間，以防主端當掉導致卡死的程序 */
  chdir(BBSHOME);

  if(!get_mailinfo())
    exit(9);

  alarm(SENDER_TIMEOUT);
  tn_signal();

  remote_site = str = strchr(rcpt, '@') + 1;
  sock = dns_smtp(str);
  if(sock >= 0)
  {
    fr = fdopen(sock, "r");
    fw = fdopen(sock, "w");

    fgets(buf, sizeof(buf), fr);
    if(memcmp(buf, "220", 3))
      smtp_fail("initial");

    while(buf[3] == '-')
      fgets(buf, sizeof(buf), fr);

    fprintf(fw, "HELO %s\r\n", BBSNAME);
    fflush(fw);
    do
    {
      fgets(buf, sizeof(buf), fr);
      if(memcmp(buf, "250", 3))
	smtp_fail("HELO");
    } while(buf[3] == '-');

    fprintf(fw, "MAIL FROM:<%s>\r\n", from);
    fflush(fw);
    do
    {
      fgets(buf, sizeof(buf), fr);
      if(memcmp(buf, "250", 3))
	smtp_fail("MAIL FROM");
    } while(buf[3] == '-');

    fprintf(fw, "RCPT TO:<%s>\r\n", rcpt);
    fflush(fw);
    do
    {
      fgets(buf, sizeof(buf), fr);
      if(memcmp(buf, "250", 3))
	smtp_fail("RCPT TO");
    } while(buf[3] == '-');

    fprintf(fw, "DATA\r\n", rcpt);
    fflush(fw);
    do
    {
      fgets(buf, sizeof(buf), fr);
      if(memcmp(buf, "354", 3))
	smtp_fail("DATA");
    } while(buf[3] == '-');


    /* ------------------------------------------------- */
    /* begin of mail header				 */
    /* ------------------------------------------------- */

    archiv32(stamp, msgid);

    /* Thor.990125: 儘可能的像 RFC 822 及 sendmail 的作法, 免得別人不接:p */
    fprintf(fw, "From: \"%s\" <%s>\r\n",
#ifdef EMAIL_JUSTIFY
      is_justify? "BBS Register" :
#endif
      sender_nick, from);
    fprintf(fw, "To: %s\r\n", rcpt);

    /* itoc.030411: mail 輸出 RFC 2047 */
    output_rfc2047_qp(fw, "Subject: ", title, MYCHARSET, "\r\n");

    fprintf(fw, "Date: %s\r\n", Atime(&stamp));
    fprintf(fw, "Message-Id: <%s@" MYHOSTNAME ">\r\n", msgid);

    /* itoc.030323: mail 輸出 RFC 2045 */
    fprintf(fw, "Mime-Version: 1.0\r\n");
    fprintf(fw, "Content-Type: %s; charset="MYCHARSET"\r\n", content_type);
    fprintf(fw, "Content-Transfer-Encoding: %s\r\n", transfer_encode);
    if(disposition[0])
      fprintf(fw, "Content-Disposition: attachment; filename=%s\r\n", disposition);

    fprintf(fw, "X-Sender: %s (%s)\r\n", sender_userid, sender_nick);
    fputs("X-Disclaimer: [" BBSNAME "] 對本信內容恕不負責\r\n\r\n", fw);


    /* ------------------------------------------------- */
    /* begin of mail body				 */
    /* ------------------------------------------------- */

#ifdef EMAIL_JUSTIFY
    if(is_justify)	/* 認證信 */
      fprintf(fw, " ID: %s (%s)  E-mail: %s\r\n\r\n", sender_userid, sender_nick, rcpt);
#endif

    if(fp = fopen(fpath, "r"))
    {
      char *ptr;

      buf[0] = '.';
      str = buf + 1;
      if(disposition[0])	/* LHD.061222: MQ_ATTACH, 去掉開頭 begin 那行 */
	fgets(str, sizeof(buf) - 3, fp);

      while(fgets(str, sizeof(buf) - 3, fp))
      {
	if(ptr = strchr(str, '\n'))
	  strcpy(ptr, "\r\n");

	fputs(*str == '.' ? buf : str, fw);
      }

      fclose(fp);
    }

#ifdef HAVE_SIGNED_MAIL
    if(!rec_get(FN_RUN_PRIVATE, buf, 8, 0))
    {
      time_t prichro;

      buf[8] = '\0';
      prichro = chrono32(buf);
      archiv32(str_hash(msgid, prichro), buf);
      fprintf(fw,"※ X-Sign: %s$%s %s\r\n", msgid, genpasswd(buf), Btime(&stamp));
    }
#endif
    fputs("\r\n.\r\n", fw);
    fflush(fw);

    fgets(buf, sizeof(buf), fr);
    if(memcmp(buf, "250", 3))
      smtp_fail("DATA(end)");

    fputs("QUIT\r\n", fw);
    fflush(fw);

    alarm(0);

    fclose(fw);
    fclose(fr);

    if(!is_readonly)
      unlink(fpath);
  }
  else
  {
    strcpy(buf, "cannot setup connection.");
    smtp_fail(NULL);
  }

  return 0;
}

