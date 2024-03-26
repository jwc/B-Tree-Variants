CXX = clang++
CXXFLAGS = -g -std=c++20 -D PAGE_SIZE=$(PAGE_SIZE) -D VALUE_SIZE=$(VALUE_SIZE)

PAGE_SIZE = 512
VALUE_SIZE = 12

UBPDEPS = ubpOpen.o ubpWrite.o ubpRead.o ubpErase.o ubpMisc.o ubpRedist.o ubpCard.o
ASBDEPS = 
ABDEPS = 

# X -> testX.cpp
TESTS = 0 1 2 3

UBPTESTS := $(foreach test,$(TESTS),ubp$(test).out) 
ASBTESTS := $(foreach test,$(TESTS),asb$(test).out) 
ABTESTS := $(foreach test,$(TESTS),ab$(test).out) 

all: ubp asb ab

ubp: $(UBPDEPS) $(UBPTESTS)
asb: $(ASBDEPS) $(ASBTESTS)
ab: $(ABDEPS) $(ABTESTS)

ubp%.out: test%.cpp
	$(CXX) $(CXXFLAGS) $(UBPDEPS) -D UBPTREE -o $@ $?

asb%.out: test%.cpp
	$(CXX) $(CXXFLAGS) $(ASBDEPS) -D ASBTREE -o $@ $?

ab%.out: test%.cpp
	$(CXX) $(CXXFLAGS) $(ABDEPS) -D ABTREE -o $@ $?

clean:
	$(RM) *.o *.out *.txt
