# This makefile contains the rules to build ARB's components based on configurations
# specified in the module.mk files in the various directories. Each module.mk contains
# the line 
#    module := <name>
# specifying the modules name. This name is a prefix for the variables specifying what
# needs to be built from what in the respective module:
#
# <name>_TARGETS := <files>
#   The artifacts of the module. These can be static libs ending in .a, dynamic libs
#   ending in .so. All other files are treated as binaries. Paths are relative to the 
#   source directory. $(dir) is provided as the module's directory:
#      module_TARGETS := $(dir)AWT.a
#
# <name>_SOURCES := <files>
#   These are the c/cxx source files. If unset, it will be filled with all c/cxx files in 
#   the respective directory. "NONE" is reserved to indicate that no c/cxx sources exist.
# 
# <name>_OBJECTS := <files>
#   Optional. Will be generated from <name>_SOURCES if empty.
#
# <name>_HEADERS := <files>
#   Contains the names of the public header files that need to be copied to INCLUDE/

# The makefiles depend on the .d (dependency) files, which depend on the header files. This 
# is why all targets will first generate all headers. 




### Helper Functions

## Pretty printing
ARB_BUILD_VERBOSE :=
ifeq ("$(origin V)", "command line") # V set on commandline
  ARB_BUILD_VERBOSE := $(V)
endif
# executes $1 but shows only "$2 $@" unless V=1 passed on make command line
define prettyprint
@if [ -n "$(ARB_BUILD_VERBOSE)" ]; then \
   echo "$(1)"; $(1); \
else \
   printf "%-9s%s\n" $(2) $@;\
   $(1) || (echo "$(1)"; false)	\
fi
endef

## removes all words in $1 from $2
define exclude-all
$(if $(strip $(1)),\
 $(call exclude-all,\
  $(filter-out $(firstword $(1)),$(1)),\
  $(filter-out $(firstword $(1)),$(2)),\
 ),\
 $(2)\
)
endef

## picks out all .o files and adds arb_main*.o unless HAS_MAIN=1
define makelinkdeps
$(filter %.o,\
 $(call exclude-all,$(if $(HAS_MAIN),$(ARB_MAIN_CXX)),$(1))\
)
endef


### intialize variables

SOURCE_DIR:=$(shell pwd)
LDFLAGS := -Llib 
CPPFLAGS := -IINCLUDE 
#-ITEMPLATES

# make linking fail on unresolved symbols (even for libs)
#LDFLAGS +=-Wl,-z,defs
# create RELRO segment header (hardening)
LDFLAGS +=-Wl,-z,relro

# collect files to clean / distclean
CLEAN :=
DISTCLEAN :=
REALCLEAN :=

# phony targets
PHONY :=

# module types
modules :=
static_libs :=
dynamic_libs :=
programs :=
objects :=
mkpt_headers :=
headers :=
aisc_targets :=
module-files := $(shell find . -name module.mk)

### programs
ARB_SED = SH/arb_sed



## load module definitions

# modules are makefiles that are included and parsed below
# they support the following special variables:
# dir (in): the directory relative to source-dir where the module resides
# modname: name of the module
# modarch: name of the archive created from the module
# 

# load module.mk files
define include_with_dir
  dir     := $(patsubst ./%,%,$(dir $(1)))
  module  :=
  include $(1)
  ifneq (,$$(module))
    modules+=$$(module)
    $$(module)_DIR:=$$(dir)
  endif
endef
$(foreach mod,$(module-files),$(eval $(call include_with_dir,$(mod))))

define make_header_rules
# create dependencies: INCLUDE/xxx.h: DIR/xxx.h
  $(foreach header,$($(1)_HEADERS:%=INCLUDE/%),
    $(header): $(header:INCLUDE/%=$($(1)_DIR)%)
  )
# prepend INCUDE/ to header files
  $(1)_HEADERS=$($(1)_HEADERS:%=INCLUDE/%)

  headers += $$($(1)_HEADERS)
  DISTCLEAN+=$$($(1)_HEADERS)
endef 
$(foreach mod,$(modules),$(eval $(call make_header_rules,$(mod))))

define make_module_rules
## module_MKPT_HEADERS
# .h.mkpt files for generated headers depend on sources and module.mk
  $(foreach header,$($(1)_MKPT_HEADERS),
    $$($(1)_DIR)$(header).mkpt: $$(patsubst %,$($(1)_DIR)%,$$($$(subst .,_,$(header))_MKPT_SRCS))
    $$($(1)_DIR)$(header).mkpt: $($(1)_DIR)module.mk
  )

# collect generated headers
  mkpt_headers += $($(1)_MKPT_HEADERS:%=$$($(1)_DIR)%)

