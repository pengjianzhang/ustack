#include "driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process.h"
#include "common/types.h"
#include "local_ip.h"
#include "connection.h"
#include "vserver.h"
#include "packet.h"
#include <string.h>
#include "server_pool.h"
#include "net_global.h"


void static_route(char * ip_string, int nic_id )
{
	struct ip_address ip;
	u32 ip4 = inet_addr(ip_string);		
	ip_address_init_v4(&ip,ip4);
	struct route_entry * re = rt_entry_create();
	rt_entry_init(re, &ip , nic_id);
	ng_rt_add_entry(re);
}

int test_arp()
{
	struct local_ip * lip_vs1;
	struct local_ip * lip_vs2;

	ng_add_ip("192.168.2.36",AF_INET,0,0);
	ng_add_ip("192.168.2.31",AF_INET,0,1);
	ng_add_ip("192.168.2.32",AF_INET,0,2);
	ng_add_ip("192.168.2.33",AF_INET,0,3);

	lip_vs1 = ng_add_ip("192.168.2.34",AF_INET,1,0);
	lip_vs2 = ng_add_ip("192.168.2.35",AF_INET,1,0);

	
	struct server_pool * pool =  server_pool_create();
	struct net_address  addr;
	net_address_init_v4(&addr,"192.168.2.87", 80);
	struct server * srv = server_pool_add_server(pool, &addr);
	u8 mac[10] = {0x00,0x26,0x2D,0x0B,0x79,0x02};
	server_set_mac(srv,mac);



	struct vserver * vs =  vserver_create(pool, htons(7788), PROTO_TCP, 1);

	if(!vserver_add_ip(vs,lip_vs1))
	{
		printf("ERROR: add lip to vs\n");
		exit(-1);
	}
	vserver_add_ip(vs,lip_vs2);

//	static_route("192.168.2.87",0);


	return 1;
}

/*
int test_case_87()
{
	struct vserver * vs; 
	u16 vs_port = htons(80);
	// create some local ip 
	
	u32 server_ip = inet_addr("192.168.2.87");
	u16 server_port = htons(80); 
	u8 server_mac[6] = { 0x00,0x07,0xE9,0x18,0xD1,0x2E};

	struct adapter * a =  driver_get_adapter(3);

	u32 ip1 = inet_addr("192.168.2.31");	
	u32 ip2 = inet_addr("192.168.2.32");	
	u32 ip3 = inet_addr("192.168.2.33");	
	struct local_ip * lip1 =  local_ip_create(ip1,a );
	struct local_ip * lip2 =  local_ip_create(ip2,a );
	struct local_ip * lip3 =  local_ip_create(ip3,a );

	// create a server pool 

	struct server_pool * pool =  server_pool_create();
	
	if(pool == NULL)
	{
		printf("Create pool Error\n");
		exit(-1);
	}

	if(!server_pool_add_server(pool, server_mac,server_ip,server_port))
	{
		printf("Add Server Error\n");
		exit(-1);
	}

	// create a vserver 	
	
	vs = vserver_create(pool, vs_port, PROTO_UDP,1);
	
	if(!vserver_add_ip( vs,lip1))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}
	if(!vserver_add_ip( vs,lip2))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}
	if(!vserver_add_ip( vs,lip3))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}

	// create a vserver 
	
	vs = vserver_create(pool, vs_port, PROTO_TCP,1);
	
	if(!vserver_add_ip( vs,lip1))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}
	if(!vserver_add_ip( vs,lip2))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}
	if(!vserver_add_ip( vs,lip3))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}


	return 1;
}



int test_case()
{
	struct vserver * vs; 
	u16 vs_port = htons(333);
	// create some local ip 
	
	u32 server_ip = inet_addr("192.168.2.245");
	u16 server_port = htons(4567); 
	u8 server_mac[6] = { 0x00,0x07,0xE9,0x18,0xD1,0x2E};

	struct adapter * a =  driver_get_adapter(3);

	u32 ip1 = inet_addr("192.168.2.31");	
	u32 ip2 = inet_addr("192.168.2.32");	
	u32 ip3 = inet_addr("192.168.2.33");	
	struct local_ip * lip1 =  local_ip_create(ip1,a );
	struct local_ip * lip2 =  local_ip_create(ip2,a );
	struct local_ip * lip3 =  local_ip_create(ip3,a );

	// create a server pool 

	struct server_pool * pool =  server_pool_create();
	
	if(pool == NULL)
	{
		printf("Create pool Error\n");
		exit(-1);
	}

	if(!server_pool_add_server(pool, server_mac,server_ip,server_port))
	{
		printf("Add Server Error\n");
		exit(-1);
	}

	// create a vserver 	
	
	vs = vserver_create(pool, vs_port, PROTO_UDP,1);
	
	if(!vserver_add_ip( vs,lip1))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}
	if(!vserver_add_ip( vs,lip2))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}
	if(!vserver_add_ip( vs,lip3))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}

	// create a vserver 
	
	vs = vserver_create(pool, vs_port, PROTO_TCP,1);
	
	if(!vserver_add_ip( vs,lip1))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}
	if(!vserver_add_ip( vs,lip2))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}
	if(!vserver_add_ip( vs,lip3))
	{
		printf("VS Add Flipper IP  Error\n");
		exit(-1);
	}


	return 1;
}
*/
