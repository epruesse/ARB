#include <cstdio>
#include <climits>
#include <cstdlib>
#include <cmath>

#include <iostream>
#include <sstream>

#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt_canvas.hxx>
#include <BI_helix.hxx>

#include "secedit.hxx"
#include "sec_graphic.hxx"
#include "../EDIT4/ed4_class.hxx"
#include "../EDIT4/ed4_defs.hxx"  //for background colors
#include "../EDIT4/ed4_visualizeSAI.hxx"

using namespace std;

#define COLORLINK (ED4_G_SBACK_0 - SEC_GC_SBACK_0)  // to link to the colors defined in primary editor ed4_defs.hxx
#define SAICOLORS (ED4_G_CBACK_0 - SEC_GC_CBACK_0)

/*-----------------------Inline Functions -------------------------------------------------------------------------------*/

inline AW_pos transform_size(AW_device *device, AW_pos size) {
    AW_pos scale = device->get_scale();
    return size/scale;
}

inline int cursor_is_between(int abs_pos, int cursor, int last_abs_pos) {
    if (cursor==abs_pos) {
        return 1;
    }

    if (last_abs_pos<0) return 0; // first position of segment/strand

    if (last_abs_pos<abs_pos) {
        if (last_abs_pos<cursor && cursor<abs_pos) {
            return 1;
        }
    }
    else if (abs_pos<last_abs_pos && (last_abs_pos-abs_pos>5000 /* yes it's a hack ;-) */)) {
        // last_abs_pos is short before end of sequence, abs_pos is short after start of sequence

        if (cursor<abs_pos) {
            return 1;
        }
    }

    return 0;
}

/*------------------------- Validates the search colors selected in secondary structure window---------------------------------*/

static inline int validSearchColor(int posColor, bool showSai){
    if (showSai) {
        if (posColor>=SEC_GC_CBACK_0 && posColor<SEC_GC_MAX)  return 1;
        else                                                  return 0;
    }
    else {
        if (posColor>=SEC_GC_SBACK_0 && posColor<SEC_GC_MAX)  return 1;
        else                                                  return 0;
    }
}

/*------------------ Gets the search pattern results for the probes selected in primary editor ---------------------------------*/
/*-------------------include the header files of ED4_sequence_terminal::results function-----------------------------------------*/

const char *SEC_root::getSearchResults(int startPos,int endPos){                                     // this function is defined in SEC_root
    //seqTerminal is a pointer to ED4_sequence_terminal declared in SEC_root

    if (seqTerminal) {                                                                        //if seqTerminal is initialised and selected
        const char *searchColResults = 0;

        if (display_sai && ED4_ROOT->visualizeSAI) {
            searchColResults = getSaiColorString(SEC_GRAPHIC->aw_root, startPos, endPos); // returns 0 if sth went wrong
        }
        if (!searchColResults) {
            searchColResults = seqTerminal->results().buildColorString(seqTerminal, startPos, endPos); // buildColorString builds the background color of each base
        }

        return searchColResults;                                                                       // returning the color strings
    }
    else return 0;                                                                                   //if not return 0
}

/*-------------------------Computing methods-------------------------------------------------------------------------------*/

void SEC_region::count_bases(SEC_root *root) {

    base_count = 0;
    delete [] abspos_array;
    abspos_array = 0;
    static int *static_array = NULL;
    static int sizeof_static_array=0;

    int i;
    if (sequence_end < sequence_start) {        //if this is the "endings-segment"
        if (root->sequence == NULL) {
            base_count = (root->get_max_index() + 1) - sequence_start + sequence_end;
        }
        else {
            int max = sequence_end;
            if (max>root->sequence_length) max = root->sequence_length;
            if (max>root->template_length) max = root->template_length;

            int size = (root->get_max_index() + 1) - sequence_start + sequence_end;
            if (size > sizeof_static_array) {
                delete [] static_array;
                static_array = new int[size];
                sizeof_static_array = size;
            }

            for (i=sequence_start; i<=(root->sequence_length-1); i++) {
                if (i<root->sequence_length) {
                    if (root->sequence[i] != '-' && root->sequence[i] != '.') {
                        goto take_pos;
                    }
                }
                if (i<root->template_length) {
                    if (root->template_sequence[i] != '-' && root->template_sequence[i] != '.') {
                        goto take_pos;
                    }
                }
                else {
                    goto take_pos;
                }
                continue;

            take_pos:
                static_array[base_count++] = i;
            }

            for (i=0; i<sequence_end; i++) {
                if (i<root->sequence_length) {
                    if (root->sequence[i] != '-' && root->sequence[i] != '.') {
                        goto take_pos2;
                    }
                }
                if (i<root->template_length) {
                    if (root->template_sequence[i] != '-' && root->template_sequence[i] != '.') {
                        goto take_pos2;
                    }
                }
                else {
                    goto take_pos2;
                }
                continue;

            take_pos2:
                static_array[base_count++] = i;
            }

            abspos_array = new int[base_count];
            for (i=base_count-1; i>=0; i--) {
                abspos_array[i] = static_array[i];
            }

        }
    }
    else {
        if (root->sequence == NULL) {
            base_count = sequence_end - sequence_start;
        }
        else {
            int max = sequence_end;
            if (max>root->sequence_length) max = root->sequence_length;
            if (max>root->template_length) max = root->template_length;

            int size = sequence_end - sequence_start;
            if (size > sizeof_static_array) {
                delete [] static_array;
                static_array = new int[size];
                sizeof_static_array = size;
            }

            for (i=sequence_start; i<sequence_end; i++) {
                if (i<root->sequence_length) {
                    if (root->sequence[i] != '-' && root->sequence[i] != '.') {
                        goto take_pos3;
                    }
                }
                if (i<root->template_length) {
                    if (root->template_sequence[i] != '-' && root->template_sequence[i] != '.') {
                        goto take_pos3;
                    }
                }
                else {
                    goto take_pos3;
                }
                continue;

            take_pos3:
                static_array[base_count++] = i;
            }
            abspos_array = new int[base_count];
            for (i=base_count-1; i>=0; i--) {
                abspos_array[i] = static_array[i];
            }
        }
    }
}

void SEC_helix_strand::compute_length(void) {
    SEC_region *other_strands_region = other_strand->get_region();
    other_strands_region->count_bases(root); //update base_count of other_strand
    region.count_bases(root);                //update own base_count

    region.align_helix_strands(root,other_strands_region);
    other_strands_region->align_helix_strands(root,&region);

    double length = region.get_base_count();
    if (length < (other_strands_region->get_base_count())) {
        length = other_strands_region->get_base_count();
    }

    double max_length = helix_info->get_max_length();
    double min_length = helix_info->get_min_length();

    if (max_length && length>max_length) length = max_length;
    if (min_length && length<min_length) length = min_length;

    helix_info->set_length(length);
}

void SEC_helix_strand::compute_coordinates(double distance, double *x_strand, double *y_strand, double previous_x, double previous_y) {
    double delta = helix_info->get_delta();
    *x_strand = previous_x + (cos(delta) * distance);
    *y_strand = previous_y + (sin(delta) * distance);
}

void SEC_helix_strand::computeLoopCoordinates(double radius, double *x_loop, double *y_loop, double previous_x, double previous_y) {
    double delta = helix_info->get_delta();
    *x_loop = previous_x + (cos(delta) * radius);
    *y_loop = previous_y + (sin(delta) * radius);
}

int SEC_region::get_faked_basecount(void) {
    if (base_count < 10) return (base_count+1);
    else return(base_count);
}

void SEC_loop::compute_umfang(void) {  //Calculates the circumferance of the loop by counting the bases of the loop
    SEC_segment *segment_pointer = segment;
    SEC_helix_strand *strand_pointer;
    SEC_region *region_pointer;
    umfang = 0;

    do {
        region_pointer = segment_pointer->get_region();
        region_pointer->count_bases(root);
        umfang += region_pointer->get_faked_basecount();
        umfang += int(root->get_distance_between_strands());                  //to account for the following strand

        strand_pointer = segment_pointer->get_next_helix();
        segment_pointer = strand_pointer->get_next_segment();
    }
    while (segment_pointer != segment);
}

void SEC_loop::compute_radius(void) {
    compute_umfang();
    radius = umfang / (2 * M_PI);   //calculates the radius of the loop by using circumferance ( 2*PI*r  )
    if (max_radius && radius>max_radius) radius = max_radius;
    if (min_radius && radius<min_radius) radius = min_radius;
}

