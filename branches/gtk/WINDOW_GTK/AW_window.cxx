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



void AW_help_entry_pressed(AW_window *) {
    GTK_NOT_IMPLEMENTED
}

void AW_POPDOWN(AW_window *){
    GTK_NOT_IMPLEMENTED
}

void AW_POPUP_HELP(AW_window *, AW_CL /* char */ helpfile){
    GTK_NOT_IMPLEMENTED
}

void AW_openURL(AW_root *aw_root, const char *url) {
    GTK_NOT_IMPLEMENTED
}


void AW_window::recalc_pos_atShow(AW_PosRecalc pr){
    GTK_NOT_IMPLEMENTED
}

AW_PosRecalc AW_window::get_recalc_pos_atShow() const {
    GTK_NOT_IMPLEMENTED
}

void AW_window::recalc_size_atShow(enum AW_SizeRecalc sr){
    GTK_NOT_IMPLEMENTED
}

void AW_window::on_hide(aw_hide_cb call_on_hide){
    GTK_NOT_IMPLEMENTED
}

void AW_window::at(int x, int y){
    GTK_NOT_IMPLEMENTED
}

void AW_window::at_x(int x){
    GTK_NOT_IMPLEMENTED
}

void AW_window::at_y(int y){
    GTK_NOT_IMPLEMENTED
}

void AW_window::at_shift(int x, int y){
    GTK_NOT_IMPLEMENTED
}

void AW_window::at_newline(){
    GTK_NOT_IMPLEMENTED
}


void AW_window::at(const char *id){
    GTK_NOT_IMPLEMENTED
}


bool AW_window::at_ifdef(const  char *id){
    GTK_NOT_IMPLEMENTED
}


void AW_window::at_set_to(bool attach_x, bool attach_y, int xoff, int yoff){
    GTK_NOT_IMPLEMENTED
}

void AW_window::at_unset_to(){
    GTK_NOT_IMPLEMENTED
}

void AW_window::at_set_min_size(int xmin, int ymin){
    GTK_NOT_IMPLEMENTED
}

void AW_window::auto_space(int xspace, int yspace){
    GTK_NOT_IMPLEMENTED
}


void AW_window::label_length(int length){
    GTK_NOT_IMPLEMENTED
}
void AW_window::button_length(int length){
    GTK_NOT_IMPLEMENTED
}

int  AW_window::get_button_length() const{
    GTK_NOT_IMPLEMENTED
}

void AW_window::calculate_scrollbars(){
    GTK_NOT_IMPLEMENTED
}

void AW_window::callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2){
    GTK_NOT_IMPLEMENTED
}

void AW_window::callback(void (*f)(AW_window*)){
    GTK_NOT_IMPLEMENTED
}


void AW_window::callback(void (*f)(AW_window*, AW_CL), AW_CL cd1){
    GTK_NOT_IMPLEMENTED
}

void AW_window::close_sub_menu(){
    GTK_NOT_IMPLEMENTED
}


void AW_window::create_button(const char *macro_name, AW_label label, const char *mnemonic /* = 0 */, const char *color /* = 0 */){
    GTK_NOT_IMPLEMENTED
}

void AW_window::create_autosize_button(const char *macro_name, AW_label label, const char *mnemonic/* = 0*/, unsigned xtraSpace /* = 1 */){

}
Widget AW_window::get_last_widget() const{
    GTK_NOT_IMPLEMENTED
    return 0;
}

void AW_window::create_toggle(const char *awar_name){
    GTK_NOT_IMPLEMENTED
}

void AW_window::create_toggle(const char *awar_name, const char *nobitmap, const char *yesbitmap, int buttonWidth /* = 0 */){
    GTK_NOT_IMPLEMENTED
}


void AW_window::create_input_field(const char *awar_name, int columns /* = 0 */){
    GTK_NOT_IMPLEMENTED
}

void AW_window::create_text_field(const char *awar_name, int columns /* = 20 */, int rows /*= 4*/){
    GTK_NOT_IMPLEMENTED
}



