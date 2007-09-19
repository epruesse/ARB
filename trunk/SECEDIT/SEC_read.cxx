// =============================================================== //
//                                                                 //
//   File      : SEC_read.cxx                                      //
//   Purpose   : read structure from declaration string            //
//   Time-stamp: <Fri Sep/07/2007 09:02 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


#include <sstream>
#include "SEC_root.hxx"
#include "SEC_iter.hxx"

#define BUFFER_SIZE 1000

using namespace std;

/***********************************************************************
 * Supplementary Functions
 ***********************************************************************/

static const char * sec_read_line(istream & in) {
    static char string_buffer[BUFFER_SIZE];
    in.getline(string_buffer, BUFFER_SIZE);

    //clean input-stream of whitespaces
    int j=0;
    for (int i=0; i<(BUFFER_SIZE-1); i++) {
        if (!(isspace(string_buffer[i]))) {
            string_buffer[j] = string_buffer[i];
            j++;
            if (string_buffer[i] == '\0') break;
        }
    }
    return string_buffer;
}

static GB_ERROR sec_scan_ints(const char * string_buffer, int *number_1, int *number_2) {
    GB_ERROR  error = 0;
    char     *scanend;          // this is a 'const char *'
    
    sec_assert(number_1);
    *number_1 = (int)strtol(string_buffer, &scanend, 10);
    if (number_2) {
        if (scanend[0] != ':') {
            error = "expected ':' after integer number";
        }
        else {
            *number_2 = (int)strtol(scanend+1, &scanend, 10);
        }
    }
    if (!error && scanend[0] != 0) { // not at string end
        error = number_2 ? "unexpected content after '=NUMBER:NUMBER'" : "unexpected content after '=NUMBER'";
    }
    return error;
}

static GB_ERROR sec_scan_doubles(const char * string_buffer, double *number_1, double *number_2) {
    GB_ERROR  error = 0;
    char     *scanend;          // this is a 'const char *'
    
    sec_assert(number_1);
    *number_1 = strtod(string_buffer, &scanend);
    if (number_2) {
        if (scanend[0] != ':') {
            error = "expected ':' after floating-point number";
        }
        else {
            *number_2 = strtod(scanend+1, &scanend);
        }
    }
    if (!error && scanend[0] != 0) { // not at string end
        error = number_2 ? "unexpected content after '=NUMBER:NUMBER'" : "unexpected content after '=NUMBER'";
    }
    return error;
}

static GB_ERROR sec_expect_keyword_and_ints(const char *string_buffer, const char *keyword, size_t keywordlen, int *number_1, int *number_2) {
    // scans 'KEYWORD = NUM:NUM' or 'KEYWORD=NUM'
    // 1 or 2 numbers are returned via number_1 and number_2

    sec_assert(strlen(keyword) == keywordlen);
    
    GB_ERROR error = 0;
    if (strncmp(string_buffer, keyword, keywordlen) != 0 || string_buffer[keywordlen] != '=') {
        error = GBS_global_string("Expected '%s='", keyword);
    }
    else {
        error = sec_scan_ints(string_buffer+keywordlen+1, number_1, number_2);
    }
    if (error) error = GBS_global_string("%s (while parsing '%s')", error, string_buffer);
    return error;
}

static GB_ERROR sec_expect_keyword_and_doubles(const char *string_buffer, const char *keyword, size_t keywordlen, double *number_1, double *number_2) {
    // scans 'KEYWORD = NUM:NUM' or 'KEYWORD=NUM'
    // 1 or 2 numbers are returned via number_1 and number_2

    sec_assert(strlen(keyword) == keywordlen);
    
    GB_ERROR error = 0;
    if (strncmp(string_buffer, keyword, keywordlen) != 0 || string_buffer[keywordlen] != '=') {
        error = GBS_global_string("Expected '%s='", keyword);
    }
    else {
        error = sec_scan_doubles(string_buffer+keywordlen+1, number_1, number_2);
    }
    if (error) error = GBS_global_string("%s (while parsing '%s')", error, string_buffer);
    return error;
}

static GB_ERROR sec_expect_constraints(const char *string_buffer, const char *keyword, size_t keywordlen, SEC_constrainted *elem) {
    double   min, max;
    GB_ERROR error = sec_expect_keyword_and_doubles(string_buffer, keyword, keywordlen, &min, &max);
    if (!error) elem->setConstraints(min, max);
    return error;
}

static GB_ERROR sec_expect_closing_bracket(istream& in) {
    const char *string_buffer = sec_read_line(in);

    if (strcmp(string_buffer, "}") == 0) return 0;
    return GBS_global_string("Expected '}' instead of '%s'", string_buffer);
}

