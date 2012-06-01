// =============================================================== //
//                                                                 //
//   File      : ali_profile.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ali_profile.hxx"

#include <BI_helix.hxx>
#include <cctype>


inline int ALI_PROFILE::is_binding_marker(char c) {
    return (c == '~' || c == 'x');
}


ALI_TLIST<ali_family_member *> *ALI_PROFILE::find_family(ALI_SEQUENCE *Sequence, ALI_PROFILE_CONTEXT *context) {
    // find the family in the pt server
    char message_buffer[100];
    ALI_PT &pt = (ALI_PT &) *(context->pt);
    ALI_ARBDB &arbdb = (ALI_ARBDB &) *(context->arbdb);
    ALI_SEQUENCE *seq;
    ali_family_member *family_member;
    ALI_TLIST<ali_family_member *> *family_list;
    ALI_TLIST<ali_pt_member *> *pt_fam_list;
    ALI_TLIST<ali_pt_member *> *pt_ext_list;
    ali_pt_member *pt_member;
    float weight, d;
    unsigned long number;

    // Initialization
    family_list = new ALI_TLIST<ali_family_member *>;

    ali_message("Searching for the family");
    pt.find_family(Sequence, context->find_family_mode);
    ali_message("Family found");

    pt_fam_list = pt.get_family_list();
    pt_ext_list = pt.get_extension_list();

    ali_message("Reading family:");

    d = (context->ext_max_weight - 1.0) / (float) pt_fam_list->cardinality();

    arbdb.begin_transaction();

    // calculate the real family members
    number = 0;
    while (!pt_fam_list->is_empty()) {
        pt_member = pt_fam_list->first();
        seq = arbdb.get_sequence(pt_member->name, context->mark_family_flag);
        if (seq) {
            weight = 1 + d * number;
            sprintf(message_buffer, "%s (weight = %f, matches = %d)",
                    pt_member->name, weight, pt_member->matches);
            ali_message(message_buffer);
            family_member = new ali_family_member(seq,
                                                  (float) pt_member->matches,
                                                  weight);
            family_list->append_end(family_member);
            number++;
        }
        else {
            ali_warning("Sequence not found in Database (Sequence ignored)");
        }
        pt_fam_list->delete_element();
    }
    delete pt_fam_list;

    ali_message("Reading family extension:");

    d = -1.0 * context->ext_max_weight / (float) pt_ext_list->cardinality();

    // calculate the extension of the family
    number = 0;
    while (!pt_ext_list->is_empty()) {
        pt_member = pt_ext_list->first();
        seq = arbdb.get_sequence(pt_member->name,
                                 context->mark_family_extension_flag);
        if (seq) {
            weight = context->ext_max_weight + d * number;
            sprintf(message_buffer, "%s (weight = %f, matches = %d)",
                    pt_member->name, weight, pt_member->matches);
            ali_message(message_buffer);
            family_member = new ali_family_member(seq,
                                                  (float) pt_member->matches,
                                                  weight);
            family_list->append_end(family_member);
            number++;
        }
        else {
            ali_warning("Sequence not found in Database (Sequence ignored)");
        }
        pt_ext_list->delete_element();
    }
    delete pt_ext_list;

    arbdb.commit_transaction();

    return family_list;
}

