#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <Xm/Xm.h>
#include <Xm/Frame.h>
#include <Xm/MenuShell.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ScrolledW.h>

#include <arbdb.h>
#include <arbdbt.h>
#include "aw_root.hxx"
#include "aw_at.hxx"
#include "aw_device.hxx"
#include "aw_Xm.hxx"
#include "aw_click.hxx"
#include "aw_print.hxx"
#include "aw_size.hxx"
#include "aw_window.hxx"
#include "aw_awar.hxx"
#include "aw_window_Xm.hxx"


/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*********************************************************************************************************************/
void aw_cp_awar_2_widget_cb(AW_root *root, AW_widget_list_for_variable *widgetlist){
    if (widgetlist->widget == (int *)root->changer_of_variable){
        root->changer_of_variable = 0;
        root->value_changed = AW_FALSE;
        return;
    }

    {
        char *var_value;
        var_value = widgetlist->awar->read_as_string();

        // und benachrichtigen der anderen
        switch(widgetlist->widget_type) {

            case AW_WIDGET_INPUT_FIELD:
                widgetlist->aw->update_input_field(widgetlist->widget, var_value );
                break;
            case AW_WIDGET_TEXT_FIELD:
                widgetlist->aw->update_text_field(widgetlist->widget, var_value );
                break;
            case AW_WIDGET_TOGGLE:
                widgetlist->aw->update_toggle(widgetlist->widget, var_value, widgetlist->cd );
                break;
            case AW_WIDGET_LABEL_FIELD:
                widgetlist->aw->update_label( widgetlist->widget, var_value );
                break;
            case AW_WIDGET_CHOICE_MENU:
                widgetlist->aw->update_option_menu( (int)widgetlist->cd );
                break;
            case AW_WIDGET_TOGGLE_FIELD:
                widgetlist->aw->update_toggle_field( (int)widgetlist->cd );
                break;
            case AW_WIDGET_SELECTION_LIST:
                widgetlist->aw->update_selection_list_intern( (AW_selection_list *)widgetlist->cd );
            default:
                break;
        }
        free(var_value);
    }
    root->value_changed = AW_FALSE;		// Maybe value changed is set because Motig calls me
}


AW_widget_list_for_variable::AW_widget_list_for_variable( AW_awar *vs, AW_CL cd1, int *widgeti, AW_widget_type type, AW_window *awi ) {
    cd		    = cd1;
    widget		= widgeti;
    widget_type	= type;
    awar		= vs;
    aw		    = awi;
    next		= 0;
    awar->add_callback((AW_RCB1)aw_cp_awar_2_widget_cb, (AW_CL)this);
}


void AW_variable_update_callback( Widget wgt, XtPointer variable_update_struct, XtPointer call_data ) {
    AWUSE(wgt);AWUSE(call_data);
    AW_variable_update_struct *vus   = (AW_variable_update_struct *) variable_update_struct;
    char	                  *tmp   = 0;
    long	                   h_int;
    float	                   h_float;
    GB_ERROR	               error = 0;
    XmListCallbackStruct      *xml;
    AW_root                   *root  = vus->awar->root;

    if (root->value_changed){
        root->changer_of_variable = (long)vus->widget;
    }

    switch (vus->widget_type) {

        case AW_WIDGET_INPUT_FIELD:
        case AW_WIDGET_TEXT_FIELD:
            if (!root->value_changed) return;
            tmp = XmTextGetString( (vus->widget) );

            switch ( vus->awar->variable_type ) {
                case AW_STRING:
                    error = vus->awar->write_string( tmp );
                    break;
                case AW_INT:
                    h_int = atoi(tmp);
                    error = vus->awar->write_int( h_int );
                    break;
                case AW_FLOAT:
                    h_float = atof(tmp);
                    error = vus->awar->write_float( h_float );
                    break;
                default:
                    error = GB_export_error("Unknown AWAR type");
            }
            XtFree( tmp );
            break;
        case AW_WIDGET_TOGGLE:
            {
                root->changer_of_variable = 0;
                error = vus->awar->toggle_toggle();
                break;
            }

        case AW_WIDGET_TOGGLE_FIELD:
            int state;
            state = XmToggleButtonGetState(vus->widget);
            if (state != True) break;

        case AW_WIDGET_CHOICE_MENU:

            switch ( vus->awar->variable_type ) {
                case AW_STRING:
                    error =	vus->awar->write_string( vus->variable_value );
                    break;
                case AW_INT:
                    error =	vus->awar->write_int(vus->variable_int_value );
                    break;
                case AW_FLOAT:
                    error =	vus->awar->write_float( vus->variable_float_value );
                    break;
                default: GB_warning("Unknown AWAR type");

            }
            break;


        case AW_WIDGET_SELECTION_LIST: {

            char *ptr;
            AW_selection_list *selection_list;
            AW_select_table_struct *list_table;
            AW_BOOL found;
            found = AW_FALSE;
            xml = (XmListCallbackStruct*)call_data;

            XmStringGetLtoR( xml->item, XmSTRING_DEFAULT_CHARSET, &tmp );


            selection_list = ((AW_selection_list *)(vus->id));

            for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ) {
                ptr = list_table->displayed;
                if ( strcmp( tmp, ptr ) == 0 ) {
                    break;
                }
            }
            if ( !list_table ) { // test if default selection exists
                list_table = selection_list->default_select;
                if (!list_table) {
                    AW_ERROR("no default for selection list specified");
                    return;
                }
            }
            switch ( vus->awar->variable_type ) {
                case AW_STRING:
                    error = vus->awar->write_string( list_table->char_value );
                    break;
                case AW_INT:
                    error = vus->awar->write_int( list_table->int_value );
                    break;
                case AW_FLOAT:
                    error = vus->awar->write_float( list_table->float_value );
                    break;
                default:
                    error = GB_export_error("Unknown AWAR type");
                    break;
            }
            XtFree( tmp );
            break;
        }
        case AW_WIDGET_LABEL_FIELD:
            break;
        default:
            GB_warning("Unknown Widget Type");
            break;
    }

    if (error) {
        root->changer_of_variable = 0;
        vus->awar->update();
        aw_message(error);
    }
    else {
        if (root->prvt->recording_macro_file){
            fprintf(root->prvt->recording_macro_file,"BIO::remote_awar($gb_main,\"%s\",",root->prvt->application_name_for_macros);
            GBS_fwrite_string(vus->awar->awar_name,root->prvt->recording_macro_file);
            fprintf(root->prvt->recording_macro_file,",");
            char *value = vus->awar->read_as_string();
            GBS_fwrite_string(value,root->prvt->recording_macro_file);
            delete value;
            fprintf(root->prvt->recording_macro_file,");\n");
        }
        if (vus->cbs) vus->cbs->run_callback();
        root->value_changed = AW_FALSE;
    }
}


void AW_value_changed_callback( Widget wgt, XtPointer rooti, XtPointer call_data ) {
    AWUSE(wgt);AWUSE(call_data);
    AW_root *root = (AW_root *)rooti;
    root->value_changed = AW_TRUE;
}



/****************************************************************************************************************************/
/****************************************************************************************************************************/

void     aw_attach_widget(Widget scrolledWindowText,AW_at *_at,int default_width = -1){
    short height = 0;
    short width = 0;
    if (!_at->to_position_exists){
        XtVaGetValues( scrolledWindowText, XmNheight, &height, XmNwidth, &width,NULL );
        if (default_width >0) width = default_width;
        _at->to_position_x = _at->x_for_next_button + width;
        _at->to_position_y = _at->y_for_next_button + height;
        _at->attach_x = _at->attach_lx;
        _at->attach_y = _at->attach_ly;
    }


    if (_at->attach_x){
        XtVaSetValues(	scrolledWindowText,
                        XmNrightAttachment, 		XmATTACH_FORM,
                        XmNrightOffset,			_at->max_x_size - _at->to_position_x,
                        NULL );
    }else{
        XtVaSetValues(	scrolledWindowText,
                        XmNrightAttachment, 		XmATTACH_OPPOSITE_FORM,
                        XmNrightOffset,			-_at->to_position_x,
                        NULL );
    }
    if (_at->attach_lx){
        XtVaSetValues(	scrolledWindowText,
                        XmNwidth,			_at->to_position_x - _at->x_for_next_button,
                        XmNleftAttachment, 		XmATTACH_NONE,
                        NULL );
    }else{
        XtVaSetValues(	scrolledWindowText,
                        XmNleftAttachment, 		XmATTACH_FORM,
                        XmNleftOffset,			_at->x_for_next_button,
                        NULL );
    }

    if (_at->attach_y){
        XtVaSetValues(	scrolledWindowText,
                        XmNbottomAttachment, 		XmATTACH_FORM,
                        XmNbottomOffset,		_at->max_y_size - _at->to_position_y,
                        NULL );
    }else{
        XtVaSetValues(	scrolledWindowText,
                        XmNbottomAttachment, 		XmATTACH_OPPOSITE_FORM,
                        XmNbottomOffset,			- _at->to_position_y,
                        NULL );
    }
    if (_at->attach_ly){
        XtVaSetValues(	scrolledWindowText,
                        XmNheight,			_at->to_position_y - _at->y_for_next_button,
                        XmNtopAttachment, 		XmATTACH_NONE,
                        NULL );

    }else{
        XtVaSetValues(	scrolledWindowText,
                        XmNtopAttachment, 		XmATTACH_FORM,
                        XmNtopOffset,			_at->y_for_next_button,
                        NULL );
    }
}
/****************************************************************************************************************************/
/****************************************************************************************************************************/

void AW_window::create_autosize_button( const char *macro_name, AW_label buttonlabel,const  char *mnemonic, unsigned xtraSpace) {
    int   len               = strlen(buttonlabel)+(xtraSpace*2);
    short length_of_buttons = _at->length_of_buttons;

    _at->length_of_buttons = len+1;
    create_button(macro_name, buttonlabel, mnemonic);
    _at->length_of_buttons = length_of_buttons;
}

