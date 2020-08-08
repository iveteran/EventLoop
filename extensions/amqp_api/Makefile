TARGET = libamqp_api.a

AMQP_LIB_PATH = ../../../../AMQP-CPP

CPPFLAGS = -Wall -std=c++11
CXXFLAGS = -I../../include \
           -I$(AMQP_LIB_PATH)/include

CXX      = g++
RM       = rm -f
AR       = ar -r

SRCDIRS  = .
SRCEXTS  = .cpp
SOURCES  = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
OBJS     = $(foreach x,$(SRCEXTS), $(patsubst %$(x),%.o,$(filter %$(x),$(SOURCES))))
DEPS     = $(patsubst %.o,%.d,$(OBJS))

.PHONY : all clean cleanall rebuild examples

$(TARGET) : $(OBJS)
	$(AR) $(TARGET) $(OBJS)

%.d : %.cpp
	@$(CXX) -MM -MD $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

-include $(DEPS)

all: $(TARGET)

rebuild: clean all

clean:
	@$(RM) *.o *.d
	make -C examples clean

cleanall: clean
	@$(RM) *.a
	make -C examples cleanall

examples:
	make -C examples
