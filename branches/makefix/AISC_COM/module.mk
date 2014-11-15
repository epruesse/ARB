module:=aisc_client
aisc_client_TARGETS := $(dir)client.a
aisc_client_SOURCES := $(dir)C/client.c

AISC_PA:=$(dir)AISC/aisc.pa
AISC_COM_PA:=$(dir)AISC/aisc_com.pa
AISC_SERVER_PA:=$(dir)AISC/aisc_server.pa
AISC_GLOBAL_PA:=$(dir)AISC/aisc_global.pa

%GENH/aisc.h:
	$(call prettyprint,\
	(cd $($(module)_DIR); \
	 $(SOURCE_DIR)/$(AISC) $(SOURCE_DIR)/$(AISC_PA) $($(module)_AISC) GENH/aisc.h),\
	AISC)

%GENH/aisc_com.h:
	$(call prettyprint,\
	(cd $($(module)_DIR); \
	 $(SOURCE_DIR)/$(AISC) $(SOURCE_DIR)/$(AISC_COM_PA) $($(module)_AISC) GENH/aisc_com.h),\
	AISC)

%GENH/global.aisc:
	$(call prettyprint,\
	echo AISC_SAVE $($(module)_AISC_SAVE) > $@\
	,ECHO)


%GENC/aisc_server.c:
	$(call prettyprint,\
	(cd $($(module)_DIR); \
	 $(SOURCE_DIR)/$(AISC) $(SOURCE_DIR)/$(AISC_SERVER_PA) $($(module)_AISC) \
	 GENC/aisc_server.c GENH/aisc_server_extern.aisc GENH/import_proto.h GENH/global.aisc )\
	,AISC)


