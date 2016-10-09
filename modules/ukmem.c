#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <asm/io.h>
#include <linux/skbuff.h>


#include <linux/miscdevice.h>

#define USTAK_VERSION "1.0"

MODULE_AUTHOR("peng jianzhang, <pengjianzhang@gmail.com>");
MODULE_DESCRIPTION("User space and kernel space memory share");
MODULE_LICENSE("GPL");
MODULE_VERSION(USTAK_VERSION);


#define	MAX_DESC_NUM 8 
#define MAX_ADAPTER_NUM 16	

struct adapter_mem
{
	unsigned long adapter_addr;	/*adapter address*/
	
	unsigned long regs_phys;	/*adapter register*/
	unsigned char mac[6];
	unsigned long rx_desc[MAX_DESC_NUM];	//physical address
	unsigned long tx_desc[MAX_DESC_NUM];	//physical address
	unsigned int rx_desc_num;
	unsigned int tx_desc_num;
};

struct ukmem
{
	struct adapter_mem adapter[MAX_ADAPTER_NUM];
	int adapter_num;
	int queue_num;	//each adapter start <queue_num> queues

	unsigned regs_pages;	/*register contains  page numbers */
	unsigned long desc_pages;		//a descripter contains page numbers

	/*alloced memory */
	unsigned long start;	/*kernel virtual address*/
	unsigned long phys;	/*physical address*/
	unsigned int  size;	/*allocated bytes*/
}ukmmap;



void ukmem_print(void)
{
	int i,j;
	struct adapter_mem * a;

	for(i = 0; i < ukmmap.adapter_num;i++)
	{

		a = &ukmmap.adapter[i];
		printk(KERN_INFO "adapter %d\n",i);
		printk(KERN_INFO "regs:%lx\n",a->regs_phys);

		for(j = 0; j < a->tx_desc_num; j++)
			printk(KERN_INFO "tx_desc:%lx",a->tx_desc[j]);

		for(j = 0; j < a->rx_desc_num; j++)
			printk(KERN_INFO "rx_desc:%lx",a->rx_desc[j]);

		printk(KERN_INFO "\n\n");
	}

	printk(KERN_INFO "get_free_pages:phys %lx\n",ukmmap.phys);

}


/*
 *regs_pages = 524288 Bytes/PAGE_SIZE = 128 pages 
 *desc_pages = 4096 Bytes/PAGE_SIZE = 1 page
 * return: 0 sucess, 1 fail
 * */
int ukmmap_init(int regs_pages, int desc_pages, int order)
{
	int i;
	unsigned long addr;

	ukmmap.adapter_num = 0;
	ukmmap.regs_pages = regs_pages;
	ukmmap.desc_pages = desc_pages;
	ukmmap.start = 0;
	ukmmap.phys = (1<<30);
	ukmmap.size = PAGE_SIZE*1024*100;

	for(i = 0; i < MAX_ADAPTER_NUM; i++)
	{
		ukmmap.adapter[i].rx_desc_num = 0;
		ukmmap.adapter[i].tx_desc_num = 0;
	}

	ukmmap.start = addr =(unsigned long) ioremap(ukmmap.phys,ukmmap.size);

	if(addr == 0){
		printk(KERN_INFO "get_free_pages error\n");
		return 1;
	}else{
		printk(KERN_INFO "ioremap OK~~\n");
	}

	return 0;
}

void ukmmap_exit(void)
{
	if(ukmmap.start)
	{
		//free_pages(ukmmap.start, ukmmap.order);
		;
	}
}


struct adapter_mem * ukmem_find_adapter(unsigned long adapter_addr)
{
	int i;
	struct adapter_mem * a;

	for(i = 0; i < ukmmap.adapter_num; i++)
	{
		a =&(ukmmap.adapter[i]);
		if(a->adapter_addr == adapter_addr) return a; 
	}

	return NULL;
}

void ukmem_alloc_adapter(unsigned long adapter_addr)
{
	int i = ukmmap.adapter_num;
	ukmmap.adapter[i].adapter_addr = adapter_addr;
	ukmmap.adapter_num++;
}
EXPORT_SYMBOL(ukmem_alloc_adapter);

