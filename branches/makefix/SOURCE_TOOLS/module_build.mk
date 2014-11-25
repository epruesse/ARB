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


######## Helper Functions ########

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


## picks out all .o files and adds arb_main*.o unless HAS_MAIN=1
define makelinkdeps
$(filter %.o,\
 $(filter-out $(if $(HAS_MAIN),$(ARB_MAIN_CXX)),$(1))\
)
endef


######## intialize variables ########

SOURCE_DIR:=$(shell pwd)
LDFLAGS := -Llib 
CPPFLAGS := -IINCLUDE 

# create RELRO segment header (hardening)
LDFLAGS +=-Wl,-z,relro

# collect files to clean / distclean
CLEAN :=
DISTCLEAN :=
REALCLEAN :=

PHONY :=         # phony targets

modules :=       # available modules
module-files := $(shell find . -name module.mk)

static_libs :=   # static libs to build
dynamic_libs :=  # dynamic libs to build
programs :=      # programs to build
objects :=       # object files to build
mkpt_headers :=  # mkpt generated header files to build
gen_headers :=   # other (aisc) generated header files
headers :=       # headers to copy to INCLUDE


### programs
ARB_SED   := SH/arb_sed
AISC_MKPT := AISC_MKPTPS/aisc_mkpt
AISC      := AISC/aisc

### aisc rules

include SOURCE_TOOLS/module_aisc.mk

### load module definitions

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
dir := UNDEF
module:= UNDEF


define make_source_rules
# default is *.c *.cxx, empty indicated by "NONE"
  ifeq ($$($(1)_SOURCES),)
    $(1)_SOURCES:=$$(wildcard $$($(1)_DIR)*.cxx) $$(wildcard $$($(1)_DIR)*.c)
  endif
  ifeq ($$($(1)_SOURCES),NONE)
    $(1)_SOURCES:=
  endif
endef
$(foreach mod,$(modules),$(eval $(call make_source_rules,$(mod))))


# AISC rules in extra file (long)
$(foreach mod,$(modules),$(eval $(call make_aisc_rules,$(mod))))


define make_object_rules
# default is c(xx)->o
  ifeq ($$($(1)_OBJECTS),)
    tmp:=$$($(1)_SOURCES)
    tmp:=$$(tmp:%.c=%.o)
    tmp:=$$(tmp:%.cxx=%.o)
    $(1)_OBJECTS:=$$(tmp)
    $$($(1)_OBJECTS): LOCAL_DEFINES:=$$($(1)_CPPFLAGS) -I./$$($(1)_DIR)
  endif
  objects+=$$($(1)_OBJECTS)

# add local CPPFLAGS to .d rules
  $$(patsubst %.o,%.d,$$($(1)_OBJECTS)): LOCAL_DEFINES:=$$($(1)_CPPFLAGS) -I./$$($(1)_DIR)
  $$(patsubst %.o,%.d,$$($(1)_OBJECTS)): MODULE:=$(1)
endef
$(foreach mod,$(modules),$(eval $(call make_object_rules,$(mod))))


define make_header_rules
# create dependencies: INCLUDE/xxx.h: DIR/xxx.h
  tmp :=
  $(foreach header,$($(1)_HEADERS),
    INCLUDE/$(notdir $(header)): $($(1)_DIR)$(header)
    tmp+=INCLUDE/$(notdir $(header))
  )
  $(1)_HEADERS:=$$(tmp)

  headers += $$(tmp)
  DISTCLEAN+=$$(tmp)

# .h.mkpt files for generated headers depend on sources and module.mk
  $(foreach header,$($(1)_MKPT_HEADERS),
    module:=$(1)
    $($(1)_DIR)$(header).mkpt: $$(patsubst %,$($(1)_DIR)%,$$($(subst /,__,$(subst .,_,$(header)))_MKPT_SRCS))
    $($(1)_DIR)$(header).mkpt: MKPT_MOD=$(subst /,__,$(subst .,_,$(header)))_MKPT
    $($(1)_DIR)$(header).mkpt: $($(1)_DIR)module.mk
    $($(1)_DIR)$(header).mkpt: module=$(1)
  )
  mkpt_headers += $($(1)_MKPT_HEADERS:%=$$($(1)_DIR)%)
  $($(1)_OBJECTS): $($(1)_MKPT_HEADERS:%=$$($(1)_DIR)%)
endef 
$(foreach mod,$(modules),$(eval $(call make_header_rules,$(mod))))

