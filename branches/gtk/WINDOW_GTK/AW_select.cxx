// ============================================================= //
//                                                               //
//   File      : AW_select.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 1, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_select.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_root.hxx"
#include "aw_awar.hxx"

void AW_selection::refresh() {
    GTK_NOT_IMPLEMENTED;
}


void AW_selection_list::clear(bool clear_default/* = true*/) {
    GTK_NOT_IMPLEMENTED;
}

bool AW_selection_list::default_is_selected() const {
    GTK_NOT_IMPLEMENTED;
    return false;
}

void AW_selection_list::delete_element_at(int /*index*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::delete_value(char const */*value*/) {
    GTK_NOT_IMPLEMENTED;
}
const char *AW_selection_list::get_awar_value() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

char *AW_selection_list::get_content_as_string(long /*number_of_lines*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

const char *AW_selection_list::get_default_display() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

const char * AW_selection_list::get_default_value() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

int AW_selection_list::get_index_of(char const */*searched_value*/) {
    GTK_NOT_IMPLEMENTED;
        return 0;
}

int AW_selection_list::get_index_of_selected() {
    GTK_NOT_IMPLEMENTED;
        return 0;
}

void AW_selection_list::init_from_array(const CharPtrArray& /*entries*/, const char */*defaultEntry*/) {
    GTK_NOT_IMPLEMENTED;

}

void AW_selection_list::insert(const char */*displayed*/, const char */*value*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::insert_default(const char */*displayed*/, const char */*value*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::insert(const char */*displayed*/, int32_t /*value*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::insert_default(const char */*displayed*/, int32_t /*value*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::insert(const char */*displayed*/, GBDATA */*pointer*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::insert_default(const char */*displayed*/, GBDATA */*pointer*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_selection_list::move_content_to(AW_selection_list */*target_list*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::move_selection(int /*offset*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::select_element_at(int /*wanted_index*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::set_awar_value(char const */*new_value*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::set_file_suffix(char const */*suffix*/) {
    GTK_NOT_IMPLEMENTED;
}

size_t AW_selection_list::size() {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_selection_list::sort(bool /*backward*/, bool /*case_sensitive*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_selection_list::to_array(StrArray& /*array*/, bool /*values*/) {
    GTK_NOT_IMPLEMENTED;
}

GB_HASH *AW_selection_list::to_hash(bool /*case_sens*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_selection_list::update() {
    GTK_NOT_IMPLEMENTED;
}

char *AW_selection_list_entry::copy_string_for_display(char const */*str*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_selection_list::refresh() {
    GTK_NOT_IMPLEMENTED;
//    if (!variable_name) return;     // not connected to awar
//
//    AW_root *root  = AW_root::SINGLETON;
//    bool     found = false;
//    int      pos   = 0;
//    AW_awar *awar  = root->awar(variable_name);
//
//    AW_selection_list_entry *lt;
//
//    switch (variable_type) {
//        case AW_STRING: {
//            char *var_value = awar->read_string();
//            for (lt = list_table; lt; lt = lt->next) {
//                if (strcmp(var_value, lt->value.get_string()) == 0) {
//                    found = true;
//                    break;
//                }
//                pos++;
//            }
//            free(var_value);
//            break;
//        }
//        case AW_INT: {
//            int var_value = awar->read_int();
//            for (lt = list_table; lt; lt = lt->next) {
//                if (var_value == lt->value.get_int()) {
//                    found = true;
//                    break;
//                }
//                pos++;
//            }
//            break;
//        }
//        case AW_FLOAT: {
//            float var_value = awar->read_float();
//            for (lt = list_table; lt; lt = lt->next) {
//                if (var_value == lt->value.get_float()) {
//                    found = true;
//                    break;
//                }
//                pos++;
//            }
//            break;
//        }
//        case AW_POINTER: {
//            GBDATA *var_value = awar->read_pointer();
//            for (lt = list_table; lt; lt = lt->next) {
//                if (var_value == lt->value.get_pointer()) {
//                    found = true;
//                    break;
//                }
//                pos++;
//            }
//            break;
//        }
//        default:
//            aw_assert(0);
//            GB_warning("Unknown AWAR type");
//            break;
//    }
//
//    if (found || default_select) {
//        pos++;
//        int top;
//        int vis;
//        XtVaGetValues(select_list_widget,
//                      XmNvisibleItemCount, &vis,
//                      XmNtopItemPosition, &top,
//                      NULL);
//        XmListSelectPos(select_list_widget, pos, False);
//
//        if (pos < top) {
//            if (pos > 1) pos --;
//            XmListSetPos(select_list_widget, pos);
//        }
//        if (pos >= top + vis) {
//            XmListSetBottomPos(select_list_widget, pos + 1);
//        }
//    }
//    else {
//        GBK_terminatef("Selection list '%s' has no default selection", variable_name);
//    }
}


//AW_DB_selection
//FIXME move to own file
AW_DB_selection::AW_DB_selection(AW_selection_list *sellist_, GBDATA *gbd_)
    : AW_selection(sellist_)
    , gbd(gbd_)
{
    GTK_NOT_IMPLEMENTED;
}

AW_DB_selection::~AW_DB_selection() {
    GTK_NOT_IMPLEMENTED;
}