/***********************************************************************
 * READ-Functions
 ***********************************************************************/

GB_ERROR SEC_region::read(istream & in, SEC_root *root, int /*version*/) {
    int         seq_start, seq_end;
    const char *string_buffer = sec_read_line(in);
    GB_ERROR    error         = sec_expect_keyword_and_ints(string_buffer, "SEQ", 3, &seq_start, &seq_end);

    if (!error) {
        sec_assert(root->get_db()->canDisplay());
        if (root->get_db()->canDisplay()) {
            const XString& x_string = root->get_xString();
            int x_count  = x_string.getXcount();

            if (seq_start >= x_count)           error = GBS_global_string("Region start (%i) out of bounds [0..%i]", seq_start, x_count-1);
            else if (seq_end >= x_count)        error = GBS_global_string("Region end (%i) out of bounds [0..%i]", seq_end, x_count-1);
            else                                set_sequence_portion(x_string.getAbsPos(seq_start), x_string.getAbsPos(seq_end));
        }
        else {
            set_sequence_portion(seq_start, seq_end);
        }
    }
    return error;
}

GB_ERROR SEC_segment::read(SEC_loop *loop_, istream & in, int version) {
    loop              = loop_;
    GB_ERROR error    = get_region()->read(in, get_root(), version);
    if (!error) error = sec_expect_closing_bracket(in);
    return error;
}

GB_ERROR SEC_helix::read(istream & in, int version, double& old_angle_in) {
    const char *string_buffer = sec_read_line(in);
    GB_ERROR    error         = 0;

    old_angle_in = NAN;     // illegal for version >= 3

    if (version >= 3) {
        double angle;

        error = sec_expect_keyword_and_doubles(string_buffer, "REL", 3, &angle, 0);
        
        if (!error) {
            string_buffer = sec_read_line(in);
            set_rel_angle(angle);
        }
    }
    else { // version 2 or lower
        double angle;
        
        error = sec_expect_keyword_and_doubles(string_buffer, "DELTA", 5, &angle, 0);
        if (!error) {
            string_buffer = sec_read_line(in);
            set_abs_angle(angle);

            if (version == 2) {
                error = sec_expect_keyword_and_doubles(string_buffer, "DELTA_IN", 8, &angle, 0);

                if (!error) {
                    string_buffer = sec_read_line(in);
                    old_angle_in  = angle+M_PI; // rotate! (DELTA_IN pointed from outside-loop to strand)
                }
            }
            else {
                old_angle_in = angle; // use strands angle
            }
        }
    }

    if (!error) error = sec_expect_constraints(string_buffer, "LENGTH", 6, this);

    return error;
}


GB_ERROR SEC_helix_strand::read(SEC_loop *loop_, istream & in, int version) {
    // this points away from root-loop, strand2 points towards root

    SEC_helix_strand *strand2 = new SEC_helix_strand;

    origin_loop = 0; 

    SEC_root *root  = loop_->get_root();
    GB_ERROR  error = get_region()->read(in, root, version);
    
    double next_loop_angle = NAN;

    if (!error) {
        helix_info  = new SEC_helix(root, strand2, this);
        origin_loop = loop_;    // needed by read()
        error       = helix_info->read(in, version, next_loop_angle);
        origin_loop = 0;

        if (error) {
            delete helix_info;
            helix_info          = 0;
            strand2->helix_info = 0;
        }
    }

    if (!error) {
        const char *string_buffer = sec_read_line(in);
        if (strncmp(string_buffer, "LOOP={", 6) != 0) {
            error = GBS_global_string("Strand must be followed by 'LOOP={' (not by '%s')", string_buffer);
        }
        else {
            SEC_loop *new_loop   = new SEC_loop(root);
            strand2->origin_loop = new_loop;

            error = new_loop->read(strand2, in, version, next_loop_angle);

            if (!error) {
                error = strand2->get_region()->read(in, root, version); // Loop is complete, now trailing SEQ information must be read
                string_buffer = sec_read_line(in);    // remove closing } from input-stream
            }
            
            if (error) {
                strand2->origin_loop = 0;
                delete new_loop;
            }
        }
    }

    // Note: don't delete strand2 in case of error -- it's done by caller via deleting 'this'

    sec_assert(origin_loop == 0);
    if (!error) origin_loop = loop_; 

    return error;
}



