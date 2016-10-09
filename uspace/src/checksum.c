/*
 * checksum.c
 *
 * a simple checksum
 *
 * TODO: more efficent , using linux kernel check  
 * 			 arch/x86/include/asm/checksum_64.h
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2012.12.26
 * */


#include <arpa/inet.h>
#include "common/types.h"
#include "linux-net/if_ether.h"
#include "linux-net/ip.h"
#include "linux-net/udp.h"
#include "linux-net/tcp.h"
#include "linux-net/ipv6.h"
#include "packet.h"


static u16 _checksum(u16 *buffer,int size, unsigned long init)  
{  
    unsigned long cksum=init;  
    while(size>1)  
    {  
        cksum+=*buffer++;  
        size-=sizeof(u16);  
    }  

    if(size)  
    {  
        cksum+=*(u8 *)buffer;  
    }  
    //将32位数转换成16 
    while (cksum>>16)  
        cksum=(cksum>>16)+(cksum & 0xffff);  

    return (u16) (~cksum);  
} 


static u16 checksum(u16 *buffer,int size, unsigned long init)  
{
	return _checksum(buffer, size, 0);
}

/*
 *包括源IP地址(4字节)、目的IP地址(4字节)、协议(2字节，第一字节补0)和TCP/UDP包长(2字节)
 *
 * */
static u16 checksum_tcpudp(u16 * buffer, int size, u32 * src, u32 * dst, int ip_len, u8 proto, u16 len)
{
	unsigned long cksum = 0;  
	u16 _proto = proto << 8;
	int i;
	
	for(i = 0; i < ip_len; i++)
	{
		cksum += src[i] >> 16;
		cksum += (src[i] & 0xffff);
		cksum += dst[i] >> 16;
		cksum += dst[i] & 0xffff;	
	}
	
	cksum += _proto;
	cksum += len;

	return _checksum(buffer, size, cksum);
}  



void checksum_ipv4(struct packet * pkt)
{
	char * data = packet_data(pkt);
	struct iphdr * ip =  (struct iphdr  * )(data + sizeof(struct ethhdr));
	
	ip->check = 0;

	ip->check = checksum((u16*)ip, sizeof(struct iphdr),0);
}

void checksum_udp(struct packet * pkt)
{
	u16 len;
	struct iphdr * ip =  packet_get_iphdr(pkt);
	struct ipv6hdr * ip6 =  packet_get_ipv6hdr(pkt);
	struct udphdr * udp =  packet_get_udphdr(pkt);
	u16 size;
	int ip_len;
	u32 * saddr;
	u32 * daddr;

	udp->check = 0;
	if(PACKET_IS_IPV4(pkt))
	{
		size = ntohs(ip->tot_len) - 20;
		len = htons (size);
		ip_len = 1;
		saddr = &(ip->saddr);
		daddr = &(ip->daddr);
	}
	else
	{
		len = ip6->payload_len;
		size = ntohs(len); 
		ip_len = 4;
		saddr = (u32*)&(ip6->saddr);
		daddr = (u32*)&(ip6->daddr);
	}

	
	udp->check = checksum_tcpudp((u16 *) udp, size, saddr,daddr,ip_len,ip->protocol,len);
}




void checksum_tcp(struct packet * pkt)
{
	u16 len;
	u16 size;
	struct iphdr * ip =  packet_get_iphdr(pkt);
	struct ipv6hdr * ip6 =  packet_get_ipv6hdr(pkt);
	struct tcphdr * tcp = packet_get_tcphdr(pkt);
	int ip_len;
	u32 * saddr;
	u32 * daddr;

	tcp->check = 0;
	if(PACKET_IS_IPV4(pkt))
	{
		size = ntohs(ip->tot_len) - 20;
		len = htons (size);
		ip_len = 1;
		saddr = &(ip->saddr);
		daddr = &(ip->daddr);
	}
	else
	{
		len = ip6->payload_len;
		size = ntohs(len); 
		ip_len = 4;
		saddr = (u32*)&(ip6->saddr);
		daddr = (u32*)&(ip6->daddr);
	}


	tcp->check = checksum_tcpudp((u16 *) tcp, size, saddr,daddr,ip_len,ip->protocol,len);
}

