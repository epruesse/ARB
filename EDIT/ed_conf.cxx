#include <stdio.h>
#include <string.h>
#include <arbdb.h>
#include <arbdb++.hxx>
#include <adtools.hxx>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <edit.hxx>

/***************************************************************************************************************************/
/***************************************************************************************************************************/
/***************************************************************************************************************************/

void aed_left_name( class AED_window *aedw, AED_area_entry *area_entry, char *text ) {
    AWUSE(aedw);

    if ( area_entry ) {
        if (area_entry->ad_extended) {
            sprintf( text, "#%10s# ",area_entry->ad_species->name());
        }else{
            sprintf( text, ">%10s< ",area_entry->ad_species->name());
        }
    }else {
        sprintf( text, ">- noname -< ");
    }
}

void aed_left_security( class AED_window *aedw, AED_area_entry *area_entry, char *text ) {
    AWUSE(aedw);

    if ( area_entry && area_entry->adt_sequence) {
        GBDATA *gbd = area_entry->adt_sequence->get_GBDATA();
        if (gbd) {
            GBDATA *gb_root = GB_get_root(gbd);
            GB_push_local_transaction(gb_root); // very dangerous
            gbd = area_entry->adt_sequence->get_GBDATA();
            if (gbd){
                if (GB_read_flag(area_entry->ad_species->get_GBDATA())) {
                    sprintf( text, "* %i ", GB_read_security_write(gbd));
                }else{
                    sprintf( text, "  %i ", GB_read_security_write(gbd));
                }
            }else{
                sprintf( text, "0 ");
            }
            GB_pop_local_transaction(gb_root);
            return;
        }
    }
    sprintf( text, "0 ");
}


void aed_left_zwei( class AED_window *aedw, AED_area_entry *area_entry, char *text ) {
    aedw=aedw;area_entry=area_entry;

    strcpy( text, "GC-Verhaeltnis " );
}


void aed_left_drei( class AED_window *aedw, AED_area_entry *area_entry, char *text ) {
    aedw=aedw;area_entry=area_entry;

    strcpy( text, "Art " );
}


void aed_left_vier( class AED_window *aedw, AED_area_entry *area_entry, char *text ) {
    aedw=aedw;area_entry=area_entry;

    strcpy( text, "unwichtig " );
}



