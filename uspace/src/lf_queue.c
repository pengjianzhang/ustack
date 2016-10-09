/*
 * lf_queue.c 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.8
 * */

/* 
 * lock-free queue design
 *
 * suppose we have 4 process, so we have 16 lock-free queues 
 *
 *p0 recv: 0-0 0-1 0-2 0-3
 *p1 recv: 1-0 1-1 1-2 1-3
 *p2 recv: 2-0 2-1 2-2 2-3
 *p3 recv: 3-0 3-1 3-2 3-3
 *
 * p0 send: 1-0 2-0 3-0
 * p1 send: 0-1 2-1 3-1
 * p2 send: 0-2 1-2 3-2 
 * p3 send: 0-3 1-3 2-3
 * 
 * */

#include <unistd.h>
#include "common/debug.h"
#include "common/list.h"
#include "packet.h"
#include "driver.h"
#include "lf_queue.h"
#include "dispatch.h"
#include "net_global.h"

#define ALIGN_BYTES 64	


/**************** lfq init START***************/

/* return cache aligned size of a lf_queue struct size */
static int lf_queue_aligned_size(int desc_num)
{
	int align = ALIGN_BYTES; /*for cache line align */  
	
	int size = sizeof(struct lf_queue) + (sizeof(u64) * desc_num) ;
	
	int align_size = ((size + align - 1) / align)*align;

	return align_size;	
} 

/*<n> process number 
 *return: lfq_queue number of this system
 *
 * */
static  int lf_queue_num(int n)
{
	return n * n ;
}


/* 
 * lf queue module used size, is page aligned
 * <n> process number 
 * */
static int __lfq_module_size(int n, int desc_num)
{
	int size =  lf_queue_aligned_size(desc_num);	
	int total = size * lf_queue_num(n);
	size_t pagesize =  getpagesize();

	return ((total + pagesize - 1)/pagesize)*pagesize;
}


static struct lf_queue * lfq_queue_create_init(struct lfq * q, int row, int colum)
{
	struct lf_queue * lq = 	(struct lf_queue *)(q->start + (row * q->nproc  + colum) *q->lf_queue_size);

	lq->size = q->desc_num;
	lq->head = 0;
	lq->tail = 0;
	INIT_LIST_HEAD(&(lq->list));

	return lq;
}

/*
 * all processes share lf_queue memory  
 *
 * <n>		: total process number	
 * <my_id>	: this process id  ( 0 <= my_id < n )	
 * <desc_num>	: each queue have <desc_num> entrys 
 * <start>	: address all processes shared (struct lf_queue )
 *
 * return 
 * 	0	fail
 * 	>0	this modules used space	
 * */
struct lfq * lfq_module_init(int n, int my_id,int desc_num, void * start)
{
	int qnum ;
	int i;
	struct lfq * lfq;

	if((n == 0) || (start == NULL) || (my_id < 0) || (my_id >= n)||(desc_num <= 0)) return NULL;
	lfq = (struct lfq *) MALLOC(sizeof(struct lfq));

	if(lfq == NULL) return NULL;

	lfq->recv_queue = (struct lf_queue **) MALLOC(n * sizeof( struct lf_queue *));
	lfq->send_queue = (struct lf_queue **) MALLOC(n * sizeof(struct lf_queue *));

	if(lfq->recv_queue == NULL || lfq->send_queue == NULL) return NULL;

	lfq->nproc = n;
	lfq->id = my_id;
	lfq->desc_num = desc_num; 
	lfq->module_size = __lfq_module_size(n,desc_num);
	lfq->lf_queue_size = lf_queue_aligned_size(desc_num);

	lfq->start = (unsigned long ) start;
	

	for(i = 0; i < n; i++)
	{
		/*get queue*/
		lfq->recv_queue[i] = lfq_queue_create_init(lfq,my_id,i);
		lfq->send_queue[i] = lfq_queue_create_init(lfq,i,my_id);
	}

	return  lfq;
}


