THIS=AISC/Makefile
MAKETHIS=$(MAKE) -f $(THIS)
#MAKETHIS=$(MAKE) -d -f $(THIS)

# dont warn about unused functions (@@@ fix)
FORCED_FLAGS=-Wno-unused

.SUFFIXES: .o .c .h .aisc

#where to find the creator files
AISC=../MAKEBIN/aisc
AISC_MKPT = ../MAKEBIN/aisc_mkpt

RUN_AISC:=$(AISC)
# valgrinding like this breaks the build process (due to wrong exitcode)
#RUN_AISC:=arb_valgrind -c 15 $(AISC)
#RUN_AISC:=arb_valgrind -c 15 -l $(AISC)
#RUN_AISC:=arb_valgrind -c 15 -l -r $(AISC)

GEN_INCLUDES = GENH/aisc.h GENH/aisc_com.h
INCLUDE_INCLUDES = aisc_include.header

GEN_SERVER_PROTO = GENH/aisc_server_proto.h
S_GEN_SERVER_PROTO = GENC/aisc_server.c

SERVER_OBJECTS = O/server.o O/struct_man.o O/aisc_extern.o
GEN_SERVER_OBJECTS = GENC/aisc_global.o GENC/aisc_server.o
GEN_SERVER_INCLUDES = C/aisc_server.h
GEN_SERVER_EXTERN_PROTO = GENH/aisc_server_extern.h

CLIENT_OBJECTS = O/client.o O/debug.o
GEN_CLIENT_OBJECTS = GENC/aisc_debug_globals.o
CLIENT_INCLUDES = C/client_privat.h C/client.h

AISC_EXTERNALS	= C/aisc_extern.c $(PRIVATE_SERVER_OBJECTS:.o=.c)

IMPORT_PROTO = GENH/import_proto.h

SERVER = server.a
CLIENT = client.a

AISC_SERVER_EXTERN = GENH/aisc_server_extern.aisc

DUMPDIR=DUMP

LINK_TO_OTHER_ARB=~/ARB.aisc_regression_test_vs# create this as symlink to other ARB checkout to activate regression test 
FAIL_ON_CHANGE=1# 0=continue on change, 1=fail on change

# ----------------------------------------

COMPARE_WITH_OTHER_ARB:=$(shell readlink $(LINK_TO_OTHER_ARB))
ifneq ($(COMPARE_WITH_OTHER_ARB),)
THIS__COMDIR=$(shell pwd)
COMDIRNAME=$(subst $(ARBHOME),,$(THIS__COMDIR))
OTHER_COMDIR=$(subst //,/,$(COMPARE_WITH_OTHER_ARB)/$(COMDIRNAME))
ifeq ($(FAIL_ON_CHANGE),0)
REGR_FAILURE=(echo "$(THIS):57: Warning: regression on generated code (ignored)" || true)
else
REGR_FAILURE=(	echo "$(THIS):59: Error: regression on generated AISC code"; \
		rm $(SERVER) $(CLIENT); \
		false) 
endif
endif

# ----------------------------------------

ifeq ($(AUTODEPENDS),1)
all:
	test -f .depends || $(MAKETHIS) "AUTODEPENDS=0" .depends
	$(MAKETHIS) "AUTODEPENDS=2" all
else
all: directories
	$(MAKETHIS) realall

endif

realall: $(CLIENT) $(SERVER) Makefile $(THIS)
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

pregenerate: $(GEN_INCLUDES) $(GEN_SERVER_PROTO) $(GEN_SERVER_OBJECTS:.o=.c)

directories:
	@mkdir -p GENH GENC O DUMP

$(SERVER): $(SERVER_OBJECTS) $(GEN_SERVER_OBJECTS) $(PRIVATE_SERVER_OBJECTS)
	$(LINK_STATIC_LIB) $@ $^

$(CLIENT): $(CLIENT_OBJECTS) $(GEN_CLIENT_OBJECTS)
	$(LINK_STATIC_LIB) $@ $^

$(GEN_SERVER_OBJECTS) $(PRIVATE_SERVER_OBJECTS): \
					$(GEN_INCLUDES) $(GEN_SERVER_PROTO)  \
					$(GEN_SERVER_EXTERN_PROTO) $(IMPORT_PROTO) \
					$(GEN_SERVER_OBJECTS:.o=.c) \
					$(PRIVATE_SERVER_OBJECTS:.o=.c)
	$(COMPILER) $(FORCED_FLAGS) -c -o $@ $(@:.o=.c) -IGENH -I. -IC -DAISC_SAVE_$(AISC_SAVE)

$(GEN_CLIENT_OBJECTS): $(GEN_INCLUDES) $(GEN_CLIENT_OBJECTS:.o=.c)
	$(CLIENTCOMPILER) $(FORCED_FLAGS) -c -o $@ $(@:.o=.c) -IGENH -I. -IC

O/%.o: C/%.c $(GEN_INCLUDES) $(CLIENT_INCLUDES) C/server.h
	$(COMPILER) $(FORCED_FLAGS) -c -o $@ $< -IGENH -I. -IC

$(CLIENT_OBJECTS):	$(CLIENT_OBJECTS:O/%.o=C/%.c) $(GEN_INCLUDES) $(CLIENT_INCLUDES)
	$(CLIENTCOMPILER) $(FORCED_FLAGS) -c -o $@ C/$(@F:.o=.c) -IGENH -I. -IC

$(DUMPDIR): 
	mkdir -p $(DUMPDIR)

GENH/%.h: AISC/%.pa AISC/*.pa $(MAIN_SOURCE) $(INCLUDE_INCLUDES) $(AISC) $(DUMPDIR) 
	$(RUN_AISC) $< $(MAIN_SOURCE) $@

GENC/%.c: AISC/%.pa AISC/*.pa $(MAIN_SOURCE) $(AISC_SERVER_EXTERN) $(AISC) $(DUMPDIR) $(GEN_SERVER_INCLUDES)
	$(RUN_AISC) $< $(MAIN_SOURCE) $@ $(AISC_SERVER_EXTERN) $(IMPORT_PROTO)

$(IMPORT_PROTO): GENC/aisc_server.c
	echo $@

$(AISC_SERVER_EXTERN): $(AISC_EXTERNALS) $(AISC_MKPT) 
	rm -f $(AISC_SERVER_EXTERN)
	$(AISC_MKPT) -a $(AISC_EXTERNALS) >$@

$(GEN_SERVER_EXTERN_PROTO): $(AISC_EXTERNALS) $(AISC_MKPT)
	rm -f $(GEN_SERVER_EXTERN_PROTO)
	$(AISC_MKPT) -C $(AISC_EXTERNALS) >$@

$(GEN_SERVER_PROTO): $(S_GEN_SERVER_PROTO) $(AISC_MKPT)
	rm -f $@
	$(AISC_MKPT) -C $(S_GEN_SERVER_PROTO) >$@

clean:
	@rm -f *.[ao] */*.[ao]
	@rm -rf GENH GENC DUMP O
	@rm -f .depends

.depends:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) C/*.c GENC/*.c -I GENH -I C -f- -w1 2>/dev/null \
		| grep -v ' /usr' \
		| sed -e 's/^C/O/ig' \
		> .depends_new
	$(ARBHOME)/SOURCE_TOOLS/mv_if_diff .depends_new .depends

ifeq ($(AUTODEPENDS),2)
include .depends
endif
