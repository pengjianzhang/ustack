/*
 * route.c 
 *
 * support ipv4/ipv6
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.3
 * 		2013.2.3 refactoring 
 * */

#include <stdio.h>
#include <string.h>
#include "common/types.h"
#include "common/config.h"
#include "common/hashtable.h"
#include "common/list.h"
#include "route.h"
#include "server_pool.h"
#include "net_global.h"

struct route_table * rt_create()
{
	struct route_table * rt = (struct route_table *)MALLOC(sizeof(struct route_table));

	if((rt->hashtable =  hashtable_alloc(ROUTE_HT_SIZE)) == NULL)
	{
		FREE(rt);
		return NULL;
	}

	
	hashtable_init(rt->hashtable,ROUTE_HT_SIZE);

	return rt;
}

/* TODO */
void route_table_free(struct route_table * rt)
{
	printf("Sorry, not really free: route_table_free %lx\n",rt);
}


static  inline int rt_hashkey(struct ip_address * ip)
{
	u32 h;
	h = ip_address_hash(ip);
	return (h & ROUTE_HT_MASK);
}


struct route_entry * rt_entry_create()
{
	return MALLOC(sizeof(struct route_entry));
}

void rt_entry_free(struct route_entry * re)
{
	if(re) FREE(re);
}


void rt_entry_init(struct route_entry * re, struct ip_address * addr, u32 nic_id)
{
	memcpy(&(re->addr),addr,sizeof(struct ip_address));
	re->nic_id = nic_id;
	INIT_LIST_HEAD(&(re->list));
}

/*
 * lookup route table for ipv4 address, using round robin algorithm 
 *<rt> route table
 *<dest_ip> ip address
 *<nic_id> nic_id, if nic_id == -1, don't compare nic_id 
 *return: a route
 * */
static struct route_entry * __rt_lookup_rr(struct route_table * rt, struct ip_address * ip,int rr_enable, int nic_id)
{
	int idx = rt_hashkey(ip);
	struct route_entry * re;
	
	list_for_each_entry(re, &rt->hashtable[idx], list) {

		if(nic_id == -1 || nic_id == re->nic_id)
		{
			if(ip_address_eq(&(re->addr),ip))
			{
				if(rr_enable){
					list_del(&(re->list));
					list_add_tail(&(re->list), &(rt->hashtable[idx]));
				}
				
				return re;
			}	
		}
	}
	
	return NULL;
}


struct route_entry * rt_lookup_rr(struct route_table * rt, struct ip_address * ip)
{
	return __rt_lookup_rr(rt, ip,1,-1);
}


struct route_entry * rt_lookup(struct route_table * rt, struct ip_address * ip)
{
	return __rt_lookup_rr(rt, ip,0,-1);
}


struct route_entry * rt_lookup_with_nic_id(struct route_table * rt, struct ip_address * ip, int nic_id)
{
		
	return __rt_lookup_rr(rt, ip,0,nic_id);
}

/*
 * return: 
 * 	0 not add <re> to <rt>
 * 	1 add success
 * */
int rt_add_entry(struct route_table * rt, struct route_entry * re)
{
	int idx ;

	if(rt_lookup(rt, &(re->addr))) return 0; 
	
	idx = rt_hashkey(&(re->addr));

	list_add(&(re->list),&(rt->hashtable[idx]));

	return 1;
}


int rt_del_entry(struct route_table * rt, struct route_entry * re)
{
	list_del(&(re->list));	

	return 1;
}



struct route_table * rt_module_init()
{
	return rt_create();
}	


void rt_find_server_route()
{
	int i;
	struct server * srv;
	struct list_head * head = server_pool_get_server_list();
	struct ip_address * srv_ip;
	int num = ng_driver_get_adapter_num();

	for_each_server(srv,head) 
	{
		srv_ip = server_get_ip_address(srv);
		for(i = 0; i < num; i++ )
			arp_send_request(srv_ip,i);
	}
}
