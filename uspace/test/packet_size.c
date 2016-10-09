#include <stdio.h>
#include "packet.h"

int main()
{
	unsigned long total = 1 <<30;
	
	unsigned long packet_size = (1024 *2);
	
	unsigned long num = total/packet_size;


	printf("sizeof(struct packet) = %d\n",sizeof(struct packet));
	
	printf("1GB packet buffer have %f W packet\n", num*1.0/10000);

	return 0;
}
