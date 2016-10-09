/*
 * types/timer.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */



#ifndef _TYPES_TIMER_H
#define _TYPES_TIMER_H

#include "common/list.h"

struct timer
{
	int (*handler)(struct timer * t);
	struct list_head list;
};



struct clock
{
	unsigned long cpu_frequency;	/*	*/
	unsigned long now;		/* each call timer_expire will update this value to current time */	
	unsigned long fill;
	unsigned long last_expire;	/* last expire time	*/
	int total_clock_seconds;
	int next_clock_second;
	struct list_head timer_table[0];
};


#endif
