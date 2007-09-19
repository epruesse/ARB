// =============================================================== //
//                                                                 //
//   File      : SEC_root.hxx                                      //
//   Purpose   : secondary structure representation                //
//   Time-stamp: <Mon Sep/17/2007 11:30 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SEC_ROOT_HXX
#define SEC_ROOT_HXX

#ifndef _CPP_IOSFWD
#include <iosfwd>
#endif

#ifndef AW_FONT_GROUP_HXX
#include <aw_font_group.hxx>
#endif

#ifndef SEC_ABSPOS_HXX
#include "SEC_abspos.hxx"
#endif
#ifndef SEC_GC_HXX
#include "SEC_gc.hxx"
#endif
#ifndef SEC_DB_HXX
#include "SEC_db.hxx"
#endif


using namespace AW;

#define DATA_VERSION 3

// ------------------
//      Debugging
// ------------------

#if defined(DEBUG)

#define CHECK_INTEGRITY         // check structure integrity after changes
#define PAINT_ABSOLUTE_POSITION // paint some positions near center (range = 0..len-1)

#endif // DEBUG

#ifdef CHECK_INTEGRITY
enum SEC_CHECK_TYPE {
    CHECK_STRUCTURE = 1,
    CHECK_SIZE      = 2,
    CHECK_POSITIONS = 4,
    CHECK_ALL       = CHECK_STRUCTURE|CHECK_SIZE|CHECK_POSITIONS, 
};
#endif

// -------------------
//      SEC_region
// -------------------

class SEC_root;

class SEC_region {
private:
    /* non redundant values */
    int sequence_start, sequence_end; // sequence_end is exclusive

    /* cached values */
    int baseCount;             // number of real bases (-1 = uninitialized)

    int * abspos_array;
#if defined(DEBUG)
    int   abspos_array_size;
#endif                          // DEBUG

    void create_abspos_array(const int *static_array);

    void set_base_count(int bc) {
        sec_assert(bc>0);
        baseCount = bc;
    }

    void count_bases(SEC_root *root); // updates abspos_array
    
public:
    SEC_region(int start, int end);
    virtual ~SEC_region();

    //methods
    void save(std::ostream & out, int indent, const XString& x_string);
    GB_ERROR read(std::istream & in, SEC_root *root, int version);

    void update_base_count(SEC_root *root) { if (baseCount == -1) count_bases(root); }
    void invalidate_base_count();

    //selector-methods
    int get_sequence_start() const { return sequence_start; }
    int get_sequence_end() const { return sequence_end; }
    
    int get_base_count() const { sec_assert(baseCount != -1); return baseCount; }

    bool contains_seq_position(int pos) const {
        if (sequence_end<sequence_start) {
            return pos<sequence_end || sequence_start <= pos;
        }
        return sequence_start <= pos && pos < sequence_end;
    }

    void set_sequence_portion(int start, int end) {
        sequence_start = start;
        sequence_end = end;
        invalidate_base_count();
    }
    void generate_x_string(XString& x_string);
    void align_helix_strands(SEC_root *root, SEC_region *other_region);

    int getBasePos(int basenr) const {
        // some helix positions do not have an absolute position
        // in that case getBasePos() returns a neighbour position
        int pos;

        if (basenr >= 0 && basenr<get_base_count()) {
            sec_assert(basenr >= 0);
            sec_assert(abspos_array);
            sec_assert(basenr<abspos_array_size);
            pos = abspos_array[basenr];
        }
        else { // special case for empty strands
            sec_assert(get_base_count() == 0);
            sec_assert(basenr <= 0); // 0 or -1
            
            pos = (basenr == 0) ? get_sequence_start() : get_sequence_end();
        }
        return pos;
    }

#if defined(CHECK_INTEGRITY)
    void check_integrity(const SEC_root *root, SEC_CHECK_TYPE what) const;
#endif // CHECK_INTEGRITY
};

// -------------------------
//      SEC_constrainted
// -------------------------

class SEC_constrainted {
    double sSize;               // standard size
    double dSize;               // constrainted size ( = drawn size)
    double Min, Max;            // limits for dSize (0 means : do not limitate)

    void refreshDrawnSize() { dSize = (sSize<Min && Min>0) ? Min : ((sSize>Max && Max>0) ? Max : sSize); }

public:
    SEC_constrainted() : sSize(0), dSize(0), Min(0), Max(0) {}

