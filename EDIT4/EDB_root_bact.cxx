#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_window.hxx>

#include "ed4_class.hxx"

#define NAME_BUFFERSIZE 100

//*******************************************
//* Database functions.     beginning   *
//*******************************************

void EDB_root_bact::calc_no_of_all(char *string_to_scan, long *group, long *species)
{
    long i=0;

    *group = 0;
    *species = 0;

    if (!string_to_scan)
        return;

    while (string_to_scan[i]) {
        if (string_to_scan[i] == 1) {
            if (string_to_scan[i+1] == 'L' || string_to_scan[i+1] == 'S') {
                (*species)++;
                i++;
            }
            else if (string_to_scan[i+1] == 'F' || string_to_scan[i+1] == 'G') {
                (*group)++;
                i++;
            }
        }
        i++;
    }
}

ED4_returncode EDB_root_bact::fill_data(ED4_multi_species_manager  *multi_species_manager,
                                        ED4_sequence_info_terminal *ref_sequence_info_terminal,
                                        ED4_sequence_terminal      *ref_sequence_terminal,
                                        char                       *str,
                                        ED4_index                  *y,
                                        ED4_index                   actual_local_position,
                                        ED4_index                  *length_of_terminals,
                                        int                         group_depth,
                                        ED4_datamode                datamode)
{
    ED4_species_manager        *species_manager;
    ED4_multi_name_manager     *multi_name_manager;
    ED4_name_manager           *name_manager;
    ED4_species_name_terminal  *species_name_terminal;
    ED4_multi_sequence_manager *multi_sequence_manager;
    char                        namebuffer[NAME_BUFFERSIZE];
    int                         count_too   = 0;
    ED4_index                   name_coords = 0, seq_coords = 0, local_count_position = 0, terminal_height;
    GBDATA                     *gb_datamode = NULL;

    switch (datamode) {
        case ED4_D_EXTENDED: {
            gb_datamode = GBT_find_SAI(gb_main, str);
            break;
        }
        case ED4_D_SPECIES: {
            gb_datamode = GBT_find_species(gb_main, str);
            break;
        }
    }


    if (!gb_datamode) { // didn't find this species
        char dummy[150];
        all_found++;
        sprintf(dummy,"%ld. %s\n",all_found, str);
        GBS_strcat(not_found_message,dummy);
        return ED4_R_BREAK;
    }

    // check whether sequence has data in desired alignment
    bool has_alignment = 0 != GB_find(gb_datamode,ED4_ROOT->alignment_name,0,down_level);
    if (!has_alignment) {
        if (datamode == ED4_D_SPECIES) { // only warn about species w/o data (SAIs are skipped silently)
            char dummy[150];
            all_found++;
            sprintf(dummy,"%ld. %s (no data in alignment)\n",all_found, str);
            GBS_strcat(not_found_message,dummy);
        }
        return ED4_R_BREAK;
    }

    terminal_height = TERMINALHEIGHT;
    local_count_position = actual_local_position;

    sprintf( namebuffer, "Species_Manager.%ld.%d", ED4_counter, count_too);

    species_manager = new ED4_species_manager( namebuffer, 0, local_count_position, 0, 0, multi_species_manager );
    species_manager->set_properties( ED4_P_MOVABLE );
    if (datamode == ED4_D_EXTENDED) {
        species_manager->flag.is_SAI = 1;
        ED4_group_manager *group_man = species_manager->get_parent(ED4_L_GROUP)->to_group_manager();
        group_man->table().ignore_me(); // ignore SAI tables
    }
    species_manager->set_species_pointer(gb_datamode);
    multi_species_manager->children->append_member(species_manager);

    sprintf( namebuffer, "MultiName_Manager.%ld.%d", ED4_counter, count_too );
    multi_name_manager = new ED4_multi_name_manager( namebuffer, 0, 0, 0, 0, species_manager );
    species_manager->children->append_member( multi_name_manager );

    sprintf( namebuffer, "MultiSeq_Manager.%ld.%d", ED4_counter, count_too++ );
    multi_sequence_manager = new ED4_multi_sequence_manager( namebuffer, MAXSPECIESWIDTH-(group_depth*BRACKETWIDTH), 0, 0, 0, species_manager );
    species_manager->children->append_member( multi_sequence_manager );

    sprintf( namebuffer, "Name_Manager%ld.%d", ED4_counter, count_too ++ );                         //hier fehlt noch y
    name_manager = new ED4_name_manager( namebuffer, 0, 0*terminal_height, 0, 0, multi_name_manager );          //hier fehlt noch y
    name_manager->set_properties( ED4_P_MOVABLE );                          //only Speciesname should be movable !!!!
    multi_name_manager->children->append_member( name_manager );

    sprintf(namebuffer, "Species_Name_Term%ld.%d",ED4_counter, count_too ++);
    species_name_terminal = new ED4_species_name_terminal( namebuffer, 0, 0, MAXSPECIESWIDTH-(group_depth*BRACKETWIDTH), terminal_height, name_manager );
    species_name_terminal->set_properties( (ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE) );
    species_name_terminal->set_links( NULL, ref_sequence_terminal );
    species_name_terminal->set_species_pointer(GB_find( gb_datamode, "name", 0, down_level));
    name_manager->children->append_member( species_name_terminal );

    name_coords += terminal_height;

    search_sequence_data_rek(multi_sequence_manager, ref_sequence_info_terminal, ref_sequence_terminal, gb_datamode,
                             count_too, &seq_coords, &max_seq_terminal_length, ED4_A_DEFAULT);

    local_count_position += max(name_coords, seq_coords);
    name_coords = seq_coords = 0;
    species_read ++;

    if (!(multi_species_manager->flag.hidden)) {
        *length_of_terminals = local_count_position-actual_local_position;
        *y += *length_of_terminals; // needed for global coordinates of manager
    }

    return ED4_R_OK;
}


