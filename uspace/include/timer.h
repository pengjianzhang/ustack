/*
 * timer.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.5
 * */



#ifndef _TIMER_H
#define _TIMER_H

#include "types/timer.h"

int timer_module_init(int seconds);


void timer_init(struct timer * t,int (*handler)(struct timer * t));

int timer_add(struct timer * t, int seconds);

void timer_del(struct timer * t);

void timer_expire();

#endif
