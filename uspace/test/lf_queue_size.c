#include "../lf_queue.h"


int main(int argc, char * argv[])
{
	int size = sizeof(struct lf_queue);	
	int total = 0;
	int nproc = 0;

	if(argc !=2 )
	{
		printf("./a.out nproc\n");

		return 1;
	}
	 
	nproc = atoi(argv[1]);
	total = size * nproc * nproc * 2;	

	printf("size %d B 2K = 2048 \n",size);
	printf("Total size %d  %f KB %f MB\n",total,total*1.0/1024, total*1.0/(1024*1024));

	return 0;
}
