/*
 * connection.c 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.1
 * */

/*
 * connection table description
 *
 *1, Transparant proxy
 *
 * cleint ip:port <-->vs ip:port [PROXY]   client ip:port <--> server ip:port
 *
 * client packet can be found by srouce ip:port, server side packet can be found by dest ip:port(client IP:PORT) 
 *
 *
 * 2, Full-NAT Proxy
 *
 * client ip:port <--> vs ip:port   self ip:port <--> server ip:port
 *  
 * client packet can be found by srouce ip:port, server side packet can be found by dest ip:port( Self IP:PORT) 
 *
 *
 * 3, How to find a connection
 * 
 * 3.1 check packet destination IP:PORT
 * 	if it belongs a VS, the packet is comming from a client, so search connection hash table by client ip:port
 * 	if it belongs a self IP, the packet is comming from server, and its a FULL-NAT mode, so search connection hash table by self IP:PORT
 * 	if it is a remote address, the packget is comming from server, and its a Transparant mode, so search connection hash table by client IP:PORT 
 *
 *
 *3.2 FULL NAT Mode 2 side search 
 *	client side hash(client IP:PORT), and server side hash(Self IP:PORT) using different argument, how to let them find the same connection table 
 *	
 * */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "common/list.h"
#include "common/types.h"
#include "common/config.h"
#include "common/random.h"
#include "common/jhash.h"
#include "common/hashtable.h"
#include "common/debug.h"
#include "linux-net/if_ether.h"
#include "linux-net/ip.h"
#include "linux-net/tcp.h"
#include "linux-net/udp.h"
#include "tcp.h"
#include "local_ip.h"
#include "connection.h"
#include "vserver.h"
#include "packet.h"
#include "timer.h"
#include "net_global.h"
#include "route.h"
#include "mem_pool.h"
#include "server_pool.h"

/* connection pool */
static struct mem_pool * conn_mem_pool;

/* hash-table ,hash function */
static struct list_head * client_hashtable;
static struct list_head * self_hashtable;

#define CONN_HT_BITS	CONFIG_CON_TAB_BITS
#define CONN_HT_SIZE	(1 << CONN_HT_BITS) 	/* hash table size */
#define CONN_HT_MASK	( (CONN_HT_SIZE) - 1)	/* bit mask for hash */


/* connection memory  pool */

static int connection_mem_pool_create()
{
	conn_mem_pool = mem_pool_create("connection pool", sizeof(struct connection));
	if(conn_mem_pool == NULL) return 0;

	return 1;
}

static struct connection * connectoin_get_free()
{
	return (struct connection *)mem_pool_alloc(conn_mem_pool);
}


static void connectoin_put_free(struct connection * cp)
{
	mem_pool_free(conn_mem_pool,(void *)cp);
}

/* end of connection memory pool */


static unsigned int connection_hashkey(struct ip_address * ip, u32 port, u32 proto)
{
	u32 h = ip_port_proto_hash(ip,port, proto);
	return (h & CONN_HT_MASK);
}



static unsigned int connection_hashkey_client(struct connection * cp)
{
	return connection_hashkey(&(cp->client_addr), cp->client_port,cp->protocol);
}

static unsigned int connection_hashkey_self(struct connection * cp)
{
	return connection_hashkey(&(cp->self_addr->addr), cp->self_port,cp->protocol);
}


static inline int  connection_client_eq(struct connection * cp, struct ip_address * ip ,u16 port,u8 proto)
{

	if(ip_address_eq( &(cp->client_addr),ip) && (cp->client_port == port) && (cp->protocol = proto) )
		return 1;
	else
		return 0;
}

static inline int  connection_self_eq(struct connection * cp, struct ip_address *ip ,u16 port,u8 proto)
{

	if(ip_address_eq(&(cp->self_addr->addr), ip) && (cp->self_port == port) && (cp->protocol = proto) )
		return 1;
	else
		return 0;
}


static void connection_add(struct connection * cp)
{
	unsigned int idx;

	idx =  connection_hashkey_client(cp);
	list_add(&(cp->client_list),&client_hashtable[idx]);	

	if(CONNECTION_IS_FULLNAT(cp))	
	{
		idx =  connection_hashkey_self(cp);
		list_add(&(cp->self_list),&self_hashtable[idx]);	
	}
}


static void connection_del(struct connection * cp)
{
	list_del(&(cp->client_list));

	if(CONNECTION_IS_FULLNAT(cp))
		list_del(&(cp->self_list));
}



/*
 * <is_client> 
 * 	1,	lookup client_hashtable
 * 	0,	lookup self_hashtable
 * 	
 * */
struct connection * connection_lookup(struct ip_address * ip ,u16 port,u8 proto, u8 is_client)
{
	unsigned int idx = connection_hashkey(ip ,port,proto);
        struct connection *cp;

