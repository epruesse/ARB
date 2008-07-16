# PHYML 2.4.5 Makefile
# modified for ARB by Ralf Westram (Jun 2006)

.SUFFIXES: .o .c .h .depend

LIBS  = -lm
PROG  = PHYML
DFLAG = -DUNIX -D$(PROG)
OBJS  = main.o utilities.o optimiz.o lk.o bionj.o models.o free.o options.o simu.o eigen.o

ifndef ARB
# ----------------------------------------
# original compilation

CC = gcc #cc
CFLAGS = -O4 -fomit-frame-pointer -Wall -static
# CFLAGS = -Wall
# CFLAGS = -g -Wall
# CFLAGS = -pg -Wall -fprofile-arcs -static

EXEC = phyml 

else
# ----------------------------------------
# compilation together with ARB

CC     = $(ACC)
CFLAGS = $(cflags) -fomit-frame-pointer -static  
EXEC   = $(ARBHOME)/bin/phyml

# ARBs standard target
$(MAIN) : $(EXEC)

endif

$(EXEC) : $(OBJS)
		$(CC) -o $(EXEC) $(OBJS) $(LIBS) $(CFLAGS)

clean :
		rm -f $(OBJS) $(EXEC)

.c.o:
		$(CC) $(CFLAGS) $(DFLAG) -c -o $@ $<

######################################################################################################

DEPENDS = $(OBJS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

bionj.o: bionj.h
bionj.o: optimiz.h
bionj.o: utilities.h

eigen.o: eigen.h
eigen.o: utilities.h

free.o: free.h
free.o: utilities.h

lk.o: free.h
lk.o: lk.h
lk.o: models.h
lk.o: optimiz.h
lk.o: utilities.h

main.o: bionj.h
main.o: free.h
main.o: lk.h
main.o: models.h
main.o: optimiz.h
main.o: options.h
main.o: simu.h
main.o: utilities.h

models.o: eigen.h
models.o: free.h
models.o: models.h
models.o: utilities.h

optimiz.o: free.h
optimiz.o: lk.h
optimiz.o: optimiz.h
optimiz.o: utilities.h

options.o: free.h
options.o: models.h
options.o: options.h
options.o: utilities.h

simu.o: free.h
simu.o: lk.h
simu.o: models.h
simu.o: optimiz.h
simu.o: simu.h
simu.o: utilities.h

utilities.o: bionj.h
utilities.o: free.h
utilities.o: lk.h
utilities.o: models.h
utilities.o: optimiz.h
utilities.o: simu.h
utilities.o: utilities.h
