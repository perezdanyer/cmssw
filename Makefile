ROOTINC    = $(shell /home/connorpa/Libraries/Root/bin/root-config --incdir)
ROOTCFLAGS = $(shell /home/connorpa/Libraries/Root/bin/root-config --cflags)
ROOTLIBS   = $(shell /home/connorpa/Libraries/Root/bin/root-config --libs)

BOOSTINC = -I/usr/include/boost
BOOSTLIBS = -L/usr/lib/x86_64-linux-gnu -lboost_system -lboost_program_options -lboost_filesystem

CFLAGS=-g -Wall -O3 -std=c++1z -fPIC -DDEBUG
# -stdc++fs 

all: validateAlignment getINFO single merge

validateAlignment: validateAlignment.o Options.o DAG.o
	g++ ${CFLAGS} $^ $(BOOSTLIBS) -o $@

single: single.o Options.o toolbox.h
	g++ ${CFLAGS} $^ $(BOOSTLIBS) -o $@

merge: merge.o Options.o toolbox.h
	g++ ${CFLAGS} $^ $(BOOSTLIBS) -o $@

getINFO: getINFO.o Options.o
	g++ ${CFLAGS} $^ $(BOOSTLIBS) -o $@

lib%.so: %.cc
	g++ ${CFLAGS} $(BOOSTINC) -shared $^ $(BOOSTLIBS) -o $@

%.o: %.cc 
	g++ $(CFLAGS) $(BOOSTINC) -c $^ -o $@  

clean: 
	rm -f *.pdf *.so *.o *.root 

cleantest:
	@rm -rf MyLFS/* TheValidation
	@./validateAlignment -v config.info

.PHONY: all clean
