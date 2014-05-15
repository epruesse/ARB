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

const char *awar_button_name = "tmp/input/button"; // @@@ eliminate this awar (a module-global int-variable serves better)
const char *awar_string_name = "tmp/input/string";

static char *input2awar(const char *prompt, const char *awar_name, const char *title) {
    AW_awar *awar_string = AW_root::SINGLETON->awar_string(awar_name);
    aw_return_val_if_fail(awar_string != NULL, NULL);

    AW_dialog dialog;
    dialog.set_title(title);
    dialog.set_message(prompt);
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

int aw_string_selection_button() {
    AW_awar *awar_button = AW_root::SINGLETON->awar(awar_button_name);
    return awar_button->read_int();
}

/**
 * Popups up a modal window asking for a string input with 
 * a selection of defaults offered.
 * @param  title          The title of the window.
 * @param  prompt         A prompt shown above the input field.
 * @param  default_input  A default string shown in the input field (or NULL)
 * @param  value_liist    A semi-colon separated list of defaults (or NULL)
 * @param  buttons        A comma separated list of buttons (default is "Ok,Cancel").
 *                        Answer can be retrieved with aw_string_selection_button()
 * @return The string.
 */
char *aw_string_selection(const char *title, const char *prompt, const char *default_input, const char *value_list, const char *buttons) {
    aw_return_val_if_fail(title, NULL);
    aw_return_val_if_fail(prompt, NULL);

    
    AW_awar *awar_button = AW_root::SINGLETON->awar_int(awar_button_name);
    AW_awar *awar_string = AW_root::SINGLETON->awar_string(awar_string_name);
    if (default_input) {
        awar_string->write_string(default_input);
    }

    AW_dialog dialog;
    dialog.set_title(title);
    dialog.set_message(prompt);
    dialog.create_input_field(awar_string);
    AW_selection_list *slist = dialog.create_selection_list(awar_string, false);
    dialog.create_buttons(buttons ? buttons : "Ok,-Abort");
    
    slist->clear();
    if (value_list) {
        char *values = strdup(value_list);
        char *word;

        for (word = strtok(values, ";"); word; word = strtok(0, ";")) {
            slist->insert(word, word);
        }
        free(values);
    }
    slist->insert_default("<new>", "");
    slist->update();

    dialog.run();

    int buttonNr = dialog.get_result();
    awar_button->write_int(buttonNr);

    return buttonNr>=0 ? awar_string->read_string() : NULL;

    /*
    char *this_input = root->awar(AW_INPUT_AWAR)->read_string();
    if (strcmp(this_input, last_input) != 0) {
        if (check_fun) {
            char *corrected_input = check_fun(this_input);
            if (corrected_input) {
                if (strcmp(corrected_input, this_input) != 0) {
                    root->awar(AW_INPUT_AWAR)->write_string(corrected_input);
                }
                        free(corrected_input);
            }
        }
        reassign(last_input, this_input);
    }
    free(this_input);
    */
}

char *aw_string_selection2awar(const char *title, const char *prompt, const char *awar_name, const char *value_list, const char *buttons) {
    // params see aw_string_selection
    //
    // default for string is taken from awar 'awar_name'.
    // result is written back to awar.
    // if abort button is pressed, old value is written back to awar.

    AW_root *aw_root   = AW_root::SINGLETON;
    AW_awar *awar      = aw_root->awar(awar_name);
    char    *old_value = awar->read_string();
    char    *result    = aw_string_selection(title, prompt, old_value, value_list, buttons);

    awar->write_string(result ? result : old_value);
    free(old_value);

    return result;
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


