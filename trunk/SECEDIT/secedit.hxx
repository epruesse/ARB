#ifndef secedit_hxx_included
#define secedit_hxx_included

#ifndef _CPP_CMATH
#include <cmath>
#endif
#ifndef _CPP_IOSFWD
#include <iosfwd>
#endif
#ifndef AW_FONT_GROUP_HXX
#include <aw_font_group.hxx>
#endif


class AW_window;
class SEC_graphic;

#ifndef NDEBUG
# define sec_assert(bed) do { if (!(bed)) { *(char*)0 = 0; } } while(0)
#else
# define sec_a3ssert(bed)
#endif

class SEC_segment;
class SEC_root;
class SEC_region;
class SEC_loop;
class SEC_helix;
class SEC_helix_strand;
class AW_device;
class AWT_canvas;
class AW_root;

enum SEC_BASE_TYPE {
    SEC_LOOP,
    SEC_SEGMENT,
    SEC_HELIX_STRAND
};


//////////////////////////////////////////////////////////////////////////////////
// 	SEC_base
//////////////////////////////////////////////////////////////////////////////////

class SEC_Base {
public:
    virtual ~SEC_Base() {}
    virtual SEC_BASE_TYPE getType() = 0;
};

//////////////////////////////////////////////////////////////////////////////////
// 	SEC_region
//////////////////////////////////////////////////////////////////////////////////

class SEC_region {
private:
    /* non redundant values */
    int sequence_start, sequence_end; //sequence_end is exclusive

    /* cached values */
    int base_count;		// number of real bases

public:
    int * abspos_array;

    SEC_region(int start=3, int end=4);
    ~SEC_region();

    //methods
    void save(std::ostream & out, int indent, SEC_root *root);
    void read(std::istream & in, SEC_root *root);
    void count_bases(SEC_root *root);
    void align_helix_strands(SEC_root *root, SEC_region *other_region);
    int get_faked_basecount();

    //selector-methods
    int get_sequence_start() 	{ return sequence_start; }
    int get_sequence_end() 		{ return sequence_end; }
    int get_base_count() 		{ return base_count; }

    void set_sequence_start(int sequence_start_) 	{ sequence_start = sequence_start_; }
    void set_sequence_end(int sequence_end_) 		{ sequence_end = sequence_end_; }
    void set_base_count(int base_count_) 		{ base_count = base_count_; }
};

//////////////////////////////////////////////////////////////////////////////////
// 	SEC_helix_strand
//////////////////////////////////////////////////////////////////////////////////

class SEC_helix_strand : public SEC_Base {
    SEC_loop *loop;              //Pointer to loop which contains the helix-strand
    SEC_helix_strand *other_strand;
    SEC_region region;
    SEC_helix *helix_info; // used by both strands
    SEC_segment *next_segment;
    SEC_root *root;

    //redundant values
    double fixpoint_x;  //coordinates, where strand meets it's loop
    double fixpoint_y;
    double attachp1_x, attachp1_y, attachp2_x, attachp2_y;   // 2 points where the segments meet this strand from left and right

    double thisLast_x,thisLast_y,otherLast_x,otherLast_y;               //these variables are used for prainting searchbackground in the
    int    thisBaseColor,otherBaseColor,thisLastAbsPos,otherLastAbsPos;  // secondary editor
    char   thisBase[2],otherBase[2];

public:
    //data for the drag operation
    double start_angle;
    double old_delta;

    SEC_helix_strand(SEC_root *root_=NULL, SEC_loop *loop=NULL, SEC_helix_strand *other_strand=NULL, SEC_helix *helix_info=NULL, SEC_segment *next_segment=NULL, int start=10, int end=11);
    virtual ~SEC_helix_strand();

    //methods
    void save_all(std::ostream & out, int indent);
    void save_core(std::ostream & out, int indent);
    void read(SEC_loop *loop_, std::istream & in);
    void compute_length();
    void compute_coordinates(double distance, double *x, double *y, double previous_x, double previous_y);
    void computeLoopCoordinates(double distance, double *x, double *y, double previous_x, double previous_y);
    void compute_attachment_points(double dir_delta);
    void update(double fixpoint_x_, double fixpoint_y_, double angle_difference);
    void paint(AW_device *device, int show_constraints);
    void unlink();
    SEC_segment * get_previous_segment();
    int connect_segments_and_strand();
    void print_ecoli_pos(long ecoli_pos, double attachpA_x, double attachpA_y, double attachpB_x, double attachpB_y, double base_x, double base_y, AW_device *device);
    void printHelixNumbers(AW_device *device, double helixStart_x, double helixStart_y, double helixEnd_x, double helixEnd_y, double base_x, double base_y, int absPos);
    void print_lonely_bases(char *buffer, AW_device *device, double attachpA_x, double attachpA_y, double attachpB_x, double attachpB_y, double base_x, double base_y,
			    int abs_pos, double half_font_height, const char *bgColor, int thisStrand);//yadhu
    void generate_x_string();
    //    void paint_this_strand(AW_device *device, double *v, double &length_of_v);
    //    void paint_other_strand(AW_device *device, double *v, double &length_of_v);
    void paint_strands(AW_device *device, double *v, double &length_of_v);

