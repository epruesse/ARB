// =============================================================== //
//                                                                 //
//   File      : ED4_text_terminals.cxx                            //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <ed4_extern.hxx>

#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_block.hxx"
#include "ed4_nds.hxx"
#include "ed4_visualizeSAI.hxx"
#include "ed4_ProteinViewer.hxx"
#include "ed4_protein_2nd_structure.hxx"
#include "ed4_seq_colors.hxx"

#include <aw_preset.hxx>
#include <aw_awar.hxx>

#include <AW_helix.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>

#include <st_window.hxx>
#include <arbdbt.h>
#include <ad_colorset.h>

#include <iostream>

inline void ensure_buffer(char*& buffer, size_t& buffer_size, size_t needed) {
    if (needed>buffer_size) {
        delete [] buffer;
        buffer_size = needed+10;
        buffer      = new char[buffer_size];

        memset(buffer, ' ', buffer_size-1);
        buffer[buffer_size-1] = 0;
    }
}

ED4_returncode ED4_consensus_sequence_terminal::draw() {
    static char   *buffer      = 0;
    static size_t  buffer_size = 0;

    AW_pos x, y;
    calc_world_coords(&x, &y);
    current_ed4w()->world_to_win_coords(&x, &y);

    PosRange index_range = calc_update_interval();
    if (index_range.is_empty()) return ED4_R_OK;

    AW_pos text_x = x + CHARACTEROFFSET; // don't change
    AW_pos text_y = y + SEQ_TERM_TEXT_YOFFSET;

    buffer_size = 0;

    if (index_range.is_unlimited()) {
        e4_assert(MAXSEQUENCECHARACTERLENGTH == 0); // allow missing update interval when no seqdata is present (yet)

        const char *no_data = "No consensus data";
        size_t      len     = strlen(no_data);
        ensure_buffer(buffer, buffer_size, len+1);
        memcpy(buffer, no_data, len);

        index_range = ExplicitRange(index_range, buffer_size-1);
    }
    else {
        const ED4_remap *rm = ED4_ROOT->root_group_man->remap();
        
        index_range = rm->clip_screen_range(index_range);
        if (index_range.is_empty()) return ED4_R_OK;

        ExplicitRange seq_range = ExplicitRange(rm->screen_to_sequence(index_range), MAXSEQUENCECHARACTERLENGTH);
        index_range             = rm->sequence_to_screen(seq_range);

        char *cons = 0;
        if (!seq_range.is_empty()) {
            cons = GB_give_buffer(seq_range.size()+1);
            get_char_table().build_consensus_string_to(cons, seq_range, ED4_ROOT->get_consensus_params());
        }

        ensure_buffer(buffer, buffer_size, index_range.end()+1);

        ED4_reference *ref            = ED4_ROOT->reference;
        bool           only_show_diff = ref->only_show_diff_for(this);

        for (int pos = index_range.start(); pos <= index_range.end(); ++pos) {
            int seq_pos = rm->screen_to_sequence(pos);
            if (seq_pos<0) {
                buffer[pos] = ' ';
            }
            else {
                char c      = cons[seq_pos-seq_range.start()];
                buffer[pos] = only_show_diff ? ref->convert(c, seq_pos) : c;
                e4_assert(buffer[pos]);
            }
        }
    }

    if (buffer_size) {
        current_device()->set_vertical_font_overlap(true);
        current_device()->text(ED4_G_SEQUENCES, buffer, text_x, text_y, 0, AW_SCREEN, index_range.end()+1);
        current_device()->set_vertical_font_overlap(false);
    }

    return (ED4_R_OK);
}

struct ShowHelix_cd {
    AW_helix *helix;
    int       real_sequence_length;
};

static bool ED4_show_helix_on_device(AW_device *device, int gc, const char *opt_string, size_t /*opt_string_size*/, size_t start, size_t size,
                                     AW_pos x, AW_pos y, AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/,
                                     AW_CL cduser) // , AW_CL real_sequence_length, AW_CL cd2)
{
    ShowHelix_cd    *cd     = (ShowHelix_cd*)cduser;
    AW_helix        *helix  = cd->helix;
    const ED4_remap *rm     = ED4_ROOT->root_group_man->remap();
    char            *buffer = GB_give_buffer(size+1);
    long             i, j, k;

    size = std::min(rm->get_max_screen_pos(), size);

    for (k=0; size_t(k)<size; k++) {
        i = rm->screen_to_sequence(k+start);

        BI_PAIR_TYPE pairType = helix->pairtype(i);
        if (pairType == HELIX_NONE) {
            buffer[k] = ' ';
        }
        else {
            j             = helix->opposite_position(i);
            char pairchar = j<cd->real_sequence_length ? opt_string[j] : '.';
            buffer[k]     = helix->get_symbol(opt_string[i], pairchar, pairType);
        }
    }
    buffer[size] = 0;
    return device->text(gc, buffer, x, y);
}

