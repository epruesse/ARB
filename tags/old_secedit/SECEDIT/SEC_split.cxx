#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <iostream>

#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>

#include"secedit.hxx"



void SEC_loop::find(int pos, SEC_helix_strand *caller, SEC_segment **found_segment, SEC_helix_strand **found_strand) {
    SEC_segment *segment_pointer;
    SEC_helix_strand *strand_pointer;
    SEC_region *region_pointer;
    int seq_start, seq_end;
    int is_root = 0;

    if (caller == NULL) {
        is_root = 1;
        caller = segment->get_next_helix();
    }

    segment_pointer = caller->get_next_segment();

    while(1) {
        region_pointer = segment_pointer->get_region();
        seq_start = region_pointer->get_sequence_start();
        seq_end = region_pointer->get_sequence_end();

        if (segment_pointer->is_endings_segment()) { // special index-check for segment containing both ends of the sequence
            // the endings-segment has to define a maximum index against which pos can be compared

            // SEC_endings_segment *endings_segment_pointer = (SEC_endings_segment *) segment_pointer;
            int max_index = root->get_max_index();
            if (pos > max_index) {
                aw_message("Index-overflow! aborting ...\n");
                return;
            }
            if ((pos >= seq_start) && (pos <= max_index)) {
                *found_segment = segment_pointer;
                break;
            }
            if ( (pos >= 0) && (pos < seq_end) ) {
                *found_segment = segment_pointer;
                break;
            }
            if (pos >= seq_end) {
                if ( segment_pointer->is_it_element_of_next_strand(pos) ) {
                    strand_pointer = segment_pointer->get_next_helix();
                    region_pointer = strand_pointer->get_region();
                    seq_start = region_pointer->get_sequence_start();
                    seq_end = region_pointer->get_sequence_end();
                    if( (pos >= seq_start) && (pos < seq_end) ) {
                        *found_strand = strand_pointer;
                        break;
                    }
                    strand_pointer = strand_pointer->get_other_strand();
                    SEC_loop *loop_pointer = strand_pointer->get_loop();
                    loop_pointer->find(pos, strand_pointer, found_segment, found_strand);
                    break;
                }
                else {
                    strand_pointer = segment_pointer->get_next_helix();
                    segment_pointer = strand_pointer->get_next_segment();
                }
            }
        }
        else {
            if( (pos >= seq_start) && (pos < seq_end) ) {
                *found_segment = segment_pointer;
                break;
            }
            if ( segment_pointer->is_it_element_of_next_strand(pos) ) { // is pos in strand or somewhere "on the other side" of the strand
                strand_pointer = segment_pointer->get_next_helix();
                region_pointer = strand_pointer->get_region();
                seq_start = region_pointer->get_sequence_start();
                seq_end = region_pointer->get_sequence_end();
                if( (pos >= seq_start) && (pos < seq_end) ) { // pos is in strand
                    *found_strand = strand_pointer;
                    break;
                }
                strand_pointer = strand_pointer->get_other_strand();
                SEC_loop *loop_pointer = strand_pointer->get_loop();
                loop_pointer->find(pos, strand_pointer, found_segment, found_strand); // search on other side
                break;
            }
            else {
                strand_pointer = segment_pointer->get_next_helix();
                segment_pointer = strand_pointer->get_next_segment();
            }
        }
        // rw: removed the following else-branch to allow segments with length==0
        //
        // 	else {
        // 	    sec_assert(0); // Invalid segment! Start and end of segment are equal!
        // 	    aw_message("Your secondary structure has an internal error - please contact your system administrator", "OK");
        // 	    return;
        // 	}
    }
}


void SEC_root::find(int pos, SEC_segment **found_segment, SEC_helix_strand **found_strand) {
    SEC_loop *root_loop = root_segment->get_loop();

    root_loop->find(pos, NULL, found_segment, found_strand);
#if defined(DEBUG)
    sec_assert(*found_segment || *found_strand); // pos should be somewhere (maybe pos is wrong?)
#endif // DEBUG
}