void ALI_PROFILE::calculate_costs(ALI_TLIST<ali_family_member *> *family_list, ALI_PROFILE_CONTEXT *context) {
    // calculate the costs for aligning against a family
    ali_family_member *family_member;
    float a[7], w[7], w_sum, sm[7][7];
    float base_gap_weights[5], w_bg_sum;
    long members;
    size_t p;
    int i;
    size_t j;
    unsigned long *l;
    float       *g;
    unsigned char **seq;
    long        *seq_len;
    float (*w_Del)[], (*percent)[];

    // allocate temporary memory
    members = family_list->cardinality();
    l = (unsigned long *) CALLOC((unsigned int) members, sizeof(long));
    g = (float *) CALLOC((unsigned int) members, sizeof(float));
    seq = (unsigned char **) CALLOC((unsigned int) members, sizeof(char *));
    seq_len = (long *) CALLOC((unsigned int) members, sizeof(long));
    if (l == 0 || g == 0 || seq == 0 || seq_len == 0)
        ali_fatal_error("Out of memory");

    // initialize arrays
    family_member = family_list->first();
    prof_len = family_member->sequence->length();
    seq[0] = family_member->sequence->sequence();
    seq_len[0] = family_member->sequence->length();
    g[0] = family_member->weight;
    i = 1;
    sub_costs_maximum = 0.0;

    while (family_list->is_next()) {
        family_member = family_list->next();
        seq[i] = family_member->sequence->sequence();
        seq_len[i] = family_member->sequence->length();
        g[i] = family_member->weight;
        i++;
        if (prof_len < family_member->sequence->length()) {
            ali_warning("Family members have different length");
            prof_len = family_member->sequence->length();
        }
    }

    // Calculate the substitution cost matrix
    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++)
            sm[i][j] = context->substitute_matrix[i][j];

    // Initialize l-array (position of last base)
    for (i = 0; i < members; i++)
        l[i] = prof_len + 1;

    // allocate memory for costs

    base_weights  = (float (**) [4])   CALLOC((unsigned int) prof_len, sizeof(float [4]));
    sub_costs     = (float (**) [6])   CALLOC((unsigned int) prof_len, sizeof(float [6]));
    binding_costs = (float (*) [5][5]) CALLOC((unsigned int) 5,        sizeof(float [5]));
    lmin          = (unsigned long *)  CALLOC((unsigned int) prof_len, sizeof(unsigned long));
    lmax          = (unsigned long *)  CALLOC((unsigned int) prof_len, sizeof(unsigned long));
    gap_costs     = (float ***)        CALLOC((unsigned int) prof_len, sizeof(float *));
    gap_percents  = (float***)         CALLOC((unsigned int) prof_len, sizeof(float *));

    if (binding_costs == 0 || sub_costs == 0 || lmin == 0 || lmax == 0 ||
        gap_costs == 0 || gap_percents == 0 || base_weights == 0) {
        ali_fatal_error("Out of memory");
    }

    // Copy the binding costs matrix
    w_bind_maximum = context->binding_matrix[0][0];
    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++) {
            (*binding_costs)[i][j] = context->binding_matrix[i][j];
            if ((*binding_costs)[i][j] > w_bind_maximum)
                w_bind_maximum = (*binding_costs)[i][j];
        }

    // calculate the costs for EVERY position
    ali_message("Calculating costs for substitution");
    for (p = 0; p < prof_len; p++) {
        // Initialization
        for (i = 0; i < 7; i++)
            a[i] = w[i] = sm[5][i] = sm[i][5] = sm[6][i] = sm[i][6] = 0.0;
        for (i = 0; i < 6; i++)
            (*sub_costs)[p][i] = 0.0;
        w_sum = 0.0;
        w_bg_sum = 0.0;

        // Statistic consensus
        for (i = 0; i < members; i++) {
            if (p < size_t(seq_len[i])) {
                a[seq[i][p]]++;
                w[seq[i][p]] += g[i];
                if (ali_is_real_base(seq[i][p]))
                    w_sum += g[i];
                if (ali_is_real_base(seq[i][p]) || ali_is_gap(seq[i][p]))
                    w_bg_sum += g[i];
            }
            else {
                a[ALI_DOT_CODE]++;
                w[ALI_DOT_CODE] += g[i];
            }
        }

        // Relative weight of bases
        if (w_sum != 0)
            for (i = 0; i < 4; i++)
                (*base_weights)[p][i] = w[i] / w_sum;
        else
            for (i = 0; i < 4; i++)
                (*base_weights)[p][i] = 0.25;

        // Relative weight of bases and gaps
        if (w_bg_sum != 0)
            for (i = 0; i < 5; i++)
                base_gap_weights[i] = w[i] / w_bg_sum;
        else
            for (i = 0; i < 5; i++)
                base_gap_weights[i] = 0.2;

        // Expandation of substitute matrix (for 'n')
        for (j = 0; j < 5; j++) {
            for (i = 0; i < 4; i++) {
                sm[5][j] += (*base_weights)[p][i] * sm[i][j];
                sm[j][5] += (*base_weights)[p][i] * sm[j][i];
            }
        }
        for (i = 0; i < 4; i++)
            sm[5][5] += (*base_weights)[p][i] * sm[i][i];

        // Expandation of substitute matrix (for '.')
        for (j = 0; j < 6; j++)
            for (i = 0; i < 5; i++) {
                sm[6][j] += base_gap_weights[i] * sm[i][j];
                sm[j][6] += base_gap_weights[i] * sm[j][i];
            }
        for (i = 0; i < 5; i++)
            sm[6][6] += base_gap_weights[i] * sm[i][i];

        // Substitution costs
        for (i = 0; i < members; i++) {
            if (p < size_t(seq_len[i])) {
                for (j = 0; j < 6; j++) {
                    (*sub_costs)[p][j] += g[i] * sm[seq[i][p]][j];
                }
            }
            else {
                for (j = 0; j < 6; j++) {
                    (*sub_costs)[p][j] += g[i] * sm[ALI_DOT_CODE][j];
                }
            }
        }
        for (j = 0; j < 6; j++) {
            (*sub_costs)[p][j] /= members;
            if ((*sub_costs)[p][j] > sub_costs_maximum)
                sub_costs_maximum = (*sub_costs)[p][j];
        }

        // Calculate dynamic deletion costs and percents of real gaps
        lmax[p] = 0;
        lmin[p] = p;
        for (i = 0; i < members; i++)
            if (l[i] < p) {
                if (lmin[p] > l[i])
                    lmin[p] = l[i];
                if (lmax[p] < l[i])
                    lmax[p] = l[i];
            }
        if (lmin[p] == p && lmax[p] == 0) {
            lmin[p] = lmax[p] = p;
        }

        w_Del = (float (*) []) CALLOC((unsigned int) (lmax[p]-lmin[p]+2),               sizeof(float));
        percent = (float (*) []) CALLOC((unsigned int) (lmax[p]-lmin[p]+2),     sizeof(float));
        if (w_Del == 0 || percent == 0)
            ali_fatal_error("Out of memory");
        (*gap_costs)[p] = (float *) w_Del;
        (*gap_percents)[p] = (float *) percent;

        // Calculate dynamic deletion costs
        for (j = 0; j <= lmax[p] - lmin[p] + 1; j++) {
            (*w_Del)[j] = 0;
            for (i = 0; i < members; i++) {
                // Normal case
                if (p < size_t(seq_len[i])) {
                    if (l[i] == prof_len + 1 || l[i] >= j + lmin[p]) {
                        (*w_Del)[j] += g[i] * sm[seq[i][p]][4] * context->multi_gap_factor;
                    }
                    else {
                        (*w_Del)[j] += g[i] * sm[seq[i][p]][4];
                    }
                }
                // expand sequence with dots
                else {
                    if (l[i] >= j + lmin[p] && l[i] < prof_len+1) {
                        (*w_Del)[j] += g[i] * sm[ALI_DOT_CODE][4] * context->multi_gap_factor;
                    }
                    else {
                        (*w_Del)[j] += g[i] * sm[ALI_DOT_CODE][4];
                    }
                }
            }
            (*w_Del)[j] /= members;
        }

        // Update the l-array
        for (i = 0; i < members; i++) {
            if (!ali_is_gap(seq[i][p]))
                l[i] = p;
        }

        // Calculate percents of real gaps
        for (j = 0; j <= lmax[p] - lmin[p] + 1; j++) {
            (*percent)[j] = 0;
            for (i = 0; i < members; i++) {
                if (l[i] >= j + lmin[p] && l[i] < prof_len+1) {
                    (*percent)[j] += 1.0;
                }
            }
            (*percent)[j] /= members;
        }
    }

    ali_message("Calculation finished");

    free(l);
    free(g);
    free(seq);
    free(seq_len);
}