void SEC_loop::compute_segments_edge(double &attachp1_x, double &attachp1_y, double &attachp2_x, double &attachp2_y, SEC_segment *segment_pointer) {

    //test, if the center of the segment lies left or right of loop's center
    double attach_attach_v[2] = { (attachp1_x - attachp2_x), (attachp1_y-attachp2_y) };
    double attach_loopcenter_v[2] = { (attachp1_x-x_loop), (attachp1_y-y_loop) };

    double tmp = attach_attach_v[0];
    attach_attach_v[0] = -attach_attach_v[1];
    attach_attach_v[1] = tmp;

    double direction = (attach_attach_v[0]*attach_loopcenter_v[0]) + (attach_attach_v[1]*attach_loopcenter_v[1]);
    if (direction > 0) {
        segment_pointer->update_center_point(attachp1_x, attachp1_y, attachp2_x, attachp2_y);
    }
    else {
        segment_pointer->update_center_point(attachp2_x, attachp2_y, attachp1_x, attachp1_y);
    }
}

void SEC_region::align_helix_strands(SEC_root *root, SEC_region *other_region){

    if ( abspos_array == NULL) return;  // no sequence available
    /** Idea: there is a forward and a revers strand f */
    SEC_region *f = this;
    SEC_region *r = other_region;


    BI_helix *helix = root->helix;
    if (!helix) return;

    /** next index not used */
    //    int fnext = 0;
    //    int rnext = r->base_count-1;

    /** next pairing index !!*/

    int sum_bc = f->base_count + r->base_count;
    int *fdest_array = new int[sum_bc];

    /*** Destination write counter */
    int fdest = 0;  // pointer to next unused position


    int f_last = 0; // last index+1 which is already written
    int f_new;      // intermediate variable, used to find next real pair
    int p_f_last = r->base_count-1; // pairing version of f_last
    int p_f_new;    // pairing index of f_next



    for ( f_last = 0; f_last < f->base_count; f_last = f_new) {
        /** geg f_last, p_f_last
            search f_new, which is an f index which has a pairing neighbour on other side
            and p_f_new which is pairing neighbour index
        */
        p_f_new = p_f_last;
        for (f_new = f_last+1; f_new < f->base_count; f_new++){
            unsigned abs_pos = f->abspos_array[f_new];
            if (abs_pos > helix->size){ // end
                goto copy_last_bases;
            }
            if (helix->entries[abs_pos].pair_type == HELIX_NONE) continue;
            int pairing_pos = helix->entries[abs_pos].pair_pos;

            for ( p_f_new = p_f_last; p_f_new >=0; p_f_new --){
                if (r->abspos_array[p_f_new] < 0) continue;     // already a gap
                if ( r->abspos_array[p_f_new] <= pairing_pos) break;    // position less or equal found
            }
            if ( (p_f_new >= 0) && (r->abspos_array[p_f_new] == pairing_pos)) break; // real pair found
        }

        /** number of elements to copy*/
        int rdist = p_f_last - p_f_new; // revers
        int fdist = f_new - f_last;

        while (f_last < f_new){
            fdest_array[fdest++] = f->abspos_array[f_last++];
        }
        /** Fill in gaps where other side has more positions */
        while ( fdist < rdist){
            fdest_array[fdest++] = -1;
            fdist++;
        }

        p_f_last = p_f_new;
        f_last = f_new;
    }

 copy_last_bases:
    // copy the rest
    while (f_last < f->base_count){
        fdest_array[fdest++] = f->abspos_array[f_last++];
    }
    delete [] f->abspos_array;
    f->abspos_array = fdest_array;
    f->base_count = fdest;

}

void SEC_root::set_root(SEC_Base *base) {
    /***Selects the clicked base and gets the respective information if clicked on segment or loop  ******************/
    SEC_loop *loop;

    if (base->getType() == SEC_SEGMENT) {
        root_segment = (SEC_segment *) base;
        loop = root_segment->get_loop();
    }
    else if (base->getType() == SEC_LOOP) {
        loop = (SEC_loop *) base;
        root_segment = loop->get_segment();
    }
    else {
        aw_message("You cannot assign the \"root-attribute\" to helix-strands");
        return;
    }

    SEC_segment      *segment_pointer = loop->get_segment();
    SEC_helix_strand *strand_pointer  = segment_pointer->get_next_helix();

    double delta      = strand_pointer->get_helix_info()->get_delta();  //getting delta from the immediate strand found from the clicked pos.

    /*---------------- Going thru the entire loop and selects the last segment and strand of the loop  -----------*/
    while(strand_pointer->get_next_segment() != loop->get_segment()) {
        segment_pointer = strand_pointer->get_next_segment();
        strand_pointer  = segment_pointer->get_next_helix();
        delta           = strand_pointer->get_helix_info()->get_delta();
    }

    setRootAngle(delta);  //setting the angle of root

    loop->set_segment(segment_pointer);  //setting the segment pointer
}


/*---------------------------------------------Updating methods-------------------------------------------------------------*/

void SEC_helix_strand::update(double fixpoint_x_, double fixpoint_y_, double angle_difference) {

    //update this strands own fixpoints
    fixpoint_x = fixpoint_x_;
    fixpoint_y = fixpoint_y_;

    if (angle_difference != 0) {
        double new_angle = angle_difference + helix_info->get_delta();
        //the next "if" corrects negative angles that can occur during drag-operations
        //if not corrected they will be reset in SEC_loop::update(), because they are believed to be new strands
        //created by a split operation which are marked by negative angles
        if (new_angle < 0) {
            new_angle += (2*M_PI);
        }
        helix_info->set_delta(new_angle);
    }

    compute_length();  //compute length of both this strand and other_strand

    //update position of other_strand in following loop
    compute_coordinates(helix_info->get_length(), &(other_strand->fixpoint_x), &(other_strand->fixpoint_y), fixpoint_x, fixpoint_y);

    other_strand->loop->update(other_strand, angle_difference);
}

void SEC_helix_strand::compute_attachment_points(double direction_strand) {
    double dbs = root->get_distance_between_strands();
    //turn dir_delta about 90 degrees
    direction_strand += (M_PI / 2);  //to paint the strands parallel to each other the angle is turned by 90 deg.

    attachp1_x = fixpoint_x + cos(direction_strand)*(dbs / 2);
    attachp1_y = fixpoint_y + sin(direction_strand)*(dbs / 2);
    attachp2_x = fixpoint_x - cos(direction_strand)*(dbs / 2);
    attachp2_y = fixpoint_y - sin(direction_strand)*(dbs / 2);
}

void SEC_segment::update_center_point(double start_x, double start_y, double end_x, double end_y) {
    //compute point exactly between start and end points
    double p_x = (start_x + end_x) / 2;
    double p_y = (start_y + end_y) / 2;

    //compute vector v pointing from start to end point
    double v[2] = {(end_x - start_x), (end_y - start_y)};

    //turn v 90 degrees
    double tmp = v[0];
    v[0] = -v[1];
    v[1] = -(-1)*tmp;

    //set length of v to 1 (?normalize?)
    double length_of_v = sqrt((v[0]*v[0]) + (v[1]*v[1]));
    v[0] = v[0] / length_of_v;
    v[1] = v[1] / length_of_v;

    //compute distance between p and center-point q
    double distance_start_p = sqrt( ((p_x-start_x)*(p_x-start_x)) + ((p_y-start_y)*(p_y-start_y)) );
    double radius = loop->get_radius();

    //to ensure that the ends of a strand are always properly connected to the segment's ends we have to re-adjust the radius in certain situations
    if (length_of_v > (2*radius)) {
        radius = 0.5*length_of_v;
    }

    double temp = (radius*radius) - (distance_start_p*distance_start_p);
    if (temp < 0) {   //to correct if the numbers are not exact enough
        temp = -temp;
    }
    double distance_p_q = sqrt( temp );

    //compute center point of segment
    x_seg = p_x + distance_p_q*v[0];
    y_seg = p_y + distance_p_q*v[1];
}

void SEC_segment::update_alpha(void) {  //alpha is an angle of segment
    region.count_bases(root);
    int base_count = region.get_faked_basecount();
    alpha = ( (double) base_count / loop->get_umfang() ) * (2*M_PI);
}

void SEC_loop::test_angle(double &strand_angle, double &gamma, SEC_helix *helix_info, double &angle_difference) {

    //now we will test, if delta points away from the current loop, or to it. If it points to this loop, then it will be mirrored
    double delta_direction = ( (cos(gamma) * cos(strand_angle)) + (sin(gamma) * sin(strand_angle)) );
    if (delta_direction < 0) { //points to the current loop if true
        if(!root->rotateBranchesMode) strand_angle +=M_PI;  //mirrors the angle of strands preventing overlapping of the strands to its loop
        helix_info->set_delta(strand_angle-angle_difference);
    }
}

