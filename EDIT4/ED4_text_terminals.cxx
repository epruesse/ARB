#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <iostream>
// #include <malloc.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_keysym.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <aw_awars.hxx>

#include <AW_helix.hxx>

#include <awt_seq_colors.hxx>
#include <awt_attributes.hxx>
#include <st_window.hxx>

#include <ed4_extern.hxx>

#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_block.hxx"
#include "ed4_nds.hxx"
#include "ed4_visualizeSAI.hxx"
#include "ed4_ProteinViewer.hxx"
#include "ed4_protein_2nd_structure.hxx"

int ED4_consensus_sequence_terminal::get_length() const {
    return get_char_table().size();
}

ED4_returncode ED4_consensus_sequence_terminal::draw( int /*only_text*/ )
{
    AW_pos x, y;
    AW_pos text_x, text_y;
    long   left_index    = 0, right_index = 0;

    static char   *buffer      = 0;
    static size_t  buffer_size = 0;

    calc_world_coords( &x, &y );
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x, &y );
    calc_update_intervall(&left_index, &right_index );

    text_x = x + CHARACTEROFFSET; // don't change
    text_y = y + SEQ_TERM_TEXT_YOFFSET;

#if defined(DEBUG) && 0
    int paint_back = 1;
    if (paint_back) {
        paint_back = 3-paint_back;
        ED4_ROOT->temp_device->box(ED4_G_FIRST_COLOR_GROUP+paint_back-1, x, y, extension.size[WIDTH], extension.size[HEIGHT], -1, 0, 0);
    }
#endif // DEBUG
    
    const ED4_remap *rm = ED4_ROOT->root_group_man->remap();
    rm->clip_screen_range(&left_index, &right_index);

    if (right_index<0 || left_index<0) {
        e4_assert(left_index == -1 && right_index <= 0); // no update intervall

        const char *no_data = "No consensus data";
        size_t      len     = strlen(no_data);
        if (buffer_size <= len) {
            delete buffer;
            buffer_size = len+10;
            buffer      = new char[buffer_size];
            memset(buffer, ' ' , buffer_size); // previously buffer was filled with zero's
        }
        memcpy(buffer, no_data, len);
        right_index = buffer_size-1;
    }
    else {
        int seq_start = rm->screen_to_sequence(left_index);
        int seq_end   = rm->screen_to_sequence(right_index);

        while (seq_end >= MAXSEQUENCECHARACTERLENGTH && right_index>0) { // behind end of sequence
            right_index--;
            seq_end = rm->screen_to_sequence(right_index);
        }

        char *cons = GB_give_buffer(seq_end+1);
        cons = get_char_table().build_consensus_string(seq_start, seq_end, cons);

        if ( size_t(right_index) >= buffer_size){
            delete [] buffer;
            buffer_size = right_index + 10;
            e4_assert(buffer_size>0);
            buffer      = new char[buffer_size+1];

            memset(buffer, ' ' , buffer_size); // previously buffer was filled with zero's
            buffer[buffer_size] = 0;
        }


        {
            int pos;
            for (pos = left_index; pos <= right_index; pos++){
                int seq_pos = rm->screen_to_sequence(pos);
                if (seq_pos<0){
                    buffer[pos] = ' ';
                }else{
                    buffer[pos] = cons[seq_pos];
                    e4_assert(buffer[pos]);
                }
            }
        }
    }


    if (buffer_size) {
        ED4_ROOT->temp_device->top_font_overlap    = 1;
        ED4_ROOT->temp_device->bottom_font_overlap = 1;
    
        ED4_ROOT->temp_device->text( ED4_G_SEQUENCES, buffer, text_x, text_y, 0, 1, 0, 0, (long) right_index+1);
        
        ED4_ROOT->temp_device->top_font_overlap    = 0;
        ED4_ROOT->temp_device->bottom_font_overlap = 0;
    }

    return ( ED4_R_OK );
}

