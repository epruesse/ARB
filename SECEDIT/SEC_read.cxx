#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>

#include <iostream>
#include <fstream>
#include <sstream>

#include <arbdb.h>

#include <aw_root.hxx>
#include <aw_device.hxx>

#include"secedit.hxx"
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

static GB_ERROR sec_expect_closing_bracket(istream& in) {
    const char *string_buffer = sec_read_line(in);

    if (strcmp(string_buffer, "}") == 0) return 0;
    return GBS_global_string("Expected '}' instead of '%s'", string_buffer);
}

/***********************************************************************
 * READ-Functions
 ***********************************************************************/

GB_ERROR SEC_segment::read(SEC_loop *loop_, istream & in) {
    loop              = loop_;
    GB_ERROR error    = region.read(in, root);
    if (!error) error = sec_expect_closing_bracket(in);
    return error;
}

GB_ERROR SEC_region::read(istream & in, SEC_root *root) {
    int         seq_start, seq_end;
    const char *string_buffer = sec_read_line(in);
    GB_ERROR    error         = sec_expect_keyword_and_ints(string_buffer, "SEQ", 3, &seq_start, &seq_end);

    if (!error) {
        if (root->template_sequence != NULL) {
            if (seq_start >= root->ap_length) {
                error = GBS_global_string("Region start (%i) out of bounds [0..%i]", seq_start, root->ap_length-1);
            }
            else if (seq_end >= root->ap_length) {
                error = GBS_global_string("Region end (%i) out of bounds [0..%i]", seq_end, root->ap_length-1);
            }
            else {
                sequence_start = root->ap[seq_start];
                sequence_end   = root->ap[seq_end];
            }
        }
        else {
            sequence_start = seq_start;
            sequence_end   = seq_end;
        }
    }
    return error;
}


GB_ERROR SEC_helix::read(istream & in) {
    const char *string_buffer = sec_read_line(in);
    GB_ERROR    error         = sec_expect_keyword_and_doubles(string_buffer, "DELTA", 5, &delta, 0);

    if (!error) {
        string_buffer = sec_read_line(in);
        error         = sec_expect_keyword_and_doubles(string_buffer, "DELTA_IN", 8, &deltaIn, 0);
        if (error) {
            static bool warned = false;
            if (!warned) {
                aw_message(GBS_global_string("Warning: %s.\nSetting DELTA_IN to 0.0", error));
                warned = true; // warn only once
            }
            error   = 0;
            deltaIn = 0.0;
        }
        else {
            string_buffer = sec_read_line(in);
        }
    }

    if (!error) {
        error = sec_expect_keyword_and_doubles(string_buffer, "LENGTH", 6, &min_length, &max_length);
    }

    return error;
}

GB_ERROR SEC_loop::read(SEC_helix_strand *other_strand, istream & in) {

    const char *string_buffer = sec_read_line(in);
    GB_ERROR    error         = sec_expect_keyword_and_doubles(string_buffer, "RADIUS", 6, &min_radius, &max_radius);

    if (!error) {
        string_buffer = sec_read_line(in);
        if (strncmp(string_buffer, "SEGMENT={", 6) == 0) {
            segment = new SEC_segment(root, 0, NULL, NULL); //creating first segment
            error   = segment->read(this, in);
        }
        else {
            error = "Loop has to contain at least one 'SEGMENT={}'";
        }
    }

    if (!error) {
        string_buffer = sec_read_line(in); //either "STRAND={" or "}" ist read from input stream

        enum { OP_UNDEF, OP_STRAND, OP_SEGMENT, OP_END } operation_selector = OP_UNDEF;

        if (strncmp(string_buffer, "STRAND={", 8) == 0) { // if STRAND does not follow on SEGMENT then the loop is already complete;
            operation_selector = OP_STRAND;
        }
        else if (strncmp(string_buffer, "}", 1) == 0) {
            operation_selector = OP_END;
#if defined(DEBUG)
            sec_assert(other_strand!=NULL);
#endif // DEBUG
            segment->set_next_helix_strand(other_strand);
        }

        SEC_segment      *segment_pointer = segment;
        SEC_helix_strand *strand_pointer  = 0;

        while (operation_selector != OP_END && !error) {
            if (operation_selector==OP_STRAND) {
                strand_pointer = new SEC_helix_strand(root, NULL, NULL, NULL, NULL, 0, 0);
                error          = strand_pointer->read(this, in);
                if (!error) {
                    // sets next_helix_strand-pointer of segment created previously to newly created strand
                    segment_pointer->set_next_helix_strand(strand_pointer);
                }
                else {
                    delete strand_pointer;
                }
            }
            else if (operation_selector==OP_SEGMENT) {
                segment_pointer = new SEC_segment(root, 0, NULL, NULL);
                error           = segment_pointer->read(this, in);
                if (!error) {
                    strand_pointer->set_next_segment(segment_pointer);
                }
                else {
                    delete segment_pointer;
                }
            }

            if (!error) {
                string_buffer = sec_read_line(in);
            
                if (strncmp(string_buffer, "STRAND={", 8) == 0) {
                    operation_selector = OP_STRAND;
                }
                else if (strncmp(string_buffer, "}", 1) == 0) {
                    operation_selector = OP_END;
                    if (other_strand) {
                        segment_pointer->set_next_helix_strand(other_strand);
                        strand_pointer->set_next_segment(segment_pointer);
                    }
                    else {
                        strand_pointer->set_next_segment(segment);
                    }
                }
                else if (strncmp(string_buffer, "SEGMENT={", 9) == 0) {
                    operation_selector = OP_SEGMENT;
                }
                else {
                    error = GBS_global_string("Illegal content while reading loop ('%s')", string_buffer);
                }
            }
        }
    }
    return error;
}


