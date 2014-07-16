# MrBayes Makefile modified for use in ARB by Arne Boeckmann (August 2013)

.SUFFIXES: .o .c .depend

CC     ?= gcc
CFLAGS  := -O3 -ffast-math -w -DNDEBUG -DUSECONFIG_H
LDLIBS  := -lreadline -lm

ifeq ($(ARB_64),1)
	CFLAGS += -DARB_64
endif
ifeq ($(DARWIN),1)
	CFLAGS += -DARB_DARWIN
endif

# work around bug in centos5 -- ep 8/7/2013
# (linking readline causes missing symbols unless also linking ncurses)
# see https://bugzilla.redhat.com/show_bug.cgi?id=499837
HAVE_CENTOS5 := $(shell test -f /etc/redhat-release && grep -q "release 5" /etc/redhat-release && echo true)
ifeq ($(HAVE_CENTOS5),true)
    LDLIBS += -lncurses
endif

OBJECTS := mb.o bayes.o command.o mbmath.o mcmc.o model.o plot.o sump.o sumt.o tree.o utils.o mbbeagle.o best.o

mb: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDLIBS) -o $@

%.o: %.c 
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm -f *.o *~ mb

DEPENDS := $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	echo "DEPENDS='$(DEPENDS)'"
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $(CFLAGS) $< 2>/dev/null >$@

.PHONY: depends depend.init clean

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from GDE/MrBayes)

bayes.o: bayes.h
bayes.o: command.h
bayes.o: config.h
bayes.o: globals.h
bayes.o: mb.h
bayes.o: mcmc.h
bayes.o: model.h
bayes.o: sumt.h
bayes.o: utils.h

best.o: best.h
best.o: command.h
best.o: config.h
best.o: globals.h
best.o: mb.h
best.o: mbmath.h
best.o: mcmc.h
best.o: model.h
best.o: sumt.h
best.o: tree.h
best.o: utils.h

command.o: bayes.h
command.o: command.h
command.o: config.h
command.o: globals.h
command.o: mb.h
command.o: mbbeagle.h
command.o: mcmc.h
command.o: model.h
command.o: plot.h
command.o: sump.h
command.o: sumt.h
command.o: tree.h
command.o: utils.h

mbbeagle.o: config.h
mbbeagle.o: globals.h
mbbeagle.o: mb.h
mbbeagle.o: mbbeagle.h
mbbeagle.o: mcmc.h
mbbeagle.o: model.h
mbbeagle.o: utils.h

mbmath.o: bayes.h
mbmath.o: config.h
mbmath.o: globals.h
mbmath.o: mb.h
mbmath.o: mbmath.h
mbmath.o: model.h
mbmath.o: utils.h

mcmc.o: bayes.h
mcmc.o: best.h
mcmc.o: command.h
mcmc.o: config.h
mcmc.o: globals.h
mcmc.o: mb.h
mcmc.o: mbbeagle.h
mcmc.o: mbmath.h
mcmc.o: mcmc.h
mcmc.o: model.h
mcmc.o: plot.h
mcmc.o: sump.h
mcmc.o: sumt.h
mcmc.o: tree.h
mcmc.o: utils.h

model.o: bayes.h
model.o: best.h
model.o: command.h
model.o: config.h
model.o: globals.h
model.o: mb.h
model.o: mbmath.h
model.o: mcmc.h
model.o: model.h
model.o: tree.h
model.o: utils.h

plot.o: bayes.h
plot.o: command.h
plot.o: config.h
plot.o: globals.h
plot.o: mb.h
plot.o: plot.h
plot.o: sump.h
plot.o: utils.h

sump.o: bayes.h
sump.o: command.h
sump.o: config.h
sump.o: globals.h
sump.o: mb.h
sump.o: mbmath.h
sump.o: mcmc.h
sump.o: sump.h
sump.o: utils.h

sumt.o: bayes.h
sumt.o: command.h
sumt.o: config.h
sumt.o: globals.h
sumt.o: mb.h
sumt.o: mbmath.h
sumt.o: mcmc.h
sumt.o: model.h
sumt.o: sump.h
sumt.o: sumt.h
sumt.o: tree.h
sumt.o: utils.h

tree.o: bayes.h
tree.o: best.h
tree.o: command.h
tree.o: config.h
tree.o: globals.h
tree.o: mb.h
tree.o: mbmath.h
tree.o: mcmc.h
tree.o: model.h
tree.o: tree.h
tree.o: utils.h

utils.o: config.h
utils.o: globals.h
utils.o: mb.h
utils.o: utils.h
