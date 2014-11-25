module:=probe_com
probe_com_TARGETS     := $(dir)server.a
probe_com_AISC        := PT.aisc
probe_com_AISC_SAVE   := NO
probe_com_AISC_EXTERN := PT_extern.c
probe_com_AISC_CLIENT := PT_com.h
probe_com_AISC_PROTO  := PT_server_prototypes.h
probe_com_AISC_SERVER := PT_server.h
