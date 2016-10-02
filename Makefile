
subsystem:
	$(MAKE) -C src
	$(MAKE) -C example
	$(MAKE) -C plugin

clean:
	$(MAKE) -C src clean
	$(MAKE) -C example clean
	$(MAKE) -C plugin

cleanall:
	$(MAKE) -C src cleanall
	$(MAKE) -C example cleanall
	$(MAKE) -C plugin