int ALI_PROFILE::find_next_helix(char h[], unsigned long h_len,
                                 unsigned long pos,
                                 unsigned long *helix_nr,
                                 unsigned long *start, unsigned long *end)
{
    // find the next helix
    unsigned long i;

    for (i = pos; i < h_len && !isdigit(h[i]); i++) ;
    if (i >= h_len) return -1;

    *start = i;
    sscanf(&h[i], "%ld", helix_nr);
    for (; i < h_len && isdigit(h[i]); i++) ;
    for (; i < h_len && !isdigit(h[i]); i++) ;
    *end = i - 1;

    return 0;
}

int ALI_PROFILE::find_comp_helix(char h[], unsigned long h_len,
                                 unsigned long pos, unsigned long helix_nr,
                                 unsigned long *start, unsigned long *end)
{
    // find the complementary part of a helix
    unsigned long nr, i;

    i = pos;
    do {
        for (; i < h_len && !isdigit(h[i]); i++) ;
        if (i >= h_len) return -1;
        *start = i;
        sscanf(&h[i], "%ld", &nr);
        for (; i < h_len && isdigit(h[i]); i++) ;
    } while (helix_nr != nr);

    for (; i < h_len && !isdigit(h[i]); i++) ;
    *end = i - 1;

    return 0;
}

