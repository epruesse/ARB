#include "aw_window.hxx"
#include "aw_dialog.hxx"
#include "aw_awar.hxx"
#include "aw_select.hxx"
#include "aw_assert.hxx"
#include "gtk/gtk.h"
#include "aw_gtk_forward_declarations.hxx"

struct AW_dialog::Pimpl {
    GtkDialog *dialog;
    GtkLabel  *label;

    AW_selection_list *slist;

    int result;
    int exit_button;
    int abort_button;

    AW_awar* selection_awar;
    AW_awar* input_awar;

    Pimpl()
        : dialog(NULL),
          label(NULL),
          slist(NULL),
          result(-1),
          exit_button(-1),
          abort_button(-1),
          selection_awar(NULL),
          input_awar(NULL)
    {}
};

AW_dialog::AW_dialog(const char *title, const char *prompt)
    : prvt(new Pimpl)
{
    prvt->dialog = GTK_DIALOG(gtk_dialog_new());
    gtk_window_set_position(GTK_WINDOW(prvt->dialog), GTK_WIN_POS_MOUSE);
    gtk_window_set_modal(GTK_WINDOW(prvt->dialog), true);
    gtk_window_set_deletable(GTK_WINDOW(prvt->dialog), false);
    gtk_container_set_border_width(GTK_CONTAINER(prvt->dialog), 5); // @@@STYLE

    set_title(title);
    set_message(prompt);
}

AW_dialog::~AW_dialog() {
    delete prvt->slist;
    gtk_widget_destroy(GTK_WIDGET(prvt->dialog));
    delete prvt;
}

void AW_dialog::run() {
    aw_assert(AW_root::SINGLETON->forbid_dialogs == false);
    LocallyModify<bool> flag(AW_root::SINGLETON->delay_timer_callbacks, true);

    gtk_widget_show_all(GTK_WIDGET(prvt->dialog));
    prvt->result = gtk_dialog_run(prvt->dialog);
    gtk_widget_hide(GTK_WIDGET(prvt->dialog));

    if (prvt->result == prvt->exit_button) {
        exit(EXIT_FAILURE);
    }
    if (prvt->result == prvt->abort_button && prvt->abort_button>=0) {
        prvt->result = -1;
    }

    aw_assert(AW_root::SINGLETON->forbid_dialogs == false);
}

void AW_dialog::set_title(const char* title) {
    aw_assert(title);
    gtk_window_set_title(GTK_WINDOW(prvt->dialog), title);
}

void AW_dialog::set_message(const char* text) {
    aw_assert(text);

    if (!prvt->label) {
        prvt->label = GTK_LABEL(gtk_label_new(text));
        GtkWidget* content = gtk_dialog_get_content_area(prvt->dialog);
        GtkWidget* align = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);//align left
        gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(prvt->label));
        gtk_box_pack_start(GTK_BOX(content), align, false, false, 1);
    } else {
        gtk_label_set_text(prvt->label, text);
    }
}

void AW_dialog::create_buttons(const char* buttons_) {
    aw_assert(buttons_);

    // create buttons from descriptor string
    int   num_buttons = 0;
    char *buttons     = ARB_strdup(buttons_);
    char *saveptr;
    char *button      = strtok_r(buttons, ",", &saveptr);

    do {
        if (button[0] == '-') { // abort button
            aw_assert(prvt->abort_button<0); // only one abort button is allowed
            prvt->abort_button = num_buttons;
            button++;
        }
        if (button[0] == '^' || button[0] == '\n') {
            // not sure how to do this
            button++;
        }
        if (strcmp(button, "EXIT") == 0) {
            prvt->exit_button = num_buttons;
        }
        gtk_dialog_add_button(prvt->dialog, button, num_buttons++);
    }
    while ( (button = strtok_r(NULL, ",", &saveptr)) );

    free(buttons);
}

void AW_dialog::create_input_field(AW_awar* awar) {
    aw_return_if_fail(awar);
    
    GtkWidget *entry = gtk_entry_new();
    awar->bind_value(G_OBJECT(entry), "text");

    GtkWidget* content = gtk_dialog_get_content_area(prvt->dialog);
    gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(entry), false, false, 7);
    prvt->input_awar = awar;
}

void AW_dialog::create_toggle(AW_awar* awar, const char *label) {
    GtkWidget *toggle = gtk_check_button_new_with_mnemonic(label);
    awar->bind_value(G_OBJECT(toggle), "active");
    
    GtkWidget* content = gtk_dialog_get_content_area(prvt->dialog);
    gtk_container_add(GTK_CONTAINER(content), toggle);
}

AW_selection_list* AW_dialog::create_selection_list(AW_awar *awar, bool fallback2default) {
    GtkWidget *tree = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
    
    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_win), tree);

    GtkWidget* content = gtk_dialog_get_content_area(prvt->dialog);
    gtk_container_add(GTK_CONTAINER(content), scrolled_win);

    prvt->slist = new AW_selection_list(awar, fallback2default);
    prvt->slist->bind_widget(tree);
    prvt->selection_awar = awar;
    return prvt->slist;
}

int AW_dialog::get_result() {
    return prvt->result;
}