void AW_window::create_button( const char *macro_name, AW_label buttonlabel,const  char *mnemonic) {
    AWUSE(mnemonic);
    Widget button                = 0;
    Widget label                 = 0;
    int    x_correcting_for_label;
    int    width_of_button;
    int    width_of_label;
    int    width_of_last_widget  = 0;
    int    height_of_last_widget = 0;

    check_at_pos();

    if (_callback && ((long)_callback!= 1) )
    {
        if (macro_name){
            _callback->id = strdup(GBS_global_string("%s/%s",this->window_defaults_name,macro_name));
            GBS_write_hash(this->get_root()->prvt->action_hash,_callback->id,(long)_callback);
        }else{
            _callback->id = 0;
        }
    }

    width_of_label = this->calculate_string_width( calculate_label_length() );
    width_of_button = this->calculate_string_width( _at->length_of_buttons );

    if ( _at->label_for_inputfield ) {
        label = XtVaCreateManagedWidget( "label",
                                         xmLabelWidgetClass,
                                         (_at->attach_any)?INFO_FORM:INFO_WIDGET,
                                         XmNwidth, (int)(width_of_label + 2),
                                         RES_LABEL_CONVERT(_at->label_for_inputfield),
                                         XmNrecomputeSize, AW_FALSE,
                                         XmNalignment, XmALIGNMENT_BEGINNING,
                                         XmNfontList, p_global->fontlist,
                                         (_at->attach_any) ? NULL:XmNx,(int)_at->x_for_next_button,
                                         XmNy, (int)(_at->y_for_next_button),
                                         NULL );

        if (_at->attach_any){ // +
            aw_attach_widget(label,_at);
        }

        AW_LABEL_IN_AWAR_LIST(this,label,_at->label_for_inputfield);
        x_correcting_for_label = width_of_label+10;
    }
    else {
        x_correcting_for_label = 0;
    }

    width_of_last_widget = x_correcting_for_label + width_of_button;

    if ( _at->to_position_exists ) {
        width_of_button = _at->to_position_x - _at->x_for_next_button - x_correcting_for_label - 3;
        width_of_last_widget = _at->to_position_x - _at->x_for_next_button;
    }
    short height = 0;
    short width = width_of_button;

    Widget fatherwidget = INFO_WIDGET;
    if ( _at->highlight ) {
        fatherwidget = XtVaCreateManagedWidget( "draw_area",
                                                xmFrameWidgetClass,
                                                INFO_WIDGET,
                                                XmNx, (int) (_at->x_for_next_button + x_correcting_for_label),
                                                XmNy, (int)(_at->y_for_next_button - 3 - _at->shadow_thickness ),
                                                XmNmarginWidth,	3,
                                                XmNmarginHeight,3,
                                                XmNshadowType, XmSHADOW_IN,
                                                XmNshadowThickness, _at->shadow_thickness,
                                                NULL);
    }
    const char *color_switch = 0;
    Pixel bg_color = 0;

    if (_at->background_colorname){
        color_switch = XmNbackground;
        XColor unused;
        XColor color;
        if( XAllocNamedColor(p_global->display,p_global->colormap,_at->background_colorname,&color,&unused) == 0) {
            fprintf(stderr,"XAllocColor failed: %s\n",_at->background_colorname);
            color_switch = 0;
        }
        else {
            bg_color = color.pixel;
        }
    }


    if (_callback){
        char *mwidth = XmNwidth;
        if (!width_of_button) mwidth = 0;
        if (_at->attach_any){
            button = XtVaCreateManagedWidget( "button",
                                              xmPushButtonWidgetClass,
                                              INFO_FORM,
                                              RES_LABEL_CONVERT(buttonlabel),
                                              XmNshadowThickness, _at->shadow_thickness,
                                              XmNalignment, XmALIGNMENT_CENTER,
                                              XmNfontList, p_global->fontlist,
                                              color_switch, bg_color,
                                              NULL );
            aw_attach_widget(button,_at,width_of_button + 2);
            width_of_last_widget = _at->to_position_x - _at->x_for_next_button;
            height_of_last_widget = _at->to_position_y - _at->y_for_next_button;
        }
        else {
            button = XtVaCreateManagedWidget( "label",
                                              xmPushButtonWidgetClass,
                                              fatherwidget,
                                              XmNx, (int) (_at->x_for_next_button + x_correcting_for_label),
                                              XmNy, (int)(_at->y_for_next_button),
                                              RES_LABEL_CONVERT(buttonlabel),
                                              XmNrecomputeSize, AW_FALSE,
                                              XmNshadowThickness, _at->shadow_thickness,
                                              XmNalignment, XmALIGNMENT_CENTER,
                                              XmNfontList, p_global->fontlist,
                                              mwidth, (int)(width_of_button + 2),
                                              color_switch, bg_color,
                                              NULL );
        }
        AW_LABEL_IN_AWAR_LIST(this,button,buttonlabel);
        AW_INSERT_BUTTON_IN_SENS_LIST ( root,
                                        _at->id_for_next_button, _at->mask_for_next_button, button );

    }
    else {
        button = XtVaCreateManagedWidget( "label",
                                          xmLabelWidgetClass,
                                          (_at->attach_any)?INFO_FORM:fatherwidget,
                                          XmNrecomputeSize, AW_FALSE,
                                          XmNx, (int)(_at->x_for_next_button + x_correcting_for_label ),
                                          XmNy, (int)(_at->y_for_next_button) + 4,
                                          XmNalignment, XmALIGNMENT_BEGINNING,
                                          RES_LABEL_CONVERT(buttonlabel),
                                          XmNfontList, p_global->fontlist,
                                          color_switch, bg_color,
                                          NULL );
        if (width_of_button >= this->calculate_string_width(strlen(buttonlabel))){
            XtVaSetValues( button, XmNwidth, width_of_button + 2, NULL);
        }
        if (_at->attach_any){
            aw_attach_widget(button,_at);
        }
        AW_JUSTIFY_LABEL(button,_at->correct_for_at_center);
        AW_LABEL_IN_AWAR_LIST(this,button,buttonlabel);
    }

    if (_at->to_position_exists){
        if (!height)	height = _at->to_position_y - _at->y_for_next_button;
        if (!height)	XtVaGetValues( button, XmNheight, &height, XmNwidth, &width,NULL );
        height_of_last_widget = height;
    }
    else {
        if (!height) XtVaGetValues( button, XmNheight, &height, XmNwidth, &width,NULL );
        width_of_last_widget = x_correcting_for_label + width;

        height_of_last_widget = height;
        short div = 1;
        if (_at->correct_for_at_center) {
            Widget w = button;
            if (_at->highlight) w = fatherwidget;

            switch (_at->correct_for_at_center){
                case 1:	div = 2;		// middle centered
                case 2:				// right justified
                    XtVaSetValues( w, XmNx, (
                                             (_at->x_for_next_button + x_correcting_for_label) -
                                             (int)(width_of_last_widget-1)/div), NULL );
                    if ( label ) {
                        XtVaSetValues( label, XmNx, ((_at->x_for_next_button) -
                                                     ((width_of_last_widget-1)/div) ), NULL );
                    }
                    width_of_last_widget -= width_of_last_widget / div;
            }
        }
        width_of_last_widget += 3;
    }
    p_w->toggle_field = button;
    this->_set_activate_callback((void *)button);
    this->unset_at_commands();
    this->increment_at_commands( width_of_last_widget, height_of_last_widget );
}

void AW_window::dump_at_position(const char *label) const {
    printf("%s at x = %i / y = %i\n", label, _at->x_for_next_button, _at->y_for_next_button);
}

void AW_window::update_label( int *widget, const char *var_value ) {
    Widget w = (Widget) widget;

    if ( get_root()->changer_of_variable != (long)widget ) {
        XtVaSetValues( w, RES_CONVERT( XmNlabelString, var_value ), NULL );
    }
    else {
        get_root()->changer_of_variable = 0;
    }
}

/******************************************************************************************/
/***********************	0/1 toggle			***************************/
/******************************************************************************************/

void AW_window::update_toggle(int *wgt,const char *var,AW_CL cd)
{
    char **bitmaps = (char**)cd;
    char *path;
    if (var[0] == '0' || var[0] == 'n') {
        path = strdup(GBS_global_string("%s/lib/pixmaps/%s",GB_getenvARBHOME(),bitmaps[0]+1));
    }else{
        path = strdup(GBS_global_string("%s/lib/pixmaps/%s",GB_getenvARBHOME(),bitmaps[1]+1));
    }
    XtVaSetValues( (Widget)wgt, RES_CONVERT( XmNlabelPixmap, path ), NULL );
    delete path;
}

void AW_window::create_toggle( const char *var_name,const char *no, const char *yes ) {
    AW_cb_struct *cbs = _callback;
    _callback = (AW_cb_struct *)1;
    int old_length_of_buttons = _at->length_of_buttons;
    _at->length_of_buttons = 0;
    AW_awar *vs;
    char **bitmaps = new char *[2];
    bitmaps[0] = GBS_string_eval(no,"*=#*1:##=#",0);
    bitmaps[1] = GBS_string_eval(yes,"*=#*1:##=#",0);
    create_button(0,bitmaps[0],0);

    _at->length_of_buttons = 	old_length_of_buttons;

    vs=	this->get_root()->awar(var_name);

    char *var_value = vs->read_as_string();
    this->update_toggle((int*)p_w->toggle_field,var_value,(AW_CL)bitmaps);
    delete var_value;

    AW_variable_update_struct *vus;
    vus = new AW_variable_update_struct( p_w->toggle_field, AW_WIDGET_TOGGLE, vs, 0, 0, 0, cbs );

    XtAddCallback(p_w->toggle_field, XmNactivateCallback,
                  (XtCallbackProc) AW_variable_update_callback,
                  (XtPointer) vus );

    AW_INSERT_BUTTON_IN_AWAR_LIST( vs,(AW_CL)bitmaps, p_w->toggle_field, AW_WIDGET_TOGGLE, this);
}

void AW_window::create_toggle( const char *var_name ) {
    create_toggle(var_name,"no.bitmap","yes.bitmap");
}

/************************************************************************************************/
/************************************************************************************************/
/************************************************************************************************/