	if(is_client){
		list_for_each_entry(cp, &client_hashtable[idx], client_list) {
			if(cp == NULL)
			{
				printf("Strange\n");
			}
			if(connection_client_eq(cp,ip ,port,proto)) return cp;
		}
        }
	else {
		list_for_each_entry(cp, &self_hashtable[idx], self_list) {
			if(connection_self_eq(cp,ip ,port,proto)) return cp;
		}
	}
	
	return NULL;
}


static int  connection_timeout(struct timer * t)
{
	struct connection * cp =( struct connection *) container_of(t,struct connection,expire);

	connection_free(cp);

	DPRINTF("Connection Time out\n");

	return 1;
}


/* 
 * each process has a queue of each adapter 
 * */
static inline int connection_set_route(struct packet * pkt, struct connection * cp, struct server * srv)
{
	struct ethhdr  * ep = packet_get_ethhdr(pkt);
	struct route_entry * re;

	/* Set Client<-->VServer route infomation */

	memcpy(cp->client_mac,ep->h_source ,ETH_ALEN); 	
	cp->client_adapter_idx = pkt->adapter_idx;
	cp->client_queue_idx = 0;		/* queue idex is unuseable , as this packet may come from other process */

	/* Set Server<-->VServer route infomation */	

	re = ng_rt_lookup_rr(&(srv->addr.ip));

	if(re == NULL) return 0;

	cp->server_adapter_idx = re->nic_id;
	cp->server_queue_idx = 0;

	return 1;
}

static inline int connection_set_self_ip_port(struct connection * cp,int nic_id,int is_ipv4, int is_tcp)
{
	struct local_ip * lip;
	u16 port;
	
	if(is_ipv4)
		lip = ng_interface_get_selfipv4_rr(nic_id);
	else
		lip = ng_interface_get_selfipv6_rr(nic_id);
		
	if(lip == NULL) return 0;

	cp->self_addr =  lip;
	
	if(is_tcp)
		port = lip_get_tcp_port(lip,0);
	else
		port = lip_get_udp_port(lip,0);

	if(port == 0)
		return 0;
			
	cp->self_port = port;	

	return 1;
}

/*
 * connection_create
 * 1.create a connection structure
 * 2.select a backend Server
 * 3.Set route infomation
 * 4.link it to connection hash table
 *
 * if VS is IPV6, servers should be IPV6
 * else if VS is IPV4, servers should be IPV4
 *
 * */
struct connection * connection_create(struct packet * pkt,struct vserver * vs)
{
	struct connection *cp = (struct connection *)connectoin_get_free();
	u8 idx = 0;

	if(cp == NULL) goto err;
	
	timer_init(&(cp->expire),connection_timeout);
	cp->vs_addr = ng_interface_lookup_vsip(packet_get_dip(pkt));
	cp->protocol = vs->protocol;
	cp->vs = vs;

	if(vs->is_fullnat)
		CONNECTION_SET_FULLNAT(cp);
	else
		CONNECTION_SET_TRANSPARANT(cp);
		
	/* Select a Server */
	cp->server = vserver_get_server(vs);	 
	if(cp->server == NULL)
	{	
		DPRINTF("can't find server\n");	
		goto err;
	}	

	if(connection_set_route(pkt,cp,cp->server) == 0)
	{

		DPRINTF("connection set route error\n");	
		goto err;
	}	
	/* set self IP:PORT */
	if(CONNECTION_IS_FULLNAT(cp)){
		if(connection_set_self_ip_port(cp,cp->server_adapter_idx,
			PACKET_IS_IPV4(pkt),VS_IS_TCP(vs)) == 0) 
			{
				DPRINTF("connection set self ip port error\n");	
				goto err;
			}
	}	

	/* Set  client IP:PORT, vs IP:PORT */
	cp->client_addr = * packet_get_sip(pkt);
	cp->client_port = packet_get_sport(pkt);

	if(VS_IS_TCP(vs))
		cp->state = TCP_INIT;

	connection_add(cp);
	timer_add(&(cp->expire),CONNECTION_TIMEOUT_SECONDS);

	return cp;
err:
	if(cp)
		connectoin_put_free(cp);

	return NULL;
}


void connection_free(struct connection *cp)
{
	if(CONNECTION_IS_FULLNAT(cp))
	{
		if(CONNECTION_IS_TCP(cp))
			lip_put_tcp_port(cp->self_addr, cp->self_port);
		else
			lip_put_udp_port(cp->self_addr, cp->self_port);
	}
	timer_del(&(cp->expire));

	connection_del(cp);
	connectoin_put_free(cp);
}

/*
 * init connection module
 *
 *
 * */
int connection_module_init()
{
	u32 conn_size = CONN_HT_SIZE;

	if(connection_mem_pool_create() == 0)
		return 0;

	client_hashtable =  hashtable_alloc(conn_size);
	if(client_hashtable == NULL) return 0;

	hashtable_init(client_hashtable, conn_size);

	self_hashtable = hashtable_alloc(conn_size);
	if(self_hashtable == NULL) return 0;

	hashtable_init(self_hashtable, conn_size);

	return 1;	
}

