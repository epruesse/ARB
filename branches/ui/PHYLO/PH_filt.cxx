// =============================================================== //
//                                                                 //
//   File      : PH_filt.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "phylo.hxx"
#include "phwin.hxx"
#include "PH_display.hxx"
#include <arbdbt.h>
#include <aw_awar.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <cctype>
#include <awt_config_manager.hxx>

static long PH_timer() {
    static long time = 0;
    return ++time;
}

PH_filter::PH_filter()
{
    memset ((char *)this, 0, sizeof(PH_filter));
}

char *PH_filter::init(long size) {
    delete [] filter;
    filter = new char[size];
    real_len = filter_len = size;
    for (int i = 0; i < size; i++) {
        filter[i] = 1;
    }
    update = PH_timer();
    return 0;
}

PH_filter::~PH_filter() {
    delete [] filter;
}

inline void strlwr(char *s) {
    for (int i = 0; s[i]; ++i) {
        s[i] = tolower(s[i]);
    }
}

float *PH_filter::calculate_column_homology() {
    long           i, j, max, num_all_chars;
    bool           mask[256];
    char           delete_when_max[100], get_maximum_from[100], all_chars[100], max_char;
    long           reference_table[256], **chars_counted;
    char           real_chars[100], low_chars[100];
    char           rest_chars[40], upper_rest_chars[20], low_rest_chars[20];
    unsigned char *sequence_buffer;
    AW_root       *aw_root;
    float         *mline = 0;

    if (!PHDATA::ROOT) return 0;                    // nothing loaded yet

    arb_progress filt_progress("Calculating filter");

    GB_transaction ta(PHDATA::ROOT->get_gb_main());

    bool isNUC = true; // rna oder dna sequence : nur zum testen und Entwicklung
    if (GBT_is_alignment_protein(PHDATA::ROOT->get_gb_main(), PHDATA::ROOT->use)) {
        isNUC = false;
    }

    aw_root=PH_used_windows::windowList->phylo_main_window->get_root();

    if (isNUC) {
        strcpy(real_chars, "ACGTU");
        strcpy(upper_rest_chars, "MRWSYKVHDBXN");
    }
    else {
        strcpy(real_chars, "ABCDEFGHIKLMNPQRSTVWYZ");
        strcpy(upper_rest_chars, "X");
    }


    strcpy(low_chars, real_chars); strlwr(low_chars); // build lower chars
    { // create low_rest_chars and rest_chars
        strcpy(low_rest_chars, upper_rest_chars);
        strlwr(low_rest_chars);
        strcpy(rest_chars, upper_rest_chars);
        strcat(rest_chars, low_rest_chars);
    }

    strcpy(all_chars, real_chars);
    strcat(all_chars, low_chars);
    strcat(all_chars, rest_chars);

    strcpy(get_maximum_from, real_chars); // get maximum from markerline from these characters
    strcat(all_chars, ".-");

    num_all_chars = strlen(all_chars);

    // initialize variables
    free(mline);
    delete options_vector;
    mline = (float *) calloc((int) PHDATA::ROOT->get_seq_len(), sizeof(float));

    options_vector = (long *) calloc(8, sizeof(long));

    options_vector[OPT_START_COL]    = aw_root->awar(AWAR_PHYLO_FILTER_STARTCOL)->read_int();
    options_vector[OPT_STOP_COL]     = aw_root->awar(AWAR_PHYLO_FILTER_STOPCOL)->read_int();
    options_vector[OPT_MIN_HOM]      = aw_root->awar(AWAR_PHYLO_FILTER_MINHOM)->read_int();
    options_vector[OPT_MAX_HOM]      = aw_root->awar(AWAR_PHYLO_FILTER_MAXHOM)->read_int();
    options_vector[OPT_FILTER_POINT] = aw_root->awar(AWAR_PHYLO_FILTER_POINT)->read_int(); // '.' in column
    options_vector[OPT_FILTER_MINUS] = aw_root->awar(AWAR_PHYLO_FILTER_MINUS)->read_int(); // '-' in column
    options_vector[OPT_FILTER_AMBIG] = aw_root->awar(AWAR_PHYLO_FILTER_REST)->read_int(); // 'MNY....' in column
    options_vector[OPT_FILTER_LOWER] = aw_root->awar(AWAR_PHYLO_FILTER_LOWER)->read_int(); // 'acgtu' in column

    delete_when_max[0] = '\0';

    long startcol = options_vector[OPT_START_COL];
    long stopcol  = options_vector[OPT_STOP_COL];
    long len      = stopcol - startcol;

    // chars_counted[column][index] counts the occurrences of single characters per column
    // index = num_all_chars   -> count chars which act as column stopper ( = forget whole column if char occurs)
    // index = num_all_chars+1 -> count masked characters ( = don't count)

    chars_counted=(long **) calloc((int) (len), sizeof(long *));
    for (i=0; i<len; i++) {
        chars_counted[i]=(long *) calloc((int) num_all_chars+2, sizeof(long));
        for (j=0; j<num_all_chars+2; j++) chars_counted[i][j]=0;
    }

    for (i=0; i<PHDATA::ROOT->get_seq_len(); i++) mline[i]=-1.0; // all columns invalid
    for (i=0; i<256; i++) {
        mask[i]            = false;
        reference_table[i] = num_all_chars;         // invalid and synonym characters
    }

    // set valid characters
    for (i=0; i<num_all_chars; i++) {
        mask[(unsigned char)all_chars[i]]            = true;
        reference_table[(unsigned char)all_chars[i]] = i;
    }

    // rna or dna sequence: set synonyms
    if (isNUC) {
        reference_table[(unsigned char)'U'] = reference_table[(unsigned char)'T']; // T=U
        reference_table[(unsigned char)'u'] = reference_table[(unsigned char)'t'];
        reference_table[(unsigned char)'N'] = reference_table[(unsigned char)'X'];
        reference_table[(unsigned char)'n'] = reference_table[(unsigned char)'x'];
    }

    // set mappings according to options
    // be careful the elements of rest and low are mapped to 'X' and 'a'
    switch (options_vector[OPT_FILTER_POINT]) {     // '.' in column
        case DONT_COUNT:
            mask[(unsigned char)'.'] = false;
            break;

        case SKIP_COLUMN_IF_MAX:
            strcat(delete_when_max, ".");
            strcat(get_maximum_from, ".");
            break;

        case SKIP_COLUMN_IF_OCCUR:
            reference_table[(unsigned char)'.']=num_all_chars;    // map to invalid position
            break;

        case COUNT_DONT_USE_MAX: // use like another valid base/acid while not maximal
            // do nothing: don't get maximum of this charcater
            // but use character ( true in mask )
            break;

        default: ph_assert(0); break;  // illegal value!
    }

    switch (options_vector[OPT_FILTER_MINUS]) {     // '-' in column
        case DONT_COUNT:
            mask[(unsigned char)'-'] = false;
            break;

        case SKIP_COLUMN_IF_MAX:
            strcat(delete_when_max, "-");
            strcat(get_maximum_from, "-");
            break;

        case SKIP_COLUMN_IF_OCCUR:
            reference_table[(unsigned char)'-']=num_all_chars;
            break;

        case COUNT_DONT_USE_MAX: // use like another valid base/acid while not maximal
            // do nothing: don't get maximum of this charcater
            // but use character ( true in mask )
            break;

        default: ph_assert(0); break;  // illegal value!
    }
    // 'MNY....' in column
    bool mapRestToX = false;
    switch (options_vector[OPT_FILTER_AMBIG]) // all rest characters counted to 'X' (see below)
    {
        case DONT_COUNT:
            for (i=0; rest_chars[i]; i++) mask[(unsigned char)rest_chars[i]] = false;
            break;

        case SKIP_COLUMN_IF_MAX:
            strcat(delete_when_max, "X");
            strcat(get_maximum_from, "X");
            mapRestToX = true;
            break;

        case SKIP_COLUMN_IF_OCCUR:
            reference_table[(unsigned char)'X'] = num_all_chars;
            mapRestToX = true;
            break;

        case COUNT_DONT_USE_MAX: // use like another valid base/acid while not maximal
            // do nothing: don't get maximum of this charcater
            // but use character ( true in mask )
            break;

        case TREAT_AS_REGULAR:
            strcat(get_maximum_from, upper_rest_chars); // add uppercase rest chars to maximas
            // lowercase rest chars are handled together with normal lowercase chars (see below)
            break;

        default: ph_assert(0); break;  // illegal value!
    }

    if (mapRestToX) {
        // map all rest_chars to 'X'
        for (i=0; rest_chars[i]; i++) {
            reference_table[(unsigned char)rest_chars[i]] = reference_table[(unsigned char)'X'];
        }
    }

    switch (options_vector[OPT_FILTER_LOWER]) { // 'acgtu' in column
        case DONT_COUNT:
            for (i=0; low_chars[i]; i++) mask[(unsigned char)low_chars[i]] = false;
            break;

        case SKIP_COLUMN_IF_MAX:
            // count all low_chars to 'a'
            for (i=0; low_chars[i]; i++) reference_table[(unsigned char)low_chars[i]] = reference_table[(unsigned char)'a'];
            strcat(delete_when_max, "a");
            strcat(get_maximum_from, "a");
            break;

        case SKIP_COLUMN_IF_OCCUR:
            for (i=0; low_chars[i]; i++) reference_table[(unsigned char)low_chars[i]] = num_all_chars;
            break;

        case COUNT_DONT_USE_MAX:  // use like another valid base/acid while not maximal
            // do nothing: don't get maximum of this charcater
            // but use character ( true in mask )
            break;

        case TREAT_AS_UPPERCASE: // use like corresponding uppercase characters
            for (i=0; low_chars[i]; i++)     reference_table[(unsigned char)low_chars[i]]      = reference_table[toupper(low_chars[i])];
            for (i=0; low_rest_chars[i]; i++) reference_table[(unsigned char)low_rest_chars[i]] = reference_table[toupper(low_rest_chars[i])];
            break;

        default: ph_assert(0); break;  // illegal value!
    }

    GB_ERROR error = NULL;
    if (PHDATA::ROOT->nentries) {
        arb_progress progress("Counting", PHDATA::ROOT->nentries);
        // counting routine
        for (i=0; i<long(PHDATA::ROOT->nentries) && !error; i++) {
            sequence_buffer = (unsigned char*)GB_read_char_pntr(PHDATA::ROOT->hash_elements[i]->gb_species_data_ptr);
            long send = stopcol;
            long slen = GB_read_string_count(PHDATA::ROOT->hash_elements[i]->gb_species_data_ptr);
            if (slen< send) send = slen;
            for (j=startcol; j<send; j++) {
                if (mask[sequence_buffer[j]]) {
                    chars_counted[j-startcol][reference_table[sequence_buffer[j]]]++;
                }
                else {
                    chars_counted[j-startcol][num_all_chars+1]++;
                }
            }
            progress.inc_and_check_user_abort(error);
        }
    }
    if (!error) {
        // calculate similarity
        arb_progress progress("Calculate similarity", len);
        for (i=0; i<len && !error; i++) {
            if (chars_counted[i][num_all_chars]==0) // else: forget whole column
            {
                max=0; max_char=' ';
                for (j=0; get_maximum_from[j]!='\0'; j++) {
                    if (max<chars_counted[i][reference_table[(unsigned char)get_maximum_from[j]]]) {
                        max_char = get_maximum_from[j];
                        max      = chars_counted[i][reference_table[(unsigned char)max_char]];
                    }
                }
                if ((max!=0) && !strchr(delete_when_max, max_char)) {
                    // delete SKIP_COLUMN_IF_MAX classes for counting
                    for (j=0; delete_when_max[j]!='\0'; j++) {
                        chars_counted[i][num_all_chars+1] += chars_counted[i][reference_table[(unsigned char)delete_when_max[j]]];
                        chars_counted[i][reference_table[(unsigned char)delete_when_max[j]]]=0;
                    }
                    mline[i+startcol] = (max/
                                         ((float) PHDATA::ROOT->nentries -
                                          (float) chars_counted[i][num_all_chars+1]))*100.0;
                    // (maximum in column / number of counted positions) * 100
                }
            }
            progress.inc_and_check_user_abort(error);
        }
    }

    for (i=0; i<len; i++) {
        free(chars_counted[i]);
    }
    free(chars_counted);

    if (!error) {
        char *filt=(char *)calloc((int) PHDATA::ROOT->get_seq_len()+1, sizeof(char));
        for (i=0; i<PHDATA::ROOT->get_seq_len(); i++)
            filt[i]=((options_vector[OPT_MIN_HOM]<=mline[i])&&(options_vector[OPT_MAX_HOM]>=mline[i])) ? '1' : '0';
        filt[i]='\0';
        aw_root->awar(AWAR_PHYLO_FILTER_FILTER)->write_string(filt);
        free(filt);

        return mline;
    }
    else {
        free(mline);
        return 0;
    }
}

