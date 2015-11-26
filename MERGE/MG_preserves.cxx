//  ==================================================================== //
//                                                                       //
//    File      : MG_preserves.cxx                                       //
//    Purpose   : find candidates for alignment preservation             //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in July 2003             //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include "merge.hxx"
#include "MG_adapt_ali.hxx"

#include <aw_awar.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <aw_select.hxx>
#include <arb_progress.h>
#include <arb_global_defs.h>
#include <arbdbt.h>

#include <set>
#include <string>

using namespace std;

// find species/SAIs to preserve alignment

#define AWAR_REMAP_CANDIDATE     AWAR_MERGE_TMP "remap_candidates"
#define AWAR_REMAP_ALIGNMENT     AWAR_MERGE_TMP "remap_alignment"
#define AWAR_REMAP_SEL_REFERENCE AWAR_MERGE_TMP "remap_reference"

struct preserve_para {
    AW_selection_list *alignmentList;
    AW_selection_list *refCandidatesList; // reference candidates
    AW_selection_list *usedRefsList;
};

static void get_global_alignments(ConstStrArray& ali_names) {
    // get all alignment names available in both databases
    GBT_get_alignment_names(ali_names, GLOBAL_gb_src);
    GBDATA *gb_presets = GBT_get_presets(GLOBAL_gb_dst);

    int i;
    for (i = 0; ali_names[i]; ++i) {
        GBDATA *gb_ali_name = GB_find_string(gb_presets, "alignment_name", ali_names[i], GB_IGNORE_CASE, SEARCH_GRANDCHILD);
        if (!gb_ali_name) ali_names.remove(i--);
    }
}

static void init_alignments(preserve_para *para) {
    // initialize the alignment selection list
    ConstStrArray ali_names;
    get_global_alignments(ali_names);
    para->alignmentList->init_from_array(ali_names, "All", "All");
}

static void clear_candidates(preserve_para *para) {
    // clear the candidate list
    AW_selection_list *candList = para->refCandidatesList;

    candList->clear();
    candList->insert_default(DISPLAY_NONE, NO_ALI_SELECTED);
    candList->update();
}

static long count_bases(const char *data, long len) {
    long count = 0;
    for (long i = 0; i<len; ++i) {
        if (data[i] != '-' && data[i] != '.') {
            ++count;
        }
    }
    return count;
}

// count real bases
// (gb_data should point to ali_xxx/data)
static long count_bases(GBDATA *gb_data) {
    long count = 0;
    switch (GB_read_type(gb_data)) {
        case GB_STRING:
            count = count_bases(GB_read_char_pntr(gb_data), GB_read_count(gb_data));
            break;
        case GB_BITS: {
            char *bitstring = GB_read_as_string(gb_data);
            long  len       = strlen(bitstring);
            count           = count_bases(bitstring, len);
            free(bitstring);
            break;
        }
        default:
            GB_export_errorf("Data type %s is not supported", GB_get_type_name(gb_data));
            break;
    }
    return count;
}

// -------------------------
//      class Candidate

class Candidate {
    string name;                // species/SAI name
    double score;
    int    found_alignments;    // counts alignments with data in both databases
    long   base_count_diff;

public:
    Candidate(bool is_species, const char *name_, GBDATA *gb_src, GBDATA *gb_dst, const CharPtrArray& ali_names)
        : name(is_species ? name_ : string("SAI:")+name_)
    {
        found_alignments = 0;
        score            = 0.0;
        base_count_diff  = 0;
        bool valid       = true;

        mg_assert(!GB_have_error());

        for (int i = 0; valid && ali_names[i]; ++i) {
            if (GBDATA *gb_src_data = GBT_find_sequence(gb_src, ali_names[i])) {
                if (GBDATA *gb_dst_data = GBT_find_sequence(gb_dst, ali_names[i])) {
                    ++found_alignments;

                    long src_bases  = count_bases(gb_src_data);
                    long dst_bases  = count_bases(gb_dst_data);
                    base_count_diff = labs(src_bases-dst_bases);

                    if (GB_have_error()) valid = false;

                    double factor;
                    if (base_count_diff>5)  factor = 0.2;
                    else                    factor = (6-base_count_diff)*(1.0/6);

                    long min_bases  = src_bases<dst_bases ? src_bases : dst_bases;
                    score          += min_bases*factor;
                }
            }
        }

        if (!valid) found_alignments = 0;
    }
    ~Candidate() {}