int ED4_show_helix_on_device(AW_device *device, int gc, const char *opt_string, size_t opt_string_size, size_t start, size_t size,
                             AW_pos x,AW_pos y, AW_pos opt_ascent,AW_pos opt_descent,
                             AW_CL cduser, AW_CL real_sequence_length, AW_CL cd2){
    AWUSE(opt_ascent);AWUSE(opt_descent);AWUSE(opt_string_size);
    AW_helix *helix = (AW_helix *)cduser;
    const ED4_remap *rm = ED4_ROOT->root_group_man->remap();
    char *buffer = GB_give_buffer(size+1);
    register long i,j,k;

    for (k=0; size_t(k)<size; k++) {
        i = rm->screen_to_sequence(k+start);

        BI_PAIR_TYPE pairType = helix->pairtype(i);
        if (pairType == HELIX_NONE) {
            buffer[k] = ' ';
        }
        else {
            j             = helix->opposite_position(i);
            char pairchar = j<real_sequence_length ? opt_string[j] : '.';
            buffer[k]     = helix->get_symbol(opt_string[i], pairchar, pairType);
        }
    }
    buffer[size] = 0;
    return device->text(gc,buffer,x,y,0.0,(AW_bitset)-1,0,cd2);
}

struct show_summary_parameters {
    const char *global_protstruct;
    const char *protstruct;
};


