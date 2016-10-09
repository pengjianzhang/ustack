/*
 * Memory pool functions.
 *
 */

#include <string.h>
#include "common/config.h"
#include "common/debug.h"
#include "common/list.h"
#include "mem_pool.h"

static struct list_head pools = LIST_HEAD_INIT(pools) ;


struct mem_pool * mem_pool_create(char *name, unsigned int size)
{
	struct mem_pool *pool;
	unsigned int align = 8;
	
	align = 8;
        size  = (size + align - 1) & -align;

	pool = CALLOC(1, sizeof(*pool));
	if (name)
		strncpy(pool->name, name, sizeof(pool->name));
	pool->size = size;
	pool->chunksize = CHUNKSIZE;
	slist_init(&(pool->free_list));

	list_add( &(pool->list), &pools);

	return pool;
}

/*
 * alloc CHUNKSIZE of memory at once, and put it in pool
 *
 * */
static int _mem_pool_alloc_chunk(struct mem_pool *pool)
{
	char * p;	
	char * start = (char*)MALLOC(pool->chunksize);
	int i, num;
	int size = pool->size;

	if(start == NULL )  return 0;

	num = pool->chunksize /size;

	for(i = 0; i < num; i++)
	{
		p = (start + i * size);
		slist_add((struct slist *)p, &(pool->free_list));
	}	

	pool->num += num;


	return num;
}


void * mem_pool_alloc(struct mem_pool * p)
{
	int ret;
	struct slist * node = slist_del_first(&(p->free_list));

	if(node == NULL)
	{
		ret = _mem_pool_alloc_chunk(p);
		if(ret == 0) return NULL;
		node = slist_del_first(&(p->free_list));
	}	
	
	if(node) p->num--;

	return (void*) node;
}



void mem_pool_free(struct mem_pool * p, void * node)
{
	slist_add((struct slist *)node,&(p->free_list));
	p->num++;
}