# remainder...
define make_module_rules
  programs     += $$(filter-out %.so %.a, $$($(1)_TARGETS))
  dynamic_libs += $$(filter %.so, $$($(1)_TARGETS))
  static_libs  += $$(filter %.a, $$($(1)_TARGETS))

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

# create module clean target (refuse ../xxx and /xxx as safeguard)
  $(1)_clean:
	-rm -rf $$(filter-out ..% /%,$$($(1)_CLEAN))
# create module build target
  $(1): $$($(1)_TARGETS) $$($(1)_HEADERS)

  PHONY+=$(1) $(1)_clean
  CLEAN+=$$($(1)_CLEAN)
endef # make_module_rules
$(foreach mod,$(modules),$(eval $(call make_module_rules,$(mod))))



## load dependencies (unless clean target called)
ifneq ($(filter-out %clean, $(MAKECMDGOALS)),)
  -include $(filter-out SOURCE_TOOLS%,$(objects:%.o=%.d))
endif


### build rules

CP_CMD = $(call prettyprint, cp $< $@,CP)

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
HEADERS=`echo $(headers:INCLUDE/%=%)|	$(ARB_SED) -e 's/ /\\\\|/g; s/|$$//;'`;\
$(A_CXX) $(cflags) $(cxxflags) $(CPPFLAGS) $(LOCAL_DEFINES) -c $<  $(CXX_INCLUDES) \
         -M -MG -MF $@.tmp -MT $(@:%.d=%.o) -MT $@;\
$(ARB_SED) -i -e 's/$(subst /,\/,$(SOURCE_DIR))\///g' $@.tmp;\
$(ARB_SED) -i -e 's|/usr[^ ]*||g' $@.tmp;\
$(ARB_SED) -e '$(foreach header,$($(MODULE)_AISC_HEADERS) $($(MODULE)_MKPT_HEADERS),\
s| $(notdir $(header))| $($(MODULE)_DIR)$(header)|;)' \
           -e 's/ \('$$HEADERS'\)/ INCLUDE\/\1/g' $@.tmp > $@;\
rm $@.tmp\
,DEPEND)
endef

# for performance reasons, copy all simple headers to INCLUDE before
# doing the dependencies
simple_headers = $(filter-out $(patsubst %,INCLUDE/%,$(gen_headers) \
                   $(notdir $(mkpt_headers))),$(headers))

%.d: %.cxx | $(simple_headers)
	+$(DEPEND_CMD)
%.d: %.c | $(simple_headers)
	+$(DEPEND_CMD)


## static libs

define AR_CMD
$(call prettyprint,\
ar rcs $@ $(filter %.o,$^),\
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
	$(CP_CMD)

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
(cd $($(module)_DIR);\
$(SOURCE_DIR)/$(AISC_MKPT) $(if $($(MKPT_MOD)_NAME),-c '$($(MKPT_MOD)_NAME)') \
		-G -w $(@F:%.mkpt=%) \
                $(if $($(MKPT_MOD)_REGEX),-F $($(MKPT_MOD)_REGEX)) \
		$($(MKPT_MOD)_FLAGS) \
		$($(MKPT_MOD)_SRCS) > $(@:$($(module)_DIR)%=%);\
if cmp -s $(@:$($(module)_DIR)%=%) $(@:$($(module)_DIR)%.mkpt=%); then :; else \
   cp $(@:$($(module)_DIR)%=%) $(@:$($(module)_DIR)%.mkpt=%); \
   echo 'UPDATE  ' $(@:$($(module)_DIR)%.mkpt=%);\
fi\
), MKPT)
endef

$(mkpt_headers:%.h=%.h.mkpt): $(AISC_MKPT)
$(mkpt_headers:%.h=%.h.mkpt):
	$(MKPT_CMD)


mkpt: $(mkpt_headers)
mkpt_clean:
	-rm -f $(mkpt_headers) $(mkpt_headers:%.h=%.h.mkpt)
PHONTY+=mkpt_clean mkpt
DISTCLEAN+=$(mkpt_headers:%.h=%.h.mkpt)
REALCLEAN+=$(mkpt_headers) 

# clean rules 
realclean: distclean
	-rm -rf $(filter-out ..% /%,$(REALCLEAN))
distclean: xclean
	-rm -rf $(filter-out ..% /%,$(DISTCLEAN))
	-rm -rf $(objects:%.o=%.d)
xclean:
	-rm -rf $(filter-out ..% /%,$(CLEAN))
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

# keep all intermediate file (no deleting please!)
.SECONDARY:

# remove predefined suffix rules etc.
# (like "AISC/aisc from AISC/aisc.c", which we really don't want)
.SUFFIXES:
MAKEFLAGS += -rR
