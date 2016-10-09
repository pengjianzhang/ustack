/*
 * driver.c
 *
 * nic driver at user space  
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2012.12.28
 * */

#define _GNU_SOURCE
#define  __USE_GNU
#include <sched.h>
#include <linux/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/user.h>



#include "config.h"
#include "types.h"
#include <linux/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/user.h>


#include "e1000_82575.h"
#include "e1000_defines.h"
#include "e1000_regs.h"
#include "packet.h"
#include "driver.h"
#include "debug.h"
#include "if_ether.h"
#include "config.h"


inline void writel(unsigned int data, unsigned int * addr)
{
	wmb();
	*((volatile unsigned long *)addr) = data;
}


static void ring_init(struct ring * r,struct adapter * a, u16 desc_num, u8 id, u8 flag)
{
	int i;
	r->a = a;
	r->desc = NULL;
	r->desc_num = desc_num;	/*256*/
	r->desc_size = DESC_BLOCK_SIZE;
	r->head = 0;
	r->tail = 0;
	r->enable = 0;
	r->queue_index = id;
	r->reg_idx = id;
	r->rx_tx_flag = flag;
	
//	r->head_reg = NULL;
	r->tail_reg = NULL;

	packet_pool_init(&(r->pool));

	for(i = 0; i < r->desc_num; i++)
		r->buffer[i] = NULL;		
}

static struct adapter * adapter_create(u8 id, u16 ring_num, u16 desc_num, u8 * mac)
{
	int i;
	struct adapter *  a =(struct adapter *) MALLOC(sizeof(struct adapter));
	struct ring * r = (struct ring *)MALLOC(sizeof(struct ring)*ring_num*2);

	for(i = 0; i < ring_num; i++)
	{	
		ring_init(r,a,desc_num,i,TX_FLAG);
		a->tx_ring[i] = r++;
		ring_init(r,a,desc_num,i,RX_FLAG);
		a->rx_ring[i] = r++;
	}	

	a->id = id;
	a->enable = 0;
	a->ring_num = ring_num;
	a->desc_num = desc_num;
	a->regs_addr = 0;
	a->desc_addr = 0;
	
	memcpy(a->mac,mac,ETH_ALEN);

	return a;
}

static void ring_set_resource(struct ring * r, struct adapter * a)
{
	int i;
	__u64	regs_addr = a->regs_addr;
	__u64	desc_addr = a->desc_addr;
	u8 reg_idx = r->reg_idx;
	
	if(r->rx_tx_flag == RX_FLAG)
	{
		r->tail_reg = (u32 *)(regs_addr +  (unsigned long)E1000_RDT(reg_idx)); 
		r->desc = (void *)(desc_addr + (a->ring_num  + r->queue_index)*r->desc_size);
	}
	else
	{
		r->tail_reg = (u32 *)(regs_addr +  (unsigned long)E1000_TDT(reg_idx));
		r->desc = (void *)(desc_addr + (r->queue_index)*r->desc_size);
	}
}

/*
 * assign regs and descripter to adapters rings, which map from kernel
 * */
static void adapter_set_resource(struct adapter * a, u64 regs, u64 desc)
{
	int i;

	a->regs_addr = (__u64)regs;
	a->desc_addr = (__u64)desc;


	for(i = 0 ; i < a->ring_num; i++)
	{
		ring_set_resource(a->rx_ring[i],a);
		ring_set_resource(a->tx_ring[i],a);
	}	
}


/*
 *init driver , adapter , 
 *rx_ring , tx_ring, rx descripter,  tx descripter , rx register , tx  register 
 * 
 * */

static void driver_init(struct driver * d)
{
	int i;
	int cpu_num = sysconf(_SC_NPROCESSORS_ONLN);
	u16 ring_num = 8;

	if(ring_num > cpu_num) ring_num = cpu_num;

	d->fd = -1;
	d->dev = "/dev/ukmem";  
	d->adapter_num = 0;
	d->ring_num = ring_num;
	d->desc_num = DESC_NUM;
		
	d->desc_size = DESC_BLOCK_SIZE;
	d->regs_size = REG_BLOCK_SIZE; 

	d->desc_total_size = 0;
	d->regs_total_size = 0;	/* */
	d->buffer_total_size = 0;	/*1 G*/

	d->regs_addr = 0;
	d->desc_addr = 0;
	d->buffer_addr = 0;
	d->buffer_addr_phys = 0;

	for(i = 0; i < 32; i++)
		d->adapters[i] = NULL;
	
}