void show_config_window_draw_area(AW_device *device, AED_window *aedw, AW_pos slider_pos_horizontal, AW_pos slider_pos_vertical, AW_pos picture_l, int top_indent_of_vertical_scrollbar ) {
    char           text[100];
    AW_pos         y;
    AW_pos         right_offset = 200;
    AED_left_side *current_entry_of_dlist;
    AW_rectangle   screen;
    AW_pos         width, height;
    int            counter      = 1;
    int            gc1          = 0;
    int            gc2          = 1;

    const AW_font_information *font_information = device->get_font_information(gc1, 'A');
    device->get_area_size( &screen );

    device->shift_x( 0 );
    device->shift_y( 0 );
    device->line( gc2, 0, 19, screen.r,  19, AED_F_INFO_SEPARATOR, 0, 0 );                             // Linie horizontal
    device->shift_x( 0 - slider_pos_horizontal );
    device->shift_y( 0 );
    device->text( gc2, "Sichtbar", 75, 15, 0.0, AED_F_TEXT_2, 0, 0 );
    device->text( gc2, "Unsichtbar", 75+right_offset, 15, 0.0, AED_F_TEXT_2, 0, 0 );
    device->line( gc2, right_offset,  0, right_offset,  screen.b, AED_F_INFO_SEPARATOR, 0, 0 );        // Linie vertikal

    y = top_indent_of_vertical_scrollbar;
    device->invisible( 1, 0, y, AED_F_TEXT_1, (AW_CL)0, (AW_CL)0 );
    device->invisible( 1, 290, y, AED_F_TEXT_1, (AW_CL)0, (AW_CL)0 );

    current_entry_of_dlist = aedw->show_dlist_left_side->first;
    while ( current_entry_of_dlist ) {
        sprintf( text, "%i: %s ", counter++, current_entry_of_dlist->text );
        strcpy( current_entry_of_dlist->text_for_dragging, text );

        device->push_clip_scale();

        device->get_area_size( &screen );

        device->shift_x( 0 - picture_l - slider_pos_horizontal );
        device->shift_y( 0 - slider_pos_vertical );
        device->set_top_clip_border( top_indent_of_vertical_scrollbar );

        y += AED_LINE_SPACING + font_information->max_letter.ascent;

        current_entry_of_dlist->absolut_x = 4 - picture_l - slider_pos_horizontal;
        current_entry_of_dlist->absolut_y = y - slider_pos_vertical;

        if( !current_entry_of_dlist->is_selected ) {
            device->text( gc1, text, 4, y , 0.0, AED_F_TEXT_1, (AW_CL)current_entry_of_dlist, (AW_CL)0 );
        }
        else {
            AW_pos help_y = y + font_information->max_letter.descent + 1;

            width  = strlen( text ) * font_information->max_letter.width + 4;
            height = font_information->max_letter.height + 2;

            device->text( gc2, text, 4, y, 0.0, AED_F_TEXT_1, (AW_CL)current_entry_of_dlist, AED_F_NAME );

            device->line( gc2, 2,       help_y,        2+width,  help_y,         AED_F_FRAME, (AW_CL)"box", 0 );     // unten
            device->line( gc2, 2,       help_y,        2,        help_y-height,  AED_F_FRAME, (AW_CL)"box", 0 );     // links
            device->line( gc2, 2+width, help_y,        2+width,  help_y-height,  AED_F_FRAME, (AW_CL)"box", 0 );     // rechts
            device->line( gc2, 2,       help_y-height, 2+width,  help_y-height,  AED_F_FRAME, (AW_CL)"box", 0 );     // oben
        }

        device->pop_clip_scale();

        y += 2.0 + font_information->max_letter.descent;

        current_entry_of_dlist = current_entry_of_dlist->next;
    }


    y = top_indent_of_vertical_scrollbar;
    current_entry_of_dlist = aedw->hide_dlist_left_side->first;
    while ( current_entry_of_dlist ) {
        sprintf( text, "%s ", current_entry_of_dlist->text );
        strcpy( current_entry_of_dlist->text_for_dragging, text );

        device->push_clip_scale();

        device->get_area_size( &screen );

        device->shift_x( 0 - picture_l - slider_pos_horizontal );
        device->shift_y( 0 - slider_pos_vertical );
        device->set_top_clip_border( top_indent_of_vertical_scrollbar );

        y += AED_LINE_SPACING + font_information->max_letter.ascent;

        current_entry_of_dlist->absolut_x = 4+right_offset - picture_l - slider_pos_horizontal;
        current_entry_of_dlist->absolut_y = y - slider_pos_vertical;

        if( !current_entry_of_dlist->is_selected ) {
            device->text( gc1, text, 4+right_offset, y , 0.0, AED_F_TEXT_1, (AW_CL)current_entry_of_dlist, (AW_CL)0 );
        }
        else {
            AW_pos help_y = y + font_information->max_letter.descent + 1;

            width  = strlen( text ) * font_information->max_letter.width + 4;
            height = font_information->max_letter.height + 2;

            device->text( gc2, text, 4+right_offset, y, 0.0, AED_F_TEXT_1, (AW_CL)current_entry_of_dlist, AED_F_NAME );

            device->line( gc2, 2+right_offset,       help_y,        2+right_offset+width,  help_y,         AED_F_FRAME, (AW_CL)"box", 0 );     // unten
            device->line( gc2, 2+right_offset,       help_y,        2+right_offset,        help_y-height,  AED_F_FRAME, (AW_CL)"box", 0 );     // links
            device->line( gc2, 2+right_offset+width, help_y,        2+right_offset+width,  help_y-height,  AED_F_FRAME, (AW_CL)"box", 0 );     // rechts
            device->line( gc2, 2+right_offset,       help_y-height, 2+right_offset+width,  help_y-height,  AED_F_FRAME, (AW_CL)"box", 0 );     // oben
        }

        device->pop_clip_scale();

        y += 2.0 + font_information->max_letter.descent;

        current_entry_of_dlist = current_entry_of_dlist->next;
    }

}


void aed_config_window_expose(AW_window *aw, AW_CL cd1, AW_CL cd2) {
    AWUSE(aw);AWUSE(cd2);
    AED_window *aedw = (AED_window *)cd1;
    AW_device *device;

    device = aedw->config_window->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);

    device->clear(AED_F_ALL);
    show_config_window_draw_area( device, aedw, aedw->config_window->slider_pos_horizontal, aedw->config_window->slider_pos_vertical,
                                  aedw->config_window->picture->l, aedw->config_window->top_indent_of_vertical_scrollbar );

}


