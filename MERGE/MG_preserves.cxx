//  ==================================================================== //
//                                                                       //
//    File      : MG_preserves.cxx                                       //
//    Purpose   : find candidates for alignment preservation             //
//    Time-stamp: <Fri Jul/06/2007 12:36 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in July 2003             //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include "merge.hxx"

#include <set>
#include <string>
#include <smartptr.h>
#include <cassert>

using namespace std;

// find species/SAIs to preserve alignment

#define AWAR_REMAP_CANDIDATE    "tmp/merge/remap_candidates"
#define AWAR_REMAP_ALIGNMENT    "tmp/merge/remap_alignment"

struct preserve_para {
    AW_window         *window;
    AW_selection_list *ali_id;
    AW_selection_list *cand_id;
};

// get all alignment names available in both databases
static char **get_global_alignments() {
    char   **src_ali_names = GBT_get_alignment_names(gb_merge);
    GBDATA  *gb_presets    = GB_search(gb_dest, "presets", GB_CREATE_CONTAINER);
    int      i;

    for (i = 0; src_ali_names[i]; ++i) {
        GBDATA *gb_ali_name = GB_find(gb_presets, "alignment_name", src_ali_names[i], down_2_level);
        if (!gb_ali_name) {
            free(src_ali_names[i]);
            src_ali_names[i] = 0;
        }
    }

    int k = 0;
    for (int j = 0; j<i; ++j) {
        if (src_ali_names[j]) {
            if (j != k) {
                src_ali_names[k] = src_ali_names[j];
                src_ali_names[j] = 0;
            }
            ++k;
        }
    }

    return src_ali_names;
}

// initialize the alignment selection list
static void init_alignments(preserve_para *para) {
    AW_window         *aww = para->window;
    AW_selection_list *id  = para->ali_id;

    aww->clear_selection_list(id);
    aww->insert_default_selection(id, "All", "All");

    // insert alignments available in both databases
    char **ali_names = get_global_alignments();
    for (int i = 0; ali_names[i]; ++i) {
        aww->insert_selection(id, ali_names[i], ali_names[i]);
    }
    GBT_free_names(ali_names);

    aww->update_selection_list(id);
    aww->get_root()->awar(AWAR_REMAP_ALIGNMENT)->write_string("All"); // select "All"
}

// clear the candidate list
static void clear_candidates(preserve_para *para) {
    AW_window         *aww = para->window;
    AW_selection_list *id  = para->cand_id;

    aww->clear_selection_list(id);
    aww->insert_default_selection(id, "????", "????");
    aww->update_selection_list(id);

    aww->get_root()->awar(AWAR_REMAP_CANDIDATE)->write_string(""); // deselect
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
        case GB_BITS:  {
            char *bitstring = GB_read_as_string(gb_data);
            long  len       = strlen(bitstring);
            count           = count_bases(bitstring, len);
            free(bitstring);
            break;
        }
        default:
            GB_export_error("Data type %s is not supported", GB_get_type_name(gb_data));
            break;
    }
    return count;
}

// -------------------------
//      class Candidate
// -------------------------