ED4_returncode EDB_root_bact::search_sequence_data_rek(ED4_multi_sequence_manager *multi_sequence_manager,
                                                       ED4_sequence_info_terminal *ref_sequence_info_terminal,
                                                       ED4_sequence_terminal      *ref_sequence_terminal,
                                                       GBDATA                     *gb_datamode,
                                                       int                         count_too,
                                                       ED4_index                  *seq_coords,
                                                       ED4_index                  *max_sequence_terminal_length,
                                                       ED4_alignment               alignment_flag)
{
    AW_device *device;
    int        j          = 0;
    int        pixel_length;
    GBDATA    *gb_ali_xxx = NULL;
    GBDATA    *gb_alignment;
    long       string_length;
    char      *key_string;
    char       namebuffer[NAME_BUFFERSIZE];

    device = ED4_ROOT->first_window->aww->get_device(AW_MIDDLE_AREA);

    if (alignment_flag == ED4_A_DEFAULT) {
        gb_ali_xxx = GB_find(gb_datamode,ED4_ROOT->alignment_name,0,down_level);
    }
    else if (alignment_flag == ED4_A_CONTAINER) {
        gb_ali_xxx = gb_datamode;
    }
    if (!gb_ali_xxx) return ED4_R_OK;

    j=0;
    for (gb_alignment = GB_find(gb_ali_xxx, 0, 0, down_level);
         gb_alignment;
         gb_alignment = GB_find(gb_alignment, 0, 0, this_level|search_next))
    {
        GB_TYPES type = GB_read_type(gb_alignment);

        if (type == GB_INTS || type == GB_FLOATS) {
            continue;
        }

        if ( type == GB_DB) { // we have to unpack container
            search_sequence_data_rek(multi_sequence_manager, ref_sequence_info_terminal, ref_sequence_terminal,
                                     gb_alignment, count_too, seq_coords, max_sequence_terminal_length, ED4_A_CONTAINER);
        }
        else { // otherwise we enter the data
            key_string = GB_read_key( gb_alignment );
            if (key_string[0] != '_') { // don't show sequences starting with an underscore
                sprintf( namebuffer, "Sequence_Manager.%ld.%d", ED4_counter, count_too++ );
                ED4_sequence_manager *seq_manager = new ED4_sequence_manager( namebuffer, 0, j*TERMINALHEIGHT, 0, 0, multi_sequence_manager );
                seq_manager->set_properties( ED4_P_MOVABLE );
                multi_sequence_manager->children->append_member( seq_manager );

                ED4_sequence_info_terminal *sequence_info_terminal =
                    new ED4_sequence_info_terminal(key_string, 0, 0, SEQUENCEINFOSIZE, TERMINALHEIGHT, seq_manager );
                sequence_info_terminal->set_properties( (ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE) );
                sequence_info_terminal->set_links( ref_sequence_info_terminal, ref_sequence_info_terminal );
                sequence_info_terminal->set_species_pointer(gb_alignment);
                seq_manager->children->append_member( sequence_info_terminal );

                ED4_text_terminal *text_terminal = 0;

                bool is_seq_data = (strcmp(key_string, "data") == 0); // not quite correct since several SAIs also use data
                bool is_aligned = is_seq_data
                    || (strcmp(key_string, "data2") == 0) // used by SAIs with two entries (e.g. first and second digit of 2-digit-numbers)
                    || (strcmp(key_string, "bits") == 0); // used by binary SAIs (e.g. MARKERLINE)

                if (is_aligned) {
                    ED4_sequence_terminal *seq_term;

                    sprintf(namebuffer, "Sequence_Term%ld.%d",ED4_counter, count_too++);
                    seq_term               = new ED4_sequence_terminal( namebuffer, SEQUENCEINFOSIZE, 0, 0, TERMINALHEIGHT, seq_manager );
                    seq_term->species_name = seq_term->get_name_of_species();

                    if (is_seq_data) seq_term->set_properties( ED4_P_CONSENSUS_RELEVANT );
                    seq_term->set_properties( ED4_P_ALIGNMENT_DATA );
                    
                    text_terminal = seq_term;
                }
                else {
                    sprintf(namebuffer, "PureText_Term%ld.%d",ED4_counter, count_too++);
                    text_terminal = new ED4_pure_text_terminal(namebuffer, SEQUENCEINFOSIZE, 0, 0, TERMINALHEIGHT, seq_manager);
                }

                text_terminal->set_properties( ED4_P_CURSOR_ALLOWED );
                text_terminal->set_links( ref_sequence_terminal, ref_sequence_terminal );
                seq_manager->children->append_member(text_terminal);
#if defined(DEBUG)
                // ensure only 1 terminal is consensus-relevant!
                if (is_seq_data) {
                    seq_manager->get_consensus_relevant_terminal(); // does an error otherwise!
                }
#endif // DEBUG
                text_terminal->set_species_pointer(gb_alignment);

                if (gb_alignment) {
                    string_length = GB_read_count( gb_alignment );
                }
                else {
                    string_length = 100;
                }

                pixel_length = device->get_string_size( ED4_G_SEQUENCES, NULL, string_length) + 100;

                *max_sequence_terminal_length = max(*max_sequence_terminal_length, pixel_length);
                text_terminal->extension.size[WIDTH] = pixel_length;

                if (MAXSEQUENCECHARACTERLENGTH < string_length) {
                    MAXSEQUENCECHARACTERLENGTH = string_length;
                    ref_sequence_terminal->extension.size[WIDTH] = pixel_length;
                }

                if (!ED4_ROOT->scroll_links.link_for_hor_slider) {
                    ED4_ROOT->scroll_links.link_for_hor_slider = text_terminal;
                }
                else if (*max_sequence_terminal_length > ED4_ROOT->scroll_links.link_for_hor_slider->extension.size[WIDTH]) {
                    ED4_ROOT->scroll_links.link_for_hor_slider = text_terminal;
                }

                *seq_coords += TERMINALHEIGHT;
                j++;
            }
        }
    }

    return ED4_R_OK;
}

