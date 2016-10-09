CFLAGS = -g -Iinclude 
LDFLAGS =  -g
CC = gcc
LD = gcc

all: ukmem_uspace 

OBJS =	src/arp.o src/connection.o src/driver.o src/main.o src/packet.o \
	src/process.o   src/checksum.o  src/debug.o src/hashtable.o src/local_ip.o \
	src/mem_pool.o  src/port_range.o  src/server_pool.o src/vserver.o \
	src/route.o src/timer.o src/tcp.o src/lf_queue.o src/net_global.o \
	src/interface.o src/dispatch.o src/test.o  

ukmem_uspace: $(OBJS)  
	$(LD) $(LDFLAGS) -o $@ $^ 

%.o:    %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f src/*.[oas] ukmem_uspace include/*.gch include/common/*.gch include/types/*.gch include/linux-net/*.gch 

