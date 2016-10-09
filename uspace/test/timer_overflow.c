#include <stdio.h>

int main()
{
	unsigned long total = 1UL << 33;
	
	float min = total*1.0 / 60;
	float hour = min / 60;

	float day = hour / 24;
	
	float month = day / 30;
	
	float year = month / 12;

	printf("y %f m %f day = %f\n ", year,month, day);

	return 0;
}
