// =============================================================== //
//                                                                 //
//   File      : ali_main.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ali_global.hxx"


#define HELIX_PAIRS   "helix_pairs"
#define HELIX_LINE    "helix_line"
#define ALI_CONSENSUS "ALI_CON"
#define ALI_ERROR     "ALI_ERR"
#define ALI_INTERVALS "ALI_INT"

static ALI_GLOBAL aligs;


void message(char *errortext);


static const char *ali_version =
    "\nALIGNER   V2.0  (Boris Reichel 5/95)\n";

static const char *ali_man_line[] = {
    "Parameter:",
    "-s<species>       aligne the sequence of <species>",
    "-f<species_1>,...,<species_n>[;<extension_1>,...,<extension_k>]  use specified family and family extension",
    "-P<pt_server>     use the PT_server <pt_server>",
    "[-D<db_server>]   use the DB_server <db_server>",
    "[-nx]             not exclusive mode (profile)",
    "[-ms]             mark species (profile)",
    "[-mf]             mark used family (profile)",
    "[-mfe]            mark used family extension (profile)",
    "[-mgf]            multi gap factor (0.1) (profile)",
    "[-if]             insert factor (2.0) (profile)",
    "[-mif]            multi insert factor (0.5) (profile)",
    "[-m]              mark all (-m  == -ms -mf -mfe) (profile)",
    "[-d<filename>]    use <filename> for default values",
    "[-msubX1,X2,...,X25] use X1,...,X25 for the substitute matrix: (profile)",
    "                     a   c   g   u   -",
    "                a   X1  X2  X3  X4  X5",
    "                c   X6  X7  X8  X9  X10",
    "                g   X11 X12 X13 X14 X15",
    "                u   X16 X17 X18 X19 X20",
    "                -   X21 X22 X23 X24 X25",
    "[-mbindX1,X2,...,X25] use X1,...,X25 for the binding matrix: (profile)",
    "                     a   c   g   u   -",
    "                a   X1  X2  X3  X4  X5",
    "                c   X6  X7  X8  X9  X10",
    "                g   X11 X12 X13 X14 X15",
    "                u   X16 X17 X18 X19 X20",
    "                -   X21 X22 X23 X24 X25",
    "[-maxf]           maximal number of family members (10) (profile)",
    "[-minf]           minimal number of family members (5) (profile)",
    "[-minw]           minimal weight for family members (0.7) (profile)",
    "[-maxew]          maximal weight for family extension members (0.2) (profile)",
    "unused [-cl]             cost threshold (low) (0.25) ALI_ERR = ','",
    "unused [-cm]             cost threshold (middle) (0.5) ALI_ERR = '-'",
    "unused [-ch]             cost threshold (high) (0.8) ALI_ERR = '='",
    "[-mm]             maximal number of maps for solution (prealigner) (1000)",
    "[-mma]            maximal number of maps for aligner (prealigner) (2)",
    "[-csub]           cost threshold for substitution (prealigner) (0.5)",
    "[-chel]           cost threshold for helix binding (prealigner) (2.0)",
    "[-ec]             error count (prealigner) (2)",
    "[-ib]             interval border (prealigner) (5)",
    "[-ic]             interval center (prealigner) (5)",
    0
};


static void print_man() {
    // Print a short parameter description
    int i;

    for (i = 0; ali_man_line[i] != 0; i++)
        fprintf(stderr, "%s\n", ali_man_line[i]);
}


void ali_fatal_error(const char *message, const char *func) {
    fprintf(stderr, "FATAL ERROR %s: %s\n", func, message);
    exit(-1);
}

void ali_error(const char *message, const char *func) {
    fprintf(stderr, "ERROR %s: %s\n", func, message);
    exit(-1);
}



static int get_species(char *species_string, unsigned int species_number, char *buffer) {
    // Get one species of a list
    while (species_number > 0 && *species_string != '\0') {
        while (*species_string != '\0' && *species_string != ',')
            species_string++;
        if (*species_string != '\0')
            species_string++;
        species_number--;
    }

    if (*species_string != '\0') {
        while (*species_string != '\0' && *species_string != ',')
            *buffer++ = *species_string++;
        *buffer = '\0';
    }
    else {
        return 0;
    }

    return 1;
}


static int check_base_invariance(char *seq1, char *seq2)
{
    while (*seq1 != '\0' && !ali_is_base(*seq1))
        seq1++;
    while (*seq2 != '\0' && !ali_is_base(*seq2))
        seq2++;
    while (*seq1 != '\0' && *seq2 != '\0') {
        if (*seq1 != *seq2)
            return 0;
        seq1++;
        seq2++;
        while (*seq1 != '\0' && !ali_is_base(*seq1))
            seq1++;
        while (*seq2 != '\0' && !ali_is_base(*seq2))
            seq2++;
    }

    if (*seq1 == *seq2)
        return 1;

    return 0;
}


