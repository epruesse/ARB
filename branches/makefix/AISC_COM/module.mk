module:=aisc_com
aisc_com_TARGETS := $(dir)client.a
aisc_com_SOURCES := $(dir)C/client.c
aisc_com_HEADERS := C/client.h C/client_privat.h C/client_types.h C/struct_man.h
aisc_com_HEADERS += C/server.h C/aisc_global.h C/aisc_func_types.h 
aisc_com_MKPT_HEADERS := C/aisc_extern_privat.h C/client.h C/struct_man.h C/server.h

C__aisc_extern_privat_h_MKPT_SRCS := C/aisc_extern.c
C__aisc_extern_privat_h_MKPT_FLAGS := -P -G
C__client_h_MKPT_SRCS := C/client.c
C__client_h_MKPT_FLAGS := -P -G
C__struct_man_h_MKPT_SRCS := C/struct_man.c
C__struct_man_h_MKPT_FLAGS := -P -G
C__server_h_MKPT_SRCS := C/server.c
C__server_h_MKPT_FLAGS := -P -G


AISC_SOURCES := $(call filter-out,$(aisc_com_SOURCES),$(wildcard $(dir)C/*.c))
AISC_HEADER := $(call filter-out,$(aisc_com_HEADERS),$(wildcard $(dir)C/*.h))

AISC_PA:=$(dir)AISC/aisc.pa
AISC_COM_PA:=$(dir)AISC/aisc_com.pa
AISC_SERVER_PA:=$(dir)AISC/aisc_server.pa
AISC_GLOBAL_PA:=$(dir)AISC/aisc_global.pa
AISC_EXTERN_C:=$(dir)C/aisc_extern.c

GENH__aisc_server_proto_h_MKPT_SRCS=GENC/aisc_server.c
GENH__aisc_server_proto_h_MKPT_NAME=aisc_server_proto.h
GENH__aisc_server_extern_h_MKPT_SRCS=../AISC_COM/C/aisc_extern.c $($(module)_AISC_EXTERN)
GENH__aisc_server_extern_h_MKPT_NAME=aisc_extern_proto.h

%GENH/aisc.h: $(AISC_PA) | $(AISC)
	$(call prettyprint,\
	$(AISC) $^ $@,\
	AISC)

%GENH/aisc_com.h: $(AISC_COM_PA) |$(AISC)
	$(call prettyprint,\
	$(AISC)  $^ $@,\
	AISC)

%GENH/global.aisc: 
	$(call prettyprint,\
	echo AISC_SAVE $($(module)_AISC_SAVE) > $@,\
	ECHO)

%GENH/aisc_server_extern.aisc: $(AISC_EXTERN_C) |$(AISC_MKPT) 
	$(call prettyprint,\
	$(AISC_MKPT) -a $^ > $@,\
	MKPT)

%GENH/import_proto.h: %GENC/aisc_server.c
	@true

%GENC/aisc_server.c: $(AISC_SERVER_PA) %GENH/aisc_server_extern.aisc %GENH/global.aisc |$(AISC) 
	$(call prettyprint,\
	$(AISC) $^ $@ $(@:%C/aisc_server.c=%H/import_proto.h) \
	,AISC)

%GENC/aisc_global.c: $(AISC_GLOBAL_PA) %GENH/aisc_server_extern.aisc %GENH/global.aisc |$(AISC) 
	$(call prettyprint,\
	$(AISC) $^ $@ $(@:%C/aisc_server.c=%H/import_proto.h) \
	,AISC)

%GENC/server.c: AISC_COM/C/server.c
	cp $^ $@

%GENC/struct_man.c: AISC_COM/C/struct_man.c
	cp $^ $@

%GENC/aisc_extern.c: AISC_COM/C/aisc_extern.c
	cp $^ $@

%GENH/trace.h: AISC_COM/C/trace.h
	cp $^ $@

%GENH/aisc_extern_privat.h: AISC_COM/C/aisc_extern_privat.h
	cp $^ $@

%GENH/aisc_server.h: AISC_COM/C/aisc_server.h
	cp $^ $@
