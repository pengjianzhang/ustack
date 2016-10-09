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
#include "common/config.h"
#include "common/types.h"
#include "common/list.h"
#include "common/hashtable.h"
#include "port_range.h"
#include "driver.h"
#include "local_ip.h"


/*
 * design thought:
 * 
 * we has two types of IP: 
 * 	--self ip, olny used at FULL-NAT mode, connecting server
 * 		all the process share self ip, but using ports seperately
 * 	--vs ip, only used for vs
 *
 * if a VS uses FULL-NAT mode, this VS should set some self ip.
 *
 * */


static  inline int lip_hashkey(struct ip_address * ip)
{
	u32 h;
	
	h = ip_address_hash(ip);
	
	return (h& LOCAL_IP_HT_MASK);
}


/*------------- local ip functions  start -----------*/
static void lip_free(struct local_ip * lip)
{
	if(lip)
	{
		if(lip->tcp_port)	port_range_free(lip->tcp_port);
		if(lip->udp_port)	port_range_free(lip->udp_port);
		FREE(lip);
	}
}

/*
 * <ip>
 * 	if the <ip> is not exist , create a local_ip struct it
 * 	if ths <ip> exist, return the local_ip struct pointer 
 *<a>	NIC adapter, which adapter this <ip> bind on 
 *
 * return:
 * 	the local_ip structure 
 * */

static struct local_ip * lip_create(int port_num)
{
	struct local_ip * lip = (struct local_ip *)CALLOC(1,sizeof(struct local_ip));

	if(lip == NULL)  goto err;
	
	if(( lip->tcp_port = port_range_create(port_num)) == NULL) goto err;	
	if(( lip->udp_port = port_range_create(port_num)) == NULL) goto err;	



	return lip;
err:
	if(lip)
		lip_free(lip);
	
	return NULL;
}

static void lip_init(struct local_ip * lip, struct ip_address * ip,u16 low, u16 high,int type , int nic_id)
{
	port_range_init(lip->tcp_port, low, high);
	port_range_init(lip->udp_port, low, high);

	INIT_LIST_HEAD(&(lip->list));
	INIT_LIST_HEAD(&(lip->nic_list));
	
	ip_address_copy(&(lip->addr), ip);
	
	lip->type = type;
	lip->nic_id = nic_id;
}

/*------------- local ip functions  END -----------*/


/*-------------ip set functions START ------------ */

static struct ip_set * ip_set_create(u16 low, u16 high)
{

	struct ip_set * set = MALLOC(sizeof(struct ip_set));	

	if(set == NULL) return NULL;

	if((set->hashtable =  hashtable_alloc(LOCAL_IP_HT_SIZE)) == NULL)
	{
		FREE(set);
		return NULL;
	}

	hashtable_init(set->hashtable,LOCAL_IP_HT_SIZE);


	return set;
}

static struct local_ip * ip_set_lookup(struct ip_set *set, struct ip_address * ip)
{
	int idx = lip_hashkey(ip);
	struct local_ip * lip;
	
	list_for_each_entry(lip, & set->hashtable[idx], list) {
		if(ip_address_eq(ip,&(lip->addr) )) return lip;
	}
	
	return NULL;
}


static int ip_set_add(struct ip_set * set, struct local_ip * lip)
{
	int idx = lip_hashkey(&(lip->addr));

	list_add(&(lip->list), &(set->hashtable[idx]));

	return 1;
}


static int ip_set_del(struct local_ip * lip)
{
	list_del(&(lip->list));

	return 1;
}



/*-------------ip set functions END ------------ */


/*----------IP Space functions START ------------*/
	
struct ip_space * lip_ip_space_create(u16 vs_low, u16 vs_high, u16 self_low, u16 self_high)
{
	struct ip_space * space = (struct ip_space *)MALLOC(sizeof(struct ip_space));
	struct ip_set * self_v4 = ip_set_create(self_low,self_high);
	struct ip_set * self_v6 = ip_set_create(self_low,self_high);
	struct ip_set * vs_v4 = ip_set_create(vs_low,vs_high);
	struct ip_set * vs_v6 = ip_set_create(vs_low,vs_high);


	space->self_ip_v4_set = self_v4;
	space->self_ip_v6_set = self_v6;
	space->vs_ip_v4_set = vs_v4;
	space->vs_ip_v6_set = vs_v6;


	space->vs_port_low = vs_low;
	space->vs_port_high = vs_high;
	space->self_port_low = self_low;
	space->self_port_high = self_high;

	return space;
}


static struct ip_set * __get_ip_set(struct ip_space * space, int is_vs, int is_ipv4)
{
	struct ip_set * set;

	if(is_vs){
		if(is_ipv4)	set = space->vs_ip_v4_set;
		else		set = space->vs_ip_v6_set;
	}else{
		if(is_ipv4)	set = space->self_ip_v4_set;
		else		set = space->self_ip_v6_set;
	}

	return set;
}

struct local_ip * lip_ip_space_lookup(struct ip_space * space, struct ip_address * ip, int is_vs)
{
	struct ip_set * set;

	set = __get_ip_set(space,is_vs, ip_address_is_ipv4(ip));
		
	return ip_set_lookup(set,ip);
}


int lip_ip_space_add(struct ip_space * space, struct local_ip * lip)
{
	struct ip_set * set = __get_ip_set(space, lip_is_vsip(lip), lip_is_ipv4( lip));
	return ip_set_add(set,lip);
}

int lip_ip_space_del(struct ip_space * space, struct local_ip * lip)
{

	return ip_set_del(lip);
}
	

struct local_ip * lip_ip_space_create_local_ip(struct ip_space * space, struct ip_address * ip,int type, int nic_id)
{
	int port_num,port_low,port_high ;
	struct local_ip * lip;

	if(type == SELF_IP_T)
	{
		
		port_high = space->self_port_high;
		port_low = space->self_port_low ;
	}
	else
	{
		port_high = space->vs_port_high;
		port_low = space->vs_port_low ;
	}	

	port_num = port_high - port_low + 1;

	lip = lip_create(port_num);

	lip_init(lip,ip,port_low,port_high,type, nic_id);


	return lip;	
}


