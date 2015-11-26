// =============================================================== //
//                                                                 //
//   File      : NT_dbrepair.cxx                                   //
//   Purpose   : repair database bugs                              //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2008       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"

#include <arbdbt.h>
#include <adGene.h>

#include <items.h>
#include <GEN.hxx>
#include <EXP.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_question.hxx>

#include <arb_str.h>
#include <arb_strarray.h>

#include <map>
#include <set>
#include <string>
#include <vector>
#include <ad_colorset.h>

using namespace std;

#if defined(WARN_TODO)
#warning the whole fix mechanism should be part of some lower-level-library
// meanwhile DB checks are only performed by ARB_NTREE
// ItemSelector should go to same library as this module
#endif

// --------------------------------------------------------------------------------
// CheckedConsistencies provides an easy way to automatically correct flues in the database
// by calling a check routine exactly once.
//
// For an example see nt_check_database_consistency()
//
// Note: this makes problems if DB is loaded with older ARB version and some already
// fixed flues a put into DB again.
// see http://bugs.arb-home.de/ticket/143

typedef GB_ERROR (*item_check_fun)(GBDATA *gb_item, ItemSelector& sel);

typedef map<string, item_check_fun>    item_check_map;
typedef item_check_map::const_iterator item_check_iter;

class CheckedConsistencies : virtual Noncopyable {
    GBDATA         *gb_main;
    size_t          species_count;
    size_t          sai_count;
    set<string>     consistencies;
    item_check_map  item_checks;

    GB_ERROR perform_selected_item_checks(ItemSelector& sel);

public:

    CheckedConsistencies(GBDATA *gb_main_) : gb_main(gb_main_) {
        GB_transaction ta(gb_main);
        GBDATA *gb_checks = GB_search(gb_main, "checks", GB_CREATE_CONTAINER);

        for (GBDATA *gb_check = GB_entry(gb_checks, "check"); gb_check; gb_check = GB_nextEntry(gb_check)) {
            consistencies.insert(GB_read_char_pntr(gb_check));
        }

        species_count = GBT_get_species_count(gb_main);
        sai_count = GBT_get_SAI_count(gb_main);
    }

    bool was_performed(const string& check_name) const {
        return consistencies.find(check_name) != consistencies.end();
    }

    GB_ERROR register_as_performed(const string& check_name) {
        GB_ERROR error = 0;
        if (was_performed(check_name)) {
            printf("check '%s' already has been registered before. Duplicated check name?\n", check_name.c_str());
        }
        else {
            GB_transaction ta(gb_main);

            GBDATA *gb_checks = GB_search(gb_main, "checks", GB_CREATE_CONTAINER);
            GBDATA *gb_check  = GB_create(gb_checks, "check", GB_STRING);

            if (!gb_check) error = GB_await_error();
            else error           = GB_write_string(gb_check, check_name.c_str());

            if (!error) consistencies.insert(check_name);
        }
        return error;
    }

    void perform_check(const string& check_name,
                       GB_ERROR (*do_check)(GBDATA *gb_main, size_t species, size_t sais),
                       GB_ERROR& error)
    {
        if (!error && !was_performed(check_name)) {
            arb_progress progress(check_name.c_str());
            error = do_check(gb_main, species_count, sai_count);
            if (!error) register_as_performed(check_name);
        }
    }

    void register_item_check(const string& check_name, item_check_fun item_check) {
        if (!was_performed(check_name)) {
            item_checks[check_name] = item_check;
        }
    }

    void perform_item_checks(GB_ERROR& error);

    GB_ERROR forgetDoneChecks() {
        GB_ERROR       error = 0;
        GB_transaction ta(gb_main);

        GBDATA *gb_checks = GB_search(gb_main, "checks", GB_CREATE_CONTAINER);
        for (GBDATA *gb_check = GB_entry(gb_checks, "check"); gb_check && !error; gb_check = GB_nextEntry(gb_check)) {
            char *check_name = GB_read_string(gb_check);

#if defined(DEBUG)
            printf("Deleting check '%s'\n", check_name);
#endif // DEBUG
            error = GB_delete(gb_check);
            consistencies.erase(check_name);
            free(check_name);
        }
        return error;
    }
};