void ukmem_add_regs(unsigned long adapter_addr,unsigned long phys)
{
	struct adapter_mem * a = ukmem_find_adapter(adapter_addr);

	if(a == NULL)
	{
		printk(KERN_INFO "ERROR: No such adapter %lx\n",adapter_addr);
		return ;
	}

	a->regs_phys = phys;
}
EXPORT_SYMBOL(ukmem_add_regs);


void ukmem_add_mac(unsigned long adapter_addr, unsigned char * mac)
{
	struct adapter_mem * a = ukmem_find_adapter(adapter_addr);

	if(a == NULL)
	{
		printk(KERN_INFO "ERROR: No such adapter %lx\n",adapter_addr);
		return ;
	}

	printk(KERN_INFO "Adapter %lx MAC %x %x %x %x %x %x\n",adapter_addr,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	
	memcpy(a->mac,mac,6);
}
EXPORT_SYMBOL(ukmem_add_mac);

#define TX_FLAG	0
#define RX_FLAG 1

void ukmem_add_desc(unsigned long adapter_addr,unsigned long desc_phys, int flag)
{

	struct adapter_mem * a = ukmem_find_adapter(adapter_addr);

	if(a == NULL)
	{
		printk(KERN_INFO "ERROR: No such adapter %lx\n",adapter_addr);
		return ;
	}

	if(flag == TX_FLAG)
		a->tx_desc[a->tx_desc_num++] = desc_phys;
	else
		a->rx_desc[a->rx_desc_num++] = desc_phys;
}


void ukmem_add_rx_desc(unsigned long adapter_addr,unsigned long desc_phys)
{
	ukmem_add_desc(adapter_addr,desc_phys,RX_FLAG);
}
EXPORT_SYMBOL(ukmem_add_rx_desc);

void ukmem_add_tx_desc(unsigned long adapter_addr,unsigned long desc_phys)
{
	ukmem_add_desc(adapter_addr,desc_phys,TX_FLAG);
}
EXPORT_SYMBOL(ukmem_add_tx_desc);



struct sk_buff * ukmem_skb_alloc(unsigned int size)
{
	return NULL;
}

EXPORT_SYMBOL(ukmem_skb_alloc);


void ukmem_skb_free(struct sk_buff * skb)
{
	return ;
}

EXPORT_SYMBOL(ukmem_skb_free);


void ukmem_set_queue_num(int num)
{
	ukmmap.queue_num = num;
}
EXPORT_SYMBOL(ukmem_set_queue_num);

static int ukmem_mmap_regs(struct file *file, struct vm_area_struct *vma)
{
	int i;
	unsigned long phys_addr;	
	unsigned long len;

	len = ukmmap.regs_pages * PAGE_SIZE;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); 

	for(i = 0; i < ukmmap.adapter_num; i++)
	{
		phys_addr = ukmmap.adapter[i].regs_phys;
		if(remap_pfn_range(vma,vma->vm_start +i* len,phys_addr >> PAGE_SHIFT,len,vma->vm_page_prot))
		{
			printk(KERN_INFO "mmap error\n");
			return -EAGAIN;
		}
	}

	return 0;
}

static int ukmem_mmap_desc(struct file *file, struct vm_area_struct *vma)
{
	int i,j;
	unsigned long phys_addr;	
	int len = ukmmap.desc_pages * PAGE_SIZE; 
	unsigned size = 0;


	for(i = 0; i < ukmmap.adapter_num; i++)
	{

		/*map tx first*/
		for(j = 0; j < ukmmap.adapter[i].tx_desc_num; j++)
		{
			phys_addr = ukmmap.adapter[i].tx_desc[j];
			if(remap_pfn_range(vma,vma->vm_start  + size,phys_addr >> PAGE_SHIFT,len,vma->vm_page_prot))
			{
				printk(KERN_INFO "mmap tx error\n");
				return -EAGAIN;
			}
			size += len;
		}

		/*then map rx*/
		for(j = 0; j < ukmmap.adapter[i].rx_desc_num; j++)
		{
			phys_addr = ukmmap.adapter[i].rx_desc[j];
			if(remap_pfn_range(vma,vma->vm_start  + size,phys_addr >> PAGE_SHIFT,len,vma->vm_page_prot))
			{
				printk(KERN_INFO "mmap rx error\n");
				return -EAGAIN;
			}
			size += len;
		}	
	}


	return 0;
}