int ED4_show_summary_match_on_device(AW_device *device, int gc, const char */*opt_string*/, size_t /*opt_string_size*/, size_t start, size_t size,
                                     AW_pos x,AW_pos y, AW_pos /*opt_ascent*/,AW_pos /*opt_descent*/,
                                     AW_CL cd_summary_paramters, AW_CL /*real_sequence_length*/, AW_CL cd2)
{
    show_summary_parameters *ssp    = (show_summary_parameters*)cd_summary_paramters;
    const ED4_remap         *rm     = ED4_ROOT->root_group_man->remap();
    char                    *buffer = GB_give_buffer(size+1);

    for (long k=0; size_t(k)<size; k++) {
        long i = rm->screen_to_sequence(k+start);

        if (ssp->protstruct[i] == ssp->global_protstruct[i]) {
            buffer[k] = ' ';
        }
        else {
            buffer[k] = '#';
        }
    }
    buffer[size] = 0;
    return device->text(gc,buffer,x,y,0.0,(AW_bitset)-1,0,cd2);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ProteinViewer: Drawing AminoAcid sequence parallel to the DNA sequence
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define is_upper(c) ('A'<=(c) && (c)<='Z')

ED4_returncode ED4_AA_sequence_terminal::draw( int /*only_text*/ )
{
    AW_pos        text_x, text_y;
    int           max_seq_len            = 0;
    static int    color_is_used[ED4_G_DRAG];
    static char **colored_strings        = 0;
    static int    len_of_colored_strings = 0;
    AW_device    *device                 = ED4_ROOT->temp_device;

    max_seq_len = strlen(this->aaSequence);

    AW_pos world_x, world_y;

    calc_world_coords( &world_x, &world_y );
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &world_x, &world_y );

    text_x    = world_x + CHARACTEROFFSET; // don't change
    text_y    = world_y + SEQ_TERM_TEXT_YOFFSET;

    const ED4_remap *rm = ED4_ROOT->root_group_man->remap();

    long left,right;
    calc_update_intervall(&left, &right );
    rm->clip_screen_range(&left, &right);

    {
        int max_seq_pos = rm->sequence_to_screen_clipped(max_seq_len-1);

        if (right>max_seq_len) right = max_seq_pos;
        if (left>right) {
            const char *no_data = "No sequence data";
            size_t      len     = strlen(no_data);

            device->text(ED4_G_STANDARD, no_data, text_x, text_y, 0, 1, 0, 0, len);
            return ED4_R_OK;
        }
    }

    if (right >= len_of_colored_strings){
        len_of_colored_strings = right + 256;
        if (!colored_strings) {
            colored_strings = (char **)GB_calloc(sizeof(char *),ED4_G_DRAG);
        }
        int i;
        for (i=0;i<ED4_G_DRAG;i++){
            free(colored_strings[i]);
            colored_strings[i] = (char *)malloc(sizeof(char) * (len_of_colored_strings+1));

            memset(colored_strings[i],' ',len_of_colored_strings);
            colored_strings[i][len_of_colored_strings] = 0;
        }
    }

    // int seq_start = rm->screen_to_sequence(left); // real start of sequence
    int seq_end = rm->screen_to_sequence(right);

    // mark all strings as unused
    memset(color_is_used,0,sizeof(color_is_used));

    //    int    iDisplayAminoAcids = ED4_ROOT->aw_root->awar(AWAR_PROTVIEW_DISPLAY_AA)->read_int();
    int iDisplayMode = ED4_ROOT->aw_root->awar(AWAR_PROTVIEW_DISPLAY_OPTIONS)->read_int();
    unsigned char *aaSequence = (unsigned char *) strdup(this->aaSequence);

    {     // transform strings, compress if needed
        AWT_reference *ref        = ED4_ROOT->reference;
        ref->expand_to_length(seq_end);
        char *char_2_char = ED4_ROOT->sequence_colors->char_2_char_aa;
        char *char_2_gc   = ED4_ROOT->sequence_colors->char_2_gc_aa;
        int scr_pos;

        for (scr_pos=left; scr_pos <= right; scr_pos++) {
            int seq_pos = rm->screen_to_sequence(scr_pos);
            int c = aaSequence[seq_pos];
            int gc = char_2_gc[c];

            color_is_used[gc] = scr_pos+1;
            colored_strings[gc][scr_pos] = char_2_char[aaSequence[seq_pos]];
        }
    }

    // painting background
    if((iDisplayMode == PV_AA_CODE) || (iDisplayMode == PV_AA_BOX)) {
        {
            AW_pos       width = ED4_ROOT->font_group.get_width(ED4_G_HELIX);
            int          real_left  = left;
            int        real_right  = right;
            AW_pos           x2  = text_x + width*real_left;
            AW_pos            x1 = x2;
            AW_pos           y1  = world_y;
            AW_pos           y2  = text_y+1;
            AW_pos       height = y2-y1+1;
            int                color  = ED4_G_STANDARD;
            char *char_2_gc_aa = ED4_ROOT->sequence_colors->char_2_gc_aa;

            int i;
            for ( i = real_left; i <= real_right; i++,x2 += width) {
                int new_pos = rm->screen_to_sequence(i);  //getting the real position of the base in the sequence
                int       c = aaSequence[new_pos];
                char    base = aaSequence[new_pos];
                if (is_upper(base) || (base=='*')) {
                    x1   = x2-width;         // store current x pos to x1
                    x2 += width*2; // add 2 char width to x2
                    i    += 2;         //jump two pos 

                    int gcChar =  char_2_gc_aa[c];
                    if ((gcChar>=0) && (gcChar<ED4_G_DRAG)) {
                        color = gcChar;  
                        if (iDisplayMode == PV_AA_BOX) {
                            device->box(color, AW_TRUE, x1, y1, width*3, height, -1, 0,0);
                        }
                        else {
                            device->arc(ED4_G_STANDARD, false, x1+(width*3)/2, y1, (width*3)/2, height/2, 0, -180, -1, 0, 0);
                        }
                    }
                }
            }
        }
    }
    free(aaSequence);

    device->top_font_overlap    = 1;
    device->bottom_font_overlap = 1;

    // output strings -- when display aminoacid sequence selected
    if((iDisplayMode == PV_AA_NAME) || (iDisplayMode == PV_AA_CODE)) {
        //    if (!iDisplayAsColoredBox || iDisplayAminoAcids) { 
        int gc;
        for (gc = 0; gc < ED4_G_DRAG; gc++){
            if (!color_is_used[gc]) continue;
            if ((int)strlen(colored_strings[gc])>=color_is_used[gc]) {
                device->text( gc, colored_strings[gc], text_x, text_y, 0, 1, 0, 0, color_is_used [gc]);
                memset(colored_strings[gc] + left,' ', right-left+1); // clear string
            }
        }
    }
    device->top_font_overlap    = 0;
    device->bottom_font_overlap = 0;

    return ( ED4_R_OK );
}