    double drawnSize() const { return dSize; } // constrainted size
    double standardSize() const { return sSize; } // // unconstrainted size

    double minSize() const { return Min; } // constraints
    double maxSize() const { return Max; }

    void setDrawnSize(double size) { // calculate constraints as needed
        if (sSize < size) {
            Min = size;
            if (Min>Max) Max = 0;
            refreshDrawnSize();
        }
        else if (sSize > size) {
            Max = size;
            if (Max<Min) Min = 0;
            refreshDrawnSize();
        }

#if defined(DEBUG) && 0
        printf("setDrawnSize(%.2f) -> sSize=%.2f dSize=%.2f Min=%.2f Max=%.2f\n", size, sSize, dSize, Min, Max);
#endif // DEBUG
    }

    void setStandardSize(double size) { // set standard size (calculated from base counts)
        sSize = size;
        refreshDrawnSize();
    }

    void setConstraints(double low, double high) {
        Min = low;
        Max = high;
        refreshDrawnSize();
    }
};

// ---------------------
//      SEC_oriented
// ---------------------

class SEC_base;

class SEC_oriented {
    Angle rel_angle;
    
    mutable Angle abs_angle;
    mutable bool  abs_angle_valid;

    const Angle& calc_abs_angle() const;
    const Angle& calc_rel_angle();

    virtual SEC_base *get_parent() = 0;
public:
    SEC_oriented() : abs_angle_valid(false) {}
    virtual ~SEC_oriented() {}

    virtual void invalidate_sub_angles() = 0;
    void invalidate(); // invalidates cached abs_angle of this and substructure

    const Angle& get_abs_angle() const { return abs_angle_valid ? abs_angle : calc_abs_angle(); }
    const Angle& get_rel_angle() const { return rel_angle; }

    void set_rel_angle(const Angle& rel) {
        rel_angle = rel;
        abs_angle_valid = false;
        invalidate_sub_angles();
    }
    void set_abs_angle(const Angle& abs) {
        if (abs_angle_valid) {
            Angle diff  = abs-get_abs_angle();
            sec_assert(rel_angle.normal().is_normalized());
            rel_angle  += diff;
            abs_angle   = abs;
        }
        else {
            abs_angle = abs;
            calc_rel_angle();
        }
        invalidate_sub_angles();
    }
    void mark_angle_absolute() { abs_angle = rel_angle; abs_angle_valid = true; } // used for root-loop (rel == abs!!)

    SEC_base *parent() { return get_parent(); }
    const SEC_base *parent() const { return const_cast<SEC_oriented*>(this)->get_parent(); }
};


// ----------------------
//      SEC_BASE_TYPE
// ----------------------

enum SEC_BASE_TYPE {
    SEC_NO_TYPE  = 0, 
    SEC_LOOP     = 1,
    SEC_HELIX    = 2,
    SEC_ANY_TYPE = SEC_LOOP|SEC_HELIX,
};

// -----------------
//      SEC_base
// -----------------

class SEC_base : public SEC_constrainted, public SEC_oriented, Noncopyable { // loop or helix
    SEC_root *root;
    
    virtual SEC_base *get_parent() = 0;
public:
    SEC_base(SEC_root *Root) : root(Root) {}
    virtual ~SEC_base() {}

    virtual SEC_BASE_TYPE getType() const        = 0;
    virtual const Position& get_fixpoint() const = 0;
    virtual void reset_angles()                  = 0; // resets all strand-loop angles (of substructure)

    virtual void orientationChanged() = 0; // recalc coordinates
    virtual void sizeChanged() = 0; // recalc size and coordinates

    AW_CL self() const { return (AW_CL)this; }
    SEC_root *get_root() const { return root; }
};

class SEC_base_part : Noncopyable { // segment or strand
    SEC_region region;

    virtual SEC_base *get_parent()    = 0;
    virtual SEC_base_part *get_next() = 0;
public:
    SEC_base_part() : region(-1, -1) {}
    virtual ~SEC_base_part() {}

    SEC_base *parent() { return get_parent(); }
    const SEC_base *parent() const { return const_cast<SEC_base_part*>(this)->get_parent(); }
    
