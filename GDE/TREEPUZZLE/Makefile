
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


PUZZ_CFLAGS = $(cflags) -DPACKAGE=\"tree-puzzle\" -DVERSION=\"5.0\" -DHAVE_LIBM=1 -DSTDC_HEADERS=1 -DHAVE_LIMITS_H=1 -Isrc

$(ARBHOME)/bin/puzzle: $(OBJECT)
		$(ACC) $(PUZZ_CFLAGS) -o $@ $(OBJECT) $(LIB) -lm

src/%.o : src/%.c
	$(ACC) $(PUZZ_CFLAGS) -c -o $@ $< $(AINCLUDES)

clean:
		-rm src/*.o

depend:
	makedepend $(MAKEDEPENDINC) $(OBJECT:.o=.c)

# DO NOT DELETE

src/gamma.o:  
src/gamma.o:  
src/gamma.o:  
src/gamma.o:  src/util.h 
src/gamma.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
src/gamma.o:  
src/gamma.o:  
src/gamma.o:  
src/gamma.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
src/gamma.o:  
src/gamma.o:   src/gamma.h
src/ml1.o:  
src/ml1.o:  
src/ml1.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
src/ml1.o:  
src/ml1.o:  
src/ml1.o:  
src/ml1.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
src/ml1.o:  
src/ml1.o:  
src/ml1.o:  
src/ml1.o:  
src/ml1.o:  src/util.h 
src/ml1.o:  src/ml.h
src/ml2.o:  
src/ml2.o:  
src/ml2.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
src/ml2.o:  
src/ml2.o:  
src/ml2.o:  
src/ml2.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
src/ml2.o:  
src/ml2.o:  
src/ml2.o:  
src/ml2.o:  
src/ml2.o:   src/util.h
src/ml2.o:   src/ml.h
src/ml3.o:  
src/ml3.o:  
src/ml3.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
src/ml3.o:  
src/ml3.o:  
src/ml3.o:  
src/ml3.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
src/ml3.o:  
src/ml3.o:  
src/ml3.o:  
src/ml3.o: src/util.h   src/ml.h
src/ml3.o: src/gamma.h
src/model1.o:  
src/model1.o:  
src/model1.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
src/model1.o:  
src/model1.o:  
src/model1.o:  
src/model1.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
src/model1.o:  src/util.h 
src/model1.o:  
src/model1.o:  
src/model1.o:   src/ml.h
src/model2.o:  
src/model2.o:  
src/model2.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
src/model2.o:  
src/model2.o:  
src/model2.o:  
src/model2.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
src/model2.o:  src/util.h 
src/model2.o:  
src/model2.o:  
src/model2.o:   src/ml.h
src/puzzle1.o: src/puzzle.h  
src/puzzle1.o:  
src/puzzle1.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
src/puzzle1.o:  
src/puzzle1.o:  
src/puzzle1.o:  
src/puzzle1.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
src/puzzle1.o:  
src/puzzle1.o:  
src/puzzle1.o:  
src/puzzle1.o:  
src/puzzle1.o:  
src/puzzle1.o:  
src/puzzle1.o: 
src/puzzle1.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
src/puzzle1.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
src/puzzle1.o: src/util.h src/ml.h src/gamma.h
src/puzzle2.o: src/puzzle.h  
src/puzzle2.o:  
src/puzzle2.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
src/puzzle2.o:  
src/puzzle2.o:  
src/puzzle2.o:  
src/puzzle2.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
src/puzzle2.o:  
src/puzzle2.o:  
src/puzzle2.o:  
src/puzzle2.o:  
src/puzzle2.o:  
src/puzzle2.o:  
src/puzzle2.o: 
src/puzzle2.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/limits.h
src/puzzle2.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/float.h
src/puzzle2.o: src/util.h src/ml.h
src/sched.o:  
src/sched.o:  
src/sched.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
src/sched.o:  
src/sched.o:  
src/sched.o:  
src/sched.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
src/sched.o:  
src/sched.o:  
src/sched.o:  
src/sched.o: src/sched.h src/util.h 
src/sched.o: 
src/util.o: src/util.h  
src/util.o:  
src/util.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stddef.h
src/util.o:  
src/util.o:  
src/util.o:  
src/util.o: /usr/lib/gcc-lib/i486-suse-linux/2.95.3/include/stdarg.h
src/util.o:  
src/util.o:  
src/util.o:  
src/util.o:  
