TARGET = libconsole.a

ROOT = ../..

CPPFLAGS = -g -Wall -std=c++20 -D SUPPORTS_READLINE
CXXFLAGS = -I$(ROOT)/include

CXX      = g++
RM       = rm -f
AR       = ar -r

SRCDIRS  = .
SRCEXTS  = .cpp
SOURCES  = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
OBJS     = $(foreach x,$(SRCEXTS), $(patsubst %$(x),%.o,$(filter %$(x),$(SOURCES))))
DEPS     = $(patsubst %.o,%.d,$(OBJS))

.PHONY : all clean cleanall rebuild example

$(TARGET) : $(OBJS)
	$(RM) $(TARGET)
	$(AR) $(TARGET) $(OBJS)

%.d : %.cpp
	@$(CXX) -MM -MD $(CXXFLAGS) $<

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

-include $(DEPS)

all: $(TARGET)

rebuild: clean all

clean:
	@$(RM) *.o *.d
	make -C example clean

cleanall: clean
	@$(RM) *.a
	make -C example cleanall

example:
	make -C example
