// =============================================================== //
//                                                                 //
//   File      : NT_dbrepair.cxx                                   //
//   Purpose   : repair database bugs                              //
//   Time-stamp: <Wed Jul/09/2008 12:32 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2008       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_dbrepair.hxx"

#include <arbdbt.h>
#include <adGene.h>
#include <aw_root.hxx>

#include <smartptr.h>
#include <arbtools.h>
#include <inline.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include <cstdlib>
#include <cstring>

#define nt_assert(bed) arb_assert(bed)

using namespace std;

// --------------------------------------------------------------------------------
// CheckedConsistencies provides an easy way to automatically correct flues in the database
// by calling a check routine exactly one.
// For an example see nt_check_database_consistency()

class CheckedConsistencies {
    GBDATA      *gb_main;
    size_t       species_count;
    size_t       sai_count;
    set<string>  consistencies;

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

            if (!gb_check) {
                error = GB_get_error();
            }
            else {
                error = GB_write_string(gb_check, check_name.c_str());
            }

            if (!error) consistencies.insert(check_name);
        }
        return error;
    }

    void perform_check(const string& check_name,
                       GB_ERROR (*do_check)(GBDATA *gb_main, size_t species, size_t sais),
                       GB_ERROR& error)
    {
        if (!error && !was_performed(check_name)) {
            aw_status(check_name.c_str());
            aw_status(0.0);
            error = do_check(gb_main, species_count, sai_count);
            aw_status(1.0);
            if (!error) register_as_performed(check_name);
        }
    }
};

// --------------------------------------------------------------------------------

static GB_ERROR NT_fix_gene_data(GBDATA *gb_main, size_t species_count, size_t /*sai_count*/) {
    GB_transaction ta(gb_main);
    size_t         deleted_gene_datas   = 0;
    size_t         generated_gene_datas = 0;
    GB_ERROR       error                = 0;
    size_t         count                = 0;

    for (GBDATA *gb_species = GBT_first_species(gb_main);
         gb_species && !error;
         gb_species = GBT_next_species(gb_species))
    {
        bool    is_organism  = (GB_entry(gb_species, GENOM_ALIGNMENT) != 0); // same test as GEN_is_organism, but w/o genome-db-assertion
        GBDATA *gb_gene_data = GEN_find_gene_data(gb_species);

        if (is_organism && !gb_gene_data) {
            gb_gene_data = GEN_findOrCreate_gene_data(gb_species);
            generated_gene_datas++;
        }
        else if (!is_organism && gb_gene_data) {
            GBDATA *gb_child = GB_child(gb_gene_data);
            if (!gb_child) {
                error = GB_delete(gb_gene_data);
                if (!error) deleted_gene_datas++;
            }
            else {
                const char *name    = "<unknown species>";
                GBDATA     *gb_name = GB_entry(gb_species, "name");
                if (gb_name) name = GB_read_char_pntr(gb_name);

                error = GBS_global_string("Non-empty 'gene_data' found for species '%s',\n"
                                          "which has no alignment '" GENOM_ALIGNMENT "',\n"
                                          "i.e. which is not regarded as full-genome organism.\n"
                                          "This causes problems - please fix!", name);
            }
        }
        
        aw_status(double(++count)/species_count);
    }

    if (error) {
        ta.abort();
    }
    else {
        if (deleted_gene_datas) {
            aw_message(GBS_global_string("Deleted %zu useless empty 'gene_data' entries.", deleted_gene_datas));
        }
        if (generated_gene_datas) {
            aw_message(GBS_global_string("Re-created %zu missing 'gene_data' entries.\nThese organisms have no genes yet!", generated_gene_datas));
        }
    }
    return error;
}

// --------------------------------------------------------------------------------

