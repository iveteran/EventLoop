TARGET = url_api_example

ROOT = ../../..

CPPFLAGS = -g -Wall -std=c++20
CXXFLAGS = -I$(ROOT)/include \
		   -I$(ROOT) \
		   -I..

CXXLDFLAGS = -L../ -lurl_api \
			 -L$(ROOT)/core -lel \
			 -lcurl \

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
	@$(CXX) -MM -MD $(CXXFLAGS) $<

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

-include $(DEPS)

all: $(TARGET)

rebuild: clean all

clean:
	@$(RM) *.o *.d

cleanall: clean
	@$(RM) $(TARGET)
