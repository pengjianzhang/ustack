/*
 * tcp.c 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "linux-net/if_ether.h"
#include "linux-net/ip.h"
#include "linux-net/tcp.h"
#include "tcp.h"
#include "common/debug.h"
#include "packet.h"
#include "driver.h"
#include "route.h"
#include "checksum.h"
#include "connection.h"
#include "packet.h"



void tcp_states(struct packet * pkt)
{
	struct tcphdr  * tcp = packet_get_tcphdr(pkt);

	printf("seq %d ack_seq %d\n",tcp->seq,tcp->ack_seq);
	printf("res1\tdoff\tfin\tsyn\trst\tpsh\tack\turg\tece\tcwr\n");
	printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",tcp->res1,tcp->doff,tcp->fin,tcp->syn,tcp->rst,tcp->psh,tcp->ack,tcp->urg,tcp->ece,tcp->cwr);
	
}

int tcp_change_state(struct packet * pkt,struct connection * cp)
{
	struct tcphdr  * tcp = packet_get_tcphdr(pkt);

	if(tcp->fin && cp->state == TCP_INIT)	
		cp->state = TCP_FIN1;
	else	
	if(tcp->ack && cp->state == TCP_FIN1)	
		cp->state = TCP_FIN1_ACK;
	else	
	if(tcp->fin && cp->state == TCP_FIN1_ACK)	
		cp->state = TCP_FIN2;
	else
	if(tcp->ack && cp->state == TCP_FIN2)	
		cp->state = TCP_CLOSED;
	
	return  cp->state;	
}