GB_ERROR CheckedConsistencies::perform_selected_item_checks(ItemSelector& sel) {
    GB_ERROR        error = NULL;
    item_check_iter end   = item_checks.end();

    for (GBDATA *gb_cont = sel.get_first_item_container(gb_main, NULL, QUERY_ALL_ITEMS);
         gb_cont && !error;
         gb_cont = sel.get_next_item_container(gb_cont, QUERY_ALL_ITEMS))
    {
        for (GBDATA *gb_item = sel.get_first_item(gb_cont, QUERY_ALL_ITEMS);
             gb_item && !error;
             gb_item = sel.get_next_item(gb_item, QUERY_ALL_ITEMS))
        {
            for (item_check_iter chk = item_checks.begin(); chk != end && !error; ++chk) {
                error = chk->second(gb_item, sel);
            }
        }
    }

    return error;
}

void CheckedConsistencies::perform_item_checks(GB_ERROR& error) {
    if (!item_checks.empty()) {
        if (!error) {
            GB_transaction ta(gb_main);
            bool           is_genome_db = GEN_is_genome_db(gb_main, -1);

            error = perform_selected_item_checks(SPECIES_get_selector());
            if (!error && is_genome_db) {
                error             = perform_selected_item_checks(GEN_get_selector());
                if (!error) error = perform_selected_item_checks(EXP_get_selector());
            }

            error = ta.close(error);
        }

        if (!error) {
            item_check_iter end = item_checks.end();
            for (item_check_iter chk = item_checks.begin(); chk != end && !error; ++chk) {
                error = register_as_performed(chk->first);
            }

            if (!error) item_checks.clear();
        }
    }
}

// --------------------------------------------------------------------------------

static GB_ERROR NT_fix_gene_data(GBDATA *gb_main, size_t species_count, size_t /* sai_count */) {
    GB_transaction ta(gb_main);
    arb_progress   progress(species_count);

    size_t   deleted_gene_datas   = 0;
    size_t   generated_gene_datas = 0;
    GB_ERROR error                = 0;

    for (GBDATA *gb_species = GBT_first_species(gb_main);
         gb_species && !error;
         gb_species = GBT_next_species(gb_species))
    {
        bool    is_organism  = (GB_entry(gb_species, GENOM_ALIGNMENT) != 0); // same test as GEN_is_organism, but w/o genome-db-assertion
        GBDATA *gb_gene_data = GEN_find_gene_data(gb_species);

        if (is_organism && !gb_gene_data) {
            gb_gene_data = GEN_findOrCreate_gene_data(gb_species); // @@@ check result & handle error
            generated_gene_datas++;
        }
        else if (!is_organism && gb_gene_data) {
            GBDATA *gb_child = GB_child(gb_gene_data);
            if (!gb_child) {
                error = GB_delete(gb_gene_data);
                if (!error) deleted_gene_datas++;
            }
            else {
                error = GBS_global_string("Non-empty 'gene_data' found for species '%s',\n"
                                          "which has no alignment '" GENOM_ALIGNMENT "',\n"
                                          "i.e. which is not regarded as full-genome organism.\n"
                                          "This causes problems - please fix!",
                                          GBT_read_name(gb_species));
            }
        }

        progress.inc_and_check_user_abort(error);
    }

    if (!error) {
        if (deleted_gene_datas) {
            aw_message(GBS_global_string("Deleted %zu useless empty 'gene_data' entries.", deleted_gene_datas));
        }
        if (generated_gene_datas) {
            aw_message(GBS_global_string("Re-created %zu missing 'gene_data' entries.\nThese organisms have no genes yet!", generated_gene_datas));
        }
    }
    return ta.close(error);
}

// --------------------------------------------------------------------------------

static GBDATA *expectField(GBDATA *gb_gene, const char *field, GB_ERROR& data_error) {
    GBDATA *gb_field = 0;
    if (!data_error) {
        gb_field = GB_entry(gb_gene, field);
        if (!gb_field) data_error = GBS_global_string("Expected field '%s' missing", field);
    }
    return gb_field;
}

static GBDATA *disexpectField(GBDATA *gb_gene, const char *field, GB_ERROR& data_error) {
    GBDATA *gb_field = 0;
    if (!data_error) {
        gb_field = GB_entry(gb_gene, field);
        if (gb_field) data_error = GBS_global_string("Unexpected field '%s' exists (wrong value in pos_joined?)", field);
    }
    GBS_reuse_buffer(field);
    return gb_field;
}