static bool ED4_show_protein_match_on_device(AW_device *device, int gc, const char *protstruct, size_t /* protstruct_len */, size_t start, size_t size,
                                             AW_pos x, AW_pos y, AW_pos /* opt_ascent */, AW_pos /* opt_descent */,
                                             AW_CL cl_protstruct) // , AW_CL /* real_sequence_length */, AW_CL cd2)
{
    /*! \brief Calls ED4_pfold_calculate_secstruct_match() for the visible area in the
     *         editor to compute the protein secondary structure match and outputs the
     *         result to the device.
     *  \param[in] protstruct    The protein structure (primary or secondary) that should be compared to \a cl_protstruct
     *  \param[in] cl_protstruct The reference protein secondary structure SAI
     */

    GB_ERROR error = 0;
    // TODO: proper use of ED4_remap?
    const ED4_remap *rm = ED4_ROOT->root_group_man->remap();
    char *buffer = GB_give_buffer(size+1);
    if (!buffer) {
        error = GB_export_error("Out of memory.");
    }
    else {
        error = ED4_pfold_calculate_secstruct_match((const unsigned char *)cl_protstruct,
                                                    (const unsigned char *)protstruct,
                                                    rm->screen_to_sequence(start),
                                                    rm->screen_to_sequence(start + size),
                                                    buffer,
                                                    (PFOLD_MATCH_METHOD)ED4_ROOT->aw_root->awar(PFOLD_AWAR_MATCH_METHOD)->read_int());
    }
    if (error) {
        aw_message(error);
        return false;
    }
    
    buffer[size] = 0;
    return device->text(gc, buffer, x, y);
}

