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
MANIFEST=mymanifest
CLASSES=$(JAVAS:.java=.class)
LINKCLASSES=*.class
BUILDLIST=.build.lst

# to speed up compilation this Makefile creates the file $(BUILDLIST)
# containing all .java files that need recompilation.

all:
	test \! -f $(BUILDLIST) || rm $(BUILDLIST)
	$(MAKE) updatejar

%.class : %.java
		@-((test -f $@ && rm $@) || true)
		@echo "$<" >>$(BUILDLIST)


#JFLAGS=-deprecation -O -g:none# optimized version
JFLAGS=-deprecation -g# debug version

compile:
		@echo javac $(JFLAGS) `cat $(BUILDLIST)`
		@javac $(JFLAGS) `cat $(BUILDLIST)`

$(JAR) : $(CLASSES) $(JAVAS) Makefile $(MANIFEST)
		test \! -f $(BUILDLIST) || ($(MAKE) compile && rm $(BUILDLIST))
		$(MAKE) linkjar

linkjar:
		jar -cmf $(MANIFEST) $(JAR) $(LINKCLASSES)

rebuild: clean all

updatejar: $(JAR)
	test \! -d $(ARBHOME)/PROBE_SERVER/ps_serverdata || cp -p $(JAR) $(ARBHOME)/PROBE_SERVER/ps_serverdata/

#test: all
#	(	mkdir -p $(TESTDIR)/debug ;\
#	        cd $(TESTDIR)/debug ;\
#		echo "----------------------------------------" ;\
#		echo "OOOOOOOOOOOOPS - incorrect server address!!!" ;\
#		java -jar $(BASE)/$(JAR) server=http://www2.mikro.biologie.tu-muenchen.de/probeserver24367472/ ;\
#		echo "----------------------------------------" ;\
#	        ls -al -rt )

testmaster: all
	(	mkdir -p $(TESTDIR)/master ;\
		cd $(TESTDIR)/master ;\
		echo "----------------------------------------" ;\
		java -jar $(BASE)/$(JAR) ;\
		echo "----------------------------------------" ;\
		ls -al -rt )

testmasterdebug: all
	(	mkdir -p $(TESTDIR)/master ;\
		cd $(TESTDIR)/master ;\
		echo "----------------------------------------" ;\
		java -jar $(BASE)/$(JAR) debug=1 ;\
		echo "----------------------------------------" ;\
		ls -al -rt )

testmasterproxy127: all
	(	mkdir -p $(TESTDIR)/master ;\
		cd $(TESTDIR)/master ;\
		echo "----------------------------------------" ;\
		java -DproxySet=true -DproxyHost=hi.er -jar $(BASE)/$(JAR) debug=1 ;\
		echo "----------------------------------------" ;\
		ls -al -rt )

testmasterproxytrance: all
	(	mkdir -p $(TESTDIR)/master ;\
		cd $(TESTDIR)/master ;\
		echo "----------------------------------------" ;\
		java -DproxySet=true -DproxyHost=trance -jar $(BASE)/$(JAR) debug=1 ;\
		echo "----------------------------------------" ;\
		ls -al -rt )

rstest: all
	(	mkdir -p $(TESTDIR)/local ;\
		cd $(TESTDIR)/local ;\
		echo "----------------------------------------" ;\
		java -jar $(BASE)/$(JAR) server=http://hi.er/probeserver/ debug=1 ;\
		echo "----------------------------------------" ;\
		ls -al -rt )

rstestproxy: all
	(	mkdir -p $(TESTDIR)/local ;\
		cd $(TESTDIR)/local ;\
		echo "----------------------------------------" ;\
		java -jar $(BASE)/$(JAR) server=http://hi.er/probeserver/ debug=1 proxy=127.0.0.1 ;\
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

tags:
		etags *.java


