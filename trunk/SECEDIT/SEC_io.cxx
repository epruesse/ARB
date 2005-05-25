#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <arbdb.h>
#include <aw_root.hxx>

#include"secedit.hxx"

#define BUFFERLENGTH 20

using namespace std;

void SEC_root::generate_x_string(void) {
    x_string = new char[template_length+1];
    x_string[template_length] = '\0';
    x_string_len = template_length;
    number_found = new int[template_length];

    //"erase" both strings to prevent false x's

    //     int i;
    //     for (i=template_length-1; i>=0; i--) {
    // 	x_string[i] = '.';
    // 	number_found[i] = 0;
    //     }

    memset(x_string,     '.', template_length*sizeof(*x_string));
    memset(number_found, 0,   template_length*sizeof(*number_found));

    SEC_loop *root_loop = root_segment->get_loop();
    root_loop->generate_x_string(NULL);

    //initialize number_found with the right values
    number = 0;
    int i;
    for (i=0; i<template_length; i++) {
        if (x_string[i] == 'x') {
            number_found[i] = number;
            number++;
        }
        else {
            number_found[i] = number;
        }
    }
}

void SEC_segment::generate_x_string(void) {
    char *x_string = root->x_string;

    int sequence_start = region.get_sequence_start();
    int sequence_end = region.get_sequence_end();

    if (sequence_start < root->x_string_len) x_string[sequence_start] = 'x';
    if (sequence_end   < root->x_string_len) x_string[sequence_end]   = 'x';
    //    if (sequence_end   <= root->x_string_len) x_string[sequence_end-1] = 'x';
}


void SEC_helix_strand::generate_x_string(void) {
    char *x_string = root->x_string;

    int sequence_start = region.get_sequence_start();
    int sequence_end = region.get_sequence_end();

    if (sequence_start < root->x_string_len) x_string[sequence_start] = 'x';
    if (sequence_end   < root->x_string_len) x_string[sequence_end]   = 'x';
    //    if (sequence_end <= root->x_string_len) x_string[sequence_end-1] = 'x';

    sequence_start = other_strand->region.get_sequence_start();
    sequence_end = other_strand->region.get_sequence_end();

    if (sequence_start < root->x_string_len) x_string[sequence_start] = 'x';
    if (sequence_end   < root->x_string_len) x_string[sequence_end] = 'x';
    //    if (sequence_end <= root->x_string_len) x_string[sequence_end-1] = 'x';

    other_strand->loop->generate_x_string(other_strand);
}


void SEC_loop::generate_x_string(SEC_helix_strand *caller) {

    int is_root = 0;
    if (caller == NULL) {
        is_root = 1;
        caller = segment->get_next_helix();
    }

    SEC_segment *segment_pointer = caller->get_next_segment();
    SEC_helix_strand *strand_pointer = segment_pointer->get_next_helix();

    while (strand_pointer != caller) {
        segment_pointer->generate_x_string();
        strand_pointer->generate_x_string();

        segment_pointer = strand_pointer->get_next_segment();
        strand_pointer = segment_pointer->get_next_helix();
    }
    segment_pointer->generate_x_string();

    if (is_root) {
        caller->generate_x_string();
    }

}

SEC_helix_strand * SEC_segment::get_previous_strand(void) {
    SEC_helix_strand *strand_pointer = next_helix_strand;
    SEC_segment *segment_pointer = strand_pointer->get_next_segment();

    while (segment_pointer != this) {
        strand_pointer = segment_pointer->get_next_helix();
        segment_pointer = strand_pointer->get_next_segment();
    }
    return strand_pointer;
}

char * SEC_root::write_data(void) {
    delete [] x_string;
    x_string = 0;
    if (template_sequence != 0) {
        generate_x_string();
    }

    ostringstream out;
    out << "MAX_INDEX=" << max_index << "\n";
    out << "ROOT_ANGLE=" << rootAngle << "\n";
    SEC_loop *root_loop = root_segment->get_loop();
    root_loop->save(out, NULL, 0);

    out << '\0';

    delete [] number_found;
    number_found = 0;

    const string&  outstr = out.str();
    char          *result = new char[outstr.length()+1];
    strcpy(result, outstr.c_str());

    return result;
}


