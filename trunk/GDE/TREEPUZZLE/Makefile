
.SUFFIXES: .o .c .depend

OBJECT =  \
 src/gamma.o \
 src/ml1.o \
 src/ml2.o \
 src/ml3.o \
 src/model1.o \
 src/model2.o \
 src/puzzle1.o \
 src/puzzle2.o \
 src/sched.o \
 src/util.o \

# src/ppuzzle.o \


PUZZ_CFLAGS = $(cflags) -DPACKAGE=\"tree-puzzle\" -DVERSION=\"5.0\" -DHAVE_LIBM=1 -DSTDC_HEADERS=1 -DHAVE_LIMITS_H=1 -Isrc

$(ARBHOME)/bin/puzzle: $(OBJECT)
		$(ACC) $(PUZZ_CFLAGS) -o $@ $(OBJECT) $(LIB) -lm

src/%.o : src/%.c
	$(ACC) $(PUZZ_CFLAGS) -c -o $@ $< $(AINCLUDES)

clean:
		-rm src/*.o

DEPENDS = $(OBJECT:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@


# DO NOT DELETE

src/gamma.o: src/util.h src/gamma.h

src/ml1.o: src/util.h src/ml.h

src/ml2.o: src/util.h src/ml.h

src/ml3.o: src/util.h src/ml.h src/gamma.h

src/model1.o: src/util.h src/ml.h

src/model2.o: src/util.h src/ml.h

src/puzzle1.o: src/puzzle.h src/util.h src/ml.h src/gamma.h

src/puzzle2.o: src/puzzle.h src/util.h src/ml.h

src/sched.o: src/sched.h src/util.h

src/util.o: src/util.h
