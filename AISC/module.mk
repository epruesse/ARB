module:=aisc

aisc_TARGETS  := $(AISC)
# aisc is special: it is needed during dependency generation,
# it must therefore not require any copies to INCLUDE and instead
# pull from TEMPLATES, CORE and UNIT_TESTER directly
aisc_CPPFLAGS := -DSIMPLE_ARB_ASSERT -ITEMPLATES -ICORE -IUNIT_TESTER
aisc_MKPT_HEADERS := aisc_proto.h
aisc_proto_h_MKPT_FLAGS := -P
aisc_proto_h_MKPT_SRCS = $(notdir $(aisc_SOURCES))

# don't add the ARB standard main wrapper to aisc:
$(AISC): HAS_MAIN=1


