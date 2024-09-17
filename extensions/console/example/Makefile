TARGET = console_demo

ROOT = ../../..

CPPFLAGS = -g -Wall -std=c++20
CXXFLAGS = -I$(ROOT)/include \
		   -I$(ROOT) \
		   -I../

LDFLAGS =

DEP_LIBS += \
           $(ROOT)/core/libel.a \
           ../libconsole.a

DEP_LIBS += -lreadline

CXX      = g++
RM       = rm -f

TARGET_OBJS = console_demo.o

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
