// ============================================================= //
//                                                               //
//   File      : AW_file.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 2, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_file.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include <cstring>
#include "aw_awar.hxx"
#include "aw_root.hxx"
#ifndef ARBDB_H
#include <arbdb.h>
#endif
#include <arb_file.h>
#include "aw_msg.hxx"
#include "aw_question.hxx"

char *AW_unfold_path(const char */*pwd_envar*/, const char */*path*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

char *AW_extract_directory(const char */*path*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

// -----------------------------
//      file selection boxes

void AW_create_fileselection_awars(AW_root *awr, const char *awar_base,
                                   const char *directory, const char *filter, const char *file_name,
                                   AW_default default_file, bool resetValues)
{
    int   base_len  = strlen(awar_base);
    bool  has_slash = awar_base[base_len-1] == '/';
    char *awar_name = new char[base_len+30]; // use private buffer, because caller will most likely use GBS_global_string for arguments

    sprintf(awar_name, "%s%s", awar_base, "/directory"+int(has_slash));
    AW_awar *awar_dir = awr->awar_string(awar_name, directory, default_file);

    sprintf(awar_name, "%s%s", awar_base, "/filter"   + int(has_slash));
    AW_awar *awar_filter = awr->awar_string(awar_name, filter, default_file);

    sprintf(awar_name, "%s%s", awar_base, "/file_name"+int(has_slash));
    AW_awar *awar_filename = awr->awar_string(awar_name, file_name, default_file);

    if (resetValues) {
        awar_dir->write_string(directory);
        awar_filter->write_string(filter);
        awar_filename->write_string(file_name);
    }
    else {
        char *stored_directory = awar_dir->read_string();
#if defined(DEBUG)
        if (strncmp(awar_base, "tmp/", 4) == 0) { // non-saved awar
            if (directory[0] != 0) { // accept empty dir (means : use current ? )
                aw_assert(GB_is_directory(directory)); // default directory does not exist
            }
        }
#endif // DEBUG

        if (strcmp(stored_directory, directory) != 0) { // does not have default value
#if defined(DEBUG)
            const char *arbhome    = GB_getenvARBHOME();
            int         arbhomelen = strlen(arbhome);

            if (strncmp(directory, arbhome, arbhomelen) == 0) { // default points into $ARBHOME
                aw_assert(resetValues); // should be called with resetValues == true
                // otherwise it's possible, that locations from previously installed ARB versions are used
            }
#endif // DEBUG

            if (!GB_is_directory(stored_directory)) {
                awar_dir->write_string(directory);
                fprintf(stderr,
                        "Warning: Replaced reference to non-existing directory '%s'\n"
                        "         by '%s'\n"
                        "         (Save properties to make this change permanent)\n",
                        stored_directory, directory);
            }
        }

        free(stored_directory);
    }

    char *dir = awar_dir->read_string();
    if (dir[0] && !GB_is_directory(dir)) {
        if (aw_ask_sure("create_directory", GBS_global_string("Directory '%s' does not exist. Create?", dir))) {
            GB_ERROR error = GB_create_directory(dir);
            if (error) aw_message(GBS_global_string("Failed to create directory '%s' (Reason: %s)", dir, error));
        }
    }
    free(dir);

    delete [] awar_name;
}


void AW_create_fileselection(AW_window */*aws*/, const char */*awar_prefix*/, const char */*at_prefix*/ /* = ""*/, const char */*pwd*/ /*= "PWD"*/, bool /*show_dir*/ /*= true*/, bool /*allow_wildcards*/ /* = false*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_refresh_fileselection(AW_root */*awr*/, const char */*awar_prefix*/) {
    GTK_NOT_IMPLEMENTED;
}


char *AW_get_selected_fullname(AW_root */*awr*/, const char */*awar_prefix*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_set_selected_fullname(AW_root */*awr*/, const char */*awar_prefix*/, const char */*to_fullname*/) {
    GTK_NOT_IMPLEMENTED;
}