    void paint_constraints(AW_device *device, double *v, double &length_of_v);
    int connect_one(SEC_segment *segment_before, SEC_segment *segment_after);
    int connect_many(SEC_segment *segment_before, SEC_segment *segment_after);

    //selector-methods
    SEC_BASE_TYPE getType();

    SEC_segment * get_next_segment() 	{ return next_segment; }
    SEC_loop * get_loop() 			{ return loop; }
    SEC_helix_strand * get_other_strand() 	{ return other_strand; }
    SEC_helix * get_helix_info() 		{ return helix_info; }
    SEC_region * get_region() 		{ return &region; }

    double get_fixpoint_x () 	{ return fixpoint_x; }
    double get_fixpoint_y () 	{ return fixpoint_y; }
    double get_attachp1_x () 	{ return attachp1_x; }
    double get_attachp1_y () 	{ return attachp1_y; }
    double get_attachp2_x () 	{ return attachp2_x; }
    double get_attachp2_y () 	{ return attachp2_y; }


    void set_loop(SEC_loop *loop_) 				{ loop = loop_; }
    void set_other_strand(SEC_helix_strand *other_strand_) 	{ other_strand = other_strand_; }
    void set_helix_info(SEC_helix *helix_info_) 		{ helix_info = helix_info_; }
    void set_next_segment(SEC_segment *next_segment_) 		{ next_segment=next_segment_; }

    void set_fixpoint_x(double fixpoint_x_) 	{ fixpoint_x = fixpoint_x_; }
    void set_fixpoint_y(double fixpoint_y_) 	{ fixpoint_y = fixpoint_y_; }
    void set_fixpoints(double x, double y) 	{ fixpoint_x = x; fixpoint_y = y; }
};

//////////////////////////////////////////////////////////////////////////////////
// 	SEC_root
//////////////////////////////////////////////////////////////////////////////////

class BI_helix;
class BI_ecoli_ref;

struct SEC_base_position { // stores a base position and the last position where the base was drawn
    int pos; // -1 -> not initialised
    AW_pos x;
    AW_pos y;
};

class ED4_sequence_terminal; //to access ED4_sequence_terminal class with in the other classes

class SEC_root {
private:

    //redundant values
    int max_index;

    SEC_segment *root_segment;

    double distance_between_strands;

    int cursor;
    SEC_base_position before_cursor;
    SEC_base_position after_cursor;
    SEC_base_position min_position;
    SEC_base_position max_position;

    int cursor_x1, cursor_x2, cursor_y1, cursor_y2;
    int fresh_sequence;  //needed to check, if the coordinates of the root-loop have to be set or not

    AW_font_group font_group;

public:
    int rotateBranchesMode;
    double skeleton_thickness;
    bool show_debug; // show debug info in structure display (todo)

    bool show_helixNrs; //to display helix number information
    bool show_strSkeleton; // to display the skeleton
    bool hide_bases; // to toggle b/w show and hide bases
    bool hide_bonds; // to toggle b/w show and hide bonds
    bool display_sai;

    double rootAngle; // to store root angle

    //variables needed for the IO-procedures
    int number;
    int *number_found;
    char *x_string;	// needed to write data
    int x_string_len;
    int *ap; // array containing absolute positions of helices
    int ap_length;

    //variables needed for the paint-functionality
    int helix_filter;
    int segment_filter;
    int loop_filter;

    //indicator variables
    int show_constraints;
    int drag_recursive;   //indicates, if a drag will recursively alter the angles of the following strands

    /* ******* Sequences */

    GBDATA *gb_template;
    int template_length;		// length of template string
    char *template_sequence;

    GBDATA *gb_sequence;
    int sequence_length;		// length of string
    char *sequence;

    BI_helix *helix;
    BI_ecoli_ref *ecoli;

    ED4_sequence_terminal *seqTerminal;     //declaring a pointer class
    const  char *getSearchResults(int startPos,int endPos); // defining a function to build color string
    void paintSearchBackground(AW_device *device, const char* searchCols, int absPos, double x, double y, double next_x, double next_y, double radius,double lineWidth,int otherStrand);
    void paintSearchPatternStrings(AW_device *device, int clickedPos,  AW_pos xPos,  AW_pos yPos);
    //used in SEC_paint.cxx

