#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <iostream.h>
#include <fstream.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx> 
#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <awt_canvas.hxx>
#include <awt.hxx>
#define _USE_AW_WINDOW
#include <BI_helix.hxx>
#include <sec_graphic.hxx>
#include "secedit.hxx"

// yadhus includes
#include "../EDIT4/ed4_defs.hxx"
#include "../EDIT4/ed4_class.hxx"

#if defined(FREESTANDING)
AW_HEADER_MAIN
#endif

extern GBDATA *gb_main;
SEC_graphic *SEC_GRAPHIC = 0;

void SEC_distance_between_strands_changed_cb(AW_root *awr, AW_CL cl_ntw)
{
    SEC_root *root = SEC_GRAPHIC->sec_root;
    double distance = awr->awar(AWAR_SECEDIT_DIST_BETW_STRANDS)->read_float();
    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;

    root->set_distance_between_strands(distance);
    root->update(0);
    ntw->refresh();
}

void SEC_pair_def_changed_cb(AW_root */*awr*/, AW_CL cl_ntw)
{
    SEC_root *root = SEC_GRAPHIC->sec_root;
    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;

    SEC_GRAPHIC->bond.update(SEC_GRAPHIC->aw_root);
    root->update(0);
    ntw->refresh();
}


#if defined(FREESTANDING)
void SEC_quit_cb(AW_window *aww, AW_CL, AW_CL){
    AWUSE(aww);
    GB_exit(gb_main);
    exit(0);
}
#endif

void SEC_template_changed_cb(GBDATA *gb_seq, AWT_canvas *ntw, GB_CB_TYPE type){
    SEC_root * sec_root = SEC_GRAPHIC->sec_root;
    GB_transaction tscope(gb_main);

    delete sec_root->template_sequence;
    sec_root->template_sequence = 0;
    sec_root->template_length = -1;

    if (type == GB_CB_DELETE){
        sec_root->gb_template = 0;
    }else{
        sec_root->template_sequence = GB_read_string(gb_seq);
        sec_root->template_length = GB_read_string_count(gb_seq);
    }
    if (sec_root->helix){
        sec_root->update();
        ntw->recalc_size();
        ntw->refresh();
    }
}

void SEC_sequence_changed_cb(GBDATA *gb_seq, AWT_canvas *ntw, GB_CB_TYPE type){
    SEC_root * sec_root = SEC_GRAPHIC->sec_root;
    GB_transaction tscope(gb_main);

    delete sec_root->sequence;
    sec_root->sequence = 0;
    sec_root->sequence_length = -1;

    if (type == GB_CB_DELETE || !sec_root->gb_sequence){
        sec_root->gb_sequence = 0;
    }else{
        sec_root->sequence = GB_read_string(gb_seq);
        sec_root->sequence_length = GB_read_string_count(gb_seq);
    }
    if (sec_root->helix){
        sec_root->update();
        ntw->recalc_size();
        ntw->refresh();
    }
}

static inline int isInView(AW_pos x, AW_pos y, const AW_rectangle &view) {
    double xborder = view.b - view.t;
    double yborder = view.r - view.l;

    xborder = xborder*0.1;
    yborder = yborder*0.1;

    return x >= (view.l+xborder) &&
        y >= (view.t+yborder) &&
        x <= (view.r-xborder) &&
        y <= (view.b-yborder);
}