ED4_returncode ED4_sequence_terminal::draw( int /*only_text*/ )
{
    AW_pos        text_x, text_y;
    int           max_seq_len            = 0;
    static int    color_is_used[ED4_G_DRAG];
    static char **colored_strings        = 0;
    static int    len_of_colored_strings = 0;
    AW_device    *device                 = ED4_ROOT->temp_device;

    resolve_pointer_to_char_pntr(&max_seq_len);
    e4_assert(max_seq_len>0);

#if defined(DEBUG) && 0
    int paint_back = 0;
    if (strcmp(species_name, "ARTGLOB3") == 0) { paint_back = 1; }
    else if (strcmp(species_name, "ARTGLOB4") == 0) { paint_back = 2; }
    else if (strcmp(species_name, "ARTLUTE2") == 0) { paint_back = 3; }
#endif // DEBUG

    AW_pos world_x, world_y;

    calc_world_coords( &world_x, &world_y );
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &world_x, &world_y );

#if defined(DEBUG) && 0
    int paint_back = 1;
    if (paint_back) {
        paint_back = 3-paint_back;
        device->box(ED4_G_FIRST_COLOR_GROUP+paint_back-1, world_x, world_y, extension.size[WIDTH], extension.size[HEIGHT], -1, 0, 0);
    }