    AW_CL self() const { return parent()->self(); }
    SEC_root *get_root() const { return parent()->get_root(); }

    SEC_base_part *next() { return get_next(); } // iterates through whole structure
    const SEC_base_part *next() const { return const_cast<SEC_base_part*>(this)->get_next();}

    const SEC_region *get_region() const { return &region; }
    SEC_region *get_region() { return &region; }

    void set_sequence_portion(int start, int end) { get_region()->set_sequence_portion(start, end); }
    size_t getNextAbspos() const;
};


// ------------------
//      SEC_helix
// ------------------

class SEC_helix_strand;
class SEC_loop;

class SEC_helix : public SEC_base {
    
    SEC_helix_strand *strand_to_root;
    size_t base_length; // max. # of bases in any strand

    SEC_base *get_parent();
public:

    SEC_helix(SEC_root *root, SEC_helix_strand *to_root, SEC_helix_strand *from_root);
    virtual ~SEC_helix() {}

    void calculate_helix_size();
    void calculate_helix_coordinates(); // assumes root-side loop has correct coordinates

    void save(std::ostream & out, int indent, const XString& x_string);
    GB_ERROR read(std::istream & in, int version, double& old_angle_in);

    size_t get_base_length()            { return base_length; }

    SEC_helix_strand *strandToRoot() const { return strand_to_root; } // strand pointing to root 
    SEC_helix_strand *strandToOutside() const; // strand pointing away from root
    
    SEC_helix_strand *strandAwayFrom(const SEC_loop *loop) const; // strand pointing away from loop
    SEC_helix_strand *strandTowards(const SEC_loop *loop) const; // strand pointing to loop

    SEC_loop *otherLoop(const SEC_loop *loop) const; // returns the other loop
    SEC_loop *rootsideLoop() const;
    SEC_loop *outsideLoop() const;

    bool hasLoop(SEC_loop *loop) const { return loop == rootsideLoop() || loop == outsideLoop(); }

    void setFixpoints(const Position& rootside, const Position& outside);

    void flip();

    void fixAngleBugs(int version);

#if defined(CHECK_INTEGRITY)
    void check_integrity(SEC_CHECK_TYPE what) const;
#endif // CHECK_INTEGRITY

    // SEC_oriented interface:
    void invalidate_sub_angles();

    // SEC_base interface :
    SEC_BASE_TYPE getType() const { return SEC_HELIX; }
    void reset_angles();
    const Position& get_fixpoint() const;
    
    void orientationChanged(); // recalc coordinates
    void sizeChanged(); // recalc size and coordinates
};

// -------------------------
//      SEC_helix_strand
// -------------------------

class SEC_segment;

class SEC_helix_strand : public SEC_base_part {
    friend class SEC_helix;
    
    SEC_loop         *origin_loop;     // Pointer to loop where strand comes from
    SEC_helix_strand *other_strand;
    SEC_helix        *helix_info; // used by both strands
    SEC_segment      *next_segment; // next segment in origin_loop

    //redundant values

    Position fixpoint;
    Position rightAttach, leftAttach; // rightAttach was ap1, leftAttach was ap2

    void set_helix_info(SEC_helix *helix_info_)            { helix_info = helix_info_; }
    void set_other_strand(SEC_helix_strand *other_strand_) { other_strand = other_strand_; }

    // SEC_base_part interface
    SEC_base *get_parent() { return helix_info; }
    SEC_base_part *get_next();
    
public:

    SEC_helix_strand();
    virtual ~SEC_helix_strand();

    GB_ERROR read(SEC_loop *loop_, std::istream & in, int version);

    void paint(AW_device *device);
    void unlink(bool fromOtherStrandAsWell);
    
    void paint_strands(AW_device *device, const Vector& strand_dir, const double& strand_length);
    void paint_constraints(AW_device *device);
    
    const SEC_root *get_root() const { return helix_info->get_root(); }
    SEC_root *get_root() { return helix_info->get_root(); }

    const SEC_helix *get_helix() const { return helix_info; }
    SEC_helix *get_helix() { return helix_info; }
    
    const SEC_helix_strand *get_other_strand() const { return other_strand; }
    SEC_helix_strand *get_other_strand() { return other_strand; }

    // fix- and attach points

