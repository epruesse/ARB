
.SUFFIXES: .o .c

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


PUZZ_CFLAGS = $(cflags) -DPACKAGE=\"tree-puzzle\" -DVERSION="5.0" -DHAVE_LIBM=1 -DSTDC_HEADERS=1 -DHAVE_LIMITS_H=1 -Isrc

$(ARBHOME)/bin/puzzle: $(OBJECT)
		$(ACC) $(PUZZ_CFLAGS) -o $@ $(OBJECT) $(LIB) -lm

src/%.o : src/%.c
	$(ACC) $(PUZZ_CFLAGS) -c -o $@ $< $(AINCLUDES)

clean:
		-rm src/*.o

depend:
	makedepend $(MAKEDEPENDINC) $(OBJECT:.o=.c)

