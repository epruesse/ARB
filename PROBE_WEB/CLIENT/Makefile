# Makefile for generation of jar archive from probe client classes

.SUFFIXES = .java .class .jar

BASE=$(ARBHOME)/PROBE_WEB/CLIENT
JAR=arb_probe_library.jar
TESTDIR=$(BASE)/testdir

ifeq ('$(MAIN)','CLIENT.a')
		TARGET=all
else
		TARGET=help_local
endif

main_target : $(TARGET)

help_local: help
		@echo "also built by 'make pc' in main ARB makefile"
		@echo ""

help:
	@echo "TARGET=$(TARGET)"
	@echo "MAIN=$(MAIN)"
	@echo ""
	@echo "    target      description"
	@echo "   ----------------------------------------------------------------------"
	@echo "    all         Build $(JAR)"
	@echo "    test        Run $(JAR)"
	@echo "    clean       remove made files"
	@echo ""

JAVAS=$(wildcard *.java)
CLASSES=$(JAVAS:.java=.class)
BUILDLIST=build.lst

all: $(JAR)

#.java.class:
%.class : %.java
		@-((test -f $@ && rm $@) || true)
		@echo "$<" >>$(BUILDLIST)

#		javac $(JAVAS)

compile:
		@echo javac `cat $(BUILDLIST)`
		@javac `cat $(BUILDLIST)`

$(JAR) : $(CLASSES) $(JAVAS)
		-(test -f $(BUILDLIST) && ($(MAKE) compile || rm $(BUILDLIST)))
		-@rm $(BUILDLIST)
		jar -cmf mymanifest $(JAR) $(CLASSES)

update:
	jar -uf $(JAR) *.class

test: all
	( mkdir -p $(TESTDIR); cd $(TESTDIR); java -jar $(BASE)/$(JAR); ls -al -rt )

test_rs: all
	(	mkdir -p $(TESTDIR) ;\
		cd $(TESTDIR) ;\
		java -jar $(BASE)/$(JAR) server=http://hi.er/probeserver/ ;\
		ls -al -rt )

clean:
	@-rm		*.class \
				$(JAR) \
				probetree.gz \
				.depends \

# dependencies

include .depends

.depends :
		$(ARBHOME)/SOURCE_TOOLS/make_java_dependencies.sh > .depends

depends:
		rm .depends
		$(MAKE) .depends


