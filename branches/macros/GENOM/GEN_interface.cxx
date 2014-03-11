// =============================================================  //
//                                                                //
//   File      : GEN_interface.cxx                                //
//   Purpose   :                                                  //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2001 //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#include "GEN_local.hxx"

#include <db_scanner.hxx>
#include <db_query.h>
#include <dbui.h>
#include <item_sel_list.h>
#include <awt_sel_boxes.hxx>
#include <aw_awar_defs.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_question.hxx>
#include <arbdbt.h>
#include <adGene.h>
#include <Location.h>
#include <arb_strarray.h>
#include <arb_strbuf.h>
#include <info_window.h>

using namespace std;

// --------------------------------------------------------------------------------

#define AWAR_GENE_DEST "tmp/gene/dest"

// --------------------------------------------------------------------------------

#if defined(WARN_TODO)
#warning replace all occurrences of AD_F_ALL by AWM_ALL
#endif

#define AD_F_ALL AWM_ALL

// --------------------------------------------------------------------------------

static void GEN_select_gene(GBDATA* /* gb_main */, AW_root *aw_root, const char *item_name) {
    char *organism  = strdup(item_name);
    char *gene = strchr(organism, '/');

    if (gene) {
        *gene++ = 0;
        aw_root->awar(AWAR_ORGANISM_NAME)->write_string(organism);
        aw_root->awar(AWAR_GENE_NAME)->write_string(gene);
    }
    else if (!item_name[0]) { // accept empty input -> deselect gene/organism
        aw_root->awar(AWAR_GENE_NAME)->write_string("");
        aw_root->awar(AWAR_ORGANISM_NAME)->write_string("");
    }
    else {
        aw_message(GBS_global_string("Illegal item_name '%s' in GEN_select_gene()", item_name));
    }
    free(organism);
}

static char *gen_get_gene_id(GBDATA * /* gb_main */, GBDATA *gb_gene) {
    GBDATA *gb_species = GB_get_grandfather(gb_gene);
    return GBS_global_string_copy("%s/%s", GBT_read_name(gb_species), GBT_read_name(gb_gene));
}

static GBDATA *gen_find_gene_by_id(GBDATA *gb_main, const char *id) {
    char   *organism = strdup(id);
    char   *gene     = strchr(organism, '/');
    GBDATA *result   = 0;

    if (gene) {
        *gene++ = 0;
        GBDATA *gb_organism = GEN_find_organism(gb_main, organism);
        if (gb_organism) {
            result = GEN_find_gene(gb_organism, gene);
        }
    }

    free(organism);
    return result;
}


GB_ERROR GEN_mark_organism_or_corresponding_organism(GBDATA *gb_species, int */*client_data*/) {
    GB_ERROR error = 0;

    if (GEN_is_pseudo_gene_species(gb_species)) {
        GBDATA *gb_organism = GEN_find_origin_organism(gb_species, 0);
        if (gb_organism) {
            GB_write_flag(gb_organism, 1);
        }
        else {
            error = GEN_organism_not_found(gb_species);
        }
    }
    else if (GEN_is_organism(gb_species)) {
        GB_write_flag(gb_species, 1);
    }

    return error;
}

static char *old_species_marks = 0; // configuration storing marked species

inline void gen_restore_old_species_marks(GBDATA *gb_main) {
    if (old_species_marks) {
        GBT_restore_marked_species(gb_main, old_species_marks);
        freenull(old_species_marks);
    }
}