class Candidate {
    string name;                // species/SAI name
    double score;
    int    found_alignments;    // counts alignments with data in both databases
    long   base_count_diff;

public:
    Candidate(bool is_species, const char *name_, GBDATA *gb_src, GBDATA *gb_dst, const char **ali_names)
        : name(is_species ? name_ : string("SAI:")+name_)
    {
        found_alignments = 0;
        score            = 0.0;
        base_count_diff  = 0;
        bool valid       = true;

        assert(!GB_get_error());

        for (int i = 0; valid && ali_names[i]; ++i) {
            if (GBDATA *gb_src_data = GBT_read_sequence(gb_src, ali_names[i])) {
                if (GBDATA *gb_dst_data = GBT_read_sequence(gb_dst, ali_names[i])) {
                    ++found_alignments;

                    long src_bases  = count_bases(gb_src_data);
                    long dst_bases  = count_bases(gb_dst_data);
                    base_count_diff = abs(src_bases-dst_bases);

                    if (GB_get_error()) valid = false;

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
        const char *name  = get_name();
        bool        isSAI = memcmp(name, "SAI:", 4) == 0;

        return GBS_global_string(isSAI ? "%-24s %i %li %f" : "%-8s %i %li %f",
                                 get_name(), found_alignments, base_count_diff, score);
    }

    bool operator < (const Candidate& other) const { // sort highest score first
        int ali_diff = found_alignments-other.found_alignments;

        if (ali_diff)  {
            return ali_diff>0;
        }

        return score > other.score;
    }
};

// use SmartPtr to memory-manage Candidate's
static bool operator < (const SmartPtr<Candidate>& c1, const SmartPtr<Candidate>& c2) {
    return c1->operator<(*c2);
}
typedef set< SmartPtr<Candidate> > Candidates;

// add all candidate species to 'candidates'
static void find_species_candidates(Candidates& candidates, const char **ali_names) {
    aw_status("Examining species (kill to stop)");
    aw_status(0.0);

    // collect names of all species in source database
    GB_HASH *src_species = GBT_generate_species_hash(gb_merge, 1); // Note: changed to ignore case (ralf 2007-07-06)
    long     src_count   = GBS_hash_count_elems(src_species);
    long     found       = 0;
    bool     aborted     = false;

    // find existing species in destination database
    for (GBDATA *gb_dst_species = GBT_first_species(gb_dest);
         gb_dst_species && !aborted;
         gb_dst_species = GBT_next_species(gb_dst_species))
    {
        GBDATA     *gb_name = GB_find(gb_dst_species, "name", 0, down_level);
        const char *name    = GB_read_char_pntr(gb_name);

        if (GBDATA *gb_src_species = (GBDATA*)GBS_read_hash(src_species, name)) {
            Candidate *cand = new Candidate(true, name, gb_src_species, gb_dst_species, ali_names);

            if (cand->has_alignments() && cand->get_score()>0.0) {
                candidates.insert(cand);
            }
            else {
                if (GB_ERROR err = GB_get_error()) {
                    aw_message(GBS_global_string("Invalid adaption candidate '%s' (%s)", name, err));
                    GB_clear_error();
                }
                delete cand;
            }

            ++found;
            aw_status(double(found)/src_count);
            aborted = aw_status() != 0; // test user abort
        }
    }

    GBS_free_hash(src_species);
}

// add all candidate SAIs to 'candidates'
static void find_SAI_candidates(Candidates& candidates, const char **ali_names) {
    aw_status("Examining SAIs");
    aw_status(0.0);

    // collect names of all SAIs in source database
    GB_HASH *src_SAIs  = GBT_generate_SAI_hash(gb_merge);
    long     src_count = GBS_hash_count_elems(src_SAIs);
    long     found       = 0;

    // find existing SAIs in destination database
    for (GBDATA *gb_dst_SAI = GBT_first_SAI(gb_dest);
         gb_dst_SAI;
         gb_dst_SAI = GBT_next_SAI(gb_dst_SAI))
    {
        GBDATA     *gb_name = GB_find(gb_dst_SAI, "name", 0, down_level);
        const char *name    = GB_read_char_pntr(gb_name);

        if (GBDATA *gb_src_SAI = (GBDATA*)GBS_read_hash(src_SAIs, name)) {
            Candidate *cand = new Candidate(false, name, gb_src_SAI, gb_dst_SAI, ali_names);

            if (cand->has_alignments() && cand->get_score()>0.0) {
                candidates.insert(cand);
            }
            else {
                if (GB_ERROR err = GB_get_error()) {
                    aw_message(GBS_global_string("Invalid adaption candidate 'SAI:%s' (%s)", name, err));
                    GB_clear_error();
                }
                delete cand;
            }

            ++found;
            aw_status(double(found)/src_count);
        }
    }

    GBS_free_hash(src_SAIs);
}

// FIND button
// (rebuild candidates list)
static void calculate_preserves_cb(AW_window *, AW_CL cl_para) {
    GB_transaction ta1(gb_merge);
    GB_transaction ta2(gb_dest);

    preserve_para *para = (preserve_para*)cl_para;
    clear_candidates(para);

    AW_window  *aww     = para->window;
    AW_root    *aw_root = aww->get_root();
    char       *ali     = aw_root->awar(AWAR_REMAP_ALIGNMENT)->read_string();
    Candidates  candidates;

    aw_openstatus("Searching candidates");

    // add candidates
    if (0 == strcmp(ali, "All")) {
        char **ali_names = get_global_alignments();

        find_SAI_candidates(candidates, const_cast<const char**>(ali_names));
        find_species_candidates(candidates, const_cast<const char**>(ali_names));

        GBT_free_names(ali_names);
    }
    else {
        char *ali_names[2] = { ali, 0 };
        find_SAI_candidates(candidates, const_cast<const char**>(ali_names));
        find_species_candidates(candidates, const_cast<const char**>(ali_names));
    }

    int                   count = 0;
    Candidates::iterator  e     = candidates.end();
    AW_selection_list    *id    = para->cand_id;

    for (Candidates::iterator i = candidates.begin();
         i != e && count<5000;
         ++i, ++count)
    {
        string name  = (*i)->get_name();
        string shown = (*i)->get_entry();

        aww->insert_selection(id, shown.c_str(), name.c_str());
    }
    free(ali);

    aw_closestatus();

    aww->update_selection_list(id);
}

// ADD button
// (add current selected candidate to references)
static void add_selected_cb(AW_window *, AW_CL cl_para) {
    preserve_para *para = (preserve_para*)cl_para;
    AW_window     *aww  = para->window;
    AW_root       *awr  = aww->get_root();

    char *current    = awr->awar(AWAR_REMAP_CANDIDATE)->read_string();
    char *references = awr->awar(AWAR_REMAP_SPECIES_LIST)->read_string();
    if (references && references[0]) {
        string ref = string(references);
        size_t pos = ref.find(current);
        int    len = strlen(current);

        for (;
             pos != string::npos;
             pos = ref.find(current, pos+1))
        {
            if (pos == 0 || ref[pos-1] == '\n') {
                if ((pos+len) == ref.length()) break; // at eos -> skip
                char behind = ref[pos+len];
                if (behind == '\n') break; // already there -> skip
            }
        }

        if (pos == string::npos) {
            string appended = ref+'\n'+current;
            awr->awar(AWAR_REMAP_SPECIES_LIST)->write_string(appended.c_str());
        }
    }
    else {
        awr->awar(AWAR_REMAP_SPECIES_LIST)->write_string(current);
    }

    free(references);
    free(current);

    awr->awar(AWAR_REMAP_ENABLE)->write_int(1);
}

// CLEAR button
// (clear references)
static void clear_references_cb(AW_window *aww) {
    AW_root *awr = aww->get_root();
    awr->awar(AWAR_REMAP_SPECIES_LIST)->write_string("");
    awr->awar(AWAR_REMAP_ENABLE)->write_int(0);
}

// SELECT PRESERVES window
AW_window *MG_select_preserves_cb(AW_root *aw_root) {
    aw_root->awar_string(AWAR_REMAP_ALIGNMENT, "", gb_dest);
    aw_root->awar_string(AWAR_REMAP_CANDIDATE, "", gb_dest);

    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, "SELECT_PRESERVES", "Select adaption candidates");
    aws->load_xfig("merge/preserves.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_preserve.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("adapt");
    aws->label("Adapt alignments");
    aws->create_toggle(AWAR_REMAP_ENABLE);

    aws->at("reference");
    aws->create_text_field(AWAR_REMAP_SPECIES_LIST);

    preserve_para *para = new preserve_para; // do not free (is passed to callback)
    para->window        = aws;

    aws->at("candidate");
    para->cand_id = aws->create_selection_list(AWAR_REMAP_CANDIDATE, 0, "", 10, 30);

    aws->at("ali");
    para->ali_id = aws->create_selection_list(AWAR_REMAP_ALIGNMENT, 0, "", 10, 30);

    aws->at("find");
    aws->callback(calculate_preserves_cb, (AW_CL)para);
    aws->create_autosize_button("FIND", "Find candidates", "F", 1);

    {
        GB_transaction ta1(gb_merge);
        GB_transaction ta2(gb_dest);

        init_alignments(para);
        clear_candidates(para);
    }

    aws->button_length(8);

    aws->at("add");
    aws->callback(add_selected_cb, (AW_CL)para);
    aws->create_button("ADD", "Add", "A");

    aws->at("clear");
    aws->callback(clear_references_cb);
    aws->create_button("CLEAR", "Clear", "C");

    return aws;
}




