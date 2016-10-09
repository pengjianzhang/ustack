/*
 * process.c 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.4
 * */


#define _GNU_SOURCE
#define  __USE_GNU
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linux-net/udp.h"
#include "linux-net/tcp.h"
#include "linux-net/ipv6.h"
#include "linux-net/ip.h"
#include "linux-net/if_ether.h"
#include "common/types.h"
#include "common/debug.h"
#include "connection.h"
#include "local_ip.h"
#include "net_global.h"
#include "dispatch.h"
#include "driver.h"
#include "packet.h"
#include "arp.h"
#include "route.h"
#include "tcp.h"
#include "packet.h"
#include "server_pool.h"

static void process_set_tcp_port(struct connection * cp, struct packet * pkt, int is_client )
{
	struct tcphdr * tcp = packet_get_tcphdr(pkt);

	if(is_client)
	{
		if(CONNECTION_IS_FULLNAT(cp))	
		{
			tcp->source = cp->self_port;
		}
		tcp->dest = server_get_port(cp->server); 	
	}
	else
	{
		if(CONNECTION_IS_FULLNAT(cp))
			tcp->dest = cp->client_port;

		tcp->source = cp->vs->port;
	}
}


static void process_set_udp_port(struct connection * cp, struct packet * pkt, int is_client )
{

	struct udphdr * udp = packet_get_udphdr(pkt);

	if(is_client)
	{
		if(CONNECTION_IS_FULLNAT(cp))	
		{
			udp->source = cp->self_port;
		}
		udp->dest =  server_get_port(cp->server); 	
	}
	else
	{
		if(CONNECTION_IS_FULLNAT(cp))
			udp->dest = cp->client_port;
		udp->source = cp->vs->port;
	}
}


static void process_set_port(struct connection * cp, struct packet * pkt, int is_client)
{
	if(CONNECTION_IS_TCP(cp))
		process_set_tcp_port(cp,pkt,is_client );
	else
		process_set_udp_port(cp,pkt,is_client );
}

static void process_set_ipv4(struct connection * cp, struct packet * pkt, int is_client )
{
	struct iphdr  * ip4 =  packet_get_iphdr(pkt);


	if(is_client){
		if(CONNECTION_IS_FULLNAT(cp))	
			ip4->saddr = ip_address_get_ipv4(lip_get_ip_address(cp->self_addr));
		
		ip4->daddr = ip_address_get_ipv4(server_get_ip_address(cp->server)); 	
	}else{
		if(CONNECTION_IS_FULLNAT(cp))
			ip4->daddr = ip_address_get_ipv4( connection_get_client_ip(cp)); 
		
		ip4->saddr = ip_address_get_ipv4(lip_get_ip_address(cp->vs_addr));
	}		
}


static int process_set_ipv6(struct connection * cp, struct packet * pkt, int is_client )
{
	struct ipv6hdr  * ip6 = packet_get_ipv6hdr(pkt);

	if(is_client){
		if(CONNECTION_IS_FULLNAT(cp))	
			ip6->saddr = *((struct in6_addr*)ip_address_get_ipv6(lip_get_ip_address(cp->self_addr)));
		
		ip6->daddr = *(struct in6_addr*)ip_address_get_ipv6(server_get_ip_address(cp->server)); 	
	}else{
		if(CONNECTION_IS_FULLNAT(cp))
			ip6->daddr = *(struct in6_addr*)ip_address_get_ipv6( connection_get_client_ip(cp)); 
		
		ip6->saddr = *(struct in6_addr*)ip_address_get_ipv6(lip_get_ip_address(cp->vs_addr));
	}
}


static void process_set_ip(struct connection * cp, struct packet * pkt, int is_client)
{
	if(PACKET_IS_IPV4(pkt))
		process_set_ipv4(cp,pkt,is_client);
	else		
		process_set_ipv6(cp,pkt,is_client);
}

static void process_set_mac(struct connection * cp, struct packet * pkt, int is_client )
{
	struct ethhdr  * ep = packet_get_ethhdr(pkt);
	u8 * mac;

	if(is_client)
	{
		/* send to server */
	 	mac = ng_driver_get_mac(cp->server_adapter_idx);	
		memcpy(ep->h_source,mac,ETH_ALEN);
		memcpy(ep->h_dest,cp->server->mac,ETH_ALEN);
	}
	else
	{
		/* send to client */
		mac = ng_driver_get_mac(cp->client_adapter_idx);	
		memcpy(ep->h_source,mac,ETH_ALEN);
		memcpy(ep->h_dest,cp->client_mac,ETH_ALEN);
	}			

}


static void process_set_checksum(struct packet * pkt)
{
	if(PACKET_IS_IPV4(pkt))
		checksum_ipv4(pkt);

	if(PACKET_IS_TCP(pkt))
		checksum_tcp(pkt);
	else
		checksum_udp(pkt);	
}

