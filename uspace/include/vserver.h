#ifndef _VSERVER_H
#define _VSERVER_H

#include "common/types.h"
#include "types/local_ip.h"
#include "types/vserver.h"



#define VS_IS_TCP(vs) (vs->protocol == PROTO_TCP)


int vserver_module_init();

struct vserver * vserver_lookup(struct ip_address * ip, u16 port, u8 protocol, struct local_ip ** vs_lip);

struct vserver *  vserver_create(struct server_pool * p, u16 port, u8 protocol, u8 is_fullnat);

void vserver_free(struct vserver * vs);

int vserver_add_ip(struct vserver * vs, struct local_ip * lip );

struct server * vserver_get_server(struct vserver * vs);

#endif