int SEC_root::check_errors(int &start1, int &end1, int &start2, int &end2,
                           SEC_segment **found_start_segment1, SEC_segment **found_start_segment2,
                           SEC_segment **found_end_segment1, SEC_segment **found_end_segment2,
                           SEC_helix_strand **found_strand, SEC_loop **old_loop) {

    if ( (end1<=start1) ) {
        aw_message("End-position of a selection must be greater than the start-position");
        return(1);
    }
    if ( (end2<=start2) ) {
        aw_message("End-position of a selection must be greater than the start-position");
        return(1);
    }

    find(start1, found_start_segment1, found_strand);
    if (*found_strand != NULL) {
    found_in_strand:
        aw_message("The position you were looking for was found in a strand, not in a segment!");
        return(1);
    }

    find(end1-1, found_end_segment1, found_strand);
    if (*found_strand != NULL) {
        goto found_in_strand;
    }

    find(start2, found_start_segment2, found_strand);
    if (*found_strand != NULL) {
        goto found_in_strand;
    }

    find(end2-1, found_end_segment2, found_strand);
    if (*found_strand != NULL) {
        goto found_in_strand;
    }

    if ( (*found_start_segment1 != *found_end_segment1) || (*found_start_segment2 != *found_end_segment2) ) {
        aw_message("Start and end of a selection must be within the same segment!");
        return(1);
    }

    *old_loop = (*found_start_segment1)->get_loop();
    if (*old_loop != (*found_start_segment2)->get_loop()) {
        aw_message("Both selections must take place within the same loop!");
        return(1);
    }

    //test, if endings would be in a strand after the split
    if ((*found_start_segment1)->is_endings_segment()) {
        SEC_region *segment_region = (*found_start_segment1)->get_region();
        if( (end1 > 0) && (end1 < (segment_region->get_sequence_end())) ) {
            if ( (start1==0) || ((start1 <= max_index) && (start1 >= (segment_region->get_sequence_start()))) ) {
                aw_message("No valid selection! Ends of sequence would be in a strand!");
                return(1);
            }
        }
    }

    if ((*found_start_segment2)->is_endings_segment()) {
        SEC_region *segment_region = (*found_start_segment2)->get_region();
        if( (end2 > 0) && (end2 < (segment_region->get_sequence_end())) ) {
            if ( (start2 == 0) || ((start2 <= max_index) && (start2 >= (segment_region->get_sequence_start()))) ) {
                aw_message("No valid selection! Ends of sequence would be in a strand!");
                return(1);
            }
        }
    }
    return(0);

}