static GBDATA *GEN_get_first_gene_data(GBDATA *gb_main, AW_root *aw_root, QUERY_RANGE range) {
    GBDATA   *gb_organism = 0;
    GB_ERROR  error      = 0;

    gen_restore_old_species_marks(gb_main);

    switch (range) {
        case QUERY_CURRENT_ITEM: {
            char *species_name = aw_root->awar(AWAR_ORGANISM_NAME)->read_string();
            gb_organism         = GEN_find_organism(gb_main, species_name);
            free(species_name);
            break;
        }
        case QUERY_MARKED_ITEMS: {
            gb_organism = GEN_first_marked_organism(gb_main);
            GBDATA *gb_pseudo   = GEN_first_marked_pseudo_species(gb_main);

            if (gb_pseudo) {    // there are marked pseudo-species..
                old_species_marks = GBT_store_marked_species(gb_main, 1); // store and unmark marked species

                error                   = GBT_with_stored_species(gb_main, old_species_marks, GEN_mark_organism_or_corresponding_organism, 0); // mark organisms related with stored
                if (!error) gb_organism = GEN_first_marked_organism(gb_main);
            }

            break;
        }
        case QUERY_ALL_ITEMS: {
            gb_organism = GEN_first_organism(gb_main);
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    if (error) GB_export_error(error);
    return gb_organism ? GEN_expect_gene_data(gb_organism) : 0;
}

static GBDATA *GEN_get_next_gene_data(GBDATA *gb_gene_data, QUERY_RANGE range) {
    GBDATA *gb_organism = 0;
    switch (range) {
        case QUERY_CURRENT_ITEM: {
            break;
        }
        case QUERY_MARKED_ITEMS: {
            GBDATA *gb_last_organism = GB_get_father(gb_gene_data);
            gb_organism              = GEN_next_marked_organism(gb_last_organism);

            if (!gb_organism) gen_restore_old_species_marks(GB_get_root(gb_last_organism)); // got all -> clean up

            break;
        }
        case QUERY_ALL_ITEMS: {
            GBDATA *gb_last_organism = GB_get_father(gb_gene_data);
            gb_organism              = GEN_next_organism(gb_last_organism);
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    return gb_organism ? GEN_expect_gene_data(gb_organism) : 0;
}

static GBDATA *GEN_get_current_gene(GBDATA *gb_main, AW_root *aw_root) {
    GBDATA *gb_species = GEN_get_current_organism(gb_main, aw_root);
    GBDATA *gb_gene    = 0;

    if (gb_species) {
        char *gene_name = aw_root->awar(AWAR_GENE_NAME)->read_string();
        gb_gene         = GEN_find_gene(gb_species, gene_name);
        free(gene_name);
    }

    return gb_gene;
}

static void add_selected_gene_changed_cb(AW_root *aw_root, const RootCallback& cb) {
    aw_root->awar(AWAR_GENE_NAME)->add_callback(cb);
    ORGANISM_get_selector().add_selection_changed_cb(aw_root, cb);
}

static GBDATA *first_gene_in_range(GBDATA *gb_gene_data, QUERY_RANGE range) {
    GBDATA *gb_first = NULL;
    switch (range) {
        case QUERY_ALL_ITEMS:    gb_first = GEN_first_gene_rel_gene_data(gb_gene_data); break;
        case QUERY_MARKED_ITEMS: gb_first = GB_first_marked(gb_gene_data, "gene"); break;
        case QUERY_CURRENT_ITEM: gb_first = GEN_get_current_gene(GB_get_root(gb_gene_data), AW_root::SINGLETON); break;
    }
    return gb_first;
}
static GBDATA *next_gene_in_range(GBDATA *gb_prev, QUERY_RANGE range) {
    GBDATA *gb_next = NULL;
    switch (range) {
        case QUERY_ALL_ITEMS:    gb_next = GEN_next_gene(gb_prev); break;
        case QUERY_MARKED_ITEMS: gb_next = GEN_next_marked_gene(gb_prev); break;
        case QUERY_CURRENT_ITEM: gb_next = NULL; break;
    }
    return gb_next;
}

#if defined(WARN_TODO)
#warning move GEN_item_selector to SL/ITEMS
#endif

static struct MutableItemSelector GEN_item_selector = {
    QUERY_ITEM_GENES,
    GEN_select_gene,
    gen_get_gene_id,
    gen_find_gene_by_id,
    gene_field_selection_list_update_cb,
    -1, // unknown
    CHANGE_KEY_PATH_GENES,
    "gene",
    "genes",
    "name",
    GEN_get_first_gene_data,
    GEN_get_next_gene_data,
    first_gene_in_range,
    next_gene_in_range,
    GEN_get_current_gene,
    add_selected_gene_changed_cb,
    &ORGANISM_get_selector(), GB_get_grandfather,
};

ItemSelector& GEN_get_selector() { return GEN_item_selector; }

static void GEN_species_name_changed_cb(AW_root *awr, AW_CL cl_gb_main) {
    char           *species_name = awr->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA         *gb_main      = (GBDATA*)cl_gb_main;
    GB_transaction  ta(gb_main);
    GBDATA         *gb_species   = GBT_find_species(gb_main, species_name);

    if (gb_species) {
        if (GEN_is_pseudo_gene_species(gb_species)) {
            awr->awar(AWAR_ORGANISM_NAME)->write_string(GEN_origin_organism(gb_species));
            awr->awar(AWAR_GENE_NAME)->write_string(GEN_origin_gene(gb_species));
        }
        else {
            awr->awar(AWAR_ORGANISM_NAME)->write_string(species_name);
        }
    }
    free(species_name);
}

static void auto_select_pseudo_species(AW_root *awr, GBDATA *gb_main, const char *organism, const char *gene) {
    GB_transaction  ta(gb_main);
    GBDATA         *gb_pseudo = GEN_find_pseudo_species(gb_main, organism, gene, 0); // search for pseudo species..

    awr->awar(AWAR_SPECIES_NAME)->write_string(gb_pseudo
                                               ? GBT_read_name(gb_pseudo) // .. if found select
                                               : organism);               // otherwise select organism
}

static void GEN_update_GENE_CONTENT(GBDATA *gb_main, AW_root *awr) {
    GB_transaction  ta(gb_main);
    GBDATA         *gb_gene      = GEN_get_current_gene(gb_main, awr);
    bool            clear        = true;
    AW_awar        *awar_content = awr->awar(AWAR_GENE_CONTENT);

    if (gb_gene) {
        // ignore complement here (to highlight gene in ARB_EDIT4);
        // separate multiple parts by \n
        char *gene_content = GBT_read_gene_sequence(gb_gene, false, '\n');
        if (gene_content) {
            awar_content->write_string(gene_content);
            clear = false;
            free(gene_content);
        }
        else {
            awar_content->write_string(GB_await_error());
        }
    }
    else {
        char      *gene_name  = awr->awar(AWAR_GENE_NAME)->read_string();
        const int  prefix_len = 10;

        if (strncmp(gene_name, "intergene_", prefix_len) == 0) { // special case (non-gene result from gene pt server)
            char *start_pos_ptr = gene_name+prefix_len;
            char *end_pos_ptr   = strchr(start_pos_ptr, '_');

            gen_assert(end_pos_ptr);
            if (end_pos_ptr) {
                *end_pos_ptr++ = 0;
                long start_pos = atol(start_pos_ptr);
                long end_pos   = atol(end_pos_ptr);

                gen_assert(end_pos >= start_pos);

                GBDATA *gb_organism = GEN_get_current_organism(gb_main, awr);
                if (gb_organism) {
                    GBDATA     *gb_seq   = GBT_read_sequence(gb_organism, GENOM_ALIGNMENT);
                    const char *seq_data = GB_read_char_pntr(gb_seq);

                    long  len    = end_pos-start_pos+1;
                    char *buffer = (char*)malloc(len+1);
                    memcpy(buffer, seq_data+start_pos, len);
                    buffer[len]  = 0;

                    awar_content->write_string(buffer);
                    clear = false;

                    free(buffer);
                }
            }
        }
        free(gene_name);
    }

    if (clear) {
        awar_content->write_string(""); // if we did not detect any gene sequence -> clear
    }
}

static void GEN_update_combined_cb(AW_root *awr, AW_CL cl_gb_main) {
    char       *organism     = awr->awar(AWAR_ORGANISM_NAME)->read_string();
    char       *gene         = awr->awar(AWAR_GENE_NAME)->read_string();
    char       *old_combined = awr->awar(AWAR_COMBINED_GENE_NAME)->read_string();
    const char *combined     = GBS_global_string("%s/%s", organism, gene);

    if (strcmp(combined, old_combined) != 0) {
        GBDATA *gb_main = (GBDATA*)cl_gb_main;
        awr->awar(AWAR_COMBINED_GENE_NAME)->write_string(combined);
        auto_select_pseudo_species(awr, gb_main, organism, gene);
        GEN_update_GENE_CONTENT(gb_main, awr);
    }

    free(old_combined);
    free(gene);
    free(organism);
}

void GEN_create_awars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main) {
    aw_root->awar_string(AWAR_COMBINED_GENE_NAME, "", gb_main);
    aw_root->awar_string(AWAR_GENE_CONTENT,       0,  gb_main);

    aw_root->awar_string(AWAR_GENE_NAME,     "", gb_main)->add_callback(GEN_update_combined_cb,      (AW_CL)gb_main);
    aw_root->awar_string(AWAR_ORGANISM_NAME, "", gb_main)->add_callback(GEN_update_combined_cb,      (AW_CL)gb_main);
    aw_root->awar_string(AWAR_SPECIES_NAME,  "", gb_main)->add_callback(GEN_species_name_changed_cb, (AW_CL)gb_main);

    aw_root->awar_string(AWAR_GENE_DEST, "", aw_def);
    
    aw_root->awar_string(AWAR_GENE_EXTRACT_ALI, "ali_gene_",    aw_def);
}

GBDATA *GEN_get_current_organism(GBDATA *gb_main, AW_root *aw_root) {
    char   *species_name = aw_root->awar(AWAR_ORGANISM_NAME)->read_string();
    GBDATA *gb_species   = GBT_find_species(gb_main, species_name);
    free(species_name);
    return gb_species;
}

GBDATA* GEN_get_current_gene_data(GBDATA *gb_main, AW_root *aw_root) {
    GBDATA *gb_species      = GEN_get_current_organism(gb_main, aw_root);
    GBDATA *gb_gene_data    = 0;

    if (gb_species) gb_gene_data = GEN_expect_gene_data(gb_species);

    return gb_gene_data;
}

static QUERY::DbQuery *GLOBAL_gene_query      = 0;

static void gene_rename_cb(AW_window *aww, AW_CL cl_gb_main) {
    AW_root *aw_root = aww->get_root();
    char    *source  = aw_root->awar(AWAR_GENE_NAME)->read_string();
    char    *dest    = aw_root->awar(AWAR_GENE_DEST)->read_string();

    if (strcmp(source, dest) != 0) {
        GBDATA   *gb_main = (GBDATA*)cl_gb_main;
        GB_ERROR  error   = GB_begin_transaction(gb_main);

        if (!error) {
            GBDATA *gb_gene_data = GEN_get_current_gene_data(gb_main, aww->get_root());

            if (!gb_gene_data) error = "Please select a species";
            else {
                GBDATA *gb_source = GEN_find_gene_rel_gene_data(gb_gene_data, source);
                GBDATA *gb_dest   = GEN_find_gene_rel_gene_data(gb_gene_data, dest);

                if (!gb_source) error   = "Please select a gene first";
                else if (gb_dest) error = GB_export_errorf("Gene '%s' already exists", dest);
                else {
                    GBDATA *gb_name = GB_search(gb_source, "name", GB_STRING);

                    if (!gb_name) error = GB_await_error();
                    else {
                        error = GB_write_string(gb_name, dest);
                        if (!error) aww->get_root()->awar(AWAR_GENE_NAME)->write_string(dest);
                    }
                }
            }
        }

        error = GB_end_transaction(gb_main, error);
        aww->hide_or_notify(error);
    }

    free(source);
    free(dest);
}

static AW_window *create_gene_rename_window(AW_root *root, AW_CL cl_gb_main) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "RENAME_GENE", "GENE RENAME");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the gene");

    aws->at("input");
    aws->create_input_field(AWAR_GENE_DEST, 15);
    aws->at("ok");
    aws->callback(gene_rename_cb, cl_gb_main);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

static void gene_copy_cb(AW_window *aww, AW_CL cl_gb_main) {
    char     *source  = aww->get_root()->awar(AWAR_GENE_NAME)->read_string();
    char     *dest    = aww->get_root()->awar(AWAR_GENE_DEST)->read_string();
    GBDATA   *gb_main = (GBDATA*)cl_gb_main;
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    if (!error) {
        GBDATA *gb_gene_data = GEN_get_current_gene_data(gb_main, aww->get_root());

        if (!gb_gene_data) {
            error = "Please select a species first.";
        }
        else {
            GBDATA *gb_source = GEN_find_gene_rel_gene_data(gb_gene_data, source);
            GBDATA *gb_dest   = GEN_find_gene_rel_gene_data(gb_gene_data, dest);

            if (!gb_source) error   = "Please select a gene";
            else if (gb_dest) error = GB_export_errorf("Gene '%s' already exists", dest);
            else {
                gb_dest             = GB_create_container(gb_gene_data, "gene");
                if (!gb_dest) error = GB_await_error();
                else error          = GB_copy(gb_dest, gb_source);

                if (!error) error = GBT_write_string(gb_dest, "name", dest);
                if (!error) aww->get_root()->awar(AWAR_GENE_NAME)->write_string(dest);
            }
        }
    }

    error = GB_end_transaction(gb_main, error);
    aww->hide_or_notify(error);

    free(source);
    free(dest);
}

static AW_window *create_gene_copy_window(AW_root *root, AW_CL cl_gb_main) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "COPY_GENE", "GENE COPY");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nof the new gene");

    aws->at("input");
    aws->create_input_field(AWAR_GENE_DEST, 15);

    aws->at("ok");
    aws->callback(gene_copy_cb, cl_gb_main);
    aws->create_button("GO", "GO", "G");

    return aws;
}


