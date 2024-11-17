TARGET_1 = echoserver
TARGET_2 = echoclient
TARGET_3 = custom_message_server
TARGET_4 = custom_message_client
TARGET_5 = logger_demo

CPPFLAGS = -g -Wall -std=c++20
#CPPFLAGS = -g -Wall -std=c++20 -D USE_SELECT
#CPPFLAGS = -g -Wall -std=c++20 -D USE_IO_URING
CXXFLAGS = -I../include

LDFLAGS = 

DEP_LIBS += \
           ../core/libel.a \
#           -luring

CXX      = g++
RM       = rm -f

TARGET_1_OBJS = echoserver.o
TARGET_2_OBJS = echoclient.o
TARGET_3_OBJS = custom_message_server.o
TARGET_4_OBJS = custom_message_client.o
TARGET_5_OBJS = logger_demo.o

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

.PHONY : all clean cleanall rebuild

all: $(TARGET_1) $(TARGET_2) $(TARGET_3) $(TARGET_4) $(TARGET_5)

$(TARGET_1) : $(TARGET_1_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_1) $(TARGET_1_OBJS) $(DEP_LIBS) $(LDFLAGS)

$(TARGET_2) : $(TARGET_2_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_2) $(TARGET_2_OBJS) $(DEP_LIBS) $(LDFLAGS)

$(TARGET_3) : $(TARGET_3_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_3) $(TARGET_3_OBJS) $(DEP_LIBS) $(LDFLAGS)

$(TARGET_4) : $(TARGET_4_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_4) $(TARGET_4_OBJS) $(DEP_LIBS) $(LDFLAGS)

$(TARGET_5) : $(TARGET_5_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_5) $(TARGET_5_OBJS) $(DEP_LIBS) $(LDFLAGS)

rebuild: clean all

clean:
	@$(RM) *.o *.d

cleanall: clean
	@$(RM) $(TARGET_1) $(TARGET_2) $(TARGET_3) $(TARGET_4) $(TARGET_5)
