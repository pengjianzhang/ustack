/*
 * filename 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.x
 * */

/*
 * port_range store host-order ports
 *
 * */

#include "common/config.h"
#include "port_range.h"
#include <stdlib.h>

/* return an available port from range <range>, or zero if none is left */
static u16 port_range_get_port1(struct port_range *range)
{
	u16 ret;

	if (!range->avail)
		return 0;
	ret = range->ports[range->get];
	range->get++;
	if (range->get >= range->size)
		range->get = 0;
	range->avail--;
	return ret;
}

/* release port <port> into port range <range>. Does nothing if <port> is zero
 * nor if <range> is null. The caller is responsible for marking the port
 * unused by either setting the port to zero or the range to NULL.
 */
void port_range_put_port(struct port_range *range, u16  port)
{
	if (!port || !range)
		return;

	range->ports[range->put] = port;
	range->avail++;
	range->put++;
	if (range->put >= range->size)
		range->put = 0;
}

/* return a new initialized port range of N ports. The ports are not
 * filled in, it's up to the caller to do it.
 */
struct port_range *port_range_create(int n)
{
	struct port_range *ret;
	n++;
	ret = CALLOC(1, sizeof(struct port_range) +
		     n  * sizeof(((struct port_range *)0)->ports[0]));
	ret->size = n;
	return ret;
}


void port_range_free(struct port_range * range)
{
	FREE(range);
}

/* fill <range> with ports from <port1> to <port2>, <port1> and <port2> are also added in.
 * */
void port_range_init(struct port_range * range, u16 port1, u16 port2)
{
	u32 i;
	u32 up = port2;
	u32 low = port1;

	for(i = low ; i <= up ; i++)
	{
		port_range_put_port(range, htons(i));
	}
}


/*
 * get a specific <port> from <range>, <port> must less than  
 *
 * return 
 *	0 fail
 *	<port> success
 *
 * */
static u16 port_range_get_port2(struct port_range * range, u16 port)
{
	int i = range->get;	
	u16 tmp;	
	
	if(ntohs(port) > RESERVED_PORTS_MAX)
		return 0;

	while(i != range->put)
	{
		if(range->ports[i] == port)
		{
			/* swap <port> to range->get position  */
			tmp = range->ports[range->get];
			range->ports[i] = tmp;
			port_range_get_port1(range);
			return port;
		}
		i = (i + 1) % range->size;
	}

	return 0;
}

/*
 * <port>:	if <port> == 0, system alloc  a random port, is fast, should be use in SELD_IP mode
 * 		if <port> != 9, alloc this <port>, this method is time-cost, should be used in VS_IP mode
 *
 * return
 * 	0 fail
 * 	usefull port are retured
 * */

u16 port_range_get_port(struct port_range * range, u16 port)
{
	int ret;
	if( port == 0 )
		return port_range_get_port1(range);
	else
		return port_range_get_port2(range,port);
}
