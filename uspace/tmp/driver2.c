





static void ring_set_resource(struct ring * r, struct adapter * a)
{
	int i;
	__u64	regs_addr = a->regs_addr;
	__u64	desc_addr = a->desc_addr;
	u8 reg_idx = r->reg_idx;
	
	if(r->rx_tx_flag == RX_FLAG)
	{
		r->tail_reg = (u32 *)(regs_addr +  (unsigned long)E1000_RDT(reg_idx)); 
		r->desc = (void *)(desc_addr + (a->ring_num  + r->queue_index)*r->desc_size);
	}
	else
	{
		r->tail_reg = (u32 *)(regs_addr +  (unsigned long)E1000_TDT(reg_idx));
		r->desc = (void *)(desc_addr + (r->queue_index)*r->desc_size);
	}
}

/*
 * assign regs and descripter to adapters rings, which map from kernel
 * */
static void adapter_set_resource(struct adapter * a, u64 regs, u64 desc)
{
	int i;

	a->regs_addr = (__u64)regs;
	a->desc_addr = (__u64)desc;


	for(i = 0 ; i < a->ring_num; i++)
	{
		ring_set_resource(a->rx_ring[i],a);
		ring_set_resource(a->tx_ring[i],a);
	}	
}








