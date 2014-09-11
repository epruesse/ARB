// ================================================================ //
//                                                                  //
//   File      : AW_modal.cxx                                       //
//   Purpose   : Modal dialogs                                      //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <aw_window.hxx>
#include <aw_global.hxx>
#include <aw_file.hxx>
#include <aw_awar.hxx>
#include "aw_root.hxx"
#include "aw_question.hxx"
#include "aw_advice.hxx"
#include "aw_msg.hxx"
#include "aw_select.hxx"
#include "aw_help.hxx"
#include "aw_dialog.hxx"

#include <arbdbt.h>
#include <arb_strarray.h>

#include <gtk/gtk.h>

#include <deque>
#include <string>
#include <algorithm>

using namespace std;

const char *awar_string_name = "tmp/input/string";

static char *input2awar(const char *prompt, const char *awar_name, const char *title) {
    AW_awar *awar_string = AW_root::SINGLETON->awar_string(awar_name);
    aw_return_val_if_fail(awar_string != NULL, NULL);

    AW_dialog dialog(title, prompt);
    dialog.create_input_field(awar_string);
    dialog.create_buttons("Ok,-Abort");

    dialog.run();

    bool ok_pressed = dialog.get_result() == 0;
    return ok_pressed ? awar_string->read_string() : NULL;
}


/** 
 * Pop up a modal window asking the user for a string.
 * @param title          The window title.
 * @param prompt         A prompt shown above the input field.
 * @param default_input  Default text shown in the input field. May be NULL.
 * @return The entered string or NULL if Cancel was pressed.
 */
char *aw_input(const char *title, const char *prompt, const char *default_input) {
    aw_return_val_if_fail(title != NULL, NULL);
    aw_return_val_if_fail(prompt != NULL, NULL);

    AW_awar *awar_string = AW_root::SINGLETON->awar_string(awar_string_name);
    awar_string->write_string(default_input);

    return input2awar(prompt, awar_string_name, title);
}

char *aw_input(const char *prompt, const char *default_input) {
    return aw_input("Enter string", prompt, default_input);
}

// --------------------------
//      aw_file_selection

char *aw_file_selection(const char *title, const char *dir, const char *def_name, const char *suffix) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(title,
                                                    NULL,
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                                    NULL);
       
    gtk_window_set_modal(GTK_WINDOW(dialog), true);

    char *edir      = GBS_eval_env(dir);    
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), edir);
    free(edir);

    char *edef_name = GBS_eval_env(def_name);
    const char* edef_name_suffix = GBS_global_string("%s%s", edef_name, suffix);
    free(edef_name);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), edef_name_suffix);

    GtkFileFilter *filter = gtk_file_filter_new();
    const char *pattern = GBS_global_string("*%s", suffix);
    gtk_file_filter_add_pattern(filter, pattern);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    char *filename = NULL;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }
        
    gtk_widget_destroy(dialog);

    return filename;
}


