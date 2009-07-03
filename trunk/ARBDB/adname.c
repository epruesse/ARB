/* ============================================================ */
/*                                                              */
/*   File      : adname.c                                       */
/*   Purpose   : species names                                  */
/*                                                              */
/*   Institute of Microbiology (Technical University Munich)    */
/*   www.arb-home.de                                            */
/*                                                              */
/* ============================================================ */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <adlocal.h>
#include <arbdbt.h>
#include <ad_config.h>

/********************************************************************************************
                Rename one or many species (only one session at a time/ uses
                commit abort transaction)
********************************************************************************************/
struct gbt_renamed_struct {
    int     used_by;
    char    data[1];

};

struct gbt_rename_struct {
    GB_HASH *renamed_hash;
    GB_HASH *old_species_hash;
    GBDATA *gb_main;
    GBDATA *gb_species_data;
    int all_flag;
} gbtrst;

GB_ERROR GBT_begin_rename_session(GBDATA *gb_main, int all_flag) {
    /* Starts a rename session.
     * If whole database shall be renamed, set 'all_flag' == 1.
     * Use GBT_abort_rename_session() or GBT_commit_rename_session() to end the session.
     */
    
    GB_ERROR error = GB_push_transaction(gb_main);
    if (!error) {
        gbtrst.gb_main         = gb_main;
        gbtrst.gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);

        if (!all_flag) { // this is meant to be used for single or few species
            int hash_size = 256;

            gbtrst.renamed_hash     = GBS_create_dynaval_hash(hash_size, GB_MIND_CASE, GBS_dynaval_free);
            gbtrst.old_species_hash = 0;
        }
        else {
            int hash_size = GBT_get_species_hash_size(gb_main);

            gbtrst.renamed_hash     = GBS_create_dynaval_hash(hash_size, GB_MIND_CASE, GBS_dynaval_free);
            gbtrst.old_species_hash = GBT_create_species_hash(gb_main);
        }
        gbtrst.all_flag = all_flag;
    }
    return error;
}

GB_ERROR GBT_rename_species(const char *oldname, const  char *newname, GB_BOOL ignore_protection)
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

    if (gbtrst.all_flag) {
        gb_assert(gbtrst.old_species_hash);
        gb_species = (GBDATA *)GBS_read_hash(gbtrst.old_species_hash,oldname);
    }
    else {
        GBDATA *gb_found_species;

        gb_assert(gbtrst.old_species_hash == 0);
        gb_found_species = GBT_find_species_rel_species_data(gbtrst.gb_species_data, newname);
        gb_species       = GBT_find_species_rel_species_data(gbtrst.gb_species_data, oldname);

        if (gb_found_species && gb_species != gb_found_species) {
            return GB_export_errorf("A species named '%s' already exists.",newname);
        }
    }

    if (!gb_species) {
        return GB_export_errorf("Expected that a species named '%s' exists (maybe there are duplicate species, database might be corrupt)",oldname);
    }

    gb_name = GB_entry(gb_species,"name");
    if (ignore_protection) GB_push_my_security(gbtrst.gb_main);
    error   = GB_write_string(gb_name,newname);
    if (ignore_protection) GB_pop_my_security(gbtrst.gb_main);

    if (!error){
        struct gbt_renamed_struct *rns;
        if (gbtrst.old_species_hash) {
            GBS_write_hash(gbtrst.old_species_hash, oldname, 0);
        }
        rns = (struct gbt_renamed_struct *)GB_calloc(strlen(newname) + sizeof (struct gbt_renamed_struct),sizeof(char));
        strcpy(&rns->data[0],newname);
        GBS_write_hash(gbtrst.renamed_hash,oldname,(long)rns);
    }
    return error;
}

static void gbt_free_rename_session_data(void) {
    if (gbtrst.renamed_hash) {
        GBS_free_hash(gbtrst.renamed_hash);
        gbtrst.renamed_hash = 0;
    }
    if (gbtrst.old_species_hash) {
        GBS_free_hash(gbtrst.old_species_hash);
        gbtrst.old_species_hash = 0;
    }
}

GB_ERROR GBT_abort_rename_session(void) {
    gbt_free_rename_session_data();
    return GB_abort_transaction(gbtrst.gb_main);
}

static const char *currentTreeName = 0;

