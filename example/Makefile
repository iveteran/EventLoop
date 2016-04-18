TARGET_1 = echoserver
TARGET_2 = echoclient

CPPFLAGS = -Wall -std=c++0x
CXXFLAGS = -I../include

LDFLAGS  = -L../src -lel 

CXX      = g++
RM       = rm -f

TARGET_1_OBJS = echoserver.o
TARGET_2_OBJS = echoclient.o

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

.PHONY : all clean cleanall rebuild

all : $(TARGET_1) $(TARGET_2)

$(TARGET_1) : $(TARGET_1_OBJS)
	$(CXX) -o $(TARGET_1) $(TARGET_1_OBJS) $(LDFLAGS)

$(TARGET_2) : $(TARGET_2_OBJS)
	$(CXX) -o $(TARGET_2) $(TARGET_2_OBJS) $(LDFLAGS)

rebuild: clean all

clean:
	@$(RM) *.o *.d

cleanall: clean
	@$(RM) $(TARGET_1) $(TARGET_2)
