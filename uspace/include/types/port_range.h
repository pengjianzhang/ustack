/*
 * include/types/port_range.h
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */



#ifndef _TYPES_PORT_RANGE_H
#define _TYPES_PORT_RANGE_H

#include "common/types.h"


struct port_range {
	int size, get, put;		/* range size, and get/put positions */
	int avail;			/* number of available ports left */
	u16 ports[0];		/* array of <size> ports, in host byte order */
};


#endif
