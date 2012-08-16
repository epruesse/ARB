// ============================================================= //
//                                                               //
//   File      : Aw_window.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Jul 26, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_gtk_migration_helpers.hxx"
#include "aw_window.hxx"
#include "aw_window_gtk.hxx"
#include "aw_xfig.hxx"
#include "aw_root.hxx"
#include "aw_device.hxx"
#include "aw_at.hxx"
#include <arbdb.h>

void AW_clock_cursor(AW_root *) {
    GTK_NOT_IMPLEMENTED;
}

void AW_normal_cursor(AW_root *) {
    GTK_NOT_IMPLEMENTED;
}

void AW_help_entry_pressed(AW_window *) {
    GTK_NOT_IMPLEMENTED;
}

void AW_POPDOWN(AW_window *){
    GTK_NOT_IMPLEMENTED;
}

void AW_POPUP_HELP(AW_window *, AW_CL /* char */ /*helpfile*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_openURL(AW_root * /*aw_root*/, const char * /*url*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_window::recalc_pos_atShow(AW_PosRecalc /*pr*/){
    GTK_NOT_IMPLEMENTED;
}

AW_PosRecalc AW_window::get_recalc_pos_atShow() const {
    GTK_NOT_IMPLEMENTED;
    return AW_KEEP_POS; // 0
}

void AW_window::recalc_size_atShow(enum AW_SizeRecalc /*sr*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::on_hide(aw_hide_cb /*call_on_hide*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::at(int /*x*/, int /*y*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::at_x(int /*x*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::at_y(int /*y*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::at_shift(int /*x*/, int /*y*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::at_newline(){
    GTK_NOT_IMPLEMENTED;
}


void AW_window::at(const char * /*id*/){
    GTK_NOT_IMPLEMENTED;
}



bool AW_window::at_ifdef(const  char * /*id*/){
    GTK_NOT_IMPLEMENTED;
    return false;
}


void AW_window::at_set_to(bool /*attach_x*/, bool /*attach_y*/, int /*xoff*/, int /*yoff*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::at_unset_to(){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::at_set_min_size(int /*xmin*/, int /*ymin*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::auto_space(int /*xspace*/, int /*yspace*/){
    GTK_NOT_IMPLEMENTED;
}


void AW_window::label_length(int /*length*/){
    GTK_NOT_IMPLEMENTED;
}
void AW_window::button_length(int /*length*/){
    GTK_NOT_IMPLEMENTED;
}

