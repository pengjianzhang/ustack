/*
 * include/types/dispatch.h
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */

#ifndef _TYPES_DISPATCH_H
#define _TYPES_DISPATCH_H

#include "common/types.h"


struct dispatch
{
	int id;
	int num;
	u16 port_base;
	u16 port_slice;

	u16 self_port_low;
	u16 self_port_high;
};

#endif

