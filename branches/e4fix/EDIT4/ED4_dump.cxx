//  ==================================================================== //
//                                                                       //
//    File      : ED4_dump.cxx                                           //
//    Purpose   : Contains dump() methods of ED4-classes                 //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in May 2002              //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include "ed4_class.hxx"
#include <arbdb.h>

#ifdef IMPLEMENT_DUMP

#define INDENT_PER_LEVEL 2
#define NEXT_INDENT      (indent+INDENT_PER_LEVEL)
#define OUT              stdout

// ----------------------------------------------------------
//      static void printProperties(ED4_properties prop)
// ----------------------------------------------------------
static void printProperties(ED4_properties prop) {
    char sep = ' ';
#define pprop(tag) do { if (prop&ED4_P_##tag) { fputc(sep, OUT); sep = '|'; fputs(#tag, OUT); } } while(0)
    pprop(IS_MANAGER);
    pprop(IS_TERMINAL);
    pprop(HORIZONTAL);
    pprop(VERTICAL);
    pprop(TMP);
    pprop(SELECTABLE);
    pprop(DRAGABLE);
    pprop(MOVABLE);
    pprop(IS_HANDLE);
    pprop(CURSOR_ALLOWED);
    pprop(IS_FOLDED);
    pprop(CONSENSUS_RELEVANT);
    fputc(' ', OUT);
#undef pprop
}

// -------------------------------------------------
//      static void printLevel(ED4_level level)
// -------------------------------------------------
static void printLevel(ED4_level level) {
    char sep = ' ';
#define plev(tag) do { if (level&ED4_L_##tag) { fputc(sep, OUT); sep = '|'; fputs(#tag, OUT); } } while(0)
    plev(ROOT);
    plev(DEVICE);
    plev(AREA);
    plev(MULTI_SPECIES);
    plev(SPECIES);
    plev(MULTI_SEQUENCE);
    plev(SEQUENCE);
    plev(TREE);
    plev(SPECIES_NAME);
    plev(SEQUENCE_INFO);
    plev(SEQUENCE_STRING);
    plev(ORF);
    plev(SPACER);
    plev(LINE);
    plev(MULTI_NAME);
    plev(NAME_MANAGER);
    plev(GROUP);
    plev(BRACKET);
    plev(PURE_TEXT);
    plev(COL_STAT);
    fputc(' ', OUT);
#undef plev
}

static bool at_start_of_line = true;
inline void print_indent(size_t indent) {
    if (at_start_of_line) while (indent-->0) fputc(' ', OUT);
}
inline void print_indented_nocr(size_t indent, const char *text) {
    print_indent(indent);
    fputs(text, OUT);
    at_start_of_line = false;
}
inline void print_indented(size_t indent, const char *text) {
    print_indented_nocr(indent, text);
    fputc('\n', OUT);
    at_start_of_line = true;
}

inline void CR() { fputc('\n', OUT); }

inline void openDump(size_t indent, const char *className, void *thisPtr, bool allow_zero_this = false) {
    print_indented_nocr(indent, className);
    fprintf(OUT, " { ");
    if (thisPtr) {
        fprintf(OUT, "((%s *)%p)\n", className, thisPtr);
        at_start_of_line = true;
    }
    else {
        if (!allow_zero_this) {
            fputs("this=zero-pointer!\n", OUT);
            at_start_of_line = true;
        }
    }
}
inline void closeDump(size_t indent) {
    print_indented(indent, "}");
}


static void dumpProperties(size_t indent, const char *varname, ED4_properties prop) {
    openDump(indent, GBS_global_string("%s(ED4_properties)", varname), 0, true);
    print_indent(NEXT_INDENT);
    printProperties(prop);
    closeDump(indent);
}

static void dumpLevel(size_t indent, const char *varname, ED4_level level) {
    openDump(indent, GBS_global_string("%s(ED4_level)", varname), 0, true);
    print_indent(NEXT_INDENT);
    printLevel(level);
    closeDump(indent);
}

