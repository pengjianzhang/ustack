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


#define SK_BUFF_OFFSET (sizeof(struct sk_buff) * 7)
#define CPU_NUM 2


struct kmem{
	unsigned long start;	/*kernel virtual address*/
	unsigned long phys;	/*physical address*/
	unsigned int order;	/*size: page number*/
	unsigned int index;
}kmem[CPU_NUM];


#define	DESC_LEN 32 
unsigned long desc[DESC_LEN];
int desc_index = 0;
int desc_size = (PAGE_SIZE * 16 ) ;

#define ADAPTER_NUM	4
struct nic_regs
{
	unsigned long phys;
	unsigned long virt;
	unsigned long len;
}regs[ADAPTER_NUM];
int regs_index = 0;
unsigned long regs_size = 0;

void ukmem_add_regs(unsigned long virt,unsigned long phys, unsigned long len)
{
	regs[regs_index].phys = phys;
	regs[regs_index].virt = virt;
	regs[regs_index].len = len;

	regs_size += len;
	regs_index++;
}
EXPORT_SYMBOL(ukmem_add_regs);

#define TX_FLAG	0
#define RX_FLAG 1
void ukmem_add_desc(unsigned long addr, int flag)
{

	desc[desc_index] = addr;

	if(flag == TX_FLAG)
		printk(KERN_INFO "add rx desc %lx %d\n",addr,desc_index);	
	else
		printk(KERN_INFO "add tx desc %lx %d\n",addr,desc_index);	

	desc_index ++;
}

EXPORT_SYMBOL(ukmem_add_desc);

static int kmem_init(int order)
{
	unsigned long  phys;
	unsigned long addr;
	char * str = "hello,world\n";
	int i;
	struct kmem * km;


	for(i = 0; i < CPU_NUM; i++)
	{
		km = &kmem[i];

		km->start = 0;
		km->phys = 0;
		km->order = 0;
		km->index = 0;
	}

	for(i = 0; i < CPU_NUM; i++)
	{
		km = &kmem[i];
			
		addr  = __get_free_pages(GFP_KERNEL , order);
		if(addr == 0)
		{
			printk(KERN_INFO "get_free_pages error\n");
			goto err;
		}

		phys = virt_to_phys((volatile void *)(addr));

		km->start = addr;
		km->phys = phys; 
		km->order = order;

		memcpy((void*)addr,(void*)str,strlen(str)+1);

		printk(KERN_INFO "physical addr cpunum:%d  %lx %lu\n",i,addr,addr);
	}

	return 0;

err:

	for(i = 0; i < CPU_NUM; i++)
	{
		km = &kmem[i];

		if(km->start)
			free_pages(km->start, km->order);
	}

	
	return -EINVAL;	
}


static int kmem_exit(void)
{
	int i;
	struct kmem * km;

	for(i = 0; i < CPU_NUM; i++)
	{
		km = &kmem[i];

		if(km->start)
			free_pages(km->start, km->order);
	}

	return 0;
}
 


struct sk_buff * ukmem_skb_alloc(unsigned int size)
{
	int cpuid = smp_processor_id();
	struct kmem * km = &kmem[cpuid];

	struct sk_buff * skb;
	unsigned long addr;	
	unsigned char * data;


	printk(KERN_INFO "!!!****size :%d index = %d\n", size ,km->index);


	if(km->index < (1<<km->order)) 
	{
		addr = (km->start + (km->index * PAGE_SIZE));
		km->index++;
	}	
	else return NULL;

	size = PAGE_SIZE/2;
        data = (unsigned char *)addr;
	
	skb = (struct sk_buff *) (addr + (PAGE_SIZE/2));

        memset(skb, 0, offsetof(struct sk_buff, tail));
        skb->truesize = size + sizeof(struct sk_buff);
                
	skb->head = data;
	skb->data = data;
//	skb->tail = data;
	skb_reset_tail_pointer(skb);
//	skb->end  = data + size;
	skb->end = skb->tail + size;

	printk(KERN_INFO "!!!****skb_alloc:%lx cpuid = %d index = %d\n", (unsigned long)skb , cpuid, km->index - 1);

	return skb;
}


EXPORT_SYMBOL(ukmem_skb_alloc);


void ukmem_skb_free(struct sk_buff * skb)
{

//	int cpuid = smp_processor_id();
//	struct kmem * km = &kmem[cpuid];

	return ;
}

EXPORT_SYMBOL(ukmem_skb_free);



int ukmem_index(struct sk_buff * skb)
{
	int cpuid = smp_processor_id();
	struct kmem * km = &kmem[cpuid];

	unsigned index = ((unsigned long)skb -  km->start)/PAGE_SIZE; 

	return index;
}

EXPORT_SYMBOL(ukmem_index);


static int ukmem_vma_fault(struct vm_area_struct *vma,struct vm_fault *vmf) 
{
	unsigned long offset;  
	struct page *page;  
	int cpuid = smp_processor_id();
	struct kmem * km = &kmem[cpuid];

	offset = (unsigned long)vmf->virtual_address - vma->vm_start;

	if((offset/PAGE_SIZE) > (1 << km->order))
		return VM_FAULT_SIGBUS;  

	page = virt_to_page( km->start + offset);

	if (!page)  
	{
		printk(KERN_INFO "NO PAGE FOUND\n");	
		return VM_FAULT_SIGBUS;  
	}
	vmf->page = page;  
	get_page(page);

	return 0;  

}

static struct vm_operations_struct ukmem_vm_ops = {
	.fault = ukmem_vma_fault,
};



static int ukmem_mmap_regs(struct file *file, struct vm_area_struct *vma)
{
	int i;
	unsigned long phys_addr;	
	unsigned long len;

	len = regs[0].len;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); 

	for(i = 0; i < regs_index; i++)
	{
		printk(KERN_INFO "HI~~ mapping regs %d\n",i);
		phys_addr = regs[i].phys;
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
	int i;
	unsigned long phys_addr;	

	for(i = 0; i < desc_index; i++)
	{
		printk(KERN_INFO "HI~~ mapping desc%d\n",i);
		phys_addr = (desc[i]);
		if(remap_pfn_range(vma,vma->vm_start  +i * PAGE_SIZE,phys_addr >> PAGE_SHIFT,PAGE_SIZE,vma->vm_page_prot))
		{
			printk(KERN_INFO "mmap error\n");
			return -EAGAIN;
		}
	}	
	
	return 0; 
}



static int ukmem_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long size = vma->vm_end - vma->vm_start;
	int ret = 0;

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
		vma->vm_flags|=VM_RESERVED;
		vma->vm_ops  = &ukmem_vm_ops;
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



static ssize_t ukmem_read(struct file *file, char __user *buf, size_t size, loff_t *off)
{
	printk(KERN_INFO "read\n");

	return 0;
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

	printk(KERN_INFO "skb size = %ld offset data = %ld  len = %ld\n", sizeof(struct sk_buff),offsetof(struct sk_buff,data), offsetof(struct sk_buff ,len));	


	if(kmem_init(10) == -EINVAL) return -EINVAL;

	if (misc_register(&ukmem_dev) < 0) {

		kmem_exit();
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

	kmem_exit();
	misc_deregister(&ukmem_dev);
}

module_exit(ukmem_exit_module);