char* EDB_root_bact::make_string()
{
    const char *sep_name     = "\1"; // Trennzeichen
    char*       configstring = new char[500];

    strcpy(configstring, sep_name);

    strcat(configstring, "FGruppe1lang");
    strcat(configstring, sep_name);
    strcat(configstring, "SHELIX_PAIRS");
    strcat(configstring, sep_name);
    strcat(configstring, "SHELIX_LINE");
    strcat(configstring, sep_name);
    strcat(configstring, "E");
    strcat(configstring, sep_name);
    strcat(configstring, "GFltDorot");  // Speciesmark L
    strcat(configstring, sep_name);
    strcat(configstring, "LCasElega");
    strcat(configstring, sep_name);
    strcat(configstring, "E");
    strcat(configstring, sep_name);
    strcat(configstring, "LBaeFrag3");
    strcat(configstring, sep_name);
    strcat(configstring, "LFlaFerr2");
    strcat(configstring, sep_name);
    strcat(configstring, "LCytLyti3");
    strcat(configstring, "\0");

    return configstring;
}

char* EDB_root_bact::make_top_bot_string()          // is only called when started manually
{
    //  char sep_name[2]={ 1, '\0'};            // Trennzeichen

    char* configstring;
    configstring = new char[400];
    sprintf(configstring, "%cFSAI's%cSHELIX_PAIRS%cSHELIX_LINE%cSALI_ERR%cSALI_CON%cSALI_INT%cSALI_BIND%cSantibiot%cSmodnuc%cSelong%cStRNA%cSALI_BOR%cSALI_PRE_I%cSALI_PRE%cSALI_INSERTS%cSinseuca2%cSregaps%cSallr5%cSbacr5%cSarcr5%cSeucr5%cSgplr5%cSinsEuca%cSprimer1%cSprimer2%cSbetar5%cSprimer3%cE",1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1);

    strcat(configstring, "\0");

    return configstring;
}