    int has_alignments() const { return found_alignments; }
    double get_score() const { return score; }

    const char *get_name() const { return name.c_str(); }
    const char *get_entry() const {
        return GBS_global_string("%24s  %i %li %f", get_name(), found_alignments, base_count_diff, score);
    }

    bool operator < (const Candidate& other) const { // sort highest score first
        int ali_diff = found_alignments-other.found_alignments;
        if (ali_diff) return ali_diff>0;
        return score > other.score;
    }
};

// use SmartPtr to memory-manage Candidate's
static bool operator < (const SmartPtr<Candidate>& c1, const SmartPtr<Candidate>& c2) {
    return c1->operator<(*c2);
}
typedef set< SmartPtr<Candidate> > Candidates;

static void find_species_candidates(Candidates& candidates, const CharPtrArray& ali_names) {
    // collect names of all species in source database
    GB_HASH      *src_species = GBT_create_species_hash(GLOBAL_gb_src);
    long          src_count   = GBS_hash_elements(src_species);
    arb_progress  progress("Examining species", src_count);
    bool          aborted     = false;

    // find existing species in destination database
    for (GBDATA *gb_dst_species = GBT_first_species(GLOBAL_gb_dst);
         gb_dst_species && !aborted;
         gb_dst_species = GBT_next_species(gb_dst_species))
    {
        const char *dst_name = GBT_read_name(gb_dst_species);

        if (GBDATA *gb_src_species = (GBDATA*)GBS_read_hash(src_species, dst_name)) {
            Candidate *cand = new Candidate(true, dst_name, gb_src_species, gb_dst_species, ali_names);

            if (cand->has_alignments() && cand->get_score()>0.0) {
                candidates.insert(cand);
            }
            else {
                if (GB_have_error()) {
                    aw_message(GBS_global_string("Invalid adaption candidate '%s' (%s)", dst_name, GB_await_error()));
                }
                delete cand;
            }

            progress.inc();
            aborted = progress.aborted();
        }
    }

    GBS_free_hash(src_species);
    progress.done();
}

static void find_SAI_candidates(Candidates& candidates, const CharPtrArray& ali_names) {
    // add all candidate SAIs to 'candidates'
    GB_HASH      *src_SAIs  = GBT_create_SAI_hash(GLOBAL_gb_src);
    long          src_count = GBS_hash_elements(src_SAIs);
    arb_progress  progress("Examining SAIs", src_count);

    // find existing SAIs in destination database
    for (GBDATA *gb_dst_SAI = GBT_first_SAI(GLOBAL_gb_dst);
         gb_dst_SAI;
         gb_dst_SAI = GBT_next_SAI(gb_dst_SAI))
    {
        const char *dst_name = GBT_read_name(gb_dst_SAI);

        if (GBDATA *gb_src_SAI = (GBDATA*)GBS_read_hash(src_SAIs, dst_name)) {
            Candidate *cand = new Candidate(false, dst_name, gb_src_SAI, gb_dst_SAI, ali_names);

            if (cand->has_alignments() && cand->get_score()>0.0) {
                candidates.insert(cand);
            }
            else {
                if (GB_have_error()) {
                    aw_message(GBS_global_string("Invalid adaption candidate 'SAI:%s' (%s)", dst_name, GB_await_error()));
                }
                delete cand;
            }

            progress.inc();
        }
    }

    GBS_free_hash(src_SAIs);
}

