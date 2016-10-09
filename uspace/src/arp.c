/*
 * arp.c
 *
 * arp protocol
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2012.12.26
 * */

/*
 *Simple arp protocol 
 *One NIC address
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "common/types.h"
#include "common/debug.h"
#include "linux-net/if_arp.h"
#include "linux-net/if_ether.h"
#include "driver.h"
#include "route.h"
#include "packet.h"
#include "local_ip.h"
#include "net_global.h"
#include "server_pool.h"

#define ARP_REQUEST_OP	0x100
#define ARP_REPLY_OP	0x200

#define IS_ARP_REQUEST(pkt)
#define IS_ARP_REPLY(pkt)

static inline int is_arp_request(struct packet * pkt)
{
	struct arppack * ap =  paket_get_arppack(pkt);

	return (ap->ar_op == ARP_REQUEST_OP);
}

static inline int is_arp_reply(struct packet * pkt)
{
	struct arppack * ap =  paket_get_arppack(pkt);

	return (ap->ar_op == ARP_REPLY_OP);
}


/*
 * add server arp reply to route module, add MAC address to server struct
 *  <pkt>: arp reply
 *  return
 *  	0	fail
 *  	1	success	
 * */
/* arp example 
92 255 53 13 252 217
 0 27 33 133 135 204
 8 6 

0 1 
8 0 
6 
4 
0 2 
0 27 33 133 135 204 
192 168 2 34 
92 255 53 13 252 217 
192 168 2 234
	
5c ff 35 d fc d9 0 1b 21 85 87 cc 8 6 0 1 8 0 6 4 0 2 0 1b 21 85 87 cc c0 a8 2 22 5c ff 35 d fc d9 c0 a8 2 ea
*/
static  int arp_server_route(struct packet * pkt)
{
	struct arppack * ap =  paket_get_arppack(pkt);
	struct list_head * head;
	struct server * srv;
	struct ip_address * srv_ip;
	struct ip_address * pkt_sip = packet_get_sip(pkt);
	struct route_entry * re;
	int nic_id = pkt->adapter_idx;

	/* check if is server ip*/

	head = server_pool_get_server_list();
		
	for_each_server(srv,head) 
	{
		srv_ip = server_get_ip_address(srv);
		
		if( ip_address_eq(srv_ip,pkt_sip))
		{
			/* add route entry to route table */	
			
			re = ng_rt_lookup_with_nic_id(srv_ip, nic_id);
			if(!re)
			{
				re = rt_entry_create();
				rt_entry_init(re, srv_ip , nic_id);
				ng_rt_add_entry(re);
				/* flush servr mac address */
				server_set_mac(srv,ap->ar_sha);
			}
		}	
	}


	return 1;
}

/*
 * send arp request for <addr>
 * <addr>: ip address
 * <nic_id>: using this nic to send
 * return
 * 	0 send fail
 * 	1 send success	
 * */

 
static u8 all_mac[ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff};
static u8 zero_mac[ETH_ALEN] = {0,0,0,0,0,0};

int arp_send_request(struct ip_address * addr,int nic_id)
{
	struct packet * pkt ;
	struct ethhdr  * ep ;
	struct arppack * ap ;
	u8 *data;
	u8 * src_mac ;
	struct local_ip * lip;

	struct ip_address * src_ip ; 

	lip = ng_interface_get_vsipv4_rr(nic_id);
	if(!lip)
		lip = ng_interface_get_selfipv4_rr(nic_id);

	if(!lip) return 0;

	pkt = ng_get_free_packet();

	if(!pkt) return 0;

	data =  packet_data(pkt);
	ep = (struct ethhdr*)(data);
	ap = ( struct arppack *)(data + sizeof(struct ethhdr));
	src_mac =  ng_driver_get_mac(nic_id);

	/* set eth header */
	memcpy(ep->h_dest,all_mac,ETH_ALEN);
	memcpy(ep->h_source,src_mac,ETH_ALEN);
	ep->h_proto = PROTO_ARP;


	/* set arp pack */
	ap->ar_hrd = 0x0100;	/* 1 for ethhdr */
	ap->ar_pro = 0x0008;	/* 0x0800 for IP address */
	ap->ar_hln = 6;	/* 6 */
	ap->ar_pln = 4;	/* 4 */
	ap->ar_op = ARP_REQUEST_OP;	/* */
	memcpy(ap->ar_sha,src_mac,ETH_ALEN);	
	src_ip = lip_get_ip_address(lip);
	*(u32*)(ap->ar_sip) = src_ip->addr.ip4;

	memcpy(ap->ar_tha,zero_mac,ETH_ALEN);	
	*(u32*)(ap->ar_tip) = addr->addr.ip4;

	pkt->len = sizeof(struct arppack) + sizeof(struct ethhdr);
	pkt->adapter_idx = nic_id;
	ng_driver_put_packet_to_send_buffer(pkt);

	return 1;	
}



/*
 * 0 fail, caller should drop this packet
 * 1 sucess, caller should send this packet to NET
 *
 * */
static int arp_response(struct packet * pkt)
{
	struct ethhdr  * ep = packet_get_ethhdr(pkt);
	struct arppack * ap =  paket_get_arppack(pkt);
	u32 ip4 = *(unsigned int *)(&(ap->ar_tip));
	struct ip_address ip;

	ip_address_init_v4(&ip,ip4 );
 

	if(!ng_interface_lookup_ip( &ip)) return 0;
	else
	{
		DPRINTF("arp_response: %d %d %d %d\n",ip.addr.b[0], ip.addr.b[1],ip.addr.b[2],ip.addr.b[3]);
		DPRINT_PACKET(pkt);
	//	sleep(1);
	}	

	if(ap->ar_op != ARP_REQUEST_OP)  return 0;

	struct route_entry * re = ng_rt_lookup_rr(&ip);
	if(re == NULL) return 0;
	u8 * hwaddr =  ng_driver_get_mac(re->nic_id);
	unsigned tmp = *(unsigned int *)(&(ap->ar_tip));	
	/*set target hw addr, target ip*/
	memcpy(ap->ar_tha,ap->ar_sha,ETH_ALEN + 4);
		
	/*set sender hw addr, sender ip*/
	memcpy(ap->ar_sha,hwaddr,ETH_ALEN );
	*(unsigned int *)(ap->ar_sip) = tmp;
	
	ap->ar_op = ARP_REPLY_OP; 
	
	/*set ether header*/
	memcpy(ep->h_dest,ep->h_source,ETH_ALEN);
	memcpy(ep->h_source,hwaddr,ETH_ALEN);

	pkt->adapter_idx = re->nic_id;

	return sizeof(struct ethhdr ) + sizeof(struct arppack);
}

/*
 *<data> arp request
 * 
 *return:
 *	<data> arp  response
 * 	return 
 * 		0 error
 * 		>0 length of  response
 * 			
 *
 * */


int arp_handler(struct packet * pkt)
{

	if(is_arp_request(pkt))
	{
		if(arp_response(pkt))
		{
			DPRINTF("\nARTP RESPONSE:\n");	
			DPRINT_PACKET(pkt);

			ng_driver_put_packet_to_send_buffer(pkt);
			
			return PACKET_STOLEN;
		}
	}
	else if(is_arp_reply(pkt))	/* add arp reply message to route module */
	{
		arp_server_route(pkt);
	}
	//	printf("I send a arp , receive a response\n");
	//	printf("drop arp\n");

	
	return PACKET_ACCEPT;
}
