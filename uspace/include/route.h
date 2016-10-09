/*
 * route.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.3
 * */


#ifndef _ROUTE_H
#define _ROUTE_H

#include "types/route.h"
#include "common/types.h"
#include "common/config.h"
#include "common/hashtable.h"
#include "common/list.h"


#define ROUTE_HT_BITS	8
#define ROUTE_HT_SIZE	(1 << ROUTE_HT_BITS)	
#define ROUTE_HT_MASK	( (ROUTE_HT_SIZE) - 1)	


struct route_table * rt_create();

void route_table_free(struct route_table * rt);

struct route_entry * rt_entry_create();

void rt_entry_free(struct route_entry * re);

void rt_entry_init(struct route_entry * re, struct ip_address * addr, u32 nic_id);

struct route_entry * rt_lookup_rr(struct route_table * rt, struct ip_address * ip);

struct route_entry * rt_lookup(struct route_table * rt, struct ip_address * ip);

struct route_entry * rt_lookup_with_nic_id(struct route_table * rt, struct ip_address * ip, int nic_id);

int rt_add_entry(struct route_table * rt, struct route_entry * re);

int rt_del_entry(struct route_table * rt, struct route_entry * re);

struct route_table * rt_module_init();


void rt_find_server_route();
#endif