void SEC_loop::update_caller(double &gamma, double &strand_angle, SEC_helix *helix_info, double &angle_difference, SEC_helix_strand *strand_pointer) {
    double delta_direction = ( (cos(gamma) * cos(strand_angle)) + (sin(gamma) * sin(strand_angle)) );
    if (delta_direction < 0) {
        strand_angle+= M_PI;
        helix_info->set_delta(strand_angle-angle_difference);
    }
    double next_x = x_loop + cos(gamma)*radius;
    double next_y = y_loop + sin(gamma)*radius;

    strand_pointer->update(next_x, next_y, angle_difference);
    strand_pointer->compute_attachment_points(strand_angle);

    //root loop's caller has to point to root loop, as all callers do
    helix_info->set_delta(strand_angle);
    helix_info->set_deltaIn(strand_angle + M_PI);
}

void SEC_loop::update(SEC_helix_strand *caller, double angle_difference) {

    int is_root = 0;
    if (caller == NULL) {
        is_root = 1;
        caller = segment->get_next_helix();
    }

    compute_radius();
    if (!is_root) {
        //compute center-point of loop
        caller->computeLoopCoordinates(radius, &x_loop, &y_loop, caller->get_fixpoint_x(), caller->get_fixpoint_y());
    }

    SEC_helix *helix_info = caller->get_helix_info();  //getting info from SEC_helix

    double gamma  = helix_info->get_delta(); //getting delta from SEC_helix class

    if(is_root) gamma = root->getRootAngle();

    double delta_direction;
    //turn around gamma, the caller is pointing to this loop, not away from it
    gamma += M_PI;   // opposite direction of delta

    double dbs                   = root->get_distance_between_strands();
    double angle_between_strands = ( dbs / umfang) * (2*M_PI);  //angle between two strands

    SEC_segment      *segment_pointer         = caller->get_next_segment();
    SEC_helix_strand *strand_pointer          = segment_pointer->get_next_helix();
    SEC_helix_strand *previous_strand_pointer = caller;

    double previous_strand_angle = gamma;                        //previous_strand_angl`e is angle of caller already in the right direction
    double strand_angle          = 0.0 ;

    double next_strand_fxpt_x, next_strand_fxpt_y;
    double previous_fxpt_x = caller->get_fixpoint_x();
    double previous_fxpt_y = caller->get_fixpoint_y();
    double attachp1_x, attachp1_y, attachp2_x, attachp2_y;

    //compute fixpoints of caller if we are in the root loop
    if (is_root) {
        previous_fxpt_x = x_loop + cos(gamma)*radius;
        previous_fxpt_y = y_loop + sin(gamma)*radius;
        caller->set_fixpoint_x(previous_fxpt_x);
        caller->set_fixpoint_y(previous_fxpt_y);
    }

    //compute attachment-points of segments for caller
    caller->compute_attachment_points(previous_strand_angle);

    while (strand_pointer != caller) {
        segment_pointer->update_alpha();
        gamma += (segment_pointer->get_alpha()) + angle_between_strands;
        next_strand_fxpt_x = x_loop + cos(gamma)*radius;
        next_strand_fxpt_y = y_loop + sin(gamma)*radius;

        //after a split-operation the new helix-strand connecting the new loops gets a negative delta
        //We will correct this by setting it to match the angle of the line from this loop's center to the new fixpoint of the strand,
        //which is obviously the gamma we just computed. Since gamma already contains the angle-correction "angle-difference" (it is related from
        //the corrected angle of caller-strand) we will use "gamma - angle_difference", angle_difference will be added later again in the strand's
        //update-method.
        helix_info = strand_pointer->get_helix_info();
        if (helix_info->get_delta() < 0) {
            helix_info->set_delta(gamma-angle_difference);
        }

        strand_angle = helix_info->get_delta() + angle_difference; //getting delta (strand angle) of the subsequent helix

        //now we will test, if delta points away from the current loop, or to it. If it points to this loop, then it will be mirrored
        test_angle(strand_angle, gamma, helix_info, angle_difference);

        strand_pointer->update(next_strand_fxpt_x, next_strand_fxpt_y, angle_difference);

        strand_pointer->compute_attachment_points(strand_angle);

        helix_info->set_deltaIn(strand_angle+M_PI);

        attachp1_x = previous_strand_pointer->get_attachp1_x();
        attachp1_y = previous_strand_pointer->get_attachp1_y();
        attachp2_x = strand_pointer->get_attachp2_x();
        attachp2_y = strand_pointer->get_attachp2_y();

        //compute edge of sector built by segment
        compute_segments_edge(attachp1_x, attachp1_y, attachp2_x, attachp2_y, segment_pointer);

        //prepare next "while-loop-cycle"
        previous_strand_angle   = strand_angle;
        previous_fxpt_x         = next_strand_fxpt_x;
        previous_fxpt_y         = next_strand_fxpt_y;
        previous_strand_pointer = strand_pointer;
        segment_pointer         = strand_pointer->get_next_segment();
        strand_pointer          = segment_pointer->get_next_helix();
    }

    next_strand_fxpt_x = caller->get_fixpoint_x();
    next_strand_fxpt_y = caller->get_fixpoint_y();

    segment_pointer->update_alpha();
    helix_info   = strand_pointer->get_helix_info();
    strand_angle = helix_info->get_delta();

    //if in root-loop we have to update the elements following the "caller"-strand and the caller's fixpoint
    if (is_root) {
        gamma += (segment_pointer->get_alpha()) + angle_between_strands;
        strand_angle += angle_difference;   //if we are in root loop, then the caller is not corrected yet
        update_caller(gamma, strand_angle, helix_info, angle_difference, strand_pointer);
    }

    //update last segment pointing to caller

    //now we will test, if delta points away from the current loop, or to it. If it points to this loop, then it will be mirrored
    delta_direction = ( (cos(gamma) * cos(strand_angle)) + (sin(gamma) * sin(strand_angle)) );
    if (delta_direction < 0) {
        strand_angle += 2*M_PI;
    }
    attachp1_x = previous_strand_pointer->get_attachp1_x();
    attachp1_y = previous_strand_pointer->get_attachp1_y();
    attachp2_x = strand_pointer->get_attachp2_x();
    attachp2_y = strand_pointer->get_attachp2_y();

    if (segment_pointer->get_alpha() >= M_PI) {
        //in this case the first point becomes actually the second, so we have to switch the computed points,
        //otherwise the vector computed in update_center_point, would point towards the opposite direction, causing a wrong center point for the segment
        segment_pointer->update_center_point(attachp2_x, attachp2_y, attachp1_x, attachp1_y);
    }
    else {
        segment_pointer->update_center_point(attachp1_x, attachp1_y, attachp2_x, attachp2_y);
    }
}

void SEC_root::update(double angle_difference) {
    if (!root_segment) return;

    SEC_loop *root_loop = root_segment->get_loop();
    if (fresh_sequence) {
        root_loop->set_x_y_loop(0,0);
        fresh_sequence = 0;
    }
    root_loop->compute_radius();
    root_loop->update(NULL, angle_difference);
}

/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/
/*-------------------------------------------  PAINTING ROUTINES/FUNCTIONS ---------------------------------------------------------*/
/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

/*-------------------------- Paints the background colors for the probes selected in primary editor ---------------------*/

void SEC_root::paintSearchBackground(AW_device *device, const char* searchCols, int absPos, double last_x, double last_y, double next_x, double next_y, double radius,double lineWidth,int otherStrand){

    int backColor     = 0;
    int nextBackColor = 0;
    int lastBackColor = 0;

    double lineCenter = radius*0.6;

    if (searchCols && searchCols[absPos] >= 0){
        if (display_sai) backColor = searchCols[absPos] - SAICOLORS;
        else             backColor = searchCols[absPos] - COLORLINK;

        if(!validSearchColor(backColor,display_sai)) return;

        device->set_line_attributes(backColor,lineWidth+3, AW_SOLID);

        if(next_x!=next_y){
            if(otherStrand){
                if (display_sai) lastBackColor = searchCols[absPos-1] - SAICOLORS;
                else             lastBackColor = searchCols[absPos-1] - COLORLINK;
                if(backColor==lastBackColor ) device->line(backColor, last_x, last_y-lineCenter, next_x, next_y-lineCenter, -1, 0, 0);
            }
            else {
                if (display_sai) nextBackColor = searchCols[absPos+1] - SAICOLORS;
                else             nextBackColor = searchCols[absPos+1] - COLORLINK;
                if(backColor==nextBackColor ) device->line(backColor, last_x, last_y-lineCenter, next_x, next_y-lineCenter, -1, 0, 0);
            }
        }
        device->circle(backColor, true, last_x, last_y-lineCenter, radius, radius, -1, 0, 0);
    }
}

/*-------------------------------Painting the search pattern strings with probe information ----------------------------------*/

