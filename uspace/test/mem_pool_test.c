

#include <stdio.h>
#include "../mem_pool.h"

#include "../mem_pool.c"




void * array[10240];


void test_alloc(struct mem_pool * t)
{
	int i;
	void * p; 

	for(i = 0; i < 10240; i++)
	{
		p = mem_pool_alloc(t);
		array[i] = p;
		((long * )p)[1] = i;
	}
}



void test_free(struct mem_pool * t)
{
	int i;

	for(i = 0; i < 10240; i++)
	{
		mem_pool_free(t,array[i]);
	}
}



int main()
{
	struct mem_pool * t  = mem_pool_create("test", 32);

	printf("size %d\n", t->size);

	test_alloc(t);
	test_free(t);
	test_alloc(t);
	test_free(t);
	test_alloc(t);
	test_free(t);
	test_alloc(t);
	test_free(t);
	

	return 0;
}