static int ukmem_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long size = vma->vm_end - vma->vm_start;
	int ret = 0;
	unsigned long regs_size = ukmmap.adapter_num * ukmmap.regs_pages * PAGE_SIZE;
	unsigned long desc_size = ukmmap.adapter_num * ukmmap.desc_pages * PAGE_SIZE * ukmmap.adapter[0].rx_desc_num * 2;

	/*check size*/
	if( (vma->vm_pgoff != 0) ||(size % PAGE_SIZE != 0)) 
	{
		printk(KERN_INFO "mmap error 1\n");
		return  -EAGAIN;
	}
	

	if(size == regs_size)
		ret = ukmem_mmap_regs(file, vma);
	else if(size == desc_size)	
		ret = ukmem_mmap_desc(file, vma);
	else
	{
                if(remap_pfn_range(vma,vma->vm_start,ukmmap.phys >> PAGE_SHIFT,size,vma->vm_page_prot))
                {
                        printk(KERN_INFO "mmap ## error\n");
                        return -EAGAIN;
                }
	}

	return ret;
}



static int ukmem_open(struct inode *inode, struct file *file)
{

	printk(KERN_INFO "open\n");


	return 0;
}


static int ukmem_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "release\n");

	return 0;
}


/*
 * read:
 * 1.copy packet buffer physical address and length to userspace 
 * 2.copy adapter MAC address to userspace 
 * */
static ssize_t ukmem_read(struct file *file, char __user *buf, size_t size, loff_t *off)
{
	unsigned long buff[MAX_ADAPTER_NUM + 4] ;
	int err;
	int len_buff = sizeof(unsigned long) * (4+ ukmmap.adapter_num);
	int i;
	unsigned char * mac;

	buff[0] = ukmmap.phys;
	buff[1] = ukmmap.size;
	buff[2] = ukmmap.adapter_num;
	buff[3] = ukmmap.queue_num;

	for(i = 0; i < ukmmap.adapter_num; i++)
	{
			mac =(unsigned char *)(& (buff[4 + i]));
			memcpy(mac ,ukmmap.adapter[i].mac,6);
	}		

	if(size >= (len_buff))
	{
		err = copy_to_user(buf,buff,len_buff);
		if(err < 0) return err;
	}	

	return len_buff;
}


static ssize_t ukmem_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	printk(KERN_INFO "write\n");

	return 0;
}

static unsigned int ukmem_poll(struct file *file, struct poll_table_struct *wait)
{
	printk(KERN_INFO "poll\n");

	return 0;
}


static int ukmem_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	printk(KERN_INFO "ioctl\n");

	return 0;
}

static struct file_operations ukmem_ops = {
	.poll		= &ukmem_poll,
	.ioctl		= &ukmem_ioctl,
	.open 		= &ukmem_open,
	.release 	= &ukmem_release,
	.read		= &ukmem_read,
	.write		= &ukmem_write,
	.mmap 		= &ukmem_mmap,
	.owner 		= THIS_MODULE,
};

static struct miscdevice ukmem_dev =
{
	.minor		= 0,
	.name		= "ukmem",
	.fops		= &ukmem_ops,
};




static int __init
ukmem_init_module(void)
{
	printk(KERN_INFO "ukmem_init^_^\n");

	if(ukmmap_init(128,1,10)) return -EINVAL;

	if (misc_register(&ukmem_dev) < 0) {
		ukmmap_exit();
		printk(KERN_ERR "Failed to register ukmem_dev\n");
		return -EINVAL;
	}

	return 0;
}

module_init(ukmem_init_module);


static void __exit
ukmem_exit_module(void)
{
	printk(KERN_INFO "ukmem exit ~_~\n");

	ukmem_print();

	ukmmap_exit();
	misc_deregister(&ukmem_dev);
}

module_exit(ukmem_exit_module);
