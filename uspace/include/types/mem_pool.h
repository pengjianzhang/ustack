
#ifndef _TYPES_MEM_POOL_H
#define _TYPES_MEM_POOL_H


#include "common/list.h" 

struct mem_pool {
	struct list_head list;	/* list of all known pools */
	struct slist free_list;
	unsigned int size;	/* chunk size */
	unsigned int chunksize;
	int num;
	char name[32];		/* name of the pool */
};

#endif