void AW_window::create_input_field( const char *var_name,  int columns ) {
    Widget	                   textField              = 0;
    Widget	                   label                  = 0;
    AW_cb_struct              *cbs;
    AW_variable_update_struct *vus;
    char	                  *String;
    int                        x_correcting_for_label = 0;
    int                        width_of_input_label;
    int                        width_of_input;
    int                        width_of_last_widget;
    int                        height_of_last_widget;


    check_at_pos();
    if (!columns) columns = _at->length_of_buttons;

    AW_awar *vs =root->awar(var_name);
    String = root->awar( var_name )->read_string();

    width_of_input_label = this->calculate_string_width( calculate_label_length() );
    width_of_input = this->calculate_string_width( columns ) + 18;

    if ( _at->label_for_inputfield ) {
        label = XtVaCreateManagedWidget( "label",
                                         xmLabelWidgetClass,
                                         (_at->attach_any)?INFO_FORM:INFO_WIDGET,
                                         XmNwidth, (int)(width_of_input_label + 2),
                                         XmNhighlightThickness, 0,
                                         RES_CONVERT( XmNlabelString, _at->label_for_inputfield ),
                                         XmNrecomputeSize, AW_FALSE,
                                         XmNalignment, XmALIGNMENT_BEGINNING,
                                         XmNfontList, p_global->fontlist,
                                         (_at->attach_any) ? NULL:XmNx,(int)_at->x_for_next_button,
                                         XmNy, (int)(_at->y_for_next_button)+ root->y_correction_for_input_labels -1,
                                         NULL );
        if (_at->attach_any){
            aw_attach_widget(label,_at);
        }
        x_correcting_for_label = width_of_input_label + 10;
    }


    width_of_last_widget = x_correcting_for_label + width_of_input + 2;

    if ( _at->to_position_exists ) {
        width_of_input = _at->to_position_x - _at->x_for_next_button - x_correcting_for_label + 2;
        width_of_last_widget = _at->to_position_x - _at->x_for_next_button;
    }
    {
        textField = XtVaCreateManagedWidget( "textField",
                                             xmTextFieldWidgetClass,
                                             (_at->attach_any)?INFO_FORM:INFO_WIDGET,
                                             XmNwidth, (int)width_of_input,
                                             XmNrows, 1,
                                             XmNvalue, String,
                                             XmNfontList, p_global->fontlist,
                                             (_at->attach_any) ? NULL:XmNx, (int)(_at->x_for_next_button + x_correcting_for_label ),
                                             XmNy, (int)(_at->y_for_next_button + 5) - 8,
                                             NULL );
        if (_at->attach_any){
            aw_attach_widget(textField,_at);
        }

    }
    free( String );

    // user-own callback
    cbs = _callback;

    // callback for enter
    vus = new AW_variable_update_struct( textField, AW_WIDGET_INPUT_FIELD, vs, 0, 0, 0, cbs );

    XtAddCallback(textField, XmNactivateCallback,
                  (XtCallbackProc) AW_variable_update_callback,
                  (XtPointer) vus );
    if (_d_callback) {
        XtAddCallback(textField, XmNactivateCallback,
                      (XtCallbackProc) AW_server_callback,
                      (XtPointer) _d_callback );
        _d_callback->id = strdup(GBS_global_string("INPUT:%s",var_name));
        GBS_write_hash(this->get_root()->prvt->action_hash,_d_callback->id,(long)_d_callback);
    }

    // callback for losing focus
    XtAddCallback(textField, XmNlosingFocusCallback,
                  (XtCallbackProc) AW_variable_update_callback,
                  (XtPointer) vus );
    // callback for value changed
    XtAddCallback(textField, XmNvalueChangedCallback,
                  (XtCallbackProc) AW_value_changed_callback,
                  (XtPointer) root );

    AW_INSERT_BUTTON_IN_AWAR_LIST( vs,0, textField, AW_WIDGET_INPUT_FIELD, this);

    AW_INSERT_BUTTON_IN_SENS_LIST ( root, _at->id_for_next_button, _at->mask_for_next_button, textField );

    short height;
    XtVaGetValues( textField, XmNheight, &height, NULL);
    height_of_last_widget = height;

    if ( _at->correct_for_at_center == 1 ) { // middle centered
        XtVaSetValues( textField, XmNx, ((int)(_at->x_for_next_button + x_correcting_for_label) - (int)(width_of_last_widget/2) + 1 ), NULL );
        if ( label ) {
            XtVaSetValues( label, XmNx, ((int)(_at->x_for_next_button) - (int)(width_of_last_widget/2) + 1 ), NULL );
        }
        width_of_last_widget = width_of_last_widget / 2;
    }
    if ( _at->correct_for_at_center == 2 ) { // right centered
        XtVaSetValues( textField, XmNx, (int)(_at->x_for_next_button + x_correcting_for_label - width_of_last_widget + 3 ), NULL );
        if ( label ) {
            XtVaSetValues( label, XmNx, (int)(_at->x_for_next_button - width_of_last_widget + 3 ), NULL );
        }
        width_of_last_widget = 0;
    }
    width_of_last_widget -= 2;

    this->unset_at_commands();
    this->increment_at_commands( width_of_last_widget, height_of_last_widget );

}


void AW_window::update_input_field(int *widget, const char *var_value ) {
    Widget w = (Widget) widget;

    XtVaSetValues( w, XmNvalue, var_value, NULL );
}




void AW_window::create_text_field( const char *var_name, int columns, int rows ) {
    Widget scrolledWindowText;
    Widget scrolledText;
    Widget label =0;
    AW_cb_struct *cbs;
    AW_variable_update_struct *vus;
    char *String = NULL;
    short width_of_last_widget = 0;
    short height_of_last_widget = 0;
    int width_of_text = 0;
    int height_of_text = 0;
    int width_of_text_label = 0;
    int x_correcting_for_label = 0;

    AW_awar *vs = root->awar(var_name);
    String = root->awar( var_name )->read_string();

    width_of_text_label = this->calculate_string_width( calculate_label_length() );
    width_of_text = this->calculate_string_width( columns ) + 18;
    height_of_text = this->calculate_string_height( rows, rows*4 ) + 9;


    if ( _at->label_for_inputfield ) {
        label = XtVaCreateManagedWidget( "label",
                                         xmLabelWidgetClass,
                                         INFO_WIDGET,
                                         XmNx, (int)_at->x_for_next_button,
                                         XmNy, (int)(_at->y_for_next_button) + this->get_root()->y_correction_for_input_labels + 5 - 6,
                                         XmNwidth, (int)(width_of_text_label + 2),
                                         RES_CONVERT( XmNlabelString, _at->label_for_inputfield ),
                                         XmNrecomputeSize, AW_FALSE,
                                         XmNalignment, XmALIGNMENT_BEGINNING,
                                         XmNfontList, p_global->fontlist,
                                         NULL );

        x_correcting_for_label = width_of_text_label + 10;

    }


    if (_at->to_position_exists){
        scrolledWindowText = XtVaCreateManagedWidget( "scrolledWindowList1",
                                                      xmScrolledWindowWidgetClass,
                                                      INFO_FORM,
                                                      XmNscrollingPolicy, XmAPPLICATION_DEFINED,
                                                      XmNvisualPolicy, XmVARIABLE,
                                                      XmNscrollBarDisplayPolicy, XmSTATIC,
                                                      XmNfontList, 			p_global->fontlist,
                                                      NULL );
        aw_attach_widget(scrolledWindowText,_at);

        width_of_text = _at->to_position_x - _at->x_for_next_button - x_correcting_for_label - 18;
        if ( _at->y_for_next_button < _at->to_position_y - 18 ) {
            height_of_text = _at->to_position_y - _at->y_for_next_button - 18;
        }
    }else{
        scrolledWindowText = XtVaCreateManagedWidget( "scrolledWindowText",
                                                      xmScrolledWindowWidgetClass,
                                                      INFO_WIDGET,
                                                      XmNscrollingPolicy, XmAPPLICATION_DEFINED,
                                                      XmNvisualPolicy, XmVARIABLE,
                                                      XmNscrollBarDisplayPolicy, XmSTATIC,
                                                      XmNx, (int)10,
                                                      XmNy, (int)_at->y_for_next_button,
                                                      XmNfontList, p_global->fontlist,
                                                      NULL );
    }

    scrolledText = XtVaCreateManagedWidget( "scrolledText1",
                                            xmTextWidgetClass,
                                            scrolledWindowText,
                                            XmNeditMode, XmMULTI_LINE_EDIT ,
                                            XmNvalue, String,
                                            XmNscrollLeftSide, AW_FALSE,
                                            XmNwidth, (int)width_of_text,
                                            XmNheight, (int)height_of_text,
                                            XmNfontList, p_global->fontlist,
                                            NULL );

    delete String;

    if (!_at->to_position_exists){
        XtVaGetValues( scrolledWindowText,	XmNheight, &height_of_last_widget,
                       XmNwidth, &width_of_last_widget, NULL);

        width_of_last_widget += (short)x_correcting_for_label;

        switch(_at->correct_for_at_center){
            case 0: // left centered
                XtVaSetValues( scrolledWindowText, XmNx, (int)(_at->x_for_next_button + x_correcting_for_label), NULL);
                break;

            case 1: // middle centered
                XtVaSetValues( scrolledWindowText, XmNx, (int)( _at->x_for_next_button + x_correcting_for_label - (width_of_last_widget/2) ), NULL);
                if ( _at->label_for_inputfield ) {
                    XtVaSetValues( label, XmNx, (int)( _at->x_for_next_button - (width_of_last_widget/2) ), NULL );
                }
                width_of_last_widget = width_of_last_widget / 2;
                break;

            case 2: // right centered
                XtVaSetValues( scrolledWindowText, XmNx, (int)( _at->x_for_next_button + x_correcting_for_label - width_of_last_widget ) , NULL);
                if ( _at->label_for_inputfield ) {
                    XtVaSetValues( label, XmNx, (int)( _at->x_for_next_button - width_of_last_widget ), NULL );
                }
                width_of_last_widget = 0;
                break;
        }
    }

    // user-own callback
    cbs = _callback;

    // callback for enter
    vus = new AW_variable_update_struct( scrolledText, AW_WIDGET_TEXT_FIELD, vs, 0, 0, 0, cbs );
    XtAddCallback(scrolledText, XmNactivateCallback, (XtCallbackProc) AW_variable_update_callback, (XtPointer) vus );
    // callback for losing focus
    XtAddCallback(scrolledText, XmNlosingFocusCallback, (XtCallbackProc) AW_variable_update_callback, (XtPointer) vus );
    // callback for value changed
    XtAddCallback(scrolledText, XmNvalueChangedCallback, (XtCallbackProc) AW_value_changed_callback, (XtPointer) root );

    AW_INSERT_BUTTON_IN_AWAR_LIST( vs,0, scrolledText, AW_WIDGET_TEXT_FIELD, this);
    AW_INSERT_BUTTON_IN_SENS_LIST ( root, _at->id_for_next_button, _at->mask_for_next_button, scrolledText );


    this->unset_at_commands();
    this->increment_at_commands( width_of_last_widget, height_of_last_widget );
}


void AW_window::update_text_field( int *widget, const char *var_value ) {
    Widget w = (Widget ) widget;

    XtVaSetValues( w, XmNvalue, var_value, NULL );
}


/****************************************************************************************************************************/
/****************************************************************************************************************************/
/****************************************************************************************************************************/