static void display_status_and_expose_cb() {
    display_status_cb();
    expose_cb();
}

static void correct_startstop_cb(AW_root *aw_root, bool start_changed) {
    AW_awar *awar_startcol = aw_root->awar(AWAR_PHYLO_FILTER_STARTCOL);
    AW_awar *awar_stopcol  = aw_root->awar(AWAR_PHYLO_FILTER_STOPCOL);

    int startcol = awar_startcol->read_int();
    int stopcol  = awar_stopcol->read_int();

    if (startcol>stopcol) {
        if (start_changed) awar_stopcol ->write_int(startcol);
        else               awar_startcol->write_int(stopcol);
    }
}
static void correct_minmaxhom_cb(AW_root *aw_root, bool min_changed) {
    AW_awar *awar_minhom = aw_root->awar(AWAR_PHYLO_FILTER_MINHOM);
    AW_awar *awar_maxhom = aw_root->awar(AWAR_PHYLO_FILTER_MAXHOM);

    int minhom = awar_minhom->read_int();
    int maxhom = awar_maxhom->read_int();

    if (minhom>maxhom) {
        if (min_changed) awar_maxhom->write_int(minhom);
        else             awar_minhom->write_int(maxhom);
    }
}

void PH_create_filter_variables(AW_root *aw_root, AW_default default_file, GBDATA *gb_main) {
    // filter awars
    long alilength;
    {
        GB_transaction ta(gb_main);
        char *aliname = GBT_get_default_alignment(gb_main);
        alilength     = GBT_get_alignment_len(gb_main, aliname);
        free(aliname);
    }

    RootCallback display_status_and_expose = makeRootCallback(display_status_and_expose_cb);

    aw_root->awar_int(AWAR_PHYLO_FILTER_STARTCOL, 0,           default_file)->set_minmax(0, alilength-1)->add_callback(display_status_and_expose)->add_callback(makeRootCallback(correct_startstop_cb, true));
    aw_root->awar_int(AWAR_PHYLO_FILTER_STOPCOL,  alilength-1, default_file)->set_minmax(0, alilength-1)->add_callback(display_status_and_expose)->add_callback(makeRootCallback(correct_startstop_cb, false));
    aw_root->awar_int(AWAR_PHYLO_FILTER_MINHOM,   0,           default_file)->set_minmax(0, 100)        ->add_callback(display_status_and_expose)->add_callback(makeRootCallback(correct_minmaxhom_cb, true));
    aw_root->awar_int(AWAR_PHYLO_FILTER_MAXHOM,   100,         default_file)->set_minmax(0, 100)        ->add_callback(display_status_and_expose)->add_callback(makeRootCallback(correct_minmaxhom_cb, false));

    RootCallback display_status = makeRootCallback(display_status_cb);

    aw_root->awar_int(AWAR_PHYLO_FILTER_POINT, DONT_COUNT, default_file)->add_callback(display_status); // '.' in column
    aw_root->awar_int(AWAR_PHYLO_FILTER_MINUS, DONT_COUNT, default_file)->add_callback(display_status); // '-' in column
    aw_root->awar_int(AWAR_PHYLO_FILTER_REST,  DONT_COUNT, default_file)->add_callback(display_status); // 'MNY....' in column
    aw_root->awar_int(AWAR_PHYLO_FILTER_LOWER, DONT_COUNT, default_file)->add_callback(display_status); // 'acgtu' in column
}