static GB_ERROR NT_convert_gene_locations(GBDATA *gb_main, size_t species_count, size_t /* sai_count */) {
    GB_transaction ta(gb_main);
    GB_ERROR       error         = 0;
    long           fixed_genes   = 0;
    long           skipped_genes = 0;
    long           genes         = 0;

    typedef vector<GBDATA*> GBvec;
    GBvec                   toDelete;

    arb_progress progress(species_count);
    
    for (GBDATA *gb_organism = GEN_first_organism(gb_main);
         gb_organism && !error;
         gb_organism = GEN_next_organism(gb_organism))
    {
        GBDATA *gb_gene_data = GEN_find_gene_data(gb_organism);
        nt_assert(gb_gene_data);
        if (gb_gene_data) {
            for (GBDATA *gb_gene = GEN_first_gene_rel_gene_data(gb_gene_data);
                 gb_gene && !error;
                 gb_gene = GEN_next_gene(gb_gene))
            {
                genes++;

                int parts = 1;
                {
                    GBDATA *gb_pos_joined    = GB_entry(gb_gene, "pos_joined");
                    if (gb_pos_joined) parts = GB_read_int(gb_pos_joined); // its a joined gene
                }

                GBDATA *gb_pos_start = GB_entry(gb_gene, "pos_start"); // test for new format
                if (!gb_pos_start) {
                    GBDATA *gb_pos_begin = GB_entry(gb_gene, "pos_begin"); // test for old format
                    if (!gb_pos_begin) {
                        error = "Neither 'pos_begin' nor 'pos_start' found - format of gene location is unknown";
                    }
                }

                if (!gb_pos_start && !error) { // assume old format
                    // parts<-1 would be valid in new format, but here we have old format
                    if (parts<1) error = GBS_global_string("Illegal value in 'pos_joined' (%i)", parts);

                    GB_ERROR      data_error = 0;   // error in this gene -> don't convert
                    GEN_position *pos        = GEN_new_position(parts, false); // all were joinable (no information about it was stored)

                    // parse old gene information into 'pos'
                    //
                    // old-format was:
                    // Start-Positions:  pos_begin, pos_begin2, pos_begin3, ...
                    // End-Positions:    pos_end, pos_end2, pos_end3, ...
                    // Joined?:          pos_joined (always >= 1)
                    // Complement:       complement (one entry for all parts)
                    // Certainty:        pos_uncertain (maybe pos_uncertain1 etc.)

                    int complement = 0;
                    {
                        GBDATA *gb_complement = GB_entry(gb_gene, "complement");
                        if (gb_complement) {
                            complement = GB_read_byte(gb_complement);
                            toDelete.push_back(gb_complement);
                        }
                    }

                    bool has_uncertain_fields = false;
                    for (int p = 1; p <= parts && !error && !data_error; ++p) {
                        GBDATA     *gb_pos_begin        = 0;
                        GBDATA     *gb_pos_end          = 0;
                        const char *pos_uncertain_field = 0;

                        if (p == 1) {
                            gb_pos_begin = expectField(gb_gene, "pos_begin", data_error);
                            gb_pos_end   = expectField(gb_gene, "pos_end", data_error);

                            pos_uncertain_field = "pos_uncertain";
                        }
                        else {
                            const char *pos_begin_field = GBS_global_string("pos_begin%i", p);
                            const char *pos_end_field   = GBS_global_string("pos_end%i", p);

                            gb_pos_begin = expectField(gb_gene, pos_begin_field, data_error);
                            gb_pos_end   = expectField(gb_gene, pos_end_field, data_error);

                            GBS_reuse_buffer(pos_end_field);
                            GBS_reuse_buffer(pos_begin_field);

                            if (!data_error) pos_uncertain_field = GBS_global_string("pos_uncertain%i", p);
                        }

                        int pospos = complement ? (parts-p) : (p-1);

                        if (!data_error) {
                            GBDATA *gb_pos_uncertain = GB_entry(gb_gene, pos_uncertain_field);

                            if (!gb_pos_uncertain) {
                                if (has_uncertain_fields) data_error = GBS_global_string("Expected field '%s' missing", pos_uncertain_field);
                            }
                            else {
                                if (p == 1) has_uncertain_fields = true;
                                else {
                                    if (!has_uncertain_fields) {
                                        data_error = GBS_global_string("Found '%s' as first certainty-information", pos_uncertain_field);
                                    }
                                }
                            }

                            if (!data_error) {
                                int begin = GB_read_int(gb_pos_begin);
                                int end   = GB_read_int(gb_pos_end);

                                pos->start_pos[pospos]  = begin;
                                pos->stop_pos[pospos]   = end;
                                pos->complement[pospos] = complement; // set all complement entries to same value (old format only had one complement entry)

                                if (gb_pos_uncertain) {
                                    const char *uncertain = GB_read_char_pntr(gb_pos_uncertain);

                                    if (!uncertain) error = GB_await_error();
                                    else {
                                        if (!pos->start_uncertain) GEN_use_uncertainties(pos);

                                        if (strlen(uncertain) != 2) {
                                            data_error = "wrong length";
                                        }
                                        else {
                                            for (int up = 0; up<2; up++) {
                                                if (strchr("<=>", uncertain[up]) == 0) {
                                                    data_error = GBS_global_string("illegal character '%c'", uncertain[up]);
                                                }
                                                else {
                                                    (up == 0 ? pos->start_uncertain[pospos] : pos->stop_uncertain[pospos]) = uncertain[up];
                                                }
                                            }
                                        }


                                        toDelete.push_back(gb_pos_uncertain);
                                    }
                                }

                                toDelete.push_back(gb_pos_begin);
                                toDelete.push_back(gb_pos_end);
                            }
                        }
                    }

                    for (int p = parts+1; p <= parts+4 && !error && !data_error; ++p) {
                        disexpectField(gb_gene, GBS_global_string("pos_begin%i", p), data_error);
                        disexpectField(gb_gene, GBS_global_string("pos_end%i", p), data_error);
                        disexpectField(gb_gene, GBS_global_string("complement%i", p), data_error);
                        disexpectField(gb_gene, GBS_global_string("pos_uncertain%i", p), data_error);
                    }

                    // now save new position data

                    if (data_error) {
                        skipped_genes++;
                    }
                    else if (!error) {
                        error = GEN_write_position(gb_gene, pos, 0);

                        if (!error) {
                            // delete old-format entries
                            GBvec::const_iterator end = toDelete.end();
                            for (GBvec::const_iterator i = toDelete.begin(); i != end && !error; ++i) {
                                GBDATA *gb_del = *i;
                                error = GB_delete(gb_del);
                            }

                            if (!error) fixed_genes++;
                        }
                    }

                    toDelete.clear();
                    GEN_free_position(pos);

                    if (data_error || error) {
                        char *gene_id = GEN_global_gene_identifier(gb_gene, gb_organism);
                        if (error) {
                            error = GBS_global_string("Gene '%s': %s", gene_id, error);
                        }
                        else {
                            aw_message(GBS_global_string("Gene '%s' was not converted, fix data manually!\nReason: %s", gene_id, data_error));
                        }
                        free(gene_id);
                    }
                }
            }
        }

        progress.inc_and_check_user_abort(error);
    }

    if (!error) {
        if (fixed_genes>0) aw_message(GBS_global_string("Fixed location entries of %li genes.", fixed_genes));
        if (skipped_genes>0) {
            aw_message(GBS_global_string("Didn't fix location entries of %li genes (see warnings).", skipped_genes));
            error = "Not all gene locations were fixed.\nFix manually, save DB and restart ARB with that DB.\nMake sure you have a backup copy of the original DB!";
        }

        if (fixed_genes || skipped_genes) {
            long already_fixed_genes = genes-(fixed_genes+skipped_genes);
            if (already_fixed_genes>0) aw_message(GBS_global_string("Location entries of %li genes already were in new format.", already_fixed_genes));
        }
    }

    return error;
}