## module_AISC
  ifneq (,$$($(1)_AISC))
    $($(1)_DIR)DUMP:
	mkdir $$@
    $(1)_CLEAN += $($(1)_DIR)DUMP
    aisc_targets += $($(1)_DIR)GENH/aisc.h

    $(1)_CLEAN +=  $($(1)_DIR)GENH/aisc.h

    $($(1)_DIR)GENH/aisc.h: $($(1)_DIR)$($(1)_AISC) | $($(1)_DIR)DUMP
    $($(1)_DIR)GENH/aisc.h: module := $(1)
    $($(1)_DIR)GENH/aisc_com.h: $($(1)_DIR)$($(1)_AISC) | $($(1)_DIR)DUMP
    $($(1)_DIR)GENH/aisc_com.h: module := $(1)
    $($(1)_DIR)GENH/global.aisc: module := $(1)
    $($(1)_DIR)PT_extern.mkpt: module := $(1)


    $($(1)_DIR)GENC/aisc_server.c: $($(1)_DIR)$($(1)_AISC) $($(1)_DIR)GENH/global.aisc | $($(1)_DIR)DUMP
    $($(1)_DIR)GENC/aisc_server.c: module := $(1)



  endif

## module_SOURCES
# default is *.c *.cxx, empty indicated by "NONE"
  ifeq ($$($(1)_SOURCES),)
    $(1)_SOURCES:=$$(wildcard $$($(1)_DIR)*.cxx) $$(wildcard $$($(1)_DIR)*.c)
  endif
  ifeq ($$($(1)_SOURCES),NONE)
    $(1)_SOURCES:=
  endif

## module_OBJECTS 
# default is c(xx)->o
  ifeq ($$($(1)_OBJECTS),)
    tmp:=$$($(1)_SOURCES)
    tmp:=$$(tmp:%.c=%.o)
    tmp:=$$(tmp:%.cxx=%.o)
    $(1)_OBJECTS:=$$(tmp)
    $$($(1)_OBJECTS): LOCAL_DEFINES:=$$($(1)_CPPFLAGS) -I./$$($(1)_DIR)
    $$($(1)_OBJECTS): $($(1)_DIR)module.mk
  endif
  objects+=$$($(1)_OBJECTS)

# add local CPPFLAGS to .d rules
  $$(patsubst %.o,%.d,$$($(1)_OBJECTS)): LOCAL_DEFINES:=$$($(1)_CPPFLAGS) -I./$$($(1)_DIR)
  $$(patsubst %.o,%.d,$$($(1)_OBJECTS)): MODULE:=$(1)

## module_TARGETS
  programs += $$(filter-out %.so, $$(filter-out %.a,$$($(1)_TARGETS)))
  dynamic_libs +=$$(filter %.so, $$($(1)_TARGETS))
  static_libs +=$$(filter %.a, $$($(1)_TARGETS))

# add objects to targets' dependency list
  $$($(1)_TARGETS): $$($(1)_OBJECTS)
  $$($(1)_TARGETS): $($(1)_DIR)module.mk
  $$($(1)_TARGETS): MODULE:=$(1)
# add depends to targets' dependency list
  $(foreach mod,$($(1)_LIBADD),
    $(foreach tgt,$($(mod)_TARGETS),
      ifeq (,$(patsubst %.so,,$(tgt))) 
        $($(1)_TARGETS): | $(tgt)
      else
        $($(1)_TARGETS): $(tgt)
      endif
    )
  )

  $(1)_LDFLAGS = \
    $(foreach mod,$($(1)_LIBADD),$$(patsubst lib/lib%.so,-l%,$$($(mod)_TARGETS)) $$($(mod)_LDFLAGS)) \
    $($(1)_LDFLAGS)

  $(1)_CLEAN+=$$($(1)_OBJECTS) $$($(1)_TARGETS)

# create module clean target
  $(1)_clean:
	-rm -rf $$($(1)_CLEAN)
# create module build target
  $(1): $$($(1)_TARGETS)

  PHONY+=$(1) $(1)_clean
  CLEAN+=$$($(1)_CLEAN)
endef # make_module_rules
$(foreach mod,$(modules),$(eval $(call make_module_rules,$(mod))))


#$(foreach mod,$(modules),$(info $(call make_module_rules,$(mod))))
#$(info $(call make_module_rules,wetc))

## load dependencies (unless clean target called)
ifneq ($(filter-out %clean, $(MAKECMDGOALS)),)
  -include $(filter-out SOURCE_TOOLS%,$(objects:%.o=%.d))
endif


### build rules

## object files
define CXX_CMD
$(call prettyprint,\
$(A_CXX) $(cflags) $(cxxflags) $(CPPFLAGS) $(LOCAL_DEFINES) -o $@ -c $<  $(CXX_INCLUDES) $(POST_COMPILE),\
CXX)
endef

%.o: %.cxx %.d
	$(CXX_CMD)
%.o: %.c %.d
	$(CXX_CMD)

## depend files


