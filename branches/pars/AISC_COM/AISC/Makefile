THIS=AISC/Makefile
MAKETHIS=$(MAKE) -f $(THIS)
#MAKETHIS=$(MAKE) -d -f $(THIS)
SED:=$(ARBHOME)/SH/arb_sed
READLINK:=$(ARBHOME)/SH/arb_readlink

.SUFFIXES: .o .c .h .aisc

#where to find the creator files
AISC=../AISC/aisc
AISC_MKPT = ../AISC_MKPTPS/aisc_mkpt

RUN_AISC:=$(AISC)
# valgrinding like this breaks the build process (due to wrong exitcode)
#RUN_AISC:=arb_valgrind -c 15 $(AISC)
#RUN_AISC:=arb_valgrind -c 15 -l $(AISC)
#RUN_AISC:=arb_valgrind -c 15 -l -r $(AISC)

GEN_INCLUDES = GENH/aisc.h GENH/aisc_com.h
INCLUDE_INCLUDES = aisc_include.header

SERVER_OBJECTS = O/server.o O/struct_man.o O/aisc_extern.o

GEN_SERVER_PROTO_OBJECTS = GENC/aisc_server.o
GEN_SERVER_OTHER_OBJECTS = GENC/aisc_global.o
GEN_SERVER_OBJECTS = $(GEN_SERVER_PROTO_OBJECTS) $(GEN_SERVER_OTHER_OBJECTS)

GEN_SERVER_PROTO_SOURCES = $(GEN_SERVER_PROTO_OBJECTS:.o=.c)
GEN_SERVER_SOURCES = $(GEN_SERVER_OBJECTS:.o=.c)

GEN_SERVER_INCLUDES = C/aisc_server.h
GEN_SERVER_PROTO = GENH/aisc_server_proto.h
GEN_SERVER_EXTERN_PROTO = GENH/aisc_server_extern.h

SERVER_INCLUDES = $(GEN_INCLUDES) $(GEN_SERVER_INCLUDES) $(GEN_SERVER_PROTO) $(GEN_SERVER_EXTERN_PROTO)

CLIENT_OBJECTS = O/client.o 
CLIENT_INCLUDES = $(GEN_INCLUDES) C/client_privat.h C/client.h

COMMON_OBJECTS = 
COMMON_INCLUDES =

AISC_EXTERNALS = C/aisc_extern.c $(PRIVATE_SERVER_OBJECTS:.o=.c)
GLOBALS_AISC = GENH/global.aisc

