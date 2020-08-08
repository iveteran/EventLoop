TARGET = lua_eval_example

CPPFLAGS = -g -Wall -std=c++0x
CXXFLAGS = -I../../../include \
		   -I../

LDFLAGS =

DEP_LIBS += \
           ../liblua_eval.a \
           ../../../src/libel.a \
           -llua

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
