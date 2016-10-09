/*
 * local_ip.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.1
 * */


#ifndef _LOCAL_IP_H
#define _LOCAL_IP_H

#include "common/config.h"
#include "common/types.h"
#include "common/list.h"
#include "types/local_ip.h"


/*
 * more simple design is 
 * each ip with a port_rang and a protocol 
 * so each ip with two local_ip( as 2 protocol TCP/UDP ) 
 *
 * */

/*
 * ip are devied into 2 types, VS IP, SELF IP
 * VS_IP	only used by vserver
 * SELF_IP	only used by proxy  connecting  pool/servers
 * */


#define SELF_IP_T	1
#define	VS_IP_T		2

#define LOCAL_IP_HT_BITS	8
#define LOCAL_IP_HT_SIZE	(1 << LOCAL_IP_HT_BITS)	
#define LOCAL_IP_HT_MASK	( (LOCAL_IP_HT_SIZE) - 1)	


struct local_ip * lip_ip_space_create_local_ip(struct ip_space * space, struct ip_address * ip,int type, int nic_id);

struct ip_space * lip_ip_space_create(u16 vs_low, u16 vs_high, u16 self_low, u16 self_high);

int lip_ip_space_add(struct ip_space * space, struct local_ip * lip);


int lip_ip_space_del(struct ip_space * space, struct local_ip * lip);

struct local_ip * lip_ip_space_lookup(struct ip_space * space, struct ip_address * ip, int is_vs);


static inline u16 lip_get_tcp_port(struct local_ip * lip,u16 port)
{
	return port_range_get_port(lip->tcp_port,port);
}


static inline u16 lip_get_udp_port(struct local_ip * lip,u16 port)
{
	return port_range_get_port(lip->udp_port,port);
}


static inline void lip_put_tcp_port(struct local_ip * lip, u16 port)
{
	port_range_put_port(lip->tcp_port,port);
}

static inline void lip_put_udp_port(struct local_ip * lip, u16 port)
{
	port_range_put_port(lip->udp_port,port);
}

static inline int  lip_is_selfip(struct local_ip * lip)
{
	return (lip->type == SELF_IP_T);
}



static inline int  lip_is_vsip(struct local_ip * lip)
{
	return (lip->type == VS_IP_T);
}

static inline int  lip_is_ipv4(struct local_ip * lip)
{
	return (lip->addr.family == AF_INET);
}



static inline struct ip_address *  lip_get_ip_address(struct local_ip * lip)
{
	return &(lip->addr);
}

#endif

