#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BBSUID	1001
#define BBSGID	1001
#define BBSUSER	"bbs"
#define BBSHOME	"/home/bbs"

#define CHZ_MSG

int checkid(void);
void print_usage(void);
void start_daemons(void);
void stop_daemons(void);

static uid_t bbs_uid;

int main(int argc, char **argv)
{
    FILE *fp;
    char buffer[100] = "\0";
    char buffer2[100] = "\0";
    pid_t init_pid;
/*
#if 1
    bbs_uid = getpwnam(BBSUSER) -> pw_uid;
#endif
*/
    bbs_uid = BBSUID;

    printf("\n");

/* floatJ: 取消權限限制  (目前quarter上的BBS權限設定似乎仍有問題，會不能runbbs) */
#if 1
      if (getuid() != 0 && checkid() < 0) {
  	fprintf(stderr, "%s:  Sorry, but you don't have the permission\n\n", argv[0]);
  	return 0;
      }
#endif
    setreuid(0, 0);

    if (chdir(BBSHOME) < 0) {
	perror(argv[0]);
	return -1;
    }

    if (argc != 2)
	print_usage();

    else if (strcmp(argv[1], "init") == 0) {
#ifdef CHZ_MSG
        printf("正在初始化BBS服務 (camera & account) :\n"); 
#else
	printf("Initializing BBS:\n");
#endif
	init_pid = fork();
	if (init_pid == 0) {
	    setuid(bbs_uid);
	    system(BBSHOME "/bin/camera");
	    system(BBSHOME "/bin/account");
	    exit(0);
	}
	else
	    waitpid(init_pid, NULL, 0);
	start_daemons();
	sleep(1);
    }

    else if (!strcmp(argv[1], "start"))
	start_daemons();

    else if (!strcmp(argv[1], "stop"))
	stop_daemons();

    else if (!strcmp(argv[1], "restart")) {
	stop_daemons();
	sleep(1);
	start_daemons();
    }

    else if (!strcmp(argv[1], "killall")) {
	stop_daemons();
	system("killall -15 bbsd");
    }

    else if (!strcmp(argv[1], "ipcrm")) {
#ifdef CHZ_MSG
        printf("正在清除\033[1;31m簡諧運動\033[m (SHM Segments) :\n");
#else
	printf("Clearing Shared Memory Segments:\n");
#endif
	fp = popen("ipcs -m | grep " BBSUSER " | awk '{print $2}'", "r");
	while (fscanf(fp, "%s", buffer) != EOF) {
	    sprintf(buffer2, "ipcrm shm %s", buffer);
	    system(buffer2);
	}
	pclose(fp);
	fp = NULL;
#ifdef CHZ_MSG
        printf("正在清除信號陣列 (Semaphore Arrays) :\n");  
#else
	printf("Clearing Semaphore Arrays:\n");
#endif
	fp = popen("ipcs -s | grep " BBSUSER " | awk '{print $2}'", "r");
	while (fscanf(fp, "%s", buffer) != EOF) {
	    sprintf(buffer2, "ipcrm sem %s", buffer);
	    system(buffer2);
	}
	pclose(fp);
	fp = NULL;
    }
    else
	print_usage();

    return 0;
}

int checkid(void)
{
    int i, num, size = 16;
    gid_t glist[16];

    if ((num = getgroups(size, glist)) < 0) {
	perror("getgroup");
	exit(-1);
    }
    for (i = 0; i < num; i++)
	if (glist[i] == BBSGID)
	    return 0;

    return -1;
}

void print_usage(void)
{
    fprintf(stderr, "Usage: runbbs [ init | start | stop | restart | killall | ipcrm ]\n\n");
    fprintf(stderr, "init    : Initailizes BBS and starts daemons.\n");
    fprintf(stderr, "start   : Starts daemons.\n");
    fprintf(stderr, "stop    : Stops daemons.\n");
    fprintf(stderr, "restart : Restarts daemons.\n");
    fprintf(stderr, "killall : Stops daemons and kills clients.\n");
    fprintf(stderr, "ipcrm   : Clears shm and sem.\n\n");
}

void start_daemons(void)
{
    int fd;

#ifdef CHZ_MSG
    printf("正在啟動BBS伺服器端相關程式 :\n");
#else
    printf("Starting daemons:\n");
#endif

    if (fork() == 0)
	execl(BBSHOME "/bin/bbsd", "bbsd", NULL);

    if (fork() == 0)
	execl(BBSHOME "/bin/bhttpd", "bhttpd", NULL);
  
#if 1
    if (fork() == 0) {
	execl(BBSHOME "/bin/bmtad", "bmtad", NULL);
    }
#endif
    if (fork() == 0) {
	setuid(bbs_uid);
	execl(BBSHOME "/bin/xchatd", "xchatd", NULL);
    }

    if (fork() == 0) {
	setuid(bbs_uid);
	fd = open(BBSHOME "/run/innbbsd.log", O_RDWR | O_CREAT | O_APPEND, 00644);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	close(fd);
	execl(BBSHOME "/innd/innbbsd", "innbbsd", NULL);
    }
    sleep(1);
    printf("\n");
}

void stop_daemons(void)
{
    char *filename[] = {"bbs.pid", "chat.pid", "bhttp.pid", "innbbsd.pid", NULL, "bmta.pid"};
    char buf[256];
    int i;
#ifndef CHZ_MSG
    printf("Stopping daemons:\n");
#else
    printf("正在停止BBS伺服器端相關程式 :\n");
#endif
    for (i=0;filename[i];i++) {
#  ifdef CHZ_MSG
        printf("正在從%s讀取PID並停止該程序 ...\n", filename[i]);
#  else
	printf("loading %s\n :", filename[i]);
#  endif
        sprintf(buf, "kill `tail -1 %s/run/%s | cut -f 1`", BBSHOME, filename[i]);
	system(buf);
    }
    printf("\n");
}
