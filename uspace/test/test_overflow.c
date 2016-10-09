#include "../timer.c"


int main()
{

	unsigned long a,b;

	a = CLOCK();

	while(1)
	{

		sleep(1);

		
		b = CLOCK();

		printf("%lx\n",b);
		if( a > b)
		{
			printf("Overflow before = %lx  now = %lx\n", a,b);
			break;
		}
	}

	return;
}