    SEC_root(SEC_segment *root_segment, int max_index_, double distance_between_strands, double skeleton_thickness);
    ~SEC_root();

    //methods
    void init_sequences(AW_root *awr, AWT_canvas *ntw);
    char * write_data();
    GB_ERROR read_data(char *filename, char *x_string_in, long current_ali_len);
    void find(int pos, SEC_segment **found_segment, SEC_helix_strand **found_strand);
    void update(double angle_difference=0);
    void split_loop (int start1, int end1, int start2, int end2);
    void paint(AW_device *device);
    void unsplit_loop(SEC_helix_strand *delete_strand);
    void set_root(SEC_Base *base);
    void create_default_bone(int align_length);
    void generate_x_string();
    int check_errors(int &start1,int &end1,int &start2,int &end2,SEC_segment **found_start_segment1,SEC_segment **found_start_segment2,SEC_segment **found_end_segment1,SEC_segment **found_end_segment2,SEC_helix_strand **found_strand,SEC_loop **old_loop);
    void split_separate_segments(SEC_segment *found_start_segment1, SEC_segment *found_start_segment2, SEC_loop *old_loop, int &start1, int &end1, int &start2, int &end2);
    void split_same_segment1(SEC_segment *found_start_segment1, SEC_loop *old_loop, int &start1, int &end1, int &start2, int &end2);
    void split_same_segment2(SEC_segment *found_start_segment1, SEC_loop *old_loop, int &start1, int &end1, int &start2, int &end2);

    // selector methods
    int get_max_index()                   const { return max_index; }
    double get_distance_between_strands() const { return distance_between_strands; }
    double get_skeleton_thickness()       const { return skeleton_thickness; }
    SEC_segment * get_root_segment ()     const { return root_segment; }
    int get_cursor()                      const { return cursor; }

    void announce_base_position(int base_pos, AW_pos x, AW_pos y);
    void clear_base_positions();

    void set_max_index(int max_index_)                                   { max_index = max_index_; }
    void set_root_segment(SEC_segment *root_segment_)                    { root_segment = root_segment_; }
    void set_distance_between_strands (double distance_between_strands_) { distance_between_strands = distance_between_strands_; }

    void set_skeleton_thickness (double skeleton_thickness_)   { skeleton_thickness = skeleton_thickness_; }
    void set_show_debug (bool show) 	 { show_debug=show; }
    void set_show_helixNrs (bool show) 	 { show_helixNrs    = show; }
    void set_show_strSkeleton (bool show){ show_strSkeleton = show; }
    void set_hide_bases (bool hide)      { hide_bases = hide; }
    void set_hide_bonds (bool hide)      { hide_bonds = hide; }
    void set_display_sai (bool show)     { display_sai = show; }

    void set_cursor(int cursor_)                     { cursor = cursor_; }
    void set_show_constraints(int show_constraints_) { show_constraints = show_constraints_; }

    void set_last_drawed_cursor_position(double x1, double y1, double x2, double y2) { cursor_x1 = int(x1); cursor_y1 = int(y1); cursor_x2 = int(x2); cursor_y2 = int(y2); }
    void get_last_drawed_cursor_position(double &x1, double &y1, double &x2, double &y2) const { x1 = cursor_x1; y1 = cursor_y1; x2 = cursor_x2; y2 = cursor_y2; }
    void clear_last_drawed_cursor_position() { set_last_drawed_cursor_position(0, 0, 0, 0); }

    void setRootAngle(double rtAngle) {
        while(rtAngle >= (2*M_PI)){
            rtAngle -= (2*M_PI);
        }
        rootAngle = rtAngle;
    }
    double getRootAngle() const { return rootAngle; }

    const AW_font_group& get_font_group() const { return font_group; }
};

//////////////////////////////////////////////////////////////////////////////////
// 	SEC_segment
//////////////////////////////////////////////////////////////////////////////////

class SEC_segment : public SEC_Base {
private:

    //redundant values
    double alpha;                //angle of segment
    double x_seg, y_seg;	//coordinates of segment

    //information, which has to be provided
    SEC_helix_strand *next_helix_strand;
    SEC_region region;
    SEC_loop *loop;
    SEC_root *root;

public:

    SEC_segment(SEC_root *root_=NULL, double alpha=0, SEC_helix_strand *next_helix_strand=NULL, SEC_loop *loop=NULL);
    virtual ~SEC_segment();

