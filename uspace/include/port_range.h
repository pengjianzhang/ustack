/*
 * port_range.h
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.1
 * */



#ifndef _PORT_RANGE_H
#define _PORT_RANGE_H

#include "common/types.h"
#include "types/port_range.h"


#define DYNAMIC_PORTS_MIN	1024
#define DYNAMIC_PORTS_MAX	65535
#define DYNAMIC_PORTS_NUM	(DYNAMIC_PORTS_MAX - DYNAMIC_PORTS_MIN + 1)	

#define RESERVED_PORTS_MIN	1
#define RESERVED_PORTS_MAX	1023
#define RESERVED_PORTS_NUM	(RESERVED_PORTS_MAX - RESERVED_PORTS_MIN + 1 )



struct port_range *port_range_create(int n);

void port_range_free(struct port_range * range);

void port_range_init(struct port_range * range, u16 port1, u16 port2);

void port_range_put_port(struct port_range *range, u16  port);

u16 port_range_get_port(struct port_range * range, u16 port);

#endif 

