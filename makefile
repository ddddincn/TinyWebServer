CXX = g++
CFLAGS = -std=c++17 -O2 -Wall -g 

TARGET = server
OBJS = ./buffer/*.cpp ./epoller/*.cpp ./http/*.cpp \
	   ./log/*.cpp ./sqlconnpool/*.cpp \
	   ./timer/*.cpp ./webserver/*.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)