AW_selection_list* AW_window::create_selection_list( const char *var_name, const char *label, const char *mnemonic, int columns, int rows  ) {
    AWUSE(mnemonic);
    Widget                     scrolledWindowList;
    Widget                     scrolledList;
    Widget                     l                     = 0;
    AW_variable_update_struct *vus;
    AW_cb_struct	          *cbs;
    int                        width_of_label        = 0;
    int                        width_of_list;
    int                        height_of_list;
    int                        width_of_last_widget  = 0;
    int                        height_of_last_widget = 0;

    if ( _at->label_for_inputfield ) {
        label = _at->label_for_inputfield;
    }

    check_at_pos();

    AW_awar *vs = 0;
    if (var_name) vs = root->awar(var_name);



    if ( label ) {
        width_of_label = this->calculate_string_width( calculate_label_length() );

        l = XtVaCreateManagedWidget( "label",
                                     xmLabelWidgetClass,
                                     INFO_WIDGET,
                                     XmNx, (int)10,
                                     XmNy, (int)(_at->y_for_next_button) + this->get_root()->y_correction_for_input_labels - 1,
                                     XmNwidth, (int)(width_of_label + 2),
                                     RES_CONVERT( XmNlabelString, label ),
                                     XmNrecomputeSize, AW_FALSE,
                                     XmNalignment, XmALIGNMENT_BEGINNING,
                                     NULL );
        width_of_label += 10;
    }


    width_of_list = this->calculate_string_width( columns ) + 9;
    height_of_list = this->calculate_string_height( rows, 4*rows ) + 9;


    if (_at->to_position_exists){
        width_of_list = _at->to_position_x - _at->x_for_next_button - width_of_label - 18;
        if ( _at->y_for_next_button  < _at->to_position_y - 18 ) {
            height_of_list = _at->to_position_y - _at->y_for_next_button - 18;
        }
        scrolledWindowList = XtVaCreateManagedWidget( "scrolledWindowList1",
                                                      xmScrolledWindowWidgetClass,
                                                      INFO_FORM,
                                                      XmNvisualPolicy, XmVARIABLE,
                                                      XmNscrollBarDisplayPolicy, XmSTATIC,
                                                      XmNshadowThickness, 0,
                                                      XmNfontList, 			p_global->fontlist,
                                                      NULL );
        aw_attach_widget(scrolledWindowList,_at);

        width_of_last_widget = _at->to_position_x - _at->x_for_next_button;
        height_of_last_widget = _at->to_position_y - _at->y_for_next_button;
    }else{
        scrolledWindowList = XtVaCreateManagedWidget( "scrolledWindowList1",
                                                      xmScrolledWindowWidgetClass,
                                                      INFO_WIDGET,
                                                      XmNscrollingPolicy, XmAPPLICATION_DEFINED,
                                                      XmNvisualPolicy, XmVARIABLE,
                                                      XmNscrollBarDisplayPolicy, XmSTATIC,
                                                      XmNshadowThickness, 0,
                                                      XmNx, (int)10,
                                                      XmNy, (int)(_at->y_for_next_button),
                                                      XmNfontList, p_global->fontlist,
                                                      NULL );
    }

    {
        int select_type = XmMULTIPLE_SELECT;
        if (vs) select_type = XmSINGLE_SELECT;

        scrolledList = XtVaCreateManagedWidget( "scrolledList1",
                                                xmListWidgetClass,
                                                scrolledWindowList,
                                                XmNwidth, (int)width_of_list,
                                                XmNheight, (int) height_of_list,
                                                XmNscrollBarDisplayPolicy, XmSTATIC,
                                                XmNselectionPolicy, select_type,
                                                XmNlistSizePolicy, XmCONSTANT,
                                                XmNfontList, p_global->fontlist,
                                                NULL );
    }

    if (!_at->to_position_exists){
        short height;
        XtVaGetValues( scrolledList, XmNheight, &height, NULL);
        height_of_last_widget = height + 20;
        width_of_last_widget = width_of_label + width_of_list + 20;

        switch(_at->correct_for_at_center){
            case 3:	break;
            case 0: // left centered
                XtVaSetValues(	scrolledWindowList, XmNx, (int)(_at->x_for_next_button + width_of_label), NULL );
                if ( label ) {
                    XtVaSetValues(	l, XmNx, (int)(_at->x_for_next_button), NULL );
                }
                break;

            case 1: // middle centered
                XtVaSetValues( scrolledWindowList, XmNx, (int)( _at->x_for_next_button - (width_of_last_widget/2) + width_of_label), NULL );
                if ( label ) {
                    XtVaSetValues( l, XmNx, (int)(_at->x_for_next_button - (width_of_last_widget/2) ), NULL );
                }
                width_of_last_widget = width_of_last_widget / 2;
                break;

            case 2: // right centered
                XtVaSetValues( scrolledWindowList, XmNx, (int)(_at->x_for_next_button - width_of_list - 18), NULL );
                if ( label ) {
                    XtVaSetValues( l, XmNx, (int)(_at->x_for_next_button - width_of_last_widget - 18), NULL );
                }
                width_of_last_widget = 0;
        }

    }

    {
        int type = GB_STRING;
        if (vs)  type = vs->variable_type;

        if ( p_global->selection_list ) {
            p_global->last_selection_list->next = new AW_selection_list( var_name, type, scrolledList );
            p_global->last_selection_list = p_global->last_selection_list->next;
        }else {
            p_global->last_selection_list = p_global->selection_list = new AW_selection_list( var_name, type, scrolledList );
        }
    }


    // user-own callback
    cbs = _callback;

    // callback for enter
    if (vs){
        vus = new AW_variable_update_struct( scrolledList, AW_WIDGET_SELECTION_LIST, vs, 0, 0, 0, cbs );
        vus->id = (void*)p_global->last_selection_list;

        XtAddCallback(scrolledList, XmNsingleSelectionCallback,
                      (XtCallbackProc) AW_variable_update_callback,
                      (XtPointer) vus );

        if ( _d_callback) {
            XtAddCallback(scrolledList, XmNdefaultActionCallback,
                          (XtCallbackProc) AW_server_callback,
                          (XtPointer) _d_callback );
        }
        AW_INSERT_BUTTON_IN_AWAR_LIST( vs,(AW_CL)p_global->last_selection_list, scrolledList, AW_WIDGET_SELECTION_LIST, this);
        AW_INSERT_BUTTON_IN_SENS_LIST ( root, _at->id_for_next_button, _at->mask_for_next_button, scrolledList );
    }

    this->unset_at_commands();
    this->increment_at_commands( width_of_last_widget, height_of_last_widget );
    return p_global->last_selection_list;
}

AW_selection_list *AW_window::create_multi_selection_list(const char *label, const char *mnemonic, int columns, int rows){
    return create_selection_list(0,label,mnemonic,columns,rows);
}

void AW_window::conc_list(AW_selection_list *from_list,AW_selection_list *to_list)
{
    AW_select_table_struct *from_list_table;

    from_list_table = from_list->list_table;
    while (from_list_table)
    {
        if (from_list->default_select != from_list_table)
        {
            if (! to_list->list_table)
                to_list->last_of_list_table = to_list->list_table = new AW_select_table_struct( from_list_table->displayed, from_list_table->char_value );
            else
            {
                to_list->last_of_list_table->next = new AW_select_table_struct( from_list_table->displayed, from_list_table->char_value );
                to_list->last_of_list_table = to_list->last_of_list_table->next;
                to_list->last_of_list_table->next = NULL;
            }
        }

        from_list_table = from_list_table->next;
    }

    clear_selection_list( from_list );
    insert_default_selection( from_list, "", "");
}

// --------------------------------------------------
// iterator through selection list:
// --------------------------------------------------

static AW_select_table_struct *current_list_table = 0;

void AW_window::init_list_entry_iterator(AW_selection_list *selection_list) {
    current_list_table = selection_list->list_table;
}

void AW_window::iterate_list_entry(int offset) {
    while (offset--) {
        if (!current_list_table) break;
        current_list_table = current_list_table->next;
    }
}

const char *AW_window::get_list_entry_char_value() {
    return current_list_table ? current_list_table->char_value : 0;
}

const char *AW_window::get_list_entry_displayed() {
    return current_list_table ? current_list_table->displayed : 0;
}

void AW_window::set_list_entry_char_value(const char *new_char_value) {
    if (!current_list_table) AW_ERROR("No Selection List Iterator");

    free(current_list_table->char_value);
    current_list_table->char_value = AW_select_table_struct::copy_string(new_char_value);
}

void AW_window::set_list_entry_displayed(const char *new_displayed) {
    if (!current_list_table) AW_ERROR("No Selection List Iterator");

    free(current_list_table->displayed);
    current_list_table->displayed = AW_select_table_struct::copy_string(new_displayed);
}

/*
const char *AW_window::get_list_entry(AW_selection_list *selection_list, int val)	//val = 1 initialisiert das Ganze, val=0 holt naechsten Eintrag
{
    static int				last		= 0;
    char 				*result;

    if (last == 1 && !val)
    {
	last = 2;
	if (!list_table)
	    return NULL;
	else
	    return list_table->char_value;;
    }

    if (last == 2 && !val)
	return NULL;

    if (val)
    {
	last = 0;
	list_table = NULL;
	return NULL;
    }

    if (!list_table)
	list_table = selection_list->list_table;

    if (!list_table)
    {
	last = 2;
	return NULL;
    }

    result = list_table->char_value;
    list_table = list_table->next;
    if (!list_table || list_table == selection_list->last_of_list_table)
	last = 1;
    return result;
    }
*/

void AW_window::set_selection_list_suffix(AW_selection_list *selection_list, const char *suffix){
    char filter[200];
    sprintf(filter,"tmp/save_box_sel_%li/filter",(long)selection_list);
    get_root()->awar_string(filter,suffix);
    sprintf(filter,"tmp/load_box_sel_%li/filter",(long)selection_list);
    get_root()->awar_string(filter,suffix);}

int AW_window::get_no_of_entries( AW_selection_list *selection_list )
{
    AW_select_table_struct *list_table;
    int		count = 0;
    for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ) count++;
    if( selection_list->default_select ) 	count++;

    return count;
}
/*-------------------- function to get index of an entry in the selection lists -------------------- */
int AW_window::get_index_of_element(AW_selection_list *selection_list, const char *selected_element){
    int no_of_elements = get_no_of_entries(selection_list);
    int element_index = 0;
 
    const char *listEntry = selection_list->first_element(); 
    if(!listEntry || !listEntry[0] || listEntry == NULL) return element_index = -1;

    for (int i=1;i<no_of_elements;i++){
        if (GBS_strscmp(listEntry,selected_element) == 0){
            element_index = i;
            return element_index;
        }
        listEntry = selection_list->next_element();             
    }
    return element_index = -1;
}

/*-------------------- function to get entry in the selection list for the index passed  -------------------- */
const char *AW_window::get_element_of_index(AW_selection_list *selection_list, int index){
    int no_of_elements = get_no_of_entries(selection_list);
    const char *element = 0;
 
    const char *listEntry = selection_list->first_element(); 
    if(!listEntry || !listEntry[0] || listEntry == NULL) return element = NULL;

    for (int i=1;i<no_of_elements;i++){
        if (index == i){
            element = strdup(listEntry);
            return element;
        }
        listEntry = selection_list->next_element();             
    }
    return element= NULL;
}

void AW_window::delete_selection_from_list( AW_selection_list *selection_list, const char *disp_string )
{
    AW_select_table_struct *list_table,
        *next = NULL,
        *prev = NULL;
    char	*ptr;
    int		count = 0;

    for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ) count++;
    if( selection_list->default_select ) 	count++;

    if (count == 2)		//Letzter Eintrag + default selection
        clear_selection_list( selection_list );

    for ( list_table 	= selection_list->list_table,
              next 		= selection_list->list_table;
          list_table;
          prev		= next,
              list_table	= list_table->next,
              next 		= list_table)
    {
        ptr = list_table->displayed;
        if ( strcmp( disp_string, ptr ) == 0 )
        {
            next = list_table->next;

            if (prev)
                prev->next = next;
            else
                selection_list->list_table = next;

            if (! list_table->next)
                if (prev)
                    selection_list->last_of_list_table = prev;

            if ( selection_list->default_select == list_table )
            {
                selection_list->default_select = NULL;
                insert_default_selection( selection_list, "", "");
            }

            delete list_table;
            return;
        }
    }
}

