TARGET = async_task_example

ROOT = ../../..

CPPFLAGS = -g -Wall -std=c++20
CXXFLAGS = -I$(ROOT)/include \
		   -I..

LDFLAGS =

# XXX: libel_async_task.a 必须放在其依赖库 libel.a 之前
DEP_LIBS += \
			../libel_async_task.a \
           $(ROOT)/core/libel.a \

CXX      = g++
RM       = rm -f

TARGET_OBJS = async_task_example.o find_prime_numbers.o

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

.PHONY : all clean cleanall rebuild

all: $(TARGET)

$(TARGET) : $(TARGET_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET) $(TARGET_OBJS) $(DEP_LIBS) $(LDFLAGS)

rebuild: clean all

clean:
	@$(RM) $(TARGET_OBJS) *.d

cleanall: clean
	@$(RM) $(TARGET)
