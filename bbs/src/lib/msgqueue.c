#include<stdlib.h>


typedef struct LinkedList
{
  int data;
  struct LinkedList *next;
} LL;

static LL *head, *tail;


static LL*
NewNode(data)
  int data;
{
  LL *p = (LL*) malloc(sizeof(LL));
  if(!p)
    exit(1);	/* running out of memory, just exit the process */

  p->data = data;
  p->next = NULL;
  return p;
}


void
MQ_SendMessage(msg)
  int msg;
{
  if(tail)
  {
    tail->next = NewNode(msg);
    tail = tail->next;
  }
  else	/* tail is NULL, it means that the queue is empty */
  {
    head = tail = NewNode(msg);
  }
}


int
MQ_GetMessage()
{
  int msg;
  LL *p;

  if(head)
  {
    p = head;
    if(!(head = head->next))	/* the message queue is empty right now */
      tail = NULL;
    msg = p->data;
    free(p);
    return msg;
  }
  else	/* head is NULL, it means that the queue is empty! */
  {
    return 0;	/* dust.091215: ���o�����ӭn�� return MQ_NONE�A���L�Sinclude�N���ޤF */
  }
}