void SEC_root::split_separate_segments(SEC_segment *found_start_segment1, SEC_segment *found_start_segment2, SEC_loop *old_loop, int &start1, int &end1, int &start2, int &end2) {
    SEC_region *region_pointer1 = found_start_segment1->get_region();
    SEC_region *region_pointer2 = found_start_segment2->get_region();

    //create new objects and initialize pointers
    SEC_helix_strand *new_strand1 = new SEC_helix_strand(this, NULL, NULL, NULL, NULL, 0, 0);
    SEC_helix_strand *new_strand2 = new SEC_helix_strand(this, NULL, NULL, NULL, NULL, 0, 0);
    SEC_loop *new_loop = new SEC_loop(this, NULL, 0, 0);
    SEC_helix *helix_info = new SEC_helix;

    SEC_segment *new_segment_4_old_loop = new SEC_segment(this, 0, (found_start_segment1->get_next_helix()), old_loop);
    SEC_segment *new_segment_4_new_loop = new SEC_segment(this, 0, (found_start_segment2->get_next_helix()), new_loop);
    found_start_segment2->set_next_helix_strand(new_strand1);
    found_start_segment1->set_next_helix_strand(new_strand2);
    found_start_segment1->set_loop(new_loop);

    new_strand1->set_other_strand(new_strand2);
    new_strand2->set_other_strand(new_strand1);
    new_strand1->set_next_segment(new_segment_4_old_loop);
    new_strand2->set_next_segment(new_segment_4_new_loop);
    new_strand1->set_loop(old_loop);
    new_strand2->set_loop(new_loop);
    new_strand1->set_helix_info(helix_info);
    new_strand2->set_helix_info(helix_info);

    //reset pointers of old structure-elements to point to new_loop
    SEC_helix_strand *strand_pointer = new_segment_4_new_loop->get_next_helix();
    SEC_segment *segment_pointer = strand_pointer->get_next_segment();
    while (segment_pointer != found_start_segment1) {
        strand_pointer->set_loop(new_loop);
        segment_pointer->set_loop(new_loop);

        strand_pointer = segment_pointer->get_next_helix();
        segment_pointer = strand_pointer->get_next_segment();
    }
    strand_pointer->set_loop(new_loop);


    //evt. setting segment-pointer of old_loop to point to new segment
    new_loop->set_segment(new_segment_4_new_loop);
    if ( ((old_loop->get_segment()) == found_start_segment1)) {
        old_loop->set_segment(new_segment_4_old_loop);
    }

    //reset the region objects of the new and the old objects

    region_pointer1 = found_start_segment1->get_region();
    region_pointer2 = new_segment_4_old_loop->get_region();

    region_pointer2->set_sequence_start(end1);
    region_pointer2->set_sequence_end(region_pointer1->get_sequence_end());

    //*****************
    region_pointer2 = new_strand1->get_region();

    region_pointer2->set_sequence_start(start2);
    region_pointer2->set_sequence_end(end2);

    //*****************
    region_pointer1->set_sequence_end(start1);

    //*****************
    region_pointer1 = found_start_segment2->get_region();
    region_pointer2 = new_segment_4_new_loop->get_region();

    region_pointer2->set_sequence_start(end2);
    region_pointer2->set_sequence_end(region_pointer1->get_sequence_end());

    //*****************
    region_pointer2 = new_strand2->get_region();

    region_pointer2->set_sequence_start(start1);
    region_pointer2->set_sequence_end(end1);

    //*****************
    region_pointer1->set_sequence_end(start2);

    //initialize helix_info-object of new strand with information
    helix_info->set_delta(-1);    //mark this strand, so the loop::update-method can compute the new delta
    //max and min length have the standard settings. Both should be computed by counting the possible amount of
    //bases on this part of the whole sequence

}


void SEC_root::split_same_segment1(SEC_segment *found_start_segment1, SEC_loop *old_loop, int &start1, int &end1, int &start2, int &end2) {
    SEC_region *region_pointer1 = found_start_segment1->get_region();
    SEC_region *region_pointer2;
    SEC_loop *new_loop = new SEC_loop(this, NULL, 0, 0);
    SEC_helix_strand *new_strand1 = new SEC_helix_strand(this, NULL, NULL, NULL, NULL, 0, 0);
    SEC_helix_strand *new_strand2 = new SEC_helix_strand(this, NULL, NULL, NULL, NULL, 0, 0);
    SEC_helix *helix_info = new SEC_helix;

    SEC_segment *new_segment1_4_new_loop = new SEC_segment(this, 0, (found_start_segment1->get_next_helix()), new_loop);
    SEC_segment *new_segment2_4_new_loop = new SEC_segment(this, 0, new_strand2, new_loop);

    new_strand1->set_other_strand(new_strand2);
    new_strand2->set_other_strand(new_strand1);
    new_strand1->set_next_segment(found_start_segment1);
    new_strand2->set_next_segment(new_segment1_4_new_loop);
    new_strand1->set_loop(old_loop);
    new_strand2->set_loop(new_loop);
    new_strand1->set_helix_info(helix_info);
    new_strand2->set_helix_info(helix_info);

    new_loop->set_segment(new_segment1_4_new_loop);
    old_loop->set_segment(found_start_segment1);

    SEC_helix_strand *strand_pointer = found_start_segment1->get_next_helix();
    SEC_segment *segment_pointer = strand_pointer->get_next_segment();
    while (segment_pointer != found_start_segment1) {
        strand_pointer->set_loop(new_loop);
        segment_pointer->set_loop(new_loop);
        strand_pointer=segment_pointer->get_next_helix();
        segment_pointer=strand_pointer->get_next_segment();
    }
    strand_pointer->set_loop(new_loop);

    strand_pointer->set_next_segment(new_segment2_4_new_loop);
    found_start_segment1->set_next_helix_strand(new_strand1);



    //reset regions of old and new objects
    region_pointer1 = found_start_segment1->get_region();
    region_pointer2 = new_segment1_4_new_loop->get_region();

    region_pointer2->set_sequence_start(end1);
    region_pointer2->set_sequence_end(region_pointer1->get_sequence_end());

    //***************
    region_pointer2 = new_segment2_4_new_loop->get_region();
    SEC_region *region_pointer3 = strand_pointer->get_other_strand()->get_region();

    region_pointer2->set_sequence_start(region_pointer3->get_sequence_end());
    region_pointer2->set_sequence_end(start2);

    //***************
    region_pointer2 = new_strand1->get_region();

    region_pointer2->set_sequence_start(start1);
    region_pointer2->set_sequence_end(end1);

    //***************
    region_pointer2 = new_strand2->get_region();

    region_pointer2->set_sequence_start(start2);
    region_pointer2->set_sequence_end(end2);

    //***************
    region_pointer1->set_sequence_end(start1);
    region_pointer1->set_sequence_start(end2);

    //initialize helix_info-object of new strand with information
    helix_info->set_delta(-1);    //mark this strand, so the loop::update-method can compute the new delta
    //max and min length have the standard settings. Both should be computed by counting the possible amount of
    //bases on this part of the whole sequence

}