// -----------------------
//      LocationEditor

const int NAME_WIDTH = 33;
const int POS_WIDTH  = NAME_WIDTH;
const int CER_WIDTH  = POS_WIDTH/2;
const int LOC_WIDTH  = 53;

#define GLE_POS1       "pos1"
#define GLE_POS2       "pos2"
#define GLE_CERT1      "cert1"
#define GLE_CERT2      "cert2"
#define GLE_COMPLEMENT "complement"
#define GLE_JOINABLE   "joinable"
#define GLE_READABLE   "location"
#define GLE_STATUS     "status"

class LocationEditor : virtual Noncopyable { // GLE
    typedef void (*PosChanged_cb)(AW_root *aw_root, LocationEditor *loced);

    char          *tag;
    AW_root       *aw_root;
    GBDATA        *gb_main;
    char          *status;
    GEN_position  *pos;
    PosChanged_cb  pos_changed_cb;

    void createAwars();

    void createInputField(AW_window *aws, const char *at, const char *aname, int width) const {
        aws->at(at);
        aws->create_input_field(loc_awar_name(aname), width);
    }

public:
    LocationEditor(AW_root *aw_root_, GBDATA *gb_main_, const char *tag_)
        : tag(strdup(tag_)),
          aw_root(aw_root_),
          gb_main(gb_main_), 
          status(NULL),
          pos(NULL),
          pos_changed_cb(NULL)
    {
        createAwars();
        set_status(NULL);
    }
    ~LocationEditor() {
        free(tag);
        free(status);
        GEN_free_position(pos);
    }

