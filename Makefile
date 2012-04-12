CXX=g++
CXXFLAGS=-DIB_USE_STD_STRING -Wall -Wno-switch -g -DPY_LONG_LONG="long long"
ROOT_DIR=..

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
BASE_SRC_DIR=${ROOT_DIR}/PosixSocketClient
INCLUDES=-I${ROOT_DIR}/Shared/ -I${BASE_SRC_DIR} -I/usr/include/python2.7
LIBRARY=-L/usr/lib/python2.7/config
endif

ifeq ($(UNAME), CYGWIN_NT-6.1-WOW64)
BASE_SRC_DIR=${ROOT_DIR}/PosixSocketClient/src
INCLUDES=-I${ROOT_DIR}/Shared/ -I${BASE_SRC_DIR} -IC:/Python27/include
LIBRARY=-LC:/Python27/lib
endif


TARGET=eu

$(TARGET):
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o EClientSocketBase.o -c $(BASE_SRC_DIR)/EClientSocketBase.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o EPosixClientSocket.o -c $(BASE_SRC_DIR)/EPosixClientSocket.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o PosixTestClient.o -c PosixTestClient.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o Main.o -c Main.cpp
	#$(CXX) $(CXXFLAGS) -o $@ EClientSocketBase.o EPosixClientSocket.o PosixTestClient.o Main.o $(LIBRARY) -lpython2.7
	$(CXX) $(CXXFLAGS) -o $@ EClientSocketBase.o EPosixClientSocket.o PosixTestClient.o Main.o $(LIBRARY) -lpython2.7
clean:
	rm -f $(TARGET) *.o
