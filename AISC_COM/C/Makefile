
all:
	echo "Available targets: proto"

GENERATED_HEADERS= \
	aisc_extern_privat.h \
	client.h \
	struct_man.h \
	server.h \

AISC_MKPT_FLAGS=-CPE -g

proto: $(GENERATED_HEADERS)

clean:
	rm $(GENERATED_HEADERS)

aisc_extern_privat.h: aisc_extern.c
	$(ARBHOME)/MAKEBIN/aisc_mkpt $(AISC_MKPT_FLAGS) $? >$@

client.h: client.c
	$(ARBHOME)/MAKEBIN/aisc_mkpt $(AISC_MKPT_FLAGS) $? >$@

struct_man.h: struct_man.c
	$(ARBHOME)/MAKEBIN/aisc_mkpt $(AISC_MKPT_FLAGS) $? >$@

server.h: server.c
	$(ARBHOME)/MAKEBIN/aisc_mkpt $(AISC_MKPT_FLAGS) $? >$@
