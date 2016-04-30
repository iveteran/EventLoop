TARGET_1 = echoserver
TARGET_2 = echoclient
TARGET_3 = hiredis_example

REDIS_SDK_PATH = $(HOME)/sdks/hiredis-master

CPPFLAGS = -Wall -std=c++0x
CXXFLAGS = -I../include \
           -I../plugin/redis \
           -I$(REDIS_SDK_PATH)

LDFLAGS  = -L../src -lel \
           $(REDIS_SDK_PATH)/libhiredis.a

CXX      = g++
RM       = rm -f

TARGET_1_OBJS = echoserver.o
TARGET_2_OBJS = echoclient.o
TARGET_3_OBJS = hiredis_example.o

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

.PHONY : all clean cleanall rebuild

ifdef ENABLE_REDIS_API
all : $(TARGET_1) $(TARGET_2) $(TARGET_3)
else
all : $(TARGET_1) $(TARGET_2)
endif

$(TARGET_1) : $(TARGET_1_OBJS)
	$(CXX) -o $(TARGET_1) $(TARGET_1_OBJS) $(LDFLAGS)

$(TARGET_2) : $(TARGET_2_OBJS)
	$(CXX) -o $(TARGET_2) $(TARGET_2_OBJS) $(LDFLAGS)

$(TARGET_3) : $(TARGET_3_OBJS)
	$(CXX) -o $(TARGET_3) $(TARGET_3_OBJS) $(LDFLAGS)

rebuild: clean all

clean:
	@$(RM) *.o *.d

cleanall: clean
	@$(RM) $(TARGET_1) $(TARGET_2) $(TARGET_3)
