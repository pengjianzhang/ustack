/*
 * include/types/route.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */


#ifndef _TYPES_ROUTE_H
#define _TYPES_ROUTE_H


#include "common/types.h"
#include "common/list.h"

/*each ip may have many route entry */
struct route_entry
{
	struct list_head list;		/* link in route hashtable*/
	struct ip_address addr;
	u32 nic_id;			/* nic id */	
};

struct route_table
{
	u32 gw_nic_id;
	struct list_head * hashtable;
};

#endif
