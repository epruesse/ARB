#include "aw_window.hxx"
#include "aw_dialog.hxx"
#include "aw_awar.hxx"
#include "aw_select.hxx"
#include "aw_assert.hxx"
#include "gtk/gtk.h"

struct AW_dialog::Pimpl {
    GtkDialog *dialog;
    GtkLabel  *label;
    AW_selection_list *slist;
    int result;
    int exit_button;
    Pimpl() 
        : dialog(NULL), label(NULL), slist(NULL), result(0), exit_button(-1)
    {}
};
    
AW_dialog::AW_dialog() 
  : prvt(new Pimpl)
{
    prvt->dialog = GTK_DIALOG(gtk_dialog_new());
    gtk_window_set_position(GTK_WINDOW(prvt->dialog), GTK_WIN_POS_MOUSE);
    gtk_window_set_modal(GTK_WINDOW(prvt->dialog), true);
}

AW_dialog::~AW_dialog() {
    delete prvt->slist;
    gtk_widget_destroy(GTK_WIDGET(prvt->dialog));
    delete prvt;
}

void AW_dialog::run() {
    gtk_widget_show_all(GTK_WIDGET(prvt->dialog));
    prvt->result = gtk_dialog_run(prvt->dialog);
    gtk_widget_hide(GTK_WIDGET(prvt->dialog));
    
    if (prvt->result == prvt->exit_button) {
        exit(EXIT_FAILURE);
    }
}

void AW_dialog::set_title(const char* title) {
    aw_return_if_fail(title != NULL);

    gtk_window_set_title(GTK_WINDOW(prvt->dialog), title);
}

void AW_dialog::set_message(const char* text) {
    aw_return_if_fail(text != NULL);

    if (!prvt->label) {
        prvt->label = GTK_LABEL(gtk_label_new(text));
        GtkWidget* content = gtk_dialog_get_content_area(prvt->dialog);
        gtk_container_add(GTK_CONTAINER(content), GTK_WIDGET(prvt->label));
    } else {
        gtk_label_set_text(prvt->label, text);
    }
}

void AW_dialog::create_buttons(const char* buttons_) {
    aw_return_if_fail(buttons_ != NULL);

    // create buttons from descriptor string
    int   num_buttons = 0;
    char *buttons = strdup(buttons_);
    char *saveptr, *button = strtok_r(buttons, ",", &saveptr);
    int   exit_button = -1;
    do {
        if (button[0] == '^' || button[0] == '\n') {
            // not sure how to do this
            button++;
        }
        if (strcmp(button, "EXIT") == 0) {
            prvt->exit_button = num_buttons;
        }
        gtk_dialog_add_button(prvt->dialog, button, num_buttons++);
    } while ( (button = strtok_r(NULL, ",", &saveptr)) );
    free(buttons);
}

void AW_dialog::create_input_field(AW_awar* awar) {
    GtkWidget *entry = gtk_entry_new();
    awar->bind_value(G_OBJECT(entry), "text");

    GtkWidget* content = gtk_dialog_get_content_area(prvt->dialog);
    gtk_container_add(GTK_CONTAINER(content), entry);
}

void AW_dialog::create_toggle(AW_awar* awar, const char *label) {
    GtkWidget *toggle = gtk_check_button_new_with_mnemonic(label);
    awar->bind_value(G_OBJECT(toggle), "active");
    
    GtkWidget* content = gtk_dialog_get_content_area(prvt->dialog);
    gtk_container_add(GTK_CONTAINER(content), toggle);
}

AW_selection_list* AW_dialog::create_selection_list(AW_awar* awar) {
    GtkWidget *tree = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
    
    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_win), tree);

    GtkWidget* content = gtk_dialog_get_content_area(prvt->dialog);
    gtk_container_add(GTK_CONTAINER(content), scrolled_win);

    prvt->slist = new AW_selection_list(awar);
    prvt->slist->bind_widget(tree);
    return prvt->slist;
}

int AW_dialog::get_result() {
    return prvt->result;
}