GB_ERROR SEC_helix_strand::read(SEC_loop *loop_, istream & in) {
    SEC_helix_strand *strand2 = new SEC_helix_strand(root, NULL, NULL, NULL, NULL, 0, 0);

    loop                  = 0; // only set loop at end and only if no error occurred (otherwise 'this' might be freed while PC is still inside this method)
    other_strand          = strand2;
    strand2->other_strand = this;

    GB_ERROR error = region.read(in, root);

    if (!error) {
        helix_info          = new SEC_helix;
        strand2->helix_info = helix_info;
        error               = helix_info->read(in);

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
            strand2->loop = new SEC_loop(root, NULL, 0, 0);
            error         = strand2->loop->read(strand2, in);
            if (!error) {
                strand2->next_segment = strand2->loop->get_segment(); // strand2's next_segment points to his loop's first segment
                strand2->region.read(in, root); // Loop is complete, now trailing SEQ information must be read

                string_buffer = sec_read_line(in);    // remove closing } from input-stream
            }
            else {
                delete strand2->loop;
                strand2->loop = 0;
            }
        }
    }

    // Note: don't delete strand2 in case of error -- it's done by caller via deleting 'this'

    sec_assert(loop == 0);
    if (!error) {
        loop = loop_;           // don't set in case of error (see above)
    }

    return error;
}

GB_ERROR SEC_root::read_data(char *input_string, char *x_string_in, long current_ali_len) {
    istringstream in(input_string);

    int n = 0;
    if (template_sequence != NULL) {
        int i;

        for (i=strlen(x_string_in)-1; i >= 0; i--) { // count number of x
            if (x_string_in[i] == 'x') n++;
        }
        ap = new int[n];
        ap_length = n;
        int j=0;
        int x_string_in_len = strlen(x_string_in);
        for (i=0; i<x_string_in_len; i++) { // create x to absposition table
            if (x_string_in[i] == 'x') {
                ap[j] = i;
                j++;
            }
        }
    }

    GB_ERROR error = 0;

    const char *string_buffer = sec_read_line(in);
    error                     = sec_expect_keyword_and_ints(string_buffer, "MAX_INDEX", 9, &max_index, 0);

    if (!error) {
        if (max_index < current_ali_len) {
            max_index = current_ali_len;
        }

        string_buffer = sec_read_line(in);
        error         = sec_expect_keyword_and_doubles(string_buffer, "ROOT_ANGLE", 10, &rootAngle, 0);
        if (error) {
            rootAngle = 0.0;
            error     = 0; // ignore this error 
            aw_message("Warning: Missing 'ROOT_ANGLE' entry (old structure format?)");
        }
        else {
            string_buffer = sec_read_line(in);
        }
    }
    
    root_segment = 0;
    
    if (!error) {
        if (strncmp(string_buffer, "LOOP={", 6) == 0) {
            SEC_loop *root_loop = new SEC_loop(this, NULL, 0, 0);
            error               = root_loop->read(NULL, in);

            if (!error) {
                root_segment = root_loop->get_segment(); // root_segment of root_loop points to first segment of root-loop
                update(0);
            }
            else {
                delete root_loop;
            }
        }
        else {
            error = GBS_global_string("Expected root loop ('LOOP={'), not '%s'", string_buffer);
        }

    }

    delete [] ap; ap = 0;

    return error;
}
