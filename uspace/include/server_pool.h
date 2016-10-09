/*
 * server pool 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.2
 * */



#ifndef _SERVER_POOL_H
#define _SERVER_POOL_H

#include "common/list.h"
#include "common/types.h"
#include "types/server_pool.h"

#define for_each_server(srv,head) list_for_each_entry(srv,head,list)
#define for_each_server_pool(pool,head) list_for_each_entry(pool,head,list)

struct list_head * server_pool_get_server_list();

struct list_head * server_pool_get_server_pool_list();

struct server_pool *  server_pool_create();

void server_pool_free(struct server_pool * p);

struct server * server_pool_add_server(struct server_pool * p, struct net_address * addr);


int server_pool_add_server_TEST(struct server_pool * p, u8 * mac, struct net_address * addr);

struct server *  server_pool_get_server(struct server_pool * p);


static inline void server_set_mac(struct server * srv, u8 * mac)
{
	memcpy(srv->mac,mac,ETH_ALEN);
}

static inline struct ip_address * server_get_ip_address(struct server * srv)
{
	return &(srv->addr.ip);
}

static inline u16 server_get_port(struct server * srv)
{
	return srv->addr.port;
}

#endif
