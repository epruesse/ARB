################################################################################
# Makefile for probcons
# Modified for ARB by Arne Boeckmann (aboeckma@mpi-bremen.de)
################################################################################

################################################################################
# 1) Choose C++ compiler.
################################################################################

CXX = $(A_CXX)

################################################################################
# 2) Set C++ flags.
#    a) DEBUG mode -- no optimizations, enable SafeVector checking, no inlining
#    b) PROFILE mode -- for gprof
#    c) RELEASE mode
################################################################################

OTHERFLAGS = -DNumInsertStates=2 -DVERSION="1.12"

# debug mode    
#CXXFLAGS = -g -W -Wall -pedantic -DENABLE_CHECKS -fno-inline $(OTHERFLAGS)

# profile mode
#CXXFLAGS = -pg -W -Wall -pedantic $(OTHERFLAGS)

# release mode
#CXXFLAGS = -O3 -W -Wall -pedantic -DNDEBUG $(OTHERFLAGS) -mmmx -msse -msse2 -mfpmath=sse -march=pentium4 -mcpu=pentium4 -funroll-loops -fomit-frame-pointer 
#CXXFLAGS = -O3 -W -Wall -pedantic -DNDEBUG $(OTHERFLAGS) -funroll-loops
CXXFLAGS = -O3 -w -pedantic -DNDEBUG $(OTHERFLAGS) -funroll-loops

################################################################################
# 3) Dependencies
################################################################################

TARGETS = probcons compare project makegnuplot

.PHONY : all
all : $(TARGETS)

probcons : MultiSequence.h ProbabilisticModel.h ScoreType.h Sequence.h FileBuffer.h SparseMatrix.h EvolutionaryTree.h Defaults.h SafeVector.h Main.cc
	$(CXX) $(CXXFLAGS) -lm -o probcons Main.cc 

compare : MultiSequence.h Sequence.h FileBuffer.h SafeVector.h CompareToRef.cc
	$(CXX) $(CXXFLAGS) -o compare CompareToRef.cc

fixref : MultiSequence.h ProbabilisticModel.h ScoreType.h Sequence.h FileBuffer.h SparseMatrix.h EvolutionaryTree.h Defaults.h SafeVector.h FixRef.cc
	$(CXX) $(CXXFLAGS) -o fixref FixRef.cc

project : MultiSequence.h Sequence.h SafeVector.h ProjectPairwise.cc
	$(CXX) $(CXXFLAGS) -o project ProjectPairwise.cc

makegnuplot : MakeGnuPlot.cc
	$(CXX) $(CXXFLAGS) -o makegnuplot MakeGnuPlot.cc

.PHONY : clean
clean:
	rm -f $(TARGETS)
