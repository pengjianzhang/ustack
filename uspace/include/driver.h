/*
 * filename 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.x
 * */



#ifndef _DRIVER_H
#define _DRIVER_H


#include "linux-net/if_ether.h"
#include "common/types.h"
#include "common/list.h"
#include "types/packet.h"
#include "types/driver.h"

/* ring type */
#define RX_FLAG	0
#define TX_FLAG	1

#define DESC_BLOCK_SIZE	4096	//each queue have 256's descripter,and these descripters are alloced together 

#define REG_BLOCK_SIZE	524288


#define DESC_NUM	256	/*igb ,82580 each ring have 256 desctipter */




struct driver * driver_module_init(int id);

int driver_alloc_rx_buffer(struct driver * d, struct packet_pool * pp);

int driver_close(struct driver * d);

int driver_recv(struct driver * d,struct list_head * head);

int driver_send(struct driver * d);

int driver_reclaim_tx_buffer(struct driver * d, struct list_head * head);

int driver_process_packets(struct driver * d, int (*process)(struct packet * p) );

int driver_put_packet_to_send_buffer(struct driver * d, struct packet * pkt);

static inline u8 *  driver_get_mac(struct driver * d, int adapter_idx)
{
	return d->adapters[adapter_idx]->mac;
}

static inline struct adapter *  driver_get_adapter(struct driver * d, int adapter_idx)
{
	return (d->adapters[adapter_idx]);
}


static inline int driver_get_adapter_num(struct driver * d)
{
	return d->adapter_num;
}

#endif