void ALI_PROFILE::delete_comp_helix(char h1[], char h2[], unsigned long h_len,
                                    unsigned long start, unsigned long end)
{
    unsigned long i;

    for (i = start; i < h_len && i <= end; i++) {
        h1[i] = '.';
        h2[i] = '.';
    }
}


void ALI_PROFILE::initialize_helix(ALI_PROFILE_CONTEXT *context) {
    // initialize the array, representing the helix
    const char *error_string;
    BI_helix bi_helix;

    unsigned long i;

    // read helix
    if ((error_string = bi_helix.init(context->arbdb->gb_main)) != 0)
        ali_warning(error_string);

    helix_len = bi_helix.size();
    helix = (long **) CALLOC((unsigned int) helix_len, sizeof(long));
    helix_borders = (char **) CALLOC((unsigned int) helix_len, sizeof(long));
    if (helix == 0 || helix_borders == 0) ali_fatal_error("Out of memory");

    // convert helix for internal use
    for (i = 0; i < helix_len; i++)
        if (bi_helix.pairtype(i) == HELIX_PAIR)
            (*helix)[i] = bi_helix.opposite_position(i);
        else
            (*helix)[i] = -1;
}


ALI_PROFILE::ALI_PROFILE(ALI_SEQUENCE *seq, ALI_PROFILE_CONTEXT *context)
{
    char message_buffer[100];
    ali_family_member *family_member;
    ALI_TLIST<ali_family_member *> *family_list;

    norm_sequence = new ALI_NORM_SEQUENCE(seq);

    multi_gap_factor = context->multi_gap_factor;

    initialize_helix(context);

    family_list = find_family(seq, context);
    if (family_list->is_empty()) {
        ali_error("Family not found (maybe incompatible PT and DB Servers)");
    }

    calculate_costs(family_list, context);

    insert_cost = sub_costs_maximum * context->insert_factor;
    multi_insert_cost = insert_cost * context->multi_insert_factor;

    sprintf(message_buffer, "Multi gap factor = %f", multi_gap_factor);
    ali_message(message_buffer);
    sprintf(message_buffer, "Maximal substitution costs = %f", sub_costs_maximum);
    ali_message(message_buffer);
    sprintf(message_buffer, "Normal insert costs = %f", insert_cost);
    ali_message(message_buffer);
    sprintf(message_buffer, "Multiple insert costs = %f", multi_insert_cost);
    ali_message(message_buffer);

    // Delete the family list
    family_member = family_list->first();
    delete family_member->sequence;
    delete family_member;
    while (family_list->is_next()) {
        family_member = family_list->next();
        delete family_member->sequence;
        delete family_member;
    }
    delete family_list;
}

ALI_PROFILE::~ALI_PROFILE()
{
    size_t i;

    free(helix);
    free(helix_borders);
    free(binding_costs);
    free(sub_costs);
    if (gap_costs) {
        for (i = 0; i < prof_len; i++)
            if ((*gap_costs)[i])
                free((*gap_costs)[i]);
        free(gap_costs);
    }
    if (gap_percents) {
        for (i = 0; i < prof_len; i++)
            if ((*gap_percents)[i])
                free((*gap_percents)[i]);
        free(gap_percents);
    }
    free(lmin);
    free(lmax);
    delete norm_sequence;
}


int ALI_PROFILE::is_in_helix(unsigned long pos, unsigned long *first, unsigned long *last) {
    // test whether a position is inside a helix
    long i;

    if (pos > helix_len)
        return 0;

    switch ((*helix_borders)[pos]) {
        case ALI_PROFILE_BORDER_LEFT:
            *first = pos;
            for (i = (long) pos + 1; i < (long) prof_len; i++)
                if ((*helix_borders)[i] == ALI_PROFILE_BORDER_RIGHT) {
                    *last = (unsigned long) i;
                    return 1;
                }
            ali_warning("Helix borders inconsistent (1)");
            return 0;
        case ALI_PROFILE_BORDER_RIGHT:
            *last = pos;
            for (i = (long) pos - 1; i >= 0; i--)
                if ((*helix_borders)[i] == ALI_PROFILE_BORDER_LEFT) {
                    *first = (unsigned long) i;
                    return 1;
                }
            ali_warning("Helix borders inconsistent (2)");
            return 0;
        default:
            for (i = (long) pos - 1; i >= 0; i--)
                switch ((*helix_borders)[i]) {
                    case ALI_PROFILE_BORDER_RIGHT:
                        return 0;
                    case ALI_PROFILE_BORDER_LEFT:
                        *first = (unsigned long) i;
                        for (i = (long) pos + 1; i < (long) prof_len; i++)
                            switch ((*helix_borders)[i]) {
                                case ALI_PROFILE_BORDER_LEFT:
                                    ali_warning("Helix borders inconsistent (3)");
                                    printf("pos = %ld\n", pos);
                                    return 0;
                                case ALI_PROFILE_BORDER_RIGHT:
                                    *last = (unsigned long) i;
                                    return 1;
                            }
                }
    }
    return 0;
}