// --------------------------------------------------------------------------------

static GB_ERROR NT_del_mark_move_REF(GBDATA *gb_main, size_t species_count, size_t sai_count) {
    GB_transaction ta(gb_main);
    GB_ERROR       error   = 0;
    size_t         all     = species_count+sai_count;
    size_t         removed = 0;

    // delete 'mark' entries from all alignments of species/SAIs

    arb_progress  progress(all);
    ConstStrArray ali_names;
    GBT_get_alignment_names(ali_names, gb_main);

    for (int pass = 0; pass < 2 && !error; ++pass) {
        for (GBDATA *gb_item = (pass == 0) ? GBT_first_species(gb_main) : GBT_first_SAI(gb_main);
             gb_item && !error;
             gb_item = (pass == 0) ? GBT_next_species(gb_item) : GBT_next_SAI(gb_item))
        {
            for (int ali = 0; ali_names[ali] && !error; ++ali) {
                GBDATA *gb_ali = GB_entry(gb_item, ali_names[ali]);
                if (gb_ali) {
                    GBDATA *gb_mark = GB_entry(gb_ali, "mark");
                    if (gb_mark) {
                        error = GB_delete(gb_mark);
                        removed++;
                    }
                }
            }

            progress.inc_and_check_user_abort(error);
        }
    }

    {
        char   *helix_name = GBT_get_default_helix(gb_main);
        GBDATA *gb_helix   = GBT_find_SAI(gb_main, helix_name);

        if (gb_helix) {
            for (int ali = 0; ali_names[ali] && !error; ++ali) {
                GBDATA *gb_ali     = GB_entry(gb_helix, ali_names[ali]);
                GBDATA *gb_old_ref = GB_entry(gb_ali, "REF");
                GBDATA *gb_new_ref = GB_entry(gb_ali, "_REF");

                if (gb_old_ref) {
                    if (gb_new_ref) {
                        error = GBS_global_string("SAI:%s has 'REF' and '_REF' in '%s' (data corrupt?!)",
                                                  helix_name, ali_names[ali]);
                    }
                    else { // move info from REF -> _REF
                        char *content       = GB_read_string(gb_old_ref);
                        if (!content) error = GB_await_error();
                        else {
                            gb_new_ref             = GB_create(gb_ali, "_REF", GB_STRING);
                            if (!gb_new_ref) error = GB_await_error();
                            else {
                                error = GB_write_string(gb_new_ref, content);
                                if (!error) error = GB_delete(gb_old_ref);
                            }
                            free(content);
                        }
                    }
                }
            }
        }

        free(helix_name);
    }

    if (!error) {
        if (removed) {
            aw_message(GBS_global_string("Deleted %zu useless 'mark' entries.", removed));
        }
    }

    return ta.close(error);
}