    const Position& get_fixpoint() const { return fixpoint; }
    bool isRootsideFixpoint() const { return helix_info->strandToOutside() == this; }

    bool pointsToRoot() const { return !isRootsideFixpoint(); }
    bool pointsToOutside() const { return isRootsideFixpoint(); }
    
    bool is3end() const { return get_region()->get_sequence_start() > other_strand->get_region()->get_sequence_start(); }

    // Attach point (left/right when looking towards the strand from its origin loop)
    const Position& rightAttachPoint() const { return rightAttach; }
    const Position& leftAttachPoint() const { return leftAttach; }
    
    const Position& startAttachPoint() const { return leftAttach; }
    const Position& endAttachPoint() const { return other_strand->rightAttach; }

    int rightAttachAbspos() const {
        const SEC_region *reg   = get_other_strand()->get_region();
        int               count = reg->get_base_count();
        
        return reg->getBasePos(count ? count-1 : 0);
    }
    int leftAttachAbspos() const { return get_region()->getBasePos(0); }

    int startAttachAbspos() const { return leftAttachAbspos(); }
    int endAttachAbspos() const { return other_strand->rightAttachAbspos(); }

    void setFixpoint(const Position& p) { fixpoint = p; }
    void setAttachPoints(const Position& left, const Position& right) { rightAttach = right; leftAttach = left; }

    // interator methods

    const SEC_segment *get_next_segment() const { return next_segment; }
    SEC_segment *get_next_segment() { return next_segment; }
    SEC_segment * get_previous_segment(); // expensive!

    const SEC_loop *get_origin_loop() const { return origin_loop; }
    SEC_loop *get_origin_loop() { return origin_loop; }
    SEC_loop *get_destination_loop() { return get_other_strand()->get_origin_loop(); }
    
    SEC_loop *get_rootside_loop() { return isRootsideFixpoint() ? get_origin_loop() : get_destination_loop(); }

    void set_origin_loop(SEC_loop *loop_)                       { origin_loop = loop_; }
    void set_next_segment(SEC_segment *next_segment_)           { next_segment=next_segment_; }

#if defined(CHECK_INTEGRITY)
    void check_integrity(SEC_CHECK_TYPE what) const;
#endif // CHECK_INTEGRITY
};


// --------------------
//      SEC_segment
// --------------------

class SEC_segment : public SEC_base_part {
private:
    double   alpha;             // angle of segment (i.e. how much of the loop is used by this segment)
    Position center1, center2; // segments are not circles, they are ellipsoids
    // center1 is used for rightAttach (of previous helix)
    // center2 is used for leftAttach (of next helix)

    SEC_helix_strand *next_helix_strand; // next helix strand after segment (pointing away from segments loop)
    SEC_loop *loop; // the loop containing 'this'
    
    // SEC_base_part interface
    SEC_base *get_parent();
    SEC_base_part *get_next() { return get_next_strand(); }
    
public:

    SEC_segment();
    virtual ~SEC_segment();

    void save(std::ostream & out, int indent, const XString& x_string);
    GB_ERROR read(SEC_loop *loop_,std::istream & in, int version);

    void calculate_segment_size();
    void calculate_segment_coordinates(const Position& start, const Position& end);

    void paint(AW_device *device, SEC_helix_strand *previous_strand_pointer);
    void unlink();
    
    void prepare_paint(SEC_helix_strand *previous_strand_pointer, double &gamma, double &eta, double &radius, int &base_count, double &angle_step);

    void mergeWith(SEC_segment *other, SEC_loop *target_loop);
    SEC_helix_strand *split(size_t start, size_t end, SEC_segment **new_segment);
    

    int is_endings_segment() {
        int seq_start = get_region()->get_sequence_start();
        int seq_end = get_region()->get_sequence_end();

        return seq_start>seq_end; 
    }

    void delete_pointer_2(SEC_helix_strand *strand) {
        SEC_segment *segment = this;
        while (1) {
            SEC_helix_strand *next_strand = segment->next_helix_strand;

            if (!next_strand) break;
            if (next_strand == strand) { segment->next_helix_strand = NULL; break; }
            
            segment = next_strand->get_next_segment();
            if (!segment || segment==this) {
#if defined(DEBUG)
                printf("SEC_segment %p did not contain pointer to SEC_helix_strand %p\n", this, strand);
#endif // DEBUG
                break;
            }
        }
    }

