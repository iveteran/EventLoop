
subsystem:
	$(MAKE) -C cdb_api
	$(MAKE) -C db_api
	$(MAKE) -C url_api
	$(MAKE) -C aio_api
	$(MAKE) -C tls_api
	$(MAKE) -C mqtt_api
	$(MAKE) -C amqp_api
	$(MAKE) -C lua_eval
	$(MAKE) -C console

examples:
	$(MAKE) -C cdb_api examples
	$(MAKE) -C db_api examples
	$(MAKE) -C url_api examples
	$(MAKE) -C aio_api examples
	$(MAKE) -C tls_api examples
	$(MAKE) -C mqtt_api examples
	$(MAKE) -C amqp_api examples
	$(MAKE) -C lua_eval examples
	$(MAKE) -C console example

clean:
	$(MAKE) -C cdb_api clean
	$(MAKE) -C db_api clean
	$(MAKE) -C url_api clean
	$(MAKE) -C aio_api clean
	$(MAKE) -C tls_api clean
	$(MAKE) -C mqtt_api clean
	$(MAKE) -C amqp_api clean
	$(MAKE) -C lua_eval clean
	$(MAKE) -C console clean

cleanall:
	$(MAKE) -C cdb_api cleanall
	$(MAKE) -C db_api cleanall
	$(MAKE) -C url_api cleanall
	$(MAKE) -C aio_api cleanall
	$(MAKE) -C tls_api cleanall
	$(MAKE) -C mqtt_api cleanall
	$(MAKE) -C amqp_api cleanall
	$(MAKE) -C lua_eval cleanall
	$(MAKE) -C console cleanall