static void calculate_preserves_cb(AW_window *aww, preserve_para *para) {
    // FIND button (rebuild candidates list)

    GB_transaction ta_src(GLOBAL_gb_src);
    GB_transaction ta_dst(GLOBAL_gb_dst);

    clear_candidates(para);

    const char *ali = aww->get_root()->awar(AWAR_REMAP_ALIGNMENT)->read_char_pntr();
    Candidates  candidates;

    arb_progress("Searching candidates");

    // add candidates
    {
        ConstStrArray ali_names;
        if (0 == strcmp(ali, "All")) {
            get_global_alignments(ali_names);
        }
        else {
            ali_names.put(ali);
        }
        find_SAI_candidates(candidates, ali_names);
        find_species_candidates(candidates, ali_names);
    }

    int                   count       = 0;
    Candidates::iterator  e           = candidates.end();
    AW_selection_list    *refCandList = para->refCandidatesList;

    for (Candidates::iterator i = candidates.begin();
         i != e && count<5000;
         ++i, ++count)
    {
        string name  = (*i)->get_name();
        string shown = (*i)->get_entry();

        refCandList->insert(shown.c_str(), name.c_str());
    }

    refCandList->update();
}



static void read_references(ConstStrArray& refs, AW_root *aw_root)  {
    char *ref_string = aw_root->awar(AWAR_REMAP_SPECIES_LIST)->read_string();
    GBT_splitNdestroy_string(refs, ref_string, " \n,;", true);
}
static void write_references(AW_root *aw_root, const CharPtrArray& ref_array) {
    char *ref_string = GBT_join_strings(ref_array, '\n');
    aw_root->awar(AWAR_REMAP_SPECIES_LIST)->write_string(ref_string);
    aw_root->awar(AWAR_REMAP_ENABLE)->write_int(ref_string[0] != 0);
    free(ref_string);
}
static void select_reference(AW_root *aw_root, const char *ref_to_select) {
    aw_root->awar(AWAR_REMAP_SEL_REFERENCE)->write_string(ref_to_select);
}
static char *get_selected_reference(AW_root *aw_root) {
    return aw_root->awar(AWAR_REMAP_SEL_REFERENCE)->read_string();
}

static void refresh_reference_list_cb(AW_root *aw_root, preserve_para *para) {
    ConstStrArray  refs;
    read_references(refs, aw_root);
    para->usedRefsList->init_from_array(refs, "", "");
}

static void add_selected_cb(AW_window *aww, preserve_para *para) {
    // ADD button (add currently selected candidate to references)
    AW_root       *aw_root = aww->get_root();
    ConstStrArray  refs;
    read_references(refs, aw_root);

    char *candidate  = aw_root->awar(AWAR_REMAP_CANDIDATE)->read_string();
    char *selected   = get_selected_reference(aw_root);
    int   cand_index = refs.index_of(candidate);
    int   sel_index  = refs.index_of(selected);

    if (cand_index == -1) refs.put_before(sel_index+1, candidate);
    else                  refs.move(cand_index, sel_index);

    write_references(aw_root, refs);
    select_reference(aw_root, candidate);

    free(selected);
    free(candidate);

    para->refCandidatesList->move_selection(1); 
}

static void clear_references_cb(AW_window *aww) {
    // CLEAR button (clear references)
    aww->get_root()->awar(AWAR_REMAP_SPECIES_LIST)->write_string("");
}

static void del_reference_cb(AW_window *aww) {
    AW_root       *aw_root = aww->get_root();
    ConstStrArray  refs;
    read_references(refs, aw_root);

    char *selected  = get_selected_reference(aw_root);
    int   sel_index = refs.index_of(selected);

    if (sel_index >= 0) {
        select_reference(aw_root, refs[sel_index+1]);
        refs.safe_remove(sel_index);
        write_references(aw_root, refs);
    }

    free(selected);
}