    //methods
    void save(std::ostream & out, int indent);
    void read(SEC_loop *loop_,std::istream & in);
    void update_center_point(double start_x, double start_y, double end_x, double end_y);
    void update_alpha();
    void paint(AW_device *device, SEC_helix_strand *previous_strand_pointer);
    void unlink();
//    void paint_cursor(double cursor_x, double cursor_y, AW_device *device);
    void print_ecoli_pos(long ecoli_pos, double base_x, double base_y, AW_device *device);
    void generate_x_string();
    void prepare_paint(SEC_helix_strand *previous_strand_pointer, double &gamma, double &eta, double &radius, int &base_count, double &angle_step);

    int is_endings_segment() {
	int seq_start = region.get_sequence_start();
	int seq_end = region.get_sequence_end();

	return seq_start>seq_end; // rw: changed to allow segments with length==0

// 	if (seq_start > seq_end) return(1);
// 	if (seq_start < seq_end) return(0);

// 	return(2); // seq_start == seq_end
    }

    int is_it_element_of_next_strand(int pos) {
	SEC_segment *segment_pointer = next_helix_strand->get_next_segment();
	int my_end = region.get_sequence_end();
	int next_segments_start = segment_pointer->region.get_sequence_start();

	if ( (pos >= my_end) && (next_segments_start < my_end) ) {
	    return(1);
	}
	if (pos >= my_end) {
	    if (pos < next_segments_start) return(1);
	    else return(0);
	}
	else {
	    if (pos < next_segments_start) return(1);
	    else return(0);
	}
    }

    void delete_pointer_2 (SEC_helix_strand *strand) {
	SEC_segment *segment = this;
	while (1) {
	    if (segment->next_helix_strand == strand) {
		segment->next_helix_strand = NULL;
		break;
	    }
	    else {
		segment = segment->next_helix_strand->get_next_segment();
		if (segment==this) break;
	    }
	}
    }

    void delete_pointer_to_me () {
	SEC_segment *segment = this;
	SEC_segment *tmp_segment;
	while(1) {
	    tmp_segment = segment->next_helix_strand->get_next_segment();
	    if (tmp_segment != NULL) {
		if (tmp_segment == this) {
		    segment->next_helix_strand->set_next_segment(NULL);
		    break;
		}
		else {
		    segment = tmp_segment;
		}
	    }
	}
    }


    //selector methods
    SEC_helix_strand * get_previous_strand();
    SEC_BASE_TYPE getType();

    SEC_helix_strand * get_next_helix() 	{ return next_helix_strand; }
    SEC_loop * get_loop() 			{ return loop; }
    SEC_region * get_region() 		{ return &region; }

    double get_alpha () 	{ return alpha; }
    double get_x_seg() 		{ return x_seg; }
    double get_y_seg() 		{ return y_seg; }

    void set_next_helix_strand(SEC_helix_strand *strand) 	{ next_helix_strand = strand; }
    void set_loop(SEC_loop *loop_) 				{ loop = loop_; }
};

//////////////////////////////////////////////////////////////////////////////////
// 	SEC_loop
//////////////////////////////////////////////////////////////////////////////////

class SEC_loop : public SEC_Base {
private:

    //redundant values
    int umfang;  //number of bases in whole loop
    double x_loop, y_loop;          //coordinates of central point
    double radius;

    SEC_segment *segment;
    double max_radius, min_radius; // constraints
    SEC_root *root;

public:

    SEC_loop(SEC_root *root_=NULL, SEC_segment *segment=NULL, double max_radius=0, double min_radius=0);
    virtual ~SEC_loop();    //Destruktor

    //methods
    void save(std::ostream & out, SEC_helix_strand *caller, int indent);
    void read(SEC_helix_strand *other_strand, std::istream & in);
    void find(int pos, SEC_helix_strand *caller, SEC_segment **found_segment, SEC_helix_strand **found_strand);
    void compute_umfang();
    void compute_radius();
    void update(SEC_helix_strand *caller, double angle_difference);
    void update_caller();
    void paint(SEC_helix_strand *caller, AW_device *device, int show_constraints);
    //void paint_root_marker(AW_device *device);
    SEC_BASE_TYPE getType();
    void generate_x_string(SEC_helix_strand *callers);
    void test_angle(double &strand_angle, double &gamma, SEC_helix *helix_info, double &angle_difference);
    void compute_segments_edge(double &attachp1_x, double &attachp1_y, double &attachp2_x, double &attachp2_y, SEC_segment *segment_pointer);
    void update_caller(double &gamma, double &strand_angle, SEC_helix *helix_info, double &angle_difference, SEC_helix_strand *strand_pointer);
    void paint_constraints(AW_device *device);