static int convert_for_back_write(char *seq_new, char *seq_orig) {
    // Convert the working sequenz into the original bases

    while (*seq_new != '\0' && (ali_is_dot(*seq_new) || ali_is_gap(*seq_new)))
        seq_new++;
    while (*seq_orig != '\0' && (ali_is_dot(*seq_orig) || ali_is_gap(*seq_orig)))
        seq_orig++;

    while (*seq_new != '\0' && *seq_orig != '\0') {
        if (*seq_new != *seq_orig) {
            switch (*seq_new) {
                case 'a':
                    switch (*seq_orig) {
                        case 'A':
                            *seq_new = 'A';
                            break;
                        default:
                            ali_error("Unexpected character in original sequence");
                    }
                    break;
                case 'c':
                    switch (*seq_orig) {
                        case 'C':
                            *seq_new = 'C';
                            break;
                        default:
                            ali_error("Unexpected character in original sequence");
                    }
                    break;
                case 'g':
                    switch (*seq_orig) {
                        case 'G':
                            *seq_new = 'G';
                            break;
                        default:
                            ali_error("Unexpected character in original sequence");
                    }
                    break;
                case 'u':
                    switch (*seq_orig) {
                        case 'U':
                            *seq_new = 'U';
                            break;
                        case 't':
                            *seq_new = 't';
                            break;
                        case 'T':
                            *seq_new = 'T';
                            break;
                    }
                    break;
                case 'n':
                    *seq_new = *seq_orig;
                    break;
                default:
                    ali_fatal_error("Unexpected character in generated sequence");
            }
        }
        seq_new++;
        seq_orig++;
        while (*seq_new != '\0' && (ali_is_dot(*seq_new) || ali_is_gap(*seq_new)))
            seq_new++;
        while (*seq_orig != '\0' && (ali_is_dot(*seq_orig) || ali_is_gap(*seq_orig)))
            seq_orig++;
    }

    if (*seq_new == *seq_orig)
        return 1;

    return 0;
}



int ARB_main(int argc, char *argv[]) {
    int                            i;
    char                           message_buffer[100];
    char                           species_name[100], species_number;
    ALI_PREALIGNER                *align_prealigner;
    ali_prealigner_approx_element *approx_elem;
    ALI_SEQUENCE                  *sequence;


    ali_message(ali_version);

    aligs.init(&argc, (const char**)argv);

    if (!aligs.species_name || argc > 1) {
        printf("Unknowen : ");
        for (i = 1; i < argc; i++)
            printf("%s ", argv[i]);
        printf("\n\n");
        print_man();
        exit (-1);
    }

    // Main loop
    species_number = 0;
    while (get_species(aligs.species_name, species_number, species_name)) {
        species_number++;

        sprintf(message_buffer,
                "\nStarting alignment of sequence: %s", species_name);
        ali_message(message_buffer);

        // Get all information of the sequence
        aligs.arbdb.begin_transaction();
        ALI_SEQUENCE *align_sequence;
        align_sequence = aligs.arbdb.get_sequence(species_name,
                                                  aligs.mark_species_flag);
        if (align_sequence == 0) {
            ali_error("Can't read sequence from database");
            ali_message("Aborting alignment of sequence");
        }
        else {
            char *align_string;
            char *align_string_original;

            align_string = align_sequence->string();
            align_string_original = aligs.arbdb.get_sequence_string(species_name,
                                                                    aligs.mark_species_flag);
            aligs.arbdb.commit_transaction();

            if (align_sequence == 0)
                ali_warning("Can't read sequence from database");
            else {
                // make profile for sequence
                ALI_PROFILE *align_profile;
                align_profile = new ALI_PROFILE(align_sequence, &aligs.prof_context);

                // write information about the profile to the database
                aligs.arbdb.begin_transaction();
                char *String = align_profile->cheapest_sequence();
                aligs.arbdb.put_SAI("ALI_CON", String);
                freeset(String, align_profile->borders_sequence());
                free(String);
                aligs.arbdb.commit_transaction();

                // make prealignment
                align_prealigner = new ALI_PREALIGNER(&aligs.preali_context,
                                                      align_profile,
                                                      0, align_profile->sequence_length() - 1,
                                                      0, align_profile->length() - 1);
                ALI_SEQUENCE *align_pre_sequence_i, *align_pre_sequence;
                ALI_SUB_SOLUTION *align_pre_solution;
                ALI_TLIST<ali_prealigner_approx_element *> *align_pre_approx;
                align_pre_sequence_i = align_prealigner->sequence();
                align_pre_sequence = align_prealigner->sequence_without_inserts();
                align_pre_solution = align_prealigner->solution();
                align_pre_approx = align_prealigner->approximation();
                delete align_prealigner;

                align_pre_solution->print();

                // write result of alignment into database
                aligs.arbdb.begin_transaction();
                String = align_pre_sequence_i->string();
                aligs.arbdb.put_SAI("ALI_PRE_I", String);
                freeset(String, align_pre_sequence->string());
                aligs.arbdb.put_SAI("ALI_PRE", String);
                free(String);
                aligs.arbdb.commit_transaction();


                sprintf(message_buffer, "%d solutions generated (taking the first)",
                        align_pre_approx->cardinality());
                ali_message(message_buffer);

                if (align_pre_approx->is_empty())
                    ali_fatal_error("List of approximations is empty");

                // Write result back to the database
                approx_elem = align_pre_approx->first();

                sequence = approx_elem->map->sequence(align_profile->sequence());
                String = sequence->string();

                if (!check_base_invariance(String, align_string))
                    ali_error("Bases changed in output sequence");

                if (!convert_for_back_write(String, align_string_original))
                    ali_fatal_error("Can't convert correctly");

                aligs.arbdb.begin_transaction();
                aligs.arbdb.put_sequence_string(species_name, String);
                aligs.arbdb.put_SAI("ALI_INSERTS", approx_elem->ins_marker);
                aligs.arbdb.commit_transaction();
                delete sequence;

                // Delete all Objects
                free(align_string);
                free(align_string_original);
                delete align_pre_solution;
                delete align_pre_approx;
                delete align_profile;
                delete align_pre_sequence;
                delete align_pre_sequence_i;
            }
            delete align_sequence;
        }
    }

    ali_message("Aligner terminated\n");
    return 0;
}