void aed_config_window_resize(AW_window *aw, AW_CL cd1, AW_CL cd2) {
    AWUSE(aw);AWUSE(cd2);
    AED_window *aedw = (AED_window *)cd1;
    AW_device   *device;
    AW_device   *size_device;
    AW_world    size_information;

    device = aedw->config_window->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);
    device->reset();
    size_device = aedw->config_window->get_size_device (AW_MIDDLE_AREA  );
    size_device->set_filter(AED_F_ALL);
    size_device->reset();

    size_device = aedw->config_window->get_size_device (AW_MIDDLE_AREA  );
    size_device->set_filter(AED_F_TEXT_1);
    show_config_window_draw_area( size_device, aedw, 0, 0, 0, 0 );
    size_device->get_size_information( &size_information );
    aedw->config_window->tell_scrolled_picture_size( size_information );
    aedw->config_window->calculate_scrollbars();
}


void aed_config_window_input(AW_window *aw, AW_CL cd1, AW_CL cd2) {
    AWUSE(cd2);
    AED_window *aedw = (AED_window *)cd1;
    AW_event                        event;
    AW_device                   *device;
    AW_device                   *click_device;
    AW_device                   *size_device;
    AW_clicked_text         clicked_text;
    AW_world            size_information;
    AED_left_side               *left_side_hit;
    AED_left_side               *left_side_destination;
    AW_BOOL                     update_other_windows = AW_FALSE;

    aw->get_event( &event );

    device          = aw->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);
    click_device    = aw->get_click_device (AW_MIDDLE_AREA, event.x, event.y, 0, 10, 0 );
    click_device->set_filter(AED_F_TEXT_1);

    if ( event.type == AW_Mouse_Press ) {
        show_config_window_draw_area( click_device, aedw, aedw->config_window->slider_pos_horizontal, aedw->config_window->slider_pos_vertical,
                                      aedw->config_window->picture->l, aedw->config_window->top_indent_of_vertical_scrollbar );
        click_device->get_clicked_text( &clicked_text );

        if ( clicked_text.exists == AW_TRUE ) {
            left_side_hit = (AED_left_side *)clicked_text.client_data1;
            if ( aedw->one_entry_dlist_left_side_is_selected )
                aedw->selected_entry_of_dlist_left_side->is_selected = AW_FALSE;
            else
                aedw->one_entry_dlist_left_side_is_selected = AW_TRUE;
            left_side_hit->is_selected = AW_TRUE;
            aedw->selected_entry_of_dlist_left_side = left_side_hit;

            device->clear(AED_F_ALL);
            show_config_window_draw_area( device, aedw, aedw->config_window->slider_pos_horizontal, aedw->config_window->slider_pos_vertical,
                                          aedw->config_window->picture->l, aedw->config_window->top_indent_of_vertical_scrollbar );
        }
    }

    if ( event.type == AW_Mouse_Release ) {
        if( aedw->drag ) {
            aedw->drag = AW_FALSE;
            drag_box( device, 2, aedw->drag_x - aedw->drag_x_correcting, aedw->drag_y - aedw->drag_y_correcting, 0, 0, aedw->selected_entry_of_dlist_left_side->text_for_dragging );
            show_config_window_draw_area( click_device, aedw, aedw->config_window->slider_pos_horizontal, aedw->config_window->slider_pos_vertical,
                                          aedw->config_window->picture->l, aedw->config_window->top_indent_of_vertical_scrollbar );

            click_device->get_clicked_text( &clicked_text );
            if ( clicked_text.exists ) {
                left_side_destination = (AED_left_side *)clicked_text.client_data1;
                if ( left_side_destination != aedw->selected_entry_of_dlist_left_side ) {    // man beendet den drag nicht auf sich selbst

                    aedw->selected_entry_of_dlist_left_side->in_side->remove_entry( aedw->selected_entry_of_dlist_left_side );

                    if( clicked_text.distance > 0 )
                        left_side_destination->in_side->insert_after_entry( left_side_destination, aedw->selected_entry_of_dlist_left_side );
                    else
                        left_side_destination->in_side->insert_before_entry( left_side_destination, aedw->selected_entry_of_dlist_left_side );
                    aedw->selected_entry_of_dlist_left_side->in_side = left_side_destination->in_side;
                    update_other_windows = AW_TRUE;
                }
            }
            else {
                if ( event.x > ( 200 - aedw->config_window->slider_pos_horizontal ) ) {
                    if ( aedw->selected_entry_of_dlist_left_side->in_side == aedw->show_dlist_left_side ) {
                        aedw->show_dlist_left_side->remove_entry( aedw->selected_entry_of_dlist_left_side );
                        aedw->hide_dlist_left_side->append( aedw->selected_entry_of_dlist_left_side );
                        aedw->selected_entry_of_dlist_left_side->in_side = aedw->hide_dlist_left_side;
                        update_other_windows = AW_TRUE;
                    }
                    else {
                        aedw->hide_dlist_left_side->remove_entry( aedw->selected_entry_of_dlist_left_side );
                        aedw->hide_dlist_left_side->append( aedw->selected_entry_of_dlist_left_side );
                        update_other_windows = AW_TRUE;
                    }
                }
                else {
                    if ( aedw->selected_entry_of_dlist_left_side->in_side == aedw->hide_dlist_left_side ) {
                        aedw->hide_dlist_left_side->remove_entry( aedw->selected_entry_of_dlist_left_side );
                        aedw->show_dlist_left_side->append( aedw->selected_entry_of_dlist_left_side );
                        aedw->selected_entry_of_dlist_left_side->in_side = aedw->show_dlist_left_side;
                        update_other_windows = AW_TRUE;
                    }
                    else {
                        aedw->show_dlist_left_side->remove_entry( aedw->selected_entry_of_dlist_left_side );
                        aedw->show_dlist_left_side->append( aedw->selected_entry_of_dlist_left_side );
                        update_other_windows = AW_TRUE;
                    }
                }
            }
            if ( update_other_windows ) {

                size_device = aedw->config_window->get_size_device (AW_MIDDLE_AREA  );
                size_device->set_filter(AED_F_TEXT_1);
                show_config_window_draw_area( size_device, aedw, 0, 0, 0, 0 );
                size_device->get_size_information( &size_information );
                aedw->config_window->tell_scrolled_picture_size( size_information );
                aedw->config_window->calculate_scrollbars();

                device->clear(AED_F_ALL);
                show_config_window_draw_area( device, aedw, aedw->config_window->slider_pos_horizontal, aedw->config_window->slider_pos_vertical,
                                              aedw->config_window->picture->l, aedw->config_window->top_indent_of_vertical_scrollbar );
                aed_resize( (AW_window *)aedw->aww, (AW_CL)aedw, (AW_CL)0 );
            }
        }
    }


}