ED4_returncode ED4_orf_terminal::draw() {
    // draw aminoacid ORFs below the DNA sequence

    static int    color_is_used[ED4_G_DRAG];
    static char **colored_strings        = 0;
    static int    len_of_colored_strings = 0;

    AW_device *device = current_device();

    // @@@ DRY calculation of index-range to-be-updated (done in several draw functions)
    AW_pos world_x, world_y;
    calc_world_coords(&world_x, &world_y);
    current_ed4w()->world_to_win_coords(&world_x, &world_y);

    AW_pos text_x = world_x + CHARACTEROFFSET; // don't change
    AW_pos text_y = world_y + SEQ_TERM_TEXT_YOFFSET;

    const ED4_remap *rm = ED4_ROOT->root_group_man->remap();

    ExplicitRange index_range = rm->clip_screen_range(calc_update_interval());
    {
        int max_seq_len = aaSeqLen;
        int max_seq_pos = rm->sequence_to_screen(max_seq_len-1);
        index_range     = ExplicitRange(PosRange(index_range), max_seq_pos);
    }

    if (index_range.is_empty()) {
        const char *no_data = "No sequence data";
        size_t      len     = strlen(no_data);

        device->text(ED4_G_STANDARD, no_data, text_x, text_y, 0, AW_SCREEN, len);
        return ED4_R_OK;
    }

    if (index_range.end() >= len_of_colored_strings) {
        len_of_colored_strings = index_range.end() + 256;
        if (!colored_strings) ARB_calloc(colored_strings, ED4_G_DRAG);

        for (int i=0; i<ED4_G_DRAG; i++) {
            freeset(colored_strings[i], (char *)ARB_alloc(sizeof(char) * (len_of_colored_strings+1)));
            memset(colored_strings[i], ' ', len_of_colored_strings);
            colored_strings[i][len_of_colored_strings] = 0;
        }
    }

    int seq_end = rm->screen_to_sequence(index_range.end());

    // mark all strings as unused
    memset(color_is_used, 0, sizeof(color_is_used));

    int iDisplayMode = ED4_ROOT->aw_root->awar(AWAR_PROTVIEW_DISPLAY_OPTIONS)->read_int();
    {
        const unsigned char *aaSequence_u = (const unsigned char *)aaSequence;
        const unsigned char *aaColor_u    = (const unsigned char *)aaColor;

        if (iDisplayMode == PV_AA_NAME || iDisplayMode == PV_AA_CODE) {
            // transform strings, compress if needed
            ED4_reference *ref = ED4_ROOT->reference;
            ref->expand_to_length(seq_end);

            char *char_2_char = ED4_ROOT->sequence_colors->char_2_char_aa;
            char *char_2_gc   = ED4_ROOT->sequence_colors->char_2_gc_aa;

            if (iDisplayMode == PV_AA_NAME) {
                for (int scr_pos=index_range.start(); scr_pos <= index_range.end(); scr_pos++) {
                    int           seq_pos = rm->screen_to_sequence(scr_pos);
                    unsigned char c       = aaSequence_u[seq_pos];
                    unsigned char cc      = aaColor_u[seq_pos];
                    int           gc      = char_2_gc[safeCharIndex(cc)];

                    color_is_used[gc] = scr_pos+1;
                    colored_strings[gc][scr_pos] = char_2_char[safeCharIndex(c)];
                }
            }
            else {
                for (int scr_pos=index_range.start(); scr_pos <= index_range.end(); scr_pos++) {
                    int  seq_pos = rm->screen_to_sequence(scr_pos);
                    char c       = aaSequence_u[seq_pos];
                    int  gc      = char_2_gc[safeCharIndex(c)];

                    color_is_used[gc] = scr_pos+1;
                    colored_strings[gc][scr_pos] = char_2_char[safeCharIndex(c)];
                }
            }
        }

        // paint background
        if ((iDisplayMode == PV_AA_CODE) || (iDisplayMode == PV_AA_BOX)) {
            AW_pos     width        = ED4_ROOT->font_group.get_width(ED4_G_HELIX);
            const int  real_left    = index_range.start();
            const int  real_right   = index_range.end();
            AW_pos     x2           = text_x + width*real_left;
            AW_pos     y1           = world_y;
            AW_pos     y2           = text_y+1;
            AW_pos     height       = y2-y1+1;
            int        color        = ED4_G_STANDARD;
            char      *char_2_gc_aa = ED4_ROOT->sequence_colors->char_2_gc_aa;

            for (int i = real_left; i <= real_right; i++, x2 += width) {
                int  new_pos = rm->screen_to_sequence(i); // getting the real position of the base in the sequence
                char base    = aaSequence_u[new_pos];

                if (isupper(base) || (base=='*')) {
                    AW_pos x1  = x2-width; // store current x pos to x1
                    x2 += width*2; // add 2 char width to x2
                    i  += 2; // jump two pos

                    int gcChar =  char_2_gc_aa[safeCharIndex(base)];
                    if ((gcChar>=0) && (gcChar<ED4_G_DRAG)) {
                        color = gcChar;
                        if (iDisplayMode == PV_AA_BOX) {
                            device->box(color, AW::FillStyle::SOLID, x1, y1, width*3, height);
                        }
                        else {
                            double    rad_x    = width*1.5;
                            double    rad_y    = height*0.7;
                            double    center_x = x1+rad_x;
                            const int DRAW_DEG = 62;

                            device->arc(ED4_G_SEQUENCES, AW::FillStyle::EMPTY, center_x, y1, rad_x, rad_y,   0,  DRAW_DEG);
                            device->arc(ED4_G_SEQUENCES, AW::FillStyle::EMPTY, center_x, y1, rad_x, rad_y, 180, -DRAW_DEG);
                        }
                    }
                }
            }
        }
    }

    if ((iDisplayMode == PV_AA_NAME) || (iDisplayMode == PV_AA_CODE)) {
        device->set_vertical_font_overlap(true);
        // output strings
        for (int gc = 0; gc < ED4_G_DRAG; gc++) {
            if (color_is_used[gc] && (int)strlen(colored_strings[gc]) >= color_is_used[gc]) {
                device->text(gc, colored_strings[gc], text_x, text_y, 0, AW_SCREEN, color_is_used [gc]);
                memset(colored_strings[gc] + index_range.start(), ' ', index_range.size()); // clear string
            }
        }
        device->set_vertical_font_overlap(false);
    }

    return (ED4_R_OK);
}

