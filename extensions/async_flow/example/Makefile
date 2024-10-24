TARGET = async_flow_example

ROOT = ../../..

CPPFLAGS = -g -Wall -std=c++20 \
		   -D TEST_ASYNC_FLOW_WITH_SCHEDULER \
		   -D TEST_ASYNC_FLOW_WITH_MULTIPLE_FLOWS \
		   -D TEST_ASYNC_FLOW_WITHOUT_SCHEDULER \
		   #-D WITH_PERIODIC_TIMER
#CPPFLAGS = -g -Wall -std=c++20 \
#		   -D TEST_ASYNC_FLOW_WITHOUT_SCHEDULER

CXXFLAGS = -I$(ROOT)/include \
		   -I..

LDFLAGS =

DEP_LIBS += \
			../libel_async_flow.a \
           $(ROOT)/core/libel.a \

CXX      = g++
RM       = rm -f

TARGET_OBJS = async_flow_example.o

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

.PHONY : all clean cleanall rebuild

all: $(TARGET)

$(TARGET) : $(TARGET_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET) $(TARGET_OBJS) $(DEP_LIBS) $(LDFLAGS)

rebuild: clean all

clean:
	@$(RM) $(TARGET_OBJS) *.d

cleanall: clean
	@$(RM) $(TARGET)
