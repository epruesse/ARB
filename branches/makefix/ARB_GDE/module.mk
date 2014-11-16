module:=arb_gde
arb_gde_TARGETS := $(dir)/ARB_GDE.a
arb_gde_HEADERS := gde.hxx
arb_gde_MKPT_HEADERS := GDE_proto.h

GDE_proto_h_MKPT_SRCS = $(patsubst ARB_GDE/%,%,$(arb_gde_SOURCES))
GDE_proto_h_MKPT_FLAGS := -P -E