void AW_window::insert_selection( AW_selection_list *selection_list, const char *displayed, const char *value ) {

    if ( selection_list->variable_type != AW_STRING ) {
        AW_ERROR( "This selection list is only defined for strings" );
        return;
    }

    if ( selection_list->list_table ) {
        selection_list->last_of_list_table->next = new AW_select_table_struct( displayed, value );
        selection_list->last_of_list_table = selection_list->last_of_list_table->next;
        selection_list->last_of_list_table->next = NULL;
    }else {
        selection_list->last_of_list_table = selection_list->list_table = new AW_select_table_struct( displayed, value );
    }
}


void AW_window::insert_default_selection( AW_selection_list *selection_list, const char *displayed, const char *value ) {

    if ( selection_list->variable_type != AW_STRING ) {
        AW_ERROR( "This selection list is only defined for strings" );
        return;
    }
    if ( selection_list->default_select ) {
        delete selection_list->default_select;
    }
    selection_list->default_select = new AW_select_table_struct( displayed, value );
}


void AW_window::insert_selection( AW_selection_list *selection_list, const char *displayed, long value ) {

    if ( selection_list->variable_type != AW_INT ) {
        AW_ERROR( "This selection list is only defined for ints" );
        return;
    }
    if ( selection_list->list_table ) {
        selection_list->last_of_list_table->next = new AW_select_table_struct( displayed, value );
        selection_list->last_of_list_table = selection_list->last_of_list_table->next;
        selection_list->last_of_list_table->next = NULL;
    }else {
        selection_list->last_of_list_table = selection_list->list_table = new AW_select_table_struct( displayed, value );
    }
}


void AW_window::insert_default_selection( AW_selection_list *selection_list, const char *displayed, long value ) {

    if ( selection_list->variable_type != AW_INT ) {
        AW_ERROR( "This selection list is only defined for ints" );
        return;
    }
    if ( selection_list->default_select ) {
        delete selection_list->default_select;
    }
    selection_list->default_select = new AW_select_table_struct( displayed, value );
}


void AW_window::clear_selection_list( AW_selection_list *selection_list ) {
    AW_select_table_struct *list_table;
    AW_select_table_struct *help;


    for ( help = selection_list->list_table; help;  ) {
        list_table = help;
        help = list_table->next;
        delete list_table;
    }
    if ( selection_list->default_select ) {
        delete selection_list->default_select;
    }

    selection_list->list_table		= NULL;
    selection_list->last_of_list_table	= NULL;
    selection_list->default_select		= NULL;


}

inline XmString XmStringCreateSimple_wrapper(const char *text) {
    return XmStringCreateSimple((char*)text);
}

void AW_window::update_selection_list( AW_selection_list * selection_list ) {

    AW_select_table_struct *list_table;
    int count = 0;


    count = 0;
    int i;
    for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ) count++;
    if( selection_list->default_select ) 	count++;

    XmString *strtab = new XmString[count];

    count = 0;
    const char *s2;
    for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ){
        s2 = list_table->displayed;
        if (!strlen(s2)) s2 = "  ";
        strtab[count] = XmStringCreateSimple_wrapper(s2);
        count++;
    }
    if( selection_list->default_select ) {
        s2 = selection_list->default_select->displayed;
        if (!strlen(s2)) s2 = "  ";
        strtab[count] = XmStringCreateSimple_wrapper(s2);
        count++;
    }
    if (!count){
        strtab[count] = XmStringCreateSimple_wrapper("   ");
        count ++;
    }

    XtVaSetValues(	selection_list->select_list_widget, XmNitemCount, count, XmNitems, strtab , NULL );

    update_selection_list_intern( selection_list );

    for (i=0; i<count; i++) XmStringFree(strtab[i]);
    delete strtab;

}


void AW_window::update_selection_list_intern( AW_selection_list *selection_list ) {
    AW_select_table_struct *list_table;

    int			pos = 0;
    char			*global_var_value = NULL;
    long			global_var_int_value;
    float			global_var_float_value;

    if (!selection_list->variable_name) return; // not connected to awar
    AW_BOOL found = AW_FALSE;
    pos = 0;
    switch ( selection_list->variable_type ) {
        case AW_STRING:	global_var_value = root->awar( selection_list->variable_name )->read_string();
            for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ) {
                if ( strcmp( global_var_value, list_table->char_value ) == 0 ) {
                    found = AW_TRUE;
                    break;
                }
                pos++;
            }
            break;
        case AW_INT:	global_var_int_value = root->awar( selection_list->variable_name )->read_int();
            for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ) {
                if ( global_var_int_value == list_table->int_value ) {
                    found = AW_TRUE;
                    break;
                }
                pos++;
            }
            break;
        case AW_FLOAT:	global_var_float_value = root->awar( selection_list->variable_name )->read_float();
            for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ) {
                if ( global_var_float_value == list_table->float_value ) {
                    found = AW_TRUE;
                    break;
                }
                pos++;
            }
            break;
        default:
            GB_warning("Unknown AWAR type");
            break;
    }

    if (found || selection_list->default_select) {
        pos++;
        int top;
        int vis;
        XtVaGetValues(selection_list->select_list_widget,
                      XmNvisibleItemCount, &vis,
                      XmNtopItemPosition, &top,
                      NULL);
        XmListSelectPos(selection_list->select_list_widget,pos,False);

        if (pos < top) {
            if (pos > 1) pos --;
            XmListSetPos(selection_list->select_list_widget,pos);
        }
        if (pos >= top + vis) {
            XmListSetBottomPos(selection_list->select_list_widget,pos + 1 );
        }

    }else{
        AW_ERROR("Selection list '%s' has no default selection",selection_list->variable_name);
    }
    delete global_var_value;
}

void AW_selection_list::selectAll(){
    int i;
    AW_select_table_struct *lt;
    for (i=0,lt = list_table; lt; i++,lt = lt->next){
        XmListSelectPos(select_list_widget,i,False);
    }
    if (default_select){
        XmListSelectPos(select_list_widget,i,False);
    }
}

void AW_selection_list::deselectAll(){
    XmListDeselectAllItems(select_list_widget);
}

const char *AW_selection_list::first_element(){
    AW_select_table_struct *lt;
    for (lt = list_table; lt; lt = lt->next){
        lt->is_selected = 1;
    }
    loop_pntr = list_table;
    return loop_pntr->char_value;
}

const char *AW_selection_list::next_element(){
    if (!loop_pntr) return 0;
    loop_pntr = loop_pntr->next;
    if (!loop_pntr) return 0;
    while (loop_pntr && !loop_pntr->is_selected) loop_pntr=loop_pntr->next;
    if (!loop_pntr) return 0;
    return loop_pntr->char_value;
}

const char *AW_selection_list::first_selected(){
    int i;
    AW_select_table_struct *lt;
    loop_pntr = 0;
    for (i=1,lt = list_table; lt; i++,lt = lt->next){
        lt->is_selected = XmListPosSelected(select_list_widget,i);
        if (lt->is_selected && !loop_pntr) loop_pntr = lt;
    }
    if (default_select){
        default_select->is_selected = XmListPosSelected(select_list_widget,i);
        if (default_select->is_selected && !loop_pntr) loop_pntr = lt;
    }
    if (!loop_pntr) return 0;
    return loop_pntr->char_value;
}


GB_ERROR AW_window::save_selection_list( AW_selection_list * selection_list, const char *filename, long number_of_lines) {
    // number_of_lines == 0		-> print all
    AW_select_table_struct *list_table;
    FILE                   *fd;
    char	               *sep;

    fd = fopen( filename, "w");
    if (!fd) {
        return GB_export_IO_error("writing", filename);
    }
    for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ) {
        number_of_lines--;
        if ( (sep = strstr(list_table->displayed,"#")))		//'#' ist ein SEPARATOR
        {
            *sep = 0;
            fprintf( fd, "%s,",list_table->displayed );
            *sep = '#';
            sep = sep + strlen("#");
            fprintf( fd, "%s\n",sep );
        }
        else if (fprintf( fd, "%s\n",list_table->displayed )<0){
            aw_message("Disc Full");
            break;
        }
        if (!number_of_lines) break;
    }
    fclose( fd );
    return 0;
}

char *AW_window::get_selection_list_contents( AW_selection_list * selection_list, long number_of_lines) {
    // number_of_lines == 0		-> print all
    AW_select_table_struct *list_table;
    void *fd;

    fd = GBS_stropen(10000);

    for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ) {
        number_of_lines--;
        GBS_strcat(fd,list_table->displayed);
        GBS_chrcat(fd,'\n');
        if (!number_of_lines) break;
    }
    return GBS_strclose(fd,0);
}


GBDATA_SET *AW_window::selection_list_to_species_set(GBDATA *gb_main,AW_selection_list *selection_list){
    AW_select_table_struct *list_table;
    GB_transaction dummy(gb_main);

    GBDATA *gb_species_data = GB_search(gb_main,	"species_data",GB_CREATE_CONTAINER);
    int counter = 0;
    for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ) {
        counter ++;
    }
    GBDATA_SET *set = GB_create_set(counter);

    for ( list_table = selection_list->list_table; list_table; list_table = list_table->next ) {
        GBDATA *gb_species = GBT_find_species_rel_species_data(gb_species_data,list_table->char_value);
        if (!gb_species) continue;
        GB_add_set(set,gb_species);
    }
    return set;
}

static int AW_sort_backward = 0;


#ifdef __cplusplus
extern "C"
#endif
int AW_sort_AW_select_table_struct(AW_select_table_struct *t1,AW_select_table_struct *t2) {
    int cmp = strcmp(t1->displayed,t2->displayed);
    return AW_sort_backward ? -cmp : cmp;
}

void AW_window::sort_selection_list( AW_selection_list * selection_list, int backward) {

    AW_select_table_struct *list_table;

    long count = 0;
    for ( list_table = selection_list->list_table; list_table; list_table = list_table->next){
        count ++;
    }
    if (!count) return;

    AW_select_table_struct **tables = new AW_select_table_struct *[count];
    count = 0;
    for ( list_table = selection_list->list_table; list_table; list_table = list_table->next){
        tables[count] = list_table;
        count ++;
    }
    AW_sort_backward = backward;
    GB_mergesort((void**)tables,0,count, (gb_compare_two_items_type)AW_sort_AW_select_table_struct,0);
    long i;
    for (i=0;i<count-1;i++) {
        tables[i]->next = tables[i+1];
    }
    tables[i]->next = 0;
    selection_list->list_table = tables[0];
    selection_list->last_of_list_table = tables[i];

    delete tables;
    return;
}

GB_ERROR AW_window::load_selection_list( AW_selection_list *selection_list, const char *filename) {

    char *nl;
    char *ko;
    char *pl;

    this->clear_selection_list(selection_list);
    char **fnames = GBS_read_dir(filename,0);
    char **fname;
    for (fname = fnames; *fname; fname++){
        char *data = GB_read_file(*fname);
        if (!data){
            GB_print_error();
            continue;
        }

        for (pl = data; pl; pl = nl) {
            ko              = strchr(pl,',');
            if (ko) *(ko++) = 0;
            else ko         = pl;
            while (*ko == ' ' || *ko == '\t') ko++;

            nl              = strchr(ko,'\n');
            if (nl) *(nl++) = 0;

            if (ko[0] && pl[0] != '#') this->insert_selection(selection_list,pl,ko);
        }
        delete data;
    }
    GBS_free_names(fnames);

    this->insert_default_selection(selection_list,"","");
    this->update_selection_list(selection_list);
    return 0;
}

