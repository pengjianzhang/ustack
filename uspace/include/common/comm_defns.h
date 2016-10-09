/*
 * comm_defns.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.4
 * */

/*
 * Put common used define in this header file
 * */


#ifndef _COMM_DEFNS_H
#define _COMM_DEFNS_H


/* ihpdr->protocol */
#define PROTO_NULL	0
#define PROTO_ICMP	88		/*TODO : find ICMP proto num */
#define PROTO_ICMPV6	99		/*TODO : find ICMPv6 proto num */
#define PROTO_TCP	6		/* in IP Hdr */
#define PROTO_UDP	17		/* in IP Hdr */

/* ethhdr->h_proto */
#define PROTO_IP	0x08		/* in eth Hdr */
#define PROTO_ARP	0x608		/* in arp Hdr */

#endif