static void lower_reference_cb(AW_window *aww) {
    AW_root       *aw_root = aww->get_root();
    ConstStrArray  refs;
    read_references(refs, aw_root);

    char *selected  = get_selected_reference(aw_root);
    int   sel_index = refs.index_of(selected);

    if (sel_index >= 0) {
        refs.move(sel_index, sel_index+1);
        write_references(aw_root, refs);
    }

    free(selected);
}
static void raise_reference_cb(AW_window *aww) {
    AW_root       *aw_root = aww->get_root();
    ConstStrArray  refs;
    read_references(refs, aw_root);

    char *selected  = get_selected_reference(aw_root);
    int   sel_index = refs.index_of(selected);

    if (sel_index > 0) {
        refs.move(sel_index, sel_index-1);
        write_references(aw_root, refs);
    }

    free(selected);
}

static void test_references_cb(AW_window *aww) {
    char           *reference_species_names = aww->get_root()->awar(AWAR_REMAP_SPECIES_LIST)->read_string();
    GB_transaction  tm(GLOBAL_gb_src);
    GB_transaction  td(GLOBAL_gb_dst);
    
    MG_remaps test_mapping(GLOBAL_gb_src, GLOBAL_gb_dst, true, reference_species_names); // will raise aw_message's in case of problems

    free(reference_species_names);
}

static void init_preserve_awars(AW_root *aw_root) {
    aw_root->awar_string(AWAR_REMAP_ALIGNMENT,     "", GLOBAL_gb_dst);
    aw_root->awar_string(AWAR_REMAP_CANDIDATE,     "", GLOBAL_gb_dst);
    aw_root->awar_string(AWAR_REMAP_SEL_REFERENCE, "", GLOBAL_gb_dst);
}

AW_window *MG_create_preserves_selection_window(AW_root *aw_root) {
    // SELECT PRESERVES window
    init_preserve_awars(aw_root);

    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, "SELECT_PRESERVES", "Select adaption candidates");
    aws->load_xfig("merge/preserves.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("mg_preserve.hlp"));
    aws->create_button("HELP", "HELP", "H");

    // ----------

    preserve_para *para = new preserve_para; // do not free (is passed to callback)

    aws->at("ali");
    para->alignmentList = aws->create_selection_list(AWAR_REMAP_ALIGNMENT, 10, 30, true);

    // ----------

    aws->at("adapt");
    aws->label("Adapt alignments");
    aws->create_toggle(AWAR_REMAP_ENABLE);

    aws->at("reference");
    para->usedRefsList = aws->create_selection_list(AWAR_REMAP_SEL_REFERENCE, 10, 30, true);

    aws->button_length(8);

    aws->at("clear");
    aws->callback(clear_references_cb);
    aws->create_button("CLEAR", "Clear", "C");

    aws->at("del");
    aws->callback(del_reference_cb);
    aws->create_button("DEL", "Del", "L");

    aws->at("up");
    aws->callback(raise_reference_cb);
    aws->create_button("UP", "Up", "U");

    aws->at("down");
    aws->callback(lower_reference_cb);
    aws->create_button("DOWN", "Down", "D");

    aws->at("test");
    aws->callback(test_references_cb);
    aws->create_button("TEST", "Test", "T");
    
    // ----------

    aws->at("find");
    aws->callback(makeWindowCallback(calculate_preserves_cb, para));
    aws->create_autosize_button("FIND", "Find candidates", "F", 1);

    aws->at("add");
    aws->callback(makeWindowCallback(add_selected_cb, para));
    aws->create_button("ADD", "Add", "A");

    aws->at("candidate");
    para->refCandidatesList = aws->create_selection_list(AWAR_REMAP_CANDIDATE, 10, 30, true);

    {
        GB_transaction ta_src(GLOBAL_gb_src);
        GB_transaction ta_dst(GLOBAL_gb_dst);

        init_alignments(para);
        clear_candidates(para);
    }

    {
        AW_awar *awar_list = aw_root->awar(AWAR_REMAP_SPECIES_LIST);
        awar_list->add_callback(makeRootCallback(refresh_reference_list_cb, para));
        awar_list->touch(); // trigger callback
    }

    return aws;
}




