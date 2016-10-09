#ifndef _LF_QUEUE_H
#define _LF_QUEUE_H

#include "types/packet.h"
#include "common/list.h"
#include "types/lf_queue.h"

struct lfq * lfq_module_init(int n, int my_id,int desc_num, void * start);



static inline int lfq_get_module_size(struct lfq * q)
{
	return q->module_size;
}

static inline int lfq_free(struct lfq * q)
{
	if(q->recv_queue)
		FREE(q->recv_queue);
	if(q->send_queue)
		FREE(q->send_queue);
	FREE(q);
	return 1;
}

int lfq_recv(struct lfq * q,struct  list_head *head);
void lfq_send(struct lfq * q, struct packet * pkt,int id);

int lfq_reclaim_recv(struct lfq * q,struct  list_head *head);
void lfq_reclaim_send(struct lfq * q, struct packet * pkt,int id);


void lfq_reclaim_filter(struct lfq * q, struct list_head * head);


void lfq_filter(struct lfq * q, struct list_head * head);

int lfq_send_flush(struct lfq * q);

#endif