ED4_returncode  EDB_root_bact::fill_species(ED4_multi_species_manager  *multi_species_manager,
                                            ED4_sequence_info_terminal *ref_sequence_info_terminal,
                                            ED4_sequence_terminal      *ref_sequence_terminal,
                                            char                       *str,
                                            int                        *index,
                                            ED4_index                  *y,
                                            ED4_index                   actual_local_position,
                                            ED4_index                  *length_of_terminals, /*height of terminals is meant*/
                                            int                         group_depth)
{
#define SHIPSIZE 1024

    char         *ship;
    int           lauf                        = 0;
    ED4_index     local_count_position        = 0;
    ED4_index     height_of_created_terminals = 0;
    bool          word                        = 0;
    ED4_datamode  datamode                    = ED4_D_SPECIES;
    ED4_returncode retCode                    = ED4_R_OK;

    ship = (char*)GB_calloc(SHIPSIZE, sizeof(*ship));
    local_count_position = actual_local_position;

    do {
        if (word == 0) { // word is needed for jump over separator
            if (str[(*index)+1] == 'L') {
                datamode = ED4_D_SPECIES;
            }
            else if (str[(*index)+1] == 'S') {
                datamode = ED4_D_EXTENDED;
            }
            else {
                printf("Unknown or misplaced tag-id '%c' - Please contact your administrator\n", str[(*index)+1]);
                e4_assert(0);
                retCode = ED4_R_ERROR;
                break;
            }

            (*index) +=2;
        }

        if (str[*index] != 1) {
            ship[lauf++] = str[*index];
            word = 1;
            (*index)++;
        }

        if (str[*index] == 1 || str[*index] == '\0') {
            ship[lauf] = '\0'; // speciesname-generation is finished
            lauf = 0;

            status_total_count += status_add_count;
            if (aw_status(status_total_count) == 1) { // Kill has been Pressed
                aw_closestatus();
                while (ED4_ROOT->first_window) {
                    ED4_ROOT->first_window->delete_window(ED4_ROOT->first_window);
                }

                GB_commit_transaction( gb_main );
                GB_close(gb_main);
                delete ship;
                delete ED4_ROOT->main_manager;
                ::exit(0);
            }

            fill_data(multi_species_manager, ref_sequence_info_terminal, ref_sequence_terminal, ship /*species name*/,
                      y, local_count_position, &height_of_created_terminals, group_depth, datamode);

            ED4_counter++;
            local_count_position += height_of_created_terminals;
            *length_of_terminals += height_of_created_terminals;
            word = 0;
        }
    } while (!((str[(*index)] == 1) && (str[(*index)+1] =='G' || str[(*index)+1]=='E' || str[(*index)+1]=='F'))
             && (str[*index] != '\0'));

    free(ship);
    return retCode;
#undef SHIPSIZE
}