void SEC_root::split_same_segment2(SEC_segment *found_start_segment1, SEC_loop *old_loop, int &start1, int &end1, int &start2, int &end2) {
    SEC_region *region_pointer1 = found_start_segment1->get_region();
    SEC_region *region_pointer2;
    SEC_loop *new_loop = new SEC_loop(this, NULL, 0, 0);
    SEC_helix_strand *new_strand1 = new SEC_helix_strand(this, NULL, NULL, NULL, NULL, 0, 0);
    SEC_helix_strand *new_strand2 = new SEC_helix_strand(this, NULL, NULL, NULL, NULL, 0, 0);
    SEC_helix *helix_info = new SEC_helix;

    SEC_segment *new_segment_4_new_loop = new SEC_segment(this, 0, new_strand2, new_loop);
    SEC_segment *new_segment_4_old_loop = new SEC_segment(this, 0, (found_start_segment1->get_next_helix()), old_loop);
    found_start_segment1->set_next_helix_strand(new_strand1);

    new_strand1->set_other_strand(new_strand2);
    new_strand2->set_other_strand(new_strand1);
    new_strand1->set_next_segment(new_segment_4_old_loop);
    new_strand2->set_next_segment(new_segment_4_new_loop);
    new_strand1->set_loop(old_loop);
    new_strand2->set_loop(new_loop);
    new_strand1->set_helix_info(helix_info);
    new_strand2->set_helix_info(helix_info);

    new_loop->set_segment(new_segment_4_new_loop);

    //reset regions of old and new objects

    region_pointer1 = found_start_segment1->get_region();
    region_pointer2 = new_segment_4_old_loop->get_region();

    region_pointer2->set_sequence_start(end2);
    region_pointer2->set_sequence_end(region_pointer1->get_sequence_end());

    //***************
    region_pointer2 = new_segment_4_new_loop->get_region();

    region_pointer2->set_sequence_start(end1);
    region_pointer2->set_sequence_end(start2);

    //***************
    region_pointer2 = new_strand1->get_region();

    region_pointer2->set_sequence_start(start1);
    region_pointer2->set_sequence_end(end1);

    //***************
    region_pointer2 = new_strand2->get_region();

    region_pointer2->set_sequence_start(start2);
    region_pointer2->set_sequence_end(end2);

    //***************
    region_pointer1->set_sequence_end(start1);

    //initialize helix_info-object of new strand with information
    helix_info->set_delta(-1);    //mark this strand, so the loop::update-method can compute the new delta
    //max and min length have the standard settings. Both should be computed by counting the possible amount of
    //bases on this part of the whole sequence
}