void SEC_cursor_position_changed_cb(AW_root *awr, AWT_canvas *ntw) {
    SEC_root * sec_root = SEC_GRAPHIC->sec_root;
    GB_transaction tscope(gb_main);
    int seq_pos = awr->awar_int(AWAR_CURSOR_POSITION)->read_int()-1; // sequence position is starting with 1
    sec_root->set_cursor(seq_pos);
    if (sec_root->helix) {
        sec_root->update();
        ntw->recalc_size();
        ntw->refresh();

        // check if cursor is in view

        AW_device *device = ntw->aww->get_device(AW_MIDDLE_AREA);
        AW_rectangle screen;
        device->get_area_size(&screen);

        AW_pos x1, y1, x2, y2;
        sec_root->get_last_drawed_cursor_position(x1, y1, x2, y2);

        if (x1 || y1 || x2 || y2) { // valid position (calls at startup have no valid positions)
            AW_pos X1, Y1, X2, Y2;

            device->transform(x1, y1, X1, Y1);
            device->transform(x2, y2, X2, Y2);

            if (!isInView(X1, Y1, screen) || !isInView(X2, Y2, screen)) { // cursor is outside view
                double curs_x = (X1+X2)/2;
                double curs_y = (Y1+Y2)/2;
                double offsetx = screen.l + (screen.l+screen.r)/2 - curs_x;
                double offsety = screen.t + (screen.t+screen.b)/2 - curs_y;

#if defined(DEBUG) && 1
                printf("Auto-Scroll: offsetx = %f / offsety = %f\n", offsetx, offsety);
#endif
                ntw->scroll(NULL, (int)(-offsetx), (int)(-offsety), AW_FALSE);
                ntw->refresh();
            }
        }
    }
}

void SEC_center_cb(AW_window *aw, AWT_canvas *ntw, AW_CL)
{
    ntw->recalc_size();
    ntw->refresh(); // do one refresh to calculate current cursor position

    AW_device *device = aw->get_device(AW_MIDDLE_AREA);
    AW_rectangle screen;
    device->get_area_size(&screen);

    AW_pos x1, y1, x2, y2;
    SEC_root * sec_root = SEC_GRAPHIC->sec_root;
    sec_root->get_last_drawed_cursor_position(x1, y1, x2, y2);

    if (x1 || y1 || x2 || y2) { // valid position (calls at startup have no valid positions)
        AW_pos X1, Y1, X2, Y2;

        device->transform(x1, y1, X1, Y1);
        device->transform(x2, y2, X2, Y2);

        double curs_x = (X1+X2)/2;
        double curs_y = (Y1+Y2)/2;
        double offsetx = screen.l + (screen.l+screen.r)/2 - curs_x;
        double offsety = screen.t + (screen.t+screen.b)/2 - curs_y;

        ntw->scroll(NULL, (int)(-offsetx), (int)(-offsety), AW_FALSE);
        ntw->refresh();
    }
}


