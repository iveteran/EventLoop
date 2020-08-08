TARGET = amqp_client_example

AMQP_LIB_PATH = ../../../../../AMQP-CPP

CPPFLAGS = -g -Wall -std=c++0x
CXXFLAGS = -I../../../include \
					 -I $(AMQP_LIB_PATH)/include \
					 -I..

CXXLDFLAGS = -L../ -lamqp_api \
			 -L../../../src -lel \
			 $(AMQP_LIB_PATH)/build/bin/libamqpcpp.a \
			 -lssl -ldl

CXX      = g++
RM       = rm -f
AR       = ar -r

SRCDIRS  = .
SRCEXTS  = .cpp
SOURCES  = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
OBJS     = $(foreach x,$(SRCEXTS), $(patsubst %$(x),%.o,$(filter %$(x),$(SOURCES))))
DEPS     = $(patsubst %.o,%.d,$(OBJS))

.PHONY : all clean cleanall rebuild

$(TARGET) : $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(CXXLDFLAGS)

%.d : %.cpp
	@$(CXX) -MM -MD $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

-include $(DEPS)

all: $(TARGET)

rebuild: clean all

clean:
	@$(RM) *.o *.d

cleanall: clean
	@$(RM) $(TARGET)
