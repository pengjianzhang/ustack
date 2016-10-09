/*
 * server_pool.c
 *
 * a simple pool
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2012.12.28
 * */


#include <string.h>
#include <stdlib.h>
#include "common/config.h"
#include "common/types.h"
#include "common/list.h"
#include "server_pool.h"

/* link all server pool in a list*/
static  LIST_HEAD( server_pool_list); 
/*link all server in a list */
static  LIST_HEAD( server_list); 

struct list_head * server_pool_get_server_list()
{
	return &server_list;
}


struct list_head * server_pool_get_server_pool_list()
{
	return &server_pool_list;
}


struct server_pool *  server_pool_create()
{
	struct server_pool * sp = (struct server_pool *) MALLOC(sizeof(struct server_pool));
	if(sp)
	{
		sp->num = 0;
		sp->next = 0;
		sp->ref = 0;

		list_add(&(sp->list),&(server_pool_list));
	}
	
	return sp;
}

void server_pool_free(struct server_pool * p)
{
	int i;
	struct server * srv;

	for(i = 0; i < p->num; i++)
	{
		srv = &(p->server_list[i]);
		list_del(&(srv->list));
	}

	list_del(&(p->list));	
	FREE(p);
}

struct server * server_pool_add_server(struct server_pool * p, struct net_address * addr)
{
	struct server * srv;

	if(p->num < MAX_SERVER);
	{
		srv = &(p->server_list[p->num++]);
		srv->addr = * addr;
		list_add(&(srv->list),&(server_list));
		return srv;
	}	

	return NULL;
}


int server_pool_add_server_TEST(struct server_pool * p, u8 * mac, struct net_address * addr)
{
	struct server * srv;

	if(p->num < MAX_SERVER);
	{
		srv = &(p->server_list[p->num++]);
		memcpy(srv->mac,mac,ETH_ALEN);
		srv->addr = * addr;
		return 1;
	}	

	return 0;
}


struct server *  server_pool_get_server(struct server_pool * p)
{
	struct server * srv = NULL;

	if(p->num > 0)
	{
		srv = & p->server_list[p->next];
		p->next = (p->next + 1)%p->num;	
	}

	return srv;
}
