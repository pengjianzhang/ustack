/*
 * connection.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.2
 * */



#ifndef _CONNECTION_H
#define _CONNECTION_H

#include "common/config.h"
#include "common/list.h"
#include "common/types.h"
#include "common/comm_defns.h"
#include "types/connection.h"

#include "packet.h"
#include "vserver.h"



#define CONNECTION_IS_FULLNAT(cp)	((cp)->flags &1)
#define CONNECTION_SET_FULLNAT(cp)	((cp)->flags = ((cp)->flags | 1))
#define CONNECTION_SET_TRANSPARANT(cp)	((cp)->flags = ((cp)->flags & 0xfe))

#define CONNECTION_IS_TCP(cp)		((cp)->protocol == PROTO_TCP)


#define CONNECTION_TIMEOUT_SECONDS	60


struct connection * connection_lookup(struct ip_address * ip ,u16 port,u8 proto, u8 is_client);


struct connection * connection_create(struct packet * pkt,struct vserver * vs);
void connection_free(struct connection *cp);

int connection_module_init();

static inline struct ip_address * connection_get_client_ip(struct connection *cp)
{
	return &(cp->client_addr);
}


static inline u16 connection_get_client_port(struct connection *cp)
{
	return cp->client_port;
}


#endif