void SEC_root::paintSearchPatternStrings(AW_device *device, int clickedPos, AW_pos xPos,  AW_pos yPos){
    int searchColor = 0;
    const char *searchPatternResults = getSearchResults(clickedPos, clickedPos+1);

    if(searchPatternResults && searchPatternResults[clickedPos]){
        searchColor = searchPatternResults[clickedPos] - COLORLINK;
        switch (searchColor){
        case SEC_GC_SBACK_0 :
            device->text(searchColor, "USER 1 ", xPos,yPos, 0, 1, 0, 0, 0);
            break;
        case SEC_GC_SBACK_1 :
            device->text(searchColor, "USER 2 ", xPos,yPos, 0, 1, 0, 0, 0);
            break;
        case SEC_GC_SBACK_2 :
            device->text(searchColor, "PROBE ", xPos,yPos, 0, 1, 0, 0, 0);
            break;
        case SEC_GC_SBACK_3 :
            device->text(searchColor, "PRIMER (LOCAL)", xPos,yPos, 0, 1, 0, 0, 0);
            break;
        case SEC_GC_SBACK_4 :
            device->text(searchColor, "PRIMER (REGION)", xPos,yPos, 0, 1, 0, 0, 0);
            break;
        case SEC_GC_SBACK_5 :
            device->text(searchColor, "PRIMER (GLOBAL)", xPos,yPos, 0, 1, 0, 0, 0);
            break;
        case SEC_GC_SBACK_6 :
            device->text(searchColor, "SIGNATURE (LOCAL)", xPos,yPos, 0, 1, 0, 0, 0);
            break;
        case SEC_GC_SBACK_7 :
            device->text(searchColor, "SIGNATURE (REGION)", xPos,yPos, 0, 1, 0, 0, 0);
            break;
        case SEC_GC_SBACK_8 :
            device->text(searchColor, "SIGNATURE (GLOBAL)", xPos,yPos, 0, 1, 0, 0, 0);
            break;
        default:
            cout<<"Please click on the probe "<<endl;
            break;
        }
    }
    else {
        cout<<"Please click on the probe "<<endl;
    }
}

/*---------------------------Paints debugging informaition----------------------------------------------------*/

void paintDebugInfo(AW_device *device, int color, double xPos, double yPos,const char *txt){
    device->circle(color, true, xPos, yPos, 0.2, 0.2, -1, 0, 0);
    device->text(SEC_GC_DEFAULT,txt, xPos, yPos, 0, 1, 0, 0, 0);
}

/*----------------------------Paints the box in the root loop selected---------------------------------------------------*/

void paint_box(SEC_root *root, SEC_Base *base, double x, double y, double radius, AW_device *device) {
    double dx = cos(M_PI / 4)*(radius);
    double dy = sin(M_PI / 4)*(radius);

    double x0 = x - dx;
    double y0 = y - dy;
    double x1 = x + dx;
    double y1 = y + dy;

    device->line(SEC_GC_DEFAULT, x0, y0, x1, y0, root->loop_filter, (AW_CL)base, 0 );
    device->line(SEC_GC_DEFAULT, x0, y0, x0, y1, root->loop_filter, (AW_CL)base, 0 );
    device->line(SEC_GC_DEFAULT, x1, y1, x0, y1, root->loop_filter, (AW_CL)base, 0 );
    device->line(SEC_GC_DEFAULT, x1, y1, x1, y0, root->loop_filter, (AW_CL)base, 0 );
}

/*------------------------------------Paints Ecoli positions---------------------------------------------------------*/

void SEC_segment::print_ecoli_pos(long ecoli_pos, double base_x, double base_y, AW_device *device) {
    //    double radius = loop->get_radius();
    double center_base_v[2] = { (base_x-x_seg), (base_y-y_seg) };
    double length_of_vector = sqrt( (center_base_v[0]*center_base_v[0]) + (center_base_v[1]*center_base_v[1]) );
    double scalar = ( length_of_vector + 3 ) / length_of_vector;
    center_base_v[0] = center_base_v[0] * scalar;
    center_base_v[1] = center_base_v[1] * scalar;

    double print_pos_x = x_seg + center_base_v[0];
    double print_pos_y = y_seg + center_base_v[1];

    char buffer[80];
    sprintf(buffer, "%i", int(ecoli_pos));
    device->text(SEC_GC_ECOLI, buffer, print_pos_x, print_pos_y, 0.5, root->segment_filter, (AW_CL)((SEC_Base *)this), 0 );
}

void SEC_helix_strand::print_ecoli_pos(long ecoli_pos, double attachpA_x, double attachpA_y, double attachpB_x, double attachpB_y, double base_x, double base_y, AW_device *device) {
    double start_base_v[2] = {(attachpB_x-attachpA_x), (attachpB_y-attachpA_y) };

    double tmp = start_base_v[0];
    start_base_v[0] = -start_base_v[1];
    start_base_v[1] = tmp;

    double length_of_v = sqrt( (start_base_v[0]*start_base_v[0]) + (start_base_v[1]*start_base_v[1]) );
    start_base_v[0] = start_base_v[0]/length_of_v;
    start_base_v[1] = start_base_v[1]/length_of_v;

    double print_pos_x = base_x - start_base_v[0]*3;
    double print_pos_y = base_y - start_base_v[1]*3;

    char buffer[80];
    sprintf(buffer, "%i", int(ecoli_pos));
    device->text(SEC_GC_ECOLI, buffer, print_pos_x, print_pos_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), 0 );
}

//--------------------------------- Paints helix numbers -----------------------------------------------------------

void SEC_helix_strand::printHelixNumbers(AW_device *device, double helixStart_x, double helixStart_y, double helixEnd_x, double helixEnd_y, double base_x, double base_y, int absPos){

    if(!root->show_helixNrs) return;

    double start_base_v[2] = {(helixEnd_x-helixStart_x), (helixEnd_y-helixStart_y) };

    double tmp = start_base_v[0];
    start_base_v[0] = -start_base_v[1];
    start_base_v[1] = tmp;

    double length_of_v = sqrt( (start_base_v[0]*start_base_v[0]) + (start_base_v[1]*start_base_v[1]) );
    start_base_v[0] = start_base_v[0]/length_of_v;
    start_base_v[1] = start_base_v[1]/length_of_v;

    double printPos_x = base_x - start_base_v[0]*2;
    double printPos_y = base_y - start_base_v[1]*2;
    char *helixNumber = root->helix->entries[absPos].helix_nr;

    if(helixNumber != NULL && (strchr(helixNumber,'-') == 0)){ // paints helix numbers on starting strand of helix
        device->text(SEC_GC_HELIX_NO, helixNumber, printPos_x, printPos_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), 0 );
    }
}

/*---------------------------------Paints HELICES----------------------------------------------------------------------------*/

