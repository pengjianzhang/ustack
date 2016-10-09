
#include <stdio.h>

int main()
{
	int size = (1<<16) * 2 / (1024);

	printf("TCP/UDP ports size = %d KB\n", size);
	printf("TCP and UDP ports size = %d KB\n", size *2);

	printf("10 self IP with TCP and UDP ports size = %f MB\n", 10 * size * 2.0/1024);

	printf("100 self IP with TCP and UDP ports size = %f MB\n", 100 * size * 2.0/1024);


	return 0;
}