int ALI_PROFILE::is_outside_helix(unsigned long pos, unsigned long *first, unsigned long *last) {
    // test, whether a position is outside a helix
    long i;

    switch ((*helix_borders)[pos]) {
        case ALI_PROFILE_BORDER_LEFT:
            return 0;
        case ALI_PROFILE_BORDER_RIGHT:
            return 0;
        default:
            for (i = (long) pos - 1; i >= 0; i--)
                switch ((*helix_borders)[i]) {
                    case ALI_PROFILE_BORDER_LEFT:
                        return 0;
                    case ALI_PROFILE_BORDER_RIGHT:
                        *first = (unsigned long) i + 1;
                        for (i = (long) pos + 1; i < (long) prof_len; i++)
                            switch ((*helix_borders)[i]) {
                                case ALI_PROFILE_BORDER_LEFT:
                                    *last = (unsigned long) i - 1;
                                    return 1;
                                case ALI_PROFILE_BORDER_RIGHT:
                                    ali_warning("Helix borders inconsistent [1]");
                                    return 0;
                            }
                        *last = prof_len - 1;
                        return 1;
                }
            *first = 0;
            for (i = (long) pos + 1; i < (long) prof_len; i++)
                switch ((*helix_borders)[i]) {
                    case ALI_PROFILE_BORDER_LEFT:
                        *last = (unsigned long) i - 1;
                        return 1;
                    case ALI_PROFILE_BORDER_RIGHT:
                        ali_warning("Helix borders inconsistent [2]");
                        return 0;
                }
            *last = prof_len - 1;
            return 1;
    }
}


char *ALI_PROFILE::cheapest_sequence() {
    // generate a 'consensus sequence'

    char *seq;
    size_t p;
    int i, min_i;
    float min;


    seq = (char *) CALLOC((unsigned int) prof_len + 1, sizeof(char));
    if (seq == 0)
        ali_fatal_error("Out of memory");

    for (p = 0; p < prof_len; p++) {
        min = (*sub_costs)[p][0];
        min_i = 0;
        for (i = 1; i < 5; i++) {
            if (min > (*sub_costs)[p][i]) {
                min = (*sub_costs)[p][i];
                min_i = i;
            }
            else {
                if (min == (*sub_costs)[p][i])
                    min_i = -1;
            }
        }
        if (min_i >= 0)
            seq[p] = ali_number_to_base(min_i);
        else {
            if (min > 0)
                seq[p] = '*';
            else
                seq[p] = '.';
        }
    }
    seq[prof_len] = '\0';

    return seq;
}

float ALI_PROFILE::w_binding(unsigned long first_seq_pos, ALI_SEQUENCE *seq) {
    // calculate the costs of a binding
    unsigned long pos_1_seq, pos_2_seq, last_seq_pos;
    long pos_compl;
    float costs = 0;

    last_seq_pos = first_seq_pos + seq->length() - 1;
    for (pos_1_seq = first_seq_pos; pos_1_seq <= last_seq_pos; pos_1_seq++) {
        pos_compl = (*helix)[pos_1_seq];
        if (pos_compl >= 0) {
            pos_2_seq = (unsigned long) pos_compl;
            if (pos_2_seq > pos_1_seq && pos_2_seq <= last_seq_pos)
                costs += w_bind(pos_1_seq, seq->base(pos_1_seq),
                                pos_2_seq, seq->base(pos_2_seq));
            else
                if (pos_2_seq < first_seq_pos || pos_2_seq > last_seq_pos)
                    costs += w_bind_maximum;
        }
    }

    return costs;
}