# - use GCC to generate dependencies
#   -MG tells it to output also missing dependencies instead of failing
# - remove path to current source dir where present
# - replace names without path (not found) from
#    $(headers) the mkpt headers of the current module
define DEPEND_CMD
$(call prettyprint,\
$(A_CXX) $(cflags) $(cxxflags) $(CPPFLAGS) $(LOCAL_DEFINES) -o $@ -c $<  $(CXX_INCLUDES) \
 -M -MG -MF $@ -MT $(@:%.d=%.o) -MT $@;\
HEADERS=`echo $(headers:INCLUDE/%=%)|	$(ARB_SED) -e 's/ /\\\\|/g; s/|$$//;'`;\
$(ARB_SED) -i -e 's/$(subst /,\/,$(SOURCE_DIR))\///g' \
           -e '$($(MODULE)_MKPT_HEADERS:%=s| \(%\)| $($(MODULE)_DIR:%/=%)\/\1 |;)' \
           -e 's/ \('$$HEADERS'\)/ INCLUDE\/\1/g' $@\
,DEPEND)
endef

%.d: %.cxx 
	$(DEPEND_CMD)
%.d: %.c 
	$(DEPEND_CMD)

## static libs

define AR_CMD
$(call prettyprint,\
ar rcs $@ $^,\
AR)
endef

$(static_libs):
	$(AR_CMD)

## dynamic libs

define LINKSO_CMD
$(call prettyprint,\
$(A_CXX) $(clflags) -shared $(GCOVFLAGS) -o $@ $(call makelinkdeps,$^) $(LDFLAGS),\
LINK_DLL)
endef

$(dynamic_libs):
	$(LINKSO_CMD)

## programs

define LINKEXEC_CMD
$(call prettyprint,\
$(A_CXX) $(clflags)  -o $@ $(call makelinkdeps,$^) $(LDFLAGS) \
 $($(MODULE)_LDFLAGS),\
LINK)
endef

$(programs): $(ARB_MAIN_CXX)
$(programs):
	$(LINKEXEC_CMD)

## headers

$(headers):
	$(call prettyprint,cp $^ $@,"CP")

## mkpt witchery 

# we want
# - the .h files generated if they don't exist
# - relevant(!) changes in the cxx to propagate
# - no update if those changes don't have effect
# to do this, we use a order-only dependency on .h.mkpt files
# - .h files must have existing .h.mkpt file
# - if .h file missing, it's copied from the .h.mkpt
# - when .h.mkpt is updated from cxx, .h is updated if different
# - that needs a separate make call currently... :/
# - WARNING: changing the .mkpt doesn't do anything!

%.h: |%.h.mkpt
	$(call prettyprint, cp $(@:%=%.mkpt) $@,CP)

define MKPT_CMD
$(call prettyprint,\
(cd $(@D);\
$(SOURCE_DIR)/$(AISC_MKPT) $(if $($(MKPT_MOD)_NAME),-c '$($(MKPT_MOD)_NAME)') \
		-G -w $(@F:%.mkpt=%) \
                $(if $($(MKPT_MOD)_REGEX),-F $($(MKPT_MOD)_REGEX)) \
		$($(MKPT_MOD)_FLAGS) \
		$($(MKPT_MOD)_SRCS) > $(@F);\
if cmp $(@F) $(@F:%.mkpt=%); then :; else \
   cp $(@F) $(@F:%.mkpt=%); \
   echo UPDATE $(@F:%.mkpt=%);\
fi\
), MKPT)
endef

$(mkpt_headers:%.h=%.h.mkpt): MKPT_MOD=$(subst .,_,$(@F:%.mkpt=%))_MKPT
$(mkpt_headers:%.h=%.h.mkpt): $(AISC_MKPT)
$(mkpt_headers:%.h=%.h.mkpt):
	$(MKPT_CMD)


mkpt: $(mkpt_headers)
mkpt_clean:
	-rm -f $(mkpt_headers) $(mkpt_headers:%.h=%.h.mkpt)
PHONTY+=mkpt_clean mkpt
DISTCLEAN+=$(mkpt_headers:%.h=%.h.mkpt)
REALCLEAN+=$(mkpt_headers) 

## aisc stuff

define AISC_CMD
endef


# clean rules 
realclean: distclean
	-rm -f $(REALCLEAN)
distclean: xclean
	-rm -f $(DISTCLEAN)
	-rm -f $(objects:%.o=%.d)
xclean:
	-rm -f $(CLEAN)
PHONY+=distclean xclean reallyclean

# help rules
list:
	@echo "module targets:" $(modules)
	@echo "CLEAN:" $(CLEAN)
	@echo "programs:" $(programs)
	@echo "headers:" $(headers)

xall: $(modules)

# special targets:
.PHONY: $(PHONY)
.SUFFIXES: .o .d .c .cxx .so .a .aisc