int  AW_window::get_button_length() const{
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_window::calculate_scrollbars(){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::callback(void (*/*f*/)(AW_window*)){
    GTK_NOT_IMPLEMENTED;
}


void AW_window::callback(void (*/*f*/)(AW_window*, AW_CL), AW_CL /*cd1*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::callback(AW_cb_struct * /* owner */ /*awcbs*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_window::close_sub_menu(){
    GTK_NOT_IMPLEMENTED;
}



void AW_window::_get_area_size(AW_area /*area*/, AW_screen_area */*square*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_window::clear_option_menu(AW_option_menu_struct */*oms*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::create_button(const char */*macro_name*/, AW_label /*label*/, const char */*mnemonic*/ /* = 0 */, const char */*color*/ /* = 0 */){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::create_autosize_button(const char */*macro_name*/, AW_label /*label*/, const char */*mnemonic*/ /* = 0*/, unsigned /*xtraSpace*/ /* = 1 */){

}
Widget AW_window::get_last_widget() const{
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_window::create_toggle(const char */*awar_name*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::create_toggle(const char */*awar_name*/, const char */*nobitmap*/, const char */*yesbitmap*/, int /*buttonWidth*/ /* = 0 */){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::create_text_toggle(const char */*var_name*/, const char */*noText*/, const char */*yesText*/, int /*buttonWidth*/ /*= 0*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_window::create_input_field(const char */*awar_name*/, int /*columns*/ /* = 0 */){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::create_text_field(const char */*awar_name*/, int /*columns*/ /* = 20 */, int /*rows*/ /*= 4*/){
    GTK_NOT_IMPLEMENTED;
}



void AW_window::create_menu(AW_label /*name*/, const char */*mnemonic*/, AW_active /*mask*/ /*= AWM_ALL*/){
    GTK_NOT_IMPLEMENTED;
}

int AW_window::create_mode(const char */*pixmap*/, const char */*help_text*/, AW_active /*mask*/, void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}


AW_option_menu_struct *AW_window::create_option_menu(const char */*awar_name*/, AW_label /*label*/ /*= 0*/, const char */*mnemonic*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_selection_list *AW_window::create_selection_list(const char */*awar_name*/, int /*columns*/ /*= 4*/, int /*rows*/ /*= 4*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}



void AW_window::create_toggle_field(const char */*awar_name*/, int /*orientation*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::create_toggle_field(const char */*awar_name*/, AW_label /*label*/, const char */*mnemonic*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_window::d_callback(void (*/*f*/)(AW_window*, AW_CL), AW_CL /*cd1*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::d_callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/){
    GTK_NOT_IMPLEMENTED;
}


AW_pos AW_window::get_scrolled_picture_width() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}
AW_pos AW_window::get_scrolled_picture_height() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_window::set_horizontal_scrollbar_left_indent(int /*indent*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_vertical_scrollbar_top_indent(int /*indent*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_window::select_mode(int /*mode*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::draw_line(int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/, int /*width*/, bool /*resize*/){

}

void AW_window::get_at_position(int */*x*/, int */*y*/) const{

}

int AW_window::get_at_xposition() const{
    GTK_NOT_IMPLEMENTED;
}

int AW_window::get_at_yposition() const {
    GTK_NOT_IMPLEMENTED;
}

AW_device_click *AW_window::get_click_device(AW_area /*area*/, int /*mousex*/, int /*mousey*/, AW_pos /*max_distance_linei*/,
                                  AW_pos /*max_distance_texti*/, AW_pos /*radi*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}
AW_device *AW_window::get_device(AW_area /*area*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_window::get_event(AW_event */*eventi*/) const{

}

AW_device_print *AW_window::get_print_device(AW_area /*area*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_device_size *AW_window::get_size_device(AW_area /*area*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_window::get_window_size(int& /*width*/, int& /*height*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::help_text(const char */*id*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::hide(){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::hide_or_notify(const char */*error*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::highlight(){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_default_option (AW_label /*choice_label*/, const char */*mnemonic*/, const char */*var_value*/, const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_default_option (AW_label /*choice_label*/, const char */*mnemonic*/, int /*var_value*/,          const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_default_option (AW_label /*choice_label*/, const char */*mnemonic*/, float /*var_value*/,        const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_default_toggle(AW_label /*toggle_label*/, const char */*mnemonic*/, const char */*var_value*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_default_toggle(AW_label /*toggle_label*/, const char */*mnemonic*/, int /*var_value*/){
    GTK_NOT_IMPLEMENTED;
}


void AW_window::insert_help_topic(AW_label /*name*/, const char */*mnemonic*/, const char */*help_text*/, AW_active /*mask*/, void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_menu_topic(const char *id, AW_label /*name*/, const char */*mnemonic*/, const char */*help_text_*/, AW_active /*mask*/, AW_CB /*cb*/, AW_CL /*cd1*/, AW_CL /*cd2*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_option(AW_label /*choice_label*/, const char */*mnemonic*/, const char */*var_value*/, const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_option(AW_label /*choice_label*/, const char */*mnemonic*/, int /*var_value*/, const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_option(AW_label /*choice_label*/, const char */*mnemonic*/, float /*var_value*/, const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_sub_menu(AW_label /*name*/, const char */*mnemonic*/, AW_active mask /*= AWM_ALL*/){
    GTK_NOT_IMPLEMENTED;
}


void AW_window::insert_toggle(AW_label /*toggle_label*/, const char */*mnemonic*/, const char */*var_value*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_toggle(AW_label /*toggle_label*/, const char */*mnemonic*/, int /*var_value*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_toggle(AW_label /*toggle_label*/, const char */*mnemonic*/, float /*var_value*/){
    GTK_NOT_IMPLEMENTED;
}

bool AW_window::is_shown() const{
    GTK_NOT_IMPLEMENTED;
    return false;
}

void AW_window::label(const char */*label*/){
    GTK_NOT_IMPLEMENTED;
}

static void AW_xfigCB_info_area(AW_window *aww, AW_xfig *xfig) {

    AW_device *device = aww->get_device(AW_INFO_AREA);
    device->reset();
    if (aww->get_root()->color_mode == 0) { // mono colr
        device->clear(-1);
    }
    device->set_offset(AW::Vector(-xfig->minx, -xfig->miny));
    xfig->print(device);
}


void AW_window::load_xfig(const char *file, bool resize /*= true*/){

    AW_xfig *xfig;

    if (file)   xfig = new AW_xfig(file, get_root()->font_width, get_root()->font_height);
    else        xfig = new AW_xfig(get_root()->font_width, get_root()->font_height); // create an empty xfig

    xfig_data = xfig;

    set_expose_callback(AW_INFO_AREA, (AW_CB)AW_xfigCB_info_area, (AW_CL)xfig_data, 0);
    xfig->create_gcs(get_device(AW_INFO_AREA), get_root()->color_mode ? 8 : 1);

    int xsize = xfig->maxx - xfig->minx;
    int ysize = xfig->maxy - xfig->miny;

    if (xsize>_at->max_x_size) _at->max_x_size = xsize;
    if (ysize>_at->max_y_size) _at->max_y_size = ysize;

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
        set_window_size(_at->max_x_size+1000, _at->max_y_size+1000);
    }

}

const char *AW_window::local_id(const char */*id*/) const{
    GTK_NOT_IMPLEMENTED;
    return 0;
}



void AW_window::restore_at_size_and_attach(const AW_at_size */*at_size*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::sens_mask(AW_active /*mask*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::sep______________() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_bottom_area_height(int /*height*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_expose_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1 /*= 0*/, AW_CL cd2 /*= 0*/) {
    AW_area_management *aram = prvt.areas[area];
    if (aram) aram->set_expose_callback(this, f, cd1, cd2);
}

void AW_window::set_focus_callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_horizontal_change_callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_horizontal_scrollbar_position(int /*position*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_info_area_height(int /*height*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_input_callback(AW_area /*area*/, void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/ /*= 0*/, AW_CL /*cd2*/ /*= 0*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_motion_callback(AW_area /*area*/, void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/ /*= 0*/, AW_CL /*cd2*/ /*= 0*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_popup_callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_resize_callback(AW_area /*area*/, void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/ /*= 0*/, AW_CL /*cd2*/ /*= 0*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_vertical_change_callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_vertical_scrollbar_position(int /*position*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_window_size(int /*width*/, int /*height*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_window_title(const char */*title*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::shadow_width (int /*shadow_thickness*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::show() {
    arb_assert(NULL != prvt.window);
    gtk_widget_show(GTK_WIDGET(prvt.window));

}

void AW_window::store_at_size_and_attach(AW_at_size */*at_size*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::tell_scrolled_picture_size(AW_world /*rectangle*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::update_label(Widget /*widget*/, const char */*var_value*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::update_option_menu() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::update_toggle_field() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::window_fit() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::wm_activate() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::create_inverse_toggle(const char */*awar_name*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::auto_increment(int /*dx*/, int /*dy*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::tell_scrolled_picture_size(AW_screen_area /*rectangle*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::d_callback(void (*/*f*/)(AW_window*)) {
    GTK_NOT_IMPLEMENTED;
}

AW_window::AW_window() {


}

AW_window::~AW_window() {

}


//aw_window_menu
//FIXME move to own file
AW_window_menu::AW_window_menu(){
    GTK_NOT_IMPLEMENTED;
}

AW_window_menu::~AW_window_menu(){
    GTK_NOT_IMPLEMENTED;
}

void AW_window_menu::init(AW_root */*root*/, const char */*wid*/, const char */*windowname*/, int /*width*/, int /*height*/) {
    GTK_NOT_IMPLEMENTED;
}

AW_window_menu_modes::AW_window_menu_modes() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window_menu_modes::init(AW_root */*root*/, const char */*wid*/, const char */*windowname*/, int /*width*/, int /*height*/) {
    GTK_NOT_IMPLEMENTED;
}


AW_window_menu_modes::~AW_window_menu_modes() {
    GTK_NOT_IMPLEMENTED;
}

AW_window_simple_menu::AW_window_simple_menu() {
    GTK_NOT_IMPLEMENTED;
}

AW_window_simple_menu::~AW_window_simple_menu() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window_simple_menu::init(AW_root */*root*/, const char */*wid*/, const char */*windowname*/) {
    GTK_NOT_IMPLEMENTED;
}

AW_window_simple::AW_window_simple() : AW_window() {
    GTK_NOT_IMPLEMENTED;
}
void AW_window_simple::init(AW_root *root_in, const char *wid, const char *windowname) {
    root = root_in; // for macro
    prvt.window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

    int width  = 100;                               // this is only the minimum size!
    int height = 100;
    int posx   = 50;
    int posy   = 50;

    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    gtk_window_set_resizable(prvt.window, true);
    gtk_window_set_title(prvt.window, window_name);

    GTK_PARTLY_IMPLEMENTED;



   // p_w->shell = aw_create_shell(this, true, true, width, height, posx, posy);

    // add this to disable resize or maximize in simple dialogs (avoids broken layouts)
    // XtVaSetValues(p_w->shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE, NULL);

//    Widget form1 = XtVaCreateManagedWidget("forms", xmFormWidgetClass,
//            p_w->shell,
//            NULL);

    //for simple windows the form and drawing area are the same. Thus xfig drawings will appear on the background of the form.
    prvt.areas[AW_INFO_AREA] = new AW_area_management(root, GTK_WIDGET(prvt.window), GTK_WIDGET(prvt.window));


  //  aw_realize_widget(this);
    create_devices();
}

void AW_window::create_devices() {

    GTK_PARTLY_IMPLEMENTED;

    unsigned long background_color;
    if (prvt.areas[AW_INFO_AREA]) {
        prvt.areas[AW_INFO_AREA]->create_devices(this, AW_INFO_AREA);
//        XtVaGetValues(p_w->areas[AW_INFO_AREA]->get_area(), XmNbackground, &background_color, NULL);
//        p_global->color_table[AW_WINDOW_DRAG] = background_color ^ p_global->color_table[AW_WINDOW_FG];
    }
    if (prvt.areas[AW_MIDDLE_AREA]) {
        prvt.areas[AW_MIDDLE_AREA]->create_devices(this, AW_MIDDLE_AREA);
    }
    if (prvt.areas[AW_BOTTOM_AREA]) {
        prvt.areas[AW_BOTTOM_AREA]->create_devices(this, AW_BOTTOM_AREA);
    }
}


AW_window_simple::~AW_window_simple() {
    GTK_NOT_IMPLEMENTED;
}


void AW_at_auto::restore(AW_at */*at*/) const {
    GTK_NOT_IMPLEMENTED;
}

void AW_at_auto::store(AW_at const */*at*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_at_maxsize::store(const AW_at */*at*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_at_maxsize::restore(AW_at */*at*/) const {
    GTK_NOT_IMPLEMENTED;
}


//FIXME comment
AW_cb_struct_guard AW_cb_struct::guard_before = NULL;
AW_cb_struct_guard AW_cb_struct::guard_after  = NULL;
AW_postcb_cb       AW_cb_struct::postcb       = NULL;


AW_cb_struct::AW_cb_struct(AW_window    * /*awi*/,
                           AW_CB         /*g*/,
                           AW_CL         /*cd1i*/      /* = 0*/,
                           AW_CL         /*cd2i*/       /*= 0*/,
                           const char   */*help_texti*/ /*= 0*/,
                           AW_cb_struct */*next*/       /*= 0*/) {
    GTK_NOT_IMPLEMENTED;
}

bool AW_cb_struct::contains(void (*g)(AW_window*, AW_CL, AW_CL)) {
    return (f == g) || (next && next->contains(g));
}


void AW_cb_struct::run_callback() {

    GTK_PARTLY_IMPLEMENTED;

    if (next) next->run_callback();                 // callback the whole list
    if (!f) return;                                 // run no callback

    //FIXME this is a very simplified version of the original run_callback. See original code below and fix it.
    useraction_init();
    f(aw, cd1, cd2);
    useraction_done(aw);

//    AW_root *root = aw->get_root();
//    if (root->disable_callbacks) {
//        // some functions (namely aw_message, aw_input, aw_string_selection and aw_file_selection)
//        // have to disable most callbacks, because they are often called from inside these callbacks
//        // (e.g. because some exceptional condition occurred which needs user interaction) and if
//        // callbacks weren't disabled, a recursive deadlock occurs.
//
//        // the following callbacks are allowed even if disable_callbacks is true
//
//        bool isModalCallback = (f == AW_CB(message_cb) ||
//                                f == AW_CB(input_history_cb) ||
//                                f == AW_CB(input_cb) ||
//                                f == AW_CB(file_selection_cb));
//
//        bool isPopdown = (f == AW_CB(AW_POPDOWN));
//        bool isHelp    = (f == AW_CB(AW_POPUP_HELP));
//        bool allow     = isModalCallback || isHelp || isPopdown;
//
//        bool isInfoResizeExpose = false;
//
//        if (!allow) {
//            isInfoResizeExpose = aw->is_expose_callback(AW_INFO_AREA, f) || aw->is_resize_callback(AW_INFO_AREA, f);
//            allow              = isInfoResizeExpose;
//        }
//
//        if (!allow) {
//            // do not change position of modal dialog, when one of the following callbacks happens - just raise it
//            // (other callbacks like pressing a button, will position the modal dialog under mouse)
//            bool onlyRaise =
//                aw->is_expose_callback(AW_MIDDLE_AREA, f) ||
//                aw->is_focus_callback(f) ||
//                root->is_focus_callback((AW_RCB)f) ||
//                aw->is_resize_callback(AW_MIDDLE_AREA, f);
//
//            if (root->current_modal_window) {
//                AW_window *awmodal = root->current_modal_window;
//
//                AW_PosRecalc prev = awmodal->get_recalc_pos_atShow();
//                if (onlyRaise) awmodal->recalc_pos_atShow(AW_KEEP_POS);
//                awmodal->activate();
//                awmodal->recalc_pos_atShow(prev);
//            }
//            else {
//                aw_message("Internal error (callback suppressed when no modal dialog active)");
//                aw_assert(0);
//            }
//#if defined(TRACE_CALLBACKS)
//            printf("suppressing callback %p\n", f);
//#endif // TRACE_CALLBACKS
//            return; // suppress the callback!
//        }
//#if defined(TRACE_CALLBACKS)
//        else {
//            if (isModalCallback) printf("allowed modal callback %p\n", f);
//            else if (isPopdown) printf("allowed AW_POPDOWN\n");
//            else if (isHelp) printf("allowed AW_POPUP_HELP\n");
//            else if (isInfoResizeExpose) printf("allowed expose/resize infoarea\n");
//            else printf("allowed other (unknown) callback %p\n", f);
//        }
//#endif // TRACE_CALLBACKS
//    }
//    else {
//#if defined(TRACE_CALLBACKS)
//        printf("Callbacks are allowed (executing %p)\n", f);
//#endif // TRACE_CALLBACKS
//    }
//
//    useraction_init();
//
//    if (f == AW_POPUP) {
//        if (pop_up_window) { // already exists
//            pop_up_window->activate();
//        }
//        else {
//            AW_PPP g = (AW_PPP)cd1;
//            if (g) {
//                pop_up_window = g(aw->get_root(), cd2, 0);
//                pop_up_window->activate();
//            }
//            else {
//                aw_message("not implemented -- please report to devel@arb-home.de");
//            }
//        }
//        if (pop_up_window && p_aww(pop_up_window)->popup_cb)
//            p_aww(pop_up_window)->popup_cb->run_callback();
//    }
//    else {
//        f(aw, cd1, cd2);
//    }
//
//    useraction_done(aw);
}