// --------------------------------------------------------------------------------
// 	Options-Menu
// --------------------------------------------------------------------------------

void AW_window::create_option_menu( const char *var_name, AW_label label, const char *mnemonic ) {
    Widget optionMenu_shell;
    Widget optionMenu;
    Widget optionMenu1;
    int x_for_position_of_menu;

    if ( _at->label_for_inputfield ) {
        label = _at->label_for_inputfield;
    }

    check_at_pos();

    if ( _at->correct_for_at_center ) {
//         _at->correct_for_at_center_intern = _at->correct_for_at_center;
        if ( label ) {
            _at->saved_x = _at->x_for_next_button;
            x_for_position_of_menu = 10;
        }
        else {
            _at->saved_x = _at->x_for_next_button;
            x_for_position_of_menu = 10;
        }
    }
    else {
        if ( label ) {
            x_for_position_of_menu = _at->x_for_next_button - 3;
        }
        else {
            x_for_position_of_menu = _at->x_for_next_button - 3 - 7;
        }
    }


    AW_awar *vs = root->awar(var_name);


    optionMenu_shell = XtVaCreatePopupShell ("optionMenu shell",
                                             xmMenuShellWidgetClass,
                                             INFO_WIDGET,
                                             XmNwidth, 1,
                                             XmNheight, 1,
                                             XmNallowShellResize, AW_TRUE,
                                             XmNoverrideRedirect, AW_TRUE,
                                             XmNfontList, p_global->fontlist,
                                             NULL );

    optionMenu = XtVaCreateWidget( "optionMenu_p1",
                                   xmRowColumnWidgetClass,
                                   optionMenu_shell,
                                   XmNrowColumnType, XmMENU_PULLDOWN,
                                   XmNfontList, p_global->fontlist,
                                   NULL );


    if ( label ) {
        char *help_label;
        help_label = this->align_string( label, calculate_label_length() );
        if (mnemonic && mnemonic[0] && strchr(label,mnemonic[0])){
            optionMenu1 = XtVaCreateManagedWidget( "optionMenu1",
                                                   xmRowColumnWidgetClass,
                                                   INFO_WIDGET,
                                                   XmNrowColumnType, XmMENU_OPTION,
                                                   XmNsubMenuId, optionMenu,
                                                   XmNfontList, p_global->fontlist,
                                                   XmNx, (int)x_for_position_of_menu,
                                                   XmNy, (int)(_at->y_for_next_button - 5),
                                                   RES_CONVERT( XmNlabelString, help_label ),
                                                   RES_CONVERT( XmNmnemonic, mnemonic ),
                                                   NULL );
        }else{
            optionMenu1 = XtVaCreateManagedWidget( "optionMenu1",
                                                   xmRowColumnWidgetClass,
                                                   INFO_WIDGET,
                                                   XmNrowColumnType, XmMENU_OPTION,
                                                   XmNsubMenuId, optionMenu,
                                                   XmNfontList, p_global->fontlist,
                                                   XmNx, (int)x_for_position_of_menu,
                                                   XmNy, (int)(_at->y_for_next_button - 5),
                                                   RES_CONVERT( XmNlabelString, help_label ),
                                                   NULL );
        }
        delete help_label;
    }else {
        optionMenu1 = XtVaCreateManagedWidget( "optionMenu1",
                                               xmRowColumnWidgetClass,
                                               (_at->attach_any)?INFO_FORM:INFO_WIDGET,
                                               XmNrowColumnType, XmMENU_OPTION,
                                               XmNsubMenuId, optionMenu,
                                               XmNfontList, p_global->fontlist,
                                               XmNx, (int)x_for_position_of_menu,
                                               XmNy, (int)(_at->y_for_next_button - 5),
                                               RES_CONVERT( XmNlabelString, "" ),
                                               NULL );
        if (_at->attach_any){
            aw_attach_widget(optionMenu1,_at);
        }
    }



    p_w->option_menu = optionMenu;
    delete p_w->option_menu_var_name;
    p_w->option_menu_var_name = strdup( var_name );
    p_w->option_menu_var_type = vs->variable_type;

    get_root()->number_of_option_menues++;

    if ( p_global->option_menu_list ) {
        p_global->last_option_menu->next =
            new AW_option_menu_struct(get_root()->number_of_option_menues, _at->id_for_next_button, var_name, vs->variable_type, optionMenu1, _at->x_for_next_button - 7, _at->y_for_next_button, _at->correct_for_at_center);
        p_global->last_option_menu = p_global->last_option_menu->next;
    }
    else {
        p_global->last_option_menu = p_global->option_menu_list =
            new AW_option_menu_struct( get_root()->number_of_option_menues, _at->id_for_next_button, var_name, vs->variable_type, optionMenu1,  _at->x_for_next_button - 7, _at->y_for_next_button, _at->correct_for_at_center);
    }

    AW_INSERT_BUTTON_IN_AWAR_LIST( vs,get_root()->number_of_option_menues, optionMenu, AW_WIDGET_CHOICE_MENU, this);
    AW_INSERT_BUTTON_IN_SENS_LIST ( root, _at->id_for_next_button, _at->mask_for_next_button, optionMenu1 );
}

void *AW_window::_create_option_entry(AW_VARIABLE_TYPE type, const char *name, const char *mnemonic,const char *name_of_color ){
    AWUSE(mnemonic);
    Widget entry;
    if ( p_w->option_menu_var_type != type ) {
        AW_ERROR("Option menu not defined for this type");
    }
    Pixel bg_color = 0;
    const char *color_switch = 0;
    if (name_of_color){
        color_switch = XmNbackground;
        XColor unused;
        XColor color;
        if( XAllocNamedColor(p_global->display,p_global->colormap,name_of_color,&color,&unused) == 0) {
            fprintf(stderr,"XAllocColor failed: %s\n",name_of_color);
            color_switch = 0;
        }else{
            bg_color = color.pixel;
        }
    }

    entry = XtVaCreateManagedWidget( "optionMenu_entry",
                                     xmPushButtonWidgetClass,
                                     p_w->option_menu,
                                     RES_LABEL_CONVERT(((char *)name)),
                                     XmNfontList, p_global->fontlist,
                                     color_switch, bg_color,
                                     NULL );
    AW_LABEL_IN_AWAR_LIST(this,entry,name);
    return (void *)entry;
}


// for string
void AW_window::insert_option( AW_label option_name, const char *mnemonic, const char *var_value,const char *name_of_color ) {
    Widget 		entry;
    AW_cb_struct	*cbs;
    if (p_w->option_menu_var_type != AW_STRING){
        AW_ERROR("insert_option(..,..,string) used for non string awar");
        return;
    }

    entry = (Widget)_create_option_entry(AW_STRING,option_name,mnemonic,name_of_color);

    // user-own callback
    cbs = _callback;

    // callback for new choice
    XtAddCallback( entry, XmNactivateCallback,
                   (XtCallbackProc) AW_variable_update_callback,
                   (XtPointer) new AW_variable_update_struct(
                                                             NULL, AW_WIDGET_CHOICE_MENU, root->awar(p_w->option_menu_var_name),
                                                             var_value, 0, 0, cbs ) );


    if ( p_global->last_option_menu->first_choice ) {
        p_global->last_option_menu->last_choice->next = new AW_option_struct( var_value, entry );
        p_global->last_option_menu->last_choice = p_global->last_option_menu->last_choice->next;
    }
    else {
        p_global->last_option_menu->last_choice = p_global->last_option_menu->first_choice = new AW_option_struct( var_value, entry );
    }

    AW_INSERT_BUTTON_IN_SENS_LIST ( root, _at->id_for_next_button, _at->mask_for_next_button, entry );

    this->unset_at_commands();

}

// for string
void AW_window::insert_default_option( AW_label option_name, const char *mnemonic, const char *var_value,const char *name_of_color ) {
    Widget 			entry;
    AW_cb_struct	*cbs;
    if (p_w->option_menu_var_type != AW_STRING){
        AW_ERROR("insert_option(..,..,string) used for non string awar");
        return;
    }

    entry = (Widget)_create_option_entry(AW_STRING,option_name,mnemonic,name_of_color);

    // user-own callback
    cbs = _callback;

    // callback for new choice
    XtAddCallback( entry, XmNactivateCallback,
                   (XtCallbackProc) AW_variable_update_callback,
                   (XtPointer) new AW_variable_update_struct( NULL, AW_WIDGET_CHOICE_MENU, root->awar(p_w->option_menu_var_name), var_value, 0, 0, cbs ) );


    p_global->last_option_menu->default_choice = new AW_option_struct( var_value, entry );

    AW_INSERT_BUTTON_IN_SENS_LIST ( root, _at->id_for_next_button, _at->mask_for_next_button, entry );

    this->unset_at_commands();

}


// for int
void AW_window::insert_option( AW_label option_name, const char *mnemonic, int var_value,const char *name_of_color ) {
    Widget 			entry;
    AW_cb_struct	*cbs;
    if (p_w->option_menu_var_type != AW_INT){
        AW_ERROR("insert_option(..,..,string) used for non int awar");
        return;
    }

    entry = (Widget)_create_option_entry(AW_INT,option_name,mnemonic,name_of_color);

    // user-own callback
    cbs = _callback;

    // callback for new choice
    XtAddCallback( entry, XmNactivateCallback,
                   (XtCallbackProc) AW_variable_update_callback,
                   (XtPointer) new AW_variable_update_struct(
                                                             NULL, AW_WIDGET_CHOICE_MENU, root->awar(p_w->option_menu_var_name),
                                                             0, var_value, 0, cbs ) );


    if ( p_global->last_option_menu->first_choice ) {
        p_global->last_option_menu->last_choice->next = new AW_option_struct( var_value, entry );
        p_global->last_option_menu->last_choice = p_global->last_option_menu->last_choice->next;
    }
    else {
        p_global->last_option_menu->last_choice = p_global->last_option_menu->first_choice = new AW_option_struct( var_value, entry );
    }

    AW_INSERT_BUTTON_IN_SENS_LIST ( root, _at->id_for_next_button, _at->mask_for_next_button, entry );

    this->unset_at_commands();

}

// for int
void AW_window::insert_default_option( AW_label option_name, const char *mnemonic, int var_value,const char *name_of_color ) {
    Widget 			entry;
    AW_cb_struct	*cbs;
    if (p_w->option_menu_var_type != AW_INT){
        AW_ERROR("insert_option(..,..,string) used for non int awar");
        return;
    }

    entry = (Widget)_create_option_entry(AW_INT,option_name,mnemonic,name_of_color);

    // user-own callback
    cbs = _callback;

    // callback for new choice
    XtAddCallback( entry, XmNactivateCallback,
                   (XtCallbackProc) AW_variable_update_callback,
                   (XtPointer) new AW_variable_update_struct( NULL, AW_WIDGET_CHOICE_MENU, root->awar(p_w->option_menu_var_name), 0, var_value, 0, cbs ) );

    p_global->last_option_menu->default_choice = new AW_option_struct( var_value, entry );

    AW_INSERT_BUTTON_IN_SENS_LIST ( root, _at->id_for_next_button, _at->mask_for_next_button, entry );

    this->unset_at_commands();

}