void aed_config_window_motion(AW_window *aw, AW_CL cd1, AW_CL cd2) {
    AWUSE(cd2);
    AED_window                  *aedw = (AED_window *)cd1;
    AW_event                    event;
    AW_event                    motion_event;
    AW_device                   *device;
    AW_device                   *click_device;
    //    AW_window     *awmm = aedw->aww;
    AW_clicked_text             clicked_text;
    AW_rectangle                screen;

    device = aw->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);
    device->get_area_size( &screen );

    if ( aedw->drag ) {                                                                                                                         // mitten im dragging
        aw->get_event ( &motion_event );
        device->shift_y( 0 );
        device->shift_x( 0 );
        drag_box( device, 2, aedw->drag_x - aedw->drag_x_correcting, aedw->drag_y - aedw->drag_y_correcting, 0, 0, aedw->selected_entry_of_dlist_left_side->text_for_dragging );

        if( motion_event.y < 20 )
            motion_event.y = 20;
        if ( motion_event.y > screen.b )
            motion_event.y = screen.b;
        if( motion_event.x < 0 )
            motion_event.x = 0;
        if( motion_event.x > screen.r )
            motion_event.x = screen.r;
        aedw->drag_x = motion_event.x;
        aedw->drag_y = motion_event.y;

        drag_box( device, 2, motion_event.x - aedw->drag_x_correcting, motion_event.y - aedw->drag_y_correcting, 0, 0, aedw->selected_entry_of_dlist_left_side->text_for_dragging );
    }

    if( !aedw->drag ) {                                                                                                                     // Beginn des draggings
        aw->get_event( &event );

        click_device = aw->get_click_device (AW_MIDDLE_AREA, event.x, event.y, 0, 0, 0 );
        click_device->set_filter(AED_F_TEXT_1);
        show_config_window_draw_area( click_device, aedw, aedw->config_window->slider_pos_horizontal, aedw->config_window->slider_pos_vertical,
                                      aedw->config_window->picture->l, aedw->config_window->top_indent_of_vertical_scrollbar );
        click_device->get_clicked_text( &clicked_text );

        if( clicked_text.exists && (AED_left_side *)clicked_text.client_data1 == aedw->selected_entry_of_dlist_left_side ) {
            aedw->drag = AW_TRUE;
            aw->get_event ( &motion_event );
            aedw->drag_x_correcting = event.x - aedw->selected_entry_of_dlist_left_side->absolut_x;
            aedw->drag_y_correcting = event.y - aedw->selected_entry_of_dlist_left_side->absolut_y;
            device->shift_y( 0 );
            device->shift_x( 0 );
            drag_box( device, 2, motion_event.x - aedw->drag_x_correcting, motion_event.y - aedw->drag_y_correcting, 0, 0, aedw->selected_entry_of_dlist_left_side->text_for_dragging );
            aedw->drag_x = motion_event.x;
            aedw->drag_y = motion_event.y;
        }
    }
}