static GB_ERROR NT_del_mark_move_REF(GBDATA *gb_main, size_t species_count, size_t sai_count) {
    GB_transaction ta(gb_main);
    GB_ERROR       error   = 0;
    size_t         count   = 0;
    size_t         all     = species_count+sai_count;
    size_t         removed = 0;

    // delete 'mark' entries from all alignments of species/SAIs

    char **ali_names = GBT_get_alignment_names(gb_main);

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

            aw_status(double(++count)/all);
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
                        char *content = GB_read_string(gb_old_ref);
                        if (content) {
                            gb_new_ref = GB_create(gb_ali, "_REF", GB_STRING);
                            if (!gb_new_ref) {
                                error = GB_get_error();
                            }
                            else {
                                error = GB_write_string(gb_new_ref, content);
                                if (!error) error = GB_delete(gb_old_ref);
                            }
                            free(content);
                        }
                        else {
                            error = GB_get_error();
                        }
                    }
                }
            }
        }

        free(helix_name);
    }

    GBT_free_names(ali_names);

    if (error) {
        ta.abort();
    }
    else {
        if (removed) {
            aw_message(GBS_global_string("Deleted %zu useless 'mark' entries.", removed));
        }
    }

    return error;
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
                if (GB_get_quark(gb_sub) == key_quark && GB_is_directory_compressed(gb_sub)) {
                    if (testUse) return true;

                    const char *decompressed = GB_read_char_pntr(gb_sub);
                    if (!decompressed) return false;
                }
                break;
                
            default :
                break;
        }
    }

    return !testUse;
}

class                  Dict;
typedef SmartPtr<Dict> DictPtr;


class KeyInfo : Noncopyable {
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
        compressed        = testDictionaryCompression(gb_main, GB_key_2_quark(gb_main, name.c_str()), true);
        compressionTested = true;
    }

    const string& getName() const { return name; }

    bool isCompressed() const {
        nt_assert(compressionTested);
        return compressed;
    }
};


class Dict : Noncopyable {
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

