/*
 * net_global.c 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.7
 * */


#include "net_global.h"
#include "route.h"
#include "local_ip.h"
#include <arpa/inet.h>

struct net_global net_global;

int ng_init(int id, struct driver * d, struct packet_zone * z, struct lfq * q, struct interface * itf,struct route_table * rt)
{
	net_global.id = id;
	net_global.d = d;		
	net_global.z = z;		
	net_global.q = q;		
	net_global.itf = itf;		
	net_global.rt = rt;
	
	return 1;
}


/*
 * add ip to system: to interface module and route module
 * <ip_string>: the ip to add
 * <family>: AF_INET or AF_INET6
 * <isvsip>: add to self ip or vs ip
 * <nic_id>: this ip bind to a nic
 *
 * return:
 * 	NULL	fail
 * 	>0 	success, local_ip address of this ip	
 * */
struct local_ip * ng_add_ip(char*  ip_string, int family,int isvsip,  int nic_id)
{
	u32 ip4;
	struct ip_address ip;
	struct in6_addr in6;
	struct route_entry * re = rt_entry_create();
	struct local_ip * lip = NULL;

	if(re == NULL) return NULL;

	if(family == AF_INET)
	{
		ip4 = inet_addr(ip_string);		
		ip_address_init_v4(&ip,ip4);
	}			
	else
	{
		inet_pton(AF_INET6,ip_string,(void*)&in6);		
		ip_address_init_v6(&ip, (u32*)&in6);
	}	
	

	if(isvsip)
		lip = interface_create_local_ip(net_global.itf,&ip,VS_IP_T,nic_id);
	else
		lip = interface_create_local_ip(net_global.itf,&ip,SELF_IP_T,nic_id);

	rt_entry_init(re, &ip, nic_id);

	if(!rt_add_entry(net_global.rt, re))  return NULL;

	return lip;
} 




