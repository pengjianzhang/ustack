/*
 * driver.c
 *
 * nic driver at user space  
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2012.12.28
 * 		2013.2.10
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
#include "common/config.h"
#include "common/types.h"
#include "common/debug.h"
#include "linux-net/e1000_82575.h"
#include "linux-net/e1000_defines.h"
#include "linux-net/e1000_regs.h"
#include "linux-net/if_ether.h"
#include "packet.h"
#include "driver.h"


/*
 * Design
 *
 * a process may own a  (tx,rx) queue of  a adapter
 *
 * */

/* utility functions */
static inline union e1000_adv_rx_desc * ring_rx_desc(struct ring * r, int i)
{
	return ((union e1000_adv_rx_desc * )(r->desc)+i);
}

static inline union e1000_adv_tx_desc * ring_tx_desc(struct ring * r, int i)
{
	return ((union e1000_adv_tx_desc * )(r->desc)+i);
}

inline void writel(unsigned int data, unsigned int * addr)
{
	wmb();
	*((volatile unsigned long *)addr) = data;
}


/*******Init Function START************/




static struct ring * ring_create(struct adapter * a, u32 * tail_reg, void * desc, u32 desc_num, u8 id)
{
	int i;
	struct ring * r;
	
	if(desc_num <= 0 || a == NULL || tail_reg == NULL || desc == NULL) return NULL;
	r = (struct ring * )MALLOC(sizeof(struct ring)+sizeof(struct packet *)*desc_num);

	r->a = a;
	r->desc = desc;
	r->desc_num = desc_num;	/*256*/
	r->head = 0;
	r->tail = 0;
	r->queue_idx = id;
//	r->head_reg = NULL;
	r->tail_reg = tail_reg;

	packet_pool_init(&(r->pool));

	for(i = 0; i < r->desc_num; i++)
		r->buffer[i] = NULL;		

	return r;
}



static  void  adapter_queue_regs_desc_addr(struct adapter * a,u32 ** queue_tail_reg, void ** queue_desc, int is_rx)
{
	u32 * r_addr;
	void * d_addr;

	u32 desc_size = sizeof(union e1000_adv_tx_desc) * a->desc_num;

	if(is_rx)
	{
		r_addr = (u32 *)(a->regs_addr +  (unsigned long)E1000_RDT(a->queue_idx)); 
		d_addr = (void *)(a->desc_addr + (a->queue_num  + a->queue_idx)*desc_size);

	}
	else
	{
		r_addr = (u32 *)(a->regs_addr +  (unsigned long)E1000_TDT(a->queue_idx));
		d_addr = (void *)(a->desc_addr + (a->queue_idx)*desc_size);
	}

	*queue_tail_reg = r_addr;
	*queue_desc = d_addr;
}

static struct adapter * adapter_create(u32 id, u8 queue_idx, u8 queue_num, u64 regs_addr, u64 desc_addr, u32 desc_num, u8 * mac)
{
	struct adapter *  a =(struct adapter *) MALLOC(sizeof(struct adapter));
	u32 * reg_rx;
	void * desc_rx;
	u32 * reg_tx;
	void * desc_tx;

	a->id = id;
	a->queue_num = 	queue_num;
	a->desc_num = desc_num;
	a->queue_idx = queue_idx;
	a->regs_addr = regs_addr;
	a->desc_addr = desc_addr;
	memcpy(a->mac,mac,ETH_ALEN);

	adapter_queue_regs_desc_addr(a,&reg_rx, &desc_rx,1);
	adapter_queue_regs_desc_addr(a,&reg_tx, &desc_tx,0);
	

	a->rx_ring = ring_create(a,reg_rx,desc_rx,desc_num,queue_idx);
	a->tx_ring = ring_create(a,reg_tx,desc_tx,desc_num,queue_idx);

	return a;
}





/*
 *init driver , adapter , 
 *rx_ring , tx_ring, rx descripter,  tx descripter , rx register , tx  register 
 * 
 * */