ED4_index EDB_root_bact::scan_string(ED4_multi_species_manager  *parent,
                                     ED4_sequence_info_terminal *ref_sequence_info_terminal,
                                     ED4_sequence_terminal      *ref_sequence_terminal,
                                     char                       *str,
                                     int                        *index,
                                     ED4_index                  *y)
{
    ED4_multi_species_manager *multi_species_manager = NULL;
    ED4_bracket_terminal      *bracket_terminal      = NULL;
    ED4_spacer_terminal       *group_spacer_terminal = NULL;
    char                       namebuffer[NAME_BUFFERSIZE];
    char                       groupname[GB_GROUP_NAME_MAX];
    ED4_index                  y_old                 = 0;
    ED4_index                  lauf                  = 0;
    ED4_index                  local_count_position  = 0;
    ED4_index                  length_of_terminals   = 0;
    static int                 group_depth           = 0;
    bool                       is_folded             = 0;

    if (!parent->parent->is_area_manager()) {       // add 25 if group is not child of; not the first time!
        local_count_position = TERMINALHEIGHT + SPACERHEIGHT;   // a folded group
    }

    while (str[(*index)] != '\0' && str[(*index)+1] != 'E') { // E =
        if (str[(*index)+1] == 'L' || str[(*index)+1] == 'S') {   // L = species, S = SAI
            length_of_terminals = 0;

            fill_species(parent, ref_sequence_info_terminal, ref_sequence_terminal, str, index,
                         y, local_count_position, &length_of_terminals, group_depth);

            local_count_position += length_of_terminals;
            ED4_counter ++;     // counter is only needed to generate
        }

        if (str[(*index)] && (str[(*index)+1] == 'G' || str[(*index)+1] == 'F')) { // Group or folded group
            group_depth++;
            is_folded = str[(*index)+1]=='F';

            for (*index += 2, lauf = 0; str[*index] != 1; (*index)++) {  // Jump over 'G' and Blank to get Groupname
                groupname[lauf++] = str[*index];
            }
            groupname[lauf] = '\0';

            create_group_header(parent, ref_sequence_info_terminal, ref_sequence_terminal, &multi_species_manager, &bracket_terminal,
                                y, groupname, group_depth, is_folded, local_count_position);

            y_old = local_count_position;
            ED4_counter++;

            local_count_position += scan_string(multi_species_manager, ref_sequence_info_terminal, ref_sequence_terminal, str, index, y);
            sprintf(namebuffer, "Group_Spacer_Terminal_End.%ld", ED4_counter);

            local_count_position += SPACERHEIGHT;

            bracket_terminal->extension.size[HEIGHT] = local_count_position - y_old;
            group_spacer_terminal = new ED4_spacer_terminal( namebuffer , 0, local_count_position - y_old + SPACERHEIGHT,
                                                             10, SPACERHEIGHT, multi_species_manager);
            bracket_terminal->set_links( NULL, multi_species_manager);

            (*y) += SPACERHEIGHT;

            multi_species_manager->children->append_member( group_spacer_terminal );

            if (is_folded) {
                multi_species_manager->hide_children();
                is_folded = 0;
            }
        }
    }

    if (str[(*index)] && str[(*index)+1] == 'E') {
        (*index)+=2;
        group_depth--;
    }

    return local_count_position;
}

