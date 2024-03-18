exec_name = a

CXXFLAGS = -g -c

all: uBPlusTree build

uBPlusTree: 
	$(CXX) $(CXXFLAGS) ubpmisc.cpp
	
	$(CXX) $(CXXFLAGS) ubpwrite.cpp

build: *.o
	$(CXX) $(CXXFLAGS) test.cpp

	$(CXX) $?

clean: *.o
	$(RM) a.exe
	
	rm $?
