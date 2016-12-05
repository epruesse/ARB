// ================================================================ //
//                                                                  //
//   File      : aw_edit.hxx                                        //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef AW_EDIT_HXX
#define AW_EDIT_HXX

class AW_window;
struct GBDATA;

typedef void (*aw_fileChanged_cb)(const char *path, bool fileWasChanged, bool editorTerminated);

void AW_edit(const char *path, aw_fileChanged_cb callback = 0, AW_window *aww = 0, GBDATA *gb_main = 0); // call external editor

#else
#error aw_edit.hxx included twice
#endif // AW_EDIT_HXX