// for float
void AW_window::insert_option( AW_label option_name, const char *mnemonic, float var_value, const char *name_of_color ) {
    Widget 			entry;
    AW_cb_struct	*cbs;
    if (p_w->option_menu_var_type != AW_FLOAT){
        AW_ERROR("insert_option(..,..,string) used for non float awar");
        return;
    }

    entry = (Widget)_create_option_entry(AW_FLOAT,option_name,mnemonic,name_of_color);

    // user-own callback
    cbs = _callback;

    // callback for new choice
    XtAddCallback( entry, XmNactivateCallback,
                   (XtCallbackProc) AW_variable_update_callback,
                   (XtPointer) new AW_variable_update_struct( NULL, AW_WIDGET_CHOICE_MENU, root->awar(p_w->option_menu_var_name), 0, 0, var_value, cbs ) );


    if ( p_global->last_option_menu->first_choice ) {
        p_global->last_option_menu->last_choice->next = new AW_option_struct( var_value, entry );
        p_global->last_option_menu->last_choice = p_global->last_option_menu->last_choice->next;
    }
    else {
        p_global->last_option_menu->last_choice = p_global->last_option_menu->first_choice = new AW_option_struct( var_value, entry );
    }

    AW_INSERT_BUTTON_IN_SENS_LIST ( root, _at->id_for_next_button, _at->mask_for_next_button, entry );

    this->unset_at_commands();

}

// for float
void AW_window::insert_default_option( AW_label option_name, const char *mnemonic, float var_value, const char *name_of_color ) {
    Widget 			entry;
    AW_cb_struct	*cbs;

    if (p_w->option_menu_var_type != AW_FLOAT){
        AW_ERROR("insert_option(..,..,string) used for non float awar");
        return;
    }
    entry = (Widget)_create_option_entry(AW_FLOAT,option_name,mnemonic,name_of_color);

    // user-own callback
    cbs = _callback;

    // callback for new choice
    XtAddCallback( entry, XmNactivateCallback,
                   (XtCallbackProc) AW_variable_update_callback,
                   (XtPointer) new AW_variable_update_struct( NULL,
                                                              AW_WIDGET_CHOICE_MENU, root->awar(p_w->option_menu_var_name),
                                                              0, 0, var_value, cbs ) );


    p_global->last_option_menu->default_choice = new AW_option_struct( var_value, entry );

    AW_INSERT_BUTTON_IN_SENS_LIST ( root, _at->id_for_next_button, _at->mask_for_next_button, entry );

    this->unset_at_commands();

}

void AW_window::update_option_menu( void ) {

    this->update_option_menu( get_root()->number_of_option_menues );
}


void AW_window::update_option_menu( int option_menu_number ) {
    AW_option_menu_struct *option_menu_list;
    AW_option_struct	  *menu_choice_list;
    AW_BOOL 			   cont;
    char				  *global_var_value       = NULL;
    long				   global_var_int_value   = 0;
    float				   global_var_float_value = 0;
    int	                   width_of_last_widget;
    int	                   height_of_last_widget;


    for ( 	option_menu_list = p_global->option_menu_list;
            option_menu_list;
            option_menu_list = option_menu_list->next)
    {
        if ( option_menu_number == option_menu_list->option_menu_number ) {
            break;
        }
    }
    if ( !option_menu_list ){
        AW_ERROR("option menu does not exist");
        return;
    }
    char *var_name = option_menu_list->variable_name;

    if ( get_root()->changer_of_variable == (long)option_menu_list->label_widget ) {
        return;
    }

    cont = AW_TRUE;

    switch ( option_menu_list->variable_type ) {
        case AW_STRING:	global_var_value = root->awar( var_name )->read_string();
            break;
        case AW_INT:	global_var_int_value = root->awar( var_name )->read_int();
            break;
        case AW_FLOAT:	global_var_float_value = root->awar( var_name )->read_float();
            break;
        default:
            break;
    }

    menu_choice_list = option_menu_list->first_choice;
    while ( menu_choice_list && cont ) {
        switch ( option_menu_list->variable_type ) {
            case AW_STRING:	if ( (strcmp( global_var_value, menu_choice_list->variable_value ) == 0) ) {
                cont = AW_FALSE;
            }
            break;
            case AW_INT:		if ( global_var_int_value == menu_choice_list->variable_int_value ) {
                cont = AW_FALSE;
            }
            break;
            case AW_FLOAT:		if ( global_var_float_value == menu_choice_list->variable_float_value ) {
                cont = AW_FALSE;
            }
            break;
            default:
                GB_warning("Unknown AWAR type");
        }
        if ( cont ) {
            menu_choice_list = menu_choice_list->next;
        }
    }



    if (  menu_choice_list ) {
        XtVaSetValues( option_menu_list->label_widget, XmNmenuHistory, menu_choice_list->choice_widget, NULL );
    }else {
        if ( option_menu_list->default_choice ) {
            XtVaSetValues( option_menu_list->label_widget, XmNmenuHistory,
                           option_menu_list->default_choice->choice_widget, NULL );
#if 0
        }else {
            char *error = 0;
            menu_choice_list = option_menu_list->first_choice;
            switch ( option_menu_list->variable_type ) {
                case AW_STRING:
                    error = root->awar(var_name)->write_string(menu_choice_list->variable_value);
                    error = root->awar(option_menu_list->variable_name)->write_string(menu_choice_list->variable_float_value);
                    break;
                case AW_INT:
                    error = root->awar(var_name)->write_int(menu_choice_list->variable_int_value);

                    break;
                case AW_FLOAT:
                    error = root->awar(var_name)->write_float(menu_choice_list->variable_float_value);
                    break;
                default: break;
            }
            fprintf(stderr,"Warning Option Menu dont allow value for awar '%s'\n",var_name);
#endif
        }
    }


    delete global_var_value;

    short length;
    short height;
    XtVaGetValues( option_menu_list->label_widget ,XmNwidth ,&length ,XmNheight, &height, NULL );
    width_of_last_widget              = length;
    height_of_last_widget             = height;

    if ( option_menu_list->correct_for_at_center_intern ) {
        if ( option_menu_list->correct_for_at_center_intern == 1 ) { // middle centered
            XtVaSetValues( option_menu_list->label_widget, XmNx,(short)((short)_at->saved_x - (short)(length/2)), NULL );
            width_of_last_widget = width_of_last_widget / 2;
        }
        if ( option_menu_list->correct_for_at_center_intern == 2 ) { // right centered
            XtVaSetValues( option_menu_list->label_widget, XmNx,(short)((short)_at->saved_x - length) + 7, NULL );
            width_of_last_widget = 0;
        }
//         _at->correct_for_at_center_intern = 0;
    }
    width_of_last_widget -= 4;

    this->unset_at_commands();
    this->increment_at_commands( width_of_last_widget, height_of_last_widget );
}


/*********************************************************************************************************************************/
/*********************************************************************************************************************************/
/*********************************************************************************************************************************/

void AW_window::create_toggle_field( const char *var_name, AW_label labeli, const char *mnemonic ) {
    AWUSE(mnemonic);
    if (labeli) this->label(labeli);
    this->create_toggle_field(var_name);
}


void AW_window::create_toggle_field( const char *var_name, int orientation ) {
    /* orientation = 0 -> vertical else horizontal layout */

    Widget label_for_toggle;
    Widget toggle_field;
    int x_correcting_for_label = 0;
    int width_of_label = 0;
    int x_for_position_of_option = 0;
    const char *label ="";
    if ( _at->label_for_inputfield ) {
        label = _at->label_for_inputfield;
    }


    check_at_pos();

    AW_awar *vs = root->awar(var_name);

    if ( _at->correct_for_at_center ) {
//         _at->correct_for_at_center_intern = _at->correct_for_at_center;
        _at->saved_x = _at->x_for_next_button;
        x_for_position_of_option = 10;
    }else {
        x_for_position_of_option = _at->x_for_next_button;
    }


    if ( label ) {
        width_of_label = this->calculate_string_width( this->calculate_label_length() );
        label_for_toggle = XtVaCreateManagedWidget( "label",
                                                    xmLabelWidgetClass,
                                                    INFO_WIDGET,
                                                    XmNx, (int)_at->x_for_next_button,
                                                    XmNy, (int)(_at->y_for_next_button) + this->get_root()->y_correction_for_input_labels,
                                                    XmNwidth, (int)(width_of_label + 2),
                                                    RES_CONVERT( XmNlabelString, label ),
                                                    XmNrecomputeSize, AW_FALSE,
                                                    XmNalignment, XmALIGNMENT_BEGINNING,
                                                    XmNfontList, p_global->fontlist,
                                                    NULL );

        _at->saved_x_correction_for_label = x_correcting_for_label = width_of_label + 10;

        p_w->toggle_label = label_for_toggle;
    } else {
        p_w->toggle_label = NULL;
        _at->saved_x_correction_for_label = 0;
    }

    if (orientation) {
        toggle_field = XtVaCreateManagedWidget( "rowColumn for toggle field",
                                                xmRowColumnWidgetClass,
                                                (_at->attach_any)?INFO_FORM:INFO_WIDGET,
                                                XmNorientation,XmHORIZONTAL,
                                                XmNx, (int)(x_for_position_of_option + x_correcting_for_label),
                                                XmNy, (int)(_at->y_for_next_button - 2),
                                                XmNradioBehavior, True,
                                                XmNradioAlwaysOne, True,
                                                XmNfontList, p_global->fontlist,
                                                NULL );
    }else{
        toggle_field = XtVaCreateManagedWidget( "rowColumn for toggle field",
                                                xmRowColumnWidgetClass,
                                                (_at->attach_any)?INFO_FORM:INFO_WIDGET,
                                                XmNx, (int)(x_for_position_of_option + x_correcting_for_label),
                                                XmNy, (int)(_at->y_for_next_button - 2),
                                                XmNradioBehavior, True,
                                                XmNradioAlwaysOne, True,
                                                XmNfontList, p_global->fontlist,
                                                NULL );
    }
    if (_at->attach_any){
        aw_attach_widget(toggle_field,_at,300);
    }

    p_w->toggle_field = toggle_field;
    delete (p_w->toggle_field_var_name);
    p_w->toggle_field_var_name = strdup(var_name );
    p_w->toggle_field_var_type = vs->variable_type;

    get_root()->number_of_toggle_fields++;

    if ( p_global->toggle_field_list ) {
        p_global->last_toggle_field->next = new AW_toggle_field_struct( get_root()->number_of_toggle_fields, var_name, vs->variable_type, toggle_field, _at->correct_for_at_center);
        p_global->last_toggle_field = p_global->last_toggle_field->next;
    }else {
        p_global->last_toggle_field = p_global->toggle_field_list = new AW_toggle_field_struct( get_root()->number_of_toggle_fields, var_name, vs->variable_type, toggle_field, _at->correct_for_at_center);
    }

    AW_INSERT_BUTTON_IN_AWAR_LIST( vs,get_root()->number_of_toggle_fields, toggle_field, AW_WIDGET_TOGGLE_FIELD, this);
    AW_INSERT_BUTTON_IN_SENS_LIST ( root, _at->id_for_next_button, _at->mask_for_next_button, toggle_field );

}

