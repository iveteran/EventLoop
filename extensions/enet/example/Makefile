TARGET = enet_echo_server
TARGET_C = enet_echo_client

ROOT = ../../..

CPPFLAGS = -g -Wall -std=c++20 # -DUSE_SELECT
CXXFLAGS = -I$(ROOT)/include \
		   -I..

LDFLAGS =

# XXX: libel_enet.a 必须放在其依赖库 libel.a 之前
# NOTE: Depend on libenet, apt install libenet-dev
DEP_LIBS += \
			../libel_enet.a \
           $(ROOT)/core/libel.a \
           -lenet

CXX      = g++
RM       = rm -f

TARGET_OBJS = enet_echo_server.o
TARGET_C_OBJS = enet_echo_client.o

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

.PHONY : all clean cleanall rebuild

all: $(TARGET) $(TARGET_C)

$(TARGET) : $(TARGET_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET) $(TARGET_OBJS) $(DEP_LIBS) $(LDFLAGS)

$(TARGET_C) : $(TARGET_C_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_C) $(TARGET_C_OBJS) $(DEP_LIBS) $(LDFLAGS)

rebuild: clean all

clean:
	@$(RM) *.d $(TARGET_OBJS) $(TARGET_C_OBJS)

cleanall: clean
	@$(RM) $(TARGET) $(TARGET_C)
