/*
 * hashtable.c 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.3
 * */



#include <stdlib.h>
#include "common/config.h"
#include "common/list.h"
#include "common/types.h"
#include "common/random.h"

u32 hash_rand;

struct list_head * hashtable_alloc(unsigned int size)
{
	struct list_head * table;
	table = (struct list_head *)MALLOC(size * sizeof(struct list_head));

	return table;
}

void hashtable_init(struct list_head * table, int size)
{
	unsigned i;

	for(i = 0; i < size; i++)
		INIT_LIST_HEAD(table + i);
}


void hashtable_free(struct list_head * table, int size)
{
	;
}



int hash_module_init()
{
	hash_rand = SEED();

	return 	1;
}