ED4_returncode ED4_sequence_terminal::draw() {
    static int    color_is_used[ED4_G_DRAG];
    static char **colored_strings        = 0;
    static int    len_of_colored_strings = 0;

#if defined(TRACE_REFRESH)
    fprintf(stderr, "ED4_sequence_terminal::draw for id='%s'\n", id); fflush(stderr);
#endif

    AW_device *device = current_device();

    int max_seq_len;
    resolve_pointer_to_char_pntr(&max_seq_len);
    e4_assert(max_seq_len>0);

    AW_pos world_x, world_y;
    calc_world_coords(&world_x, &world_y);
    current_ed4w()->world_to_win_coords(&world_x, &world_y);

    AW_pos text_x = world_x + CHARACTEROFFSET;    // don't change
    AW_pos text_y = world_y + SEQ_TERM_TEXT_YOFFSET;

    const ED4_remap *rm = ED4_ROOT->root_group_man->remap();

    ExplicitRange index_range = rm->clip_screen_range(calc_update_interval());
    if (index_range.is_empty()) return ED4_R_OK;

    int left  = index_range.start(); // @@@ do similar to ED4_orf_terminal::draw here
    int right = index_range.end();

    {
        int max_seq_pos = rm->sequence_to_screen(max_seq_len-1);

        if (right>max_seq_len) right = max_seq_pos;
        if (left>right) {
            const char *no_data = "No sequence data";
            size_t      len     = strlen(no_data);

            device->text(ED4_G_STANDARD, no_data, text_x, text_y, 0, AW_SCREEN, len);
            return ED4_R_OK;
        }
    }

    if (right >= len_of_colored_strings) {
        len_of_colored_strings = right + 256;
        if (!colored_strings) ARB_calloc(colored_strings, ED4_G_DRAG);

        for (int i=0; i<ED4_G_DRAG; i++) {
            freeset(colored_strings[i], (char *)malloc(sizeof(char) * (len_of_colored_strings+1)));
            memset(colored_strings[i], ' ', len_of_colored_strings);
            colored_strings[i][len_of_colored_strings] = 0;
        }
    }

    int seq_start = rm->screen_to_sequence(left); // real start of sequence
    int seq_end = rm->screen_to_sequence(right);

    // mark all strings as unused
    memset(color_is_used, 0, sizeof(color_is_used));

    // transform strings, compress if needed
    {
        ED4_reference *ref        = ED4_ROOT->reference;
        unsigned char *db_pointer = (unsigned char *)resolve_pointer_to_string_copy();

        ref->expand_to_length(seq_end);

        GB_alignment_type aliType = GetAliType();
        char *char_2_char = (aliType && (aliType==GB_AT_AA)) ? ED4_ROOT->sequence_colors->char_2_char_aa : ED4_ROOT->sequence_colors->char_2_char;
        char *char_2_gc   = (aliType && (aliType==GB_AT_AA)) ? ED4_ROOT->sequence_colors->char_2_gc_aa : ED4_ROOT->sequence_colors->char_2_gc;

        bool only_show_diff = ref->only_show_diff_for(this);
        for (int scr_pos=left; scr_pos <= right; scr_pos++) {
            int seq_pos = rm->screen_to_sequence(scr_pos);
            int c = db_pointer[seq_pos];
            int gc = char_2_gc[c];

            color_is_used[gc] = scr_pos+1;
            colored_strings[gc][scr_pos] = char_2_char[only_show_diff ? ref->convert(c, seq_pos) : c];
        }

        free(db_pointer);
    }

    // Set background

    {
        GB_transaction       ta(GLOBAL_gb_main);
        ST_ML_Color         *statColors   = 0;
        char                *searchColors = results().buildColorString(this, seq_start, seq_end); // defined in ED4_SearchResults class : ED4_search.cxx
        ED4_species_manager *spec_man     = get_parent(ED4_L_SPECIES)->to_species_manager();
        int                  color_group  = AW_color_groups_active() ? GBT_get_color_group(spec_man->get_species_pointer()) : 0;

        PosRange selection;
        int      is_selected = ED4_get_selected_range(this, selection);
        int      is_marked   = GB_read_flag(spec_man->get_species_pointer());

        if (species_name &&
            ED4_ROOT->column_stat_activated &&
            (st_ml_node || (st_ml_node = STAT_find_node_by_name(ED4_ROOT->st_ml, this->species_name))))
            {
                statColors = STAT_get_color_string(ED4_ROOT->st_ml, 0, st_ml_node, seq_start, seq_end);
            }

        const char *saiColors = 0;

        if (species_name                       &&
            ED4_ROOT->visualizeSAI             &&
            spec_man->get_type() != ED4_SP_SAI &&
            (is_marked || ED4_ROOT->visualizeSAI_allSpecies))
        {
            saiColors = ED4_getSaiColorString(ED4_ROOT->aw_root, seq_start, seq_end);
        }

        if (statColors || searchColors || is_marked || is_selected || color_group || saiColors) {
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

            if (is_selected && selection.is_unlimited()) {
                selection = ExplicitRange(selection, rm->screen_to_sequence(real_right));
            }

            for (i = real_left; i <= real_right; i++, x2 += width) {
                int new_pos = rm->screen_to_sequence(i);  // getting the real position of the base in the sequence

                // Note: if you change background color priorities,
                // please update help in ../HELP_SOURCE/oldhelp/e4_background_priority.hlp@COLOR_PRIORITY

                if (searchColors && searchColors[new_pos]) {
                    color = searchColors[new_pos];
                }
                else if (is_selected && selection.contains(new_pos)) {
                    color = ED4_G_SELECTED;
                }
                else if (statColors) {
                    color = statColors[new_pos] + ED4_G_CBACK_0;
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
                    if (x2>old_x) {
                        if (old_color!=ED4_G_STANDARD) {
                            device->box(old_color, AW::FillStyle::SOLID, old_x, y1, x2-old_x, height); // paints the search pattern background
                        }
                    }
                    old_x = x2;
                    old_color = color;
                }
            }

            if (x2>old_x) {
                if (color!=ED4_G_STANDARD) {
                    device->box(color, AW::FillStyle::SOLID, old_x, y1, x2-old_x, height);
                }
            }
        }
    }

    device->set_vertical_font_overlap(true);

    if (shall_display_secstruct_info) {
        if (ED4_ROOT->helix->is_enabled()) {
            // output helix
            int   db_len;
            char *db_pointer = resolve_pointer_to_string_copy(&db_len);

            e4_assert(size_t(db_len) == ED4_ROOT->helix->size());
            ShowHelix_cd  cd         = { ED4_ROOT->helix, max_seq_len };
            device->text_overlay(ED4_G_HELIX,
                                 (char *)db_pointer, db_len,
                                 AW::Position(text_x,  text_y + ED4_ROOT->helix_spacing), 0.0,  AW_ALL_DEVICES_UNSCALED,
                                 (AW_CL)&cd, 1.0, 1.0, ED4_show_helix_on_device);
            free(db_pointer);
        }

        if (ED4_ROOT->protstruct) {
            // output protein structure match
            ED4_species_manager *spec_man = get_parent(ED4_L_SPECIES)->to_species_manager();
            if (spec_man->get_type() != ED4_SP_SAI && ED4_ROOT->aw_root->awar(PFOLD_AWAR_ENABLE)->read_int()) {  // should do a remap
                int   db_len;
                char *protstruct = resolve_pointer_to_string_copy(&db_len);
                
                if (protstruct) {
                    device->text_overlay(ED4_G_HELIX,
                                         protstruct, db_len,
                                         AW::Position(text_x,  text_y + ED4_ROOT->helix_spacing),  0.0,  AW_ALL_DEVICES_UNSCALED,
                                         (AW_CL)ED4_ROOT->protstruct, 1.0, 1.0, ED4_show_protein_match_on_device);
                    free(protstruct);
                }
            }
        }
    }
    // output strings
    {
        int gc;
        for (gc = 0; gc < ED4_G_DRAG; gc++) {
            if (!color_is_used[gc]) continue;
            device->text(gc, colored_strings[gc], text_x, text_y, 0, AW_SCREEN, color_is_used[gc]);
            memset(colored_strings[gc] + left, ' ', right-left+1); // clear string
        }
    }

    device->set_vertical_font_overlap(false);

    return (ED4_R_OK);
}