void SEC_helix_strand::paint_strands(AW_device *device, double *v, double &length_of_v){
    char this_buffer[]       = "?";
    char other_buffer[]      = "?";
    SEC_region& other_region = other_strand->region;

    int this_base_count  = region.get_base_count();
    int other_base_count = other_region.get_base_count();
    int max_base_count   = max(this_base_count, other_base_count);

    double this_db  = length_of_v/(this_base_count-1); // compute the distance between the bases of this strand
    double other_db = length_of_v/(other_base_count-1); // compute the distance between the bases of this strand

    const AW_font_group& font_group   = root->get_font_group();
    double               font_height2 = transform_size(device, font_group.get_ascent(SEC_GC_HELIX)) / 2.0; // half ascent-size of used font

    device->set_line_attributes(SEC_SKELE_HELIX,root->get_skeleton_thickness(), AW_SOLID);  //setting the line attributes
    double font_size;

    {
        double h = transform_size(device, font_group.get_height(SEC_GC_HELIX));
        double w = transform_size(device, font_group.get_width(SEC_GC_HELIX));

        font_size = h>w ? h : w;
    }
    double this_other_off_x = 0; // offset between this_x and other_x
    double this_other_off_y = 0; // (same for y)

    int seqStart      = region.get_sequence_start();
    int seqEnd        = region.get_sequence_end();
    int otherSeqStart = other_region.get_sequence_start();
    int otherSeqEnd   = other_region.get_sequence_end();

    double radius     = transform_size(device, font_group.get_ascent(SEC_GC_HELIX)) * 0.75;
    double lineWidth  = font_group.get_max_width();

    const char *otherBgColor, *thisBgColor;

    char thisBgColor_buf[seqEnd-seqStart+1];
    {
        const char *thisBgColor_tmp = root->getSearchResults(seqStart, seqEnd);
        if (thisBgColor_tmp) {
            memcpy(thisBgColor_buf,thisBgColor_tmp+seqStart,seqEnd-seqStart+1);
            thisBgColor=thisBgColor_buf-seqStart;
        }
        else {
            thisBgColor=0;
        }
    }

    otherBgColor = root->getSearchResults(otherSeqStart, otherSeqEnd);

    thisBase[1] = otherBase[1] = 0;
    int paintLastBase   = 0;
    int thisHelixNrTag  = 0;
    int otherHelixNrTag = 0;
    int thisLonelyBaseClrTag =(0),otherLonelyBaseClrTag =(0);

    for (int i=0,j=(max_base_count-1); i<max_base_count; i++,j--) {
        int this_abs_pos;
        int other_abs_pos;

        if (region.abspos_array) this_abs_pos = region.abspos_array[i];
        else                 this_abs_pos = i+region.get_sequence_start();

        if (other_region.abspos_array) other_abs_pos = other_region.abspos_array[j];
        else                   other_abs_pos = other_region.get_sequence_end()-1-j;

        int this_legal = this_abs_pos>=0 && i<this_base_count;
        int other_legal = other_abs_pos>=0 && j<other_base_count;


        double this_x  = attachp2_x + v[0]*this_db*i;
        double this_y  = attachp2_y + v[1]*this_db*i;
        double other_x;
        double other_y;

        if (i==0) { // first loop -> calc position absolute
            other_x = attachp1_x + v[0]*other_db*i;
            other_y = attachp1_y + v[1]*other_db*i;
            this_other_off_x = other_x-this_x; // calc relative offset this <-> other
            this_other_off_y = other_y-this_y;
        }
        else { // following loops -> calc position relative
            // this is done to have constant distances between opposite bases in one strand
            other_x = this_x+this_other_off_x;
            other_y = this_y+this_other_off_y;
        }

        this_buffer[0]  = (this_abs_pos>=0  && this_abs_pos<root->sequence_length)  ? root->sequence[this_abs_pos]  : '.';
        other_buffer[0] = (other_abs_pos>=0 && other_abs_pos<root->sequence_length) ? root->sequence[other_abs_pos] : '.';

        if(i==0){
            thisBaseColor  = SEC_GC_HELIX; thisLastAbsPos  = this_abs_pos;  thisBase[0]  = this_buffer[0];  thisLast_x  = this_x;  thisLast_y  = this_y;
            otherBaseColor = SEC_GC_HELIX; otherLastAbsPos = other_abs_pos; otherBase[0] = other_buffer[0]; otherLast_x = other_x; otherLast_y = other_y;
        }

        int thisValid  = thisLastAbsPos>=0  && thisLastAbsPos<this_abs_pos   && thisLastAbsPos<root->sequence_length  && i>0;
        int otherValid = otherLastAbsPos>=0 && otherLastAbsPos>other_abs_pos && otherLastAbsPos<root->sequence_length && i>0;

        // paint search background and draw base characters - always paints the last base and the search background from last base to current base postion

        if (this_legal && i>0) {
            if (root->helix && root->helix->entries[this_abs_pos].pair_type==HELIX_NONE) {
                print_lonely_bases(this_buffer,  device, attachp2_x, attachp2_y, other_strand->attachp1_x, other_strand->attachp1_y,
                                   this_x,  this_y, this_abs_pos,  font_height2,thisBgColor,1);
                thisLonelyBaseClrTag =1;
            }
            else {
                if(thisLonelyBaseClrTag && root->show_strSkeleton) {
                    device->line(SEC_SKELE_NHELIX, thisLast_x, thisLast_y-font_height2, this_x, this_y-font_height2, -1, 0, 0);
                    thisLonelyBaseClrTag = 0;
                }
                else {
                    if(root->show_strSkeleton) device->line(SEC_SKELE_HELIX, thisLast_x, thisLast_y-font_height2, this_x, this_y-font_height2, -1, 0, 0);
                }
                if(thisValid) root->paintSearchBackground(device, thisBgColor, thisLastAbsPos, thisLast_x, thisLast_y, this_x, this_y, radius,lineWidth,0);
                if(!root->hide_bases)  device->text(thisBaseColor, thisBase, thisLast_x, thisLast_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this),thisLastAbsPos, 0 );
                root->announce_base_position(thisLastAbsPos, thisLast_x, thisLast_y-font_height2);
                //paints the Helix Numbers
                if(!thisHelixNrTag && this_base_count >= 2 && i%(this_base_count/2) == 0)
                    {
                        printHelixNumbers(device, attachp2_x, attachp2_y, other_strand->attachp1_x, other_strand->attachp1_y, (this_x+thisLast_x)/2, (this_y+thisLast_y)/2,this_abs_pos);
                        thisHelixNrTag = 1;
                    }
                thisBaseColor = SEC_GC_HELIX; thisLast_x = this_x; thisLast_y = this_y; thisBase[0] = this_buffer[0]; thisLastAbsPos = this_abs_pos;
            }
        }

        if (other_legal && i>0) {
            if (root->helix && root->helix->entries[other_abs_pos].pair_type==HELIX_NONE) {
                print_lonely_bases(other_buffer, device, other_strand->attachp2_x, other_strand->attachp2_y, attachp1_x, attachp1_y,
                                   other_x, other_y, other_abs_pos, font_height2,otherBgColor,0);
                otherLonelyBaseClrTag = 1;
            }
            else {
                if(otherLonelyBaseClrTag  && root->show_strSkeleton){
                    device->line(SEC_SKELE_NHELIX, otherLast_x, otherLast_y-font_height2, other_x, other_y-font_height2, -1, 0, 0);
                    otherLonelyBaseClrTag = 0;
                }
                else {
                    if(root->show_strSkeleton) device->line(SEC_SKELE_HELIX, otherLast_x, otherLast_y-font_height2, other_x, other_y-font_height2, -1, 0, 0);
                }
                if(otherValid) root->paintSearchBackground(device, otherBgColor, otherLastAbsPos,otherLast_x, otherLast_y, other_x, other_y, radius,lineWidth,1);
                if(!root->hide_bases) device->text(otherBaseColor, otherBase, otherLast_x, otherLast_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), otherLastAbsPos,0 );
                root->announce_base_position(otherLastAbsPos, otherLast_x, otherLast_y-font_height2);
                //paints the Helix Numbers
                if(!otherHelixNrTag &&
                   other_base_count >= 2 && // to avoid "divide by zero"
                   j%(other_base_count/2) == 0)
                    {
                        printHelixNumbers(device, other_strand->attachp2_x, other_strand->attachp2_y, attachp1_x, attachp1_y, (other_x+otherLast_x)/2, (other_y+otherLast_y)/2,other_abs_pos);
                        otherHelixNrTag = 1;
                    }
                otherBaseColor = SEC_GC_HELIX; otherLast_x=other_x; otherLast_y=other_y; otherBase[0]=other_buffer[0]; otherLastAbsPos=other_abs_pos;
            }
        }

        // draw bonds:
        if (this_legal && other_legal) {
            paintLastBase = 1; //if it is valid strand set to true for painting last base of each strand
            if(!root->hide_bonds)
                SEC_GRAPHIC->bond.paint(device, root, this_buffer[0], other_buffer[0], this_x, this_y-font_height2, other_x, other_y-font_height2, this_db, font_size);
        }

        // draw ecoli positions:
        if (root->ecoli) {
            long ecoli_pos, dummy;

            if (this_legal) {
                root->ecoli->abs_2_rel(this_abs_pos, ecoli_pos, dummy);
                if ((ecoli_pos%50)==0) {
                    print_ecoli_pos(ecoli_pos, attachp2_x, attachp2_y, other_strand->attachp1_x, other_strand->attachp1_y, this_x, this_y, device);
                }
            }
            if (other_legal) {
                root->ecoli->abs_2_rel(other_abs_pos, ecoli_pos, dummy);
                if ((ecoli_pos%50)==0) {
                    print_ecoli_pos(ecoli_pos, other_strand->attachp2_x, other_strand->attachp2_y, attachp1_x, attachp1_y, other_x, other_y, device);
                }
            }
        }
    }

    //this paints the last base of both the helix strands
    if(paintLastBase){
        root->paintSearchBackground(device, thisBgColor, thisLastAbsPos, thisLast_x, thisLast_y, 0, 0, radius,lineWidth,0);
        if(!root->hide_bases) device->text(thisBaseColor, thisBase, thisLast_x, thisLast_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this),thisLastAbsPos,0 );
        root->announce_base_position(thisLastAbsPos, thisLast_x, thisLast_y-font_height2);
        root->paintSearchBackground(device, otherBgColor, otherLastAbsPos, otherLast_x, otherLast_y, 0, 0, radius,lineWidth,1);
        if(!root->hide_bases) device->text(otherBaseColor, otherBase, otherLast_x, otherLast_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), otherLastAbsPos,0 );
        root->announce_base_position(otherLastAbsPos, otherLast_x, otherLast_y-font_height2);
        paintLastBase = 0;
    }
}