    GBDATA *get_gb_main() const { return gb_main; }

    void add_pos_changed_cb(PosChanged_cb cb) { pos_changed_cb = cb; }

    const GEN_position *get_pos() const { return pos; }
    void set_pos(GEN_position*& pos_) {
        GEN_free_position(pos);
        pos  = pos_;
        pos_ = NULL; // take ownership
        set_status(status);

        if (pos && pos_changed_cb) {
            pos_changed_cb(aw_root, this);
        }
    }
    void set_status(const char *status_) {
        if (status != status_) freedup(status, status_);
        loc_awar(GLE_STATUS)->write_string(status ? status : (pos ? "Ok" : "No data"));
    }

    const char *loc_awar_name(const char *aname) const {
        const int   BUFSIZE = 100;
        static char buf[BUFSIZE];

        IF_ASSERTION_USED(int printed =) sprintf(buf, "tmp/loc/%s/%s", tag, aname);
        gen_assert(printed<BUFSIZE);

        return buf;
    }
    AW_awar *loc_awar(const char *aname) const { return aw_root->awar(loc_awar_name(aname)); }
    const char *awar_charp_value(const char *aname) const { return loc_awar(aname)->read_char_pntr(); }

    void createEditFields(AW_window *aws);
    
    GEN_position *create_GEN_position_from_fields(GB_ERROR& error);
    void revcomp() {
        loc_awar(GLE_READABLE)->write_string(GBS_global_string("complement(%s)", awar_charp_value(GLE_READABLE)));
    }
};

inline const char *elemOr(ConstStrArray& a, size_t i, const char *Default) {
    return i<a.size() ? a[i] : Default;
}
inline char elemOr(const char *a, size_t len, size_t i, char Default) {
    return i<len ? a[i] : Default;
}

