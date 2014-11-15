module:=aisc_c
aisc_c_MKPT_HEADERS:=aisc_extern_privat.h client.h struct_man.h server.h
aisc_c_HEADERS := $(aisc_c_MKPT_HEADERS) client_privat.h client_types.h aisc_global.h

aisc_extern_privat_h_MKPT_SRCS := aisc_extern.c
aisc_extern_privat_h_MKPT_FLAGS := -P -G
client_h_MKPT_SRCS := client.c
client_h_MKPT_FLAGS := -P -G
struct_man_h_MKPT_SRCS := struct_man.c
struct_man_h_MKPT_FLAGS := -P -G
server_h_MKPT_SRCS := server.c
server_h_MKPT_FLAGS := -P -G
