/*
 * include/types/net_global.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */


#ifndef _TYPES_NET_GLOBAL_H
#define _TYPES_NET_GLOBAL_H


#include "types/driver.h"
#include "types/packet.h"
#include "types/lf_queue.h"
#include "types/interface.h"
#include "types/route.h"

struct net_global
{
	int id;

	struct driver * d;
	struct interface *  itf;
	struct packet_zone * z;
	struct lfq * q;
	struct route_table * rt;
} ;


#endif