void aed_config_window_horizontal_scroll(AW_window *aw, AW_CL cd1, AW_CL cd2) {
    AWUSE(aw);AWUSE(cd2);
    AED_window *aedw = (AED_window *)cd1;
    AW_device *device;

    device = aedw->config_window->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);

    device->clear(AED_F_ALL);
    show_config_window_draw_area( device, aedw, aedw->config_window->slider_pos_horizontal, aedw->config_window->slider_pos_vertical,
                                  aedw->config_window->picture->l, aedw->config_window->top_indent_of_vertical_scrollbar );
}


void aed_config_window_vertical_scroll(AW_window *aw, AW_CL cd1, AW_CL cd2) {
    AWUSE(aw);AWUSE(cd2);
    AED_window *aedw = (AED_window *)cd1;
    AW_device *device;

    device = aedw->config_window->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);

    device->clear(AED_F_ALL);
    show_config_window_draw_area( device, aedw, aedw->config_window->slider_pos_horizontal, aedw->config_window->slider_pos_vertical,
                                  aedw->config_window->picture->l, aedw->config_window->top_indent_of_vertical_scrollbar );
}


void aed_config_window_expose_info_area(AW_window *aw, AW_CL cd1, AW_CL cd2) {
    AWUSE(aw);AWUSE(cd2);
    AED_window *aedw = (AED_window *)cd1;
    AW_device *info_device;

    info_device = aedw->config_window->get_device (AW_INFO_AREA  );
    info_device->set_filter(AED_F_ALL);
    info_device->clear(AED_F_ALL);
    info_device->shift_y( 0 );
    info_device->shift_x( 0 );
    info_device->text( 1, "Anleitung 1", 4, 20, 0.0, AED_F_INFO, (AW_CL)0, (AW_CL)0 );
    info_device->text( 1, "Anleitung 2", 4, 36, 0.0, AED_F_INFO, (AW_CL)0, (AW_CL)0 );
}



void aed_config_window_popdown(AW_window *aw, AW_CL cd1, AW_CL cd2) {
    AWUSE(aw);AWUSE(cd2);
    AED_window *aedw = (AED_window *)cd1;

    aedw->config_window->hide();
}


void aed_config_1(AW_window *aw, AW_CL /*cd1*/, AW_CL /*cd2*/) {
    //    AED_window *aedw = (AED_window *)cd1;
    aw->set_info_area_height(100);
}


