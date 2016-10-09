/*
 * types/connection.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */

#ifndef _TYPES_CONNECTION_H
#define _TYPES_CONNECTION_H

#include "common/list.h"
#include "common/types.h"
#include "types/interface.h"
#include "types/timer.h"
#include "linux-net/if_ether.h"

/*
 * Client Info
 *
 *
 * VS Info
 *
 *
 *
 * Server Info
 *	Using a struct server pointer : server MAC, IP , Port
 *	
 *
 *
 * */
struct connection {
	struct list_head        client_list;	/* hashed by client IP:PORT */
	struct list_head        self_list;	/* hashed by self IP:PORT */

	struct local_ip *	vs_addr; 	/* virtual address , need <vs_addr>, as vs contains more than one IP */
	struct local_ip * 	self_addr;
	struct server * 	server;		/* destination address */
	struct vserver * vs;			/* this connnection belongs to a vritual server  */
	struct timer	expire;

	struct ip_address client_addr;	/* client address */
	u16	client_port;
	u16	self_port;			/* self port, used in FULL-NAT Mode */

	/* route info */
 	u8 client_adapter_idx;			/* VServer <--> Client using this adapter */
	u8 client_queue_idx;			/* VServer <--> Client using this rx_queue/tx_queue index */ 
	u8 server_adapter_idx;			/* VServer <--> Server using this adapter */
	u8 server_queue_idx;			/* VServer <--> Server using this rx_queue/tx_queue index */
	u8 client_mac[ETH_ALEN]; 		/* client mac address  */	

	u8	state;				/* TCP State */
	u8	flags;				/* address family;   FULL-NAT, default Transparant */
	u8 	protocol;			/*protocol (TCP/UDP);*/

};


#endif

