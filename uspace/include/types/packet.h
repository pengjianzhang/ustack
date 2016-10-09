/*
 * types/packet.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */


#ifndef _TYPES_PACKET_H
#define _TYPES_PACKET_H

#include "common/types.h"
#include "common/list.h"
#include "types/lf_queue.h"

struct packet
{
	struct list_head  next;	/*link package tDo freelist ... */

	u64	phys;		/* physical address */
	u16	len;		/* data length */
	u16	size;		/* 2k */

	u16	protocol_eth;	/* protocol at ethhdr */
	u8	protocol_ip;	/* protocl at iphdr */		

	u8	family;		/* AF_INET, AF_INET6 */
	u8	adapter_idx;	/* adapter recv this packagyee */
//	u8	queue_idx;	/* rx queue recv this package */	

	enum lfq_state lfq_state;	/* lfq state  */	

	int 	id;		/* this packet is alloced to a process,ynot changed from init */
	
	struct net_address saddr;	/*src address ip:port */	
	struct net_address daddr;	/*dst address ip:port */
		
        int	transport_header;	/* offset of transport layer header */
        int	network_header;		/* offset of network layer header */
	int	mac_header;		/* offset of mac layer header */

};

struct packet_pool
{
	__u64	num;
	struct list_head  list;
};

struct packet_zone
{
	u64	start_all;	/* packet memory address */
	u64	phys_all;	/* physical address of the total packet memory */

	__u64 	start;	//package start
	__u64	phys;	//physical address of <start>
	__u64	size;		
	__u64	packet_num;
	__u16	packet_size;	//2k
	__u16	packet_offset;	//packet info offset from packet data head	
	int	id;
	struct packet_pool  free_packet_pool;
};










#endif
