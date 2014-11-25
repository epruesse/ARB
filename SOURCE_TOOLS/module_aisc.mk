# Rules for building AISC based RPC layer 
# (used by PROBE_COM and NAMES_COM)

GEN__aisc_server_proto_h_MKPT_SRCS=GEN/aisc_server.c
GEN__aisc_server_proto_h_MKPT_NAME=aisc_server_proto.h

GEN__aisc_server_extern_h_MKPT_SRCS=../AISC_COM/C/aisc_extern.c $($(module)_AISC_EXTERN)
GEN__aisc_server_extern_h_MKPT_NAME=aisc_extern_proto.h

define make_aisc_rules
ifneq (,$$($(1)_AISC))
# built from .aisc config in *_COM dir:
    $(foreach file,aisc_com.h aisc.h aisc_server.c aisc_global.c,
        $($(1)_DIR)GEN/$(file): $($(1)_DIR)$($(1)_AISC) 
    )
    # (import_proto.h is side-effect of aisc_server.c)
    $(1)_AISC_HEADERS  = GEN/aisc.h GEN/aisc_com.h GEN/import_proto.h

# built from config in makefile
    $($(1)_DIR)GEN/global.aisc: module := $(1)

# built from _AISC_EXTERN file (PT_extern.c, names_extern.c)
    $($(1)_DIR)GEN/aisc_server_extern.aisc: $($(1)_DIR)$($(1)_AISC_EXTERN)

# register MKPT generated headers
    $(1)_MKPT_HEADERS += GEN/aisc_server_proto.h GEN/aisc_server_extern.h

# copied from above
    $($(1)_DIR)$($(1)_AISC_SERVER): $($(1)_DIR)GEN/aisc.h  |  $($(1)_DIR)GEN
	$$(CP_CMD)
    $($(1)_DIR)$($(1)_AISC_CLIENT): $($(1)_DIR)GEN/aisc_com.h |  $($(1)_DIR)GEN
	$$(CP_CMD)
    $($(1)_DIR)$($(1)_AISC_PROTO): $($(1)_DIR)GEN/aisc_server_proto.h | $($(1)_DIR)GEN
	$$(CP_CMD)
    $(1)_HEADERS += $($(1)_AISC_SERVER) $($(1)_AISC_CLIENT) $($(1)_AISC_PROTO)
    $(1)_CLEAN += $($(1)_AISC_SERVER) $($(1)_AISC_CLIENT) $($(1)_AISC_PROTO)
    gen_headers += $($(1)_AISC_SERVER) $($(1)_AISC_CLIENT) $($(1)_AISC_PROTO)

# copied from AISC_COM/C
    $(foreach file,server.c struct_man.c aisc_extern.c trace.h aisc_extern_privat.h aisc_server.h,
      $($(1)_DIR)GEN/$(file): AISC_COM/C/$(file) | $($(1)_DIR)GEN
	$$(CP_CMD)
    )
    $(1)_AISC_HEADERS += GEN/trace.h GEN/aisc_extern_privat.h GEN/aisc_server.h
    $(1)_SOURCES += $(patsubst %,$($(1)_DIR)GEN/%, aisc_server.c aisc_global.c \
                      server.c struct_man.c aisc_extern.c)
# extra cpp rules
    $(1)_CPPFLAGS += -I$($(1)_DIR)GEN -DAISC_SAVE_$($(1)_AISC_SAVE)

    $(1)_CLEAN += $($(1)_DIR)GEN
endif
endef

# creates the temp dir for the build
%/GEN:
	$(call prettyprint,mkdir $@,MKDIR)

%GEN/aisc.h: AISC_COM/AISC/aisc.pa | $(AISC) %GEN
	$(call prettyprint,\
	$(AISC) $^ $@,\
	AISC)

%GEN/aisc_com.h: AISC_COM/AISC/aisc_com.pa | $(AISC) %GEN
	$(call prettyprint,\
	$(AISC) $^ $@,\
	AISC)

%GEN/global.aisc: | %GEN
	$(call prettyprint,\
	echo AISC_SAVE $($(module)_AISC_SAVE) > $@,\
	ECHO)

%GEN/aisc_server_extern.aisc: AISC_COM/C/aisc_extern.c | $(AISC_MKPT) %GEN
	$(call prettyprint,\
	$(AISC_MKPT) -a $^ > $@,\
	MKPT-a)

%GEN/import_proto.h: %GEN/aisc_server.c |%GEN
	@:

%GEN/aisc_server.c: AISC_COM/AISC/aisc_server.pa %GEN/aisc_server_extern.aisc %GEN/global.aisc | $(AISC)  %GEN
	$(call prettyprint,\
	$(AISC) $^ $@ $(@:%aisc_server.c=%import_proto.h) \
	,AISC)

%GEN/aisc_global.c: AISC_COM/AISC/aisc_global.pa %GEN/aisc_server_extern.aisc %GEN/global.aisc | $(AISC)  %GEN
	$(call prettyprint,\
	$(AISC) $^ $@ $(@:%aisc_server.c=%import_proto.h) \
	,AISC)