static AWT_config_mapping_def phyl_filter_config_mapping[] = {
    { AWAR_PHYLO_FILTER_STARTCOL, "startcol" },
    { AWAR_PHYLO_FILTER_STOPCOL,  "stopcol" },
    { AWAR_PHYLO_FILTER_MINHOM,   "minhom" },
    { AWAR_PHYLO_FILTER_MAXHOM,   "maxhom" },
    { AWAR_PHYLO_FILTER_POINT,    "filtdot" },
    { AWAR_PHYLO_FILTER_MINUS,    "filtminus" },
    { AWAR_PHYLO_FILTER_REST,     "filtambig" },
    { AWAR_PHYLO_FILTER_LOWER,    "filtlower" },

    { 0, 0 }
};

AW_window *PH_create_filter_window(AW_root *aw_root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root, "PHYL_FILTER", "PHYL FILTER");
    aws->load_xfig("phylo/filter.fig");
    aws->button_length(10);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    const int SCALERWIDTH = 200;
    aws->auto_space(5, 5);

    aws->at("startcol");
    aws->create_input_field_with_scaler(AWAR_PHYLO_FILTER_STARTCOL, 6, SCALERWIDTH);

    aws->at("stopcol");
    aws->create_input_field_with_scaler(AWAR_PHYLO_FILTER_STOPCOL, 6, SCALERWIDTH);

    aws->at("minhom");
    aws->create_input_field_with_scaler(AWAR_PHYLO_FILTER_MINHOM, 3, SCALERWIDTH);

    aws->at("maxhom");
    aws->create_input_field_with_scaler(AWAR_PHYLO_FILTER_MAXHOM, 3, SCALERWIDTH);

    aws->label_length(20);

    aws->at("point_opts");
    aws->label("'.'");
    aws->create_option_menu(AWAR_PHYLO_FILTER_POINT, true);
    aws->insert_option(filter_text[DONT_COUNT],           "0", DONT_COUNT);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_MAX],   "0", SKIP_COLUMN_IF_MAX);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_OCCUR], "0", SKIP_COLUMN_IF_OCCUR);
    aws->insert_option(filter_text[COUNT_DONT_USE_MAX],   "0", COUNT_DONT_USE_MAX);
    aws->update_option_menu();

    aws->at("minus_opts");
    aws->label("'-'");
    aws->create_option_menu(AWAR_PHYLO_FILTER_MINUS, true);
    aws->insert_option(filter_text[DONT_COUNT],           "0", DONT_COUNT);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_MAX],   "0", SKIP_COLUMN_IF_MAX);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_OCCUR], "0", SKIP_COLUMN_IF_OCCUR);
    aws->insert_option(filter_text[COUNT_DONT_USE_MAX],   "0", COUNT_DONT_USE_MAX);
    aws->update_option_menu();

    aws->at("rest_opts");
    aws->label("ambiguity codes");
    aws->create_option_menu(AWAR_PHYLO_FILTER_REST, true);
    aws->insert_option(filter_text[DONT_COUNT],           "0", DONT_COUNT);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_MAX],   "0", SKIP_COLUMN_IF_MAX);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_OCCUR], "0", SKIP_COLUMN_IF_OCCUR);
    aws->insert_option(filter_text[COUNT_DONT_USE_MAX],   "0", COUNT_DONT_USE_MAX);
    aws->insert_option(filter_text[TREAT_AS_REGULAR],     "0", TREAT_AS_REGULAR);
    aws->update_option_menu();

    aws->at("lower_opts");
    aws->label("lowercase chars");
    aws->create_option_menu(AWAR_PHYLO_FILTER_LOWER, true);
    aws->insert_option(filter_text[DONT_COUNT],           "0", DONT_COUNT);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_MAX],   "0", SKIP_COLUMN_IF_MAX);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_OCCUR], "0", SKIP_COLUMN_IF_OCCUR);
    aws->insert_option(filter_text[COUNT_DONT_USE_MAX],   "0", COUNT_DONT_USE_MAX);
    aws->insert_option(filter_text[TREAT_AS_UPPERCASE],   "0", TREAT_AS_UPPERCASE);
    aws->update_option_menu();

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "phylfilter", phyl_filter_config_mapping);

    return aws;
}