Widget _aw_create_toggle_entry(AW_window *aww,Widget toggle_field,
                               const char *label,const char *mnemonic,
                               AW_variable_update_struct *awus,
                               AW_toggle_struct *awts, AW_BOOL default_toggle){
    AW_root *root = aww->get_root();

    Widget			toggleButton;

    toggleButton = XtVaCreateManagedWidget( "toggleButton",
                                            xmToggleButtonWidgetClass,
                                            toggle_field,
                                            RES_LABEL_CONVERT2(label,aww),
                                            RES_CONVERT( XmNmnemonic, mnemonic ),
                                            XmNindicatorSize, 16,
                                            XmNfontList, p_global->fontlist,

                                            NULL );
    awts->toggle_widget = toggleButton;
    awus->widget = toggleButton;
    XtAddCallback( toggleButton, XmNvalueChangedCallback,
                   (XtCallbackProc) AW_variable_update_callback,
                   (XtPointer) awus );
    if (default_toggle) {
        delete p_global->last_toggle_field->default_toggle;
        p_global->last_toggle_field->default_toggle = awts;
    }else {
        if ( p_global->last_toggle_field->first_toggle ) {
            p_global->last_toggle_field->last_toggle->next = awts;
            p_global->last_toggle_field->last_toggle = awts;
        }else {
            p_global->last_toggle_field->last_toggle = awts;
            p_global->last_toggle_field->first_toggle = awts;
        }
    }
    AW_INSERT_BUTTON_IN_SENS_LIST ( root, aww->_at->id_for_next_button,
                                    aww->_at->mask_for_next_button, toggleButton );

    aww->unset_at_commands();
    return	toggleButton;
}


// for string
void AW_window::insert_toggle( AW_label toggle_label, const char *mnemonic, const char *var_value ) {

    if ( p_w->toggle_field_var_type != AW_STRING ) {
        AW_ERROR("This toggle field only defined for strings");
        return;
    }

    _aw_create_toggle_entry(this,p_w->toggle_field,toggle_label,mnemonic,
                            new AW_variable_update_struct( NULL,
                                                           AW_WIDGET_TOGGLE_FIELD, root->awar(p_w->toggle_field_var_name),
                                                           var_value, 0, 0, _callback ),
                            new AW_toggle_struct( var_value, 0 ),
                            AW_FALSE );
}


// for string
void AW_window::insert_default_toggle( AW_label toggle_label, const char *mnemonic, const char *var_value ) {
    if ( p_w->toggle_field_var_type != AW_STRING ) {
        AW_ERROR("This toggle field only defined for strings");
        return;
    }
    _aw_create_toggle_entry(this,p_w->toggle_field,toggle_label,mnemonic,
                            new AW_variable_update_struct( NULL,
                                                           AW_WIDGET_TOGGLE_FIELD, root->awar(p_w->toggle_field_var_name),
                                                           var_value, 0, 0, _callback ),
                            new AW_toggle_struct( var_value, 0 ),
                            AW_TRUE );
}



// for int
void AW_window::insert_toggle( AW_label toggle_label, const char *mnemonic, int var_value ) {

    if ( p_w->toggle_field_var_type != AW_INT ) {
        AW_ERROR("This toggle field only defined for int");
        return;
    }
    _aw_create_toggle_entry(this,p_w->toggle_field,toggle_label,mnemonic,
                            new AW_variable_update_struct( NULL,
                                                           AW_WIDGET_TOGGLE_FIELD, root->awar(p_w->toggle_field_var_name),
                                                           0, var_value, 0, _callback ),
                            new AW_toggle_struct( var_value, 0 ),
                            AW_FALSE );
}


// for int
void AW_window::insert_default_toggle( AW_label toggle_label, const char *mnemonic, int var_value ) {
    if ( p_w->toggle_field_var_type != AW_INT ) {
        AW_ERROR("This toggle field only defined for int");
        return;
    }
    _aw_create_toggle_entry(this,p_w->toggle_field,toggle_label,mnemonic,
                            new AW_variable_update_struct( NULL,
                                                           AW_WIDGET_TOGGLE_FIELD, root->awar(p_w->toggle_field_var_name),
                                                           0, var_value, 0, _callback ),
                            new AW_toggle_struct( var_value, 0 ),
                            AW_TRUE );
}


// for float
void AW_window::insert_toggle( AW_label toggle_label, const char *mnemonic, float var_value ) {
    if ( p_w->toggle_field_var_type != AW_FLOAT ) {
        AW_ERROR("This toggle field not defined for float");
        return;
    }
    _aw_create_toggle_entry(this,p_w->toggle_field,toggle_label,mnemonic,
                            new AW_variable_update_struct( NULL,
                                                           AW_WIDGET_TOGGLE_FIELD, root->awar(p_w->toggle_field_var_name),
                                                           0, 0, var_value, _callback ),
                            new AW_toggle_struct( var_value, 0 ),
                            AW_FALSE );

}


// for float
void AW_window::insert_default_toggle( AW_label toggle_label, const char *mnemonic, float var_value ) {
    if ( p_w->toggle_field_var_type != AW_FLOAT ) {
        AW_ERROR("This toggle field only defined for float");
        return;
    }
    _aw_create_toggle_entry(this,p_w->toggle_field,toggle_label,mnemonic,
                            new AW_variable_update_struct( NULL,
                                                           AW_WIDGET_TOGGLE_FIELD, root->awar(p_w->toggle_field_var_name),
                                                           0, 0, var_value, _callback ),
                            new AW_toggle_struct( var_value, 0 ),
                            AW_TRUE );

}


void AW_window::update_toggle_field( void ) {
    this->update_toggle_field( get_root()->number_of_toggle_fields );
}


void AW_window::update_toggle_field( int toggle_field_number ) {
    AW_toggle_field_struct		*toggle_field_list = p_global->toggle_field_list;
    AW_toggle_struct		*toggle_list;
    Widget				help_toggle = 0;
    AW_BOOL 			cont					= AW_TRUE;
    AW_BOOL 			found					= AW_FALSE;
    char				*global_var_value = NULL;
    long				global_var_int_value = 0;
    float				global_var_float_value = 0;
    int	width_of_last_widget;
    int	height_of_last_widget;

    if ( 1 ) {

        while ( toggle_field_list  && cont ) {
            if ( toggle_field_number == toggle_field_list->toggle_field_number ) {
                cont = AW_FALSE;
                found = AW_TRUE;
            }
            else {
                toggle_field_list = toggle_field_list->next;
            }
        }


        if ( found ) {
            cont = AW_TRUE;
            found = AW_FALSE;

            switch ( toggle_field_list->variable_type ) {
                case AW_STRING:	global_var_value = root->awar( toggle_field_list->variable_name )->read_string();
                    break;
                case AW_INT:	global_var_int_value = root->awar( toggle_field_list->variable_name )->read_int();
                    break;
                case AW_FLOAT:	global_var_float_value = root->awar( toggle_field_list->variable_name )->read_float();
                    break;
                default:
                    GB_warning("Unknown AWAR type");
            }

            toggle_list = toggle_field_list->first_toggle;
            while ( toggle_list && cont ) {

                switch ( toggle_field_list->variable_type ) {
                    case AW_STRING:	if ( (strcmp( global_var_value, toggle_list->variable_value ) == 0) ) {
                        cont = AW_FALSE;
                        found = AW_TRUE;
                    }
                    break;
                    case AW_INT:		if ( global_var_int_value == toggle_list->variable_int_value ) {
                        cont = AW_FALSE;
                        found = AW_TRUE;
                    }
                    break;
                    case AW_FLOAT:		if ( global_var_float_value == toggle_list->variable_float_value ) {
                        cont = AW_FALSE;
                        found = AW_TRUE;
                    }
                    break;
                    default:
                        GB_warning("Unknown AWAR type");
                }
                if ( cont ) {
                    toggle_list = toggle_list->next;
                }
            }
        }else {
            AW_ERROR("update_toggle_field: toggle field %i does not exist",
                     toggle_field_number );
            return;
        }
        if (found) {
            help_toggle = toggle_list->toggle_widget;
        }


        toggle_list = toggle_field_list->first_toggle;
        while ( toggle_list ) {
            XmToggleButtonSetState( toggle_list->toggle_widget, False, False );
            toggle_list = toggle_list->next;
        }
        if ( toggle_field_list->default_toggle ) {
            XmToggleButtonSetState( toggle_field_list->default_toggle->toggle_widget, False, False );
        }


        if ( found ) {
            XmToggleButtonSetState( help_toggle, True, False );
        }else {
            if ( toggle_field_list->default_toggle ) {
                // printf("update auf default \n");
                XmToggleButtonSetState( toggle_field_list->default_toggle->toggle_widget, True, False );
            }else {
                GB_export_error("toggle field %i does not have value (s:%s) or (i:%i) or (f:%f)\n",
                                toggle_field_number,
                                global_var_value, global_var_int_value, global_var_float_value );
                GB_print_error();
            }
        }

        delete global_var_value;

    }

    short length;
    short height;
    XtVaGetValues( p_w->toggle_field ,XmNwidth ,&length ,XmNheight, &height, NULL );
    length                += (short)_at->saved_x_correction_for_label;
    width_of_last_widget   = length;
    height_of_last_widget  = height;

    if ( toggle_field_list->correct_for_at_center_intern ) {
        if ( toggle_field_list->correct_for_at_center_intern == 1 ) { // middle centered
            XtVaSetValues( p_w->toggle_field, XmNx,(short)((short)_at->saved_x - (short)(length/2) + (short)_at->saved_x_correction_for_label ), NULL );
            if ( p_w->toggle_label ) {
                XtVaSetValues( p_w->toggle_label, XmNx,(short)((short)_at->saved_x - (short)(length/2) ), NULL );
            }
            width_of_last_widget = width_of_last_widget / 2;
        }
        if ( toggle_field_list->correct_for_at_center_intern == 2 ) { // right centered
            XtVaSetValues( p_w->toggle_field, XmNx,(short)((short)_at->saved_x - length + (short)_at->saved_x_correction_for_label ), NULL );
            if ( p_w->toggle_label ) {
                XtVaSetValues( p_w->toggle_label, XmNx,(short)( (short)_at->saved_x - length ) , NULL );
            }
            width_of_last_widget = 0;
        }
//         _at->correct_for_at_center_intern = 0;
    }

    this->unset_at_commands();
    this->increment_at_commands( width_of_last_widget, height_of_last_widget );


}
