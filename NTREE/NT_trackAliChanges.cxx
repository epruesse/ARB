// =============================================================== //
//                                                                 //
//   File      : NT_trackAliChanges.cxx                            //
//   Purpose   : Track alignment and sequences changes             // 
//   Time-stamp: <Fri Jun/29/2007 21:00 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <cstdlib>
#include <cstring>
#include <ctime>


#include <awt.hxx>
#include <awt_sel_boxes.hxx>
#include <arbdbt.h>

#include "NT_trackAliChanges.h"

extern GBDATA *gb_main;

#define AWAR_TRACK_BASE     "track/"
#define AWAR_TRACK_ALI      AWAR_TRACK_BASE "ali"
#define AWAR_TRACK_INITIALS AWAR_TRACK_BASE "initials"

static GB_ERROR writeHistory(GBDATA *gb_species, const char *stamp, const char *entry) {
    char     *newContent = GBS_global_string_copy("%s %s", stamp, entry);
    GB_ERROR  error      = 0;
    GBDATA   *gb_history = GB_find(gb_species, "seq_history", 0, down_level);

    if (!gb_history) {
        gb_history = GB_create(gb_species, "seq_history", GB_STRING);
        if (!gb_history) error = GB_get_error();
        else error = GB_write_string(gb_history, newContent);
    }
    else {
        char *oldContent = GB_read_string(gb_history);
        long  oldSize    = GB_read_string_count(gb_history);
        long  newSize    = strlen(newContent);
        long  size       = oldSize+1+newSize+1;
        char *content    = (char*)malloc(size);

        memcpy(content, newContent, newSize);
        content[newSize] = '\n';
        memcpy(content+newSize+1, oldContent, oldSize+1);

        error = GB_write_string(gb_history, content);

        free(content);
        free(oldContent);
    }

    free(newContent);

    return error;
}

static void trackAlignmentChanges(AW_window *aww) {
    GB_transaction  ta(gb_main);
    AW_root        *root           = aww->get_root();
    char           *ali            = root->awar(AWAR_TRACK_ALI)->read_string();
    char           *checksum_field = GBS_string_2_key(GBS_global_string("checksum_%s", ali));
    long            newSpecies     = 0;
    long            ali_changed    = 0;
    long            seq_changed    = 0;
    long            unchanged      = 0;
    long            species        = GBT_count_species(gb_main);
    long            count          = 0;
    GB_ERROR        error          = 0;
    char           *stamp;

    {
        char      *initials = root->awar(AWAR_TRACK_INITIALS)->read_string();
        char       atime[256];
        time_t     t        = time(0);
        struct tm *tms      = localtime(&t);

        strftime(atime, 255,"%Y/%m/%d %k:%M", tms);
        stamp = GBS_global_string_copy("%s %s", atime, initials);
        free(initials);
    }

    aw_openstatus(GBS_global_string("Tracking changes in '%s'", ali));

    for (GBDATA *gb_species = GBT_first_species(gb_main);
         gb_species && !error;
         gb_species = GBT_next_species(gb_species))
    {
        GBDATA *gb_seq = GBT_read_sequence(gb_species, ali);
        if (gb_seq) {
            // has data in wanted alignment
            const char *seq          = GB_read_char_pntr(gb_seq);
            long        char_cs      = GBS_checksum(seq, 0, ".-");
            long        ali_cs       = GBS_checksum(seq, 0, "");
            char       *char_entry   = GBS_global_string_copy("%lx", char_cs);
            char       *ali_entry    = GBS_global_string_copy("%lx", ali_cs);
            const char *save_comment = 0;

            GBDATA *gb_checksum = GB_find(gb_species, checksum_field, 0, down_level);
            if (!gb_checksum) {
                newSpecies++;
                gb_checksum = GB_create(gb_species, checksum_field, GB_STRING);
                if (!gb_checksum) {
                    error = GB_get_error();
                }
                else {
                    save_comment = "new";
                }
            }
            else {
                char *oldValue = GB_read_string(gb_checksum);
                if (!oldValue) {
                    error = GB_get_error();
                }
                else {
                    char *comma = strchr(oldValue, ',');
                    if (!comma) {
                        error = GBS_global_string("Invalid value '%s' in field '%s'", oldValue, checksum_field);
                    }
                    else {
                        comma[0] = 0;
                        if (strcmp(char_entry, oldValue) == 0) { // seq checksum is equal
                            if (strcmp(ali_entry, comma+1) == 0) { // ali checksum is equal
                                unchanged++;
                            }
                            else { // only alignment checksum changed
                                ali_changed++;
                                save_comment = "alignment changed";
                            }
                        }
                        else {
                            seq_changed++;
                            save_comment = "sequence changed";
                        }
                    }
                    free(oldValue);
                }
            }

            if (save_comment) {
                error             = GB_write_string(gb_checksum, GBS_global_string("%s,%s", char_entry, ali_entry));
                if (!error) error = writeHistory(gb_species, stamp, GBS_global_string("%s %s", ali, save_comment));
            }

            free(ali_entry);
            free(char_entry);
        }
        count++;
        aw_status(count/(double)species);
    }

    aw_closestatus();

    if (seq_changed) aw_message(GBS_global_string("%li species with changed sequence", seq_changed));
    if (ali_changed) aw_message(GBS_global_string("%li species with changed alignment", ali_changed));
    if (newSpecies) aw_message(GBS_global_string("%li new species", newSpecies));

    free(stamp);
    free(checksum_field);
    free(ali);
}

void NT_create_trackAliChanges_Awars(AW_root *root, AW_default properties) {
    root->awar_string(AWAR_TRACK_ALI, "???", gb_main);
    root->awar_string(AWAR_TRACK_INITIALS, GB_getenvUSER(), properties);
}

AW_window *NT_create_trackAliChanges_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "TRACK_ALI_CHANGES", "Track alignment changes");
    aws->load_xfig("trackali.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback( AW_POPUP_HELP,(AW_CL)"track_ali_changes.hlp");
    aws->create_button("HELP","HELP","H");

    aws->at("initials");
    aws->create_input_field(AWAR_TRACK_INITIALS);
    
    aws->at("ali_sel");
    awt_create_selection_list_on_ad(gb_main, aws, AWAR_TRACK_ALI, "*=");

    aws->at("go");
    aws->callback(trackAlignmentChanges);
    aws->create_autosize_button("TRACK", "Track changes", "T");

    return aws;
}