ED4_returncode EDB_root_bact::create_group_header(ED4_multi_species_manager   *parent,
                                                  ED4_sequence_info_terminal  *ref_sequence_info_terminal,
                                                  ED4_sequence_terminal       *ref_sequence_terminal,
                                                  ED4_multi_species_manager  **multi_species_manager,
                                                  ED4_bracket_terminal       **bracket_terminal,
                                                  ED4_index                   *y,
                                                  char                        *groupname,
                                                  int                          group_depth,
                                                  bool                         is_folded,
                                                  ED4_index                    local_count_position)
{
    ED4_species_manager        *species_manager        = NULL;
    ED4_species_name_terminal  *species_name_terminal  = NULL;
    ED4_sequence_manager       *sequence_manager       = NULL;
    ED4_sequence_info_terminal *sequence_info_terminal = NULL;
    ED4_sequence_terminal      *sequence_terminal      = NULL;
    ED4_spacer_terminal        *group_spacer_terminal  = NULL;
    ED4_group_manager          *group_manager          = NULL;


    char namebuffer[NAME_BUFFERSIZE];
    int height_spacer;
    int height_terminal;
    int pixel_length;
    AW_device *device;


    height_terminal = TERMINALHEIGHT;
    height_spacer   = SPACERHEIGHT;

    sprintf(namebuffer, "Group_Manager.%ld", ED4_counter);                              //create new group manager
    group_manager = new ED4_group_manager(namebuffer, 0, local_count_position, 0, 0, parent);
    parent->children->append_member(group_manager);

    sprintf(namebuffer, "Bracket_Terminal.%ld", ED4_counter);
    *bracket_terminal = new ED4_bracket_terminal(namebuffer, 0, 0, BRACKETWIDTH, 0, group_manager);
    group_manager->children->append_member(*bracket_terminal);

    sprintf(namebuffer, "MultiSpecies_Manager.%ld", ED4_counter);                           //create new multi_species_manager
    *multi_species_manager = new ED4_multi_species_manager(namebuffer, BRACKETWIDTH, 0, 0, 0,group_manager);    //Objekt Gruppen name_terminal noch
    group_manager->children->append_member( *multi_species_manager );                   //auszeichnen

    if (is_folded)                                              //only set FOLDED-flag if group
    {                                                   //is folded
        group_manager->set_properties((ED4_properties) (ED4_P_IS_FOLDED | ED4_P_MOVABLE));
        (*multi_species_manager)->set_properties((ED4_properties) (ED4_P_IS_FOLDED | ED4_P_IS_HANDLE));
        (*bracket_terminal)->set_properties((ED4_properties) (ED4_P_IS_FOLDED | ED4_P_IS_HANDLE));
    }
    else
    {
        group_manager->set_properties((ED4_properties) ( ED4_P_MOVABLE));
        (*multi_species_manager)->set_properties((ED4_properties) (ED4_P_IS_HANDLE));
        (*bracket_terminal)->set_properties((ED4_properties) ( ED4_P_IS_HANDLE));
    }

    sprintf(namebuffer, "Group_Spacer_Terminal_Beg.%ld", ED4_counter);                      //Spacer at beginning of group
    group_spacer_terminal = new ED4_spacer_terminal( namebuffer , 0, 0, 10, height_spacer, *multi_species_manager);     //For better Overview
    (*multi_species_manager)->children->append_member( group_spacer_terminal );

    sprintf( namebuffer, "Consensus_Manager.%ld", ED4_counter );                            //Create competence terminal
    species_manager = new ED4_species_manager( namebuffer, 0, height_spacer, 0, 0, *multi_species_manager );
    species_manager->set_properties( ED4_P_MOVABLE );
    species_manager->flag.is_consensus = 1;
    (*multi_species_manager)->children->append_member( species_manager );

    // ************
    //  sprintf(namebuffer, "Sequence_Info_Term%d.%d",counter, count_too ++);
    // *************

    species_name_terminal = new ED4_species_name_terminal(  groupname,0, 0, MAXSPECIESWIDTH-(group_depth*BRACKETWIDTH) , height_terminal, species_manager );
    species_name_terminal->set_properties( (ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE) );    //only some terminals
    species_name_terminal->set_links( NULL, ref_sequence_terminal );
    species_manager->children->append_member( species_name_terminal );                          //properties

    sprintf( namebuffer, "Consensus_Seq_Manager.%ld", ED4_counter);
    sequence_manager = new ED4_sequence_manager( namebuffer, MAXSPECIESWIDTH-(group_depth*BRACKETWIDTH), 0, 0, 0, species_manager );
    sequence_manager->set_properties( ED4_P_MOVABLE );
    species_manager->children->append_member( sequence_manager );

    sequence_info_terminal = new ED4_sequence_info_terminal( "CONS", /*NULL,*/ 0, 0, SEQUENCEINFOSIZE, height_terminal, sequence_manager ); // Info fuer Gruppe
    sequence_info_terminal->set_links( ref_sequence_info_terminal, ref_sequence_info_terminal );
    sequence_info_terminal->set_properties( (ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE) );
    sequence_manager->children->append_member( sequence_info_terminal );

    device = ED4_ROOT->first_window->aww->get_device(AW_MIDDLE_AREA);
    pixel_length = device->get_string_size( ED4_G_SEQUENCES, CONSENSUS , 0);

    sequence_terminal = new ED4_consensus_sequence_terminal( CONSENSUS, SEQUENCEINFOSIZE, 0, pixel_length + 10, height_terminal, sequence_manager );
    sequence_terminal->set_properties( ED4_P_CURSOR_ALLOWED );
    sequence_terminal->set_links( ref_sequence_terminal, ref_sequence_terminal );
    sequence_manager->children->append_member( sequence_terminal );
    sequence_terminal->parent->resize_requested_by_child();

    species_read ++;
    (*y) += height_terminal + height_spacer;

    return ED4_R_OK;
}