GB_ERROR SEC_loop::read(SEC_helix_strand *rootside_strand, istream & in, int version, double loop_angle) {
    // loop_angle is only used by old versions<3
    
    const char *string_buffer = sec_read_line(in);
    GB_ERROR    error         = sec_expect_constraints(string_buffer, "RADIUS", 6, this);

    if (!error) {
        set_fixpoint_strand(rootside_strand);

        if (version == 3) {
            string_buffer = sec_read_line(in);

            double angle;
            error = sec_expect_keyword_and_doubles(string_buffer, "REL", 3, &angle, 0);
            if (!error) set_rel_angle(angle);
        }
        else {
            set_abs_angle(loop_angle);
            sec_assert(get_rel_angle().valid());
        }
    }

    if (!error) {
        enum { EXPECT_SEGMENT, EXPECT_STRAND } expect = (!rootside_strand && version >= 3) ? EXPECT_STRAND : EXPECT_SEGMENT;

        SEC_segment      *first_new_segment   = 0;
        SEC_helix_strand *first_new_strand    = 0;
        SEC_segment      *last_segment        = 0;
        SEC_helix_strand *last_outside_strand = rootside_strand;

        bool done = false;
        while (!done && !error) {
            string_buffer = sec_read_line(in);

            if (strncmp(string_buffer, "}", 1) == 0) {
                done = true;
            }
            else if (expect == EXPECT_SEGMENT) {
                if (strncmp(string_buffer, "SEGMENT={", 9) == 0) {
                    SEC_segment *new_seg = new SEC_segment;

                    error = new_seg->read(this, in, version);
                    if (!error) {
                        if (last_outside_strand) last_outside_strand->set_next_segment(new_seg);
                        last_outside_strand = 0;
                        last_segment        = new_seg;

                        if (!first_new_segment) first_new_segment = new_seg;
                        
                        expect = EXPECT_STRAND;
                    }
                    else delete new_seg;
                }
                else error = GBS_global_string("Expected SEGMENT (in '%s')", string_buffer);
            }
            else {
                sec_assert(expect == EXPECT_STRAND);
                if (strncmp(string_buffer, "STRAND={", 8) == 0) {
                    SEC_helix_strand *new_strand = new SEC_helix_strand;

                    error = new_strand->read(this, in, version);
                    if (!error) {
                        if (last_segment) last_segment->set_next_strand(new_strand);
                        last_outside_strand = new_strand;
                        last_segment        = 0;

                        if (!first_new_strand) {
                            first_new_strand = new_strand;
                            if (!rootside_strand) set_fixpoint_strand(first_new_strand);
                        }

                        expect = EXPECT_SEGMENT;
                    }
                    else delete new_strand;
                }
                else error = GBS_global_string("Expected STRAND (in '%s')", string_buffer);
            }
        }
        
        if (!error && !first_new_segment) error = "Expected at least one SEGMENT in LOOP{}";
        if (!error && !first_new_strand && !rootside_strand) error = "Expected at least one STRAND in root-LOOP{}";

        if (!error) {
            if (last_segment) {
                sec_assert(last_segment->get_next_strand() == 0);
                if (rootside_strand) {
                    last_segment->set_next_strand(rootside_strand);
                }
                else { // root loop (since version 3)
                    last_segment->set_next_strand(first_new_strand);
                }
            }
            else {
                sec_assert(last_outside_strand);
                sec_assert(version<3); // version 3 loops always end with segment

                sec_assert(!rootside_strand); // only occurs in root-loop
                last_outside_strand->set_next_segment(first_new_segment);
                
            }
        }
        else {
            if (rootside_strand) rootside_strand->set_next_segment(0); // unlink me from parent
        }
    }

    if (error) set_fixpoint_strand(0);

    return error;
}