GEN_position *LocationEditor::create_GEN_position_from_fields(GB_ERROR& error) {
    const char *ipos1  = awar_charp_value(GLE_POS1);
    const char *ipos2  = awar_charp_value(GLE_POS2);
    const char *icert1 = awar_charp_value(GLE_CERT1);
    const char *icert2 = awar_charp_value(GLE_CERT2);
    const char *icomp  = awar_charp_value(GLE_COMPLEMENT);

    const char *sep = ",; ";

    ConstStrArray pos1, pos2;
    GBT_split_string(pos1, ipos1, sep, true);
    GBT_split_string(pos2, ipos2, sep, true);

    size_t clen1 = strlen(icert1);
    size_t clen2 = strlen(icert2);
    size_t clen  = strlen(icomp);

    size_t max_size = pos1.size();
    bool   joinable = loc_awar(GLE_JOINABLE)->read_int();

    GEN_position *gp = NULL;
    if (max_size>0) {
        gp = GEN_new_position(max_size, joinable);
        GEN_use_uncertainties(gp);

        for (size_t i = 0; i<max_size; ++i) {
            const char *p1c = elemOr(pos1, i, "1");
            size_t      p1  = atoi(p1c);
            size_t      p2  = atoi(elemOr(pos2, i, p1c));

            char c  = elemOr(icomp,  clen,  i, '0');
            char c1 = elemOr(icert1, clen1, i, '=');
            char c2 = elemOr(icert2, clen2, i, '=');

            gen_assert(c1 && c2);

            gp->start_pos[i]       = p1;
            gp->stop_pos[i]        = p2;
            gp->complement[i]      = !!(c-'0');
            gp->start_uncertain[i] = c1;
            gp->stop_uncertain[i]  = c2;
        }
    }
    else {
        error = "No data";
    }

    return gp;
}

static int loc_update_running = 0;

inline GB_ERROR update_location_from_GEN_position(LocationEditor *loced, const GEN_position *gp) {
    GB_ERROR error = NULL;
    if (gp) {
        try {
            LocationPtr loc = to_Location(gp);
            loced->loc_awar(GLE_READABLE)->write_string(loc->as_string().c_str());
        }
        catch (const char *& err) { error = GBS_static_string(err); }
        catch (string& err) { error = GBS_static_string(err.c_str()); }
        catch (...) { gen_assert(0); }
    }
    return error;
}


static void GLE_update_from_detailFields(AW_root *, AW_CL cl_loced) {
    // update location according to other fields
    if (!loc_update_running) {
        loc_update_running++;

        GB_ERROR        error = NULL;
        LocationEditor *loced = (LocationEditor*)cl_loced;
        GEN_position   *gp    = loced->create_GEN_position_from_fields(error);
        
        if (!error) {
            error = update_location_from_GEN_position(loced, gp);
        }

        loced->set_pos(gp);
        loced->set_status(error);

        loc_update_running--;
    }
}


static SmartCharPtr sizetarray2string(const size_t *array, int size) {
    GBS_strstruct out(size*5);
    for (int i = 0; i<size; ++i) {
        out.putlong(array[i]);
        out.put(' ');
    }
    out.cut_tail(1);
    return out.release();
}
inline SmartCharPtr dupSizedPart(const unsigned char *in, int size) {
    const char *in2 = reinterpret_cast<const char*>(in);
    return GB_strpartdup(in2, in2+size-1);
}
inline SmartCharPtr dupComplement(const unsigned char *in, int size) {
    char *dup = (char*)malloc(size+1);
    for (int i = 0; i<size; ++i) {
        dup[i] = in[i] ? '1' : '0';
    }
    dup[size] = 0;
    return dup;
}

static void GLE_update_from_location(AW_root *, AW_CL cl_loced) {
    // update other fields according to location
    if (!loc_update_running) {
        loc_update_running++;

        GB_ERROR        error  = NULL;
        LocationEditor *loced  = (LocationEditor*)cl_loced;
        const char     *locstr = loced->awar_charp_value(GLE_READABLE);
        GEN_position   *gp     = NULL;

        try {
            LocationPtr loc = parseLocation(locstr);
            gp              = loc->create_GEN_position();
        }
        catch (const char *& err) { error = GBS_static_string(err); }
        catch (string& err) { error = GBS_static_string(err.c_str()); }
        catch (...) { gen_assert(0); }

        if (!error) {
            loced->loc_awar(GLE_POS1)->write_string(&*sizetarray2string(gp->start_pos, gp->parts));
            loced->loc_awar(GLE_POS2)->write_string(&*sizetarray2string(gp->stop_pos, gp->parts));

            loced->loc_awar(GLE_CERT1)->write_string(&*dupSizedPart(gp->start_uncertain, gp->parts));
            loced->loc_awar(GLE_CERT2)->write_string(&*dupSizedPart(gp->stop_uncertain, gp->parts));
            loced->loc_awar(GLE_COMPLEMENT)->write_string(&*dupComplement(gp->complement, gp->parts));

            loced->loc_awar(GLE_JOINABLE)->write_int(gp->parts>1 ? gp->joinable : 1);
        }

        loced->set_pos(gp);
        loced->set_status(error);

        loc_update_running--;
    }
}

static void GLE_revcomp_cb(AW_window*, AW_CL cl_loced) {
    LocationEditor *loced  = (LocationEditor*)cl_loced;
    loced->revcomp();
}

