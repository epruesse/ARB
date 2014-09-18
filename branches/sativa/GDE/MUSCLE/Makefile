#Simple makefile to build muscle inside ARB
#By Arne Boeckmann (aboeckma@mpi-bremen.de)

.PHONY: all clean

CXX=$(A_CXX)
CXXFLAGS=-c -O3 -msse2 -mfpmath=sse -D_FILE_OFFSET_BITS=64 -DNDEBUG=1
LDFLAGS= -g

OBJDIR=obj
SRCDIR=src

SOURCES=$(wildcard $(SRCDIR)/*.cpp)
HEADERS=$(wildcard $(SRCDIR)/*.h)
OBJECTS=$(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.cpp=.o)))

MUSCLE=$(ARBHOME)/bin/muscle

SIMPLE_DEPENDS=Makefile $(HEADERS)

all: $(OBJDIR)
	$(MAKE) $(MUSCLE)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(SIMPLE_DEPENDS)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(MUSCLE): $(OBJECTS) $(SIMPLE_DEPENDS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $(MUSCLE)

clean:
	rm -rf $(OBJDIR)
	rm -f $(MUSCLE)



