#include <stdio.h>
#include "common/types.h"
#include "common/debug.h"
#include "packet.h"


void debug_print_packet(struct packet * pkt)
{
	int i;
	unsigned char * skb = packet_data(pkt);

	for(i = 0; i < 64; i++)
	{
		printf("%d ",skb[i]);
	}

	printf("********\n");

	for(i = 0; i < 64; i++)
	{
		printf("%x ",skb[i]);
	}

	printf("\n\n");

}