            if (!error) works    = testDictionaryCompression(gb_main, GB_key_2_quark(gb_main, key.c_str()), false);
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

#define STATUS_PREFIX "Dictionary-bug: "

template<typename CONT, typename KEY>
bool contains(const CONT& container, const KEY& key) {
    return container.find(key) != container.end();
}

static GB_ERROR findAffectedKeys(GBDATA *gb_key_data, KeyCounter& kcount, Keys& keys, Dicts& dicts) {
    GB_ERROR  error   = 0;
    GBDATA   *gb_main = GB_get_root(gb_key_data) ;

    for (int pass = 1; pass <= 2; ++pass) {
        for (GBDATA *gb_key = GB_entry(gb_key_data, "@key"); !error && gb_key; gb_key = GB_nextEntry(gb_key)) {
            GBDATA     *gb_name = GB_entry(gb_key, "@name");
            const char *keyName = GB_read_char_pntr(gb_name);

            if (!keyName) {
                error = GBS_global_string("@key w/o @name (%s)", GB_expect_error());
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
                    if (GB_is_directory_compressed(gb_sub)) {
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
            default :
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
                    if (GB_is_directory_compressed(gb_sub)) {
                        data = GB_read_as_string(gb_sub);
                    }
                }
                break;
            default :
                break;
        }
    }
    return data;
}


static GB_ERROR NT_fix_dict_compress(GBDATA *gb_main, size_t, size_t) {
    GB_transaction  ta(gb_main);
    GBDATA         *gb_key_data = GB_search(gb_main, "__SYSTEM__/@key_data", GB_FIND);
    GB_ERROR        error       = 0;

    Dict::gb_main = gb_main;

    aw_status(STATUS_PREFIX "init");
    aw_status(0.0);

    if (!gb_key_data) {
        error = "No @key_data found.. DB corrupted?";
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

        if (!error) {
            // check which keys are compressed
            int cnt            = 0;
            aw_status(STATUS_PREFIX "search compression");
            aw_status(0.0);
            for (Keys::iterator ki = keys.begin(); ki != keys.end(); ++ki) {
                KeyInfoPtr k = ki->second;
                k->testCompressed(gb_main);
                aw_status(++cnt/double(affectedKeys));
            }

            // test which key/dict combinations work
            aw_status(STATUS_PREFIX "test compression");
            aw_status(0.0);
            cnt              = 0;
            int combinations = 0; // possible key/dict combinations

            DictMap   use;      // keyname -> dictionary (which dictionary to use)
            StringSet multiDecompressable; // keys which can be decompressed with multiple dictionaries

            for (int pass = 1; pass <= 2; ++pass) {
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
                                            multiDecompressable.insert(keyname);
                                        }
                                    }
                                    aw_status(++cnt/double(combinations));
                                    break;
                            }
                        }
                    }
                }
            }

            StringSet notDecompressable; // keys which can be decompressed with none of the dictionaries
            for (Keys::iterator ki = keys.begin(); ki != keys.end(); ++ki) {
                KeyInfoPtr    k       = ki->second;
                const string& keyname = k->getName();

                if (k->isCompressed()) {
                    if (!contains(use, keyname)) notDecompressable.insert(keyname);
                    if (contains(multiDecompressable, keyname)) use.erase(keyname);
                }
            }

            bool dataLost   = false;
            int  reassigned = 0;

            if (!notDecompressable.empty()) {
                // bad .. found undecompressable data
                int nd_count = notDecompressable.size();
                aw_message(GBS_global_string("Detected corrupted dictionary compression\n"
                                             "Data of %i DB-keys is lost and will be deleted", nd_count));

                aw_status(STATUS_PREFIX "deleting corrupt data");
                aw_status(0.0);
                cnt = 0;

                StringSet deletedData;
                long      deleted    = 0;
                long      notDeleted = 0;
                
                for (StringSet::iterator ki = notDecompressable.begin(); !error && ki != notDecompressable.end(); ++ki) {
                    const string& keyname    = *ki;

                    error = deleteDataOfKey(gb_main, GB_key_2_quark(gb_main, keyname.c_str()), deletedData, deleted, notDeleted);
                    aw_status(++cnt/double(nd_count));
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

            if (!error && !multiDecompressable.empty()) {
                for (StringSet::iterator ki = multiDecompressable.begin(); !error && ki != multiDecompressable.end(); ++ki) {
                    const string&   keyname  = *ki;
                    int             possible = 0;
                    vector<DictPtr> possibleDicts;

                    printf("--------------------------------------------------------------------------------\n");

                    for (Dicts::iterator di = dicts.begin(); !error && di != dicts.end(); ++di) {
                        DictPtr d = *di;
                        if (d->mayBeUsedWith(keyname) && d->canDecompress(keyname)) {
                            error = d->assignToKey(keyname);
                            if (!error) {
                                char *data = readFirstCompressedDataOf(gb_main, GB_key_2_quark(gb_main, keyname.c_str()));

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
                            selected = aw_message(question, buttons, false, NULL);
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

            // now all redundencies should be eliminated and we can assign dictionaries to affected keys
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
                if (dataLost) aw_message("We apologize for the data-loss.");
                aw_message("Dictionary bugs fixed.\n"
                           "Please save your database with a new name.");
            }
        }

    }

    Dict::gb_main = NULL;
    
    if (error) ta.abort();
    return error;
}

// --------------------------------------------------------------------------------

GB_ERROR NT_repair_DB(GBDATA *gb_main) {
    // status is already open and will be closed by caller!

    CheckedConsistencies check(gb_main);
    GB_ERROR             err = 0;

    check.perform_check("fix gene_data",     NT_fix_gene_data,     err);
    check.perform_check("fix_dict_compress", NT_fix_dict_compress, err); // do this before REF (cause 'REF' is affected)
    check.perform_check("del_mark_move_REF", NT_del_mark_move_REF, err);

    return err;
}


