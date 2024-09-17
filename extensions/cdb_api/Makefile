TARGET = libcdb_api.a

ROOT = ../..

# git clone https://github.com/iveteran/hiredis-ext.git
# cd hiredis-ext && make
REDIS_SDK_PATH = $(ROOT)/../third_party/hiredis-ext

CPPFLAGS = -Wall -std=c++20
CXXFLAGS = -I$(ROOT)/include \
           -I$(REDIS_SDK_PATH)

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
	@$(CXX) -MM -MD $(CXXFLAGS) $<

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