    SEC_helix_strand *get_previous_strand();

    const SEC_helix_strand *get_next_strand() const { return next_helix_strand; }
    SEC_helix_strand *get_next_strand() { return next_helix_strand; }
    
    const SEC_loop *get_loop() const { return loop; }
    SEC_loop *get_loop() { return loop; }

    double get_alpha() { return alpha; }

    void set_next_strand(SEC_helix_strand *strand) { next_helix_strand = strand; }
    void set_loop(SEC_loop *loop_) { loop = loop_; }
    
#if defined(CHECK_INTEGRITY)
    void check_integrity(SEC_CHECK_TYPE what) const;
#endif // CHECK_INTEGRITY
};

// -----------------
//      SEC_loop
// -----------------

class SEC_loop : public SEC_base {
    double   Circumferance;     // unit is in "segment-base-distances"
    Position center;            // center point of loop
    SEC_helix_strand *primary_strand; // primary strand of loop
    // loop orientation points towards that strand
    // for non-root-loops, this strand points towards root
    
    void compute_circumferance();
    void compute_radius();

    SEC_base *get_parent() { return is_root_loop() ? 0 : get_rootside_helix(); }

public:

    SEC_loop(SEC_root *root_);
    virtual ~SEC_loop();
    
    void save(std::ostream & out, int indent, const XString& x_string);
    GB_ERROR read(SEC_helix_strand *rootside_strand, std::istream & in, int version, double loop_angle);
    
    void calculate_loop_size();
    void calculate_loop_coordinates();

    void paint(AW_device *device);
    void paint_constraints(AW_device *device);

    const Position& get_center() const { return center; }
    const double& get_circumferance() const { return Circumferance; } 

    bool is_root_loop() const;

    SEC_helix_strand *get_rootside_strand() const { return is_root_loop() ? 0 : primary_strand; }
    SEC_helix *get_rootside_helix() const { return is_root_loop() ? 0 : primary_strand->get_helix(); }
    SEC_helix_strand *get_fixpoint_strand() const { return primary_strand; }
    SEC_helix *get_fixpoint_helix() const { return primary_strand->get_helix(); }

    void set_fixpoint_strand(SEC_helix_strand *strand) { primary_strand = strand; }

    // void flip_rootside_helices(SEC_helix_strand *new_fixpoint_strand, const Angle& new_rel_angle);
    void toggle_root(SEC_loop *root_loop);

    void set_center(const Position& p) { center = p; }

    void fixAngleBugs(int version);

#if defined(CHECK_INTEGRITY)
    void check_integrity(SEC_CHECK_TYPE what) const;
#endif // CHECK_INTEGRITY

    // SEC_oriented interface: 
    void invalidate_sub_angles();

    // SEC_base interface :
    SEC_BASE_TYPE getType() const { return SEC_LOOP; }
    void reset_angles();
    
    const Position& get_fixpoint() const {
        // Note: does not return center for root-loop.
        SEC_helix *helix = get_fixpoint_helix();
        return helix->strandAwayFrom(this)->get_fixpoint();
    }

    void orientationChanged();  // recalc coordinates
    void sizeChanged(); // recalc size and coordinates
};

// --------------------------
//      SEC_displayParams
// --------------------------

enum ShowBonds {
    SHOW_NO_BONDS     = 0,
    SHOW_HELIX_BONDS  = 1,
    SHOW_NHELIX_BONDS = 2, 
};

enum ShowCursorPos {
    SHOW_NO_CURPOS    = 0,
    SHOW_ABS_CURPOS   = 1,
    SHOW_ECOLI_CURPOS = 2,
    SHOW_BASE_CURPOS  = 3,
};

struct SEC_displayParams {
    bool   show_helixNrs;       // display helix number information?
    double distance_between_strands; // distance between strands (1.0 => strand distance == normal distance of bases in loop)

    ShowBonds show_bonds;       // which bonds to show
    int       bond_thickness;   // linewidth for bonds

    bool hide_bases;            // hide bases?

    ShowCursorPos show_curpos;  // which position to show at cursor
    bool          show_ecoli_pos; // show ecoli positions?
    
    bool display_search;        // show search results
    bool display_sai;           // visualize SAIs

