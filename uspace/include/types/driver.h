/*
 * types/driver.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.11
 * */



#ifndef _TYPES_DRIVER_H
#define _TYPES_DRIVER_H

#include "common/types.h"
#include "types/packet.h"
#include "linux-net/if_ether.h"


struct ring {
	struct adapter 	* a;
	void *desc;                    /* descriptor ring memory */
	u32 desc_num;			/* number of desc  256 */
	u32 head;			/* buffers are set  bewteen head <--> tail */
	u32 tail;			/* head tail crespone to head_reg,tail_reg in NIC device */

//	u32 * head;			/* head register */
	u32 * tail_reg;			/* tail register */

	u8 queue_idx;	
	struct packet_pool pool;	

	struct packet * buffer[0];	/* */	
};

/* board specific private data structure */
struct adapter {
	u32 id;		/* adapter id */
	u8 mac[ETH_ALEN]; /*mac address */	
	/* TX */
	struct ring *tx_ring;
	/* RX */
	struct ring *rx_ring;

	u8 queue_num;
	u32 desc_num;
	u8 queue_idx;
	u64	regs_addr;
	u64	desc_addr;
};



struct driver
{
	u8 id;		/* process id, adapter on this process, have this queue id */
	int fd;
	char * dev;  
	struct adapter * adapters[32]; /* array of adapters*/

	u8 adapter_num;
	u8 ring_num;
	u16 desc_num;
	
	u64 desc_total_size;
	u64 regs_total_size;
	u64 buffer_total_size;

	u32 desc_size;
	u32 regs_size; 

	u64 regs_addr;
	u64 desc_addr;
	u64 buffer_addr;
	u64 buffer_addr_phys;	
};



#endif