// --------------------------------------------------------------------------------

static bool testDictionaryCompression(GBDATA *gbd, GBQUARK key_quark, bool testUse) {
    // returns true, if
    // testUse == true  and ANY entries below 'gbd' with quark 'key_quark' uses dictionary compression
    // testUse == false and ALL entries below 'gbd' with quark 'key_quark' can be decompressed w/o errors

    nt_assert(GB_read_type(gbd) == GB_DB);

    for (GBDATA *gb_sub = GB_child(gbd); gb_sub; gb_sub = GB_nextChild(gb_sub)) {
        switch (GB_read_type(gb_sub)) {
            case GB_DB:
                // return false if any compression failed or return true if any uses dict-compression
                if (testDictionaryCompression(gb_sub, key_quark, testUse) == testUse) return testUse;
                break;

            case GB_STRING:
            case GB_LINK:
                if (GB_get_quark(gb_sub) == key_quark && GB_is_dictionary_compressed(gb_sub)) {
                    if (testUse) return true;

                    const char *decompressed = GB_read_char_pntr(gb_sub);
                    if (!decompressed) return false;
                }
                break;

            default:
                break;
        }
    }

    return !testUse;
}

class                  Dict;
typedef SmartPtr<Dict> DictPtr;


class KeyInfo : virtual Noncopyable {
    string  name;               // keyname
    DictPtr original;

    bool compressionTested;
    bool compressed;

    void init() {
        compressionTested = false;
        compressed        = false;
    }

public:
    KeyInfo(const char *Name)                       : name(Name)                         { init(); }
    KeyInfo(const char *Name, DictPtr originalDict) : name(Name), original(originalDict) { init(); }

    void testCompressed(GBDATA *gb_main) {
        nt_assert(!compressionTested);
        compressed        = testDictionaryCompression(gb_main, GB_find_or_create_quark(gb_main, name.c_str()), true);
        compressionTested = true;
    }

    const string& getName() const { return name; }

    bool isCompressed() const {
        nt_assert(compressionTested);
        return compressed;
    }
};


class Dict : virtual Noncopyable {
    string    group;            // lowercase keyname
    string    orgkey;
    DictData *data;

    map<string, bool> decompressWorks; // key -> bool

public:
    static GBDATA *gb_main;

    Dict(const char *Group, const char *OrgKey, DictData *Data) : group(Group), orgkey(OrgKey), data(Data) {}

    const string& getGroup() const { return group; }
    const string& getOriginalKey() const { return orgkey; }

    bool mayBeUsedWith(const string& key) const { return strcasecmp(group.c_str(), key.c_str()) == 0; }

    GB_ERROR assignToKey(const string& key) const { return GB_set_dictionary(gb_main, key.c_str(), data); }
    GB_ERROR unassignFromKey(const string& key) const { return GB_set_dictionary(gb_main, key.c_str(), NULL); }

    bool canDecompress(const string& key) {
        nt_assert(mayBeUsedWith(key));
        if (decompressWorks.find(key) == decompressWorks.end()) {
            bool     works = false;
            GB_ERROR error = assignToKey(key);

            if (!error) works    = testDictionaryCompression(gb_main, GB_find_or_create_quark(gb_main, key.c_str()), false);
            decompressWorks[key] = works;

            GB_ERROR err2 = unassignFromKey(key);
            if (err2) {
                aw_message(GBS_global_string("Error while removing @dictionary from key '%s': %s", key.c_str(), err2));
            }
        }
        return decompressWorks[key];
    }
};
GBDATA *Dict::gb_main = NULL;


