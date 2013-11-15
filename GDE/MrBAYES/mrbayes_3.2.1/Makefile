#MrBayes Makefile modified for use in ARB by Arne Boeckmann (August 2013)

CC     ?= gcc
CFLAGS  = -O3 -ffast-math -Wall -DNDEBUG -DUSECONFIG_H
LDLIBS  = -lreadline -lm

# work around bug in centos5 -- ep 8/7/2013
# (linking readline causes missing symbols unless also linking ncurses)
# see https://bugzilla.redhat.com/show_bug.cgi?id=499837
HAVE_CENTOS5 = $(shell test -f /etc/redhat-release && grep -q "release 5" /etc/redhat-release && echo true)
ifeq ($(HAVE_CENTOS5),true)
    LDLIBS += -lncurses
endif

OBJECTS:= mb.o bayes.o command.o mbmath.o mcmc.o model.o plot.o sump.o sumt.o tree.o utils.o mbbeagle.o best.o

mb: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDLIBS) -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm -f *.o *~ mb

bayes.o: bayes.c mb.h globals.h bayes.h command.h mcmc.h model.h utils.h
command.o: command.c mb.h globals.h command.h bayes.h model.h mcmc.h \
  plot.h sump.h sumt.h tree.h utils.h
mbmath.o: mbmath.c mb.h globals.h mbmath.h bayes.h model.h utils.h
mcmc.o: mcmc.c mb.h globals.h bayes.h mcmc.h model.h command.h mbmath.h \
  sump.h sumt.h plot.h tree.h utils.h mbbeagle.h
model.o: model.c mb.h globals.h bayes.h model.h command.h mcmc.h mbmath.h \
  tree.h utils.h
plot.o: plot.c mb.h globals.h command.h bayes.h plot.h sump.h utils.h
  sump.o: sump.c mb.h globals.h command.h bayes.h sump.h mcmc.h utils.h
sumt.o: sumt.c mb.h globals.h command.h bayes.h mbmath.h sumt.h sump.h \
  mcmc.h model.h utils.h
tree.o: tree.c bayes.h mb.h mcmc.h mbmath.h model.h globals.h tree.h \
  utils.h
utils.o: utils.c mb.h globals.h utils.h
mbbeagle.o: mbbeagle.c mbbeagle.h utils.h
best.o: best.c best.h
