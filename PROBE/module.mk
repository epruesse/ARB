module:=pt
pt_TARGETS:=bin/arb_pt_server
pt_LIBADD:=probe_com db servercntrl helix ptclean aisc_com
pt_HEADERS:=PT_global_defs.h
pt_MKPT_HEADERS:=pt_prototypes.h
pt_prototypes_h_MKPT_SRCS=$(patsubst PROBE/%,%,$(pt_SOURCES))
pt_prototypes_h_MKPT_FLAGS=-P -E