GB_ERROR gbt_rename_tree_rek(GBT_TREE *tree,int tree_index){
    char buffer[256];
    static int counter = 0;
    if (!tree) return 0;
    if (tree->is_leaf){
        if (tree->name){
            struct gbt_renamed_struct *rns = (struct gbt_renamed_struct *)GBS_read_hash(gbtrst.renamed_hash,tree->name);
            if (rns){
                char *newname;
                if (rns->used_by == tree_index){ /* species more than once in the tree */
                    sprintf(buffer,"%s_%i", rns->data, counter++);
                    GB_warning("Species '%s' more than once in '%s', creating zombie '%s'",
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
    }else{
        gbt_rename_tree_rek(tree->leftson,tree_index);
        gbt_rename_tree_rek(tree->rightson,tree_index);
    }
    return 0;
}

#ifdef __cplusplus
extern "C"
#endif
GB_ERROR GBT_commit_rename_session(int (*show_status)(double gauge), int (*show_status_text)(const char *)){
    GB_ERROR error = 0;

    // rename species in trees
    {
        int tree_count;
        char **tree_names = GBT_get_tree_names_and_count(gbtrst.gb_main, &tree_count);

        if (tree_names) {
            int count;
            gb_assert(tree_count); // otherwise tree_names should be zero

            if (show_status_text) show_status_text(GBS_global_string("Renaming species in %i tree%c", tree_count, "s"[tree_count<2]));
            if (show_status) show_status(0.0);

            for (count = 0; count<tree_count; ++count) {
                char     *tname = tree_names[count];
                GBT_TREE *tree  = GBT_read_tree(gbtrst.gb_main,tname,-sizeof(GBT_TREE));

                if (tree) {
                    currentTreeName = tname; // provide tree name (used for error message)
                    gbt_rename_tree_rek(tree, count+1);
                    currentTreeName = 0;

                    GBT_write_tree(gbtrst.gb_main, 0, tname, tree);
                    GBT_delete_tree(tree);
                }
                if (show_status) show_status((double)(count+1)/tree_count);
            }
            GBT_free_names(tree_names);
        }
    }
    // rename configurations
    if (!error) {
        int config_count;
        char **config_names = GBT_get_configuration_names_and_count(gbtrst.gb_main, &config_count);

        if (config_names) {
            int count;
            gb_assert(config_count); // otherwise config_names should be zero

            if (show_status_text) show_status_text(GBS_global_string("Renaming species in %i config%c", config_count, "s"[config_count<2]));
            if (show_status) show_status(0.0);

            for (count = 0; !error && count<config_count; ++count) {
                GBT_config *config = GBT_load_configuration_data(gbtrst.gb_main, config_names[count], &error);
                if (!error) {
                    int need_save = 0;
                    int mode;

                    for (mode = 0; !error && mode<2; ++mode) {
                        char              **configStrPtr = (mode == 0 ? &config->top_area : &config->middle_area);
                        GBT_config_parser  *parser       = GBT_start_config_parser(*configStrPtr);
                        GBT_config_item    *item         = GBT_create_config_item();
                        void               *strstruct    = GBS_stropen(1000);

                        error = GBT_parse_next_config_item(parser, item);
                        while (!error && item->type != CI_END_OF_CONFIG) {
                            if (item->type == CI_SPECIES) {
                                struct gbt_renamed_struct *rns = (struct gbt_renamed_struct *)GBS_read_hash(gbtrst.renamed_hash, item->name);
                                if (rns) { // species was renamed
                                    freedup(item->name, rns->data);
                                    need_save = 1;
                                }
                            }
                            GBT_append_to_config_string(item, strstruct);
                            error = GBT_parse_next_config_item(parser, item);
                        }

                        if (!error) freeset(*configStrPtr, GBS_strclose(strstruct));

                        GBT_free_config_item(item);
                        GBT_free_config_parser(parser);
                    }

                    if (!error && need_save) error = GBT_save_configuration_data(config, gbtrst.gb_main, config_names[count]);
                }
                if (show_status) show_status((double)(count+1)/config_count);
            }
            GBT_free_names(config_names);
        }
    }

    // rename links in pseudo-species
    if (!error && GEN_is_genome_db(gbtrst.gb_main, -1)) {
        GBDATA *gb_pseudo;
        for (gb_pseudo = GEN_first_pseudo_species(gbtrst.gb_main);
             gb_pseudo && !error;
             gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
        {
            GBDATA *gb_origin_organism = GB_entry(gb_pseudo, "ARB_origin_species");
            if (gb_origin_organism) {
                const char                *origin = GB_read_char_pntr(gb_origin_organism);
                struct gbt_renamed_struct *rns    = (struct gbt_renamed_struct *)GBS_read_hash(gbtrst.renamed_hash, origin);
                if (rns) {          // species was renamed
                    const char *newname = &rns->data[0];
                    error               = GB_write_string(gb_origin_organism, newname);
                }
            }
        }
    }

    gbt_free_rename_session_data();

    error = GB_pop_transaction(gbtrst.gb_main);
    return error;
}

