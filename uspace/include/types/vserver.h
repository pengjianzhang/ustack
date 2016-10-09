/*
 * types/vserver.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */

#ifndef _TYPES_VSERVER_H
#define _TYPES_VSERVER_H

#include "common/list.h"
#include "common/types.h"

#define VS_IP_NUM_MAX	16

struct vserver
{
	struct list_head  list;	/* vs list */	
	struct list_head  vs_lip_node;	/* hash table for fast lookup */
	struct server_pool * pool;

	u8 name[32];
	struct local_ip * addr[VS_IP_NUM_MAX]; /* flipper ip, network order*/
	u8 ip_num;			

	u16	port;		/*network order*/
	u8	protocol;	/*TCP UDP*/		

	u8 is_fullnat;		/*set 1, enable fullnat mode,  default TRANSPARANT mode */
};


struct vs_lip_node
{
	struct list_head hash_list;	/*link in hashtable */	
	struct list_head next; 		/* link in vs */

	struct vserver * vs;
	struct local_ip * lip;
};


#endif