/**************** lfq init END***************/



/* get packet from a <queue>, and add the packets to the tail of <head> 
 *return: the number of packet get from <queue>
 *
 * */

static int __get_packet_from_queue(struct lf_queue * queue,struct list_head * lhead)
{
	int head = queue->head;
	int tail = queue->tail;	
	struct packet * pkt;
	u64  phys;
	int count = 0;
	int i = head;

	while(i != tail)
	{
		phys = queue->desc[i];	
		pkt = ng_packet_of_phys(phys);
		list_add_tail(&(pkt->next),lhead);
		count++;
		i++;
		if(i >= queue->size) i = 0;		
	}

	if(count > 0)
	{
		barrier();
		if(!bCAS(&(queue->head),head,i))
		{
			DPRINTF("ERRPR __get_packet_from_queue\n");
			exit(-1);
		}
	}		

	return count;
}

/* put packet to a queue 
 *<queue>: put packet to the queue, if queue is full, not do it
 *<head>: packet list, get packet from the list
 * 
 * return: the number put to the queue
 *
 * */

static int __put_packet_to_queue(struct lf_queue * queue,struct list_head * lhead)
{
	int head = queue->head;
	int tail = queue->tail;	
	struct packet * pkt;
	int count = 0;
	int i = tail;
	int i_next;

	i_next = (i + 1)%queue->size;
	if( i_next == head) return 0;

	do{
		if(list_empty(lhead)) break;		
		pkt = list_first_entry(lhead, struct packet, next);
		list_del(&(pkt->next));	
		queue->desc[i] = pkt->phys;	
		count++;
		i = i_next;
		i_next++;	
		if(i_next >= queue->size) i_next = 0;		
		
	}while(i_next != head);	

	if(count > 0)
	{
		wmb();
		if(!bCAS(&(queue->tail),tail,i))
		{
			DPRINTF("ERRPR __get_packet_from_queue\n");
			exit(-1);
		}
	}		

	return count;
}


/* 
 * recv packets from other process, add new packet to tail of <head> 
 * 
 * */

static inline int __lfq_recv(struct lfq * q, struct lf_queue ** qhead,struct  list_head *head)
{
	int i;
	int count = 0;

	for(i = 0; i < q->nproc; i++)
	{
		if(i != q->id ) 
		{
			count += __get_packet_from_queue(qhead[i],head);
		}
	}

	return count;
}

int lfq_recv(struct lfq * q,struct  list_head *head)
{
	return __lfq_recv(q, q->recv_queue,head);
}



static inline void __lfq_send(struct lfq * q,struct lf_queue ** qhead, struct packet * pkt, int id)
{
	struct lf_queue * queue = qhead[id];
	list_add_tail(&(pkt->next), &(queue->list));
}

void lfq_send(struct lfq * q, struct packet * pkt,int id)
{
	__lfq_send(q,q->send_queue,pkt,id);
}


static inline int __lfq_send_flush(struct lfq * q, struct lf_queue ** qhead)
{
	int i;
	int count = 0;
	struct lf_queue * queue;

	for(i = 0; i < q->nproc; i++)
	{
		queue = qhead[i];

		if(i != q->id) 
		{
			count += __put_packet_to_queue(queue,&(queue->list));
		}
	}

	return count;
}

int lfq_send_flush(struct lfq * q)
{
	return  __lfq_send_flush( q, q->send_queue);
}




/*
 * filter packets from <head>, and send these packets to the packet owner process for reclaim
 *
 * */
void lfq_reclaim_filter(struct lfq * q, struct list_head * head)
{
	struct packet * pos;
	struct packet * n;

	if(!list_empty(head))
	{
		list_for_each_entry_safe(pos, n, head, next)
		{
			if(pos->id != q->id)
			{	
				list_del(&(pos->next));
				packet_set_lfq_state(pos,LFQ_RECLAIM);
				lfq_send(q, pos,pos->id);
			}	
		}

	
		lfq_send_flush(q);
	}
}