    bool show_strSkeleton;      // display the skeleton?
    int  skeleton_thickness;
    
    bool edit_direction;        // true = 5'->3', false = 5'<-3'

#if defined(DEBUG)
    bool show_debug;            // show debug info in structure display
#endif // DEBUG
    
    void reread(AW_root *aw_root);
};


// -----------------
//      SEC_root
// -----------------

class AWT_canvas;
class SEC_drawn_positions;
class SEC_db_interface;
class SEC_graphic;

enum SEC_bgpaint_mode {
    BG_PAINT_NONE   = 0, 
    BG_PAINT_FIRST  = 1,
    BG_PAINT_SECOND = 2,
    BG_PAINT_BOTH   = BG_PAINT_FIRST | BG_PAINT_SECOND,
};

class SEC_root {
    SEC_loop *root_loop;
    int       cursorAbsPos;     // cursor position (-1 == unset)
    XString  *xString;

    bool constructing; // whether structure is under construction or a complete ring-structure

    SEC_displayParams  displayParams;
    SEC_db_interface  *db;

    // -----------------------------
    //      updated before paint
    // -----------------------------
    
    AW_font_group font_group;

    double char_radius[SEC_GC_DATA_COUNT]; // radius and..
    double bg_linewidth[SEC_GC_DATA_COUNT]; // ..linewidth for drawing background (index = gc)
    Vector center_char[SEC_GC_FONT_COUNT]; // correction vector to center the base character at its position (world coordinates)

    char *bg_color;       // only valid after paint (contains EDIT4 GCs), may be NULL

    Vector   *autoscroll;       // if non-zero, scroll canvas before next paint
    int       nailedAbsPos;     // if not -1, auto-scroll such that position does not move
    Position  drawnAbsPos;      // position where nailedAbsPos was drawn before

    // --------------------------
    //      valid after paint
    // --------------------------

    SEC_drawn_positions *drawnPositions; // after paint this contains draw positions for every absolute position
    LineVector           cursor_line; // main line of the cursor

    SEC_BASE_TYPE show_constraints; 

    
    void paintHelixNumbers(AW_device *device);
    void paintEcoliPositions(AW_device *device);
#if defined(PAINT_ABSOLUTE_POSITION)
    void showSomeAbsolutePositions(AW_device *device);
#endif
    void fixStructureBugs(int version);
    
    void cacheBackgroundColor();

    static bool hasBase(int pos, const char *seq, int len) {
        sec_assert(pos<len);
        char c = seq[pos];
        return c != '-' && c != '.';
    }

public:
    
    SEC_root();
    ~SEC_root();

    void init(SEC_graphic *gfx, AWT_canvas *ntw);

    bool under_construction() const { return constructing; }
    void set_under_construction(bool construct) { constructing = construct; }

    const SEC_db_interface *get_db() const { return db; }
    bool canDisplay() const { return db && db->canDisplay(); }
    const BI_helix *get_helix() const { sec_assert(db); return db->helix(); }
    BI_PAIR_TYPE getBondtype(int abspos) { return get_helix() ? get_helix()->pairtype(abspos) : HELIX_NONE; }
    const char *helixNrAt(int abspos) const { return get_helix()->helixNr(abspos); }

    const size_t *getHelixPositions(const char *helixNr) const;
    const double& get_char_radius(int gc) const { return char_radius[gc]; }

    void reread_display_params(AW_root *aw_root) { displayParams.reread(aw_root); }
    const SEC_displayParams& display_params() const { return displayParams; }

    bool has_xString() const { return xString; }
    const XString& get_xString() const {
        sec_assert(xString);
        return *xString;
    }

#if defined(CHECK_INTEGRITY)
    void check_integrity(SEC_CHECK_TYPE what) const;
#endif // CHECK_INTEGRITY

    // ------------------------------

    void paintBackgroundColor(AW_device *device, SEC_bgpaint_mode mode, const Position& p1, int color1, int gc1, const Position& p2, int color2, int gc2, int skel_gc, AW_CL cd1, AW_CL cd2);
    void paintSearchPatternStrings(AW_device *device, int clickedPos,  AW_pos xPos,  AW_pos yPos);

    char *buildStructureString();
    GB_ERROR read_data(const char *input_string, const char *x_string_in);

