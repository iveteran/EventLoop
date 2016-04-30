
ENABLE_REDIS_API = 1
ENABLE_POSTGRESQL_API =

subsystem:
ifdef ENABLE_REDIS_API
	$(MAKE) -C plugin/redis
	$(MAKE) ENABLE_REDIS_API=1 -C src
	$(MAKE) ENABLE_REDIS_API=1 -C example
else
	$(MAKE) -C src
	$(MAKE) -C example
endif

clean:
ifdef ENABLE_REDIS_API
	$(MAKE) -C plugin/redis clean
endif
	$(MAKE) -C src clean
	$(MAKE) -C example clean

cleanall:
ifdef ENABLE_REDIS_API
	$(MAKE) -C plugin/redis cleanall
endif
	$(MAKE) -C src cleanall
	$(MAKE) -C example cleanall