void SEC_segment::save(ostream & out, int indent) {
    int i;
    for (i=0; i<indent; i++) {
        out << "\t";
    }
    out << "SEGMENT={\n";
    region.save(out, (indent+1), root);

    for (i=0; i<indent; i++) {
        out << "\t";
    }

    out << "}\n";    //close brackets for block SEGMENT
}


void SEC_region::save(ostream & out, int indent, SEC_root *root) {
    for (int i=0; i<indent; i++) {
        out << "\t";
    }
    out << "SEQ=" << root->number_found[sequence_start] << ":" << root->number_found[sequence_end-1] << "\n";
}


void SEC_loop::save(ostream & out, SEC_helix_strand *caller, int indent) {
    int is_root = 0;
    if (caller == NULL) {
        is_root = 1;
        caller = segment->get_previous_strand();
    }
    int i;
    for (i=0; i<indent; i++) {
        out << "\t";
    }
    out << "LOOP={\n";

    indent++;

    for (i=0; i<indent; i++) {
        out << "\t";
    }
    out << "RADIUS=" << min_radius << ":" << max_radius << "\n";

    SEC_segment *next_segment;
    SEC_helix_strand *next_strand;
    next_segment = caller->get_next_segment();
    next_segment->save(out, indent);
    next_strand = next_segment->get_next_helix();

    while (next_strand != caller) {
        next_strand->save_all(out, indent);
        next_segment = next_strand->get_next_segment();  //returns pointer to following segment
        next_segment->save(out, indent);
        next_strand = next_segment->get_next_helix();       //returns pointer to following helix_strand
    }

    if (is_root) {    				//if root_loop then save caller-strand, which is otherwise omitted
        next_strand->save_all(out, indent);
    }
    indent--;
    for (i=0; i<indent; i++) {
        out << "\t";
    }
    out << "}\n";   //Close brackets for block LOOP
}


void SEC_helix::save(ostream & out, int indent) {
    int i;
    for (i=0; i<indent; i++) {
        out << "\t";
    }
    out << "DELTA=" << delta << "\n";

    //writing to export file
    for (i=0; i<indent; i++) {
        out << "\t";
    }
    out << "DELTA_IN=" << deltaIn << "\n";

    for (i=0; i<indent; i++) {
        out << "\t";
    }
    out << "LENGTH=" << min_length << ":" << max_length << "\n";;
}


void SEC_helix_strand::save_all(ostream & out, int indent) {
    int i;

    for (i=0; i<indent; i++) {
        out << "\t";
    }

    out << "STRAND={\n";
    region.save(out, (indent+1), root);
    helix_info->save(out, (indent+1));
    other_strand->loop->set_segment(other_strand->next_segment);  //set pointer of following loop to first segment after the strand
    other_strand->loop->save(out, other_strand, (indent+1));
    other_strand->save_core(out, (indent+1));     //save information about second strand and return to calling method

    for (i=0; i<indent; i++) {
        out << "\t";
    }
    out << "}\n";    //close brackets for block STRAND
}


void SEC_helix_strand::save_core(ostream & out, int indent) {
    region.save(out, indent, root);
}


/********************************
 *Konstruktoren
 ********************************/

SEC_segment::SEC_segment(SEC_root *root_, double alpha_, SEC_helix_strand *next_helix_strand_, SEC_loop *loop_)
{
    next_helix_strand = next_helix_strand_;
    root = root_;
    loop = loop_;
    alpha = alpha_;
    x_seg = 0;
    y_seg = 0;
}


SEC_helix_strand::SEC_helix_strand(SEC_root *root_, SEC_loop *loop_, SEC_helix_strand *other_strand_, SEC_helix *helix_info_, SEC_segment *next_segment_, int start, int end):
    region(start, end)
{
    loop         = loop_;
    root         = root_;
    other_strand = other_strand_;
    helix_info   = helix_info_;
    next_segment = next_segment_;
    
    start_angle     = 0;
    old_delta       = 0;
    fixpoint_x      = 0;
    fixpoint_y      = 0;
    attachp1_x      = 0;
    attachp1_y      = 0;
    attachp2_x      = 0;
    attachp2_y      = 0;
    start_angle     = 0;
    old_delta       = 0;
    thisBaseColor   = 0;
    otherBaseColor  = 0;
    thisLastAbsPos  = 0;
    otherLastAbsPos = 0;
    thisLast_x      = 0;
    thisLast_y      = 0;
    otherLast_x     = 0;
    otherLast_y     = 0;

    is_rootside_fixpoint = false;
}


