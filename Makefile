all:
	cd modules;make 
	$(MAKE) -C uspace 
	cp modules/ukmem.ko  .	
	cp modules/igb/igb.ko .	
	cp uspace/ukmem_uspace .
clean:
	$(MAKE) clean -C modules 
	$(MAKE) clean -C uspace 
	rm -f igb.ko ukmem.ko ukmem_uspace


