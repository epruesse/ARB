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

char * SEC_read_line(istream & in) {
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


char * SEC_get_number_string(char * string_buffer, char identificator) {
    char *ptr = strchr(string_buffer, identificator);    //get pointer to character identificator in string_buffer
    if (ptr == NULL) {
        //aw_message("Error, character not found!");
        cout << "Error, character " << identificator << " not found!\n";
        return(0);
    }
    if (identificator == ':') {  //generates two separate strings from one like "5.5:3.3"
        *ptr = '\0';
    }
    return (ptr+1);   //return pointer to character following "identificator"
}


void SEC_make_numbers(double *number_1, double *number_2, char * string_buffer) {
    char *num1_string = SEC_get_number_string(string_buffer, '=');   //get pointer to string representing beginning of first number
    char *num2_string;
    if (number_2 != NULL) {
        num2_string = SEC_get_number_string(string_buffer, ':');
        *number_2 = strtod(num2_string, NULL);    //convert string to double, perhaps 1 char* pointer must be given to strtod additionally
    }
    *number_1 = strtod(num1_string, NULL);    //convert string to double, perhaps 1 char* pointer must be given to strtod additionally
}



/***********************************************************************
 * READ-Functions
 ***********************************************************************/

void SEC_segment::read(SEC_loop *loop_, istream & in) {
    loop = loop_;
    region.read(in, root);
    /*char * string_buffer = */ SEC_read_line(in);          //remove closing } from input stream
}


void SEC_region::read(istream & in, SEC_root *root) {
    char *string_buffer = SEC_read_line(in);
    if(strncmp(string_buffer, "SEQ=", 4)) {
        //aw_message("Unknown data format! Keyword \"SEQ\" not detected!");
        cout << "Unknown data format! Keyword \"SEQ\" not detected!\n";
        return;
    }
    else {
        double seq_start, seq_end;
        SEC_make_numbers(&seq_start, &seq_end, string_buffer);   //convert to numbers
        if (root->template_sequence != NULL) {
            if (seq_start < root->ap_length) {
                sequence_start = root->ap[(int) seq_start];
            }
            else {
#if defined(DEBUG)
                sec_assert(0);
#endif // DEBUG
                sequence_start = int(seq_start);
            }
            if (seq_end < root->ap_length) {
                sequence_end = root->ap[(int) seq_end];
            }
            else {
#if defined(DEBUG)
                sec_assert(0);
#endif // DEBUG
                sequence_end = int(seq_end);
            }
        }
        else {
            sequence_start = int(seq_start);
            sequence_end = int(seq_end);
        }
    }
}


void SEC_helix::read(istream & in) {

    char *string_buffer = SEC_read_line(in);

    if(strncmp(string_buffer, "DELTA=", 6)) {
        cout << "Unknown data format! \"STRAND\" must contain angle-information !\n";
    }
    else {
        SEC_make_numbers(&delta, NULL, string_buffer);   //read out DELTA

        string_buffer = SEC_read_line(in);
	if(strncmp(string_buffer, "DELTA_IN=", 9)) {
	    cout << "Unknown data format! \"STRAND\" must contain angle-information !\n";
	}
	else {
 	    SEC_make_numbers(&deltaIn, NULL, string_buffer);   //read out DELTA_IN

	    string_buffer = SEC_read_line(in);
	    if(strncmp(string_buffer, "LENGTH=", 7)) {
		cout << "Unknown data format! \"STRAND\" must contain \"max_length\" and \"min_lenth\" !\n";
	    }
	    SEC_make_numbers(&min_length, &max_length, string_buffer);    //read out LENGTH-information
	}
    }
}

void SEC_loop::read(SEC_helix_strand *other_strand, istream & in) {

    char *string_buffer = SEC_read_line(in);
    if(strncmp(string_buffer, "RADIUS=", 7)) {
        cout << "Unknown data format! \"LOOP\" must contain \"max_radius\" and \"min_radius\" !\n";
    }
    else {
        SEC_make_numbers(&min_radius, &max_radius, string_buffer);

        string_buffer = SEC_read_line(in);
        if(strncmp(string_buffer, "SEGMENT={", 6)) {
            cout << "Unknown data format! Loop must contain at least one segment!\n";
        }
        else {
            segment = new SEC_segment(root, 0, NULL, NULL);  //creating first segment
            segment->read(this, in);

            SEC_helix_strand *strand_pointer = 0;
            string_buffer = SEC_read_line(in);               //either "STRAND={" or "}" ist read from input stream
            int operation_selektor = -1;

            if(!(strncmp(string_buffer, "STRAND={", 8))) {		//if STRAND does not follow on SEGMENT then the loop is already complete;
                operation_selektor=1;
            }
            else if (!(strncmp(string_buffer, "}", 1))) {
                operation_selektor=3;
#if defined(DEBUG)
                sec_assert(other_strand!=NULL);
#endif // DEBUG
                segment->set_next_helix_strand(other_strand);
            }

            SEC_segment *segment_pointer = segment;
            while(operation_selektor < 3) {
                if (operation_selektor==1) {
                    strand_pointer = new SEC_helix_strand(root, NULL, NULL, NULL, NULL, 0, 0);
                    strand_pointer->read(this, in);
                    segment_pointer->set_next_helix_strand(strand_pointer);  //sets next_helix_strand-pointer of segment created previously to newly created strand
                }
                else if (operation_selektor==2) {
                    segment_pointer = new SEC_segment(root, 0, NULL, NULL);
                    segment_pointer->read(this, in);
                    strand_pointer->set_next_segment(segment_pointer);
                }
                string_buffer = SEC_read_line(in);
                if(!(strncmp(string_buffer, "STRAND={", 8))) {
                    operation_selektor=1;
                }
                else if (!(strncmp(string_buffer, "}", 1))) {
                    operation_selektor=3;
                    if (other_strand == NULL) {
                        strand_pointer->set_next_segment(segment);
                    }
                    else {
                        segment_pointer->set_next_helix_strand(other_strand);
                        strand_pointer->set_next_segment(segment_pointer);
                    }
                }
                else if(!(strncmp(string_buffer, "SEGMENT={", 9))) {
                    operation_selektor=2;
                }
            } //end while
        } //close bracket of second else
    } // close bracket of else
}


void SEC_helix_strand::read(SEC_loop *loop_, istream & in) {
    SEC_helix_strand *strand2 = new SEC_helix_strand(root, NULL, NULL, NULL, NULL, 0, 0);

    loop = loop_;
    other_strand = strand2;
    strand2->other_strand = this;

    region.read(in, root);
    helix_info = new SEC_helix;
    strand2->helix_info = helix_info;
    helix_info->read(in);

    char *string_buffer = SEC_read_line(in);
    if(strncmp(string_buffer, "LOOP={", 6)) {
        cout << "Unknown data format! Strand must be followed by \"Loop\"!\n";
    }
    else {
        strand2->loop = new SEC_loop(root, NULL, 0, 0);
        strand2->loop->read(strand2, in);
        strand2->next_segment = strand2->loop->get_segment();             //strand2's next_segment points to his loop's first segment
        strand2->region.read(in, root);     //Loop is complete, now trailing SEQ information must be read

        string_buffer = SEC_read_line(in);    //remove closing } from input-stream
    }
}

GB_ERROR SEC_root::read_data(char *input_string, char *x_string_in, long current_ali_len) {
    istringstream in(input_string);

    int n = 0;
    if (template_sequence != NULL) {
        int i;

        for (i=strlen(x_string_in)-1; i >= 0; i--) {		// count number of x
            if (x_string_in[i] == 'x') n++;
        }
        ap = new int[n];
        ap_length = n;
        int j=0;
        int x_string_in_len = strlen(x_string_in);
        for (i=0; i<x_string_in_len; i++) {				// create x to absposition table
            if (x_string_in[i] == 'x') {
                ap[j] = i;
                j++;
            }
        }
    }

    char *string_buffer = SEC_read_line(in);
    if(strncmp(string_buffer, "MAX_INDEX=", 10)) {
        return GB_export_error("Unknown data format! \"MAX_INDEX\" must be defined!");
    }
    double tmp_max_index;
    SEC_make_numbers(&tmp_max_index, NULL, string_buffer);
    max_index = (int) tmp_max_index;
    if (max_index < current_ali_len) {
	max_index = current_ali_len;
    }

    string_buffer = SEC_read_line(in);
    if(strncmp(string_buffer, "ROOT_ANGLE=", 11)==0) {
	rootAngle = strtod(string_buffer+11,0);
	string_buffer = SEC_read_line(in);
    }
    else {
	rootAngle = 0.0;
    }

    if(strncmp(string_buffer, "LOOP={", 6)) {
        return GB_export_error("Unknown data format! String must define \"Root-Loop\"!");
    }

    SEC_loop *root_loop = new SEC_loop(this, NULL, 0, 0);
    root_loop->read(NULL, in);

    root_segment = root_loop->get_segment();   //root_segment of root_loop points to first segment of root-loop
    update(0);

    delete [] ap; ap = 0;
    return 0;
}