char *EDB_root_bact::generate_config_string(char *confname)                 // and save it in database
{
    int i;
    long string_length;
    char *generated_string = NULL;
    int counter = 0;
    GBDATA *gb_area;
    ED4_device_manager *device_manager = ED4_ROOT->main_manager->search_spec_child_rek (ED4_L_DEVICE)->to_device_manager();

    for (i=0; i<device_manager->children->members(); i++) {
        if (device_manager->children->member(i)->is_area_manager()) {
            GB_begin_transaction( gb_main );
            device_manager->children->member(i)->generate_configuration_string( &generated_string );

            string_length = strlen(generated_string);
            if (generated_string[string_length - 1] == 1)
                generated_string[string_length - 1] = '\0';

            GBDATA *gb_configuration = GBT_create_configuration(gb_main,confname);

            if (counter == 0) {
                gb_area = GB_search(gb_configuration,"top_area",GB_STRING);
                counter ++;
            }
            else {
                gb_area = GB_search(gb_configuration,"middle_area",GB_STRING);
            }

            GB_ERROR error = 0;
            error = GB_write_string(gb_area, generated_string);
            if (error) aw_message(error);

            GB_commit_transaction( gb_main );

            //          printf("AREA:\n%s\n\n\n",generated_string);

            delete [] generated_string;
            generated_string = NULL;
        }
    }

    return generated_string;
}


EDB_root_bact::EDB_root_bact()
{
}

EDB_root_bact::~EDB_root_bact()
{
}

//*******************************************
//* Database functions.     end     *
//*******************************************