void SEC_root::split_loop (int start1, int end1, int start2, int end2) {
    SEC_root *root = this;
    SEC_segment *found_start_segment1 = NULL;
    SEC_segment *found_start_segment2 = NULL;
    SEC_segment *found_end_segment1 = NULL;
    SEC_segment *found_end_segment2 = NULL;
    SEC_helix_strand *found_strand = NULL;
    SEC_loop *old_loop = NULL;

    if (check_errors(start1, end1, start2, end2,
                     &found_start_segment1, &found_start_segment2,
                     &found_end_segment1, &found_end_segment2,
                     &found_strand, &old_loop)) {
        return;
    }

    if (found_start_segment1 != found_start_segment2) {
        split_separate_segments(found_start_segment1,  found_start_segment2, old_loop, start1, end1, start2, end2);
    }
    else {

        if (start2>=start1) {
            if (start2==start1) {
                aw_message("The two selections are not separate!");
                return;
            }
            if (start2<end1) {
                aw_message("The two selections are not separate!");
                return;
            }
            if (start2==end1) {
                aw_message("Error! New loop would contain no data!");
                return;
            }
        }
        else {
            if (end2>=start1) {
                if (end2==start1) {
                    aw_message("New loop would contain no data!");
                    return;
                }
                else {
                    aw_message("Error! The two selections are not separate!");
                    return;
                }
            }
        }

        int decision = 0;
        //special treating, otherwise the sequence can be doubled
        if (found_start_segment1->is_endings_segment() && (end1 < start2)) {
            SEC_helix_strand *strand_pointer = found_start_segment1->get_next_helix();
            SEC_region *region_pointer = strand_pointer->get_region();
            if ((region_pointer->get_sequence_start() >= end1) && (region_pointer->get_sequence_start() < start2)) {
                decision = 1;
            }
        }

        if (decision) {
            split_same_segment1(found_start_segment1, old_loop, start1, end1, start2, end2);
        }
        else {
            split_same_segment2(found_start_segment1, old_loop, start1, end1, start2, end2);
        }
    }
    root->update(0);
    return;
}


void SEC_helix_strand::unlink(void) {
    next_segment = NULL;
    other_strand = NULL;
    loop = NULL;
    root = NULL;

}

void SEC_segment::unlink(void) {
    next_helix_strand = NULL;
}


SEC_segment * SEC_helix_strand::get_previous_segment(void) {

    SEC_segment *segment_before;
    SEC_helix_strand *strand_pointer = next_segment->get_next_helix();

    if (strand_pointer == this) {
        segment_before = next_segment;   //we are in a loop with only one segment
    }
    else {
        while (strand_pointer != this) {
            segment_before = strand_pointer->get_next_segment();
            strand_pointer = segment_before->get_next_helix();
        }
    }
    return segment_before;
}


void SEC_loop::update_caller(void){

    //updates delta of a caller of the root loop if neccessary
    SEC_helix_strand *caller = segment->get_next_helix();
    SEC_helix *helix_info = caller->get_helix_info();

    double delta = helix_info->get_delta();
    double fixpoint_x = caller->get_fixpoint_x();
    double fixpoint_y = caller->get_fixpoint_y();
    double direction = (cos(delta)*(fixpoint_x - x_loop)) + (sin(delta)*(fixpoint_y - y_loop));

    if (direction > 0) {
        helix_info->set_delta(delta + M_PI);
    }

}


int SEC_helix_strand::connect_many(SEC_segment *segment_before, SEC_segment *segment_after) {

    SEC_region *region_before = segment_before->get_region();
    SEC_region *region_after = segment_after->get_region();

    //update sequence data
    region_before->set_sequence_end(region_after->get_sequence_end());

    //update pointers
    SEC_helix_strand *strand_pointer = segment_after->get_next_helix();
    segment_before->set_next_helix_strand(strand_pointer);
    strand_pointer->set_loop(loop);
    SEC_segment *segment_pointer = strand_pointer->get_next_segment();
    strand_pointer = segment_pointer->get_next_helix();
    while (strand_pointer != other_strand) {
        segment_pointer->set_loop(loop);
        strand_pointer->set_loop(loop);

        segment_pointer = strand_pointer->get_next_segment();
        strand_pointer = segment_pointer->get_next_helix();
    }
    segment_pointer->set_loop(loop);

    //the same procedure for the other_strand
    SEC_segment *other_segment_before = other_strand->get_previous_segment();
    SEC_segment *other_segment_after = next_segment;

    SEC_region *other_region_before = other_segment_before->get_region();
    SEC_region *other_region_after = other_segment_after->get_region();

    other_region_before->set_sequence_end(other_region_after->get_sequence_end());
    other_segment_before->set_next_helix_strand(other_segment_after->get_next_helix());

    loop->set_segment(segment_before);

    if ((root->get_root_segment())->get_loop() == other_strand->loop) {
        //if we are to delete the root-loop, we have to reset the segment pointer of loop
        root->set_root_segment(segment_before);
        //also we have to account for that the "caller" of a loop allways points TO a loop
        loop->update_caller();
    }
    if ((root->get_root_segment())->get_loop() == loop) {
        //afterwards we will delete next_segment. If root_segment points to it, then root_segment has to be reset
        root->set_root_segment(segment_before);
        loop->update_caller();
    }

    return 4;
}