void SEC_species_name_changed_cb(AW_root *awr, AWT_canvas *ntw){
    SEC_root * sec_root = SEC_GRAPHIC->sec_root;
    GB_transaction tscope(gb_main);
    if (sec_root->gb_sequence){ // remove old callback
        GB_remove_callback(sec_root->gb_sequence,(GB_CB_TYPE)(GB_CB_DELETE | GB_CB_CHANGED),(GB_CB)SEC_sequence_changed_cb, (int *)ntw);
        sec_root->gb_sequence = 0;
    }
    char *species_name = awr->awar_string(AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species = GBT_find_species(gb_main,species_name);
    if (gb_species){
        char *ali_name = GBT_get_default_alignment(gb_main);
        sec_root->gb_sequence = GBT_read_sequence(gb_species, ali_name);
        if (sec_root->gb_sequence){
            GB_add_callback(sec_root->gb_sequence,(GB_CB_TYPE)(GB_CB_DELETE | GB_CB_CHANGED),(GB_CB)SEC_sequence_changed_cb, (int *)ntw);
        }
        delete ali_name;
    }

    sec_root->seqTerminal = ED4_find_seq_terminal(species_name); // initializing the seqTerminal to get the current terminal - yadhu

    SEC_sequence_changed_cb( sec_root->gb_sequence, ntw, GB_CB_CHANGED);

    delete species_name;
}

void SEC_root::init_sequences(AW_root *awr, AWT_canvas *ntw){
    GB_transaction tscope(gb_main);


    GBDATA *gb_species = GBT_find_SAI(gb_main,"ECOLI");
    if (gb_species){
        char *ali_name = GBT_get_default_alignment(gb_main);
        gb_template = GBT_read_sequence(gb_species, ali_name);
        if (gb_template){
            GB_add_callback(gb_template,(GB_CB_TYPE)(GB_CB_DELETE | GB_CB_CHANGED),(GB_CB)SEC_template_changed_cb, (int *)ntw);
        }
        delete ali_name;
    }else{
        aw_message("ECOLI SAI not found");
    }

    SEC_template_changed_cb( gb_template,ntw, GB_CB_CHANGED);

    awr->awar_string(AWAR_SPECIES_NAME,"",gb_main)->add_callback((AW_RCB1)SEC_species_name_changed_cb, (AW_CL)ntw);;
    SEC_species_name_changed_cb(awr,ntw);

    awr->awar_string(AWAR_CURSOR_POSITION,"",gb_main)->add_callback((AW_RCB1)SEC_cursor_position_changed_cb, (AW_CL)ntw);;
    SEC_cursor_position_changed_cb(awr, ntw);

    helix = new BI_helix(awr);
    GB_ERROR error = helix->init(gb_main);
    if (error){
        aw_message(error);
    }

    ecoli = new BI_ecoli_ref();
    ecoli->init(gb_main);

    /******************************************************************************
     * Initialize the two strings that protocoll if a valid base-position was found
     ******************************************************************************/
    //if (number_found == NULL) {
    //number_found = new int[template_length];
    //}
    //if (found_abs_pos == NULL) {
    //	found_abs_pos = new char[template_length];
    // }

    SEC_root * sec_root = SEC_GRAPHIC->sec_root;
    sec_root->update(0);
}

void sec_mode_event( AW_window *aws, AWT_canvas *ntw, AWT_COMMAND_MODE mode)
{
    SEC_root *sec_root = SEC_GRAPHIC->sec_root;
    AWUSE(aws);
    const char *text = 0;

    sec_root->show_constraints = 0;

    switch(mode){
        case AWT_MODE_ZOOM: {
            text="ZOOM MODE    LEFT: drag to zoom   RIGHT: zoom out";
            break;
        }
	case AWT_MODE_LZOOM: {
            text="LOGICAL ZOOM MODE    LEFT: drag to zoom   RIGHT: zoom out";
            break;
        }
        case AWT_MODE_MOVE: {
            text="HELIX MODE    LEFT: build helix   RIGHT: remove helix";
            break;
        }
        case AWT_MODE_SETROOT: {
            text="SET ROOT MODE    LEFT: Set logical center of secondary structure";
            break;
        }
        case AWT_MODE_ROT: {
            text="ROTATE MODE    LEFT: Rotate SUBhelix RIGHT: rotate strand";
            break;
        }
        case AWT_MODE_MOD: {
            text="CONSTRAINT MODE    LEFT: modify constraint";
            sec_root->show_constraints = 1;
            break;
        }
        case AWT_MODE_LINE: {
            text="SET CURSOR MODE    LEFT: Set Cursor in ARB_EDIT4";
            break;
        }
        default: {
#if defined(DEBUG)
            sec_assert(0);
#endif // DEBUG
            break;
        }
    }
    aws->get_root()->awar("tmp/LeftFooter")->write_string( text);
    ntw->set_mode(mode);
    ntw->refresh();
}

void SEC_undo_cb(AW_window *aw, AWT_canvas *ntw, AW_CL undo_type)
{
    AWUSE(aw);
    GB_ERROR error = GB_undo(gb_main,(GB_UNDO_TYPE)undo_type);
    if (error) {
        aw_message(error);
    }
    else{
        GB_begin_transaction(gb_main);
        GB_commit_transaction(gb_main);
        ntw->refresh();
    }
}

#define MAXLINESIZE 	500
#define ASS   		"ARB secondary structure v1"
#define ASS_START 	"["ASS"]\n"
#define ASS_EOS 	"[end of structure]\n"
#define ASS_EOF		"[end of "ASS"]\n"

#if 0
// static GB_ERROR read_helix_info(char **helix, int *helix_length,  char **helix_nr, int *helix_nr_length)
// {
//     GB_transaction transAction(gb_main);
//     GB_ERROR err = 0;

//     char *helix_name = GBT_get_default_helix(gb_main);
//     char *helix_nr_name = GBT_get_default_helix_nr(gb_main);
//     char *alignment_name = GBT_get_default_alignment(gb_main);

//     GBDATA *gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
//     long size2 = GBT_get_alignment_len(gb_main,alignment_name);
//     if (size2<=0) err = (char *)GB_get_error();
//     if (!err) {
//         GBDATA *gb_helix_nr_con = GBT_find_SAI_rel_exdata(gb_extended_data, helix_nr_name);
//         GBDATA *gb_helix_con = GBT_find_SAI_rel_exdata(gb_extended_data, helix_name);
//         GBDATA *gb_helix = 0;
//         GBDATA *gb_helix_nr = 0;

//         if (gb_helix_nr_con) 	gb_helix_nr = GBT_read_sequence(gb_helix_nr_con,alignment_name);
//         if (gb_helix_con) 	gb_helix = GBT_read_sequence(gb_helix_con,alignment_name);

//         *helix = 0;
//         *helix_nr = 0;

//         if (!gb_helix) {
//             err =  "Cannot find the helix";
//         }
//         else if (gb_helix_nr) {
//             *helix_nr = GB_read_string(gb_helix_nr);
//             *helix_nr_length = GB_read_string_count(gb_helix_nr);
//         }

//         if (!err) {
//             *helix = GB_read_string(gb_helix);
//             *helix_length = GB_read_string_count(gb_helix);
//         }
//     }

//     free(alignment_name);
//     free(helix_nr_name);
//     free(helix_name);

//     return err;
// }
#endif

static char *encode_xstring_rel_helix(GB_CSTR x_string, int xlength, BI_helix *helix, int *no_of_helices_ptr)
{
    int allocated = 100;
    int no_of_helices = 0;
    char *rel_helix = (char*)malloc(sizeof(*rel_helix)*allocated);
    int pos;
    int start_helix = -1;
    char *start_helix_nr = 0;
    int end_helix = -1;

    for (pos=0; ; pos++) {
        BI_helix::BI_helix_entry *entry = &(helix->entries[pos]);
        //char *helix_nr = 0;

        if (entry->pair_type!=HELIX_NONE) {
            char *helix_nr = entry->helix_nr;

            if (helix_nr==start_helix_nr) { // same helix_nr as last
                end_helix = pos;
            }
            else { // new helix_nr
                if (start_helix_nr) { // not first helix -> store state of last helix
                insert_helix:
                    helix_nr = entry->helix_nr; // re-init (needed in case of goto insert_helix)
                    int is_used_in_secondary_structure = !!(x_string[start_helix]=='x' && x_string[end_helix+1]=='x');

#if defined(DEBUG) && 0
                    printf("Helix #%s von %i bis %i (used=%i)\n", start_helix_nr, start_helix, end_helix, is_used_in_secondary_structure);
#endif

                    if (no_of_helices==allocated) {
                        allocated += 100;
                        rel_helix = (char*)realloc(rel_helix, sizeof(*rel_helix)*allocated);
                    }
                    rel_helix[no_of_helices++] = '0'+is_used_in_secondary_structure;
                    rel_helix[no_of_helices] = 0;
                    if (pos==(xlength-1)) {
                        break;
                    }
                }
                start_helix = pos;
                end_helix = pos;
                start_helix_nr = helix_nr;
            }
        }

        if (pos==(xlength-1)) { // last position?
            if (start_helix_nr) {
                goto insert_helix;
            }
        }
    }

    *no_of_helices_ptr = no_of_helices;
    return rel_helix;
}

static void decode_xstring_rel_helix(GB_CSTR rel_helix, char *x_buffer, int xlength, BI_helix *helix, int *no_of_helices_ptr)
{
    int no_of_helices = 0;
    int pos;
    int start_helix = 0;
    int end_helix = -1;
    char *start_helix_nr = 0;
    int rel_pos = 0;

    for (pos=0; ; pos++) {
        BI_helix::BI_helix_entry *entry = &(helix->entries[pos]);
        //char *helix_nr;

        if (entry->pair_type!=HELIX_NONE) {
            char *helix_nr = entry->helix_nr;

            if (helix_nr==start_helix_nr) { // same helix as last
                end_helix = pos;
            }
            else { // new helix nr
                if (start_helix_nr) { // not first helix -> write last to xstring
                insert_helix:
                    helix_nr = entry->helix_nr; // re-init (needed in case of goto insert_helix)
                    char flag = rel_helix[rel_pos++];

                    no_of_helices++;
                    if (flag=='1') {
#if defined(DEBUG)
                        sec_assert(end_helix!=-1);
#endif // DEBUG
                        x_buffer[start_helix] = 'x';
                        x_buffer[end_helix+1] = 'x';
                    }
                    else if (flag!='0') {
                        if (flag==0) break; // eos
#if defined(DEBUG)
                        sec_assert(0); // illegal character
#endif // DEBUG
                        break;
                    }
                    if (pos==(xlength-1)) {
                        break;
                    }
                }
                start_helix = pos;
                end_helix = pos;
                start_helix_nr = helix_nr;
            }
        }
        if (pos==(xlength-1)) {
            if (start_helix_nr) {
                goto insert_helix;
            }
        }
    }

    *no_of_helices_ptr = no_of_helices;
}

static void export_structure_to_file(AW_window *, AW_CL /*cl_ntw*/)
{
    //    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
    //    SEC_graphic *sec_graphic = (SEC_graphic*)ntw->tree_disp;
    AW_root *aw_root = SEC_GRAPHIC->aw_root;
    char *filename = aw_root->awar(AWAR_SECEDIT_IMEXPORT_BASE"/file_name")->read_string();
    FILE *out = fopen(filename, "wt");
    GB_ERROR error = 0;

    if (out) {
        SEC_root *sec_root = SEC_GRAPHIC->sec_root;

        fputs(ASS_START, out);

        char *strct = sec_root->write_data();
        fputs(strct, out);
        free(strct);

        fputs(ASS_EOS, out);

        char *x_string = sec_root->x_string;
        int no_of_helices;
        int xlength = strlen(x_string);
        char *xstring_rel_helix = encode_xstring_rel_helix(x_string, xlength, sec_root->helix, &no_of_helices);

        fprintf(out, "no of helices=%i\nlength of xstring=%i\nxstring_rel_helix=%s\n", no_of_helices, xlength, xstring_rel_helix);
        free(xstring_rel_helix);

        fputs(ASS_EOF, out);
        fclose(out);
    }
    else {
        error = GB_export_error("Can't write secondary structure to '%s'", filename);
    }

    if (error) {
        aw_message(error, "Not ok");
    }
}

static const char *scan_token(char *line, const char *token)
{
    char *gleich = strchr(line, '=');

    if (gleich) {
        gleich[0] = 0;
        if (strcmp(line, token)==0) {
            return gleich+1;
        }
    }

    return 0;
}

static void import_structure_from_file(AW_window *, AW_CL cl_ntw)
{
    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
    //    SEC_graphic *sec_graphic = (SEC_graphic*)ntw->tree_disp;
    AW_root *aw_root = SEC_GRAPHIC->aw_root;
    char *filename = aw_root->awar(AWAR_SECEDIT_IMEXPORT_BASE"/file_name")->read_string();
    FILE *in = fopen(filename, "rt");
    GB_ERROR error = 0;

    if (!in) {
        error = GB_export_error("Can't open file '%s'", filename);
    }

    if (!error) {
        int salloc = 1000;
        char *strct = new char[salloc];
        int slen = 0;
        char *x_string = 0;
        char line[MAXLINESIZE];
        SEC_root *sec_root = SEC_GRAPHIC->sec_root;

        fgets(line, MAXLINESIZE, in);
        int llen = strlen(line);
        if (llen>=MAXLINESIZE) {
        long_line:
            error = GB_export_error("File '%s' contains line longer than %i chars -- import aborted", filename, llen);
        }

        if (!error) {
            if (strcmp(line, ASS_START)!=0) {
            wrong_format:
                error = GB_export_error("File '%s' is not a ARB secondary structure file", filename);
            }
            else {
                while (1) {
                    fgets(line, MAXLINESIZE, in);
                    llen = strlen(line);
                    if (llen>=MAXLINESIZE) goto long_line;

                    if (strcmp(line, ASS_EOS)==0) { // end of structure
                        break;
                    }

                    if ((slen+llen+1)>salloc) {
                        salloc += 1000;
                        char *new_strct = new char[salloc];
                        memcpy(new_strct, strct, slen);
                        delete strct;
                        strct = new_strct;
                    }

                    strcpy(strct+slen, line);
                    slen += llen;
                }

                fgets(line, MAXLINESIZE, in);
                llen = strlen(line);
                if (llen>=MAXLINESIZE) goto long_line;
                const char *rest = scan_token(line, "no of helices");
                if (!rest) goto wrong_format;
                int no_of_helices = atoi(rest);

                fgets(line, MAXLINESIZE, in);
                llen = strlen(line);
                if (llen>=MAXLINESIZE) goto long_line;
                rest = scan_token(line, "length of xstring");
                if (!rest) goto wrong_format;
                int xlength = atoi(rest);

                fgets(line, MAXLINESIZE, in);
                llen = strlen(line);
                if (llen>=MAXLINESIZE) goto long_line;

                char *rel_helix = 0;
                {
                    const char *crel_helix = scan_token(line, "xstring_rel_helix");
                    if (!crel_helix) goto wrong_format;
                    rel_helix = strdup(crel_helix);
                    int rh_len = strlen(rel_helix);
                    rel_helix[rh_len-1] = 0; // delete \n at eol
                }

                x_string = new char[xlength+1];
                memset(x_string, '.', xlength);
                x_string[xlength] = 0;

#if defined(DEBUG) && 0
                printf("x_string='%s'\n", x_string);
#endif

                fgets(line, MAXLINESIZE, in);
                llen = strlen(line);
                if (llen>=MAXLINESIZE) goto long_line;
                if (strcmp(line, ASS_EOF)!=0) goto wrong_format;

                int current_no_of_helices;
                decode_xstring_rel_helix(rel_helix, x_string, xlength, sec_root->helix,&current_no_of_helices);
                free(rel_helix);
#if defined(DEBUG) && 0
                printf("x_string='%s'\n", x_string);
#endif
                if (current_no_of_helices!=no_of_helices) {
                    error = GB_export_error("You can't import a secondary structure if you changed the # of helices");
                }
            }
        }

        if (!error) {
            SEC_GRAPHIC->write_data_to_db(strct, x_string);
            ntw->refresh();
        }

        delete strct;
        delete x_string;
    }

    if (in) {
        fclose(in);
    }

    if (error) {
        aw_message(error, "Not ok");
    }
}


#undef MAXLINESIZE
#undef ASS
#undef ASS_START
#undef ASS_EOS
#undef ASS_EOF

static AW_window *SEC_importExport(AW_root *root, int export_to_file, AWT_canvas *ntw)
{
    AW_window_simple *aws = new AW_window_simple;

    if (export_to_file) aws->init(root, "export_secondary_structure", "Export secondary structure to ...", 100, 100);
    else 		aws->init(root, "import_secondary_structure", "Import secondary structure from ...", 100, 100);

    aws->load_xfig("sec_imexport.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback( AW_POPUP_HELP, (AW_CL)"sec_imexport.hlp");
    aws->create_button("HELP","HELP","H");

    awt_create_selection_box((AW_window *)aws, AWAR_SECEDIT_IMEXPORT_BASE);

    aws->at("cancel");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CANCEL","CANCEL","C");

    aws->at("save");
    if (export_to_file) {
        aws->callback(export_structure_to_file, (AW_CL)ntw);
        aws->create_button("EXPORT","EXPORT","E");
    }
    else {
        aws->callback(import_structure_from_file, (AW_CL)ntw);
        aws->create_button("IMPORT","IMPORT","I");
    }

    return aws;
}

static AW_window *SEC_import(AW_root *root, AW_CL cl_ntw) { return SEC_importExport(root, 0, (AWT_canvas*)cl_ntw); }
static AW_window *SEC_export(AW_root *root, AW_CL cl_ntw) { return SEC_importExport(root, 1, (AWT_canvas*)cl_ntw); }

static void SEC_new_structure(AW_window *, AW_CL cl_ntw, AW_CL) {
    GB_transaction dummy(gb_main);

    if (aw_message("This will delete your current secondary structure!", "Abort,Continue")) {
        AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
        //	SEC_graphic *sec_graphic = (SEC_graphic *) ntw->tree_disp;
        SEC_root *sec_root = SEC_GRAPHIC->sec_root;
        char *ali_name = GBT_get_default_alignment(gb_main);
        long ali_len = GBT_get_alignment_len(gb_main,ali_name);

        sec_root->create_default_bone(ali_len);
        SEC_GRAPHIC->save(0, 0, 0, 0);

        ntw->recalc_size();
        ntw->refresh();
    }
}

AW_window *SEC_create_layout_window(AW_root *awr) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(awr, "SEC_LAYOUT", "SECEDIT Layout", 100, 100);
    aws->load_xfig("sec_layout.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"sec_layout.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->at("strand_dist");
    aws->label("Distance between strands:");
    aws->create_input_field(AWAR_SECEDIT_DIST_BETW_STRANDS);

    int x, dummy;
    aws->at("chars");
    aws->get_at_position(&x, &dummy);

#define PAIR_FIELDS(lower, upper)				\
    aws->at(lower "_pair"); 					\
    aws->create_input_field(AWAR_SECEDIT_##upper##_PAIRS, 30);	\
    aws->at_x(x);						\
    aws->create_input_field(AWAR_SECEDIT_##upper##_PAIR_CHAR, 1)

    PAIR_FIELDS("strong", STRONG);
    PAIR_FIELDS("normal", NORMAL);
    PAIR_FIELDS("weak", WEAK);
    PAIR_FIELDS("no", NO);
    PAIR_FIELDS("user", USER);

#undef PAIR_FIELDS

    return aws;
}

AW_window *SEC_create_main_window(AW_root *awr){
    GB_transaction tscope(gb_main);

    AW_window_menu_modes *awm = new AW_window_menu_modes();
    awm->init(awr,"ARB_SECEDIT", "ARB_SECEDIT", 800,600,10,10);

    SEC_create_awars(awr, AW_ROOT_DEFAULT);

    AW_gc_manager aw_gc_manager;
    SEC_GRAPHIC = new SEC_graphic(awr,gb_main);
    AWT_canvas *ntw = new AWT_canvas(gb_main,awm, SEC_GRAPHIC, aw_gc_manager,AWAR_SPECIES_NAME);

    SEC_add_awar_callbacks(awr, AW_ROOT_DEFAULT, ntw);

    SEC_GRAPHIC->sec_root->init_sequences(awr,ntw);
    GB_ERROR err = SEC_GRAPHIC->load(gb_main,0,0,0);
    if (err) return 0;

    ntw->recalc_size();
    ntw->set_mode(AWT_MODE_ZOOM); // Default-Mode

    awm->create_menu( 0, "File", "F", "secedit_file.hlp",  AWM_ALL );

    awm->insert_menu_topic("secedit_new", "New Structure", "N", 0, AWM_ALL, (AW_CB)SEC_new_structure, (AW_CL)ntw, 0);
    awm->insert_menu_topic("secedit_import", "Import Structure ...", "I", "secedit_imexport.hlp", AWM_ALL, AW_POPUP, (AW_CL)SEC_import, (AW_CL)ntw);
    awm->insert_menu_topic("secedit_export", "Export Structure ...", "E", "secedit_imexport.hlp", AWM_ALL, AW_POPUP, (AW_CL)SEC_export, (AW_CL)ntw);

    awm->insert_menu_topic("print_secedit", "Print Structure ...", "P","secedit2prt.hlp",	AWM_ALL,	(AW_CB)AWT_create_print_window, (AW_CL)ntw, 0 );
#if defined(FREESTANDING)
    awm->insert_menu_topic( "quit", "Quit",  "Q","quit.hlp", AWM_ALL, (AW_CB)SEC_quit_cb, 0,0);
#else
    awm->insert_menu_topic( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);
#endif

    awm->create_menu("props","Properties","r","properties.hlp", AWM_ALL);
    awm->insert_menu_topic("props_menu",	"Menu: Colors and Fonts ...",	"M","props_frame.hlp",	AWM_ALL, AW_POPUP, (AW_CL)AW_preset_window, 0 );
    awm->insert_menu_topic("props_secedit",	"SECEDIT: Colors and Fonts ...","C","secedit_props_data.hlp",AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager );
    awm->insert_menu_topic("sec_layout", "Layout", "L", "sec_layout.hlp", AWM_ALL, AW_POPUP, (AW_CL)SEC_create_layout_window, 0);
    awm->insert_menu_topic("save_props",	"Save Defaults (in ~/.arb_prop/edit4)",	"D","savedef.hlp",	AWM_ALL, (AW_CB) AW_save_defaults, 0, 0 );

    awm->create_mode( 0, "zoom.bitmap", "sec_mode.hlp", AWM_ALL, (AW_CB)sec_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_ZOOM);
    awm->create_mode( 0, "lzoom.bitmap", "sec_mode.hlp", AWM_ALL, (AW_CB)sec_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_LZOOM); // logical ZOOM
    awm->create_mode( 0, "sec_modify.bitmap", "sec_mode.hlp", AWM_ALL, (AW_CB)sec_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_MOVE);
    awm->create_mode( 0, "setroot.bitmap", "sec_mode.hlp", AWM_ALL, (AW_CB)sec_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_SETROOT);
    awm->create_mode( 0, "rot.bitmap", "sec_mode.hlp", AWM_ALL, (AW_CB)sec_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_ROT);
    awm->create_mode( 0, "info.bitmap", "sec_mode.hlp", AWM_ALL, (AW_CB)sec_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_MOD);
    awm->create_mode( 0, "sec_setcurs.bitmap", "sec_mode.hlp", AWM_ALL, (AW_CB)sec_mode_event,(AW_CL)ntw,(AW_CL)AWT_MODE_LINE);

    awm->set_info_area_height( 250 );
    awm->at(5,2);
    awm->auto_space(5,-2);

    int db_pathx,db_pathy;
    awm->get_at_position( &db_pathx,&db_pathy );

    awm->button_length(0);
    awm->help_text("quit.hlp");
#if defined(FREESTANDING)
    awm->callback( SEC_quit_cb,0,0);
    awm->create_button("Quit", "Quit");
#else
    awm->callback((AW_CB0)AW_POPDOWN);
    awm->create_button("Close", "Close");
#endif

    awm->callback(AW_POPUP_HELP,(AW_CL)"arb_secedit.hlp");
    awm->button_length(0);
    awm->help_text("help.hlp");
    awm->create_button("HELP", "HELP","H");

    awm->callback( AW_help_entry_pressed );
    awm->create_button(0,"?");

    awm->callback((AW_CB)SEC_undo_cb,(AW_CL)ntw,(AW_CL)GB_UNDO_UNDO);
    awm->help_text("undo.hlp");
    awm->create_button("Undo", "Undo");

    awm->callback((AW_CB)SEC_undo_cb,(AW_CL)ntw,(AW_CL)GB_UNDO_REDO);
    awm->help_text("undo.hlp");
    awm->create_button("Redo", "Redo");

    awm->callback((AW_CB)SEC_center_cb,(AW_CL)ntw,(AW_CL)0);
    awm->help_text("center.hlp");
    awm->create_button("Center", "Center");

    awm->button_length(100);
    awm->at_newline();
    awm->create_button(0,"tmp/LeftFooter");
    awm->at_newline();
    awm->get_at_position( &db_pathx,&db_pathy );
    awm->set_info_area_height( db_pathy+6 );

    return awm;
}

#if defined(FREESTANDING)
int
main(int argc, char **argv){
    AWUSE(argc);
    AWUSE(argv);

    AW_root *aw_root;
    AW_default aw_default;
    aw_initstatus();
    aw_root = new AW_root;
    aw_default = aw_root->open_default(".arb_prop/secedit.arb");
    aw_root->init_variables(aw_default);
    aw_root->init("ARB_SECEDIT");
    gb_main = NULL;

    gb_main = GBT_open(":","rw",0);
    if (!gb_main) {
        aw_message(GB_get_error(),"OK");
        exit(0);
    }


    AW_window *aww = SEC_create_main_window(aw_root);
    aww->show();

    aw_root->main_loop();
    return 0;
}
#endif



SEC_BASE_TYPE SEC_helix_strand::getType() {
    return SEC_HELIX_STRAND;
}

SEC_BASE_TYPE SEC_segment::getType() {
    return SEC_SEGMENT;
}

SEC_BASE_TYPE SEC_loop::getType() {
    return SEC_LOOP;
}