ED4_returncode ED4_sequence_info_terminal::draw() {
    AW_pos x, y;
    calc_world_coords(&x, &y);
    current_ed4w()->world_to_win_coords(&x, &y);

    AW_pos text_x = x + CHARACTEROFFSET; // don't change
    AW_pos text_y = y+INFO_TERM_TEXT_YOFFSET;

    char    buffer[10];
    GBDATA *gbdata = data();

    if (gbdata) {
        GB_push_transaction(gbdata);
        buffer[0] = '0' + GB_read_security_write(gbdata);
        GB_pop_transaction(gbdata);
    }
    else {
        buffer[0] = ' ';
    }
    strncpy(&buffer[1], this->id, 8);
    buffer[9] = 0;

    if (containing_species_manager()->is_selected()) {
        current_device()->box(ED4_G_SELECTED, AW::FillStyle::SOLID, x, y, extension.size[WIDTH], text_y-y+1);
    }

    current_device()->set_vertical_font_overlap(true);
    current_device()->text(ED4_G_STANDARD, buffer, text_x, text_y, 0, AW_SCREEN, 0);
    current_device()->set_vertical_font_overlap(false);

    return (ED4_R_OK);

}

// ---------------------------
//      ED4_text_terminal

ED4_returncode ED4_text_terminal::Show(int IF_ASSERTION_USED(refresh_all), int is_cleared)
{
    e4_assert(update_info.refresh || refresh_all);
    current_device()->push_clip_scale();
    if (adjust_clipping_rectangle()) {
        if (update_info.clear_at_refresh && !is_cleared) {
            clear_background();
        }
        draw();
    }
    current_device()->pop_clip_scale();

    return ED4_R_OK;
}