typedef map<string, int>        KeyCounter; // groupname -> occur count
typedef SmartPtr<KeyInfo>       KeyInfoPtr;
typedef map<string, KeyInfoPtr> Keys; // keyname -> info
typedef map<string, DictPtr>    DictMap;
typedef vector<DictPtr>         Dicts;
typedef set<string>             StringSet;

#define STATUS_PREFIX "Dictionary: "

template<typename CONT, typename KEY>
bool contains(const CONT& container, const KEY& key) {
    return container.find(key) != container.end();
}

static GB_ERROR findAffectedKeys(GBDATA *gb_key_data, KeyCounter& kcount, Keys& keys, Dicts& dicts) {
    GB_ERROR  error   = 0;
    GBDATA   *gb_main = GB_get_root(gb_key_data);

    for (int pass = 1; pass <= 2; ++pass) {
        for (GBDATA *gb_key = GB_entry(gb_key_data, "@key"); !error && gb_key; gb_key = GB_nextEntry(gb_key)) {
            GBDATA     *gb_name = GB_entry(gb_key, "@name");
            const char *keyName = GB_read_char_pntr(gb_name);

            if (!keyName) {
                error = GBS_global_string("@key w/o @name (%s)", GB_await_error());
            }
            else {
                char *keyGroup = strdup(keyName);
                ARB_strlower(keyGroup);

                switch (pass) {
                    case 1:
                        kcount[keyGroup]++;
                        break;
                    case 2:
                        if (kcount[keyGroup]>1) {
                            GBDATA *gb_dictionary = GB_entry(gb_key, "@dictionary");
                            if (gb_dictionary) {
                                DictPtr dict  = new Dict(keyGroup, keyName, GB_get_dictionary(gb_main, keyName));
                                keys[keyName] = new KeyInfo(keyName, dict);
                                dicts.push_back(dict);
                            }
                            else keys[keyName] = new KeyInfo(keyName);
                        }
                        else kcount.erase(keyGroup);
                        break;
                }
                free(keyGroup);
            }
        }
    }
    return error;
}

static GB_ERROR deleteDataOfKey(GBDATA *gbd, GBQUARK key_quark, StringSet& deletedData, long& deleted, long& notDeleted) {
    GB_ERROR error = 0;
    for (GBDATA *gb_sub = GB_child(gbd); gb_sub; gb_sub = GB_nextChild(gb_sub)) {
        switch (GB_read_type(gb_sub)) {
            case GB_DB:
                error = deleteDataOfKey(gb_sub, key_quark, deletedData, deleted, notDeleted);
                break;

            case GB_STRING:
            case GB_LINK:
                if (GB_get_quark(gb_sub) == key_quark) {
                    if (GB_is_dictionary_compressed(gb_sub)) {
                        string path(GB_get_db_path(gb_sub));
                        error = GB_delete(gb_sub);
                        if (!error) {
                            deletedData.insert(path);
                            deleted++;
                        }
                    }
                    else {
                        notDeleted++;
                    }
                }
                break;
            default:
                break;
        }
    }
    return error;
}

static char *readFirstCompressedDataOf(GBDATA *gbd, GBQUARK key_quark) {
    char *data = 0;
    for (GBDATA *gb_sub = GB_child(gbd); !data && gb_sub; gb_sub = GB_nextChild(gb_sub)) {
        switch (GB_read_type(gb_sub)) {
            case GB_DB:
                data = readFirstCompressedDataOf(gb_sub, key_quark);
                break;

            case GB_STRING:
            case GB_LINK:
                if (GB_get_quark(gb_sub) == key_quark) {
                    if (GB_is_dictionary_compressed(gb_sub)) {
                        data = GB_read_as_string(gb_sub);
                    }
                }
                break;
            default:
                break;
        }
    }
    return data;
}