void aed_popup_config_window(AW_window *aw, AW_CL cd1, AW_CL cd2) {
    AWUSE(aw);AWUSE(cd2);
    AED_window              *aedw = (AED_window *)cd1;
    AW_device               *device;
    AW_device               *click_device;
    AW_device               *info_device;
    AW_device               *size_device;
    AW_world                size_information;

    if ( aedw->config_window_created ) {
        aedw->config_window->show();
    }
    else {
        aedw->config_window = new AW_window_menu();
        aedw->config_window_created = AW_TRUE;

        aedw->config_window->init(aedw->root->aw_root,"EDIT_CONFIG", "ARB_EDIT_CONFIG",200,200);
        aedw->config_window->create_menu( "1", "Window", "W", "no help", AWM_ALL );
        aedw->config_window->insert_menu_topic( "1.1", "hide", "h", "no help",  AWM_ALL,        aed_config_window_popdown, (AW_CL)aedw, (AW_CL)0 );
        aedw->config_window->insert_menu_topic( "1.1", "bigger info", "h", "no help",   AWM_ALL,        aed_config_1, (AW_CL)aedw, (AW_CL)0 );


        aedw->config_window->set_expose_callback (AW_MIDDLE_AREA,               aed_config_window_expose, (AW_CL)aedw, 0 );
        aedw->config_window->set_resize_callback (AW_MIDDLE_AREA,                   aed_config_window_resize, (AW_CL)aedw, 0 );
        aedw->config_window->set_input_callback (AW_MIDDLE_AREA,                    aed_config_window_input, (AW_CL)aedw, 0 );
        aedw->config_window->set_motion_callback (AW_MIDDLE_AREA,                   aed_config_window_motion, (AW_CL)aedw, 0 );
        aedw->config_window->set_vertical_change_callback(      aed_config_window_vertical_scroll, (AW_CL)aedw, 0 );
        aedw->config_window->set_horizontal_change_callback(    aed_config_window_horizontal_scroll, (AW_CL)aedw, 0 );
        aedw->config_window->set_expose_callback (AW_INFO_AREA,     aed_config_window_expose_info_area, (AW_CL)aedw, 0 );


        device = aedw->config_window->get_device (AW_MIDDLE_AREA  );
        device->set_filter(AED_F_ALL);
        aed_initialize_device( device );

        click_device = aedw->config_window->get_click_device (AW_MIDDLE_AREA,0,0,0,0,0);
        aed_initialize_device(click_device);

        info_device = aedw->config_window->get_device (AW_INFO_AREA  );
        aed_initialize_device( info_device );

        size_device = aedw->config_window->get_size_device (AW_MIDDLE_AREA  );
        aed_initialize_device( size_device );


        aedw->config_window->set_vertical_scrollbar_top_indent( 20 );

        device->set_foreground_color( 2, AW_WINDOW_FG );

        aedw->config_window->at           ( 15, 15 );
        aedw->config_window->callback     ( aed_config_window_popdown, (AW_CL)aedw, (AW_CL)0 );
        aedw->config_window->create_button( "CLOSE", "DONE", "D" );

        aedw->config_window->set_info_area_height( 100 );
        aedw->config_window->set_info_area_height( 0 );
        aedw->config_window->set_bottom_area_height( 0 );

        size_device = aedw->config_window->get_size_device (AW_MIDDLE_AREA  );
        size_device->set_filter(AED_F_TEXT_1);
        show_config_window_draw_area( size_device, aedw, 0, 0, 0, 0 );
        size_device->get_size_information( &size_information );
        aedw->config_window->tell_scrolled_picture_size( size_information );
        aedw->config_window->calculate_scrollbars();



        aedw->config_window->show();

    }

}


AED_left_side::AED_left_side( void(*f)(class AED_window *, AED_area_entry *area_entry, char *text), const char *string ) {
    is_selected = AW_FALSE;
    previous = NULL;
    next = NULL;
    text = new char[(strlen(string)+1)];
    strcpy( text, string );
    make_text = f;
}


AED_left_side::~AED_left_side() {
    delete text;
}


void AED_dlist_left_side::remove_entry(AED_left_side *to_be_removed) {
    if ( first == to_be_removed ) {
        if(to_be_removed->next) {
            first = to_be_removed->next;
            to_be_removed->next->previous = NULL;
        }
        else
            first = NULL;
    }
    else {
        to_be_removed->previous->next = to_be_removed->next;
        if(to_be_removed->next)
            to_be_removed->next->previous = to_be_removed->previous;
    }
    if( last == to_be_removed ) {
        if( to_be_removed->previous )
            last = to_be_removed->previous;
        else
            last = NULL;
    }
    size--;
}


void AED_dlist_left_side::insert_after_entry(AED_left_side *add_after_this, AED_left_side *to_be_inserted) {
    if( last == add_after_this )
        last = to_be_inserted;
    to_be_inserted->next                            = add_after_this->next;
    to_be_inserted->previous                    = add_after_this;
    if ( add_after_this->next )
        add_after_this->next->previous      = to_be_inserted;
    add_after_this->next                            = to_be_inserted;

    size++;
}


void AED_dlist_left_side::insert_before_entry(AED_left_side *add_before_this, AED_left_side *to_be_inserted) {

    if( (add_before_this)->previous )
        (add_before_this)->previous->next   = to_be_inserted;
    else
        first = to_be_inserted;
    to_be_inserted->next                            = add_before_this;
    to_be_inserted->previous                    = add_before_this->previous;
    add_before_this->previous                   = to_be_inserted;

    size++;
}


void AED_dlist_left_side::append(AED_left_side *to_be_inserted) {

    if(last) {
        last->next = to_be_inserted;
        to_be_inserted->previous = last;
        to_be_inserted->next = NULL;
    }
    else {
        first = last = to_be_inserted;
        to_be_inserted->previous = NULL;
        to_be_inserted->next = NULL;
    }
    last = to_be_inserted;
    size++;
}
