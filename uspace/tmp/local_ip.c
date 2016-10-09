/*
 * local_ip.c 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.1
 * */

#include <stdlib.h>
#include "config.h"
#include "types.h"
#include "port_range.h"
#include "driver.h"
#include "list.h"
#include "hashtable.h"
#include "jhash.h"
#include "random.h"
#include "local_ip.h"


static struct list_head * local_ip_hashtable ;

static u32 local_ip_rand;

int local_ip_module_init()
{
	local_ip_hashtable =  hashtable_alloc(LOCAL_IP_HT_SIZE);

	if(local_ip_hashtable == NULL) return 0;
	
	hashtable_init(local_ip_hashtable,LOCAL_IP_HT_SIZE);

	local_ip_rand = SEED();

	return 1;
}


static inline int local_ip_hashkey(u32 ip, u8 protocol )
{
	return (jhash_2word(ip,(u32)protocol,local_ip_rand) & LOCAL_IP_HT_MASK);
}

static inline int local_ip_hashkey2(struct local_ip *lip )
{
	return local_ip_hashkey(lip->addr.ip4, lip->protocol )
}


struct local_ip * local_ip_lookup(u32 ip, u8 protocol)
{
	int idx = local_ip_hashkey(ip,protocol);
	struct local_ip * lip;
	
	list_for_each_entry(lip, &local_ip_hashtable[idx], list) {
		if((lip->addr.ip4 == ip) && (lip->protocol == protocol)) return lip;
	}
	
	return NULL;
}


void local_ip_add_vs(struct local_ip * lip, struct vserver * vs, int is_vsip)
{
	if(lip->vs)
		local_ip_del_vs(lip)

	lip->vs = vs;
	lip->is_vsip = is_vsip;

	if(lip->is_vsip && lip->vs)
		list_add(&(lip->vs_list),&(vs->vsip_list));			
	else if ( (lip->is_vsip == 0 ) && lip->vs)
		list_add(&(lip->vs_list),&(vs->selfip_list));			

}

void local_ip_del_vs(struct local_ip * lip)
{
	if(lip->is_vsip && lip->vs)
		list_add(&(lip->vs_list),&(vs->vsip_list));			
	else if ( (lip->is_vsip == 0 ) && lip->vs)
		list_add(&(lip->vs_list),&(vs->selfip_list));			
}

void local_ip_add(struct local_ip * lip, struct vserver * vs, int is_vsip)
{
	int idx = local_ip_hashkey2(lip);
	list_add(&(lip->list), &(local_ip_hashtable[idx]));
	local_ip_add_vs(lip,vs,is_vsip)
}



void local_ip_del(struct local_ip * lip)
{
	list_del(&(lip->list));

	
	local_ip_del_vs(lip);
}


void local_ip_free(struct local_ip * lip)
{
	if(lip->range)
		port_range_free(lip->range);

	FREE(lip);
}


struct local_ip * local_ip_create()
{
	struct local_ip * lip = (struct local_ip *)CALLOC(1,sizeof(struct local_ip));

	if(lip == NULL)  goto err;
	
	lip->range = port_range_create((MAX_PORT - MIN_PORT + 1));
	if(lip->range == NULL) goto err;	
	
	return lip;
err:
	if(lip)
		local_ip_free(lip);
	
	return NULL;
}


/*
 *<a>		each local ip should reside on a NIC adapter
 * */
void  local_ip_init2(struct local_ip * lip, u32 ip, struct adapter * a, u8 is_vsip,u8 protocol, struct vserver * vs)
{
	INIT_LIST_HEAD(&(lip->list));
	INIT_LIST_HEAD(&(lip->vs_list));	
	lip->addr.ip4 = ip;
	lip->is_vsip = is_vsip; 
	lip->protocol = protocol; 
	lip->vs = vs;
	lip->a = a;
	port_range_init(lip->range, MIN_PORT,MAX_PORT);

	local_ip_add(lip, vs,is_vsip);
}


void  local_ip_init(struct local_ip * lip, u32 ip, struct adapter * a)
{
	local_ip_init2(lip,ip,a,0,NULL);
}


/*
 * alloc a port  <port> from <lip>
 *
 * <port>:	if <port> == 0, system alloc  a random port, is fast, should be use in SELD_IP mode
 * 		if <port> != 9, alloc this <port>, this method is time-cost, should be used in VS_IP mode
 * 		
 *
 * */



u16 local_ip_get_port(struct local_ip * lip,u16 port)
{
	return port_range_get_port(lip->range,port);
}

void local_ip_put_port(struct local_ip * lip, u16 port)
{
	port_range_put_port(lip->range,port);
}
