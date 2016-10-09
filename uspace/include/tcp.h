

/*
 * tcp.h 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.1.x
 * */


#ifndef _TCP_H
#define _TCP_H

#include "types/packet.h"
#include "types/connection.h"

void tcp_states(struct packet * pkt);

int tcp_handler(struct packet * pkt);

int tcp_change_state(struct packet * pkt,struct connection * cp);


#define TCP_INIT	0
#define TCP_FIN1	1
#define TCP_FIN1_ACK	2
#define TCP_FIN2	3
#define TCP_FIN2_ACK	4
#define TCP_CLOSED	5


#endif