GB_ERROR SEC_root::read_data(const char *input_string, const char *x_string_in) {
    istringstream in(input_string);

    delete xString;
    xString = 0;

    GB_ERROR error   = 0;
    int      version = -1;      // version of structure string

    sec_assert(db->canDisplay());

    if (!error) {
        const char *string_buffer  = sec_read_line(in);
        double      firstLoopAngle = 0; 

        error = sec_expect_keyword_and_ints(string_buffer, "VERSION", 7, &version, 0); // version 3++
        if (error) { // version < 3!
            version = 2;        // or less

            int ignoreMaxIndex;
            error = sec_expect_keyword_and_ints(string_buffer, "MAX_INDEX", 9, &ignoreMaxIndex, 0); // version 1+2
            if (!error) {
                string_buffer = sec_read_line(in);
                error = sec_expect_keyword_and_doubles(string_buffer, "ROOT_ANGLE", 10, &firstLoopAngle, 0); // version 2 only
                if (error) {
                    firstLoopAngle += M_PI;
                    version = 1; // version 1 had no ROOT_ANGLE entry
                    error   = 0;
                }
                else {
                    firstLoopAngle += M_PI;
                    string_buffer = sec_read_line(in);
                }
            }
        }
        else {
            if (version>DATA_VERSION) {
                error = GBS_global_string("Structure has version %i, your ARB can only handle versions up to %i", version, DATA_VERSION);
            }
            else {
                string_buffer = sec_read_line(in);
            }
        }

        if (!error) { //   && db->canDisplay()
            sec_assert(!xString);

            size_t len     = strlen(x_string_in);
            size_t exp_len = db->length();

            if (len != exp_len && len != (exp_len+1)) {
                error = GBS_global_string("Wrong xstring-length (found=%u, expected=%u-%u)",
                                          len, exp_len, exp_len+1);
            }
            else {
                xString = new XString(x_string_in, len, db->length());
                size_t xlen = xString->getLength(); // internally one position longer than alignment length

#if defined(DEBUG)
                printf("x_string_in len=%u\nxlen=%u\nali_length=%u\n", strlen(x_string_in), xlen, db->length());
#endif // DEBUG
            }
        }

        set_root_loop(0);
    
        if (!error) {
#if defined(DEBUG)
            printf("Reading structure format (version %i)\n", version);
#endif // DEBUG

            if (strncmp(string_buffer, "LOOP={", 6) == 0) {
                SEC_loop *root_loop = new SEC_loop(this); // , NULL, 0, 0);

                set_root_loop(root_loop);
                set_under_construction(true);
                error = root_loop->read(NULL, in, version, firstLoopAngle);

                if (!error) {
                    set_under_construction(false); // mark as "constructed"
                }
                else {
                    set_root_loop(0);
                    delete root_loop;
                }
            }
            else {
                error = GBS_global_string("Expected root loop ('LOOP={'), not '%s'", string_buffer);
            }
        }
    }

    if (!error) {
        if (version<3) fixStructureBugs(version);
#if defined(CHECK_INTEGRITY)
        check_integrity(CHECK_STRUCTURE);
#endif // CHECK_INTEGRITY
        if (version<3) {
            relayout();
            root_loop->fixAngleBugs(version);
        }
    }

    return error;
}

void SEC_helix::fixAngleBugs(int version) {
    if (version<3) {
        outsideLoop()->fixAngleBugs(version); // first correct substructure

        // versions<3 silently mirrored angles between strand and origin-loop
        // to ensure that strand always points away from loop (and not inside).
        // This correction was done during refresh and therefore not saved to the DB.
        // Since version 3 no angles will be mirrored automatically, so we need to correct them here.

        Vector loop2strand(rootsideLoop()->get_center(), get_fixpoint());
        if (scalarProduct(loop2strand, get_abs_angle().normal()) < 0) { // < 0 means angle is an acute angle
            printf("* Autofix acute angle (loop2strand=%.2f, abs=%.2f) of helix nr '%s'\n",
                   Angle(loop2strand).radian(),
                   get_abs_angle().radian(),
                   get_root()->helixNrAt(strandToOutside()->startAttachAbspos()));
            set_rel_angle(Angle(get_rel_angle()).rotate180deg());
        }
    }
}

void SEC_loop::fixAngleBugs(int version) {
    if (version<3) {
        for (SEC_strand_iterator strand(this); strand; ++strand) {
            if (strand->pointsToOutside()) {
                strand->get_helix()->fixAngleBugs(version);
            }
        }
    }
}

void SEC_root::fixStructureBugs(int version) {
    if (version < 3) {
        // old versions produced data structure with non-adjacent regions
        SEC_base_part *start_part = root_loop->get_fixpoint_strand();
        SEC_base_part *part       = start_part;
        SEC_region    *reg        = part->get_region();

        do {
            SEC_base_part *next_part = part->next();
            SEC_region    *next_reg  = next_part->get_region();

            if (!are_adjacent_regions(reg, next_reg)) {
                printf("* Fixing non-adjacent regions: %i..%i and %i..%i\n",
                       reg->get_sequence_start(), reg->get_sequence_end(),
                       next_reg->get_sequence_start(), next_reg->get_sequence_end());

                // known bug occurs at segment->strand transition..
                sec_assert(part->parent()->getType() == SEC_LOOP);

                // .. and segment region ends one position too early
                sec_assert(get_helix()->helixNr(reg->get_sequence_end()) == 0);
                sec_assert(get_helix()->helixNr(next_reg->get_sequence_start()) != 0);

                // make them adjacent
                reg->set_sequence_portion(reg->get_sequence_start(), next_reg->get_sequence_start());
            }

            part = next_part;
            reg  = next_reg;
        }
        while (part != start_part);
    }
}
