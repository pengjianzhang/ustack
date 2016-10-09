#define _GNU_SOURCE
#define  __USE_GNU
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common/hashtable.h"
#include "common/list.h"
#include "common/types.h"
#include "common/debug.h"
#include "driver.h"
#include "process.h"
#include "local_ip.h"
#include "connection.h"
#include "vserver.h"
#include "packet.h"
#include "server_pool.h"
#include "lf_queue.h"
#include "dispatch.h"
#include "interface.h"
#include "net_global.h"
#include "route.h"
#include "test.h"




/*
 * init all the modules 
 * */
int init(int n, int id )
{

	struct driver * d;	
	struct lfq * q;
	unsigned long used;
	struct packet_zone * z;
	unsigned long addr,phys,size;
	struct interface * itf;
	u16 self_low,  self_high;
	struct route_table * rt;
	int nic_num ; 

	if((rt = rt_module_init()) == NULL)
		return 0;

	if(!hash_module_init())
		return 0;

	if(!dispatch_module_init(n,id))
		return 0;

	
	d = driver_module_init(id);
	if(d == NULL) return 0;
	
	dispatch_get_port_range(&self_low,&self_high);
	nic_num = driver_get_adapter_num(d);
	itf = interface_module_init(nic_num, 1,65535,self_low,self_high);

	if(itf == NULL) return 0;

	
	q = lfq_module_init(n, id,1024,(void *)d->buffer_addr);
	if(q == NULL) return 0;

	used = lfq_get_module_size(q);
	addr = d->buffer_addr+used;
	phys =  d->buffer_addr_phys+used;
	size = d->buffer_total_size - used;

	z = packet_module_init(addr,phys,size ,n,id);
	if(z == NULL) return 0;

	if(!timer_module_init(DEFAULT_SECONDS))
		return 0;

	if(!connection_module_init())
		return 0;

		
	if(!vserver_module_init())
		return 0;

	ng_init(id,d,z,q,itf,rt);

	return 1;
}


void usage()
{
	printf("Usage:./ukmem_uspace nproc");
	return;
}

int bind_cpu(int id)
{
	int ret = 1;
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(id, &mask);

	if (sched_setaffinity(0, sizeof(mask), &mask) == -1){
		printf("Error: could not set CPU affinity, continuing...\n");
		ret = 0;
	}

	return ret;
}

/* fork <n> child if n > 1
 * if <is_daemon> is set, daemonize this child process 
 * bind this processes on cpu
 * return: process id, from 0 .. <n - 1> 
 * */
int spawn(int n,int is_daemon)
{
	int ret ;
	int i;

	if(n == 1) return 0;

	int cpunum = sysconf(_SC_NPROCESSORS_ONLN);

	if(n > cpunum)
	{
		printf("Warrning: This system has %d cpus, can't fork %d processes\n",cpunum,n);
		n = cpunum;
 	}
	
	/* the father launches the required number of processes */
	for (i = 0; i < n; i++) {
		ret = fork();
		if (ret < 0) {
			printf("Cannot fork.\n");
			exit(1); /* there has been an error */
		}
		else if (ret == 0) /* child breaks here */
			break;
	}

	/* father is here*/
	if(i == n)
		exit(0);	
	
	if(is_daemon)	{
		fclose(stdin);
		fclose(stdout); 
		fclose(stderr);
	}


	if(!bind_cpu(i)){	
		printf("Error:Process %d can't bind on cpu %d\n",i,i);
		exit(-1);
	}

	return i; 
}



int main_loop()
{
	LIST_HEAD(recv_from_driver) ;
	LIST_HEAD(recv_from_dispatch) ;
	LIST_HEAD(to_reclaim) ;
	LIST_HEAD(free_head) ;
	
	/* before loop, find route to server */
	rt_find_server_route();

	struct packet_pool * pp =  packet_zone_get_free_packet_pool(net_global.z);

	while(1)
	{
		/* set free packet to rx queue */
		driver_alloc_rx_buffer(net_global.d,pp); 

		/* recv packet from net*/	
		driver_recv(net_global.d,&recv_from_driver);

		/* after recv packet from net, init these packet's field, and filter unknow protocol packets */
		packet_init_and_filter(&recv_from_driver, &free_head);
		
		/* dispatch packets to all processes, recv packet from other process */
		dispatch_packet(net_global.q,&recv_from_driver,&recv_from_dispatch,&free_head);

		/* process packet recved by this process */
		process_packets(&recv_from_driver, &free_head);			

		/* process packet recv from other process by dispatch */
		process_packets(&recv_from_dispatch, &to_reclaim);	

		/* get packet from tx queue */
		driver_reclaim_tx_buffer(net_global.d,&to_reclaim);

		/* return <to_reclaim> to other process for reclaim, iff packet not belongs to this process 
		 * so the remaining packet shoube be reclaim by this process
		 * */
		dispatch_reclaim(net_global.q,&to_reclaim);
		
		packet_zone_reclaim(net_global.z, &free_head);
		packet_zone_reclaim(net_global.z, &to_reclaim);
		
		driver_send(net_global.d);
		dispatch_flush(net_global.q);

		timer_expire();
	}

	lfq_free(net_global.q);
	driver_close(net_global.d);
}


int start_kernel_module()
{

}

int main(int argc, char * argv[])
{
	int ret ;
	int nproc = 1;
	int id = 0;
	int is_daemon = 0;
	int cpu_num ;

	if(argc != 2){
		usage();
		return 1;
	}

	nproc = atoi(argv[1]);

	id = spawn(nproc,is_daemon);
/*
	if(id == 1)
	{
		DPRINTF("Sleep id = %d\n",id);
		sleep(1000);
	} 
*/
	if(!init(nproc,id))
		return -1;
	
	if(!test_arp()) 
		return -1;

/*
	if(!test_case())
		return -1;
*/

	main_loop();

	return 1;
}
