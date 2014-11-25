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


