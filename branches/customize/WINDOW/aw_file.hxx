// ================================================================ //
//                                                                  //
//   File      : aw_file.hxx                                        //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef AW_FILE_HXX
#define AW_FILE_HXX

#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif

char *AW_unfold_path(const char *pwd_envar, const char *path);
char *AW_extract_directory(const char *path);

// -----------------------------
//      file selection boxes

enum DirDisplay { MULTI_DIRS, ANY_DIR };

void AW_create_fileselection_awars(AW_root *awr, const char *awar_base, const char *directory, const char *filter, const char *file_name);

void AW_create_fileselection(AW_window *aws, const char *awar_prefix, const char *at_prefix, const  char *pwd, DirDisplay disp_dirs, bool allow_wildcards);
inline void AW_create_standard_fileselection(AW_window *aws, const char *awar_prefix) {
    AW_create_fileselection(aws, awar_prefix, "", "PWD", ANY_DIR, false);
}
void AW_refresh_fileselection(AW_root *awr, const char *awar_prefix);

char *AW_get_selected_fullname(AW_root *awr, const char *awar_prefix);
void AW_set_selected_fullname(AW_root *awr, const char *awar_prefix, const char *to_fullname);

#else
#error aw_file.hxx included twice
#endif // AW_FILE_HXX
