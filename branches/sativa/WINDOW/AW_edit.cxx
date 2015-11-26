// ================================================================ //
//                                                                  //
//   File      : AW_edit.cxx                                        //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "aw_edit.hxx"
#include "aw_window.hxx"
#include "aw_root.hxx"
#include "aw_msg.hxx"

#include <arbdbt.h>

#include <sys/stat.h>


struct fileChanged_cb_data : virtual Noncopyable {
    char              *fpath;                       // full name of edited file
    int                lastModtime;                 // last known modification time of 'fpath'
    bool               editorTerminated;            // do not free before this has been set to 'true'
    aw_fileChanged_cb  callback;

    fileChanged_cb_data(char **fpath_ptr, aw_fileChanged_cb cb) {
        fpath            = *fpath_ptr;
        *fpath_ptr       = 0;   // take ownage
        lastModtime      = getModtime();
        editorTerminated = false;
        callback         = cb;
    }

    ~fileChanged_cb_data() {
        free(fpath);
    }

    int getModtime() {
        struct stat st;
        if (stat(fpath, &st) == 0) return st.st_mtime;
        return 0;
    }

    bool fileWasChanged() {
        int  modtime = getModtime();
        bool changed = modtime != lastModtime;
        lastModtime  = modtime;
        return changed;
    }
};

static void editor_terminated_cb(const char *IF_DEBUG(message), void *cb_data) {
    fileChanged_cb_data *data = (fileChanged_cb_data*)cb_data;

#if defined(DEBUG)
    printf("editor_terminated_cb: message='%s' fpath='%s'\n", message, data->fpath);
#endif // DEBUG

    data->callback(data->fpath, data->fileWasChanged(), true);
    data->editorTerminated = true; // trigger removal of check_file_changed_cb
}

#define AWT_CHECK_FILE_TIMER 700 // in ms

static unsigned check_file_changed_cb(AW_root *, fileChanged_cb_data *data) {
    unsigned restart = 0;
    if (data->editorTerminated) {
        delete data;
    }
    else {
        bool changed = data->fileWasChanged();

        if (changed) data->callback(data->fpath, true, false);
        restart = AWT_CHECK_FILE_TIMER;
    }
    return restart;
}

void AW_edit(const char *path, aw_fileChanged_cb callback, AW_window *aww, GBDATA *gb_main) {
    // Start external editor on file 'path' (asynchronously)
    // 
    // if 'callback' is specified, it is called everytime the file is changed
    // and once after the editor terminated.
    //
    // 'aww' and 'gb_main' have to passed if a callback is specified!

    const char          *editor  = GB_getenvARB_TEXTEDIT();
    char                *fpath   = GBS_eval_env(path);
    char                *command = 0;
    fileChanged_cb_data *cb_data = 0;
    GB_ERROR             error   = 0;

    if (callback) {
        aw_assert(aww);
        aw_assert(gb_main);

        cb_data = new fileChanged_cb_data(&fpath, callback); // fpath now is 0 and belongs to cb_data

        char *arb_notify       = GB_generate_notification(gb_main, editor_terminated_cb, "editor terminated", (void*)cb_data);
        if (!arb_notify) error = GB_await_error();
        else {
            char *arb_message = GBS_global_string_copy("arb_message \"Could not start editor '%s'\"", editor);
            char *quotedFile  = GBK_singlequote(cb_data->fpath);

            command = GBS_global_string_copy("((%s %s || %s); %s)&", editor, quotedFile, arb_message, arb_notify);
            free(quotedFile);
            free(arb_message);
            free(arb_notify);
        }
    }
    else {
        char *quotedFile  = GBK_singlequote(fpath);
        command = GBS_global_string_copy("%s %s &", editor, quotedFile);
        free(quotedFile);
    }

    if (command) {
        aw_assert(!error);
        error = GBK_system(command);
        if (error) {
            aw_message(error); error = NULL;
            if (callback) error = GB_remove_last_notification(gb_main);
        }
        else { // successfully started editor
            // Can't be sure editor really started when callback is used (see command above).
            // But it doesn't matter, cause arb_notify is called anyway and removes all callbacks
            if (callback) {
                // add timed callback tracking file change
                AW_root *aw_root = aww->get_root();
                aw_root->add_timed_callback(AWT_CHECK_FILE_TIMER, makeTimedCallback(check_file_changed_cb, cb_data));
                cb_data          = 0; // now belongs to check_file_changed_cb
            }
        }
    }

    if (error) aw_message(error);

    free(command);
    delete cb_data;
    free(fpath);
}

