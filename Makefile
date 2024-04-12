CXX = clang++
CXXFLAGS = -g -std=c++20 -D PAGE_SIZE=$(PAGE_SIZE) -D VALUE_SIZE=$(VALUE_SIZE) -D COUNT=10000

PAGE_SIZE = 512
VALUE_SIZE = 12

UBPFILES = ubpOpen.o ubpWrite.o ubpRead.o ubpErase.o ubpMisc.o ubpRedist.o ubpCard.o
ASBFILES = asberase.o asbinsert.o asbmisc.o asbsearch.o
ABFILES = ABTree.o

# X -> testX.cpp	
TESTS = 1 2

UBPTESTS := $(foreach test,$(TESTS),ubp$(test).out) 
ASBTESTS := $(foreach test,$(TESTS),asb$(test).out) 
ABTESTS := $(foreach test,$(TESTS),ab$(test).out) 

all: ubp asb ab

ubp: $(UBPFILES) $(UBPTESTS)
asb: $(ASBFILES) $(ASBTESTS)
ab: $(ABFILES) $(ABTESTS)

ubp%.out: test%.cpp
	$(CXX) $(CXXFLAGS) $(UBPFILES) -D UBPTREE -o $@ $?

asb%.out: test%.cpp
	$(CXX) $(CXXFLAGS) $(ASBFILES) -D ASBTREE -o $@ $?

ab%.out: test%.cpp
	$(CXX) $(CXXFLAGS) $(ABFILES) -D ABTREE -o $@ $?

clean:
	$(RM) *.o *.out *.txt *.db
	$(RM) mydb.*.dat
	$(RM) -r *.dSYM