void SEC_helix_strand::print_lonely_bases(char *buffer, AW_device *device, double attachpA_x, double attachpA_y, double attachpB_x, double attachpB_y, double base_x, double base_y,
                                          int abs_pos, double half_font_height, const char *bgColor, int thisStrand) {
    double start_end_v[2] = {(attachpB_x-attachpA_x), (attachpB_y-attachpA_y) };

    double tmp = start_end_v[0];
    start_end_v[0] = -start_end_v[1];
    start_end_v[1] = tmp;

    double length_of_v = sqrt( (start_end_v[0]*start_end_v[0]) + (start_end_v[1]*start_end_v[1]) );
    start_end_v[0] = start_end_v[0]/length_of_v;
    start_end_v[1] = start_end_v[1]/length_of_v;

    double print_pos_x = base_x - start_end_v[0]*0.5;
    double print_pos_y = base_y - start_end_v[1]*0.5;

    const AW_font_group& font_group = root->get_font_group();
    double               radius     = transform_size(device,font_group.get_ascent(SEC_GC_HELIX))*0.75;

    double lineWidth = font_group.get_max_width();
    device->set_line_attributes(SEC_SKELE_NHELIX,root->get_skeleton_thickness(), AW_SOLID);  //setting the line attributes

    if(thisStrand){
        if(root->show_strSkeleton) device->line(SEC_SKELE_NHELIX, thisLast_x, thisLast_y-half_font_height, print_pos_x, print_pos_y-half_font_height, -1, 0, 0);
        root->paintSearchBackground(device, bgColor, thisLastAbsPos, thisLast_x, thisLast_y, print_pos_x, print_pos_y, radius,lineWidth,0);
        if(!root->hide_bases) device->text(thisBaseColor, thisBase, thisLast_x, thisLast_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), thisLastAbsPos,0 );
        root->announce_base_position(thisLastAbsPos, thisLast_x, thisLast_y-half_font_height);
        thisBaseColor=SEC_GC_NHELIX; thisLast_x=print_pos_x; thisLast_y=print_pos_y; thisBase[0]=buffer[0]; thisLastAbsPos=abs_pos;
    }
    else {
        if(root->show_strSkeleton) device->line(SEC_SKELE_NHELIX, otherLast_x, otherLast_y-half_font_height, print_pos_x, print_pos_y-half_font_height, -1, 0, 0);
        root->paintSearchBackground(device, bgColor, otherLastAbsPos, otherLast_x, otherLast_y, print_pos_x, print_pos_y, radius,lineWidth,1);
        if(!root->hide_bases) device->text(otherBaseColor, otherBase, otherLast_x, otherLast_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), otherLastAbsPos,0 );
        root->announce_base_position(otherLastAbsPos, otherLast_x, otherLast_y-half_font_height);
        otherBaseColor=SEC_GC_NHELIX; otherLast_x=print_pos_x; otherLast_y=print_pos_y; otherBase[0]=buffer[0]; otherLastAbsPos=abs_pos;
    }
}

/*---------------------------------Paints ROOT information----------------------------------------------------------------------------*/

void SEC_root::paint(AW_device *device) {
    if (!root_segment){
        device->line(SEC_GC_DEFAULT,0,0,100,100);
    }else{
        SEC_loop *root_loop = root_segment->get_loop();
        clear_base_positions(); // reset positions next to cursor
        for (int gc = SEC_GC_FIRST_FONT; gc <= SEC_GC_LAST_FONT; ++gc) {
            font_group.registerFont(device, gc);
        }


        paint_box(this,root_loop,root_loop->get_x_loop(), root_loop->get_y_loop(), (root_loop->get_radius()/3), device); //mark the root_loop
        root_loop->paint(NULL, device, show_constraints);

        if(show_debug)  paintDebugInfo(device,SEC_GC_CURSOR, root_loop->get_x_loop(), root_loop->get_y_loop(),"Loop_centre");

#if defined(DEBUG) && 0
        printf("-----------------------\n");
        printf("before_cursor pos=%i x=%f y=%f\n", before_cursor.pos, before_cursor.x, before_cursor.y);
        printf("after_cursor pos=%i x=%f y=%f\n", after_cursor.pos, after_cursor.x, after_cursor.y);
        printf("min_position pos=%i x=%f y=%f\n", min_position.pos, min_position.x, min_position.y);
        printf("max_position pos=%i x=%f y=%f\n", max_position.pos, max_position.x, max_position.y);
#endif

        // paint cursor:

        if ((before_cursor.pos!=-1 || min_position.pos!=-1) && (after_cursor.pos!=-1 || max_position.pos!=-1)) {
            SEC_base_position *pos1 = before_cursor.pos!=-1 ? &before_cursor : &max_position; // if no position found before cursor -> take last  position of sequence
            SEC_base_position *pos2 = after_cursor.pos !=-1 ? &after_cursor  : &min_position; // if no position found after  cursor -> take first position of sequence

#if defined(DEBUG) && 0
            printf("pos1 pos=%i x=%f y=%f\n", pos1->pos, pos1->x, pos1->y);
            printf("pos2 pos=%i x=%f y=%f\n", pos2->pos, pos2->x, pos2->y);
#endif

            AW_pos dx = (pos2->x - pos1->x)/2; // distance between drawed base positions
            AW_pos dy = (pos2->y - pos1->y)/2;
            AW_pos mx = (pos1->x + pos2->x)/2; // midpoint
            AW_pos my = (pos1->y + pos2->y)/2;
            AW_pos x1, y1, x2, y2;
            x1 = mx - dy/2;
            x2 = mx + dy/2;
            y1 = my + dx/2;
            y2 = my - dx/2;

            AW_pos X1, Y1, X2, Y2;
            device->transform(x1, y1, X1, Y1);
            device->transform(x2, y2, X2, Y2);
            double DX = fabs(X2-X1);
            double DY = fabs(Y2-Y1);
            double cursorlength = sqrt(DX*DX + DY*DY);

#if defined(DEBUG) && 0
            printf("DX = %f / DY = %f / cursorlength = %f\n", DX, DY, cursorlength);
#endif

            double resize = 15.0/cursorlength; // resize cursor to keep it visible
            dx *= resize;
            dy *= resize;
            x1 = mx - dy/2;
            x2 = mx + dy/2;
            y1 = my + dx/2;
            y2 = my - dx/2;

#if defined(DEBUG) && 0
            // draw a testline to see the baseline on that the cursor is positioned
            device->set_line_attributes(SEC_GC_CURSOR, 1, AW_SOLID);
            device->line(SEC_GC_DEFAULT, pos1->x, pos1->y, pos2->x, pos2->y);
#endif

            device->set_line_attributes(SEC_GC_CURSOR, 3, AW_SOLID);
            device->line(SEC_GC_CURSOR, x1, y1, x2, y2);
            set_last_drawed_cursor_position(x1, y1, x2, y2);
        }
    }
}

/*---------------------------------Paints SEGMENTS of the loop------------------------------------------------------------------------*/

void SEC_segment::prepare_paint(SEC_helix_strand *previous_strand_pointer, double &gamma, double &eta, double &radius, int &base_count, double &angle_step  ) {
    //get attachpoint_1 of previous strand
    double attachp1_x = previous_strand_pointer->get_attachp1_x();
    double attachp1_y = previous_strand_pointer->get_attachp1_y();

    //compute angle to begin with
    gamma = (2*M_PI) + atan2( (attachp1_y - y_seg), (attachp1_x - x_seg) );
    while (gamma > (2*M_PI)) {
        gamma -= (2*M_PI);
    }

    //compute angle to end with, should normally be unneccessary, because we have the angle alpha, which spans the segment. But it seems to be not exact enough
    double attachp2_x = next_helix_strand->get_attachp2_x();
    double attachp2_y = next_helix_strand->get_attachp2_y();

    eta = (2*M_PI) + atan2( (attachp2_y - y_seg), (attachp2_x - x_seg) );
    while (eta > (2*M_PI)) {
        eta -= (2*M_PI);
    }

    //to ensure that the ends of a strand are always properly connected to the segment's ends we have to re-adjust the radius in certain situations
    radius = loop->get_radius();
    double distance = sqrt( (attachp2_x-attachp1_x)*(attachp2_x-attachp1_x) + (attachp2_y-attachp1_y)*(attachp2_y-attachp1_y) );
    if (distance > (2*radius)) {
        radius = 0.5*distance;
    }

    //compute angle with which to step forward through the segment
    base_count = region.get_base_count();
    //     if (base_count == 0) {
    //  base_count = 1;
    //     }
    double dif_angle;
    if (eta >= gamma) dif_angle = (eta-gamma);
    else dif_angle = (2*M_PI) - (gamma-eta);
    angle_step = dif_angle / (base_count+1);
}

