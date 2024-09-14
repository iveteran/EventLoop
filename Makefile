
subsystem:
	$(MAKE) -C core
	$(MAKE) -C example
	$(MAKE) -C extensions

clean:
	$(MAKE) -C core clean
	$(MAKE) -C example clean
	$(MAKE) -C extensions clean

cleanall:
	$(MAKE) -C core cleanall
	$(MAKE) -C example cleanall
	$(MAKE) -C extensions cleanall