AISC_DEPENDS = AISC/*.pa $(MAIN_SOURCE) $(AISC) $(DUMPDIR) $(GLOBALS_AISC)

IMPORT_PROTO = GENH/import_proto.h

SERVER = server.a
CLIENT = client.a

AISC_SERVER_EXTERN = GENH/aisc_server_extern.aisc

DUMPDIR=DUMP

LINK_TO_OTHER_ARB=~/ARB.aisc_regression_test_vs# create this as symlink to other ARB checkout to activate regression test 
FAIL_ON_CHANGE=1# 0=continue on change, 1=fail on change

# ----------------------------------------

COMPARE_WITH_OTHER_ARB:=$(shell $(READLINK) $(LINK_TO_OTHER_ARB))
ifneq ($(COMPARE_WITH_OTHER_ARB),)
THIS__COMDIR=$(shell pwd)
COMDIRNAME=$(subst $(ARBHOME),,$(THIS__COMDIR))
OTHER_COMDIR=$(subst //,/,$(COMPARE_WITH_OTHER_ARB)/$(COMDIRNAME))
ifeq ($(FAIL_ON_CHANGE),0)
REGR_FAILURE=(echo "$(THIS):57: Warning: regression on generated code (ignored)" || true)
else
REGR_FAILURE=(	echo "$(THIS):59: Error: regression on generated AISC code"; \
		rm $(SERVER) $(CLIENT) $(COMMON); \
		false) 
endif
endif

# ----------------------------------------

ifeq ($(AUTODEPENDS),1)
all:
	+test -f .depends || $(MAKETHIS) "AUTODEPENDS=0" .depends
	+$(MAKETHIS) "AUTODEPENDS=2" all
else
all: directories
	+$(MAKETHIS) realall

endif

realall: $(CLIENT) $(SERVER) $(COMMON) Makefile $(THIS)
ifeq ($(COMPARE_WITH_OTHER_ARB),)
	@echo "AISC regression tests are disabled ($(LINK_TO_OTHER_ARB) not found from AISC_COM/$(THIS))"
else
 ifeq ($(THIS__COMDIR),$(OTHER_COMDIR))
	@echo "$(THIS):63: Warning: Skipping AISC regression tests (test versus self always ok)"
 else
	@echo "Running AISC regression tests (versus $(COMPARE_WITH_OTHER_ARB))"
#       compare generated code using ../../SOURCE_TOOLS/check_dirs_equal.sh
	@( \
		../SOURCE_TOOLS/check_dirs_equal.sh $(THIS__COMDIR)/GENC $(OTHER_COMDIR)/GENC '*.c' && \
		../SOURCE_TOOLS/check_dirs_equal.sh $(THIS__COMDIR)/GENH $(OTHER_COMDIR)/GENH '*.h' && \
		../SOURCE_TOOLS/check_dirs_equal.sh $(THIS__COMDIR)/DUMP $(OTHER_COMDIR)/DUMP '*.dump' && \
		echo "No change in generated code" \
	) || $(REGR_FAILURE)
 endif
endif

pregenerate: $(GEN_SERVER_SOURCES) $(SERVER_INCLUDES) $(CLIENT_INCLUDES) $(AISC_EXTERNALS)

directories:
	@mkdir -p GENH GENC O DUMP

GENH:	directories
GENC:	directories
O:	directories
DUMP:	directories

$(SERVER): $(SERVER_OBJECTS) $(GEN_SERVER_OBJECTS) $(PRIVATE_SERVER_OBJECTS)
	$(LINK_STATIC_LIB) $@ $^

$(CLIENT): $(CLIENT_OBJECTS) $(GEN_CLIENT_OBJECTS)
	$(LINK_STATIC_LIB) $@ $^

$(COMMON): $(COMMON_OBJECTS)
	$(LINK_STATIC_LIB) $@ $^ 

$(GEN_SERVER_OBJECTS) $(PRIVATE_SERVER_OBJECTS): $(SERVER_INCLUDES) $(GEN_SERVER_SOURCES) $(PRIVATE_SERVER_OBJECTS:.o=.c)
	$(COMPILER) -c -o $@ $(@:.o=.c) -IGENH -I. -IC -DAISC_SAVE_$(AISC_SAVE) $(POST_COMPILE) $<

O/%.o: C/%.c $(GEN_INCLUDES) $(CLIENT_INCLUDES) C/server.h
	$(COMPILER) -c -o $@ $< -IGENH -I. -IC $(POST_COMPILE) $<

$(CLIENT_OBJECTS): $(CLIENT_OBJECTS:O/%.o=C/%.c) $(GEN_INCLUDES) $(CLIENT_INCLUDES)
	$(CLIENTCOMPILER) -c -o $@ C/$(@F:.o=.c) -IGENH -I. -IC $(POST_COMPILE) $<

$(DUMPDIR): 
	mkdir -p $(DUMPDIR)

$(GLOBALS_AISC).tmp: 
	@echo "AISC_SAVE $(AISC_SAVE);" > $@

$(GLOBALS_AISC): $(GLOBALS_AISC).tmp
	$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $(GLOBALS_AISC).tmp $(GLOBALS_AISC)

GENH/%.h: AISC/%.pa $(AISC_DEPENDS) $(INCLUDE_INCLUDES) 
	$(RUN_AISC) $< $(MAIN_SOURCE) $@

GENC/%.c: AISC/%.pa $(AISC_DEPENDS) $(AISC_SERVER_EXTERN) $(GEN_SERVER_INCLUDES)
	$(RUN_AISC) $< $(MAIN_SOURCE) $@ $(AISC_SERVER_EXTERN) $(IMPORT_PROTO) $(GLOBALS_AISC)

$(IMPORT_PROTO): $(GEN_SERVER_PROTO_SOURCES)
	echo $@

$(AISC_SERVER_EXTERN): $(AISC_EXTERNALS) $(AISC_MKPT) $(THIS) 
	rm -f $(AISC_SERVER_EXTERN)
	$(AISC_MKPT) -a $(AISC_EXTERNALS) >$@

$(GEN_SERVER_EXTERN_PROTO): $(AISC_EXTERNALS) $(AISC_MKPT) $(THIS)
	rm -f $(GEN_SERVER_EXTERN_PROTO)
	$(AISC_MKPT) -w $(subst GENH/,,$@) $(AISC_EXTERNALS) >$@

$(GEN_SERVER_PROTO): $(GEN_SERVER_PROTO_SOURCES) $(AISC_MKPT) $(THIS)
	rm -f $@
	$(AISC_MKPT) -w $(subst GENH/,,$@) $(GEN_SERVER_PROTO_SOURCES) >$@

clean:
	@rm -f *.[ao] */*.[ao]
	@rm -rf GENH GENC DUMP O
	@rm -f .depends

.depends:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) C/*.c GENC/*.c -I GENH -I C -f- -w1 2>/dev/null \
		| grep -v ' /usr' \
		| $(SED) -e 's/^C/O/ig' \
		> .depends_new
	$(ARBHOME)/SOURCE_TOOLS/mv_if_diff .depends_new .depends

ifeq ($(AUTODEPENDS),2)
include .depends
endif
