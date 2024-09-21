TARGET = libel_amqp.a

ROOT = ../..

# git clone https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git
# cd AMQP-CPP && make build && cd build && cmake -DAMQP-CPP_LINUX_TCP=ON ..
AMQP_LIB_PATH = $(ROOT)/../third_party/AMQP-CPP

CPPFLAGS = -Wall -std=c++20
CXXFLAGS = -I$(ROOT)/include \
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
