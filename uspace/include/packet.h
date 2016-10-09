/*
 * packet.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */



#ifndef _PACKET_H
#define _PACKET_H

#include "common/list.h"
#include "common/types.h"
#include "types/packet.h"
#include "linux-net/if_ether.h"
#include "linux-net/ip.h"
#include "linux-net/tcp.h"
#include "linux-net/udp.h"
#include "common/comm_defns.h"

#define PACKET_SIZE	2048
//#define PACKET_OFFSET	1600	
#define PACKET_OFFSET	1792


#define PACKET_IS_ARP(pkt)	((pkt)->protocol_eth == PROTO_ARP)
/* ipv6 and ipv4 is PROTO_IP */
#define PACKET_IS_IPV4(pkt)	((pkt)->family == AF_INET)
#define PACKET_IS_IPV6(pkt)	((pkt)->family == AF_INET6)
#define PACKET_IS_IP(pkt)	((pkt)->protocol_eth == PROTO_IP)
#define PACKET_IS_TCP(pkt)	((pkt)->protocol_ip == PROTO_TCP)
#define PACKET_IS_UDP(pkt)	((pkt)->protocol_ip == PROTO_UDP)

#define PACKET_IS_ICMP(pkt)	((pkt)->protocol_ip == PROTO_ICMP)

#define PACKET_IS_ICMPV6(pkt)	((pkt)->protocol_ip == PROTO_ICMPV6)


#define PACKET_ACCEPT	0 /* this packet continue  */
#define PACKET_DROP	1 /* this packet should be DROP */
#define PACKET_STOLEN	2 /* this packet is processed by some VS or some handler */




static inline struct packet * packet_of(__u8 * data)
{
	return (struct packet *) ((__u64)data + PACKET_OFFSET);
}


static inline u8 * packet_data(struct packet * pkt)
{
	return	(u8 *)((u64)pkt - PACKET_OFFSET);
}

//struct packet * packet_init(__u64 start, __u64 phys);

void packet_set_data_len(struct packet * pkt, __u16 len);

struct packet_pool * packet_pool_create();

void packet_pool_init(struct packet_pool * pp);

struct packet_zone *packet_zone_create();

void pool_add_packets(struct packet_zone * zone, struct packet_pool * pool);


void packet_init_and_filter(struct list_head * head, struct list_head * drop_out );


struct packet_zone * packet_module_init( __u64 start, __u64 phys, __u64 size, int nproc, int id);

static inline struct packet * packet_pool_get(struct packet_pool * pp)
{
	struct packet * pkt = NULL;
	struct list_head * list;

	if(pp->num >0)
	{
		list = pp->list.next;
		list_del(list);
		pp->num--;
		pkt = list_entry(list, struct packet, next);
	}

	return pkt;
}


static inline void packet_pool_put(struct packet_pool * pp, struct packet * pkt)
{
	list_add_tail(&(pkt->next),&(pp->list));
	pp->num++;
}



/*
 * get all the packet from <pp> to <head>
 * */
static inline void packet_pool_get_all(struct packet_pool * pp, struct list_head * head)
{
	if(pp->num > 0)
	{
		pp->num = 0;
		list_splice_tail_init(&(pp->list),head);
	}
}


/*
 * put all the packet from <head> to  <pp> 
 * */
static inline void packet_pool_put_all(struct packet_pool * pp, struct list_head * head)
{
	struct list_head * list;
	struct packet * pkt;

	while(!list_empty(head))
	{
		list = head->next;
		list_del(list);
		pkt = list_entry(list, struct packet, next);
		packet_pool_put(pp,pkt);
	}
}

static inline void packet_zone_reclaim(struct packet_zone * z, struct list_head * head)
{
		packet_pool_put_all(&(z->free_packet_pool),head);
}

static inline struct packet_pool * packet_zone_get_free_packet_pool(struct packet_zone * z)
{
	return &(z->free_packet_pool);
}

/*
static inline struct packet * get_free_packet(struct packet_zone * z)
{
	return packet_pool_get(&(z->free_packet_pool));
}


static inline void put_free_packet(struct packet_zone * z,struct packet * pkt)
{
	packet_pool_put(&(z->free_packet_pool),pkt);
}
*/

static inline u8 * packet_transport_header(struct packet *pkt)
{
	return (packet_data(pkt) + pkt->transport_header);
}

static inline u8 * packet_network_header(struct packet * pkt)
{
	return (packet_data(pkt) + pkt->network_header);
}


static inline u8 * packet_mac_header(struct packet * pkt)
{
	return (packet_data(pkt) + pkt->mac_header);
}


static inline struct ethhdr  * packet_get_ethhdr(struct packet * pkt)
{
	return (struct ethhdr  * )packet_mac_header(pkt);
} 

static inline struct arppack * paket_get_arppack(struct packet * pkt)
{
	return	(struct arppack * ) packet_network_header(pkt);
} 

static inline struct iphdr  * packet_get_iphdr(struct packet * pkt)
{
	return	(struct iphdr  * ) packet_network_header(pkt);
}

static inline struct ipv6hdr  * packet_get_ipv6hdr(struct packet * pkt)
{
	return	(struct ipv6hdr  * )packet_network_header(pkt);
}

static inline struct udphdr  * packet_get_udphdr(struct packet * pkt)
{
	return ( struct udphdr *) packet_transport_header(pkt);
}

static inline struct tcphdr  * packet_get_tcphdr(struct packet * pkt)
{
	return ( struct tcphdr *) packet_transport_header(pkt);
}


static inline u16 packet_get_dport(struct packet * pkt)
{
	return pkt->daddr.port;
}

static inline u16 packet_get_sport(struct packet * pkt)
{
	return pkt->saddr.port;
}

static inline struct ip_address * packet_get_sip(struct packet * pkt)
{
	return &(pkt->saddr.ip);
}

static inline struct ip_address * packet_get_dip(struct packet * pkt)
{
	return &(pkt->daddr.ip);
}


static inline u8 packet_get_transport_protocol(struct packet * pkt)
{
	return pkt->protocol_ip;
}


static inline struct packet * packet_of_phys(struct packet_zone * z, u64 phys )
{
	return packet_of((u8 *) (z->start_all + (phys - z->phys_all)));
}

static inline void packet_set_lfq_state(struct packet * pkt, enum lfq_state s)
{
	pkt->lfq_state = s;
}

static inline enum lfq_state packet_get_lfq_state(struct packet * pkt)
{
	return pkt->lfq_state;
}

#endif
