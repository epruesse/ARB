module:=aisc

AISC:=$(dir)aisc
aisc_TARGETS  := $(dir)aisc
aisc_CPPFLAGS := -DSIMPLE_ARB_ASSERT
aisc_MKPT_HEADERS := aisc_proto.h
aisc_proto_h_MKPT_FLAGS := -P
aisc_proto_h_MKPT_SRCS = $(notdir $(aisc_SOURCES))


# don't add the ARB standard main wrapper to aisc:
$(dir)aisc: HAS_MAIN=1


