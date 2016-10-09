/*
 * dispatch.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.4
 * */


#ifndef _DISPATCH_H
#define _DISPATCH_H

#include "common/types.h"
#include "types/packet.h"
#include "types/lf_queue.h"

int dispatch_module_init(int n, int id);

void dispatch_get_port_range(u16 * low, u16 * high);

void dispatch_packet(struct lfq * q, struct list_head * send_head,struct list_head * recv_head, struct list_head * reclaim_head);

void dispatch_reclaim(struct lfq * q, struct list_head  * to_reclaim);

void dispatch_flush(struct lfq * q);

/*
 * check <port> if belongs to this process
 * */
/*
static inline int dispatch_check_self_port(u16 port)
{
	if(port >= disp.self_port_low && port <= disp.self_port_high)
		return 1;
	else 
		return 0;
}
*/

#endif
