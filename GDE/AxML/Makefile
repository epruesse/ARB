# Makefile February 2002 by Alexandros P. Stamatakis

.SUFFIXES: .o .c .h

OBJECT = axml.o
RM 	   = rm -f

# ----------------------------------------
# normal compilation:
ifndef ARB

CC 		= gcc
CFLAGS 	= -O3
TARGET  = axml
MAIN 	= $(EXE)
#-xO5
# -pg
LDFLAGS = -lm

# ----------------------------------------
# compile for ARB:
else

CC 		= $(ACC)
TARGET 	= $(ARBHOME)/bin/axml
LDFLAGS = $(LIB) -lm
CFLAGS  = $(cflags)

$(MAIN) : all

endif

all : $(TARGET)

$(TARGET) : $(OBJECT)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean :
	$(RM) $(OBJECT) $(TARGET)

depend:
	makedepend $(MAKEDEPENDINC) $(OBJECT:.o=.c)

# DO NOT DELETE

axml.o:  axml.h
