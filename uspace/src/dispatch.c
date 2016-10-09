/*
 * dispatch.c 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.4
 * */

#include "common/debug.h"
#include "common/types.h"
#include "common/jhash.h"
#include "types/dispatch.h"
#include "packet.h"
#include "local_ip.h"
#include "net_global.h"
#include "dispatch.h"
#include "lf_queue.h"

static struct dispatch disp;


int dispatch_module_init(int n, int id)
{
	disp.id = id;
	disp.num = n;
	disp.port_base = 1024;
	disp.port_slice = (65535 - disp.port_base) / n;
	u16 _low = id * disp.port_slice + disp.port_base;
	u16 _high = _low + disp.port_slice - 1;

	disp.self_port_low = _low;
	disp.self_port_high = _high;
		
	return 1;
}

void dispatch_get_port_range(u16 * low, u16 * high)
{
	*low = disp.self_port_low;
	*high = disp.self_port_high;
}

static inline int dispatch_port_to_id(u16 port)
{
	int id = (port - disp.port_base) / disp.port_slice;	

	return id;
}

/* only dispatch IP packet 
 *
 * <itf> interface
 * <pkt> packet 
 *
 * return:
 *	process id, this packet shoube be dispatched to 
 * */
static int __dispatch_ip_packet(struct packet * pkt)
{
	int id;
	u32 h;
	struct ip_address * src_ip =&(pkt->saddr.ip) ;
	struct ip_address * dst_ip = &(pkt->daddr.ip) ;
	struct local_ip * lip;
	u16 dst_port = pkt->daddr.port;


	/*prepare src_ip */
	lip =  ng_interface_lookup_vsip(src_ip);
	/* comming from client*/
	if(lip)
	{
		h = ip_address_hash(src_ip); 
		id = h % disp.num;
	}
	else
	{
		/* prepare dst ip , search is self ip? */
		lip =  ng_interface_lookup_selfip(dst_ip);

		/* is self ip */
		if(lip)
		{
			id = dispatch_port_to_id(dst_port);
		}
		else
		{
			h = ip_address_hash(dst_ip);
			id = h % disp.num;
		}
	}

	return id;
}

/*
 * send packet to othetr process, 
 * receive active|non-active packet from other process
 * receive XMIT packet from other process, and put it to driver
 *
 * <q>	  : lfq 
 * <send_head> : dispatch packet,send packets in <send_head> to other process
 * <recv_head> : receive active packets to <recv_packet> from other process
 * <reclaim_head>:  receive reclaimed packets from other process 
 * */
void dispatch_packet(struct lfq * q, struct list_head * send_head,struct list_head * recv_head, struct list_head * reclaim_head)
{
	struct packet * pos;
	struct packet * n;
	int id;

	enum lfq_state s;

	if(!list_empty(send_head))
	{
		list_for_each_entry_safe(pos, n, send_head, next)
		{
			
			if(PACKET_IS_IP(pos))
			{
				id = __dispatch_ip_packet(pos);
				
				if(id != q->id)
				{	
					DPRINTF("dispatch %d-->%d\n",q->id,id);
					list_del(&(pos->next));
					packet_set_lfq_state(pos,LFQ_DISPATCH);
					lfq_send(q, pos,id);
				}
			}	
		}

		lfq_send_flush(q);
	}	
	
	lfq_recv(q,recv_head);

	if(!list_empty(recv_head))
	{
		list_for_each_entry_safe(pos, n, recv_head, next)
		{
			s = packet_get_lfq_state(pos);	
			DPRINTF("dispatch recv %d-->%d\n",pos->id,q->id);
			switch(s)
			{
			case LFQ_START:	break;
			case LFQ_DISPATCH: break;
			case LFQ_RECLAIM:
				packet_set_lfq_state(pos,LFQ_END);
				list_move_tail(&(pos->next),reclaim_head);
				break;
			case LFQ_XMIT:	
				list_del(&(pos->next));
				/* other process let me to xmit this packet */
				ng_driver_put_packet_to_send_buffer(pos);
				break;
			case LFQ_END:
			default:
				DPRINTF("error @@@\n");
				exit(-1);
			}	
		}
	}
} 

void dispatch_reclaim(struct lfq * q, struct list_head  * to_reclaim)
{	
	struct packet * pos;
	struct packet * n;
	int id;


	if(!list_empty(to_reclaim))
	{
		list_for_each_entry_safe(pos, n, to_reclaim, next)
		{
			id = pos->id;
			if(id != q->id)
			{	
				list_del(&(pos->next));
				packet_set_lfq_state(pos,LFQ_RECLAIM);
				lfq_send(q, pos,id);
			}	
		}

		lfq_send_flush(q);
	}
}


void dispatch_flush(struct lfq * q)
{
	lfq_send_flush(q);
}

