module:=aisc_mkptps
aisc_mkptps_TARGETS  := $(AISC_MKPT)
# aisc_mkpt is special: it is needed during dependency generation,
# it must therefore not require any copies to INCLUDE and instead
# pull from TEMPLATES, CORE and UNIT_TESTER directly
aisc_mkptps_CPPFLAGS := -DSIMPLE_ARB_ASSERT -ITEMPLATES -ICORE -IUNIT_TESTER
aisc_mkptps_LDADD    := $(use_ARB_main)

# unset HAS_MAIN in case we are built as dependencz from something
# that has HAS_MAIN set (like aisc, e.g.)
$(AISC_MKPT): HAS_MAIN=
