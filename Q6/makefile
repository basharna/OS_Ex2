CXX = g++
CXXFLAGS = -std=c++11 -Wall -O3 -fprofile-arcs -ftest-coverage

all: mync ttt UDSD

mync: mync.o
	$(CXX) $(CXXFLAGS) -o mync mync.o

ttt: ttt.o
	$(CXX) $(CXXFLAGS) -o ttt ttt.o

UDSD: UDS_UDP.o
	$(CXX) $(CXXFLAGS) -o UDSD UDS_UDP.o

ttt.o: ttt.cpp
	$(CXX) $(CXXFLAGS) -c ttt.cpp

mync.o: mync.cpp
	$(CXX) $(CXXFLAGS) -c mync.cpp

UDS_UDP.o: UDS_UDP.cpp
	$(CXX) $(CXXFLAGS) -c UDS_UDP.cpp

clean:
	rm -f *.o mync ttt UDSD *.gcno *.gcda *.gcov