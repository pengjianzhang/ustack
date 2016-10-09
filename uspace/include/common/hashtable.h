/*
 * hashtable.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.3
 * */




#ifndef _COMMON_HASHTABLE_H
#define _COMMON_HASHTABLE_H

#include "common/jhash.h"
#include "common/list.h"
#include "common/types.h"

extern u32 hash_rand; 

struct list_head * hashtable_alloc(unsigned int size);


void hashtable_init(struct list_head * table, int size);


void hashtable_free(struct list_head * table, int size);

int hash_module_init();

static inline u32 ip_address_hash(struct ip_address * ip)
{
	u32 h;

	if(ip->family == AF_INET)
		h = jhash_1word(ip->addr.ip4,hash_rand);
	else
		h = jhash(&(ip->addr.ip6),4, hash_rand); /* ipv6 size is 4 * 32 bit*/

	return (h);
}

static inline u32 ip_port_proto_hash(struct ip_address * ip,u32 port, u32 proto)
{
	u32 h;
	u32 t;

	if(ip->family == AF_INET)
	{
		h = jhash_3words(ip->addr.ip4, port, proto, hash_rand);
	}
	else
	{
		t = JHASH_GOLDEN_RATIO;
		t += port;
		t += proto;
		t += hash_rand;

		h = jhash(&(ip->addr.ip6),4, t); /* ipv6 size is 4 * 32 bit*/
	}
	return (h);
}

#endif
