/*
 * types/interface.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */


#ifndef _TYPES_INTERFACE_H
#define _TYPES_INTERFACE_H


#include "common/list.h"
#include "types/local_ip.h"

struct nic
{
	int id;
		
	/* ip bind on this nic  */
	
	struct list_head self_list_ipv4;
	struct list_head self_list_ipv6;

	struct list_head vs_list_ipv4;
	struct list_head vs_list_ipv6;

};


struct interface
{
	int num;
	struct ip_space * space;
	struct nic nic[0];
};


#endif
