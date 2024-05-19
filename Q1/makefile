CXX = g++
CXXFLAGS = -std=c++11 -Wall -O3

all: ttt

ttt: ttt.o
	$(CXX) $(CXXFLAGS) -o ttt ttt.o

ttt.o: ttt.cpp
	$(CXX) $(CXXFLAGS) -c ttt.cpp

clean:
	rm -f *.o ttt