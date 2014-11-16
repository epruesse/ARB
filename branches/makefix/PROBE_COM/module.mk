module:=probe_com
probe_com_TARGETS := $(dir)server.a
probe_com_HEADERS := PT_com.h PT_server.h PT_server_prototypes.h
probe_com_AISC    := PT.aisc
probe_com_AISC_SAVE := NO
probe_com_AISC_EXTERN := PT_extern.c

PROBE_COM/PT_com.h:               $(dir)GENH/aisc_com.h
	cp $^ $@
PROBE_COM/PT_server.h:            $(dir)GENH/aisc.h
	cp $^ $@
PROBE_COM/PT_server_prototypes.h: $(dir)GENH/aisc_server_proto.h
	cp $^ $@
