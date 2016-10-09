/*
 * include/types/serve_ pool.h
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */

#ifndef _TYPES_SERVER_POOL_H
#define _TYPES_SERVER_POOL_H

#include "common/types.h"
#include "linux-net/if_ether.h"


struct server
{
	struct list_head list;	/*link in global server_list */
	struct net_address addr; 
	u8	mac[ETH_ALEN];	/* mac addr, TODO get it from route-table */
	u64 bitsin;
	u64 bitsout;
	u64 current_conn;
	u64 total_conn;	
};


#define MAX_SERVER 64 	
struct server_pool
{
	struct list_head list;	/*link in global server_pool_list*/
	struct server server_list[MAX_SERVER];
	int num;
	int next;
	int ref;
};

#endif
