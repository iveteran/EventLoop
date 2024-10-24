TARGET = libel_kcp.a

ROOT = ../..

CCFLAGS = -g -Wall
CPPFLAGS = -g -Wall -std=c++20
CXXFLAGS = -I$(ROOT)/include

CC 		 = gcc
CXX      = g++
RM       = rm -f
AR       = ar -r

SRCDIRS  = .
SRCEXTS  = .cpp .c
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

%.o : %.c
	$(CC) -c $(CCFLAGS) $(CXXFLAGS) $<

-include $(DEPS)

all: $(TARGET)

rebuild: clean all

clean:
	@$(RM) $(OBJS) *.d
	make -C example clean

cleanall: clean
	@$(RM) *.a
	make -C example cleanall

example:
	make -C example