void AW_window::create_menu(AW_label name, const char *mnemonic, AW_active mask /*= AWM_ALL*/){
    GTK_NOT_IMPLEMENTED
}

int AW_window::create_mode(const char *pixmap, const char *help_text, AW_active mask, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2){
    GTK_NOT_IMPLEMENTED
    return 0;
}


AW_option_menu_struct *AW_window::create_option_menu(const char *awar_name, AW_label label /*= 0*/, const char *mnemonic /*= 0*/){
    GTK_NOT_IMPLEMENTED
    return 0;
}

AW_selection_list *AW_window::create_selection_list(const char *awar_name, int columns /*= 4*/, int rows /*= 4*/){
    GTK_NOT_IMPLEMENTED
    return 0;
}



void AW_window::create_toggle_field(const char *awar_name, int orientation /*= 0*/){
    GTK_NOT_IMPLEMENTED
}

void AW_window::create_toggle_field(const char *awar_name, AW_label label, const char *mnemonic) {
    GTK_NOT_IMPLEMENTED
}


void AW_window::d_callback(void (*f)(AW_window*, AW_CL), AW_CL cd1){
    GTK_NOT_IMPLEMENTED
}

void AW_window::d_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2){
    GTK_NOT_IMPLEMENTED
}

void AW_window::draw_line(int x1, int y1, int x2, int y2, int width, bool resize){

}

void AW_window::get_at_position(int *x, int *y) const{

}

int AW_window::get_at_xposition() const{
    GTK_NOT_IMPLEMENTED
}

int AW_window::get_at_yposition() const {
    GTK_NOT_IMPLEMENTED
}

AW_device_click *AW_window::get_click_device(AW_area area, int mousex, int mousey, AW_pos max_distance_linei,
                                  AW_pos max_distance_texti, AW_pos radi){
    GTK_NOT_IMPLEMENTED
    return 0;
}
AW_device *AW_window::get_device(AW_area area){
    GTK_NOT_IMPLEMENTED
    return 0;
}

void AW_window::get_event(AW_event *eventi) const{

}

AW_device_print *AW_window::get_print_device(AW_area area){
    GTK_NOT_IMPLEMENTED
    return 0;
}

AW_device_size *AW_window::get_size_device(AW_area area){
    GTK_NOT_IMPLEMENTED
    return 0;
}

void AW_window::get_window_size(int& width, int& height){
    GTK_NOT_IMPLEMENTED
}

void AW_window::help_text(const char *id){
    GTK_NOT_IMPLEMENTED
}

void AW_window::hide(){
    GTK_NOT_IMPLEMENTED
}

void AW_window::hide_or_notify(const char *error){
    GTK_NOT_IMPLEMENTED
}

