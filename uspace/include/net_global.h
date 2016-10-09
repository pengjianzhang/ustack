/*
 * net_global.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.7
 * */


#ifndef _NET_GLOBAL_H
#define _NET_GLOBAL_H


#include "interface.h"
#include "driver.h"
#include "packet.h"
#include "lf_queue.h"
#include "local_ip.h"
#include "route.h"
#include "types/net_global.h"
#include "common/types.h"

extern struct net_global net_global;


int ng_init(int id, struct driver * d, struct packet_zone * z, struct lfq * q, struct interface * itf,struct route_table * rt);

struct local_ip * ng_add_ip(char*  ip_string, int family,int isvsip,  int nic_id);

static inline struct local_ip *	ng_interface_create_vsip(struct ip_address * ip,int nic_id)
{
	return interface_create_local_ip(net_global.itf,ip,VS_IP_T,nic_id);
}

static inline struct local_ip *	ng_interface_create_selfip(struct ip_address * ip,int nic_id)
{
	return interface_create_local_ip(net_global.itf,ip,SELF_IP_T,nic_id);
}



static inline struct local_ip * ng_interface_lookup_vsip(struct ip_address * ip)
{
	return  interface_lookup(net_global.itf, ip,1);
}


static inline struct local_ip * ng_interface_lookup_selfip(struct ip_address * ip)
{
	return  interface_lookup(net_global.itf, ip,0);
}


static inline struct local_ip * ng_interface_lookup_ip(struct ip_address * ip)
{
	struct local_ip * lip;

	lip = interface_lookup(net_global.itf, ip,0);
	
	if(lip == NULL)
		lip = interface_lookup(net_global.itf, ip,1);

	return lip;
}

static inline struct local_ip * ng_interface_get_selfipv4_rr(int nic_id)
{
	return interface_get_lip_rr(net_global.itf,nic_id, 0, 1);
}


static inline struct local_ip * ng_interface_get_vsipv4_rr(int nic_id)
{
	return interface_get_lip_rr(net_global.itf,nic_id, 1, 1);
}


static inline struct local_ip * ng_interface_get_selfipv6_rr(int nic_id)
{
	return interface_get_lip_rr(net_global.itf,nic_id, 0, 0);
}

static inline struct route_entry * ng_rt_lookup_rr(struct ip_address * ip)
{
	return rt_lookup_rr(net_global.rt, ip);
}

static inline struct route_entry * ng_rt_lookup(struct ip_address * ip)
{
	return rt_lookup(net_global.rt, ip);
}


static inline struct route_entry * ng_rt_lookup_with_nic_id(struct ip_address * ip, int nic_id)
{
	return rt_lookup_with_nic_id(net_global.rt,ip,nic_id);

}

static inline int ng_rt_add_entry(struct route_entry * re )
{
	return rt_add_entry(net_global.rt,re);
}

static inline u8 *  ng_driver_get_mac(int adapter_idx)
{
	return driver_get_mac(net_global.d, adapter_idx);
}

static inline int ng_driver_put_packet_to_send_buffer(struct packet * pkt)
{
	return driver_put_packet_to_send_buffer(net_global.d, pkt);
}


static inline struct packet * ng_packet_of_phys(u64 phys )
{
	return packet_of_phys(net_global.z, phys );
}

static inline struct packet * ng_get_free_packet()
{
	struct packet_pool * pp =  packet_zone_get_free_packet_pool(net_global.z);
	return packet_pool_get(pp);
}

static inline int ng_driver_get_adapter_num()
{
	return driver_get_adapter_num(net_global.d);
}


#endif