#endif // DEBUG

    text_x    = world_x + CHARACTEROFFSET; // don't change
    text_y    = world_y + SEQ_TERM_TEXT_YOFFSET;

    const ED4_remap *rm = ED4_ROOT->root_group_man->remap();

    long left,right;
    calc_update_intervall(&left, &right );
    rm->clip_screen_range(&left, &right);

    {
        int max_seq_pos = rm->sequence_to_screen_clipped(max_seq_len-1);

        if (right>max_seq_len) right = max_seq_pos;
        if (left>right) {
            const char *no_data = "No sequence data";
            size_t      len     = strlen(no_data);

            device->text(ED4_G_STANDARD, no_data, text_x, text_y, 0, 1, 0, 0, len);
            return ED4_R_OK;
        }
    }

    if (right >= len_of_colored_strings){
        len_of_colored_strings = right + 256;
        if (!colored_strings) {
            colored_strings = (char **)GB_calloc(sizeof(char *),ED4_G_DRAG);
        }
        int i;
        for (i=0;i<ED4_G_DRAG;i++){
            free(colored_strings[i]);
            colored_strings[i] = (char *)malloc(sizeof(char) * (len_of_colored_strings+1));

            memset(colored_strings[i],' ',len_of_colored_strings);
            colored_strings[i][len_of_colored_strings] = 0;
        }
    }

    int seq_start = rm->screen_to_sequence(left); // real start of sequence
    int seq_end = rm->screen_to_sequence(right);

    // mark all strings as unused
    memset(color_is_used,0,sizeof(color_is_used));

    // transform strings, compress if needed
    {
        AWT_reference *ref        = ED4_ROOT->reference;
        unsigned char *db_pointer = (unsigned char *)resolve_pointer_to_string_copy();

        ref->expand_to_length(seq_end);

        GB_alignment_type aliType = GetAliType();
        char *char_2_char = (aliType && (aliType==GB_AT_AA))? ED4_ROOT->sequence_colors->char_2_char_aa:ED4_ROOT->sequence_colors->char_2_char;
        char *char_2_gc   = (aliType && (aliType==GB_AT_AA))? ED4_ROOT->sequence_colors->char_2_gc_aa:ED4_ROOT->sequence_colors->char_2_gc;

        int scr_pos;
        int is_ref = ref->reference_species_is(species_name);

        for (scr_pos=left; scr_pos <= right; scr_pos++) {
            int seq_pos = rm->screen_to_sequence(scr_pos);
            int c = db_pointer[seq_pos];
            int gc = char_2_gc[c];

            color_is_used[gc] = scr_pos+1;
            colored_strings[gc][scr_pos] = char_2_char[is_ref ? c : ref->convert(c, seq_pos)];
        }

        free(db_pointer);
    }

    // Set background

    {
        GB_transaction       dummy(gb_main);
        ST_ML_Color         *colors       = 0;
        char                *searchColors = results().buildColorString(this, seq_start, seq_end); // defined in ED4_SearchResults class : ED4_search.cxx
        ED4_species_manager *spec_man     = get_parent(ED4_L_SPECIES)->to_species_manager();
        int                  is_marked    = GB_read_flag(spec_man->get_species_pointer());
        int                  selection_col1, selection_col2;
        int                  is_selected  = ED4_get_selected_range(this, &selection_col1, &selection_col2);
        int                  color_group  = AWT_species_get_dominant_color(spec_man->get_species_pointer());
        // int                  color_group  = AW_find_color_group(spec_man->get_species_pointer());

        if (species_name &&
            ED4_ROOT->column_stat_activated &&
            (st_ml_node || (st_ml_node = st_ml_convert_species_name_to_node(ED4_ROOT->st_ml,this->species_name))))
            {
                colors = st_ml_get_color_string(ED4_ROOT->st_ml, 0, st_ml_node, seq_start, seq_end);
            }

        const char *saiColors = 0;

        if (species_name                                     &&
            ED4_ROOT->visualizeSAI                           &&
            !spec_man->flag.is_SAI                           &&
            (is_marked || ED4_ROOT->visualizeSAI_allSpecies))
        {
            saiColors = ED4_getSaiColorString(ED4_ROOT->aw_root, seq_start, seq_end);
        }

        if (colors || searchColors || is_marked || is_selected || color_group || saiColors) {
            int    i;
            AW_pos width      = ED4_ROOT->font_group.get_width(ED4_G_HELIX);
            int    real_left  = left;
            int    real_right = right;
            AW_pos x2         = text_x + width*real_left;
            AW_pos old_x      = x2;
            AW_pos y1         = world_y;
            AW_pos y2         = text_y+1;
            AW_pos height     = y2-y1+1;
            int    old_color  = ED4_G_STANDARD;
            int    color      = ED4_G_STANDARD;

            if (is_selected && selection_col2<0) {
                selection_col2 = rm->screen_to_sequence(real_right);
            }

            for ( i = real_left; i <= real_right; i++,x2 += width) {
                int new_pos = rm->screen_to_sequence(i);  //getting the real position of the base in the sequence

                if (searchColors && searchColors[new_pos]) {
                    color = searchColors[new_pos];
                }
                else if (is_selected && new_pos>=selection_col1 && new_pos<=selection_col2) {
                    color = ED4_G_SELECTED;
                }
                else if (colors) {
                    color = colors[new_pos] + ED4_G_CBACK_0;
                    if (color > ED4_G_CBACK_9) color = ED4_G_CBACK_9;
                }
                else if (saiColors) {
                    color = saiColors[new_pos];
                    if (color < ED4_G_CBACK_0 || color > ED4_G_CBACK_9)  color = ED4_G_STANDARD;
                }
                else if (is_marked) {
                    color = ED4_G_MARKED;
                }
                else if (color_group) {
                    color = ED4_G_FIRST_COLOR_GROUP+color_group-1;
                }
                else {
                    color = ED4_G_STANDARD;
                }

                if (color != old_color) {   // draw till oldcolor
                    if (x2>old_x){
                        if (old_color!=ED4_G_STANDARD) {
                            device->box(old_color, AW_TRUE, old_x, y1, x2-old_x, height, -1, 0,0); // paints the search pattern background
                        }
                    }
                    old_x = x2;
                    old_color = color;
                }
            }

            if (x2>old_x){
                if (color!=ED4_G_STANDARD) {
                    device->box(color, AW_TRUE, old_x, y1, x2-old_x, height, -1, 0,0);
                }
            }
        }
    }

    device->top_font_overlap    = 1;
    device->bottom_font_overlap = 1;

    // output helix
    if (ED4_ROOT->helix->is_enabled()) { // should do a remap
        int screen_length = rm->clipped_sequence_to_screen(ED4_ROOT->helix->size());
        if ((right+1) < screen_length) {
            screen_length = right+1;
        }
        if (screen_length) {
            char *db_pointer = resolve_pointer_to_string_copy();
            device->text_overlay( ED4_G_HELIX,
                                  (char *)db_pointer, screen_length,
                                  text_x , text_y + ED4_ROOT->helix_spacing , 0.0 , -1,
                                  (AW_CL)ED4_ROOT->helix, (AW_CL)max_seq_len, 0,
                                  1.0,1.0, ED4_show_helix_on_device);
            free(db_pointer);
        }
    }

    // output protein structure match
    if (ED4_ROOT->protstruct)
    {   // should do a remap
        int screen_length = rm->clipped_sequence_to_screen(ED4_ROOT->protstruct_len);
        if ((right+1) < screen_length) {
            screen_length = right+1;
        }
        if (screen_length) {
            char *db_pointer = resolve_pointer_to_string_copy();

            show_summary_parameters ssp;
            char *predicted_summary = (char*)GB_calloc(max_seq_len+1, 1); // @@@ FIXME: cache predicted_summary in terminal
            ED4_predict_summary(db_pointer, predicted_summary, max_seq_len);

            ssp.global_protstruct = ED4_ROOT->protstruct;
            ssp.protstruct        = predicted_summary;

            device->text_overlay( ED4_G_HELIX,
                                  (char *)db_pointer, screen_length,
                                  text_x , text_y + ED4_ROOT->helix_spacing , 0.0 , -1,
                                  (AW_CL)ED4_ROOT->protstruct, (AW_CL)max_seq_len, 0,
                                  1.0,1.0, ED4_show_summary_match_on_device);

            free(db_pointer);
            free(predicted_summary);
        }
    }

    // output strings
    {
        int gc;
        for (gc = 0; gc < ED4_G_DRAG; gc++){
            if (!color_is_used[gc]) continue;
            device->text( gc, colored_strings[gc], text_x, text_y, 0, 1, 0, 0, color_is_used [gc]);
            memset(colored_strings[gc] + left,' ', right-left+1); // clear string
        }
    }

    device->top_font_overlap    = 0;
    device->bottom_font_overlap = 0;

    return ( ED4_R_OK );
}


