#
# nezhad - Makefile
#
# Author: dccmx <dccmx@dccmx.com>
#

VERSION    = 0.1

OBJFILES   = eventloop.cpp tcp_connection.cpp tcp_server.cpp tcp_client.cpp utils.cpp message.cpp
INCFILES   = eventloop.h tcp_connection.h tcp_server.h tcp_client.h tcp_callbacks.h callback.h object.h singleton_tmpl.h utils.h message.h

CFLAGS_GEN = -Wall -g $(CFLAGS) -DVERSION=\"$(VERSION)\" -D_MSG_MINIMUM_PACKAGING
CFLAGS_DBG = -ggdb $(CFLAGS_GEN)
CFLAGS_OPT = -Wno-format -Wno-unused-result -std=c++0x $(CFLAGS_GEN)

LDFLAGS   += 
LIBS      += 

all: echoserver echoclient
	@echo
	@echo "Make Complete. See README for how to use."
	@echo
	@echo "Having problems with it? Send complains and bugs to iveteran.yuu@gmail.com"
	@echo

example: example.cpp $(OBJFILES) $(INCFILES)
	$(CXX) $(LDFLAGS) -o example $(CFLAGS_OPT) $(LIBS) $^

echoserver: echoserver.cpp $(OBJFILES) $(INCFILES)
	$(CXX) $(LDFLAGS) -o echoserver $(CFLAGS_OPT) $(LIBS) $^

echoclient: echoclient.cpp $(OBJFILES) $(INCFILES)
	$(CXX) $(LDFLAGS) -o echoclient $(CFLAGS_OPT) $(LIBS) $^

clean:
	rm -f example echoserver echoclient core core.[1-9][0-9]* memcheck.out callgrind.out.[1-9][0-9]* massif.out.[1-9][0-9]*
