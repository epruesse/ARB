
SUBS = \
		CLIENT/CLIENT.dummy \

# --------------------------------------------------------------------------------

$(MAIN): $(SUBS)

depends:
		@echo "removing auto-generated dependency files"
		rm CLIENT/.depends

%.dummy:
	@$(MAKE) -C $(@D) -r \
		"ACC = $(ACC)" \
		"AINCLUDES = $(AINCLUDES)" \
		"ARB  = yes" \
		"ARBHOME = $(ARBHOME)" \
		"CCPLIB = $(CCPLIB)" \
		"CPP = $(CPP)" \
		"CPPINCLUDES = $(CPPINCLUDES)" \
		"LD_LIBRARY_PATH  = $(LD_LIBRARY_PATH)" \
		"LIBPATH = $(LIBPATH)" \
		"LINK_STATIC_LIB = $(LINK_STATIC_LIB)" \
		"MAIN = $(@F:.dummy=.a)" \
		"MAKE = $(MAKE)" \
		"SHARED_LIB_SUFFIX = $(SHARED_LIB_SUFFIX)" \
		"SYSLIBS = $(SYSLIBS)" \
		"XHOME = $(XHOME)" \
		"cflags = $(cflags) -DIN_ARB_$(@D:/=)" \


# DO NOT DELETE