ED4_returncode ED4_sequence_info_terminal::draw( int /*only_text*/ )
{
    AW_pos x, y;

    calc_world_coords( &x, &y );
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x, &y );

    AW_pos text_x = x + CHARACTEROFFSET; // don't change
    AW_pos text_y = y+INFO_TERM_TEXT_YOFFSET;

    char    buffer[10];
    GBDATA *gbdata = data();

    if (gbdata){
        GB_push_transaction(gbdata);
        buffer[0] = '0' + GB_read_security_write(gbdata);
        GB_pop_transaction(gbdata);
    }
    else {
        buffer[0] = ' ';
    }
    strncpy(&buffer[1],this->id,8);
    buffer[9] = 0;

    ED4_species_name_terminal *name_term = corresponding_species_name_terminal();
    if (name_term->flag.selected) {
        ED4_ROOT->temp_device->box(ED4_G_SELECTED, AW_TRUE, x, y, extension.size[WIDTH], text_y-y+1, (AW_bitset)-1, 0, 0);
    }

    ED4_ROOT->temp_device->top_font_overlap    = 1;
    ED4_ROOT->temp_device->bottom_font_overlap = 1;
    
    ED4_ROOT->temp_device->text( ED4_G_STANDARD, buffer, text_x, text_y, 0, 1, 0, 0, 0);

    ED4_ROOT->temp_device->top_font_overlap    = 0;
    ED4_ROOT->temp_device->bottom_font_overlap = 0;

    return ( ED4_R_OK );

}

// --------------------------------------------------------------------------------
//  ED4_text_terminal
// --------------------------------------------------------------------------------

ED4_returncode ED4_text_terminal::Show(int IF_DEBUG(refresh_all), int is_cleared)
{
    e4_assert(update_info.refresh || refresh_all);
    ED4_ROOT->temp_device->push_clip_scale();
    if (adjust_clipping_rectangle()) {
        if (update_info.clear_at_refresh && !is_cleared) {
            clear_background();
        }
        draw();
    }
    ED4_ROOT->temp_device->pop_clip_scale();

    ED4_cursor *cursor = &ED4_ROOT->temp_ed4w->cursor;
    if (this == cursor->owner_of_cursor){
        ED4_ROOT->temp_device->push_clip_scale();
        cursor->ShowCursor(0, ED4_C_NONE, 0);
        ED4_ROOT->temp_device->pop_clip_scale();
    }

    return ED4_R_OK;
}

