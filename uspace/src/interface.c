/*
 * interface.c 
 *
 * Copyright (C) 2012-2013 Peng Jianzhang
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2013.2.3
 * */


#include "interface.h"
#include "common/list.h"
#include "local_ip.h"

static void nic_init(struct nic * nic, int id)
{
	nic->id = id;
	
	INIT_LIST_HEAD(&(nic->self_list_ipv4));
	INIT_LIST_HEAD(&(nic->self_list_ipv6));
	INIT_LIST_HEAD(&(nic->vs_list_ipv4));
	INIT_LIST_HEAD(&(nic->vs_list_ipv6));
}	

/*
 * <num>: nic number
 *
 * */
struct interface * interface_module_init(int num, u16 vs_low,u16 vs_high,u16 self_low, u16 self_high)
{
	int i;
	struct ip_space * space;
	struct interface * itf = (struct interface *) MALLOC(sizeof(struct interface) + sizeof(struct nic)*num); 
	

	if(itf == NULL) return NULL;

	space = lip_ip_space_create(vs_low,vs_high,self_low,self_high);

	if(space == NULL)
	{
		FREE(itf);
		return NULL;
	}

	for(i = 0; i < num; i++)
	{
		nic_init(&(itf->nic[i]),i);		
	}		

	itf->num = num;
	itf->space = space;

	return itf;
}


static struct list_head * __get_list_head2(struct nic * nic, int is_vsip, int is_ipv4)
{
	struct list_head * head;

	if(is_vsip)
	{
		if(is_ipv4)
			head =&(nic->vs_list_ipv4);
		else
			head =&(nic->vs_list_ipv6);
	}
	else
	{
		if(is_ipv4)
			head =&(nic->self_list_ipv4);
		else
			head =&(nic->self_list_ipv6);
	}

	return head;
}


static struct list_head * __get_list_head3(struct interface * itf,int nic_id, int is_vsip, int is_ipv4)
{
	struct nic * nic;

	if(nic_id >= itf->num)
		return NULL;

	nic =&(itf->nic[nic_id]);

	
	return __get_list_head2(nic, is_vsip, is_ipv4);
}

void nic_add_lip(struct interface * itf,struct local_ip * lip)
{
	struct list_head * head;
	head = __get_list_head3(itf, lip->nic_id,
		lip_is_vsip(lip),lip_is_ipv4(lip));

	list_add(&(lip->nic_list),head);	
}


void nic_del_lip(struct interface * ift, struct local_ip * lip)
{
	list_del(&(lip->nic_list));
}


struct local_ip * interface_get_lip_rr(struct interface * itf,int nic_id, int is_vsip, int is_ipv4)
{
	struct local_ip * lip = NULL;
	struct list_head * head =  __get_list_head3(itf,nic_id, is_vsip, is_ipv4);

	if(!list_empty(head))
	{
		lip = list_first_entry(head, struct local_ip, nic_list);
		list_move_tail(&(lip->nic_list),head);
	}

	return lip;
}


struct local_ip * interface_lookup(struct interface * itf, struct ip_address * ip, int is_vs)
{
	return lip_ip_space_lookup(itf->space, ip, is_vs);
}


/*
 * create a local ip, and add it to interface 
 * */
struct local_ip * interface_create_local_ip(struct interface * itf, struct ip_address * ip,int type, int nic_id)
{

	struct local_ip * lip =  lip_ip_space_create_local_ip(itf->space,ip,type,nic_id);

	if(lip == NULL) return NULL;
		
	lip_ip_space_add(itf->space, lip);
	nic_add_lip(itf,lip);

	return lip;
}


/*
 * add a local ip to a interface
 * */
void interface_add_lip(struct interface * itf, struct local_ip * lip)
{
	lip_ip_space_add(itf->space, lip);
	nic_add_lip(itf,lip);
}


/*
 * del a local ip from interface 
 * 
 * */
void interface_del_lip(struct interface * itf, struct local_ip * lip)
{
	lip_ip_space_del(itf->space, lip);
	nic_del_lip(itf,lip);
}
