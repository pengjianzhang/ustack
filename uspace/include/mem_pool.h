/*
  Memory pool


*/

#ifndef _MEM_POOL_H
#define _MEM_POOL_H


#include "common/config.h"
#include "common/list.h" 
#include "types/mem_pool.h"

#define CHUNKSIZE	(1024*1024)	/*1M*/



struct mem_pool * mem_pool_create(char *name, unsigned int size);

void * mem_pool_alloc(struct mem_pool * p);


void  mem_pool_free(struct mem_pool * p, void * node);


#endif /* _COMMON_MEMORY_H */


