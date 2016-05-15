
ENABLE_CDB_API = 1
ENABLE_POSTGRESQL_API =

subsystem:
ifdef ENABLE_CDB_API
	$(MAKE) -C plugin/cdb_api
	$(MAKE) ENABLE_CDB_API=1 -C src
	$(MAKE) ENABLE_CDB_API=1 -C example
else
	$(MAKE) -C src
	$(MAKE) -C example
endif

clean:
ifdef ENABLE_CDB_API
	$(MAKE) -C plugin/cdb_api clean
endif
	$(MAKE) -C src clean
	$(MAKE) -C example clean

cleanall:
ifdef ENABLE_CDB_API
	$(MAKE) -C plugin/cdb_api cleanall
endif
	$(MAKE) -C src cleanall
	$(MAKE) -C example cleanall

