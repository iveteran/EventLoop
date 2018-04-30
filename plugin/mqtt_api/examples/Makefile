TARGET = mqtt_client_example

MQTT_LIB_PATH = ../../../../../mosquitto

CPPFLAGS = -g -Wall -std=c++0x
CXXFLAGS = -I../../../include \
					 -I $(MQTT_LIB_PATH)/lib \
					 -I..

CXXLDFLAGS = -L../ -lmqtt_api \
			 -L../../../src -lel \
			 $(MQTT_LIB_PATH)/build/lib/libmosquitto.so

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