void LocationEditor::createAwars() {
    aw_root->awar_string(loc_awar_name(GLE_POS1),       "", gb_main)->add_callback(GLE_update_from_detailFields,      (AW_CL)this);
    aw_root->awar_string(loc_awar_name(GLE_POS2),       "", gb_main)->add_callback(GLE_update_from_detailFields,      (AW_CL)this);
    aw_root->awar_string(loc_awar_name(GLE_CERT1),      "", gb_main)->add_callback(GLE_update_from_detailFields,      (AW_CL)this);
    aw_root->awar_string(loc_awar_name(GLE_CERT2),      "", gb_main)->add_callback(GLE_update_from_detailFields,      (AW_CL)this);
    aw_root->awar_string(loc_awar_name(GLE_COMPLEMENT), "", gb_main)->add_callback(GLE_update_from_detailFields,      (AW_CL)this);
    aw_root->awar_int   (loc_awar_name(GLE_JOINABLE),    1, gb_main)->add_callback(GLE_update_from_detailFields,      (AW_CL)this);
    aw_root->awar_string(loc_awar_name(GLE_READABLE),   "", gb_main)->add_callback(GLE_update_from_location, (AW_CL)this);
    aw_root->awar_string(loc_awar_name(GLE_STATUS),     "", gb_main);
}

void LocationEditor::createEditFields(AW_window *aws) {
    createInputField(aws, "pos1",   GLE_POS1,       POS_WIDTH);
    createInputField(aws, "cert1",  GLE_CERT1,      CER_WIDTH);
    createInputField(aws, "pos2",   GLE_POS2,       POS_WIDTH);
    createInputField(aws, "cert2",  GLE_CERT2,      CER_WIDTH);
    createInputField(aws, "comp",   GLE_COMPLEMENT, CER_WIDTH);
    createInputField(aws, "loc",    GLE_READABLE,   LOC_WIDTH);
    createInputField(aws, "status", GLE_STATUS,     LOC_WIDTH);

    aws->at("rev");
    aws->callback(GLE_revcomp_cb, (AW_CL)this);
    aws->button_length(8);
    aws->create_button("REV", "RevComp");
    
    aws->at("join");
    aws->create_toggle(loc_awar_name(GLE_JOINABLE));
}

static void gene_changed_cb(AW_root *aw_root, AW_CL cl_loced) {
    LocationEditor *loced   = (LocationEditor*)cl_loced;
    GBDATA         *gb_gene = GEN_get_current_gene(loced->get_gb_main(), aw_root);
    GEN_position   *pos     = gb_gene ? GEN_read_position(gb_gene) : NULL;

    GB_ERROR error = NULL;
    if (pos) {
        GEN_use_uncertainties(pos);
        error = update_location_from_GEN_position(loced, pos);
    }
    loced->set_pos(pos);
    loced->set_status(error);
}

static void boundloc_changed_cb(AW_root *aw_root, LocationEditor *loced) {
    GBDATA*             gb_main = loced->get_gb_main();
    const GEN_position *pos     = loced->get_pos();
    gen_assert(pos);

    GB_push_transaction(gb_main);
    GBDATA   *gb_gene = GEN_get_current_gene(gb_main, aw_root);
    GB_ERROR  error;
    if (gb_gene) {
        error = GEN_write_position(gb_gene, pos, 0);
    }
    else {
        error = "That had no effect (no gene is selected)";
    }
    GB_end_transaction_show_error(gb_main, error, aw_message);

    if (error) {
        gene_changed_cb(aw_root, (AW_CL)loced);
    }
}

static void gene_create_cb(AW_window *aww, AW_CL cl_gb_main, AW_CL cl_loced) {
    GBDATA   *gb_main = (GBDATA*)cl_gb_main;
    GB_ERROR  error   = GB_begin_transaction(gb_main);

    if (!error) {
        AW_root *aw_root      = aww->get_root();
        GBDATA  *gb_gene_data = GEN_get_current_gene_data(gb_main, aw_root);

        if (!gb_gene_data) error = "Please select an organism";
        else {
            char    *dest    = aw_root->awar(AWAR_GENE_DEST)->read_string();
            GBDATA  *gb_dest = GEN_find_gene_rel_gene_data(gb_gene_data, dest);

            if (gb_dest) error = GBS_global_string("Gene '%s' already exists", dest);
            else {
                LocationEditor     *loced = (LocationEditor*)cl_loced;
                const GEN_position *pos   = loced->get_pos();

                if (!pos) error = "Won't create a gene with invalid position";
                else  {
                    gb_dest             = GEN_find_or_create_gene_rel_gene_data(gb_gene_data, dest);
                    if (!gb_dest) error = GB_await_error();
                    else error          = GEN_write_position(gb_dest, pos, 0);

                    if (!error) {
                        aw_root->awar(AWAR_GENE_NAME)->write_string(dest);
                        aww->hide();
                    }
                }
            }

            free(dest);
        }
    }
    GB_end_transaction_show_error(gb_main, error, aw_message);

    gen_assert(!GB_have_error());
}

