module:=probe_com
probe_com_TARGETS := $(dir)server.a
probe_com_HEADERS := PT_com.h PT_server.h PT_server_prototypes.h
probe_com_AISC    := PT.aisc
probe_com_AISC_SAVE := NO

PROBE_COM/PT_com.h:               $(dir)GENH/aisc_com.h
	cp $^ $@
PROBE_COM/PT_server.h:            $(dir)GENH/aisc.h
	cp $^ $@
PROBE_COM/PT_server_prototypes.h: $(dir)GENH/aisc_server_proto.h
	cp $^ $@



# AISC AISC/aisc_server.pa PT.aisc GENC/aisc_server.c GENH/aisc_server_extern.aisc GENH/import_proto.h GENH/global.aisc
# AISC AISC/aisc_global.pa PT.aisc GENC/aisc_global.c GENH/aisc_server_extern.aisc GENH/import_proto.h GENH/global.aisc


GENH/aisc.h: PT.aisc


# O/client.o C/client.c -IGENH -I. -IC 
# ld client.a O/client.o

# O/server.o C/server.c -IGENH -I. -IC 
# O/struct_man.o C/struct_man.c -IGENH -I. -IC 
# O/aisc_extern.o C/aisc_extern.c -IGENH -I. -IC 
# rm -f GENH/aisc_server_extern.aisc
# aisc_mkpt -a C/aisc_extern.c PT_extern.c >GENH/aisc_server_extern.aisc
# rm -f GENH/aisc_server_proto.h
# ../AISC_MKPTPS/aisc_mkpt -w aisc_server_proto.h GENC/aisc_server.c >GENH/aisc_server_proto.h
# rm -f GENH/aisc_server_extern.h
# ../AISC_MKPTPS/aisc_mkpt -w aisc_server_extern.h C/aisc_extern.c PT_extern.c >GENH/aisc_server_extern.h
# GENC/aisc_server.o GENC/aisc_server.c -IGENH -I. -IC -DAISC_SAVE_NO 
# GENC/aisc_global.o GENC/aisc_global.c -IGENH -I. -IC -DAISC_SAVE_NO
# PT_extern.o PT_extern.c -IGENH -I. -IC -DAISC_SAVE_NO 
# ld  server.a O/server.o O/struct_man.o O/aisc_extern.o GENC/aisc_server.o GENC/aisc_global.o PT_extern.o