SEC_region::SEC_region(int start, int end) {
    sequence_start = start;
    sequence_end = end;
    abspos_array = NULL;
}


SEC_loop::SEC_loop(SEC_root *root_, SEC_segment *segment_, double max_radius_, double min_radius_) {
    segment = segment_;
    root = root_;
    radius = 0;
    umfang = 0;
    x_loop = 0;
    y_loop = 0;
    max_radius = max_radius_;
    min_radius = min_radius_;
}


SEC_helix::SEC_helix(double delta_, double deltaIn_, double max_length_, double min_length_)
{
    delta = delta_;
    deltaIn = deltaIn_;
    max_length = max_length_;
    min_length = min_length_;
    length = 0;
}


SEC_root::SEC_root(SEC_segment *root_segment_, int max_index_, double distance_between_strands_, double skeleton_thickness_)
{
    memset ((char *)this, 0, sizeof(*this));
    max_index = max_index_;
    root_segment = root_segment_;
    set_distance_between_strands(distance_between_strands_);
    set_skeleton_thickness(skeleton_thickness_);
    set_show_debug(false);
    set_show_helixNrs(true);
    set_show_strSkeleton(true);
    set_hide_bases(false);
    set_hide_bonds(false);
    set_display_sai(false);
    helix_filter = (-1);
    segment_filter = (-1);
    loop_filter = (-1);
    cursor = 0;
    show_constraints = 0;
    fresh_sequence = 1;

    rotateBranchesMode = 0;
    rootAngle = 0;
    seqTerminal = 0;

    drag_recursive = 0;
    number_found = NULL;
    x_string = NULL;
    x_string_len = 0;
    number = 0;
    ap = NULL;
    ap_length = 0;

    gb_template = NULL;
    template_length = 0;		// length of template string
    template_sequence = NULL;

    gb_sequence = NULL;
    sequence_length = 0;		// length of string
    sequence = NULL;

    helix = NULL;
    ecoli = NULL;
}


/********************
 *Destruktoren
 ********************/

SEC_segment::~SEC_segment() {
    //Only by deleting loops segments can be erased, but loops delete the
    //strands before they delete their segments. Therefore the pointers of the
    //strands to the segments do not have to be removed.
    //delete_pointer_to_me();
    loop = NULL;
    root = NULL;
}


SEC_helix_strand::~SEC_helix_strand() {
    if(next_segment != NULL) {
        next_segment->delete_pointer_2(this);
    }

    delete helix_info;

    if (other_strand != NULL) {
        if (other_strand->loop) {
            other_strand->loop->set_segment(next_segment); //now every loop-element can be reached by loop-destructor
        }
        
        if (other_strand->next_segment != NULL) {
            other_strand->next_segment->delete_pointer_2(other_strand);
        }
        other_strand->helix_info = NULL;
        other_strand->other_strand = NULL;
        other_strand->next_segment = NULL;
        delete other_strand;
    }

    root = NULL;
    delete loop;
}


SEC_region::~SEC_region() {
}


SEC_loop::~SEC_loop() {
    if (segment != NULL) {
        root = NULL;
        //save the segments of the loop in an array
        SEC_segment *segment_buffer[BUFFERLENGTH];
        SEC_segment *segment_pointer = segment;
        SEC_helix_strand *strand_pointer;
        int i = 0;
        do {
            segment_buffer[i++] = segment_pointer;
            strand_pointer = segment_pointer->get_next_helix();
            if (strand_pointer != NULL) {
                segment_pointer = strand_pointer->get_next_segment();
            }
            else {
                segment_pointer = segment;    //to break the while-loop
            }
        }
        while (segment_pointer != segment);

        //delete all strands connected to loop
        int j;
        for (j=0; j<i; j++) {
            strand_pointer = segment_buffer[j]->get_next_helix();
            if (strand_pointer != NULL) {
                strand_pointer->set_loop(NULL);
                delete strand_pointer;
            }
        }

        //delete alle segments connected to loop
        segment = NULL;
        for (j=0; j<i; j++) {
            if (segment_buffer[j] != NULL) {
                segment_buffer[j]->set_loop(NULL);
                delete segment_buffer[j];
            }
        }
    }
}





SEC_helix::~SEC_helix() {
}


SEC_root::~SEC_root() {
    delete (root_segment->get_loop());
}