    //selector methods
    double get_max_radius() 	{ return max_radius; }
    double get_min_radius() 	{ return min_radius; }
    SEC_segment * get_segment() 	{ return segment; }
    double get_x_loop() 			{ return x_loop; }
    double get_y_loop() 			{ return y_loop; }
    double get_radius() 		{ return radius; }
    int get_umfang () 		{ return umfang; }

    double& get_max_radius_ref() { return max_radius; }
    double& get_min_radius_ref() { return min_radius; }

    void set_segment(SEC_segment *segment_) 	{ segment = segment_; }
    void set_max_radius(double max_radius_) 	{ max_radius = max_radius_; }
    void set_min_radius(double min_radius_) 	{ min_radius = min_radius_; }
    void set_x_y_loop(double x_, double y_) 		{ x_loop = x_; y_loop = y_; }
};

//////////////////////////////////////////////////////////////////////////////////
// 	SEC_helix
//////////////////////////////////////////////////////////////////////////////////

class SEC_helix {
private:

    //redundant values
    double length;

    double delta;
    double deltaIn;
    double max_length, min_length; // constraints

public:

    SEC_helix(double delta=7.7,double deltaIn=0.0, double max_length=0, double min_length=0);
    ~SEC_helix();

    //methods
    void save(std::ostream & out, int indent);
    void read(std::istream & in);

    //selector methods
    double get_max_length () 	{ return max_length; }
    double get_min_length()		{ return min_length; }

    double get_length() 		{ return length; }

    double get_delta () 		{ return delta; }
    double get_deltaIn () 		{ return deltaIn;}

    double& get_max_length_ref() 	{ return max_length; }
    double& get_min_length_ref() 	{ return min_length; }

    void set_length(double length_) 	{ length = length_; }

    void set_delta (double delta_) {
 	while(delta_ >= (2*M_PI)) {
	    delta_ -= (2*M_PI);
	}
	delta = delta_;
    }

    void set_deltaIn (double deltaIn_) {
	while(deltaIn_ >= (2*M_PI)) {
	    deltaIn_ -= (2*M_PI);
	}
	deltaIn = deltaIn_;
    }
};

SEC_root *create_test_root();

// --------------------------------------------------------------------------------
// Global Prototypes:
// --------------------------------------------------------------------------------

AW_window *SEC_create_main_window(AW_root *awr);
void SEC_create_awars(AW_root *root,AW_default def);
void SEC_add_awar_callbacks(AW_root *aw_root, AW_default def, AWT_canvas *ntw);


void SEC_distance_between_strands_changed_cb(AW_root *awr, AW_CL cl_ntw);
void SEC_skeleton_thickness_changed_cb(AW_root *awr, AW_CL cl_ntw);
void SEC_show_debug_toggled_cb(AW_root *awr, AW_CL cl_ntw);
void SEC_show_helixNrs_toggled_cb(AW_root *awr, AW_CL cl_ntw);
void SEC_show_strSkeleton_toggled_cb(AW_root *awr, AW_CL cl_ntw);
void SEC_hide_bases_toggled_cb(AW_root *awr, AW_CL cl_ntw);
void SEC_hide_bonds_toggled_cb(AW_root *awr, AW_CL cl_ntw);
void SEC_display_sai_toggled_cb(AW_root *awr, AW_CL cl_ntw);
void SEC_pair_def_changed_cb(AW_root *awr, AW_CL cl_ntw);

// --------------------------------------------------------------------------------
// inlines:
// --------------------------------------------------------------------------------

inline void SEC_root::announce_base_position(int base_pos, AW_pos x, AW_pos y) {
    if (base_pos<cursor && (before_cursor.pos==-1 || before_cursor.pos<base_pos)) {
	before_cursor.pos = base_pos;
	before_cursor.x = x;
	before_cursor.y = y;
    }
    if (base_pos>=cursor && (after_cursor.pos==-1 || after_cursor.pos>base_pos)) {
	after_cursor.pos = base_pos;
	after_cursor.x = x;
	after_cursor.y = y;
    }
    if (min_position.pos==-1 || min_position.pos>base_pos) {
	min_position.pos = base_pos;
	min_position.x = x;
	min_position.y = y;
    }
    if (max_position.pos==-1 || max_position.pos<base_pos) {
	max_position.pos = base_pos;
	max_position.x = x;
	max_position.y = y;
    }
}

inline void SEC_root::clear_base_positions()
{
    before_cursor.pos = -1;
    after_cursor.pos = -1;
    min_position.pos = -1;
    max_position.pos = -1;
}




#endif
