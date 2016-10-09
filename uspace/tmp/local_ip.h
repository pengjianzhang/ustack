/*
 * local_ip.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.1
 * */


#ifndef _LOCAL_IP_H
#define _LOCAL_IP_H

#include "config.h"
#include "types.h"
#include "driver.h"
#include "list.h"
#include "hashtable.h"



/*
 * more simple design is 
 *
 * each ip with a port_rang and a protocol 
 *
 * so each ip with two local_ip( as 2 protocol TCP/UDP ) 
 *
 * */

/*
 * ip are devied into 2 types, VS IP, SELF IP
 * VS_IP	only used by vserver
 * SELF_IP	only used by proxy  connecting  pool/servers
 * */

struct local_ip 
{
	struct list_head list;		/*link in local_ip_hashtable*/

	struct list_head vs_list;		/* link in vs */

	union inet_address addr;	/* ip address, ipv4 or ipv6 */	
	struct adapter * a;		/* address bind on a nic adapter */
	struct vserver * vs;		/* a local ip bellong to a VS */
	u8 is_vsip;			/* VSIP, or SELF_IP */
	u8 protocol;			/*TCP or UDP */
	int user;			/* user ref count */
	struct port_range * range;	/* tcp or udp port range */
};



#define LOCAL_IP_HT_SIZE	1024
#define LOCAL_IP_HT_MASK	1023



int local_ip_module_init();

struct local_ip * local_ip_create();



void  local_ip_init(struct local_ip * lip, u32 ip, struct adapter * a);

void  local_ip_init2(struct local_ip * lip, u32 ip, struct adapter * a, u8 is_vsip,u8 protocol, struct vserver * vs);

void local_ip_free(struct local_ip * lip);


struct local_ip * local_ip_lookup(u32 ip, u8 protocol);


void local_ip_add(struct local_ip * lip);

void local_ip_del(struct local_ip * lip);


u16 local_ip_get_port(struct local_ip * lip,u16 port);


void local_ip_put_port(struct local_ip * lip, u16 port);


#endif