static GB_ERROR NT_fix_dict_compress(GBDATA *gb_main, size_t, size_t) {
    GB_transaction  ta(gb_main);
    GBDATA         *gb_key_data = GB_search(gb_main, GB_SYSTEM_FOLDER "/" GB_SYSTEM_KEY_DATA, GB_FIND);
    GB_ERROR        error       = 0;

    Dict::gb_main = gb_main;

    if (!gb_key_data) {
        error = "No " GB_SYSTEM_KEY_DATA " found.. DB corrupted?";
    }
    else {
        KeyCounter kcount;      // strlwr(keyname) -> count
        Keys       keys;
        Dicts      dicts;

        error = findAffectedKeys(gb_key_data, kcount, keys, dicts);

        // count affected keys
        int affectedKeys = 0;
        for (KeyCounter::iterator kci = kcount.begin(); kci != kcount.end(); ++kci) {
            affectedKeys += kci->second;
        }

        if (!error && affectedKeys>0) {
            // check which keys are compressed

            {
                arb_progress progress(STATUS_PREFIX "search compressed data", affectedKeys);

                for (Keys::iterator ki = keys.begin(); ki != keys.end(); ++ki) {
                    KeyInfoPtr k = ki->second;
                    k->testCompressed(gb_main);
                    ++progress;
                }
            }

            // test which key/dict combinations work
            int combinations = 0;          // possible key/dict combinations

            DictMap   use;      // keyname -> dictionary (which dictionary to use)
            StringSet multiDecompressible; // keys which can be decompressed with multiple dictionaries

            for (int pass = 1; pass <= 2; ++pass) {
                arb_progress *progress  = NULL;
                if (pass == 2 && combinations) progress = new arb_progress(STATUS_PREFIX "test compression", combinations);

                for (Dicts::iterator di = dicts.begin(); di != dicts.end(); ++di) {
                    DictPtr d = *di;

                    for (Keys::iterator ki = keys.begin(); ki != keys.end(); ++ki) {
                        KeyInfoPtr    k       = ki->second;
                        const string& keyname = k->getName();

                        if (k->isCompressed() && d->mayBeUsedWith(keyname)) {
                            switch (pass) {
                                case 1:
                                    combinations++;
                                    break;
                                case 2:
                                    if (d->canDecompress(keyname)) {
                                        if (!contains(use, keyname)) { // first dictionary working with keyname
                                            use[keyname] = d;
                                        }
                                        else { // already have another dictionary working with keyname
                                            multiDecompressible.insert(keyname);
                                        }
                                    }
                                    ++(*progress);
                                    break;
                            }
                        }
                    }
                }
                delete progress;
            }

            StringSet notDecompressible; // keys which can be decompressed with none of the dictionaries
            for (Keys::iterator ki = keys.begin(); ki != keys.end(); ++ki) {
                KeyInfoPtr    k       = ki->second;
                const string& keyname = k->getName();

                if (k->isCompressed()) {
                    if (!contains(use, keyname)) notDecompressible.insert(keyname);
                    if (contains(multiDecompressible, keyname)) use.erase(keyname);
                }
            }

            bool dataLost   = false;
            int  reassigned = 0;

            if (!notDecompressible.empty()) {
                // bad .. found undecompressible data
                int nd_count = notDecompressible.size();
                aw_message(GBS_global_string("Detected corrupted dictionary compression\n"
                                             "Data of %i DB-keys is lost and will be deleted", nd_count));

                arb_progress progress(STATUS_PREFIX "deleting corrupt data", nd_count);

                StringSet deletedData;
                long      deleted    = 0;
                long      notDeleted = 0;

                for (StringSet::iterator ki = notDecompressible.begin(); !error && ki != notDecompressible.end(); ++ki) {
                    const string& keyname    = *ki;

                    error = deleteDataOfKey(gb_main, GB_find_or_create_quark(gb_main, keyname.c_str()), deletedData, deleted, notDeleted);
                    ++progress;
                }

                if (!error) {
                    nt_assert(deleted); // at least 1 db-entry should have been deleted

                    aw_message(GBS_global_string("Deleted %li of %li affected DB-entries", deleted, deleted+notDeleted));
                    aw_message("see console for a list of affected keys");

                    printf("Deleted keys:\n");
                    for (StringSet::iterator di = deletedData.begin(); di != deletedData.end(); ++di) {
                        printf("* %s\n", di->c_str());
                    }
                }
            }

            if (!error && !multiDecompressible.empty()) {
                for (StringSet::iterator ki = multiDecompressible.begin(); !error && ki != multiDecompressible.end(); ++ki) {
                    const string&   keyname  = *ki;
                    int             possible = 0;
                    vector<DictPtr> possibleDicts;

                    printf("--------------------------------------------------------------------------------\n");

                    for (Dicts::iterator di = dicts.begin(); !error && di != dicts.end(); ++di) {
                        DictPtr d = *di;
                        if (d->mayBeUsedWith(keyname) && d->canDecompress(keyname)) {
                            error = d->assignToKey(keyname);
                            if (!error) {
                                char *data = readFirstCompressedDataOf(gb_main, GB_find_or_create_quark(gb_main, keyname.c_str()));

                                nt_assert(data);
                                possible++;
                                printf("possibility %i = '%s'\n", possible, data);
                                free(data);

                                possibleDicts.push_back(d);

                                error = d->unassignFromKey(keyname);
                            }
                        }
                    }

                    if (!error) {
                        nt_assert(possible>0);

                        int selected;
                        if (possible>1) {
                            char *question = GBS_global_string_copy("%i possibilities to decompress field '%s' have been detected\n"
                                                                    "and example data was dumped to the console.\n"
                                                                    "Please examine output and decide which is the correct possibility!",
                                                                    possible, keyname.c_str());

                            const char *buttons = "Abort";
                            for (int p = 1; p <= possible; ++p) buttons = GBS_global_string("%s,%i", buttons, p);
                            selected = aw_question("dict_decompress_bug", question, buttons, false, NULL);
                            free(question);
                        }
                        else {
                            selected = 1;
                        }

                        if (!selected) {
                            error = "Aborted by user";
                        }
                        else {
                            use[keyname] = possibleDicts[selected-1];
                        }
                    }
                }
            }

            // now all redundancies should be eliminated and we can assign dictionaries to affected keys
            if (!error) {
                for (Keys::iterator ki = keys.begin(); !error && ki != keys.end(); ++ki) {
                    KeyInfoPtr    k       = ki->second;
                    const string& keyname = k->getName();

                    if (k->isCompressed()) {
                        if (!contains(use, keyname)) {
                            error = GBS_global_string("No dictionary detected for key '%s'", keyname.c_str());
                        }
                        else {
                            DictPtr d = use[keyname];

                            if (d->getOriginalKey() != keyname) {
                                d->assignToKey(keyname); // set the dictionary
                                aw_message(GBS_global_string("Assigning '%s'-dictionary to '%s'",
                                                             d->getOriginalKey().c_str(), keyname.c_str()));
                                reassigned++;
                            }
                        }
                    }
                }
            }

            if (dataLost||reassigned) {
                aw_message(dataLost
                           ? "We apologize for the data-loss."
                           : "No conflicts detected in compressed data.");
                aw_message("Dictionaries fixed.\n"
                           "Please save your database with a new name.");
            }
        }
    }

    Dict::gb_main = NULL;
    return ta.close(error);
}

