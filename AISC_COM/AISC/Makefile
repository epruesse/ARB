#the next lines should be defined in the meta Makefile

COMPILER = acc -g
AR = ld -r -o

.SUFFIXES: .o .c .h .aisc

#where to find the creator files
AISC = ../MAKEBIN/aisc
AISC_MKPT = ../MAKEBIN/aisc_mkpt

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

all:	$(CLIENT) $(SERVER)

$(SERVER):	$(SERVER_OBJECTS) $(GEN_SERVER_OBJECTS) $(PRIVATE_SERVER_OBJECTS)
	$(AR) $@ $^

$(CLIENT):	$(CLIENT_OBJECTS) $(GEN_CLIENT_OBJECTS)
	$(AR) $@ $^

$(GEN_SERVER_OBJECTS) $(PRIVATE_SERVER_OBJECTS): \
					$(GEN_INCLUDES) $(GEN_SERVER_PROTO)  \
					$(GEN_SERVER_EXTERN_PROTO) $(IMPORT_PROTO) \
					$(GEN_SERVER_OBJECTS:.o=.c) \
					$(PRIVATE_SERVER_OBJECTS:.o=.c)
	$(COMPILER) -Wno-unused -c -o $@ $(@:.o=.c) -IGENH -I. -IC -DAISC_SAVE_$(AISC_SAVE)

$(GEN_CLIENT_OBJECTS): $(GEN_INCLUDES) $(GEN_CLIENT_OBJECTS:.o=.c)
	$(CLIENTCOMPILER) -Wno-unused -c -o $@ $(@:.o=.c) -IGENH -I. -IC

O/%.o: C/%.c $(GEN_INCLUDES) $(CLIENT_INCLUDES) C/server.h
	$(COMPILER) -Wno-unused -c -o $@ $< -IGENH -I. -IC

$(CLIENT_OBJECTS):	$(CLIENT_OBJECTS:O/%.o=C/%.c) $(GEN_INCLUDES) $(CLIENT_INCLUDES)
	$(CLIENTCOMPILER) -Wno-unused -c -o $@ C/$(@F:.o=.c) -IGENH -I. -IC

GENH/%.h: AISC/%.pa AISC/*.pa $(MAIN_SOURCE) $(INCLUDE_INCLUDES)
	@echo "----------------------------------------"
	@echo "1 - LD_LIBRARY_PATH=$(LD_LIBRARY_PATH)"
#	ldd $(AISC)
	$(AISC) $< $(MAIN_SOURCE) $@

GENC/%.c: AISC/%.pa AISC/*.pa $(MAIN_SOURCE) $(AISC_SERVER_EXTERN)
	@echo "----------------------------------------"
	@echo "2 - LD_LIBRARY_PATH=$(LD_LIBRARY_PATH)"
#	ldd $(AISC)
	$(AISC) $< $(MAIN_SOURCE) $@ $(AISC_SERVER_EXTERN) $(IMPORT_PROTO)


$(IMPORT_PROTO): GENC/aisc_server.c
	echo $@

$(AISC_SERVER_EXTERN): $(AISC_EXTERNALS)
	rm -f $(AISC_SERVER_EXTERN)
	$(AISC_MKPT) -a $(AISC_EXTERNALS) >$@

$(GEN_SERVER_EXTERN_PROTO): $(AISC_EXTERNALS)
	rm -f $(GEN_SERVER_EXTERN_PROTO)
	$(AISC_MKPT) -C $(AISC_EXTERNALS) >$@

$(GEN_SERVER_PROTO): $(S_GEN_SERVER_PROTO)
	rm -f $@
	$(AISC_MKPT) -C $(S_GEN_SERVER_PROTO) >$@

clean:
	rm -f *.[ao]
	rm -f */*.[ao]
	rm -f $(OBJECTS:.o=)
	rm -f *%
	rm -f */*%
	rm -f core
	rm -f */core
	rm -f GENH/*
	rm -f GENC/*

depends:
	makedepend C/*.c GENC/*.c -I GENH -I C -f- -w1 | grep -v ' /usr' | sed -e 's/^C/O/ig' > .depends_new
	$(ARBHOME)/SOURCE_TOOLS/mv_if_diff .depends_new .depends

.depends: depends

include .depends
