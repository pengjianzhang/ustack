/*
 * rss.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.3
 * */



#ifndef _RSS_H
#define _RSS_H

#include "driver.h"

int rss_hash_v4(unsigned saddr, unsigned daddr, unsigned sport,unsigned dport);

static inline u8 rss_hash_v4_idx(unsigned saddr, unsigned daddr, unsigned sport,unsigned dport)
{
	u8 idx = rss_hash_v4(saddr, daddr,sport,dport) & 0x7f;
	idx =  idx % (driver.ring_num);
	
	return idx;
}

#endif