// --------------------------------------------------------------------------------

static GB_ERROR remove_dup_colors(GBDATA *gb_item, ItemSelector& IF_DEBUG(sel)) {
    // Databases out there may contain multiple 'ARB_color' entries.
    // Due to some already fixed bug - maybe introduced in r5309 and fixed in r5825

    GBDATA   *gb_color = GB_entry(gb_item, GB_COLORGROUP_ENTRY);
    GB_ERROR  error    = NULL;

#if defined(DEBUG)
    int del_count = 0;
#endif // DEBUG

    if (gb_color) {
        GB_push_my_security(gb_color);
        while (!error) {
            GBDATA *gb_next_color = GB_nextEntry(gb_color);
            if (!gb_next_color) break;

            error = GB_delete(gb_next_color);
#if defined(DEBUG)
            if (!error) del_count++;
#endif // DEBUG
        }
        GB_pop_my_security(gb_color);
    }

#if defined(DEBUG)
    if (del_count) fprintf(stderr,
                           "- deleted %i duplicated '" GB_COLORGROUP_ENTRY "' from %s '%s'\n",
                           del_count,
                           sel.item_name,
                           sel.generate_item_id(GB_get_root(gb_item), gb_item));
#endif // DEBUG

    return error;
}

// --------------------------------------------------------------------------------

GB_ERROR NT_repair_DB(GBDATA *gb_main) {
    // status is already open and will be closed by caller!

    CheckedConsistencies check(gb_main);
    GB_ERROR             err = 0;
    bool                 is_genome_db;
    {
        GB_transaction ta(gb_main);
        is_genome_db = GEN_is_genome_db(gb_main, -1);
    }

    check.perform_check("fix gene_data",     NT_fix_gene_data,     err);
    check.perform_check("fix_dict_compress", NT_fix_dict_compress, err); // do this before NT_del_mark_move_REF (cause 'REF' is affected)
    check.perform_check("del_mark_move_REF", NT_del_mark_move_REF, err);

    if (is_genome_db) {
        check.perform_check("convert_gene_locations", NT_convert_gene_locations, err);
    }

    check.register_item_check("duplicated_item_colors", remove_dup_colors);
    check.perform_item_checks(err);

    return err;
}

void NT_rerepair_DB(AW_window*, GBDATA *gb_main) {
    // re-perform all DB checks
    GB_ERROR err = 0;
    {
        CheckedConsistencies check(gb_main);
        err = check.forgetDoneChecks();
    }
    if (!err) {
        arb_progress progress("DB-Repair");
        err = NT_repair_DB(gb_main);
    }

    if (err) aw_message(err);
}


