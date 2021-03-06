 
static unsigned int K[] = 
{
0x6d5a56da,
0xdab4adb4,
0xb5695b68,
0x6ad2b6d1,
0xd5a56da2,
0xab4adb44,
0x5695b689,
0xad2b6d12,
0x5a56da25,
0xb4adb44a,
0x695b6895,
0xd2b6d12a,
0xa56da255,
0x4adb44ab,
0x95b68956,
0x2b6d12ad,
0x56da255b,
0xadb44ab6,
0x5b68956c,
0xb6d12ad8,
0x6da255b0,
0xdb44ab61,
0xb68956c3,
0x6d12ad87,
0xda255b0e,
0xb44ab61d,
0x68956c3b,
0xd12ad876,
0xa255b0ec,
0x44ab61d8,
0x8956c3b0,
0x12ad8761,
0x255b0ec2,
0x4ab61d84,
0x956c3b09,
0x2ad87612,
0x55b0ec24,
0xab61d848,
0x56c3b090,
0xad876120,
0x5b0ec241,
0xb61d8482,
0x6c3b0905,
0xd876120b,
0xb0ec2416,
0x61d8482c,
0xc3b09059,
0x876120b3,
0xec24167,
0x1d8482ce,
0x3b09059c,
0x76120b39,
0xec241672,
0xd8482ce4,
0xb09059c9,
0x6120b392,
0xc2416725,
0x8482ce4a,
0x9059c94,
0x120b3929,
0x24167253,
0x482ce4a7,
0x9059c94f,
0x20b3929e,
0x4167253d,
0x82ce4a7a,
0x59c94f5,
0xb3929ea,
0x167253d4,
0x2ce4a7a8,
0x59c94f50,
0xb3929ea1,
0x67253d43,
0xce4a7a87,
0x9c94f50e,
0x3929ea1d,
0x7253d43a,
0xe4a7a874,
0xc94f50e8,
0x929ea1d1,
0x253d43a3,
0x4a7a8747,
0x94f50e8e,
0x29ea1d1c,
0x53d43a38,
0xa7a87471,
0x4f50e8e3,
0x9ea1d1c7,
0x3d43a38f,
0x7a87471f,
0xf50e8e3e,
0xea1d1c7d,
0xd43a38fb,
0xa87471f6,
0x50e8e3ec,
0xa1d1c7d8,
};

unsigned int  compute_hash_bits(unsigned input, int bnum,unsigned result, int offset)
{
	int i,j;
	unsigned char byte;	

	for(i = 0; i < bnum/8; i++)
	{
		byte = (input &0xff);
		for(j = 0; j < 8; j++)
		{
			if((byte >> 7) == 1) 
				result ^= K[i*8 + j +offset];
			byte = byte << 1;
		}
		input = input >> 8;
	}	

	return result;	
}




/*IP PORT is network not host type */
int best_cpu(unsigned saddr, unsigned daddr, unsigned sport,unsigned dport)
{
	unsigned result = 0;
	unsigned queue = 0;	
	int i;	
	int flag = 0;

	result = compute_hash_bits(saddr,32,result,0);
	result = compute_hash_bits(daddr,32,result,32);
	result = compute_hash_bits(sport,16,result,64);
	queue = compute_hash_bits(dport,16,result,80);
	
	return (queue);// & 0x7;
}



int main()
{

	int a = best_cpu(188,299,88,984);
	printf("%x\n",a);
}
