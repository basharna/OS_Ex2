CXX = g++
CXXFLAGS = -std=c++11 -Wall -O3

all: mync ttt

mync: mync.o
	$(CXX) $(CXXFLAGS) -o mync mync.o

ttt: ttt.o
	$(CXX) $(CXXFLAGS) -o ttt ttt.o

ttt.o: ttt.cpp
	$(CXX) $(CXXFLAGS) -c ttt.cpp

mync.o: mync.cpp
	$(CXX) $(CXXFLAGS) -c mync.cpp

clean:
	rm -f *.o mync ttt