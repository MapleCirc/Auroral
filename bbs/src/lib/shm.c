#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void *
shm_new(int shmkey, int shmsize)
{
  void *shmptr;
  int shmid;

  shmid = shmget(shmkey, shmsize, 0);
  if (shmid < 0)
  {
    shmid = shmget(shmkey, shmsize, IPC_CREAT | 0600);
    if (shmid < 0)
      exit(-1);
  }
  else
  {
    shmsize = 0;
  }

  shmptr = (void *) shmat(shmid, NULL, 0);
  if (shmptr == (void *) -1) {
    fprintf(stderr, "Cannot allocate SHM: %s\n", strerror(errno));
    exit(-2);
  }

  if (shmsize)
    memset(shmptr, 0, shmsize);

  return shmptr;
}
