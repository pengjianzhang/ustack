/*
 * timer.c 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.5
 * */



#include <stdlib.h>
#include "common/random.h"
#include "common/list.h"
#include "common/debug.h"
#include "common/config.h"
#include "timer.h"


static struct clock * __clock;

/*
 * TCP timer must be more precise, such as 1 ms
 *
 * Overflow time > 100 year, so don't consider overflow
 *
 * */

unsigned long CLOCK()
{
        union
        {
                unsigned long now;
                unsigned _now[2];
        }       t;

        __asm__ __volatile__("rdtsc" : "=a" (t._now[0]), "=d" (t._now[1]));

        return (t.now >> 20);
}


int timer_module_init(int seconds)
{
	int i;
	int size;
	unsigned long t = CLOCK();

	if(seconds < 0)
		size  = DEFAULT_SECONDS;
	else
		size = seconds;

	__clock  = (struct clock * )CALLOC(1,sizeof(struct clock)+sizeof(struct list_head)*size);

	if(__clock == NULL) return 0;

	__clock->cpu_frequency = DEFAULT_CPU_FREQUENCY_MHZ;
	__clock->last_expire = __clock->now = t;
	__clock->total_clock_seconds = size;	
	__clock->next_clock_second = 0;	
	__clock->fill = 0;

	for(i = 0; i < size; i++)
		INIT_LIST_HEAD(&(__clock->timer_table[i]));

	return 1;
}

void timer_init(struct timer * t,int (*handler)(struct timer * t))
{
	INIT_LIST_HEAD(&(t->list));
	t->handler = handler;
}

/*
 * add <t> to timer_table, this timer<t> expect to be alarmed at <seconds> seconds later
 *<t>
 * */
int timer_add(struct timer * t, int seconds)
{
	int id;

	if(seconds > __clock->total_clock_seconds) 
		seconds = __clock->total_clock_seconds;
	if(seconds < 0)
		seconds = 1;	

	id = (__clock->next_clock_second + seconds)	% __clock->total_clock_seconds;
	
	list_add(&(t->list), &(__clock->timer_table[id]));

	return seconds;
}

/*
 * remove time <t> from timer_table,make shure this timer is in timer_table 
 * */
void timer_del(struct timer * t)
{
	if(!list_empty(&(t->list)))
	{	
		list_del(&(t->list));
		INIT_LIST_HEAD(&(t->list));
	}
}

/*
 * expire timers which is expired
 * handler shoubd del timer, this function dose not del timmer 
 *
 * */
void timer_expire()
{
	unsigned long now = CLOCK();
	unsigned long elapse; 
	int flag = 0;
	struct timer * pos;
	struct timer * n;
	

	elapse = (now - __clock->last_expire +  __clock->fill);	

	if(elapse > __clock->cpu_frequency) 
		flag = 1;

	while(elapse > __clock->cpu_frequency)
	{
	 	__clock->next_clock_second = ( __clock->next_clock_second + 1);

		if( __clock->next_clock_second >= __clock->total_clock_seconds) 
			__clock->next_clock_second = 0 ;
		
		list_for_each_entry_safe(pos, n, &(__clock->timer_table[__clock->next_clock_second]), list)
		{
			pos->handler(pos);
		}

//		DPRINTF("expire %d\n", __clock->next_clock_second );		
	
		elapse -=  __clock->cpu_frequency;
	}

	if(flag)
	{	
		__clock->fill =  elapse;
		__clock->last_expire = now;
	}

	__clock->now = now;
}
