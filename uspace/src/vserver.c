/*
 * vserver.c
 *
 * a simple vserver 
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2012.12.28
 * */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "common/config.h"
#include "common/types.h"
#include "common/hashtable.h"
#include "common/jhash.h"
#include "common/random.h"
#include "vserver.h"
#include "local_ip.h"
#include "server_pool.h"


/* All VS link together, for search by name */
static struct list_head  vs_list;
static struct list_head * vs_hashtable;	

#define VS_HT_BITS	7
#define VS_HT_SIZE	( 1 << VS_HT_BITS)	
#define VS_HT_MASK	( VS_HT_SIZE - 1)	

int vserver_module_init()
{
	INIT_LIST_HEAD(&(vs_list));
	vs_hashtable =  hashtable_alloc(VS_HT_SIZE);
	if(vs_hashtable == NULL) return 0;
	hashtable_init(vs_hashtable,VS_HT_SIZE);

	return 1;
}

static inline  int vs_hashkey(struct ip_address * ip, u16 port,u8 protocol)
{
	
	u32 h = ip_port_proto_hash(ip,port, protocol);
	return (h & VS_HT_MASK);
}



struct vserver * vserver_lookup(struct ip_address * ip, u16 port, u8 protocol, struct local_ip ** vs_lip)
{
	int idx =  vs_hashkey(ip,port,protocol);

	struct vs_lip_node * node;
	struct local_ip * lip;	
	struct vserver * vs;

	list_for_each_entry(node, &(vs_hashtable[idx]), hash_list) 
	{
		vs = node->vs;
		lip = node->lip;	
		if( ip_address_eq(&(lip->addr),ip) && (vs->port == port ) && (vs->protocol == protocol))
		{
			if(vs_lip != NULL)
				*vs_lip = lip;
			return vs;
		}
	}
	
	return NULL;
}



/*
 * if return 0, ERROR
 *
 * */
static int __vs_lip_add(struct vserver * vs, struct local_ip * lip)
{
	int idx = vs_hashkey(&(lip->addr),vs->port,vs->protocol);
	struct vs_lip_node * node = (struct vs_lip_node *)MALLOC(sizeof(struct vs_lip_node));

	if(node == NULL) return 0;

	list_add(&(node->hash_list), &(vs_hashtable[idx]));
	list_add(&(node->next),&(vs->vs_lip_node));
	node->vs = vs;
	node->lip = lip;

	return 1;
}

static void __vs_lip_del(struct vs_lip_node * node)
{
	list_del(&(node->hash_list));
	list_del(&(node->next));
	FREE(node);
}

static void __vs_add(struct vserver * vs)
{

	int i;
	
	/* link to hashtable */
	for(i = 0; i < vs->ip_num; i++)
		__vs_lip_add(vs,vs->addr[i]);
		
	list_add(&(vs->list),&(vs_list));	/* link to global vs_list */

}


static void __vs_del(struct vserver * vs)
{

	int i;
	
	struct vs_lip_node * pos;
	struct vs_lip_node * n;
	/* del from hashtable */
	
	list_for_each_entry_safe(pos, n, &(vs->vs_lip_node), next)	
	{	
		__vs_lip_del(pos);
	}	

	list_del(&(vs->list));	/* del from  global vs_list */
}



struct vserver *  vserver_create(struct server_pool * p, u16 port, u8 protocol, u8 is_fullnat)
{
	struct vserver * vs = NULL;

	vs = (struct vserver *)MALLOC(sizeof(struct vserver));
	if(vs == NULL) return NULL;

	vs->pool = p;	
	vs->port = port;
	vs->protocol = protocol;
	vs->is_fullnat = is_fullnat;
	vs->ip_num = 0;
	
	INIT_LIST_HEAD(&(vs->list));
	INIT_LIST_HEAD(&(vs->vs_lip_node));


	__vs_add(vs);

	return  vs;
} 


void vserver_free(struct vserver * vs)
{
	if(vs)
	{
		__vs_del(vs);
		FREE(vs);
	}
}

int vserver_add_ip(struct vserver * vs, struct local_ip * lip )
{
	int ret = 0;
		
	if(vs->ip_num < VS_IP_NUM_MAX ){
		vs->addr[vs->ip_num++] = lip;
		ret = __vs_lip_add(vs, lip);	/* may return 0 */
	}

	return ret;
}


struct server * vserver_get_server(struct vserver * vs)
{
	return server_pool_get_server(vs->pool);
}