static AW_window *get_gene_create_or_locationEdit_window(AW_root *root, bool createGene, GBDATA *gb_main) {
    static AW_window_simple *awa[2] = { NULL, NULL};
    static LocationEditor   *le[2] = { NULL, NULL};

    AW_window_simple*& aws   = awa[createGene];
    LocationEditor*&   loced = le[createGene];

    if (!aws) {
        gen_assert(gb_main);

        aws   = new AW_window_simple;
        loced = new LocationEditor(root, gb_main, createGene ? "create" : "edit");

        if (createGene) aws->init(root, "CREATE_GENE",   "GENE CREATE");
        else            aws->init(root, "EDIT_LOCATION", "EDIT LOCATION");

        aws->load_xfig("ad_gen_create.fig");

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "Close", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("gen_create.hlp"));
        aws->create_button("HELP", "Help", "H");

        aws->button_length(NAME_WIDTH);
        
        aws->at("organism");
        aws->create_button(NULL, AWAR_ORGANISM_NAME);

        aws->at("gene");
        if (createGene) {
            aws->create_input_field(AWAR_GENE_DEST, NAME_WIDTH);
        }
        else {
            aws->create_button(NULL, AWAR_GENE_NAME);
            AW_awar *awar_cgene = aws->get_root()->awar(AWAR_COMBINED_GENE_NAME);
            awar_cgene->add_callback(gene_changed_cb, (AW_CL)loced);
            awar_cgene->touch();

            loced->add_pos_changed_cb(boundloc_changed_cb);
        }
        aws->button_length(0);

        loced->createEditFields(aws);

        if (createGene) {
            aws->at_shift(0, 30);
            aws->callback(gene_create_cb, (AW_CL)gb_main, (AW_CL)loced);
            aws->create_autosize_button("CREATE", "Create gene", "G");
        }
    }
    return aws;
}
static void popup_gene_location_editor(AW_window *aww, AW_CL cl_gb_main, AW_CL) {
    AW_window *aws = get_gene_create_or_locationEdit_window(aww->get_root(), false, (GBDATA*)cl_gb_main);
    aws->activate();
}
static AW_window *create_gene_create_window(AW_root *root, AW_CL cl_gb_main) {
    return get_gene_create_or_locationEdit_window(root, true, (GBDATA*)cl_gb_main);
}

static void gene_delete_cb(AW_window *aww, AW_CL cl_gb_main) {
    if (aw_ask_sure("gene_delete", "Are you sure to delete the gene?")) {
        GBDATA         *gb_main = (GBDATA*)cl_gb_main;
        GB_transaction  ta(gb_main);
        GBDATA         *gb_gene = GEN_get_current_gene(gb_main, aww->get_root());

        GB_ERROR error = gb_gene ? GB_delete(gb_gene) : "Please select a gene first";
        if (error) {
            error = ta.close(error);
            aw_message(error);
        }
    }
}

static void GEN_create_field_items(AW_window *aws, GBDATA *gb_main) {
    static BoundItemSel *bis = new BoundItemSel(gb_main, GEN_get_selector());
    gen_assert(bis->gb_main == gb_main);

    aws->insert_menu_topic("gen_reorder_fields", "Reorder fields ...",    "R", "spaf_reorder.hlp", AD_F_ALL, AW_POPUP, (AW_CL)DBUI::create_fields_reorder_window, (AW_CL)bis);
    aws->insert_menu_topic("gen_delete_field",   "Delete/Hide field ...", "D", "spaf_delete.hlp",  AD_F_ALL, AW_POPUP, (AW_CL)DBUI::create_field_delete_window, (AW_CL)bis);
    aws->insert_menu_topic("gen_create_field",   "Create fields ...",     "C", "spaf_create.hlp",  AD_F_ALL, AW_POPUP, (AW_CL)DBUI::create_field_create_window, (AW_CL)bis);
    aws->sep______________();
    aws->insert_menu_topic("gen_unhide_fields",  "Show all hidden fields", "S", "scandb.hlp", AD_F_ALL, makeWindowCallback(gene_field_selection_list_unhide_all_cb, gb_main, FIELD_FILTER_NDS));
    aws->insert_menu_topic("gen_refresh_fields", "Refresh fields",         "f", "scandb.hlp", AD_F_ALL, makeWindowCallback(gene_field_selection_list_update_cb,     gb_main, FIELD_FILTER_NDS));
    aws->sep______________();
    aws->insert_menu_topic("gen_edit_loc", "Edit gene location", "l", "gen_create.hlp", AD_F_ALL, popup_gene_location_editor, (AW_CL)gb_main, 0);
}

#if defined(WARN_TODO)
#warning move GEN_popup_gene_infowindow to SL/DB_UI
#endif

static AW_window *popup_new_gene_window(AW_root *aw_root, GBDATA *gb_main, int detach_id);

static void popup_detached_gene_window(AW_window *aw_parent, const InfoWindow *infoWin) {
    const InfoWindow *reusable = InfoWindowRegistry::infowin.find_reusable_of_same_type_as(*infoWin);
    if (reusable) reusable->reuse();
    else { // create a new window if none is reusable
        popup_new_gene_window(aw_parent->get_root(),
                              infoWin->get_gbmain(),
                              InfoWindowRegistry::infowin.allocate_detach_id(*infoWin));
    }
}

