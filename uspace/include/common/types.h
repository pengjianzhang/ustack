#ifndef _TYPES_H
#define _TYPES_H

#include <string.h>
#include <arpa/inet.h>
#include <linux/types.h>

typedef __u16	u16;
typedef __u32	u32;
typedef __u64	u64;
typedef __u8	u8;

#define IPV6_SIZE_BYTES	16
#define	IPV4_SIZE_BYTES	4

#define IPV6_SIZE_U32	4
#define IPV4_SIZE_U32	1

union inet_address {            
	u32           all[4];
	u32          ip4;
	u8	     b[16]; 	
	u32          ip6[4];
	struct in_addr  in4;
	struct in6_addr in6;
};

#ifndef AF_INET
#define AF_INET		4	
#endif
#ifndef AF_INET6
#define AF_INET6	10
#endif

struct ip_address{
	int family;		/* IPV4,IPV6 */
	union inet_address addr;
};


struct net_address
{
	struct ip_address ip;
	u16 port;
};

static inline int ip_address_eq(struct ip_address * ip1, struct ip_address * ip2)
{
	int ret = 0;

	if(ip1->family == ip2->family)
	{
		if(ip1->family == AF_INET)
		{
			if(ip1->addr.ip4 == ip2->addr.ip4)
				ret = 1;
		}
		else
		{
			if((ip1->addr.ip6[0] == ip2->addr.ip6[0])
			&&(ip1->addr.ip6[1] == ip2->addr.ip6[1])
			&&(ip1->addr.ip6[2] == ip2->addr.ip6[2])
			&&(ip1->addr.ip6[3] == ip2->addr.ip6[3]))
				ret = 1;
		}
	}


	return ret;
}

static inline void ip_address_copy(struct ip_address * dest, struct ip_address * src)
{
	memcpy((void*)dest,(void*)src,sizeof(struct ip_address));
}


static inline int ip_address_is_ipv4(struct ip_address * ip)
{
	return (ip->family == AF_INET);
}


static inline u32 ip_address_get_ipv4(struct ip_address * ip)
{
	return ip->addr.ip4;
}

static inline u32 * ip_address_get_ipv6(struct ip_address * ip)
{
	return ip->addr.ip6;
}

static inline void ip_address_init_v4(struct ip_address * ip, u32 ip4)
{
	ip->family = AF_INET;
	ip->addr.ip4 = ip4;
}

static inline void ip_address_init_v6(struct ip_address * ip, u32 * ip6)
{
	ip->family = AF_INET6;
	ip->addr.in6 = *((struct in6_addr *)(ip6));
}


static inline void net_address_init_v4(struct net_address * addr,char * ip, u16 port)
{
	u32 ip4 = inet_addr(ip);		
	ip_address_init_v4(&(addr->ip), ip4);
	addr->port = htons(port);	
}


static inline void net_address_init_v6(struct net_address * addr,char * ip, u16 port)
{
	struct in6_addr in6;
	inet_pton(AF_INET6,ip,(void*)&in6);		
	ip_address_init_v6(&(addr->ip), (u32*)&in6);
	addr->port = htons(port);	
}


#endif
