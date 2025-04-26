CXX = g++
CXXFLAGS = -Wall -std=c++11
LDFLAGS = -lws2_32

all: myhttp

myhttp: httpd.cpp
	$(CXX) $(CXXFLAGS) -o myhttp httpd.cpp $(LDFLAGS)

clean:
	del myhttp.exe
