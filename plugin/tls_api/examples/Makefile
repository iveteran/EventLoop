TARGET_1 = tls_echoserver
TARGET_2 = tls_echoclient

CPPFLAGS = -g -Wall -std=c++0x
CXXFLAGS = -I../../../include \
		   -I../

LDFLAGS =

DEP_LIBS += \
           ../../../src/libel.a \
           ../libtls_api.a \
           -lssl -lcrypto

CXX      = g++
RM       = rm -f

TARGET_1_OBJS = tls_echoserver.o
TARGET_2_OBJS = tls_echoclient.o

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

.PHONY : all clean cleanall rebuild

all: $(TARGET_1) $(TARGET_2)

$(TARGET_1) : $(TARGET_1_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_1) $(TARGET_1_OBJS) $(DEP_LIBS) $(LDFLAGS)

$(TARGET_2) : $(TARGET_2_OBJS) $(DEP_LIBS)
	$(CXX) -o $(TARGET_2) $(TARGET_2_OBJS) $(DEP_LIBS) $(LDFLAGS)

rebuild: clean all

clean:
	@$(RM) *.o *.d

cleanall: clean
	@$(RM) $(TARGET_1) $(TARGET_2)
