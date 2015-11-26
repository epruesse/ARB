// =============================================================== //
//                                                                 //
//   File      : adname.cxx                                        //
//   Purpose   : species names                                     //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_local.h"

#include <ad_config.h>
#include "TreeNode.h"

#include <arb_progress.h>
#include <arb_strbuf.h>
#include <arb_strarray.h>
#include <arb_file.h>
#include <arb_diff.h>

#include <cctype>
#include "ad_colorset.h"

struct gbt_renamed {
    int     used_by;
    char    data[1];
};

static struct {
    GB_HASH *renamed_hash;
    GB_HASH *old_species_hash;
    GBDATA  *gb_main;
    GBDATA  *gb_species_data;
    int      all_flag;
} NameSession;

#if defined(WARN_TODO)
#warning change all_flag into estimated number of renames ( == 0 shall mean all)
#endif

GB_ERROR GBT_begin_rename_session(GBDATA *gb_main, int all_flag) {
    /* Starts a rename session (to rename one or many species)
     * all_flag == 1 -> rename all species in DB
     * Call GBT_abort_rename_session() or GBT_commit_rename_session() to close the session.
     */

    GB_ERROR error = GB_push_transaction(gb_main);
    if (!error) {
        NameSession.gb_main         = gb_main;
        NameSession.gb_species_data = GBT_get_species_data(gb_main);

        if (!all_flag) { // this is meant to be used for single or few species
            int hash_size = 128;

            NameSession.renamed_hash     = GBS_create_dynaval_hash(hash_size, GB_MIND_CASE, GBS_dynaval_free);
            NameSession.old_species_hash = 0;
        }
        else {
            NameSession.renamed_hash     = GBS_create_dynaval_hash(GBT_get_species_count(gb_main), GB_MIND_CASE, GBS_dynaval_free);
            NameSession.old_species_hash = GBT_create_species_hash(gb_main);
        }
        NameSession.all_flag = all_flag;
    }
    return error;
}

GB_ERROR GBT_rename_species(const char *oldname, const  char *newname, bool ignore_protection)
{
    GBDATA   *gb_species;
    GBDATA   *gb_name;
    GB_ERROR  error;

    if (strcmp(oldname, newname) == 0)
        return 0;

#if defined(DEBUG) && 1
    if (isdigit(oldname[0])) {
        printf("oldname='%s' newname='%s'\n", oldname, newname);
    }
#endif

    if (NameSession.all_flag) {
        gb_assert(NameSession.old_species_hash);
        gb_species = (GBDATA *)GBS_read_hash(NameSession.old_species_hash, oldname);
    }
    else {
        GBDATA *gb_found_species;

        gb_assert(NameSession.old_species_hash == 0);
        gb_found_species = GBT_find_species_rel_species_data(NameSession.gb_species_data, newname);
        gb_species       = GBT_find_species_rel_species_data(NameSession.gb_species_data, oldname);

        if (gb_found_species && gb_species != gb_found_species) {
            return GBS_global_string("A species named '%s' already exists.", newname);
        }
    }

    if (!gb_species) {
        return GBS_global_string("Expected that a species named '%s' exists (maybe there are duplicate species, database might be corrupt)", oldname);
    }

    gb_name = GB_entry(gb_species, "name");
    if (ignore_protection) GB_push_my_security(NameSession.gb_main);
    error   = GB_write_string(gb_name, newname);
    if (ignore_protection) GB_pop_my_security(NameSession.gb_main);

    if (!error) {
        if (NameSession.old_species_hash) {
            GBS_write_hash(NameSession.old_species_hash, oldname, 0);
        }
        gbt_renamed *rns = (gbt_renamed *)GB_calloc(strlen(newname) + sizeof (gbt_renamed), sizeof(char));
        strcpy(&rns->data[0], newname);
        GBS_write_hash(NameSession.renamed_hash, oldname, (long)rns);
    }
    return error;
}

static void gbt_free_rename_session_data() {
    if (NameSession.renamed_hash) {
        GBS_free_hash(NameSession.renamed_hash);
        NameSession.renamed_hash = 0;
    }
    if (NameSession.old_species_hash) {
        GBS_free_hash(NameSession.old_species_hash);
        NameSession.old_species_hash = 0;
    }
}

GB_ERROR GBT_abort_rename_session() {
    gbt_free_rename_session_data();
    return GB_abort_transaction(NameSession.gb_main);
}

static const char *currentTreeName = 0;