static void driver_init(struct driver * d, u8 process_id)
{
	int i;
	d->id = process_id;
	d->fd = -1;
	d->dev = "/dev/ukmem";  
	d->adapter_num = 0;
	d->ring_num = 0;
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
	d->ring_num = t[3];

	d->desc_total_size = d->desc_size * d->adapter_num * d->ring_num *2;
	d->regs_total_size = d->regs_size * d->adapter_num ;	/* */

	printf("ring_num %ld\n",d->ring_num );

	regs = d->regs_addr = (u64)mmap64(0,d->regs_total_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	desc = d->desc_addr = (u64)mmap64(0,d->desc_total_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	d->buffer_addr  = (u64) mmap64(0, d->buffer_total_size ,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	
	for(i = 0; i < d->adapter_num; i++ )
	{
		a = adapter_create(i,d->id, d->ring_num, regs,desc, d->desc_num, (u8*)(&t[4 + i]));
		d->adapters[i] = a;	
		regs += d->regs_size;
		desc += d->desc_size * d->ring_num * 2;
	}
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


static struct driver * driver_create(int process_id)
{
	struct driver * d;

	d = (struct driver * )MALLOC(sizeof(struct driver));
	
	if(d == NULL) return NULL;

	driver_init(d,process_id);
	driver_open(d);

	return d;
}


struct driver * driver_module_init(int id)
{
	return driver_create(id);
}

/*******Init Function END************/


/*********Add empty packet to rx-queue Function START*******************/



static int ring_alloc_rx_buffer(struct ring * r,int i, struct packet_pool * pp)
{
	struct packet * pkt =  packet_pool_get(pp);
	union e1000_adv_rx_desc * rx_desc =  ring_rx_desc(r,i) ;

	if(pkt == NULL) return 0;

//	printf("ring_alloc_rx_buffer phys = %lx\n",pkt->phys);
//	printf("ring_alloc_rx_buffer rx_desc->read.pkt_addr = %lx\n",rx_desc->read.pkt_addr);

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
	ring_alloc_rx_buffers(a->rx_ring,pp);

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


/*********Add empty packet to rx-queue Function END*******************/



/*****************Recv Functions START*********************/
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
	//	pkt->queue_idx = r->queue_idx;	/* recv place*/
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
	return ring_recv(a->rx_ring);
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
		r = a->rx_ring;
		packet_pool_get_all(&(r->pool),head);
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


/*****************Recv Functions END*********************/



/******************Send Functions START******************************/

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
	return ring_send(a->tx_ring);
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



/*
 * put packet to a tx_ring buffer, according to packet's info
 *
 * */
int driver_put_packet_to_send_buffer(struct driver * d, struct packet * pkt)
{
	struct adapter * a =  driver_get_adapter(d,pkt->adapter_idx);
	struct ring * r = a->tx_ring;
	
	DPRINTF("Put packet %lx to adapter %d\n",pkt,pkt->adapter_idx);
	packet_pool_put(&(r->pool), pkt);

	return 1;
}


/******************Send Functions END******************************/

/******************Reclaim functions Start******************************/

/**
 * Reclaim resources after transmit completes
 * returns true if ring is completely cleaned
 *
 * <r>: ring to reclaim
 * <i>: relaim ring's tx_queue[<i>]
 * <head>: relaimed packet put to <head> list
 * return:
 * 	0: fail to reclaim, as packet not sended, don't reclaim at this entry 
 * 	1: sucess to reclaim
 **/

static int ring_reclaim_tx_buffer(struct ring * r,int i, struct list_head * head)
{
	int ret = 0;
	struct packet * pkt; 
	volatile union e1000_adv_tx_desc * tx_desc =  ring_tx_desc(r,i) ;

	if(tx_desc->wb.status & E1000_TXD_STAT_DD)
	{
		pkt = r->buffer[i];
		r->buffer[i] = NULL;
		tx_desc->wb.status = 0;


		if(pkt == NULL)
		{
			DPRINTF("ERROR appear\n");
			exit(-1);
		};
		
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
	return ring_reclaim_tx_buffers(a->tx_ring, head);
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


/******************Reclaim functions END******************************/





