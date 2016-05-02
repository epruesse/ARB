
all:
	echo "Available targets: proto"

GENERATED_HEADERS= \
	aisc_extern_privat.h \
	client.h \
	common.h \
	server.h \
	struct_man.h \

AISC_MKPT_FLAGS=-P -G
MKPT_DEP=../../AISC_MKPTPS/aisc_mkpt Makefile

proto:
	$(MAKE) $(GENERATED_HEADERS)

clean:
	rm $(GENERATED_HEADERS)

aisc_extern_privat.h: aisc_extern.c $(MKPT_DEP)  
	$(ARBHOME)/AISC_MKPTPS/aisc_mkpt $(AISC_MKPT_FLAGS) -w $@ $< >$@.tmp
	$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $@.tmp $@

client.h: client.c $(MKPT_DEP)
	$(ARBHOME)/AISC_MKPTPS/aisc_mkpt $(AISC_MKPT_FLAGS) -w $@ $< >$@.tmp
	$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $@.tmp $@

common.h: common.c $(MKPT_DEP)
	$(ARBHOME)/AISC_MKPTPS/aisc_mkpt $(AISC_MKPT_FLAGS) -w $@ $< >$@.tmp
	$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $@.tmp $@

struct_man.h: struct_man.c $(MKPT_DEP)
	$(ARBHOME)/AISC_MKPTPS/aisc_mkpt $(AISC_MKPT_FLAGS) -w $@ $< >$@.tmp
	$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $@.tmp $@

server.h: server.c $(MKPT_DEP)
	$(ARBHOME)/AISC_MKPTPS/aisc_mkpt $(AISC_MKPT_FLAGS) -w $@ $< >$@.tmp
	$(ARBHOME)/SOURCE_TOOLS/mv_if_diff $@.tmp $@