/* 
 * ----------Client-->VS---------------------
 *
 * FULL-NAT:client-->vs-->server 
 * client_ip:client_port -->vs_ip:vs_port
 * local_ip:client_port -->server_ip:server_port 
 *
 * TRANSPARANT:client-->vs-->server 
 * client_ip:client_port -->vs_ip:vs_port
 * client_ip:client_port-->server_ip:server_port
 * 
 *----------Server->VS-------------------------
 * 
 * FULL-NAT Server-->VS-->Client
 * server_ip:server_port --> local_ip:client_port
 * vs_ip:vs_port --> client_ip:client_port
 *
 * TRANSPARANT:Server-->VS-->Client 
 * server_ip:server_port --> client_ip:client_port 
 * vs_ip:vs_port --> client_ip:client_port
 * */

int process_connection(struct connection * cp, struct packet * pkt, int is_client)
{
	process_set_port(cp,pkt,is_client);
	process_set_ip(cp,pkt,is_client);
	process_set_mac(cp,pkt,is_client);

	/*TODO queue number should be deside */
	if(is_client)	
		pkt->adapter_idx = cp->server_adapter_idx;
	else
		pkt->adapter_idx = cp->client_adapter_idx;

	process_set_checksum(pkt);
	/*
	if(is_tcp)
		tcp_states(pkt);
	*/
	
	ng_driver_put_packet_to_send_buffer(pkt);

	return PACKET_STOLEN;
}


/*
 * 1. get protocol, only process TCP/UDP 	
 * 2. get dest IP:PORT source IP:PORT 
 * 3. check is from client or server
 * 4. if is from client, check packet belongs to a VS
 * 	if not belongs a VS, drop it
 * 	if is belongs a VS
 * 		find the connection, if not find ,create a connection 
 *5. if is from server 	
 * 	find the connection, if not find drop it
 *
 * 6. if connection exists, modify packet,then xmit it
 *
 *
 * TODO: optimize the search algorithm to quick deside the connection
 *
 * */

int process_ip_packet(struct packet * pkt)
{
	struct ip_address * sip;
	struct ip_address * dip;
	u16 sport, dport;
	struct local_ip * lip ;
	struct local_ip * vs_lip ;
	struct vserver * vs;
	struct connection * cp = NULL;
	int is_client  = 0;
	u8 proto = packet_get_transport_protocol(pkt);


	sport = packet_get_sport(pkt);
	dport = packet_get_dport(pkt);
	sip = packet_get_sip(pkt);	
	dip = packet_get_dip(pkt);	

	lip = ng_interface_lookup_vsip(dip);

	/* packet to a  vs ip, may be client-->vs */
	if(lip){
		is_client = 1;	
		vs = vserver_lookup(dip, dport, proto, &vs_lip);

		DPRINTF("vs lookup %lx comming from client\n",vs);
		if(vs){
			DPRINTF("Find VS\n");
			is_client = 1;
			cp = connection_lookup(sip ,sport,proto,is_client);
			if(!cp){
				cp = connection_create(pkt,vs);
				DPRINTF("Creating Connection %lx\n",cp);
			}
		}else
			 return PACKET_DROP;
	}else /* comming from server*/{
		lip = ng_interface_lookup_selfip(dip);

		/* transporant mode, server-->client */
		if(lip == NULL) is_client = 1;	

		cp = connection_lookup(dip ,dport,proto,is_client);
		
		if(cp)
			DPRINTF("Coming from server\n");	
	}

	if(cp){
		if(process_connection(cp,pkt, is_client)){
			if(PACKET_IS_TCP(pkt)){

				if (tcp_change_state(pkt,cp) == TCP_CLOSED){
					DPRINTF("Find A TCP Close\n");
					connection_free(cp);
				}
			}
			return PACKET_STOLEN;
		}else
			return PACKET_DROP;
	}else
		return PACKET_ACCEPT;
}

/*
 * process a packet 
 * return:
 * 	PACKET_ACCEPT  no routing want to process this packet, leave it to caller, 
 * 	  		caller should put this packet to free_packet_pool
 *	PACKET_DROP	caller should drop this packet
 * 	PACKET_STOLEN  some routing process this packet 
 * */
static int main_process_packets(struct packet * pkt)
{
//	printf("A Packet is comming\n");

	if(PACKET_IS_IP(pkt))	/* is IP */
	{
//		DPRINTF("process a ip packet\n");
		return process_ip_packet(pkt);
	}
	else if(PACKET_IS_ARP(pkt))	
	{
		return arp_handler(pkt);
	}	

	return PACKET_DROP;
}

/*
 * process packets
 * dispached/init/filter shoud be done before calling this function, 
 * <packet_head>: packets list to process
 * <free_head>:	if a packet from <packet_head> should be drop , put it to <free_head>,socaller should free packets in <free_head>
 *
 * */
void process_packets(struct list_head * packet_head, struct list_head * free_head)
{
	struct packet * pos;
	struct packet * n;
	int ret;	
	int count = 0;

	list_for_each_entry_safe(pos, n, packet_head, next){

		list_del(&(pos->next));
		ret = main_process_packets(pos);

		if(ret != PACKET_STOLEN){
			list_add(&(pos->next),free_head);	
		}
	}	
} 