    void add_autoscroll(const Vector& scroll);
    void nail_position(size_t absPos); // re-position on absPos
    void nail_cursor(); // re-position on cursor
    void position_cursor(bool toCenter, bool evenIfVisible); // scroll/center cursor (screen-only)
    void set_cursor(int abspos, bool performRefresh); // sets new cursor position

    bool perform_autoscroll();

private: 
    void calculate_size();
    void calculate_coordinates();
public:

#if defined(CHECK_INTEGRITY)
    void recalc() {
        calculate_coordinates();
    }
    void relayout() {
        calculate_size();
        calculate_coordinates();
    }
#else
    void recalc() {                     check_integrity(static_cast<SEC_CHECK_TYPE>(CHECK_STRUCTURE|CHECK_SIZE));
        calculate_coordinates();        check_integrity(CHECK_POSITIONS);
    }
    void relayout() {                   check_integrity(CHECK_STRUCTURE);
        calculate_size();               check_integrity(CHECK_SIZE);
        calculate_coordinates();        check_integrity(CHECK_POSITIONS);
    }
#endif

    GB_ERROR split_loop(int start1, int end1, int start2, int end2);

    GB_ERROR paint(AW_device *device);
    GB_ERROR unsplit_loop(SEC_helix_strand *delete_strand);
    void     set_root(SEC_loop *loop);
    void     create_default_bone();
    void     generate_x_string();

    void update_shown_positions();
    bool shallDisplayPosition(int abspos) const { return db->shallDisplayPosition(abspos); }
    void invalidate_base_positions(); // force base counts of all regions to be refreshed 

    int getBackgroundColor(int abspos) { return bg_color ? bg_color[abspos] : 0; }
    const Vector& get_center_char_vector(int gc) {
        sec_assert(gc >= SEC_GC_FIRST_FONT && gc <= SEC_GC_LAST_FONT);
        return center_char[gc];
    }

    size_t max_index() {
        size_t len = db->length();
        sec_assert(len); // zero len -> no index exists
        return len-1;
    }
    
    int get_cursor() const { return cursorAbsPos; }

    SEC_loop *get_root_loop() const { return root_loop; }
    void set_root_loop(SEC_loop *loop) { root_loop = loop; }

    SEC_BASE_TYPE get_show_constraints() { return show_constraints; }
    void set_show_constraints(SEC_BASE_TYPE show) { show_constraints = show; }

    void set_last_drawed_cursor_position(const LineVector& line) { cursor_line = line; }
    const LineVector& get_last_drawed_cursor_position() const { return cursor_line; }
    void clear_last_drawed_cursor_position() { set_last_drawed_cursor_position(LineVector()); } // invalidate cursor_line

    SEC_base_part *find(int pos); // find part containing position pos
    
    void announce_base_position(int base_pos, const Position& draw_pos);
    void clear_announced_positions();

    const AW_font_group& get_font_group() const { return font_group; }


    // draw annotation to explicit coordinates (annotation is drawn "above" line left->right)
    void paintAnnotation(AW_device *device, int gc,
                         const Position& annotate, const Position& left, const Position& right,
                         double noteDistance, const char *text,
                         bool lineToAnnotated, bool linesToLeftRight, bool boxText,
                         AW_CL cd1, AW_CL cd2);

    // draw a annotation next to a base (only works after paint()) 
    void paintPosAnnotation(AW_device *device, int gc, size_t absPos, const char *text, bool lineToBase, bool boxText); 
};


// --------------------------------------------------------------------------------
// inlines:
// --------------------------------------------------------------------------------

inline void SEC_helix::flip() {
    strand_to_root = strand_to_root->get_other_strand();
}

inline SEC_helix_strand *SEC_helix::strandToOutside() const { // strand pointing away from root
    return strandToRoot()->get_other_strand();
}

inline SEC_helix_strand *SEC_helix::strandAwayFrom(const SEC_loop *loop) const { // strand pointing away from loop
    if (strandToRoot()->get_origin_loop() == loop) {
        return strandToRoot();
    }
    sec_assert(strandToOutside()->get_origin_loop() == loop);
    return strandToOutside();
}

inline SEC_helix_strand *SEC_helix::strandTowards(const SEC_loop *loop) const { // strand pointing to loop
    return strandAwayFrom(loop)->get_other_strand();
}

