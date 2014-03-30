#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <iostream.h>
#include <math.h>
#include <strstream.h>

#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>

#include <awt_canvas.hxx>

#include <BI_helix.hxx>

#include "secedit.hxx"
#include "sec_graphic.hxx"

int SEC_font_info::max_width;
int SEC_font_info::max_height;
int SEC_font_info::max_ascent;
int SEC_font_info::max_descent;

SEC_font_info font_info[SEC_GC_LAST_FONT+1];

static void init_font_infos(AW_device *device) {
    SEC_font_info::reset_maximas();
    for (int f=SEC_GC_FIRST_FONT; f<=SEC_GC_LAST_FONT; f++) {
        font_info[f].update(device->get_font_information(f, 'A'));
    }
}

inline AW_pos transform_size(AW_device *device, AW_pos size) {
    AW_pos scale = device->get_scale();
    return size/scale;
}

void SEC_region::count_bases(SEC_root *root) {
    
    base_count = 0;
    delete abspos_array;
    abspos_array = NULL;
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
                delete static_array;
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
                delete static_array;
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


void SEC_region::align_helix_strands(SEC_root *root, SEC_region *other_region){

    if ( abspos_array == NULL) return;	// no sequence available
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
    int fdest = 0;	// pointer to next unused position


    int f_last = 0;	// last index+1 which is already written
    int f_new;		// intermediate variable, used to find next real pair
    int p_f_last = r->base_count-1;	// pairing version of f_last
    int p_f_new;	// pairing index of f_next



    for ( f_last = 0; f_last < f->base_count; f_last = f_new) {
        /** geg f_last, p_f_last
            search f_new, which is an f index which has a pairing neighbour on other side
            and p_f_new which is pairing neighbour index
        */
        p_f_new = p_f_last;
        for (f_new = f_last+1; f_new < f->base_count; f_new++){
            unsigned abs_pos = f->abspos_array[f_new];
            if (abs_pos > helix->size){	// end
                goto copy_last_bases;
            }
            if (helix->entries[abs_pos].pair_type == HELIX_NONE) continue;
            int pairing_pos = helix->entries[abs_pos].pair_pos;

            for ( p_f_new = p_f_last; p_f_new >=0; p_f_new --){
                if (r->abspos_array[p_f_new] < 0) continue;		// already a gap
                if ( r->abspos_array[p_f_new] <= pairing_pos) break;	// position less or equal found
            }
            if ( (p_f_new >= 0) && (r->abspos_array[p_f_new] == pairing_pos)) break; // real pair found
        }

        /** number of elements to copy*/
        int rdist = p_f_last - p_f_new;	// revers
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

    

    delete f->abspos_array;
    f->abspos_array = fdest_array;
    f->base_count = fdest;

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

int SEC_region::get_faked_basecount(void) {
    if (base_count < 10) return (base_count+1);
    else return(base_count);
}

void SEC_loop::compute_umfang(void) {
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
    radius = umfang / (2 * M_PI);
    
    if (max_radius && radius>max_radius) radius = max_radius;
    if (min_radius && radius<min_radius) radius = min_radius;
}

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
    x = p_x + distance_p_q*v[0];
    y = p_y + distance_p_q*v[1];
}


void SEC_segment::update_alpha(void) {
    region.count_bases(root);
    int base_count = region.get_faked_basecount();
    alpha = ( (double) base_count / loop->get_umfang() ) * (2*M_PI);
}



void SEC_helix_strand::compute_attachment_points(double dir_delta) {
    double dbs = root->get_distance_between_strands();
    
    //turn dir_delta about 90 degrees
    dir_delta += (M_PI / 2);
    
    attachp1_x = fixpoint_x + cos(dir_delta)*(dbs / 2);
    attachp1_y = fixpoint_y + sin(dir_delta)*(dbs / 2);
    attachp2_x = fixpoint_x - cos(dir_delta)*(dbs / 2);
    attachp2_y = fixpoint_y - sin(dir_delta)*(dbs / 2);
}


void SEC_loop::test_angle(double &strand_angle, double &gamma, SEC_helix *helix_info, double &angle_difference) {

    //now we will test, if delta points away from the current loop, or to it. If it points to this loop, then it will be mirrored
    double delta_direction = ( (cos(gamma) * cos(strand_angle)) + (sin(gamma) * sin(strand_angle)) );
    if (delta_direction < 0) {
        strand_angle += M_PI;
        helix_info->set_delta(strand_angle-angle_difference);
    }
}


void SEC_loop::compute_segments_edge(double &attachp1_x, double &attachp1_y, double &attachp2_x, double &attachp2_y, SEC_segment *segment_pointer) {
    
	//test, if the center of the segment lies left or right of loop's center
	double attach_attach_v[2] = { (attachp1_x - attachp2_x), (attachp1_y-attachp2_y) };
	double attach_loopcenter_v[2] = { (attachp1_x-x), (attachp1_y-y) };

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


void SEC_loop::update_caller(double &gamma, double &strand_angle, SEC_helix *helix_info, double &angle_difference, SEC_helix_strand *strand_pointer) {
    double delta_direction = ( (cos(gamma) * cos(strand_angle)) + (sin(gamma) * sin(strand_angle)) );
    if (delta_direction < 0) {
        strand_angle += M_PI;
        helix_info->set_delta(strand_angle-angle_difference);
    }
    double next_x = x + cos(gamma)*radius;
    double next_y = y + sin(gamma)*radius;

    strand_pointer->update(next_x, next_y, angle_difference);
    strand_pointer->compute_attachment_points(strand_angle);
	
    //root loop's caller has to point to root loop, as all callers do
    helix_info->set_delta(strand_angle + M_PI);
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
        caller->compute_coordinates(radius, &x, &y, caller->get_fixpoint_x(), caller->get_fixpoint_y());
    }
    
    SEC_helix *helix_info = caller->get_helix_info();
    double gamma = helix_info->get_delta();
    double delta_direction;
    //turn around gamma, the caller is pointing to this loop, not away from it
    gamma += M_PI;
    
    double dbs = root->get_distance_between_strands();
    double angle_between_strands = ( dbs / umfang) * (2*M_PI);  //angle between two strands
    double next_x, next_y;
    SEC_segment *segment_pointer = caller->get_next_segment();
    SEC_helix_strand *strand_pointer = segment_pointer->get_next_helix();
    SEC_helix_strand *previous_strand_pointer = caller;
    double previous_strand_angle = gamma;                        //previous_strand_angle is angle of caller already in the right direction
    double strand_angle;
    double previous_x = caller->get_fixpoint_x();
    double previous_y = caller->get_fixpoint_y();
    double attachp1_x, attachp1_y, attachp2_x, attachp2_y;
   
    //compute fixpoints of caller if we are in the root loop
    if (is_root) {
        previous_x = x + cos(gamma)*radius;
        previous_y = y + sin(gamma)*radius;
        caller->set_fixpoint_x(previous_x);
        caller->set_fixpoint_y(previous_y);
    }
    
    //compute attachment-points of segments for caller
    caller->compute_attachment_points(previous_strand_angle);
    
    
    while (strand_pointer != caller) {
        segment_pointer->update_alpha();
        gamma += (segment_pointer->get_alpha()) + angle_between_strands;
        next_x = x + cos(gamma)*radius;
        next_y = y + sin(gamma)*radius;
	
        //after a split-operation the new helix-strand connecting the new loops gets a negative delta
        //We will correct this by setting it to match the angle of the line from this loop's center to the new fixpoint of the strand,
        //which is obviously the gamma we just computed. Since gamma already contains the angle-correction "angle-difference" (it is related from
        //the corrected angle of caller-strand) we will use "gamma - angle_difference", angle_difference will be added later again in the strand's
        //update-method.
        helix_info = strand_pointer->get_helix_info();
        if (helix_info->get_delta() < 0) {
            helix_info->set_delta(gamma-angle_difference);
        }
	
        strand_angle = helix_info->get_delta() + angle_difference;

        //now we will test, if delta points away from the current loop, or to it. If it points to this loop, then it will be mirrored
        test_angle(strand_angle, gamma, helix_info, angle_difference);
	
        strand_pointer->update(next_x, next_y, angle_difference);
	
        strand_pointer->compute_attachment_points(strand_angle);
        attachp1_x = previous_strand_pointer->get_attachp1_x();
        attachp1_y = previous_strand_pointer->get_attachp1_y();
        attachp2_x = strand_pointer->get_attachp2_x();
        attachp2_y = strand_pointer->get_attachp2_y();

        //compute edge of sector built by segment
        compute_segments_edge(attachp1_x, attachp1_y, attachp2_x, attachp2_y, segment_pointer);

        //prepare next "while-loop-cycle"
        previous_strand_angle = strand_angle;
        previous_x = next_x;
        previous_y = next_y;

        previous_strand_pointer = strand_pointer;
        segment_pointer = strand_pointer->get_next_segment();
        strand_pointer = segment_pointer->get_next_helix();
    }

    next_x = caller->get_fixpoint_x();
    next_y = caller->get_fixpoint_y();
    segment_pointer->update_alpha();
    helix_info = strand_pointer->get_helix_info();
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
        strand_angle += M_PI;
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


void SEC_helix_strand::compute_coordinates(double distance, double *x, double *y, double previous_x, double previous_y) {
    double delta = helix_info->get_delta();
    *x = previous_x + (cos(delta) * distance);
    *y = previous_y + (sin(delta) * distance);
}
  


void SEC_root::update(double angle_difference) {
    if (!root_segment) return;
    
    SEC_loop *root_loop = root_segment->get_loop();
    if (fresh_sequence) {
        root_loop->set_x_y(0,0);
        fresh_sequence = 0;
    }
    root_loop->compute_radius();
    root_loop->update(NULL, angle_difference);
}


void paint_box(SEC_root *root, SEC_Base *base, double x, double y, double radius, AW_device *device) {
    double dx = cos(M_PI / 4)*(radius);
    double dy = sin(M_PI / 4)*(radius);
    //    double width = 2*dx;
    //    double height = 2*dy;

    double x0 = x - dx;
    double y0 = y - dy;
    double x1 = x + dx;
    double y1 = y + dy;
    
    device->line(SEC_GC_DEFAULT, x0, y0, x1, y0, root->loop_filter, (AW_CL)base, 0 );
    device->line(SEC_GC_DEFAULT, x0, y0, x0, y1, root->loop_filter, (AW_CL)base, 0 );
    device->line(SEC_GC_DEFAULT, x1, y1, x0, y1, root->loop_filter, (AW_CL)base, 0 );
    device->line(SEC_GC_DEFAULT, x1, y1, x1, y0, root->loop_filter, (AW_CL)base, 0 );
}


// void SEC_segment::paint_cursor(double cursor_x, double cursor_y, AW_device *device) {
//     double radius = loop->get_radius();

//     double center_cursor_v[2] = { (cursor_x-x), (cursor_y-y) };
//     double length_of_vector = sqrt( (center_cursor_v[0]*center_cursor_v[0]) + (center_cursor_v[1]*center_cursor_v[1]) );
//     center_cursor_v[0] = center_cursor_v[0] / length_of_vector;
//     center_cursor_v[1] = center_cursor_v[1] / length_of_vector;
    
// #define CURSORSIZE 0.3
    
//     double start_x = x + center_cursor_v[0]*((1.0-CURSORSIZE)*radius);
//     double start_y = y + center_cursor_v[1]*((1.0-CURSORSIZE)*radius);
//     double end_x   = x + center_cursor_v[0]*((1.0+CURSORSIZE)*radius);
//     double end_y   = y + center_cursor_v[1]*((1.0+CURSORSIZE)*radius);
    
// #undef CURSORSIZE    
    
//     device->set_line_attributes(SEC_GC_CURSOR, 3, AW_SOLID);
//     device->line(SEC_GC_CURSOR, start_x, start_y, end_x, end_y, root->segment_filter, (AW_CL)((SEC_Base *)this), 0 );
//     root->set_last_drawed_cursor_position(start_x, start_y, end_x, end_y);
// }

// void SEC_helix_strand::paint_cursor(double attachpA_x, double attachpA_y, double attachpB_x, double attachpB_y, double cursor_x, double cursor_y, AW_device *device) {
//     double length = 0.5 * root->get_distance_between_strands();
//     double start_cursor_v[2] = {(attachpB_x-attachpA_x), (attachpB_y-attachpA_y) };
    
//     double tmp = start_cursor_v[0];
//     start_cursor_v[0] = start_cursor_v[1];
//     start_cursor_v[1] = (-1)*tmp;

//     double length_of_v = sqrt( (start_cursor_v[0]*start_cursor_v[0]) + (start_cursor_v[1]*start_cursor_v[1]) );
//     start_cursor_v[0] = start_cursor_v[0]/length_of_v;
//     start_cursor_v[1] = start_cursor_v[1]/length_of_v;
    
// #define CURSORSIZE 0.5
    
//     double start_x = cursor_x + start_cursor_v[0]*(CURSORSIZE*length);
//     double start_y = cursor_y + start_cursor_v[1]*(CURSORSIZE*length);
//     double end_x   = cursor_x - start_cursor_v[0]*(CURSORSIZE*length);
//     double end_y   = cursor_y - start_cursor_v[1]*(CURSORSIZE*length);
    
// #undef CURSORSIZE    
    
//     device->set_line_attributes(SEC_GC_CURSOR, 2, AW_SOLID);
//     device->line(SEC_GC_CURSOR, start_x, start_y, end_x, end_y, root->helix_filter, (AW_CL)((SEC_Base *)this), 0 );
//     root->set_last_drawed_cursor_position(start_x, start_y, end_x, end_y);
// }


void SEC_segment::print_ecoli_pos(long ecoli_pos, double base_x, double base_y, AW_device *device) {
    //    double radius = loop->get_radius();
    double center_base_v[2] = { (base_x-x), (base_y-y) };
    double length_of_vector = sqrt( (center_base_v[0]*center_base_v[0]) + (center_base_v[1]*center_base_v[1]) );
    double scalar = ( length_of_vector + 3 ) / length_of_vector;
    center_base_v[0] = center_base_v[0] * scalar;
    center_base_v[1] = center_base_v[1] * scalar;

    double print_pos_x = x + center_base_v[0];
    double print_pos_y = y + center_base_v[1];
    
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


void SEC_helix_strand::print_lonely_bases(char *buffer, AW_device *device, double attachpA_x, double attachpA_y, double attachpB_x, double attachpB_y, double base_x, double base_y,
                                          int abs_pos, double half_font_height) {
    double start_end_v[2] = {(attachpB_x-attachpA_x), (attachpB_y-attachpA_y) };
    
    double tmp = start_end_v[0];
    start_end_v[0] = -start_end_v[1];
    start_end_v[1] = tmp;

    double length_of_v = sqrt( (start_end_v[0]*start_end_v[0]) + (start_end_v[1]*start_end_v[1]) );
    start_end_v[0] = start_end_v[0]/length_of_v;
    start_end_v[1] = start_end_v[1]/length_of_v;

    double print_pos_x = base_x - start_end_v[0]*0.5;
    double print_pos_y = base_y - start_end_v[1]*0.5;
    
    device->text(SEC_GC_NHELIX, buffer, print_pos_x, print_pos_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), abs_pos);
    root->announce_base_position(abs_pos, print_pos_x, print_pos_y-half_font_height);
}


void SEC_root::paint(AW_device *device) {
    if (!root_segment){
        device->line(SEC_GC_DEFAULT,0,0,100,100);
    }else{
        SEC_loop *root_loop = root_segment->get_loop();
        clear_base_positions(); // reset positions next to cursor
        init_font_infos(device);
	
        paint_box(this,root_loop,root_loop->get_x(), root_loop->get_y(), (root_loop->get_radius()/3), device); //mark the root_loop
        root_loop->paint(NULL, device, show_constraints);
	
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


void SEC_segment::prepare_paint(SEC_helix_strand *previous_strand_pointer, double &gamma, double &eta, double &radius, int &base_count, double &angle_step) {
    //get attachpoint_1 of previous strand
    double attachp1_x = previous_strand_pointer->get_attachp1_x();
    double attachp1_y = previous_strand_pointer->get_attachp1_y();

    //compute angle to begin with
    gamma = (2*M_PI) + atan2( (attachp1_y - y), (attachp1_x - x) );
    while (gamma > (2*M_PI)) {
        gamma -= (2*M_PI);
    }

    //compute angle to end with, should normally be unneccessary, because we have the angle alpha, which spans the segment. But it seems to be not exact enough
    double attachp2_x = next_helix_strand->get_attachp2_x();
    double attachp2_y = next_helix_strand->get_attachp2_y();

    eta = (2*M_PI) + atan2( (attachp2_y - y), (attachp2_x - x) );
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
    // 	base_count = 1;
    //     }
    double dif_angle;
    if (eta >= gamma) dif_angle = (eta-gamma);
    else dif_angle = (2*M_PI) - (gamma-eta);
    angle_step = dif_angle / (base_count+1);
}

static int inline cursor_is_between(int abs_pos, int cursor, int last_abs_pos) {
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

void SEC_segment::paint(AW_device *device, SEC_helix_strand *previous_strand_pointer) {
    double angle_step, radius, eta, gamma;
    int base_count; 
    prepare_paint(previous_strand_pointer, gamma, eta, radius, base_count, angle_step);
    
    //    int cursor = root->get_cursor();
    int abs_pos, last_abs_pos = -1;
    double next_x, next_y;
    int i;
    char buffer[2];
    buffer[1] = 0;
    long ecoli_pos, dummy;
    double font_height2 = transform_size(device, font_info[SEC_GC_LOOP].get_ascent()) / 2.0;
    
    for (i = 0; i<base_count; i++) {
        gamma += angle_step;
        next_x = x + cos(gamma)*radius;
        next_y = y + sin(gamma)*radius;
        if (region.abspos_array) {
            abs_pos = region.abspos_array[i];
        }
        else {
            abs_pos = i + region.get_sequence_start();
        }
        root->announce_base_position(abs_pos, next_x, next_y-font_height2);
	    
        if ( (abs_pos < root->sequence_length)) {
            buffer[0] = root->sequence[abs_pos];
        } else {
            buffer[0] = '.';
        }
	
        device->text(SEC_GC_LOOP, buffer, next_x, next_y, 0.5, root->segment_filter, (AW_CL)((SEC_Base *)this), abs_pos );
	
        //	if (cursor_is_between(abs_pos, cursor, last_abs_pos)) {
        //	    paint_cursor(next_x, next_y, device);
        //	}
	
        if (root->ecoli != NULL) {
            root->ecoli->abs_2_rel(abs_pos, ecoli_pos, dummy);
            if ((ecoli_pos%50)==0) {
                print_ecoli_pos(ecoli_pos, next_x, next_y, device);
            }
        }
        last_abs_pos = abs_pos;
    }
}


void SEC_helix_strand::paint_constraints(AW_device *device, double *v, double &length_of_v) {
    char buffer[80];
    sprintf(buffer, "%.2f - %.2f", helix_info->get_min_length(), helix_info->get_max_length());

    //compute vector v_turn as vector v turned 90 degrees
    double v_turn[2] = { (v[1]*(root->get_distance_between_strands()/2)), ((-1)*v[0]*(root->get_distance_between_strands()/2))};
	
    double text_x = attachp1_x + v_turn[0] + v[0]*(length_of_v/2);
    double text_y = attachp1_y + v_turn[1] + v[1]*(length_of_v/2);
	
    device->text(SEC_GC_DEFAULT, buffer, text_x, text_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), 0);
}

static inline int max(int i1, int i2) {
    return i1>i2 ? i1 : i2;
}



void SEC_helix_strand::paint_strands(AW_device *device, double *v, double &length_of_v)
{
    char this_buffer[] = "?";
    char other_buffer[] = "?";
    SEC_region& other_region = other_strand->region;
    
    int this_base_count = region.get_base_count();
    int other_base_count = other_region.get_base_count();
    int max_base_count = max(this_base_count, other_base_count);
    
    double this_db = length_of_v/(this_base_count-1); // compute the distance between the bases of this strand
    double other_db = length_of_v/(other_base_count-1); // compute the distance between the bases of this strand
    
    double font_height2 = transform_size(device, font_info[SEC_GC_HELIX].get_ascent())/2.0; // half ascent-size of used font
    double font_size;
    
    {
        double h = transform_size(device, font_info[SEC_GC_HELIX].get_height());
        double w = transform_size(device, font_info[SEC_GC_HELIX].get_width());
	
        font_size = h>w ? h : w;
    }
    
    double this_other_off_x = 0; // offset between this_x and other_x
    double this_other_off_y = 0; // (same for y)
    
    for (int i=0,j=(max_base_count-1); i<max_base_count; i++,j--) {
        int this_abs_pos;
        int other_abs_pos;
	    
        if (region.abspos_array) 	this_abs_pos = region.abspos_array[i];
        else 				this_abs_pos = i+region.get_sequence_start();	
	
        if (other_region.abspos_array) 	other_abs_pos = other_region.abspos_array[j];
        else 				other_abs_pos = other_region.get_sequence_end()-1-j;
	
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
	
        // draw base characters:
        if (this_legal) {
            if (root->helix && root->helix->entries[this_abs_pos].pair_type==HELIX_NONE) {
                print_lonely_bases(this_buffer,  device, attachp2_x, attachp2_y, other_strand->attachp1_x, other_strand->attachp1_y,
                                   this_x,  this_y, this_abs_pos,  font_height2);
            }
            else {
                device->text(SEC_GC_HELIX, this_buffer, this_x, this_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), this_abs_pos);
                root->announce_base_position(this_abs_pos, this_x, this_y-font_height2);
            }
        }
	
        if (other_legal) {
            if (root->helix && root->helix->entries[other_abs_pos].pair_type==HELIX_NONE) {
                print_lonely_bases(other_buffer, device, other_strand->attachp2_x, other_strand->attachp2_y, attachp1_x, attachp1_y,
                                   other_x, other_y, other_abs_pos, font_height2);
            }
            else {
                device->text(SEC_GC_HELIX, other_buffer, other_x, other_y, 0.5, root->helix_filter, (AW_CL)((SEC_Base *)this), other_abs_pos);
                root->announce_base_position(other_abs_pos, other_x, other_y-font_height2);
            }
        }
	
        // draw bonds:
        if (this_legal && other_legal) {
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
}

#if 0
void SEC_helix_strand::paint_other_strand(AW_device *device, double *v, double &length_of_v) {

    char buffer[2];
    buffer[1] = 0;

    double base_count = other_strand->region.get_base_count();
    
    //compute distance between the bases on this strand
    double db = length_of_v / (base_count - 1);

    //    int cursor = root->get_cursor();
    double point_of_base_x, point_of_base_y;
    int abs_pos; /*, last_abs_pos = -1;*/
    int j;
    long ecoli_pos, dummy;
    double font_height2 = transform_ysize(device, font_info[SEC_GC_HELIX].get_ascent()) / 2.0;
    
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
        //	last_abs_pos = abs_pos;	
    }
}


void SEC_helix_strand::paint_this_strand(AW_device *device, double *v, double &length_of_v) {
    
    double base_count = region.get_base_count();
    double db = length_of_v / (base_count - 1);

    char buffer[2];
    buffer[1] = 0;

    //    int cursor = root->get_cursor();
    double point_of_base_x, point_of_base_y;
    int abs_pos; /*,  last_abs_pos = -1;*/
    long ecoli_pos, dummy;
    double font_height2 = transform_ysize(device, font_info[SEC_GC_HELIX].get_ascent()) / 2.0;
    
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
        //	last_abs_pos = abs_pos;
    }
}
#endif

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

    
}


void SEC_loop::paint_constraints(AW_device *device) {
    
    //paints circles with radius min_radius and max_radius
    device->circle(1, x, y, max_radius, max_radius, root->loop_filter, (AW_CL)((SEC_Base *)this), 0);
    device->circle(1, x, y, min_radius, min_radius, root->loop_filter, (AW_CL)((SEC_Base *)this), 0);

    //show radiuses as string just under the new circles
    char buffer[40];
    sprintf(buffer, "%.2f", max_radius);
    device->text(SEC_GC_DEFAULT, buffer, x, (y+max_radius+2), 0.5, root->loop_filter, (AW_CL)((SEC_Base *)this), 0);
    sprintf(buffer, "%.2f", min_radius);
    device->text(SEC_GC_DEFAULT, buffer, x, (y+min_radius+2), 0.5, root->loop_filter, (AW_CL)((SEC_Base *)this), 0);
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
    
}


void SEC_root::set_root(SEC_Base *base) {
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
	SEC_segment *segment_pointer = loop->get_segment();
	SEC_helix_strand *strand_pointer = segment_pointer->get_next_helix();
	double direction;
	double fixpoint_x = strand_pointer->get_fixpoint_x();
	double fixpoint_y = strand_pointer->get_fixpoint_y();
	double loop_x = loop->get_x();
	double loop_y = loop->get_y();
	double delta = strand_pointer->get_helix_info()->get_delta();
	double found = 0;
	
	//search caller of this loop and set it's segment-pointer appropriatly
	while (strand_pointer->get_next_segment() != loop->get_segment()) {
	    direction = (cos(delta)*(fixpoint_x - loop_x)) + (sin(delta)*(fixpoint_y - loop_y));
	    if (direction < 0) {
            found = 1;
            break;
	    }
	    segment_pointer = strand_pointer->get_next_segment();
	    strand_pointer = segment_pointer->get_next_helix();
	    delta = strand_pointer->get_helix_info()->get_delta();
	    fixpoint_x = strand_pointer->get_fixpoint_x();
	    fixpoint_y = strand_pointer->get_fixpoint_y();
	}
	if (!found) {
	    //check direction, because maybe the while-loop was never entered
	    direction = (cos(delta)*(fixpoint_x - loop_x)) + (sin(delta)*(fixpoint_y - loop_y));
	    if (direction > 0) {
            segment_pointer = loop->get_segment();
            strand_pointer = segment_pointer->get_next_helix();
            delta = strand_pointer->get_helix_info()->get_delta();
            strand_pointer->get_helix_info()->set_delta(delta + M_PI);
	    }
	}
	
	loop->set_segment(segment_pointer);
}


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
