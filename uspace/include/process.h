/*
 * process.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.4
 * */


#ifndef _PROCESS_H
#define _PROCESS_H

#include "common/list.h"

void process_packets(struct list_head * packet_head, struct list_head * free_head);

#endif
