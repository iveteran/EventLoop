TARGET = lua_eval_example

ROOT = ../../..

CPPFLAGS = -g -Wall -std=c++0x
CXXFLAGS = -I$(ROOT)/include \
		   -I../

LDFLAGS =

DEP_LIBS += \
           ../liblua_eval.a \
           $(ROOT)/core/libel.a \
           -llua5.4

CXX      = g++
RM       = rm -f

TARGET_OBJS = lua_eval_example.o

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

.PHONY : all clean cleanall rebuild

all: $(TARGET)

$(TARGET) : $(TARGET_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET) $(TARGET_OBJS) $(DEP_LIBS) $(LDFLAGS)

rebuild: clean all

clean:
	@$(RM) *.o *.d

cleanall: clean
	@$(RM) $(TARGET)
