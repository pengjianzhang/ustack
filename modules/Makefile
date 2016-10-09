
obj-m := ukmem.o
obj-m += igb/

test-objs := ukmem.o
KDIR := /lib/modules/2.6.35/build

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
