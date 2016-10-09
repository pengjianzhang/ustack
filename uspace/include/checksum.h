#ifndef _CHECKSUM_H
#define _CHECKSUM_H

#include "types/packet.h"

void checksum_ipv4(struct packet * pkt);

void checksum_udp(struct packet * pkt);


void checksum_tcp(struct packet * pkt);

#endif

