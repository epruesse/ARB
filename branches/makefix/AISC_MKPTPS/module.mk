module:=aisc_mkptps
AISC_MKPT:=$(dir)aisc_mkpt
aisc_mkptps_TARGETS  := $(AISC_MKPT)
aisc_mkptps_CPPFLAGS := -DSIMPLE_ARB_ASSERT
aisc_mkptps_LDADD    := $(use_ARB_main)


