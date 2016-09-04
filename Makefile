
subsystem:
	$(MAKE) -C src
	$(MAKE) -C example

clean:
	$(MAKE) -C src clean
	$(MAKE) -C example clean

cleanall:
	$(MAKE) -C src cleanall
	$(MAKE) -C example cleanall

