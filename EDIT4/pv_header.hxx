#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_keysym.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <aw_awars.hxx>

#include <awt_seq_colors.hxx>
#include <awt_attributes.hxx>
#include <st_window.hxx>

#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_block.hxx"
#include "ed4_nds.hxx"
#include "ed4_defs.hxx"

#include <awt_codon_table.hxx>
#include <awt_changekey.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_pro_a_nucs.hxx>
#include <awt_translate.hxx>

#define AWAR_PROTVIEW                                               "protView/"
#define AWAR_PROTVIEW_FORWARD_TRANSLATION         AWAR_PROTVIEW "forward_translation" 
#define AWAR_PROTVIEW_FORWARD_TRANSLATION_1      AWAR_PROTVIEW "forward_translation_1" 
#define AWAR_PROTVIEW_FORWARD_TRANSLATION_2      AWAR_PROTVIEW "forward_translation_2" 
#define AWAR_PROTVIEW_FORWARD_TRANSLATION_3      AWAR_PROTVIEW "forward_translation_3" 
#define AWAR_PROTVIEW_REVERSE_TRANSLATION           AWAR_PROTVIEW "reverse_translation" 
#define AWAR_PROTVIEW_REVERSE_TRANSLATION_1        AWAR_PROTVIEW "reverse_translation_1" 
#define AWAR_PROTVIEW_REVERSE_TRANSLATION_2        AWAR_PROTVIEW "reverse_translation_2" 
#define AWAR_PROTVIEW_REVERSE_TRANSLATION_3        AWAR_PROTVIEW "reverse_translation_3" 
#define AWAR_PROTVIEW_DEFINED_FIELDS                      AWAR_PROTVIEW "defined_fields" 