void AW_window::highlight(){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_default_option (AW_label choice_label, const char *mnemonic, const char *var_value, const char *name_of_color /*= 0*/){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_default_option (AW_label choice_label, const char *mnemonic, int var_value,          const char *name_of_color /*= 0*/){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_default_option (AW_label choice_label, const char *mnemonic, float var_value,        const char *name_of_color /*= 0*/){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_default_toggle(AW_label toggle_label, const char *mnemonic, const char *var_value){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_default_toggle(AW_label toggle_label, const char *mnemonic, int var_value){
    GTK_NOT_IMPLEMENTED
}


void AW_window::insert_help_topic(AW_label name, const char *mnemonic, const char *help_text, AW_active mask, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_menu_topic(const char *id, AW_label name, const char *mnemonic, const char *help_text_, AW_active mask, AW_CB cb, AW_CL cd1, AW_CL cd2){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_option(AW_label choice_label, const char *mnemonic, const char *var_value, const char *name_of_color /*= 0*/){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_option(AW_label choice_label, const char *mnemonic, int var_value, const char *name_of_color /*= 0*/){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_option(AW_label choice_label, const char *mnemonic, float var_value, const char *name_of_color /*= 0*/){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_sub_menu(AW_label name, const char *mnemonic, AW_active mask /*= AWM_ALL*/){
    GTK_NOT_IMPLEMENTED
}


void AW_window::insert_toggle(AW_label toggle_label, const char *mnemonic, const char *var_value){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_toggle(AW_label toggle_label, const char *mnemonic, int var_value){
    GTK_NOT_IMPLEMENTED
}

void AW_window::insert_toggle(AW_label toggle_label, const char *mnemonic, float var_value){
    GTK_NOT_IMPLEMENTED
}

bool AW_window::is_shown() const{
    GTK_NOT_IMPLEMENTED
    return false;
}

void AW_window::label(const char *label){
    GTK_NOT_IMPLEMENTED
}

void AW_window::load_xfig(const char *file, bool resize /*= true*/){
    GTK_NOT_IMPLEMENTED
}

const char *AW_window::local_id(const char *id) const{
    GTK_NOT_IMPLEMENTED
    return 0;
}



void AW_window::restore_at_size_and_attach(const AW_at_size *at_size){
    GTK_NOT_IMPLEMENTED
}

void AW_window::sens_mask(AW_active mask){
    GTK_NOT_IMPLEMENTED
}

void AW_window::sep______________() {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_bottom_area_height(int height) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_expose_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1 /*= 0*/, AW_CL cd2 /*= 0*/) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_focus_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_horizontal_change_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_horizontal_scrollbar_position(int position) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_info_area_height(int height) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_input_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1 /*= 0*/, AW_CL cd2 /*= 0*/) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_motion_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1 /*= 0*/, AW_CL cd2 /*= 0*/) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_popup_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_resize_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1 /*= 0*/, AW_CL cd2 /*= 0*/) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_vertical_change_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_vertical_scrollbar_position(int position) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_window_size(int width, int height) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::set_window_title(const char *title){
    GTK_NOT_IMPLEMENTED
}

void AW_window::shadow_width (int shadow_thickness) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::show() {
    GTK_NOT_IMPLEMENTED
}

void AW_window::store_at_size_and_attach(AW_at_size *at_size) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::tell_scrolled_picture_size(AW_world rectangle) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::update_label(Widget widget, const char *var_value) {
    GTK_NOT_IMPLEMENTED
}

void AW_window::update_option_menu() {
    GTK_NOT_IMPLEMENTED
}

void AW_window::update_toggle_field() {
    GTK_NOT_IMPLEMENTED
}

void AW_window::window_fit() {
    GTK_NOT_IMPLEMENTED
}

void AW_window::wm_activate() {
    GTK_NOT_IMPLEMENTED
}

AW_window::AW_window() {

}

AW_window::~AW_window() {
    GTK_NOT_IMPLEMENTED
}


//aw_window_menu
//FIXME move to own file
AW_window_menu::AW_window_menu(){
    GTK_NOT_IMPLEMENTED
}

void AW_window_menu::init(AW_root *root, const char *wid, const char *windowname, int width, int height) {
    GTK_NOT_IMPLEMENTED
}

AW_window_menu_modes::AW_window_menu_modes() {
    GTK_NOT_IMPLEMENTED
}

void AW_window_menu_modes::init(AW_root *root, const char *wid, const char *windowname, int width, int height) {
    GTK_NOT_IMPLEMENTED
}


AW_window_menu_modes::~AW_window_menu_modes() {
    GTK_NOT_IMPLEMENTED
}

AW_window_simple_menu::AW_window_simple_menu() {
    GTK_NOT_IMPLEMENTED
}

void AW_window_simple_menu::init(AW_root *root, const char *wid, const char *windowname) {
    GTK_NOT_IMPLEMENTED
}

AW_window_simple::AW_window_simple() {
    GTK_NOT_IMPLEMENTED
}
void AW_window_simple::init(AW_root *root, const char *wid, const char *windowname) {

}


void AW_at_auto::restore(AW_at *at) const {
    GTK_NOT_IMPLEMENTED
}

void AW_at_auto::store(AW_at const *at) {
    GTK_NOT_IMPLEMENTED
}



