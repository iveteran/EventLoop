
ENABLE_CDB_API=1
ENABLE_DB_API=0

subsystem:
ifdef ENABLE_CDB_API
	$(MAKE) -C plugin/cdb_api
endif
ifdef ENABLE_DB_API
	#$(MAKE) -C plugin/db_api
endif
	$(MAKE) ENABLE_CDB_API=1 ENABLE_DB_API=0 -C src
	$(MAKE) ENABLE_CDB_API=1 -C example

clean:
ifdef ENABLE_CDB_API
	$(MAKE) -C plugin/cdb_api clean
endif
ifdef ENABLE_DB_API
	$(MAKE) -C plugin/db_api clean
endif
	$(MAKE) -C src clean
	$(MAKE) -C example clean

cleanall:
ifdef ENABLE_CDB_API
	$(MAKE) -C plugin/cdb_api cleanall
endif
ifdef ENABLE_DB_API
	$(MAKE) -C plugin/db_api cleanall
endif
	$(MAKE) -C src cleanall
	$(MAKE) -C example cleanall