static AW_window *popup_new_gene_window(AW_root *aw_root, GBDATA *gb_main, int detach_id) { // INFO_WINDOW_CREATOR
    AW_window_simple_menu *aws      = new AW_window_simple_menu;
    const ItemSelector&    itemType = GEN_get_selector();

    DBUI::init_info_window(aw_root, aws, itemType, detach_id);
    aws->load_xfig("ad_spec.fig");

    aws->button_length(8);

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("search");
    aws->callback(AW_POPUP, (AW_CL)GEN_create_gene_query_window, (AW_CL)gb_main);
    aws->create_button("SEARCH", "SEARCH", "S");

    aws->at("help");
    aws->callback(makeHelpCallback("gene_info.hlp"));
    aws->create_button("HELP", "HELP", "H");

    DbScanner         *scanner = create_db_scanner(gb_main, aws, "box", 0, "field", "enable", DB_VIEWER, 0, "mark", FIELD_FILTER_NDS, itemType);
    const InfoWindow&  infoWin = InfoWindowRegistry::infowin.registerInfoWindow(aws, scanner, detach_id);

    aws->create_menu("GENE", "G", AD_F_ALL);
    aws->insert_menu_topic("gene_delete", "Delete",     "D", "spa_delete.hlp", AD_F_ALL, (AW_CB)gene_delete_cb, (AW_CL)gb_main, 0);
    aws->insert_menu_topic("gene_rename", "Rename ...", "R", "spa_rename.hlp", AD_F_ALL, AW_POPUP, (AW_CL)create_gene_rename_window, (AW_CL)gb_main);
    aws->insert_menu_topic("gene_copy",   "Copy ...",   "y", "spa_copy.hlp",   AD_F_ALL, AW_POPUP, (AW_CL)create_gene_copy_window,   (AW_CL)gb_main);
    aws->insert_menu_topic("gene_create", "Create ...", "C", "gen_create.hlp", AD_F_ALL, AW_POPUP, (AW_CL)create_gene_create_window, (AW_CL)gb_main);
    aws->sep______________();

    aws->create_menu("FIELDS", "F", AD_F_ALL);
    GEN_create_field_items(aws, gb_main);

    aws->at("detach");
    infoWin.add_detachOrGet_button(popup_detached_gene_window);

    aws->show();
    infoWin.attach_selected_item();

    return aws;
}

void GEN_popup_gene_infowindow(AW_root *aw_root, GBDATA *gb_main) {
    static AW_window *aws = 0;
    if (!aws) {
        aws = popup_new_gene_window(aw_root, gb_main, InfoWindow::MAIN_WINDOW);
    }
    else {
        aws->activate();
    }
}

#if defined(WARN_TODO)
#warning move GEN_create_gene_query_window to SL/DB_UI
#endif

AW_window *GEN_create_gene_query_window(AW_root *aw_root, AW_CL cl_gb_main) {
    static AW_window_simple_menu *aws = 0;

    if (!aws) {
        aws = new AW_window_simple_menu;
        aws->init(aw_root, "GEN_QUERY", "Gene SEARCH and QUERY");
        aws->create_menu("More functions", "f");
        aws->load_xfig("ad_query.fig");

        QUERY::query_spec awtqs(GEN_get_selector());

        awtqs.gb_main             = (GBDATA*)cl_gb_main;
        awtqs.species_name        = AWAR_SPECIES_NAME;
        awtqs.tree_name           = AWAR_TREE;
        awtqs.select_bit          = GB_USERFLAG_QUERY;
        awtqs.use_menu            = 1;
        awtqs.ere_pos_fig         = "ere3";
        awtqs.where_pos_fig       = "where3";
        awtqs.by_pos_fig          = "by3";
        awtqs.qbox_pos_fig        = "qbox";
        awtqs.rescan_pos_fig      = 0;
        awtqs.key_pos_fig         = 0;
        awtqs.query_pos_fig       = "content";
        awtqs.result_pos_fig      = "result";
        awtqs.count_pos_fig       = "count";
        awtqs.do_query_pos_fig    = "doquery";
        awtqs.config_pos_fig      = "doconfig";
        awtqs.do_mark_pos_fig     = "domark";
        awtqs.do_unmark_pos_fig   = "dounmark";
        awtqs.do_delete_pos_fig   = "dodelete";
        awtqs.do_set_pos_fig      = "doset";
        awtqs.do_refresh_pos_fig  = "dorefresh";
        awtqs.open_parser_pos_fig = "openparser";
        awtqs.popup_info_window   = GEN_popup_gene_infowindow;

        QUERY::DbQuery *query = create_query_box(aws, &awtqs, "gen");
        GLOBAL_gene_query     = query;

        aws->create_menu("More search",     "s");
        aws->insert_menu_topic("gen_search_equal_fields_within_db", "Search For Equal Fields and Mark Duplicates",               "E", "search_duplicates.hlp", AWM_ALL, (AW_CB)QUERY::search_duplicated_field_content, (AW_CL)query, 0);
        aws->insert_menu_topic("gen_search_equal_words_within_db",  "Search For Equal Words Between Fields and Mark Duplicates", "W", "search_duplicates.hlp", AWM_ALL, (AW_CB)QUERY::search_duplicated_field_content, (AW_CL)query, 1);

        aws->button_length(7);

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");
        aws->at("help");
        aws->callback(makeHelpCallback("gene_search.hlp"));
        aws->create_button("HELP", "HELP", "H");
    }

    return aws;
}
