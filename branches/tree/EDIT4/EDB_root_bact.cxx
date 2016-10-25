// =============================================================== //
//                                                                 //
//   File      : EDB_root_bact.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <ed4_extern.hxx>
#include "ed4_class.hxx"

#include <aw_msg.hxx>
#include <arb_progress.h>
#include <arbdbt.h>
#include <arb_strbuf.h>
#include <ad_config.h>

void EDB_root_bact::calc_no_of_all(const char *string_to_scan, long *group, long *species) {
    *group = 0;
    *species = 0;

    if (string_to_scan) {
        long i = 0;
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
}

ED4_returncode EDB_root_bact::fill_data(ED4_multi_species_manager *multi_species_manager,
                                        ED4_reference_terminals&   refterms,
                                        char                      *str,
                                        int                        group_depth,
                                        ED4_datamode               datamode)
{
    GBDATA *gb_item = NULL;
    switch (datamode) {
        case ED4_D_EXTENDED: gb_item = GBT_find_SAI(GLOBAL_gb_main, str); break;
        case ED4_D_SPECIES:  gb_item = GBT_find_species(GLOBAL_gb_main, str); break;
    }

    if (!gb_item) { // didn't find this species/SAI
        not_found_counter++;
        if (not_found_counter <= MAX_SHOWN_MISSING_SPECIES) {
            char dummy[150];
            sprintf(dummy, "%zu. %s\n", not_found_counter, str);
            e4_assert(not_found_message);
            GBS_strcat(not_found_message, dummy);
        }
        return ED4_R_BREAK;
    }

    // check whether sequence has data in desired alignment
    bool has_alignment = 0 != GB_entry(gb_item, ED4_ROOT->alignment_name);
    if (!has_alignment) {
        if (datamode == ED4_D_SPECIES) { // only warn about species w/o data (SAIs are skipped silently)
            not_found_counter++;
            if (not_found_counter <= MAX_SHOWN_MISSING_SPECIES) {
                char dummy[150];
                sprintf(dummy, "%zu. %s (no data in alignment)\n", not_found_counter, str);
                GBS_strcat(not_found_message, dummy);
            }
        }
        return ED4_R_BREAK;
    }

    ED4_species_type spec_type = (datamode == ED4_D_EXTENDED) ? ED4_SP_SAI : ED4_SP_SPECIES;


    {
        char namebuffer[NAME_BUFFERSIZE];
        int count_two = 0;

        sprintf(namebuffer, "Species_Manager.%ld.%d", ED4_counter, count_two);
        ED4_species_manager *species_manager = new ED4_species_manager(spec_type, namebuffer, 0, 0, multi_species_manager);

        species_manager->set_property(PROP_MOVABLE);
        if (spec_type == ED4_SP_SAI) {
            ED4_abstract_group_manager *group_man = species_manager->get_parent(ED4_level(LEV_GROUP|LEV_ROOTGROUP))->to_abstract_group_manager();
            group_man->table().ignore_me(); // ignore SAI tables (does not work - instead ignore SAIs when calculating consensus)
        }
        species_manager->set_species_pointer(gb_item);
        multi_species_manager->append_member(species_manager);

        sprintf(namebuffer, "MultiName_Manager.%ld.%d", ED4_counter, count_two);
        ED4_multi_name_manager *multi_name_manager = new ED4_multi_name_manager(namebuffer, 0, 0, species_manager);
        species_manager->append_member(multi_name_manager);

        sprintf(namebuffer, "MultiSeq_Manager.%ld.%d", ED4_counter, count_two++);
        ED4_multi_sequence_manager *multi_sequence_manager = new ED4_multi_sequence_manager(namebuffer, 0, 0, species_manager);
        species_manager->append_member(multi_sequence_manager);

        sprintf(namebuffer, "Name_Manager%ld.%d", ED4_counter, count_two++);
        ED4_name_manager *name_manager = new ED4_name_manager(namebuffer, 0, 0, multi_name_manager);
        name_manager->set_property(PROP_MOVABLE); // only Speciesname should be movable
        multi_name_manager->append_member(name_manager);

        {
            sprintf(namebuffer, "Species_Name_Term%ld.%d", ED4_counter, count_two++);
            ED4_species_name_terminal *species_name_terminal = new ED4_species_name_terminal(namebuffer, MAXNAME_WIDTH-(group_depth*BRACKET_WIDTH), TERMINAL_HEIGHT, name_manager);
            species_name_terminal->set_property((ED4_properties) (PROP_SELECTABLE | PROP_DRAGABLE | PROP_IS_HANDLE));
            species_name_terminal->set_links(NULL, refterms.sequence());
            species_name_terminal->set_species_pointer(GB_entry(gb_item, "name"));
            name_manager->append_member(species_name_terminal);
        }

        {
            sprintf(namebuffer, "Flag_Term%ld.%d", ED4_counter, count_two++);
            ED4_flag_terminal *flag_terminal = new ED4_flag_terminal(namebuffer, FLAG_WIDTH, TERMINAL_HEIGHT, name_manager);
            flag_terminal->set_links(NULL, refterms.sequence());
            name_manager->append_member(flag_terminal);
        }

        GBDATA *gb_ali_xxx = GB_entry(gb_item, ED4_ROOT->alignment_name);
        if (gb_ali_xxx) {
            search_sequence_data_rek(multi_sequence_manager, refterms, gb_ali_xxx, count_two, &max_seq_terminal_length, datamode == ED4_D_EXTENDED);
        }
    }

    return ED4_R_OK;
}

ED4_returncode EDB_root_bact::search_sequence_data_rek(ED4_multi_sequence_manager *multi_sequence_manager,
                                                       ED4_reference_terminals&    refterms,
                                                       GBDATA                     *gb_ali_xxx, // alignment-container (or any subcontainer of)
                                                       int                         count_too,
                                                       ED4_index                  *max_sequence_terminal_length,
                                                       bool                        isSAI)
{
    char       namebuffer[NAME_BUFFERSIZE];
    AW_device *device = ED4_ROOT->first_window->get_device();

    e4_assert(gb_ali_xxx);

    for (GBDATA *gb_ali_child = GB_child(gb_ali_xxx); gb_ali_child; gb_ali_child = GB_nextChild(gb_ali_child)) {
        GB_TYPES type = GB_read_type(gb_ali_child);

        if (type == GB_INTS || type == GB_FLOATS) {
            continue;
        }

        if (type == GB_DB) {  // we have to unpack container
            search_sequence_data_rek(multi_sequence_manager, refterms, gb_ali_child, count_too, max_sequence_terminal_length, isSAI);
        }
        else { // otherwise we enter the data
            char *key_string = GB_read_key(gb_ali_child);
            if (key_string[0] != '_') { // don't show sequences starting with an underscore
                sprintf(namebuffer, "Sequence_Manager.%ld.%d", ED4_counter, count_too++);
                ED4_sequence_manager *seq_manager = new ED4_sequence_manager(namebuffer, 0, 0, multi_sequence_manager);
                seq_manager->set_property(PROP_MOVABLE);
                multi_sequence_manager->append_member(seq_manager);

                {
                    ED4_sequence_info_terminal *sequence_info_terminal = new ED4_sequence_info_terminal(key_string, SEQUENCE_INFO_WIDTH, TERMINAL_HEIGHT, seq_manager);
                    sequence_info_terminal->set_property((ED4_properties) (PROP_SELECTABLE | PROP_DRAGABLE | PROP_IS_HANDLE));
                    sequence_info_terminal->set_both_links(refterms.sequence_info());
                    sequence_info_terminal->set_species_pointer(gb_ali_child);
                    seq_manager->append_member(sequence_info_terminal);
                }

                ED4_text_terminal *text_terminal = 0;

                bool is_data    = false;
                bool is_data2   = false;
                bool is_bits    = false;
                bool is_quality = false;

                if      (strcmp(key_string, "data")    == 0) is_data    = true; // SAI or species
                else if (strcmp(key_string, "data2")   == 0) is_data2   = true; // used by SAIs with two entries (e.g. first and second digit of 2-digit-numbers)
                else if (strcmp(key_string, "bits")    == 0) is_bits    = true; // used by binary SAIs (e.g. MARKERLINE)
                else if (strcmp(key_string, "quality") == 0) is_quality = true; // used by "quality" entry written by chimera check; see ../STAT/ST_quality.cxx@chimera_check_quality_string

                bool is_aligned = is_data || is_data2 || is_bits || is_quality;

                if (is_aligned) {
                    bool shall_display_secinfo = is_data;

                    if (isSAI) {
                        GBDATA *gb_sai        = GB_get_grandfather(gb_ali_child);
                        GBDATA *gb_disp_sec   = GB_searchOrCreate_int(gb_sai, "showsec", 0);
                        shall_display_secinfo = GB_read_int(gb_disp_sec);
                    }

                    sprintf(namebuffer, "Sequence_Term%ld.%d", ED4_counter, count_too++);
                    ED4_sequence_terminal *seq_term = new ED4_sequence_terminal(namebuffer, 0, TERMINAL_HEIGHT, seq_manager, shall_display_secinfo);
                    seq_term->species_name          = seq_term->get_name_of_species();

                    if (is_data) seq_term->set_property(PROP_CONSENSUS_RELEVANT);
                    seq_term->set_property(PROP_ALIGNMENT_DATA);

                    text_terminal = seq_term;
                }
                else {
                    sprintf(namebuffer, "PureText_Term%ld.%d", ED4_counter, count_too++);
                    text_terminal = new ED4_pure_text_terminal(namebuffer, 0, TERMINAL_HEIGHT, seq_manager);
                }

                text_terminal->set_property(PROP_CURSOR_ALLOWED);
                text_terminal->set_both_links(refterms.sequence());
                seq_manager->append_member(text_terminal);
#if defined(DEBUG)
                // ensure only 1 terminal is consensus-relevant!
                if (is_data) {
                    seq_manager->get_consensus_relevant_terminal(); // does an error otherwise!
                }
#endif // DEBUG
                text_terminal->set_species_pointer(gb_ali_child);

                long string_length;
                if (gb_ali_child) {
                    string_length = GB_read_count(gb_ali_child);
                }
                else {
                    string_length = 100;
                }

                int pixel_length = device->get_string_size(ED4_G_SEQUENCES, NULL, string_length) + 100; // @@@ "+ 100" looks like a hack

                *max_sequence_terminal_length = std::max(*max_sequence_terminal_length, long(pixel_length));
                text_terminal->extension.size[WIDTH] = pixel_length;

                if (MAXSEQUENCECHARACTERLENGTH < string_length) {
                    MAXSEQUENCECHARACTERLENGTH = string_length;
                    refterms.sequence()->extension.size[WIDTH] = pixel_length;
                }

                if (!ED4_ROOT->scroll_links.link_for_hor_slider) {
                    ED4_ROOT->scroll_links.link_for_hor_slider = text_terminal;
                }
                else if (*max_sequence_terminal_length > ED4_ROOT->scroll_links.link_for_hor_slider->extension.size[WIDTH]) {
                    ED4_ROOT->scroll_links.link_for_hor_slider = text_terminal;
                }
            }
            free(key_string);
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
    char* configstring;
    configstring = new char[400];
    sprintf(configstring, "%cFSAI's%cSHELIX_PAIRS%cSHELIX_LINE%cSALI_ERR%cSALI_CON%cSALI_INT%cSALI_BIND%cSantibiot%cSmodnuc%cSelong%cStRNA%cSALI_BOR%cSALI_PRE_I%cSALI_PRE%cSALI_INSERTS%cSinseuca2%cSregaps%cSallr5%cSbacr5%cSarcr5%cSeucr5%cSgplr5%cSinsEuca%cSprimer1%cSprimer2%cSbetar5%cSprimer3%cE", 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);

    strcat(configstring, "\0");

    return configstring;
}


ED4_returncode  EDB_root_bact::fill_species(ED4_multi_species_manager *multi_species_manager,
                                            ED4_reference_terminals&   refterms,
                                            const char                *str,
                                            int                       *index,
                                            int                        group_depth,
                                            arb_progress              *progress)
{
    const int MAXNAMELEN = 1024;

    bool         expect_separator = true;
    ED4_datamode datamode         = ED4_D_SPECIES;
    ED4_returncode retCode        = ED4_R_OK;

    char *name = ARB_calloc<char>(MAXNAMELEN);
    int   npos = 0;

    do {
        if (expect_separator) {
            if (str[(*index)+1] == 'L') {
                datamode = ED4_D_SPECIES;
            }
            else if (str[(*index)+1] == 'S') {
                datamode = ED4_D_EXTENDED;
            }
            else {
                const char *entry = str+*index+1;
                char        tag   = entry[0];
                const char *sep   = strchr(entry, 1);

                if (sep) {
                    int   len     = sep-entry+1;
                    char *content = ARB_strndup(entry+1, len);
                    char *message = GBS_global_string_copy("Unknown or misplaced tag-id '%c' (with content '%s'). Error in configuration-data!\nTrying to continue..", tag, content);

                    fprintf(stderr, "ARB_EDIT4: %s\n", message);
                    aw_message(message);

                    free(message);
                    free(content);
                    retCode = ED4_R_WARNING;

                    (*index) += sep-entry+1; // set index to next separator
                    continue;
                }
                else {
                    fprintf(stderr, "Error reading configuration: Unexpected end of data (at '%s')\n", str+*index);

                    e4_assert(0);
                    retCode = ED4_R_ERROR;
                    break;
                }
                e4_assert(0); // never reached!
            }

            (*index) += 2;
        }

        if (str[*index] != 1) {
            name[npos++] = str[*index];
            expect_separator = false;
            (*index)++;
        }

        if (str[*index] == 1 || str[*index] == '\0') {
            name[npos] = '\0'; // speciesname-generation finished
            npos = 0;

            if (progress) {
                progress->inc();
                if (progress->aborted()) ED4_exit();
            }

            fill_data(multi_species_manager, refterms, name, group_depth, datamode);

            ED4_counter++;
            expect_separator = true;
        }
    }
    while (!((str[(*index)] == 1) && (str[(*index)+1] == 'G' || str[(*index)+1]=='E' || str[(*index)+1]=='F')) && (str[*index] != '\0'));

    free(name);

    return retCode;
}

void EDB_root_bact::scan_string(ED4_multi_species_manager *parent,
                                ED4_reference_terminals&   refterms,
                                const char                *str,
                                int                       *index,
                                arb_progress&              progress)
{
    static int group_depth = 0;

    while (str[(*index)] != '\0' && str[(*index)+1] != 'E') { // E =
        if (str[(*index)+1] == 'L' || str[(*index)+1] == 'S') {   // L = species, S = SAI
            fill_species(parent, refterms, str, index, group_depth, &progress);
            ED4_counter++; // counter is only needed to generate ids
        }

        if (str[(*index)] && (str[(*index)+1] == 'G' || str[(*index)+1] == 'F')) { // Group or folded group
            group_depth++;
            bool is_folded = str[(*index)+1]=='F';

            char groupname[GB_GROUP_NAME_MAX];
            {
                ED4_index gpos = 0;
                for (*index += 2, gpos = 0; str[*index] != 1; (*index)++) {  // Jump over 'G' and Blank to get Groupname
                    groupname[gpos++] = str[*index];
                }
                groupname[gpos] = '\0';
            }

            ED4_multi_species_manager *multi_species_manager;
            ED4_build_group_manager_start(parent, groupname, group_depth, is_folded, refterms, multi_species_manager);

            ED4_counter++;
            scan_string(multi_species_manager, refterms, str, index, progress);

            ED4_build_group_manager_end(multi_species_manager);

            if (is_folded) multi_species_manager->hide_children();
        }
    }

    if (str[(*index)] && str[(*index)+1] == 'E') {
        (*index)+=2;
        group_depth--;
    }
}

void EDB_root_bact::save_current_config(char *confname) { // and save it in database
    GB_ERROR   error;
    GBT_config cfg(GLOBAL_gb_main, confname, error);
    error = NULL; // ignore not-found error

    int                 counter        = 0;
    ED4_device_manager *device_manager = ED4_ROOT->get_device_manager();

    for (int i=0; i<device_manager->members(); i++) {
        ED4_base *area = device_manager->member(i);
        if (area->is_area_manager()) {
            GBS_strstruct area_config(10000);
            area->generate_configuration_string(area_config);
            cfg.set_definition(counter++, area_config.release());
        }
    }

    // add/update comment
    {
        char *newComment = GBS_log_dated_action_to(cfg.get_comment(), "saved from ARB_EDIT4");
        cfg.set_comment(newComment);
        free(newComment);
    }

    error = cfg.save(GLOBAL_gb_main, confname, true);
    aw_message_if(error);
}

