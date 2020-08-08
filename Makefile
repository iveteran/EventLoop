
subsystem:
	$(MAKE) -C src
	$(MAKE) -C example
	$(MAKE) -C extensions

clean:
	$(MAKE) -C src clean
	$(MAKE) -C example clean
	$(MAKE) -C extensions clean

cleanall:
	$(MAKE) -C src cleanall
	$(MAKE) -C example cleanall
	$(MAKE) -C extensions cleanall

