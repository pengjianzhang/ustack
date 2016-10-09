
#ifndef _TYPES_LF_QUEUE_H
#define _TYPES_LF_QUEUE_H

#include "common/list.h"
#include "common/types.h"
#include "common/config.h"


enum lfq_state
{ 
	LFQ_START,	/* this package recv from net */
	LFQ_DISPATCH,	/* packet dispatch to other to process */
	LFQ_RECLAIM,	/* other process return  packet*/
	LFQ_XMIT,	/* let this packet XMIT by other process */
	LFQ_END		/* this packet is not active */	
};
 
struct lfq
{
	int nproc;				/* total processes number  */ 
	int id;					/*this process id */
	int desc_num;				/* each lf_queue have <desc_num>  entry */
	int module_size;			/* total bytes of this module used */	
	int lf_queue_size;			/* each lf_queue_size */
	unsigned long start;			/*this module memory start address */	
	
	struct lf_queue ** recv_queue;		/* recv packet from other process's dispatch */ 
	struct lf_queue ** send_queue;		/* send packet to other process by dispatch */
};

struct lf_queue
{
	int size;	/* <desc> array entry number */
	struct list_head  list;	/* put packet to list, sender use it to buffer packet */ 
	int head  ____cacheline_aligned;	
	int tail  ____cacheline_aligned;
	u64  desc[0];	/* packet physical address */
};



#endif
