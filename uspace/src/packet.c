/*
 * packet.c 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */



#include "common/list.h"
#include "common/config.h"
#include "common/comm_defns.h"
#include "common/debug.h"
#include "linux-net/if_ether.h"
#include "linux-net/if_arp.h"
#include "linux-net/ip.h"
#include "linux-net/ipv6.h"
#include "packet.h"
#include "lf_queue.h"



struct packet * packet_init(__u64 start, __u64 phys, int id)
{
	struct packet * pkt = packet_of((__u8 *)start);

	pkt->lfq_state = LFQ_END;
	pkt->phys = phys;
	pkt->len = 0;
	pkt->size = PACKET_SIZE;
	pkt->id = id;

	INIT_LIST_HEAD(&(pkt->next));

	return pkt;
}

void packet_set_data_len(struct packet * pkt, __u16 len)
{
	pkt->len = len;
}

struct packet_pool * packet_pool_create()
{
	return (struct packet_pool *)MALLOC(sizeof(struct packet_pool)); 
}


void packet_pool_init(struct packet_pool * pp)
{
	pp->num = 0;
	INIT_LIST_HEAD(&(pp->list));
}

struct packet_zone *packet_zone_create()
{
	return (struct packet_zone *)MALLOC(sizeof(struct packet_zone));
}





void pool_add_packets(struct packet_zone * zone, struct packet_pool * pool)
{
	__u64 i;
	__u64 start = zone->start;
	__u64 phys = zone->phys;
	__u64 num = zone->size / PACKET_SIZE;
	struct packet * pkt;

	for(i = 0; i < num; i++)	
	{
		pkt = packet_init(start,phys,zone->id);
		packet_pool_put(pool,pkt);
		start += PACKET_SIZE;
		phys += PACKET_SIZE;
	}
}


static void packet_zone_init(struct packet_zone * zone, u64 start_all, u64 phys_all, __u64 start, __u64 phys, __u64 size, int id)
{
	zone->start_all = start_all;
	zone->phys_all = phys_all;
	zone->start = start;
	zone->phys = phys;
	zone->size = size;
	zone->id = id;

	packet_pool_init(&(zone->free_packet_pool));
	pool_add_packets(zone, &(zone->free_packet_pool));
}

/*
 * init packet buffer
 * init lf_queue module 
 *
 *[<---lf-queue-memory--->|<----packet-memory---->]
 *
 * packet buffer and lf_queue use reserved memory before kernel start, see kernel start argument 
 *
 * */

struct packet_zone * packet_module_init( __u64 start, __u64 phys, __u64 size, int nproc, int id)
{
	int i;
	u64 total_num, num;	
	struct packet_zone * z;
	struct packet_pool * p;
	u64 phys_all,start_all;

	if(nproc == 0 || size <= 0 || start <= 0 || phys <= 0) return NULL;

	total_num = size/PACKET_SIZE;
	num = total_num/nproc;	
	
	size = 	num * PACKET_SIZE;
	phys_all = phys;
	start_all = start;	

	start += size*id;
	phys += size*id;

	z = packet_zone_create();

	packet_zone_init(z,start_all,phys_all, start, phys,size,id);

	return z;
}


/*only support TCP/UDP/ICMP */

/*
 *
 * return:
 * 	1 sucess
 * 	0 fail, unknow/unsupported protocol	
 * */
static inline int __packet_init_port(struct packet * pkt, void * header, int proto)
{
	struct tcphdr  * tcph;
	struct udphdr  * udph;
	u16 sport,dport;

	if(proto == PROTO_TCP){
		tcph = (struct tcphdr *)(header);
		sport =tcph->source ;
		dport = tcph->dest;
	}else if(proto == PROTO_UDP){
		udph = (struct udphdr *)(header);
		sport = udph->source ;
		dport = udph->dest;
	}
	else if(proto == PROTO_ICMP)	/* TODO: how to deal with icmp */
	{
		DPRINTF("TODO: init IMCP protocol");	
	}
	else if(proto == PROTO_ICMPV6)	/* TODO: how to deal with imcpv6 */
	{
		DPRINTF("TODO: init IMCPv6 protocol");	
	}
	else
		return 0;

	pkt->saddr.port = sport;
	pkt->daddr.port = dport;

	return 1;		
}