ED4_returncode ED4_text_terminal::draw() {
    AW_pos x, y;
    calc_world_coords(&x, &y);
    current_ed4w()->world_to_win_coords(&x, &y);

    AW_pos text_x = x + CHARACTEROFFSET; // don't change
    AW_pos text_y = y + INFO_TERM_TEXT_YOFFSET;

    current_device()->set_vertical_font_overlap(true);

    if (is_species_name_terminal()) {
        GB_CSTR real_name      = to_species_name_terminal()->get_displayed_text();
        int     width_of_char;
        int     height_of_char = -1;
        bool    paint_box      = inside_species_seq_manager();
        bool    is_marked      = false;

        if (paint_box) {
            ED4_species_manager *species_man = get_parent(ED4_L_SPECIES)->to_species_manager();
            GBDATA *gbd = species_man->get_species_pointer();

            if (gbd) {
                GB_transaction ta(gbd);
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

        if (containing_species_manager()->is_selected()) {
            current_device()->box(ED4_G_SELECTED, AW::FillStyle::SOLID, x, y, extension.size[WIDTH], text_y-y+1);
        }
        current_device()->text(ED4_G_STANDARD, real_name, text_x+width_of_char, text_y, 0, AW_SCREEN, 0);

        if (paint_box) {
            int xsize = (width_of_char*6)/10;
            int ysize = (height_of_char*6)/10;
            int xoff  = xsize>>1;
            int yoff  = 0;
            int bx    = int(text_x+xoff);
            int by    = int(text_y-(yoff+ysize));

            current_device()->box(ED4_G_STANDARD, AW::FillStyle::SOLID, bx, by, xsize, ysize);
            if (!is_marked && xsize>2 && ysize>2) {
                current_device()->clear_part(bx+1, by+1, xsize-2, ysize-2, AW_ALL_DEVICES);
            }
        }
    }
    else {
        char *db_pointer = resolve_pointer_to_string_copy();

        if (is_sequence_info_terminal()) {
            current_device()->text(ED4_G_STANDARD, db_pointer, text_x, text_y, 0, AW_SCREEN, 4);
        }
        else if (is_pure_text_terminal()) { // normal text (i.e. remark)
            text_y += (SEQ_TERM_TEXT_YOFFSET-INFO_TERM_TEXT_YOFFSET);
            current_device()->text(ED4_G_SEQUENCES, db_pointer, text_x, text_y, 0, AW_SCREEN, 0);
        }
        else {
            e4_assert(0); // unknown terminal type
        }

        free(db_pointer);
    }
    current_device()->set_vertical_font_overlap(false);

    return (ED4_R_OK);
}

ED4_text_terminal::ED4_text_terminal(const ED4_objspec& spec_, GB_CSTR temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_terminal(spec_, temp_id, x, y, width, height, temp_parent)
{
}

