#ifndef _ARP_H
#define _ARP_H

#include "types/packet.h"


int arp_handler(struct packet * pkt);

int arp_send_request(struct ip_address * addr,int nic_id);

#endif