inline SEC_loop *SEC_helix::otherLoop(const SEC_loop *loop) const { // returns loop on other side of strand
    return strandTowards(loop)->get_origin_loop();
}

inline SEC_loop *SEC_helix::rootsideLoop() const { return strandToOutside()->get_origin_loop(); }
inline SEC_loop *SEC_helix::outsideLoop() const { return strandToRoot()->get_origin_loop(); }

inline const Position& SEC_helix::get_fixpoint() const { return strandToOutside()->get_fixpoint(); }

inline void SEC_helix::setFixpoints(const Position& rootside, const Position& outside) {
    strandToRoot()->setFixpoint(outside);
    strandToOutside()->setFixpoint(rootside);
}

inline void SEC_helix::orientationChanged() { // recalc coordinates
    // we need to recalculate the rootside loop, cause changing the
    // helix-orientation affects the attached loop segments.

    SEC_loop *loop = rootsideLoop();

    if (loop->is_root_loop()) {
        // at root-loop do a complete recalc
        // (fixpoint-strand is not relayouted otherwise)
        get_root()->recalc();
    }
    else {
#if defined(CHECK_INTEGRITY)
        loop->check_integrity(CHECK_STRUCTURE);
        loop->check_integrity(CHECK_SIZE);
#endif // CHECK_INTEGRITY
        loop->calculate_loop_coordinates();
#if defined(CHECK_INTEGRITY)
        loop->check_integrity(CHECK_POSITIONS);
#endif // CHECK_INTEGRITY
    }
}
inline void SEC_helix::sizeChanged() { // recalc size and coordinates
#if defined(CHECK_INTEGRITY)
    check_integrity(CHECK_STRUCTURE);
#endif // CHECK_INTEGRITY
    calculate_helix_size();
#if defined(CHECK_INTEGRITY)
    check_integrity(CHECK_SIZE);
#endif // CHECK_INTEGRITY
    calculate_helix_coordinates();
#if defined(CHECK_INTEGRITY)
    check_integrity(CHECK_POSITIONS);
#endif // CHECK_INTEGRITY
}

inline SEC_base *SEC_helix::get_parent() { return rootsideLoop(); }

// --------------------

inline SEC_base_part *SEC_helix_strand::get_next() { return get_other_strand()->get_next_segment(); }

// --------------------

inline bool SEC_loop::is_root_loop() const { return get_root()->get_root_loop() == this; }

inline void SEC_loop::orientationChanged() { // recalc coordinates
    if (is_root_loop()) {
        get_root()->recalc();
    }
    else {
        // loop center is calculated by helix, that is why we recalc the helix here
        
        SEC_helix *helix = get_fixpoint_helix();
#if defined(CHECK_INTEGRITY)
        helix->check_integrity(CHECK_STRUCTURE);
        helix->check_integrity(CHECK_SIZE);
#endif // CHECK_INTEGRITY
        helix->calculate_helix_coordinates();
#if defined(CHECK_INTEGRITY)
        helix->check_integrity(CHECK_POSITIONS);
#endif // CHECK_INTEGRITY
    }
}
inline void SEC_loop::sizeChanged() { // recalc size and coordinates
    if (is_root_loop()) {
        get_root()->relayout();
    }
    else {
        SEC_helix *helix = get_fixpoint_helix();
#if defined(CHECK_INTEGRITY)
        helix->check_integrity(CHECK_STRUCTURE);
#endif // CHECK_INTEGRITY
        helix->calculate_helix_size();
#if defined(CHECK_INTEGRITY)
        helix->check_integrity(CHECK_SIZE);
#endif // CHECK_INTEGRITY
        helix->calculate_helix_coordinates();
#if defined(CHECK_INTEGRITY)
        helix->check_integrity(CHECK_POSITIONS);
#endif // CHECK_INTEGRITY
    }
}

// --------------------

inline SEC_base *SEC_segment::get_parent() { return loop; }

// --------------------

inline bool are_adjacent_regions(const SEC_region *reg1, const SEC_region *reg2) {
    int end1   = reg1->get_sequence_end();
    int start2 = reg2->get_sequence_start();

    if (end1 == start2) return true;

    return start2 == 0;
}


#else
#error SEC_root.hxx included twice
#endif 

