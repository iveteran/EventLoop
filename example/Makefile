TARGET_1 = echoserver
TARGET_2 = echoclient
TARGET_3 = hiredis_example
TARGET_4 = pg_example

REDIS_SDK_PATH = $(HOME)/sdks/hiredis-master

CPPFLAGS = -g -Wall -std=c++0x
CXXFLAGS = -I../include \
           -I../plugin/cdb_api \
           -I$(REDIS_SDK_PATH) \
           -I../plugin/db_api
LDFLAGS = 

DEP_LIBS = \
           ../plugin/db_api/libdb_api.a \
           ../src/libel.a \
           $(REDIS_SDK_PATH)/libhiredis.a \
           -lpq

CXX      = g++
RM       = rm -f

TARGET_1_OBJS = echoserver.o
TARGET_2_OBJS = echoclient.o
TARGET_3_OBJS = hiredis_example.o
TARGET_4_OBJS = pg_example.o

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

.PHONY : all clean cleanall rebuild

ifdef ENABLE_CDB_API
all : $(TARGET_1) $(TARGET_2) $(TARGET_3) $(TARGET_4)
else
all : $(TARGET_1) $(TARGET_2)
endif

$(TARGET_1) : $(TARGET_1_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_1) $(TARGET_1_OBJS) $(DEP_LIBS) $(LDFLAGS)

$(TARGET_2) : $(TARGET_2_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_2) $(TARGET_2_OBJS) $(DEP_LIBS) $(LDFLAGS)

$(TARGET_3) : $(TARGET_3_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_3) $(TARGET_3_OBJS) $(DEP_LIBS) $(LDFLAGS)

$(TARGET_4) : $(TARGET_4_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_4) $(TARGET_4_OBJS) $(DEP_LIBS) $(LDFLAGS)

rebuild: clean all

clean:
	@$(RM) *.o *.d

cleanall: clean
	@$(RM) $(TARGET_1) $(TARGET_2) $(TARGET_3) $(TARGET_4)
