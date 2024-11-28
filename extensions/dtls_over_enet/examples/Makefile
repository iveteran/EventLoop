TARGET_1 = doe_echo_server
TARGET_2 = doe_echo_client

ROOT = ../../..

CPPFLAGS = -g -Wall -std=c++20
CXXFLAGS = -I$(ROOT)/include \
		   -I..

LDFLAGS =

DEP_LIBS += \
           ../libel_doe.a \
           ../../enet/libel_enet.a \
           ../../dtls_api/libel_dtls.a \
           $(ROOT)/core/libel.a \
           -lssl -lcrypto \
           -lenet

CXX      = g++
RM       = rm -f

TARGET_1_OBJS = doe_echo_server.o
TARGET_2_OBJS = doe_echo_client.o

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

.PHONY : all clean cleanall rebuild

all: $(TARGET_1) $(TARGET_2)

$(TARGET_1) : $(TARGET_1_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_1) $(TARGET_1_OBJS) $(DEP_LIBS) $(LDFLAGS)

$(TARGET_2) : $(TARGET_2_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_2) $(TARGET_2_OBJS) $(DEP_LIBS) $(LDFLAGS)

rebuild: clean all

clean:
	@$(RM) *.o *.d

cleanall: clean
	@$(RM) $(TARGET_1) $(TARGET_2)
