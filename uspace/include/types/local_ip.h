/*
 * include/types/local_ip.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */


#ifndef _TYPES_LOCAL_IP_H
#define _TYPES_LOCAL_IP_H

#include "common/list.h"
#include "types/port_range.h"
#include "common/types.h"

struct local_ip 
{
	struct list_head list;		/* link in local_ip_hashtable */
	
	struct list_head nic_list;	/* link in a nic  */

	int type;			/* SELF_IP_T or VS_IP_T */
	struct ip_address addr;

	int nic_id;			/* bind on a nic */

	struct port_range * tcp_port;	/* tcp port range */
	struct port_range * udp_port;	/* udp port range */
};


struct ip_set
{
	struct list_head * hashtable;
};


struct ip_space
{
	u16 vs_port_low;
	u16 vs_port_high;
	u16 self_port_low;
	u16 self_port_high;		

	struct ip_set * self_ip_v4_set;
	struct ip_set * self_ip_v6_set;
	struct ip_set * vs_ip_v4_set;
	struct ip_set * vs_ip_v6_set;
};


#endif
