/*
 * interface.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.4
 * */


#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "common/list.h"
#include "types/local_ip.h"
#include "types/interface.h"
#include "local_ip.h"

struct interface * interface_module_init(int num, u16 vs_low,u16 vs_high,u16 self_low, u16 self_high);

struct local_ip * interface_create_local_ip(struct interface * itf, struct ip_address * ip,int type, int nic_id);

struct local_ip * interface_lookup(struct interface * itf, struct ip_address * ip, int is_vs);

void interface_add_lip(struct interface * itf,struct local_ip * lip);

void interface_del_lip(struct interface * ift, struct local_ip * lip);

struct local_ip * interface_get_lip_rr(struct interface * itf,int nic_id, int is_vsip, int is_ipv4);

static inline struct local_ip * interface_create_vsip(struct interface * itf, struct ip_address * ip, int nic_id)
{
	return interface_create_local_ip(itf, ip,VS_IP_T,nic_id);
}

static inline struct local_ip * interface_create_selfip(struct interface * itf, struct ip_address * ip, int nic_id)
{
	return interface_create_local_ip(itf, ip,SELF_IP_T,nic_id);
}

#endif
