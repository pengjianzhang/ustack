#include "connection.h"
#include <stdio.h>

int main()
{
	int size = sizeof(struct connection);	

	printf("Connection Size = %d\n",size);


	printf("100W Connection need %f Mb\n", (size * 1000000.0)/(1024*1024));
	printf("1000W Connection need %f Mb\n", (size * 10000000.0)/(1024*1024));

	return 1;
}