// =========================================================================================
// ED4_base

void ED4_base::dump_base(size_t indent) const {
    openDump(indent, "ED4_Base", (void*)this);
#if 0
    print_indented(NEXT_INDENT, GBS_global_string("my_species_pointer = %p", get_species_pointer()));
    print_indented(NEXT_INDENT, GBS_global_string("lastXpos           = %f", lastXpos));
    print_indented(NEXT_INDENT, GBS_global_string("lastYpos           = %f", lastYpos));
    print_indented(NEXT_INDENT, GBS_global_string("timestamp          = %i", timestamp));
    print_indented(NEXT_INDENT, GBS_global_string("parent             = %p", parent));
    print_indented(NEXT_INDENT, GBS_global_string("id                 = '%s'", id));
    print_indented(NEXT_INDENT, GBS_global_string("index              = %li", index));
    print_indented(NEXT_INDENT, GBS_global_string("width_link         = %p", width_link));
    print_indented(NEXT_INDENT, GBS_global_string("height_link        = %p", height_link));
    print_indented(NEXT_INDENT, GBS_global_string("flag.hidden        = %i", flag.hidden));
    print_indented(NEXT_INDENT, GBS_global_string("flag.is_consensus  = %i", flag.is_consensus));
    print_indented(NEXT_INDENT, GBS_global_string("flag.is_SAI        = %i", flag.is_SAI));

    if (spec) spec.dump(NEXT_INDENT);
    dumpProperties(NEXT_INDENT, "dynamic_prop", dynamic_prop);
    extension.dump(NEXT_INDENT);
    update_info.dump(NEXT_INDENT);
#else
    spec.dump(NEXT_INDENT);
    print_indented(NEXT_INDENT, GBS_global_string("id                 = '%s'", id));
    print_indented(NEXT_INDENT, GBS_global_string("parent             = %p", parent));
    print_indented(NEXT_INDENT, GBS_global_string("flag.is_consensus  = %i", flag.is_consensus));
    print_indented(NEXT_INDENT, GBS_global_string("flag.is_SAI        = %i", flag.is_SAI));
#endif

    closeDump(indent);
}

// =========================================================================================
// ED4_members

void ED4_members::dump(size_t indent) const {
    openDump(indent, "ED4_members", (void*)this);
    for (ED4_index i=0; i<members(); i++) {
        member(i)->dump(NEXT_INDENT);
    }
    closeDump(indent);
}

// =========================================================================================
// managers and terminals

void ED4_manager::dump_base(size_t indent) const {
    openDump(indent, "ED4_Manager", (void*)this);
    ED4_base::dump_base(NEXT_INDENT);
    children->dump(NEXT_INDENT);
    closeDump(indent);
}


#define STANDARD_BASE_DUMP_CODE(mytype) do {      \
        openDump(indent, #mytype, (void*)this); \
        mytype::dump_my_base(NEXT_INDENT);      \
        closeDump(indent);                      \
    } while (0)
    
#define STANDARD_LEAF_DUMP_CODE(mytype) do {           \
        openDump(indent, #mytype, (void*)this); \
        mytype::dump_my_base(NEXT_INDENT);        \
        closeDump(indent);                      \
    } while (0)


#define STANDARD_DUMP_BASE(self) void self::dump_base(size_t indent) const { STANDARD_BASE_DUMP_CODE(self); }
#define STANDARD_DUMP_LEAF(self) void self::dump(size_t indent) const { STANDARD_LEAF_DUMP_CODE(self); }

#define STANDARD_DUMP_MID(self) STANDARD_DUMP_BASE(self); STANDARD_DUMP_LEAF(self)

STANDARD_DUMP_BASE(ED4_abstract_group_manager);
STANDARD_DUMP_BASE(ED4_abstract_sequence_terminal);
STANDARD_DUMP_BASE(ED4_terminal);
STANDARD_DUMP_BASE(ED4_text_terminal);

