module:=db
db_TARGETS := lib/libARBDB.so
db_HEADERS := ad_cb.h ad_cb_prot.h ad_config.h ad_p_prot.h ad_prot.h 
db_HEADERS += ad_remote.h ad_t_prot.h adGene.h adperl.h arbdb.h 
db_HEADERS += arbdb_base.h arbdbt.h dbitem_set.h
db_MKPT_HEADERS := gb_prot.h gb_t_prot.h ad_prot.h ad_cb_prot.h ad_p_prot.h ad_t_prot.h
db_LIBADD :=core	
db_LDFLAGS := $(ARB_GLIB_LIBS)

# db depends on:
# aisc_mkpt to build headers
# core (probably)
# cb for cb.h
# window for aw_awar_defs.h

# specify how generated headers are built:

# three sets of sources:
xdb_toolkit_SRCS := adChangeKey.cxx adRevCompl.cxx aditem.cxx adname.cxx 
xdb_toolkit_SRCS += adseqcompr.cxx adtables.cxx adtools.cxx adtree.cxx 
xdb_toolkit_SRCS += adali.cxx
xdb_other_SRCS := arbdbpp.cxx ad_config.cxx
xdb_normal_SRCS := $(call exclude-all,$(xdb_toolkit_SRCS) $(xdb_other_SRCS),$(notdir $(wildcard $(dir)*.cxx)))

# six interface header files:
gb_prot_h_MKPT_NAME :=Internal database interface
gb_prot_h_MKPT_REGEX:=^gb_,^gbs_,^gbcm,^gbm_,^gbl_
gb_prot_h_MKPT_SRCS := $(xdb_normal_SRCS)

gb_t_prot_h_MKPT_NAME :=Internal toolkit
gb_t_prot_h_MKPT_REGEX:=^gb_,^gbt_,^gbs_
gb_t_prot_h_MKPT_SRCS :=$(xdb_toolkit_SRCS)

ad_prot_h_MKPT_NAME :=ARB database interface
ad_prot_h_MKPT_REGEX:=^GB_,^GEN_,^EXP_,^GBS_,^GBT_,^GBCM,^GBC_ -S callback -P
ad_prot_h_MKPT_SRCS :=$(xdb_normal_SRCS)

ad_cb_prot_h_MKPT_NAME :=ARB database callback interface
ad_cb_prot_h_MKPT_REGEX:='^GB_+callback' 
ad_cb_prot_h_MKPT_SRCS :=$(xdb_normal_SRCS)

ad_p_prot_h_MKPT_NAME :=ARB perl interface
ad_p_prot_h_MKPT_REGEX:=^GBP_ 
ad_p_prot_h_MKPT_SRCS :=$(xdb_normal_SRCS)

ad_t_prot_h_MKPT_NAME :=ARB toolkit
ad_t_prot_h_MKPT_REGEX:=^GB_,^GEN_,^EXP_,^GBT
ad_t_prot_h_MKPT_SRCS :=$(xdb_toolkit_SRCS)
