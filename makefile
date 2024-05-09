CXX = g++
CFLAGS = -std=c++17 -O2 -Wall -g 

TARGET = server
OBJS = ./buffer/*.cpp ./epoller/*.cpp ./http/*.cpp \
	   ./log/*.cpp ./sqlconnpool/*.cpp ./threadpool/*.cpp \
	   ./timer/*.cpp ./webserver/*.cpp main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ./$(OBJS) $(TARGET)