static int packet_recv_init_arp(struct packet * pkt)
{

	u8 * data = packet_data(pkt);
	struct ethhdr  * ep = (struct ethhdr *)data;
	struct arphdr * ap =(struct arphdr*)(data + sizeof(struct ethhdr));

	pkt->family = AF_INET;
	pkt->protocol_ip = PROTO_NULL;
	
	
	pkt->saddr.ip.family = AF_INET;
	pkt->daddr.ip.family = AF_INET;
	pkt->saddr.ip.addr.ip4 = *((u32*)(ap->ar_sip));
	pkt->daddr.ip.addr.ip4 = *((u32*)(ap->ar_tip));

	pkt->transport_header = 0;
	pkt->network_header = (u8*)ap - data;
	pkt->mac_header = (u8*)ep - data;

	return 1;

}

static int packet_recv_init_ipv4(struct packet * pkt)
{
	u8 * data = packet_data(pkt);
	struct ethhdr  * ep = (struct ethhdr *)data;
	struct iphdr * ip ;
	void*  header ;
	ip =(struct iphdr*)(data + sizeof(struct ethhdr));

	pkt->family = AF_INET;
	pkt->protocol_ip = ip->protocol;

	header = (void*)((u8*)ip + sizeof(struct iphdr));
	
	if(__packet_init_port(pkt,header ,ip->protocol )==0)
		return 0;
	

	pkt->saddr.ip.family = AF_INET;
	pkt->daddr.ip.family = AF_INET;
	pkt->saddr.ip.addr.ip4 = ip->saddr;
	pkt->daddr.ip.addr.ip4 = ip->daddr;

	pkt->transport_header = (u8*)header - data;
	pkt->network_header = (u8*)ip - data;
	pkt->mac_header = (u8*)ep - data;

	return 1;
}


/*only support TCP/UDP/ICMP */
static int packet_recv_init_ipv6(struct packet * pkt)
{
	u8 * data = packet_data(pkt);
	struct ethhdr  * ep = (struct ethhdr *)data;
	struct ipv6hdr * ip6 ;
	void*  header ;

	ip6 =(struct ipv6hdr*)(data + sizeof(struct ethhdr));

	pkt->family = AF_INET6;
	pkt->protocol_ip = ip6->nexthdr;

	header = (void*)((u8 *)ip6 + sizeof(struct ipv6hdr));
	if(__packet_init_port(pkt, header, pkt->protocol_ip)==0)
		return 0;

	pkt->saddr.ip.family = AF_INET6;
	pkt->daddr.ip.family = AF_INET6;
	pkt->saddr.ip.addr.in6 = ip6->saddr;
	pkt->daddr.ip.addr.in6 = ip6->daddr;

	pkt->transport_header = (u8*)header - data;
	pkt->network_header = (u8*)ip6 - data;
	pkt->mac_header = (u8*)ep - data;

	return 1;
}




/* 
 * before process this packet, call this function to init packet fields 
 *  
 * this function init packet fields 
 *
 * packet::len have been inited by driver
 *
 * return:
 * 	0 fail
 * 	1 sucess	 
 * */
static int __packet_recv_init(struct packet * pkt)
{
	int ret = 0;
	u8 * data = packet_data(pkt);
	struct ethhdr  * ep = (struct ethhdr *)data;
	struct iphdr * ip ;

	pkt->lfq_state = LFQ_START; 
	pkt->protocol_eth = ep->h_proto;	

	ip =(struct iphdr*)(data + sizeof(struct ethhdr));
	
	/* make shure we can deal with IP/IPV6 extension header 
	 * this need test
	 * !!!make shure , using packet_get_tcphdr/udphdr is safe
	 * ipv4 , not support extension
	 *
	 * */

	if(PACKET_IS_IP(pkt)){
		if(ip->version == 4){ /* ipv4 , not support extension*/
			ret = packet_recv_init_ipv4(pkt);
		}else { /* ipv6 not tested TODO: test, not support extension  */
			ret = packet_recv_init_ipv6(pkt);
		}
	}
	else if(PACKET_IS_ARP(pkt))
	{
		ret = packet_recv_init_arp(pkt);
	}

	return ret;
}


/*
 * init packet and filter some unknow protocol packet
 * <head> packet list to be init and filter
 * <out> packet filtered, should be droped out 	
 * */
void packet_init_and_filter(struct list_head * head, struct list_head * drop_out )
{
	struct packet * pos;
	struct packet * n;
	int ret;

	list_for_each_entry_safe(pos, n, head, next)
	{
		ret = __packet_recv_init(pos);

		if(ret == 0)
		{	
			list_move_tail(&(pos->next),drop_out);
		}	
	}
}