static GB_ERROR gbt_rename_tree_rek(TreeNode *tree, int tree_index) {
    if (tree) {
        if (tree->is_leaf) {
            if (tree->name) {
                gbt_renamed *rns = (gbt_renamed *)GBS_read_hash(NameSession.renamed_hash, tree->name);
                if (rns) {
                    char *newname;
                    if (rns->used_by == tree_index) { // species more than once in the tree
                        static int counter = 0;
                        char       buffer[256];

                        sprintf(buffer, "%s_%i", rns->data, counter++);
                        GB_warningf("Species '%s' more than once in '%s', creating zombie '%s'",
                                    tree->name, currentTreeName, buffer);
                        newname = buffer;
                    }
                    else {
                        newname = &rns->data[0];
                    }
                    freedup(tree->name, newname);
                    rns->used_by = tree_index;
                }
            }
        }
        else {
            gbt_rename_tree_rek(tree->get_leftson(), tree_index);
            gbt_rename_tree_rek(tree->get_rightson(), tree_index);
        }
    }
    return NULL;
}

GB_ERROR GBT_commit_rename_session() { // goes to header: __ATTR__USERESULT
    bool         is_genome_db = GEN_is_genome_db(NameSession.gb_main, -1);
    arb_progress commit_progress("Correcting name references", 3+is_genome_db);
    GB_ERROR     error        = 0;

    commit_progress.allow_title_reuse();

    // rename species in trees
    {
        ConstStrArray tree_names;
        GBT_get_tree_names(tree_names, NameSession.gb_main, false);

        if (!tree_names.empty()) {
            int          tree_count = tree_names.size();
            arb_progress progress(GBS_global_string("Correcting names in %i tree%c", tree_count, "s"[tree_count<2]),
                                  tree_count*3);

            for (int count = 0; count<tree_count && !error; ++count) {
                const char *tname = tree_names[count];
                TreeNode   *tree  = GBT_read_tree(NameSession.gb_main, tname, new SimpleRoot);
                ++progress;

                if (tree) {
                    currentTreeName = tname; // provide tree name (used for error message)
                    gbt_rename_tree_rek(tree, count+1);
                    currentTreeName = 0;

                    ++progress;

                    GBT_write_tree(NameSession.gb_main, tname, tree);
                    destroy(tree);

                    progress.inc_and_check_user_abort(error);
                }
                else {
                    GBT_message(NameSession.gb_main, GBS_global_string("Warning: failed to read '%s' (Reason: %s)", tname, GB_await_error()));
                    ++progress;
                    ++progress;
                }
            }
        }
        commit_progress.inc_and_check_user_abort(error);
    }
    // rename configurations
    if (!error) {
        ConstStrArray config_names;
        GBT_get_configuration_names(config_names, NameSession.gb_main);

        if (!config_names.empty()) {
            int          config_count = config_names.size();
            arb_progress progress(GBS_global_string("Correcting names in %i config%c", config_count, "s"[config_count<2]), config_count);

            for (int count = 0; !error && count<config_count; ++count) {
                GBT_config config(NameSession.gb_main, config_names[count], error);
                if (!error) {
                    int need_save = 0;
                    for (int area = 0; !error && area<2; ++area) {
                        GBT_config_parser   parser(config, area);
                        GBS_strstruct      *strstruct    = GBS_stropen(1000);

                        while (1) {
                            const GBT_config_item& item = parser.nextItem(error);
                            if (error || item.type == CI_END_OF_CONFIG) break;

                            if (item.type == CI_SPECIES) {
                                gbt_renamed *rns = (gbt_renamed *)GBS_read_hash(NameSession.renamed_hash, item.name);
                                if (rns) { // species was renamed
                                    GBT_append_to_config_string(GBT_config_item(CI_SPECIES, rns->data), strstruct);
                                    need_save = 1;
                                    continue;
                                }
                            }
                            GBT_append_to_config_string(item, strstruct);
                        }

                        if (!error) {
                            config.set_definition(area, GBS_strclose(strstruct));
                        }
                        else {
                            error = GBS_global_string("Failed to parse configuration '%s' (Reason: %s)", config_names[count], error);
                            GBS_strforget(strstruct);
                        }
                    }

                    if (!error && need_save) {
                        error = config.save(NameSession.gb_main, config_names[count], false);
                    }
                }
                progress.inc_and_check_user_abort(error);
            }
        }
        commit_progress.inc_and_check_user_abort(error);
    }

    // rename species in saved colorsets
    if (!error) {
        GBDATA *gb_species_colorset_root = GBT_colorset_root(NameSession.gb_main, "species");
        if (gb_species_colorset_root) {
            ConstStrArray colorset_names;
            GBT_get_colorset_names(colorset_names, gb_species_colorset_root);

            int colorset_count = colorset_names.size();
            if (colorset_count>0) {
                arb_progress progress(GBS_global_string("Correcting names in %i colorset%c", colorset_count, "s"[colorset_count<2]), colorset_count);

                for (int c = 0; c<colorset_count && !error; ++c) {
                    GBDATA *gb_colorset     = GBT_find_colorset(gb_species_colorset_root, colorset_names[c]);
                    if (!gb_colorset) error = GB_await_error();
                    else {
                        ConstStrArray colorDefs;
                        error = GBT_load_colorset(gb_colorset, colorDefs);
                        if (!error) {
                            StrArray modifiedDefs;
                            bool     changed = false;

                            for (int d = colorDefs.size()-1; d>=0; --d) {
                                const char *def   = colorDefs[d];
                                const char *equal = strchr(def, '=');

                                if (equal) { // only handle correct entries (do not touch rest)
                                    if (strcmp(equal+1, "0") == 0) { // unneeded "no color"-entry (see [14094])
                                        colorDefs.remove(d);
                                        changed = true;
                                    }
                                    else {
                                        gbt_renamed *rns;
                                        {
                                            LocallyModify<char>  tempSplit(const_cast<char*>(equal)[0], 0);
                                            rns = (gbt_renamed *)GBS_read_hash(NameSession.renamed_hash, def);
                                        }
                                        if (rns) { // species was renamed
                                            char *newDef = GBS_global_string_copy("%s%s", rns->data, equal);
                                            colorDefs.replace(d, newDef); // replace colorDefs
                                            modifiedDefs.put(newDef);     // keep heapcopy until colorDefs gets written

                                            changed = true;
                                        }
                                    }
                                }
                            }

                            if (changed && !error) error = GBT_save_colorset(gb_colorset, colorDefs);
                        }
                    }
                    progress.inc_and_check_user_abort(error);
                }
            }
        }
        commit_progress.inc_and_check_user_abort(error);
    }

    // rename links in pseudo-species
    if (!error && is_genome_db) {
        {
            arb_progress progress("Correcting names of organism references");

            GBDATA *gb_pseudo;
            for (gb_pseudo = GEN_first_pseudo_species(NameSession.gb_main);
                 gb_pseudo && !error;
                 gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
            {
                GBDATA *gb_origin_organism = GB_entry(gb_pseudo, "ARB_origin_species");
                if (gb_origin_organism) {
                    const char  *origin = GB_read_char_pntr(gb_origin_organism);
                    gbt_renamed *rns    = (gbt_renamed *)GBS_read_hash(NameSession.renamed_hash, origin);
                    if (rns) {          // species was renamed
                        const char *newname = &rns->data[0];
                        error               = GB_write_string(gb_origin_organism, newname);
                    }
                }
            }
        }
        commit_progress.inc_and_check_user_abort(error);
    }

    gbt_free_rename_session_data();

    error = GB_end_transaction(NameSession.gb_main, error);
    return error;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

// #define TEST_AUTO_UPDATE // uncomment to auto-update test result db

void TEST_SLOW_rename_session() {
    const char *inputname    = "TEST_opti_ascii_in.arb";
    const char *outputname   = "TEST_opti_ascii_renamed.arb";
    const char *expectedname = "TEST_opti_ascii_renamed_expected.arb";

    {
        GB_shell  shell;
        GBDATA   *gb_main;
        TEST_EXPECT_RESULT__NOERROREXPORTED(gb_main = GB_open(inputname, "rw"));

        for (int session = 1; session <= 2; ++session) {
            TEST_ANNOTATE(GBS_global_string("session=%i", session));

            TEST_EXPECT_NO_ERROR(GBT_begin_rename_session(gb_main, 0));
            if (session == 2) { // session 1 tests renaming nothing
                // only in config 'some':
                TEST_EXPECT_NO_ERROR(GBT_rename_species("FrnPhilo", "olihPnrF", true));
                TEST_EXPECT_NO_ERROR(GBT_rename_species("DsfDesul", "luseDfsD", true));
                // also in config 'other':
                TEST_EXPECT_NO_ERROR(GBT_rename_species("CalSacch", "hccaSlaC", true));
                TEST_EXPECT_NO_ERROR(GBT_rename_species("LacReute", "etueRcaL", true));
            }
            TEST_EXPECT_NO_ERROR(GBT_commit_rename_session());
        }

        TEST_EXPECT_NO_ERROR(GB_save_as(gb_main, outputname, "a"));
        GB_close(gb_main);
    }

#if defined(TEST_AUTO_UPDATE)
    TEST_COPY_FILE(outputname, expectedname);
#endif
    TEST_EXPECT_TEXTFILE_DIFFLINES(outputname, expectedname, 0);
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(outputname));
}
TEST_PUBLISH(TEST_SLOW_rename_session);

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