ED4_returncode ED4_text_terminal::draw( int /*only_text*/ )
{
    AW_pos x, y;
    AW_pos text_x, text_y;

    calc_world_coords( &x, &y );
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x, &y );

    text_x = x + CHARACTEROFFSET; // don't change
    text_y = y + INFO_TERM_TEXT_YOFFSET;

    ED4_ROOT->temp_device->top_font_overlap    = 1;
    ED4_ROOT->temp_device->bottom_font_overlap = 1;

    if (is_species_name_terminal()) {
        GB_CSTR real_name      = to_species_name_terminal()->get_displayed_text();
        int     is_marked      = 0;
        int     width_of_char;
        int     height_of_char = -1;
        int     paint_box      = !parent->flag.is_consensus && !parent->parent->parent->flag.is_SAI; // do not paint marked-boxes for consensi and SAIs

        if (paint_box) {
            ED4_species_manager *species_man = get_parent(ED4_L_SPECIES)->to_species_manager();
            GBDATA *gbd = species_man->get_species_pointer();

            if (gbd) {
                is_marked = GB_read_flag(gbd);
            }

            width_of_char = ED4_ROOT->font_group.get_width(ED4_G_STANDARD);
            height_of_char = ED4_ROOT->font_group.get_height(ED4_G_STANDARD);
#define MIN_MARK_BOX_SIZE 8
            if (width_of_char<MIN_MARK_BOX_SIZE) width_of_char = MIN_MARK_BOX_SIZE;
            if (height_of_char<MIN_MARK_BOX_SIZE) height_of_char = MIN_MARK_BOX_SIZE;
#undef MIN_MARK_BOX_SIZE
        }
        else {
            width_of_char = 0;
        }

        if (flag.selected) {
            ED4_ROOT->temp_device->box(ED4_G_SELECTED, AW_TRUE, x, y, extension.size[WIDTH], text_y-y+1, (AW_bitset)-1, 0, 0);
        }
        ED4_ROOT->temp_device->text( ED4_G_STANDARD, real_name, text_x+width_of_char, text_y, 0, 1, 0, 0, 0);

        if (paint_box) {
            int xsize = (width_of_char*6)/10;
            int ysize = (height_of_char*6)/10;
            int xoff  = xsize>>1;
            int yoff  = 0; 
            int bx    = int(text_x+xoff);
            int by    = int(text_y-(yoff+ysize));

            ED4_ROOT->temp_device->box(ED4_G_STANDARD, AW_TRUE, bx, by, xsize, ysize, (AW_bitset)-1, 0, 0);
            if (!is_marked && xsize>2 && ysize>2) {
                ED4_ROOT->temp_device->clear_part(bx+1, by+1, xsize-2, ysize-2, (AW_bitset)-1);
            }
        }
    }
    else {
        char *db_pointer = resolve_pointer_to_string_copy();

        if (is_sequence_info_terminal()) {
            ED4_ROOT->temp_device->text( ED4_G_STANDARD, db_pointer, text_x, text_y, 0, 1, 0, 0, 4);
        }
        else if (is_pure_text_terminal()) { // normal text (i.e. remark)
            text_y += (SEQ_TERM_TEXT_YOFFSET-INFO_TERM_TEXT_YOFFSET);
            ED4_ROOT->temp_device->text( ED4_G_SEQUENCES, db_pointer, text_x, text_y, 0, 1, 0, 0, 0);
        }
        else{
            GB_CORE;
        }

        free(db_pointer);
    }
    ED4_ROOT->temp_device->top_font_overlap    = 0;
    ED4_ROOT->temp_device->bottom_font_overlap = 0;

    return ( ED4_R_OK );
}

ED4_text_terminal::ED4_text_terminal(GB_CSTR temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_terminal( temp_id, x, y, width, height, temp_parent )
{
}


ED4_text_terminal::~ED4_text_terminal()
{
}