STANDARD_DUMP_MID(ED4_sequence_terminal);

STANDARD_DUMP_LEAF(ED4_area_manager);
STANDARD_DUMP_LEAF(ED4_bracket_terminal);
STANDARD_DUMP_LEAF(ED4_columnStat_terminal);
STANDARD_DUMP_LEAF(ED4_consensus_sequence_terminal);
STANDARD_DUMP_LEAF(ED4_device_manager);
STANDARD_DUMP_LEAF(ED4_group_manager);
STANDARD_DUMP_LEAF(ED4_line_terminal);
STANDARD_DUMP_LEAF(ED4_main_manager);
STANDARD_DUMP_LEAF(ED4_multi_name_manager);
STANDARD_DUMP_LEAF(ED4_multi_sequence_manager);
STANDARD_DUMP_LEAF(ED4_multi_species_manager);
STANDARD_DUMP_LEAF(ED4_name_manager);
STANDARD_DUMP_LEAF(ED4_orf_terminal);
STANDARD_DUMP_LEAF(ED4_pure_text_terminal);
STANDARD_DUMP_LEAF(ED4_root_group_manager);
STANDARD_DUMP_LEAF(ED4_sequence_info_terminal);
STANDARD_DUMP_LEAF(ED4_sequence_manager);
STANDARD_DUMP_LEAF(ED4_spacer_terminal);
STANDARD_DUMP_LEAF(ED4_species_manager);
STANDARD_DUMP_LEAF(ED4_species_name_terminal);
STANDARD_DUMP_LEAF(ED4_tree_terminal);

// =========================================================================================
// member structures

void ED4_objspec::dump(size_t indent) const {
    openDump(indent, "ED4_objspec", (void*)this);
    dumpProperties(NEXT_INDENT, "static_prop", static_prop);
    dumpLevel(NEXT_INDENT, "level", level);
    dumpLevel(NEXT_INDENT, "allowed_children", allowed_children);
    dumpLevel(NEXT_INDENT, "handled_level", handled_level);
    dumpLevel(NEXT_INDENT, "restriction_level", restriction_level);
    print_indented(NEXT_INDENT, GBS_global_string("justification       =%f", justification));
    closeDump(indent);
}

void ED4_extension::dump(size_t indent) const {
    openDump(indent, "ED4_extension", (void*)this);
    print_indented(NEXT_INDENT, GBS_global_string("position[0] = %f (x)", position[0]));
    print_indented(NEXT_INDENT, GBS_global_string("position[1] = %f (y)", position[1]));
    print_indented(NEXT_INDENT, GBS_global_string("size[0]     = %f (x)", size[0]));
    print_indented(NEXT_INDENT, GBS_global_string("size[1]     = %f (y)", size[1]));
    print_indented(NEXT_INDENT, GBS_global_string("y_folded    = %li", y_folded));
    closeDump(indent);
}
void ED4_update_info::dump(size_t indent) const {
    openDump(indent, "ED4_update_info", (void*)this);
    print_indented(NEXT_INDENT, GBS_global_string("resize                         = %u", resize));
    print_indented(NEXT_INDENT, GBS_global_string("refresh                        = %u", refresh));
    print_indented(NEXT_INDENT, GBS_global_string("clear_at_refresh               = %u", clear_at_refresh));
    print_indented(NEXT_INDENT, GBS_global_string("linked_to_folding_line         = %u", linked_to_folding_line));
    print_indented(NEXT_INDENT, GBS_global_string("linked_to_scrolled_rectangle   = %u", linked_to_scrolled_rectangle));
    print_indented(NEXT_INDENT, GBS_global_string("refresh_horizontal_scrolling   = %u", refresh_horizontal_scrolling));
    print_indented(NEXT_INDENT, GBS_global_string("delete_requested               = %u", delete_requested));
    closeDump(indent);
}

#endif