int SEC_helix_strand::connect_one(SEC_segment *segment_before, SEC_segment *segment_after) {
    SEC_region *region_before = segment_before->get_region();
    SEC_region *region_after = segment_after->get_region();

    segment_after = next_segment;
    region_after = segment_after->get_region();

    region_before->set_sequence_end(region_after->get_sequence_end());
    segment_before->set_next_helix_strand(segment_after->get_next_helix());
    loop->set_segment(segment_before);
    if ((root->get_root_segment())->get_loop() == other_strand->loop) {
        //if we are to delete the root-loop, we have to reset the segment pointer of loop
        root->set_root_segment(segment_before);
        //also we have to account for that the "caller" of a loop allways points TO a loop
        loop->update_caller();
    }
    if ((root->get_root_segment())->get_loop() == loop) {
        //afterwards we will delete next_segment. If root_segment points to it, then root_segment has to be reset
        root->set_root_segment(segment_before);
        loop->update_caller();
    }

    return 1;
}


int SEC_helix_strand::connect_segments_and_strand(void) {
    SEC_segment *segment_before;
    SEC_segment *segment_after;

    segment_before = get_previous_segment();
    segment_after = other_strand->get_next_segment();

    if (segment_before == next_segment) {
        //we are in a loop with only one segment, and the following loop has also got only one segment
        //this is no valid structure
        return 2;
    }

    if (segment_after->get_next_helix() != other_strand) {
        return(connect_many(segment_before, segment_after));
    }
    else {   //when the following loop has only got one segment
        return(connect_one(segment_before, segment_after));
    }
}



void SEC_root::unsplit_loop(SEC_helix_strand *delete_strand) {
    SEC_segment *segment_before = delete_strand->get_previous_segment();
    SEC_helix_strand *other_strand = delete_strand->get_other_strand();
    int decision;

    if (segment_before == delete_strand->get_next_segment()) {  //if loop of delete_strand has got only one segment
        decision = other_strand->connect_segments_and_strand();
        decision++;
    }
    else {
        decision = delete_strand->connect_segments_and_strand();
    }

    SEC_loop *other_loop;
    SEC_segment *other_segment;
    switch (decision) {
        case 1:
            other_loop = other_strand->get_loop();
            other_loop->set_segment(NULL);
            delete other_loop;
            other_segment = other_strand->get_next_segment();
            delete other_segment;
            other_segment = delete_strand->get_next_segment();
            delete other_segment;
            delete_strand->unlink();
            other_strand->unlink();
            delete delete_strand;
            break;
        case 2:
            other_loop = delete_strand->get_loop();
            other_loop->set_segment(NULL);
            delete other_loop;
            other_segment = other_strand->get_next_segment();
            delete other_segment;
            other_segment = delete_strand->get_next_segment();
            delete other_segment;
            delete_strand->unlink();
            other_strand->unlink();
            delete delete_strand;
            break;
        case 3:
            aw_message("The structure has to consist of at least two loops connected by one helix-strand");
            break;
        case 4:
            other_loop = other_strand->get_loop();
            other_loop->set_segment(NULL);
            delete other_loop;
            other_segment = delete_strand->get_next_segment();
            delete other_segment;
            other_segment = other_strand->get_next_segment();
            delete other_segment;
            delete_strand->unlink();
            other_strand->unlink();
            delete delete_strand;
            break;
    }

    update(0);
}
