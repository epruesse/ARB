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
	@echo "    test        Run $(JAR) on DEBUG server"
	@echo "    test_master Run $(JAR) on Master server"
	@echo "    test_rs     Run $(JAR) on my local server (Ralf)"
	@echo "    clean       remove made files"
	@echo "    depends     update dependencies"
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
		jar -cmf mymanifest $(JAR) *.class
# $(CLASSES)

update:
	jar -uf $(JAR) *.class

test: all
	(	mkdir -p $(TESTDIR)/debug ;\
	        cd $(TESTDIR)/debug ;\
		echo "----------------------------------------" ;\
		echo "OOOOOOOOOOOOPS - incorrect server address!!!" ;\
		java -jar $(BASE)/$(JAR) server=http://www2.mikro.biologie.tu-muenchen.de/probeserver24367472/ ;\
		echo "----------------------------------------" ;\
	        ls -al -rt )

test_master: all
	(	mkdir -p $(TESTDIR)/master ;\
		cd $(TESTDIR)/master ;\
		echo "----------------------------------------" ;\
		java -jar $(BASE)/$(JAR)  ;\
		echo "----------------------------------------" ;\
		ls -al -rt )

test_rs: all
	(	mkdir -p $(TESTDIR)/local ;\
		cd $(TESTDIR)/local ;\
		echo "----------------------------------------" ;\
		java -jar $(BASE)/$(JAR) server=http://hi.er/probeserver/ ;\
		echo "----------------------------------------" ;\
		ls -al -rt )

clean:
	@-rm		*.class \
			$(JAR) \
			.depends \

# dependencies

include .depends

.depends :
		$(ARBHOME)/SOURCE_TOOLS/make_java_dependencies.sh > .depends

depends:
		rm .depends
		$(MAKE) .depends


