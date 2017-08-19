TARGET   = libel.a

CPPFLAGS = -Wall -std=c++0x# -DUSE_SELECT
#CPPFLAGS = -Wall -std=c++0x -D_BINARY_MSG_EXTEND_PACKAGING
CXXFLAGS = -I../include \

CXX      = g++
RM       = rm -f
AR       = ar -r

SRCDIRS  = .
ifdef ENABLE_CDB_API
	SRCDIRS += ../plugin/cdb_api
endif
SRCEXTS  = .cpp
SOURCES  = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
OBJS     = $(foreach x,$(SRCEXTS), $(patsubst %$(x),%.o,$(filter %$(x),$(SOURCES))))
DEPS     = $(patsubst %.o,%.d,$(OBJS))

.PHONY : all clean cleanall rebuild

$(TARGET) : $(OBJS)
	$(AR) $(TARGET) $(OBJS)

%.d : %.cpp
	@$(CXX) -MM -MD $(CXXFLAGS) $<

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

-include $(DEPS)

all : $(TARGET)

rebuild: clean all

clean:
	@$(RM) *.o *.d

cleanall: clean
	@$(RM) $(TARGET)