static void driver_open(struct driver * d)
{
	
	struct adapter * a;
	int fd ;
	u64 t[20];
	int count;
	int i;
	u64 regs;
	u64 desc;
	volatile char * addr;

	fd = open(d->dev, O_RDWR);
	d->fd = fd; 

	/* read packet buffer physical address , size, adapter mac address */
	count = read(fd,t,sizeof(u64)*20);
	d->buffer_addr_phys = t[0];
	d->buffer_total_size = t[1];
	d->adapter_num = t[2];
	d->desc_total_size = d->desc_size * d->adapter_num * d->ring_num *2;
	d->regs_total_size = d->regs_size * d->adapter_num ;	/* */

	printf("physical %lx\n",d->buffer_addr_phys );

	regs = d->regs_addr = (u64)mmap64(0,d->regs_total_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	desc = d->desc_addr = (u64)mmap64(0,d->desc_total_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	d->buffer_addr  = (u64) mmap64(0, d->buffer_total_size ,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);


#ifdef __DEBUG
/*
	u64 k;
	char * p = (char *)(d->buffer_addr);
	for(k = 0; k < d->buffer_total_size ; k++)
		p[k] = 0;
*/
#endif
	
	for(i = 0; i < d->adapter_num; i++ )
	{
		a  = adapter_create(i,d->ring_num, d->desc_num, (u8*)(&t[3 + i]));
		adapter_set_resource(a,regs,desc);
		d->adapters[i] = a;	
		regs += d->regs_size;
		desc += d->desc_size * d->ring_num * 2;
	}
}


static inline union e1000_adv_rx_desc * ring_rx_desc(struct ring * r, int i)
{
	return ((union e1000_adv_rx_desc * )(r->desc)+i);
}

static inline union e1000_adv_tx_desc * ring_tx_desc(struct ring * r, int i)
{
	return ((union e1000_adv_tx_desc * )(r->desc)+i);
}


/*
 *
 * */

static int ring_alloc_rx_buffer(struct ring * r,int i, struct packet_pool * pp)
{
	struct packet * pkt =  packet_pool_get(pp);
	union e1000_adv_rx_desc * rx_desc =  ring_rx_desc(r,i) ;

	if(pkt == NULL) return 0;

	rx_desc->read.pkt_addr = pkt->phys;
	rx_desc->read.hdr_addr = 0;
	r->buffer[i] = pkt;

	return 1;
}

/* 
 * alloc buffer to rx ring from <tail> position to <head> -1 ,
 *  and adjust <tail> position, 
 *  and finnaly set <tail_reg>   
 * 
 * before call this function , please adjust <head> first
 *
 *  return the number of alloced buffers
 * */
static int ring_alloc_rx_buffers(struct ring * r, struct packet_pool * pp)
{
	int i = r->tail;	
	int count = 0;
	int end = r->head;
	int size = r->desc_num;
	int ret;
	
	while( (i + 1)%size != end )
	{
		ret = ring_alloc_rx_buffer(r,i, pp);
		
		if(ret == 0) break;
		i = (i + 1) % size;
		count++;
	}	
	
	if(count > 0)
	{
		r->tail = i;
		writel(i,r->tail_reg);
	}

	return count;
}


static int adapter_alloc_rx_buffers(struct adapter * a, struct packet_pool * pp)
{
	int i;

	for(i = 0;  i < a->ring_num; i++)
		ring_alloc_rx_buffers(a->rx_ring[i],pp);

	return 1;
}

/*
 *get buffer from free_packet_pool
 * */
int driver_alloc_rx_buffer(struct driver * d, struct packet_pool * pp)
{
	int i;

	for(i = 0; i < d->adapter_num; i++)
		adapter_alloc_rx_buffers(d->adapters[i],pp);

	return 1;
}

/*
 * close driver , free data struct build from _driver_open()
 *
 * */
int driver_close(struct driver * d)
{
	close(d->fd);

	return 1;
}



/*
 *receive packets to r->pool
 * */
static int ring_recv_rx_buffer(struct ring * r,int i)
{
	struct packet * pkt = r->buffer[i];
	union e1000_adv_rx_desc * rx_desc =  ring_rx_desc(r,i) ;
	unsigned staterr;
	int length;

	staterr = rx_desc->wb.upper.status_error;
	if(staterr & E1000_RXD_STAT_DD)
	{
	//	DPRINTF("recv a package\n");

		length = rx_desc->wb.upper.length;
		pkt->len = length;
		rx_desc->wb.upper.status_error = 0;
		r->buffer[i] = NULL;
		pkt->adapter_idx = r->a->id;			
		pkt->queue_idx = r->queue_index;
		packet_pool_put(&(r->pool), pkt);

		return 1;
	}

	return 0;
}

static int ring_recv(struct ring * r)
{
	int i = r->head;	
	int count = 0;
	int end = r->tail;
	int size = r->desc_num;
	int ret;
	
	while( i != end )
	{
		ret = ring_recv_rx_buffer(r,i);
		if(ret == 0) break;
		i = (i + 1) % size;
		count++;
	}

	if(count > 0)
		r->head = i;

	return count;
}

static int adapter_recv(struct adapter * a)
{
	int i;
	int count = 0;

	for(i = 0;  i < a->ring_num; i++)
		count += ring_recv(a->rx_ring[i]);

	return count; 
}



static int driver_get_received_packets(struct driver * d, struct list_head * head)
{
	int i,j;
	
	struct adapter * a;
	struct ring * r;
	int ret;

	for(i = 0; i < d-> adapter_num; i++)
	{
		a = d->adapters[i];

		for(j = 0; j < a->ring_num; j++)
		{
			r = a->rx_ring[j];
			packet_pool_get_all(&(r->pool),head);
		}
	}

	return 1;
}


/*
 *recv packet from networks
 * */

int driver_recv(struct driver * d,  struct list_head * head)
{
	int count = 0;
	int i;

	for(i = 0; i < d->adapter_num; i++)
		count += adapter_recv(d->adapters[i]);
	
	driver_get_received_packets(d,head);

	return count;
}

static int ring_send_tx_buffer(struct ring * r,int idx)
{
	union e1000_adv_tx_desc * tx_desc = ring_tx_desc(r, idx);
	struct packet * pkt = packet_pool_get(&(r->pool));
	unsigned int  olinfo_status = 0, cmd_type_len;
	unsigned len;

	if(pkt == NULL) return 0;
	cmd_type_len = (E1000_ADVTXD_DTYP_DATA | E1000_ADVTXD_DCMD_IFCS |
			E1000_ADVTXD_DCMD_DEXT)| E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;

	r->buffer[idx] = pkt;
	
	len = pkt->len;
	if(len < 60) len = 60;
	olinfo_status |= ((len) << E1000_ADVTXD_PAYLEN_SHIFT);
 
	tx_desc->read.buffer_addr =  pkt->phys;
	tx_desc->read.cmd_type_len = cmd_type_len | (unsigned int)len;
	tx_desc->read.olinfo_status =  olinfo_status;

	return 1;
}


static int ring_send(struct ring * r)
{
	int i = r->tail;	
	int count = 0;
	int end = r->head;
	int size = r->desc_num;
	int ret;
	
	while( (i + 1) % size != end )
	{
		ret = ring_send_tx_buffer(r,i);
		if(ret == 0) break;
		i = (i + 1) % size;
		count++;
	}

	if(count > 0)
	{
		r->tail = i;
		writel(i,r->tail_reg);
	}

	return count;
}


static int adapter_send(struct adapter * a)
{
	int i;
	int count = 0;

	for(i = 0;  i < a->ring_num; i++)
		count += ring_send(a->tx_ring[i]);

	return count;
}



/*
 *send packet to networks
 *
 * */
int driver_send(struct driver * d)
{
	int i;
	int count = 0;

	for(i = 0; i < d->adapter_num; i++)
		count += adapter_send(d->adapters[i]);

	return count;
}


/**
 * Reclaim resources after transmit completes
 * returns true if ring is completely cleaned
 **/

static int ring_reclaim_tx_buffer(struct ring * r,int i, struct list_head * head)
{
	int ret = 0;
	struct packet * pkt; 
	volatile union e1000_adv_tx_desc * tx_desc =  ring_tx_desc(r,i) ;

//	assert(pkt != NULL);

	if(tx_desc->wb.status & E1000_TXD_STAT_DD)
	{
		pkt = r->buffer[i];
		r->buffer[i] = NULL;
		tx_desc->wb.status = 0;


		if(pkt == NULL)
		{
			DPRINTF("ERROR appear\n");
		};
//		put_free_packet(pkt);
		
		list_add_tail(&(pkt->next),head);
		ret = 1;
	}

	return ret;
}

/* 
 *
 *  return the number of reclaimed buffers
 * */
static int ring_reclaim_tx_buffers(struct ring * r,struct list_head * head)
{
	int i = r->head;	
	int count = 0;
	int end = r->tail;
	int size = r->desc_num;
	int ret;
	
	while( (i + 1)%size != end )
	{
		ret = ring_reclaim_tx_buffer(r,i,head);
		if(ret == 0) break;
		i = (i + 1) % size;
		count++;	
	}	
	
	if(count > 0)
		r->head = i;

	return count;
}


static int adapter_reclaim_tx_buffers(struct adapter * a,struct list_head * head)
{
	int i;
	int count = 0;

	for(i = 0;  i < a->ring_num; i++)
		count += ring_reclaim_tx_buffers(a->tx_ring[i], head);

	return count;
}

/*
 *get buffer from free_packet_pool
 * */
int driver_reclaim_tx_buffer(struct driver * d, struct list_head * head)
{
	int i;
	int count = 0;

	for(i = 0; i < d->adapter_num; i++)
		count += adapter_reclaim_tx_buffers(d->adapters[i],head);

	return count;
}


/*
 * 
 * put packet to a tx_ring buffer, according to packet's info
 *
 * */
int driver_put_packet_to_send_buffer(struct driver * d, struct packet * pkt)
{
	struct adapter * a =  driver_get_adapter(d,pkt->adapter_idx);
	struct ring * r = a->tx_ring[pkt->queue_idx];
	
	DPRINTF("Put packet %lx to adapter %d queue %d\n",pkt,pkt->adapter_idx, pkt->queue_idx);
	packet_pool_put(&(r->pool), pkt);

	return 1;
}



static void driver_adapter_queue_assign(struct driver * d)
{

}

struct driver * driver_module_init()
{
	struct driver * d = (struct driver * )MALLOC(sizeof(struct driver));

	if(d == NULL) return NULL;
	driver_init(d);
	driver_open(d);
	
	return d;
}