void SEC_segment::paint(AW_device *device, SEC_helix_strand *previous_strand_pointer) {
    double angle_step;
    double radius;
    double eta;
    double gamma;
    int    base_count;

    prepare_paint(previous_strand_pointer, gamma, eta, radius, base_count, angle_step);

    int    abs_pos;
    int    last_abs_pos = -1;
    double next_x;
    double next_y;
    int    i;
    long   ecoli_pos;
    long   dummy;

    const AW_font_group& font_group   = root->get_font_group();
    double               font_height2 = transform_size(device, font_group.get_ascent(SEC_GC_LOOP)) / 2.0;

    char buffer[2];
    buffer[1] = 0;

    double circleRadius = transform_size(device, font_group.get_ascent(SEC_GC_LOOP))*0.75;
    double lineWidth    = font_group.get_max_width();
    double nextCircle_x;
    double nextCircle_y;
    double nextGamma;

    double start_x; // to get the previous strand attachment points
    double start_y; 

    if(root->show_debug) {
        paintDebugInfo(device,SEC_GC_LOOP, start_x, start_y,"AP1");
        paintDebugInfo(device,SEC_GC_LOOP, previous_strand_pointer->get_attachp2_x(),previous_strand_pointer->get_attachp2_y(), "AP2");
        paintDebugInfo(device,SEC_GC_LOOP, previous_strand_pointer->get_fixpoint_x(), previous_strand_pointer->get_fixpoint_y(), "FP");
        paintDebugInfo(device,SEC_GC_LOOP, x_seg, y_seg,"Seg_centre");
    }

    int seqStart = region.get_sequence_start();
    int seqEnd   = region.get_sequence_end();

    int tempAbsPos = -1;
    const char *bgColor = 0;//, *bgColorLastSeg = 0;

    if(seqStart<seqEnd)  bgColor = root->getSearchResults(seqStart-1, seqEnd);
    else {
        int tempSeqEnd = root->get_max_index()+1;
        if(tempSeqEnd>(seqStart-1)) bgColor  = root->getSearchResults(seqStart-1, tempSeqEnd);
        //  bgColorLastSeg   = root->getSearchResults(1,seqEnd);
    }

    // To paint the last base of the previous helix strand and to paint the backgroud joining helix and segment if the search pattern found
    int lastSearchColor = 0;
    start_x = previous_strand_pointer->get_attachp1_x();
    start_y = previous_strand_pointer->get_attachp1_y();

    if(bgColor && bgColor[seqStart] >= 0 && bgColor[seqStart-1] >= 0){
        if(root->display_sai) lastSearchColor = bgColor[seqStart-1] - SAICOLORS;
        else                  lastSearchColor = bgColor[seqStart-1] - COLORLINK;
    }

    char helixStartBase[2], helixEndBase[2];
    helixStartBase[0] = helixStartBase[1] = 0;
    helixEndBase[0]   = helixEndBase[1]   = 0;

    for (i = 0; i<base_count; i++) {
        gamma += angle_step; nextGamma = gamma; nextGamma += angle_step;
        next_x = x_seg + cos(gamma)*radius; nextCircle_x = x_seg + cos(nextGamma)*radius;
        next_y = y_seg + sin(gamma)*radius; nextCircle_y = y_seg + sin(nextGamma)*radius;

        if (region.abspos_array) {
            abs_pos    = region.abspos_array[i];
        }
        else {
            abs_pos    = i + region.get_sequence_start();
        }

        if ((abs_pos < root->sequence_length)) {
            buffer[0] = root->sequence[abs_pos];
        }
        else {
            buffer[0] = '.';
        }

        if (abs_pos<root->sequence_length && i == 0) {
            for (tempAbsPos = abs_pos; tempAbsPos >= 0; tempAbsPos--) {
                if (root->sequence[tempAbsPos] != '-' && root->sequence[tempAbsPos] != '.') {
                    helixEndBase[0] = root->sequence[tempAbsPos]; //getting the last base of helix region
                    break;
                }
            }
        }

        device->set_line_attributes(SEC_SKELE_LOOP,root->get_skeleton_thickness(), AW_SOLID);  //setting the line attributes

        if(i==0  && root->show_strSkeleton)  device->line(SEC_SKELE_LOOP, start_x, start_y-font_height2,next_x, next_y-font_height2, -1, 0, 0);

        // this repaints the start base of the loop infact the end base of helix and paints the search background
        if(lastSearchColor){
            root->paintSearchBackground(device, bgColor, abs_pos,start_x,start_y,next_x, next_y, circleRadius,lineWidth,1);
            if (tempAbsPos != -1) {
                if (!root->hide_bases) device->text(SEC_GC_HELIX, helixEndBase, start_x, start_y, 0.5, root->helix_filter,(AW_CL)((SEC_Base *)previous_strand_pointer),tempAbsPos,0 );
                root->announce_base_position(tempAbsPos, start_x, start_y-font_height2);
            }
            lastSearchColor = 0;
        }

        // this paints the actual bases in the loop
        if(root->show_strSkeleton) device->line(SEC_SKELE_LOOP, next_x, next_y-font_height2, nextCircle_x, nextCircle_y-font_height2, -1, 0, 0);
        root->paintSearchBackground(device, bgColor, abs_pos,next_x, next_y, nextCircle_x, nextCircle_y, circleRadius,lineWidth,0);
        if(!root->hide_bases)   device->text(SEC_GC_LOOP, buffer, next_x, next_y, 0.5, root->segment_filter, (AW_CL)((SEC_Base *)this),abs_pos, 0 );
        root->announce_base_position(abs_pos, next_x, next_y-font_height2);

        if (root->ecoli != NULL) {
            root->ecoli->abs_2_rel(abs_pos, ecoli_pos, dummy);
            if ((ecoli_pos%50)==0) {
                print_ecoli_pos(ecoli_pos, next_x, next_y, device);
            }
        }
        last_abs_pos = abs_pos;

        if (abs_pos < root->sequence_length) helixStartBase[0] = root->sequence[last_abs_pos+1];
        else helixStartBase[0]=0;
    }

    //this paints the first base of helix if search pattern is found
    if(bgColor && bgColor[last_abs_pos+1] && helixStartBase[0]!='-'){
        if(!root->hide_bases)  device->text(SEC_GC_HELIX, helixStartBase, nextCircle_x, nextCircle_y, 0.5,root->helix_filter,(AW_CL)((SEC_Base *)previous_strand_pointer),(last_abs_pos+1),0 );
        root->announce_base_position((last_abs_pos+1), nextCircle_x, nextCircle_y-font_height2);
    }
}

/*---------------------------------Paints CONSTRAINTS----------------------------------------------------------------------------*/

void SEC_helix_strand::paint_constraints(AW_device *device, double *v, double &length_of_v) {
    char buffer[80];
    sprintf(buffer, "%.2f - %.2f", helix_info->get_min_length(), helix_info->get_max_length());

    //compute vector v_turn as vector v turned 90 degrees
    double v_turn[2] = { (v[1]*(root->get_distance_between_strands()/2)), ((-1)*v[0]*(root->get_distance_between_strands()/2))};

    double text_x = attachp1_x + v_turn[0] + v[0]*(length_of_v/2);
    double text_y = attachp1_y + v_turn[1] + v[1]*(length_of_v/2);

    device->text(SEC_GC_DEFAULT, buffer, text_x, text_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), 0);
}

void SEC_helix_strand::paint(AW_device *device, int show_constraints) {

    //compute vector v pointing from attachp1 to other_strand->attachp2
    double v[2] = { (other_strand->attachp2_x - attachp1_x),
                    (other_strand->attachp2_y - attachp1_y)
    };

    //normalize v
    double length_of_v = sqrt( (v[0]*v[0]) + (v[1]*v[1]) );
    v[0] = v[0] / length_of_v;
    v[1] = v[1] / length_of_v;

    if (show_constraints) {
        paint_constraints(device, v, length_of_v);
    }

    //     paint_other_strand(device, v, length_of_v);
    //     paint_this_strand(device, v, length_of_v);
    paint_strands(device, v, length_of_v);

    other_strand->loop->paint(other_strand, device, show_constraints);

    if(root->show_debug) {
        paintDebugInfo(device,SEC_GC_HELIX, other_strand->attachp1_x, other_strand->attachp1_y,"AP1");
        paintDebugInfo(device,SEC_GC_HELIX, other_strand->attachp2_x,other_strand->attachp2_y, "AP2");
        paintDebugInfo(device,SEC_GC_HELIX, other_strand->fixpoint_x, other_strand->fixpoint_y, "FP");
    }
}

void SEC_loop::paint_constraints(AW_device *device) {

    //paints circles with radius min_radius and max_radius
    device->circle(1, false, x_loop, y_loop, max_radius, max_radius, root->loop_filter, (AW_CL)((SEC_Base *)this), 0);
    device->circle(1, false, x_loop, y_loop, min_radius, min_radius, root->loop_filter, (AW_CL)((SEC_Base *)this), 0);

    //show radiuses as string just under the new circles
    char buffer[40];
    sprintf(buffer, "%.2f", max_radius);
    device->text(SEC_GC_DEFAULT, buffer, x_loop, (y_loop+max_radius+2), 0.5, root->loop_filter, (AW_CL)((SEC_Base *)this), 0);
    sprintf(buffer, "%.2f", min_radius);
    device->text(SEC_GC_DEFAULT, buffer, x_loop, (y_loop+min_radius+2), 0.5, root->loop_filter, (AW_CL)((SEC_Base *)this), 0);
}

