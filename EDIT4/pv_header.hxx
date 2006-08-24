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

// Definitions used
#define FORWARD_STRAND             1
#define COMPLEMENTARY_STRAND   2
#define DB_FIELD_STRAND             3

#define FORWARD_STRANDS            3
#define COMPLEMENTARY_STRANDS  3
#define DB_FIELD_STRANDS             1

#define START_POS_1  0
#define START_POS_2  1
#define START_POS_3  2

// typedefs
typedef enum {
    PV_FAILED = 0,
    PV_SUCCESS
} PV_ERROR;