void SEC_loop::paint(SEC_helix_strand *caller, AW_device *device, int show_constraints) {

    int is_root = 0;
    if (caller == NULL) {
        is_root = 1;
        caller = segment->get_next_helix();
    }

    if (show_constraints) {
        paint_constraints(device);
    }

    SEC_segment *segment_pointer = caller->get_next_segment();
    SEC_helix_strand *strand_pointer = segment_pointer->get_next_helix();
    SEC_helix_strand *previous_strand_pointer = caller;

    while (strand_pointer != caller) {
        segment_pointer->paint(device, previous_strand_pointer);
        strand_pointer->paint(device, show_constraints);
        previous_strand_pointer = strand_pointer;
        segment_pointer = strand_pointer->get_next_segment();
        strand_pointer = segment_pointer->get_next_helix();
    }
    segment_pointer->paint(device, previous_strand_pointer);   //paint last segment before caller

    if (is_root) {
        caller->paint(device, show_constraints);
    }

    if(root->show_debug) {
        paintDebugInfo(device,SEC_GC_CURSOR,  caller->get_attachp1_x(), caller->get_attachp1_y(), "AP1");
        paintDebugInfo(device,SEC_GC_CURSOR,  caller->get_attachp2_x(), caller->get_attachp2_y(), "AP2");
        paintDebugInfo(device,SEC_GC_CURSOR,  caller->get_fixpoint_x(), caller->get_fixpoint_y(),  "FP");
    }
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#if 0
void SEC_helix_strand::paint_other_strand(AW_device *device, double *v, double &length_of_v) {

    char buffer[2];
    buffer[1] = 0;

    double base_count = other_strand->region.get_base_count();

    //compute distance between the bases on this strand
    double db = length_of_v / (base_count - 1);

    double point_of_base_x;
    double point_of_base_y;
    int    abs_pos;
    int    j;
    long   ecoli_pos;
    long   dummy;

    const AW_font_group& font_group   = root->get_font_group();
    double               font_height2 = transform_ysize(device, font_group.get_ascent(SEC_GC_HELIX)) / 2.0;

    j = 0;

#if defined(DEBUG) && 1
    device->line(SEC_GC_DEFAULT, attachp1_x, attachp1_y, attachp2_x, attachp2_y, root->helix_filter, 0, 0);
#endif

    for (int i = int(base_count-1); i>=0; i--,j++) {
        if (other_strand->region.abspos_array) {
            abs_pos = other_strand->region.abspos_array[i];
        }
        else {
            abs_pos = (other_strand->region.get_sequence_end() - 1) - j;
        }

        if (abs_pos < 0) continue;
        point_of_base_x = attachp1_x + v[0]*db*j;
        point_of_base_y = attachp1_y + v[1]*db*j;

        if (abs_pos >=0 && abs_pos < root->sequence_length) {
            buffer[0] = root->sequence[abs_pos];
        }else{
            buffer[0] = '.';
        }

        if (root->helix && root->helix->entries[abs_pos].pair_type==HELIX_NONE) {
            print_lonely_bases(buffer, device, other_strand->attachp2_x, other_strand->attachp2_y, attachp1_x, attachp1_y, point_of_base_x, point_of_base_y,
                               abs_pos, font_height2);
        }
        else {
            device->text(SEC_GC_HELIX, buffer, point_of_base_x, point_of_base_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), abs_pos );
            root->announce_base_position(abs_pos, point_of_base_x, point_of_base_y-font_height2);
        }

        if (root->ecoli != NULL) {
            root->ecoli->abs_2_rel(abs_pos, ecoli_pos, dummy);
            if ((ecoli_pos%50) == 0) {
                print_ecoli_pos(ecoli_pos, other_strand->attachp2_x, other_strand->attachp2_y, attachp1_x, attachp1_y, point_of_base_x, point_of_base_y, device);
            }
        }
        //  last_abs_pos = abs_pos;
    }
}


void SEC_helix_strand::paint_this_strand(AW_device *device, double *v, double &length_of_v) {

    double base_count = region.get_base_count();
    double db         = length_of_v / (base_count - 1);

    char buffer[2];
    buffer[1] = 0;

    double point_of_base_x;
    double point_of_base_y;
    int    abs_pos;
    long   ecoli_pos;
    long   dummy;

    const AW_font_group& font_group   = root->get_font_group();
    double               font_height2 = transform_ysize(device, font_group.get_ascent(SEC_GC_HELIX)) / 2.0;

    for(int i=0; i<base_count; i++) {
        if (region.abspos_array) {
            abs_pos = region.abspos_array[i];
        }
        else {
            abs_pos = i + region.get_sequence_start();
        }
        if (abs_pos <0) continue;
        point_of_base_x = attachp2_x + v[0]*db*i;
        point_of_base_y = attachp2_y + v[1]*db*i;

        if (abs_pos < root->sequence_length){
            buffer[0] = root->sequence[abs_pos];
        }
        else {
            buffer[0] = '.';
        }

        if (root->helix && root->helix->entries[abs_pos].pair_type == HELIX_NONE) {
            print_lonely_bases(buffer, device, attachp2_x, attachp2_y, other_strand->attachp1_x, other_strand->attachp1_y, point_of_base_x, point_of_base_y,
                               abs_pos, font_height2);
        }
        else {
            device->text(SEC_GC_HELIX, buffer, point_of_base_x, point_of_base_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), abs_pos );
            root->announce_base_position(abs_pos, point_of_base_x, point_of_base_y-font_height2);
        }

        if (root->ecoli != NULL) {
            root->ecoli->abs_2_rel(abs_pos, ecoli_pos, dummy);
            if ((ecoli_pos%50) == 0) {
                print_ecoli_pos(ecoli_pos, attachp2_x, attachp2_y, other_strand->attachp1_x, other_strand->attachp1_y, point_of_base_x, point_of_base_y, device);
            }
        }
        //  last_abs_pos = abs_pos;
    }
}
#endif


/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/
//....................CREATING A DEFAULT BONE TO BEGIN WITH.............................
/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

void SEC_root::create_default_bone(int align_length) {
    SEC_segment *loop1_segment1 = new SEC_segment(this, 0, NULL, NULL);
    SEC_segment *loop2_segment1 = new SEC_segment(this, 0, NULL, NULL);

    SEC_loop *loop1 = new SEC_loop(this, NULL, 0, 0);
    SEC_loop *loop2 = new SEC_loop(this, NULL, 0, 0);

    SEC_helix *helix_a = new SEC_helix(M_PI, 0, 0);

    SEC_helix_strand *strand_a1 = new SEC_helix_strand(this, NULL, NULL, NULL, NULL, 0, 0);
    SEC_helix_strand *strand_a2 = new SEC_helix_strand(this, NULL, NULL, NULL, NULL, 0, 0);

    root_segment = loop1_segment1;
    max_index = align_length;
    cursor = 0;
    fresh_sequence = 0;
    set_distance_between_strands(4);
    set_skeleton_thickness(1);

    //initialize segment pointers of loops
    loop1->set_segment(loop1_segment1);
    loop2->set_segment(loop2_segment1);

    //initialize next_helix_strand pointers of segments
    loop1_segment1->set_next_helix_strand(strand_a1);
    loop2_segment1->set_next_helix_strand(strand_a2);

    //initialize loop-pointers of strands
    strand_a1->set_loop(loop1);
    strand_a2->set_loop(loop2);

    //initialize loop pointers of segments
    loop1_segment1->set_loop(loop1);
    loop2_segment1->set_loop(loop2);

    //initialize other_strand pointers
    strand_a1->set_other_strand(strand_a2);
    strand_a2->set_other_strand(strand_a1);

    //initialize helix_info pointers
    strand_a1->set_helix_info(helix_a);
    strand_a2->set_helix_info(helix_a);

    //initialize next_segment pointers
    strand_a1->set_next_segment(loop1_segment1);
    strand_a2->set_next_segment(loop2_segment1);

    //initialize SEQ-data
    SEC_region *temp_region;
    int part_segment = int((5.0/12)*align_length);
    int part_strand = int((1.0/6)*align_length);

    temp_region = loop1_segment1->get_region();
    temp_region->set_sequence_start(0);
    temp_region->set_sequence_end(part_segment);

    temp_region = strand_a1->get_region();
    temp_region->set_sequence_start(part_segment);
    temp_region->set_sequence_end(part_segment + part_strand);

    temp_region = loop2_segment1->get_region();
    temp_region->set_sequence_start(part_segment + part_strand);
    temp_region->set_sequence_end(2*part_segment + part_strand);

    temp_region = strand_a2->get_region();
    temp_region->set_sequence_start(2*part_segment + part_strand);
    temp_region->set_sequence_end(align_length);

    this->update(0);
}

/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

