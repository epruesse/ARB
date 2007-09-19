#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <iostream>
#include <sstream>

#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt_canvas.hxx>
#include <awt_iupac.hxx>
#include <BI_helix.hxx>

#include <ed4_extern.hxx>

#include "SEC_root.hxx"
#include "SEC_graphic.hxx"
#include "SEC_iter.hxx"
#include "SEC_drawn_pos.hxx"
#include "SEC_bonddef.hxx"
#include "SEC_toggle.hxx"

using namespace std;

// ------------------
//      Debugging
// ------------------

#if defined(DEBUG)
// #define PAINT_REGION_INDEX // // paint region-internal index next to base

static bool valid_cb_params(AW_CL cd1, AW_CL cd2) {
    return cd1 == 0 || cd2 != -1;
}

static void paintDebugInfo(AW_device *device, int color, const Position& pos, const char *txt, AW_CL cd1, AW_CL cd2) {
    sec_assert(valid_cb_params(cd1, cd2));
    device->circle(color, true, pos.xpos(), pos.ypos(), 0.06, 0.06, -1, cd1, cd2);
    device->text(SEC_GC_DEFAULT, txt, pos.xpos(), pos.ypos(), 0, 1, cd1, cd2, 0);
}
static void paintStrandDebugInfo(AW_device *device, int color, SEC_helix_strand *strand) {
    paintDebugInfo(device, color, strand->rightAttachPoint(), "RAP", strand->self(), strand->rightAttachAbspos());
    paintDebugInfo(device, color, strand->leftAttachPoint(), "LAP", strand->self(), strand->leftAttachAbspos());
    paintDebugInfo(device, color, strand->get_fixpoint(), strand->isRootsideFixpoint() ? "RFP" : "FP", strand->self(), strand->startAttachAbspos());
}

#endif // DEBUG

// ------------------
//      PaintData
// ------------------

class PaintData {
    int gc_edit4_to_secedit[ED4_G_DRAG+1]; // GC translation table (EDIT4 -> SECEDIT)
    int line_property_gc[SEC_GC_LAST_DATA+1][SEC_GC_LAST_DATA+1]; // of two GCs, which is responsible for line properties

public:
    PaintData() {
        int gc;

        // GC translation (EDIT4->SECEDIT)
        for (gc = 0; gc <= ED4_G_DRAG; gc++) {
            gc_edit4_to_secedit[gc] = -1; // invalid
        }
        for (gc = ED4_G_SBACK_0; gc <= ED4_G_SBACK_8; gc++) {
            gc_edit4_to_secedit[gc] = gc-ED4_G_SBACK_0+SEC_GC_SBACK_0;
        }
        for (gc = ED4_G_CBACK_0; gc <= ED4_G_CBACK_9; gc++) {
            gc_edit4_to_secedit[gc] = gc-ED4_G_CBACK_0+SEC_GC_CBACK_0;
        }

        // calc line property GCs
        for (gc = SEC_GC_FIRST_DATA; gc <= SEC_GC_LAST_DATA; gc++) {
            for (int gc2 = SEC_GC_FIRST_DATA; gc <= SEC_GC_LAST_DATA; gc++) {
                int prop_gc;
                if (gc == gc2) {
                    prop_gc = gc;
                }
                else {
                    if (gc == SEC_GC_LOOP || gc2 == SEC_GC_LOOP) {
                        prop_gc = SEC_GC_LOOP; // use loop-properties in loop and at loop-helix-transition 
                    }
                    else if (gc == SEC_GC_NHELIX || gc2 == SEC_GC_NHELIX) {
                        prop_gc = SEC_GC_NHELIX; // use nhelix-properties in nhelix and at helix-nhelix-transition
                    }
                    else {
                        prop_gc = SEC_GC_HELIX; // use helix-properties in helix
                    }
                }
                line_property_gc[gc][gc2] = prop_gc;
            }
        }
    }

    int convert_BackgroundGC(int edit4_gc) const {
        // returns -1 if edit4_gc is invalid
        sec_assert(edit4_gc >= 0 && edit4_gc <= ED4_G_DRAG);
        return gc_edit4_to_secedit[edit4_gc];
    }

    int get_linePropertyGC(int gc1, int gc2) {
        // of the GCs of two positions, it returns the GC which is
        // defining the properties for the background painted inbetween the two positions
        sec_assert(gc1 >= SEC_GC_FIRST_DATA && gc1 <= SEC_GC_LAST_DATA);
        sec_assert(gc2 >= SEC_GC_FIRST_DATA && gc2 <= SEC_GC_LAST_DATA);
        return line_property_gc[gc1][gc2];
    }
};

static PaintData paintData;

// --------------------
//      Annotations
// --------------------

void SEC_root::paintAnnotation(AW_device *device, int gc,
                               const Position& pos, const Position& left, const Position& right,
                               double noteDistance, const char *text,
                               bool lineToPos, bool linesToLeftRight, bool boxText,
                               AW_CL cd1, AW_CL cd2)
{
    // draw annotation to explicit position 'pos' (annotation is drawn "above" the line left->right)
    // The distance between pos and note is determined by
    // * textsize (minimal half textsize/boxsize) and
    // * the given 'noteDistance'  
    // lineToPos        == true -> draw a line from text to 'pos' 
    // linesToLeftRight == true -> draw lines from text to 'left' and 'right'
    // boxText          == true -> draw a box around text

    sec_assert(valid_cb_params(cd1, cd2));

    Vector strand(left, right);
    Angle  pos2note(strand);
    pos2note.rotate270deg();

    int    fontgc        = gc <= SEC_GC_LAST_FONT ? gc : SEC_GC_DEFAULT;
    double half_charSize = center_char[fontgc].length();
    size_t text_len      = strlen(text);

    // calculate textsize
    AW_pos half_width  = 0.5 * device->rtransform_size(device->get_string_size(gc, text, text_len));
    AW_pos half_height = center_char[fontgc].y();

    double note_distance  = max(half_height, half_width) * (boxText ? 1.3 : 1.0);
    note_distance         = max(note_distance, noteDistance);

    // Position note_center = pos + pos2note.normal()*(text_len*2*half_charSize);
    Position note_center = pos + pos2note.normal()*note_distance;

    if (device->filter & AW_PRINTER) {
        boxText = false; // don't print/xfig-export boxes
    }

    if (lineToPos || linesToLeftRight) {
        device->set_line_attributes(gc, 1, AW_SOLID);

        if (lineToPos) {
            Vector dist = pos2note.normal()*half_charSize;
            device->line(gc, boxText ? note_center : note_center-dist, pos+dist, -1, cd1, cd2);
        }
        if (linesToLeftRight) {
            Vector out(pos, note_center);

            if (out.length()*2 >= strand.length()) { // short strands -> draw simple bracket
                Vector toLeft(note_center, left);
                Vector toRight(note_center, right);

                device->line(gc, boxText ? note_center : note_center+toLeft*(half_width/toLeft.length()),
                             left-toLeft*(half_charSize/toLeft.length()), -1, cd1, cd2);
                device->line(gc, boxText ? note_center : note_center+toRight*(half_width/toRight.length()),
                             right-toRight*(half_charSize/toRight.length()), -1, cd1, cd2);

                // Vector dist = Vector(left, note_center).normalize()*half_charSize;
                // device->line(gc, boxText ? note_center : note_center-dist, left+dist, -1, cd1, cd2);
                // dist = Vector(right, note_center).normalize()*half_charSize;
                // device->line(gc, boxText ? note_center : note_center-dist, right+dist, -1, cd1, cd2);
            }
            else {
                Vector rightIndent = out;
                rightIndent.rotate270deg();

                Position rightOut = right+out+rightIndent;
                Position leftOut  = left+out-rightIndent;

                Vector posPad = Vector(right, rightOut).set_length(half_charSize);
                device->line(gc, right+posPad, rightOut, -1, cd1, cd2);
                posPad.rotate90deg();
                device->line(gc, left+posPad, leftOut, -1, cd1, cd2);

                if (boxText) {
                    device->line(gc, leftOut, rightOut, -1, cd1, cd2);
                }
                else {
                    Vector rightTextPad(note_center, rightOut);
                    rightTextPad.set_length(half_width);
                
                    device->line(gc, note_center+rightTextPad, rightOut, -1, cd1, cd2);
                    device->line(gc, note_center-rightTextPad, leftOut, -1, cd1, cd2);
                }
            }
        }
    }

    Vector   center_textcorner(-half_width, half_height); // from center to lower left corner
    Position textcorner = note_center+center_textcorner;

    if (boxText) {
        Vector    center_corner(-half_width-half_height*0.3, half_height*1.3); // box is 25% bigger than text
        Rectangle box(note_center+center_corner, -2*center_corner);

        device->clear_part(box, -1);
        device->box(gc, false, box, -1, cd1, cd2);
    }
    
    device->text(gc, text, textcorner, 0, -1, cd1, cd2, 0);
}

void SEC_root::paintPosAnnotation(AW_device *device, int gc, size_t absPos, const char *text, bool lineToBase, bool boxText) {
    // draw a annotation next to a base (only works after paint()).
    // if nothing was drawn at absPos, annotate a position between previous and next drawn position.
    // text       == NULL    -> draw absPos as number
    // lineToBase == true    -> draw a line to the base itself
    // boxText    == true    -> draw a box around text

    size_t          abs1, abs2;
    const Position& pos1 = drawnPositions->drawn_before(absPos, &abs1);
    const Position& pos2 = drawnPositions->drawn_after (absPos, &abs2);

    LineVector vec12(pos1, pos2);
    Position   mid12 = vec12.centroid();
    Position   pos;
    {
        const Position *posDrawn = drawnPositions->drawn_at(absPos);
        if (posDrawn) { // absPos was drawn
            pos = *posDrawn;
        }
        else { // absPos was not drawn -> use position inbetween
            pos = mid12;
        }
    }

    if (!text) text = GBS_global_string("%u", absPos);
    
    paintAnnotation(device, gc, pos, pos1, pos2, vec12.length(), text, lineToBase, false, boxText, 0, absPos);
}

void SEC_root::paintEcoliPositions(AW_device *device) {
    long abspos = db->ecoli()->rel_2_abs(0);
    paintPosAnnotation(device, SEC_GC_ECOLI, size_t(abspos), "1", true, true);

    const BI_ecoli_ref *ecoli = db->ecoli();
    for (int ep = 99; ep < ecoli->base_count(); ep += 100) {
        abspos = ecoli->rel_2_abs(ep);
        paintPosAnnotation(device, SEC_GC_ECOLI, size_t(abspos), GBS_global_string("%u", ep+1), true, true);
    }
}

void SEC_root::paintHelixNumbers(AW_device *device) {
    for (SEC_base_iterator elem(this); elem; ++elem) {
        if (elem->getType() == SEC_HELIX) {
            SEC_helix& helix = static_cast<SEC_helix&>(*elem);

            // paint helix number of right (3') helix strand
            SEC_helix_strand *strand = helix.strandToRoot()->is3end() ? helix.strandToRoot() : helix.strandToOutside();

            int         absPos  = strand->startAttachAbspos();
            const char *helixNr = helixNrAt(absPos);

            if (helixNr) {
                if (helix.standardSize() == 0) { // helix with zero length (just one position on each strand)
                    paintPosAnnotation(device, SEC_GC_HELIX_NO,
                                       strand->startAttachAbspos(), helixNr, true, true);
                }
                else {
                    const Position& start = strand->startAttachPoint();
                    const Position& end   = strand->endAttachPoint();
                    Position helixCenter           = centroid(start, end);

                    paintAnnotation(device, SEC_GC_HELIX_NO,
                                    helixCenter, start, end,
                                    // displayParams.distance_between_strands*2, 
                                    displayParams.distance_between_strands, 
                                    helixNr, false, true, true, strand->self(), absPos);
                }
            }
        }
    }
}


#if defined(PAINT_ABSOLUTE_POSITION)
void SEC_root::showSomeAbsolutePositions(AW_device *device) {
    if (device->filter != AW_SIZE) { // ignore for size calculation
        Rectangle screen(device->rtransform(device->get_area_size()));
        Vector        diag3 = screen.diagonal()/3;
        Rectangle showInside(screen.upper_left_corner()+diag3*1.85, diag3);

        device->box(SEC_GC_DEFAULT, false, showInside, -1, 0, -1);

        PosMap::const_iterator end = drawnPositions->end();
        for (PosMap::const_iterator pos = drawnPositions->begin(); pos != end; ++pos) {
            if (showInside.contains(pos->second)) {
                paintPosAnnotation(device, SEC_GC_DEFAULT, pos->first, NULL, true, true);
            }
        }
    }
}
#endif // PAINT_ABSOLUTE_POSITION

void SEC_root::announce_base_position(int base_pos, const Position& draw_pos) {
    drawnPositions->announce(base_pos, draw_pos);
}
void SEC_root::clear_announced_positions() {
    if (!drawnPositions) drawnPositions = new SEC_drawn_positions;
    drawnPositions->clear();
}


// ---------------------------
//      Paints CONSTRAINTS
// ---------------------------

void SEC_helix_strand::paint_constraints(AW_device *device) {
    double minS = helix_info->minSize();
    double maxS = helix_info->maxSize();

    if (minS>0 || maxS>0) {
        const Position& startP = startAttachPoint();
        const Position& endP   = endAttachPoint();

        bool drawMidLine = minS>0 && maxS>0;
        Position minP = startP + Vector(startP, endP) * (drawMidLine ? minS/maxS : 0.5);

        get_root()->paintAnnotation(device, SEC_GC_DEFAULT,
                                    minP, startP, endP,
                                    get_root()->display_params().distance_between_strands*2,
                                    GBS_global_string("%.1f-%.1f", minS, maxS),
                                    drawMidLine, true, true,
                                    self(), startAttachAbspos());
    }
}

void SEC_loop::paint_constraints(AW_device *device) {
    int abspos = get_fixpoint_strand()->startAttachAbspos();

    double minS = minSize();
    double maxS = maxSize();

    if (minS>0 || maxS>0) {
        if (minS>0) device->circle(SEC_GC_DEFAULT, false, center, minS, minS, -1, self(), abspos);
        if (maxS>0) device->circle(SEC_GC_DEFAULT, false, center, maxS, maxS, -1, self(), abspos);

        device->text(SEC_GC_DEFAULT, GBS_global_string("%.1f-%.1f", minS, maxS), center+Vector(0, max(minS, maxS)/2), 0.5, -1, self(), abspos);
    }
}

// --------------------------
//      Background colors
// --------------------------

#warning move to SEC_db_interface
void SEC_root::cacheBackgroundColor() {
    free(bg_color);
    bg_color = 0;

    const ED4_sequence_terminal *sterm = db->get_seqTerminal();
    if (sterm) {
        int start = 0;
        int len   = db->length();
        int end   = len-1;

        bg_color = (char*)malloc(len);

        const char *bg_sai    = displayParams.display_sai ? ED4_getSaiColorString(db->awroot(), start, end) : 0;
        const char *bg_search = displayParams.display_search ? ED4_buildColorString(sterm, start, end) : 0;

        if (bg_sai) {
            if (bg_search) {
                for (int i = start; i <= end; ++i) {
                    bg_color[i] = bg_search[i] ? bg_search[i] : bg_sai[i];
                }
            }
            else memcpy(bg_color, bg_sai, len);
        }
        else {
            if (bg_search) memcpy(bg_color, bg_search, len);
            else memset(bg_color, 0, len);
        }
    }
}

void SEC_root::paintBackgroundColor(AW_device *device, SEC_bgpaint_mode mode, const Position& p1, int color1, int gc1, const Position& p2, int color2, int gc2, int skel_gc, AW_CL cd1, AW_CL cd2) {
    // paints background colors for p2 and connection between p1 and p2.
    // gc1/gc2 are foreground gc used to detect size of background regions
    // 
    // Paints skeleton as well

    sec_assert(valid_cb_params(cd1, cd2));
    
    color1 = paintData.convert_BackgroundGC(color1); // convert EDIT4-GCs into SECEDIT-GCs
    color2 = paintData.convert_BackgroundGC(color2);

    if (color1 >= 0 || color2 >= 0 || displayParams.show_strSkeleton) {
        const double& radius1 = char_radius[gc1];
        const double& radius2 = char_radius[gc2];

        Position s1    = p1;
        Position s2    = p2;
        bool     space = false;

        if (displayParams.hide_bases) {
            space = true; // no base chars -> enough space to paint
        }
        else {
            Vector v12(p1, p2);
            double vlen = v12.length();
            
            if ((radius1+radius2) < vlen) { // test if there is enough space between characters
                s1 = p1 + v12*(radius1/vlen); // skeleton<->base attach-points
                s2 = p2 - v12*(radius2/vlen);
                space = true;
            }
        }

        if (mode & BG_PAINT_FIRST && color1 >= 0) { // paint first circle ?
            device->circle(color1, true, p1, radius1, radius1, -1, cd1, cd2);
        }

        if (mode & BG_PAINT_SECOND && color2 >= 0) { // paint second circle ?
            device->circle(color2, true, p2, radius1, radius1, -1, cd1, cd2);
        }

        if (color1 == color2 && color1 >= 0) { // colors are equal -> paint background between points
            device->set_line_attributes(color1, bg_linewidth[paintData.get_linePropertyGC(gc1, gc2)], AW_SOLID);
            device->line(color1, p1, p2, -1, cd1, cd2);
        }
        
        if (space) {
            if (displayParams.show_strSkeleton) { // paint skeleton
                device->set_line_attributes(skel_gc, displayParams.skeleton_thickness, AW_SOLID);
#if defined(DEBUG)
                if (displayParams.show_debug) { s1 = p1; s2 = p2; } // in debug mode always show full skeleton
#endif // DEBUG
                device->line(skel_gc, s1, s2, -1, cd1, cd2);
            }
        }
    }
}

void SEC_root::paintSearchPatternStrings(AW_device *device, int clickedPos, AW_pos xPos,  AW_pos yPos) {
    int searchColor = getBackgroundColor(clickedPos);
    
    if (searchColor >= SEC_GC_SBACK_0 && searchColor <= SEC_GC_SBACK_8) {
        static const char *text[SEC_GC_SBACK_8-SEC_GC_SBACK_0+1] = {
            "User 1",
            "User 2", 
            "Probe", 
            "Primer (local)", 
            "Primer (region)", 
            "Primer (global)", 
            "Signature (local)", 
            "Signature (region)", 
            "Signature (global)",
        };

        device->text(searchColor, text[searchColor-SEC_GC_SBACK_0], xPos, yPos, 0, 1, 0, clickedPos, 0);
    }
    else {
        aw_message("Please click on a search result");
    }
}

// --------------
//      Bonds
// --------------


void SEC_bond_def::paint(AW_device *device, char base1, char base2, const Position& p1, const Position& p2, const Vector& toNextBase, const double& char_radius, AW_CL cd1, AW_CL cd2) const {
    if (base1 && base2) {
        char Bond = get_bond(base1, base2);
        if (Bond == ' ') {
            // check IUPACs
            const char *iupac1 = AWT_decode_iupac(base1, ali_type, 0);
            const char *iupac2 = AWT_decode_iupac(base2, ali_type, 0);

            bool useBond[SEC_BOND_PAIR_CHARS];
            for (int i = 0; i<SEC_BOND_PAIR_CHARS; i++) useBond[i] = false;

            int maxIdx = -1;
            for (int i1 = 0; iupac1[i1]; ++i1) {
                for (int i2 = 0; iupac2[i2]; ++i2) {
                    char        b     = get_bond(iupac1[i1], iupac2[i2]);

                    if (b != ' ') {
                        const char *found = strchr(SEC_BOND_PAIR_CHAR, b);
                        int         idx       = found-SEC_BOND_PAIR_CHAR;

                        useBond[idx] = true;
                        if (idx>maxIdx) maxIdx = idx;
                    }
                }
            }

            if (maxIdx >= 0) {
                for (int i = 0; i<SEC_BOND_PAIR_CHARS; i++) {
                    if (useBond[i]) {
                        paint(device, i == maxIdx ? SEC_GC_BONDS : SEC_GC_ERROR, SEC_BOND_PAIR_CHAR[i], p1, p2, toNextBase, char_radius, cd1, cd2);
                    }
                }
            }
        }
        else {
            paint(device, SEC_GC_BONDS, Bond, p1, p2, toNextBase, char_radius, cd1, cd2);
        }
    }
}

void SEC_bond_def::paint(AW_device *device, int GC, char bond, const Position& p1, const Position& p2, const Vector& toNextBase, const double& char_radius, AW_CL cd1, AW_CL cd2) const {
    Vector v12(p1, p2);
    double oppoDist = v12.length();
    double bondLen  = oppoDist-2*char_radius;

    if (bondLen <= 0.0) return; // not enough space to draw bond

    Vector pb = v12*(char_radius/oppoDist);

    Position b1 = p1+pb; // start/end pos of bond
    Position b2 = p2-pb;

    Position center = centroid(b1, b2);

    Vector aside = toNextBase*0.15; // 15% towards next base position

    switch (bond) {
        case '-':               // single line
            device->line(GC, b1, b2, -1, cd1, cd2);
            break;

        case '#':               // double cross
        case '=':               // double line
            device->line(GC, b1+aside, b2+aside, -1, cd1, cd2);
            device->line(GC, b1-aside, b2-aside, -1, cd1, cd2);
            
            if (bond == '#') {
                Vector   outside = v12*(bondLen/oppoDist/4);
                Position c1      = center+outside;
                Position c2      = center-outside;

                aside *= 2;
            
                device->line(GC, c1-aside, c1+aside, -1, cd1, cd2);
                device->line(GC, c2-aside, c2+aside, -1, cd1, cd2);
            }
            break;
            
        case '~': {
            double radius = aside.length();
            {
                double maxRadius = bondLen/4;
                if (maxRadius<radius) radius = maxRadius;
            }

            Vector outside = v12*(radius/oppoDist);

            Position c1 = center+outside;
            Position c2 = center-outside;

            aside *= 2;

            Angle angle(outside);
            int   deg = int(angle.degrees()+0.5);

            device->arc(GC, false, c1, radius, radius, deg+180-1, 180+30, -1, cd1, cd2);
            device->arc(GC, false, c2, radius, radius, deg-1,     180+30, -1, cd1, cd2);
            break;
        }

        case '+':               // cross
            aside *= 2;
            device->line(GC, center-aside, center+aside, -1, cd1, cd2);
            if (2*aside.length() < bondLen) {
                aside.rotate90deg();
                device->line(GC, center-aside, center+aside, -1, cd1, cd2);
            }
            else {
                device->line(GC, b1, b2, -1, cd1, cd2);
            }
            break;

        case 'o':
        case '.': {             // circles
            double radius            = aside.length();
            if (bond == 'o') radius *= 2;
            device->circle(GC, false, center, radius, radius, -1, cd1, cd2);
            break;
        }

        case '@':  // error in bonddef
            device->text(GC, "Err", center+Vector(0, char_radius), 0.5, -1, cd1, cd2);
            break;

        default:
            sec_assert(0); // // illegal bond char
            break;
    }
}

// ----------------------
//      Paint helices
// ----------------------

struct StrandPositionData {
    int      abs[2];            // absolute sequence position
    int      previous[2];       // previous drawn index
    bool     drawn[2];          // draw position ?
    bool     isPair;            // true if position is pairing
    Position realpos[2];        // real position
};

void SEC_helix_strand::paint_strands(AW_device *device, const Vector& strand_vec, const double& strand_len) {
    static StrandPositionData *data      = 0;
    static int                 allocated = 0;

    const SEC_region* Region[2] = { get_region(), other_strand->get_region() };
    int base_count = Region[0]->get_base_count();

    sec_assert(Region[1]->get_base_count() == base_count); // not aligned ?

    if (allocated<base_count) {
        delete [] data;
        data      = new StrandPositionData[base_count];
        allocated = base_count;
    }

    SEC_root       *root  = get_root();
    const BI_helix *helix = root->get_helix();

    double base_dist = base_count>1 ? strand_len / (base_count-1) : 1;
    Vector vnext     = strand_vec * base_dist; // vector from base to next base (in strand)
    
    // first calculate positions
    {
        StrandPositionData *curr = &data[0];
        
        int idx[2] = { 0, base_count-1 };
        Position pos[2] = { leftAttach, rightAttach };
        Vector toNonBind[2]; // vectors from normal to non-binding positions
        toNonBind[1] = (strand_vec*0.5).rotate90deg();
        toNonBind[0] = -toNonBind[1];

        for (int strand = 0; strand<2; ++strand) {
            curr->abs[strand]      = Region[strand]->getBasePos(idx[strand]);
            curr->previous[strand] = 0;
            curr->drawn[strand]    = (curr->abs[strand] >= 0);
        }
        
        for (int dIdx = 1; ; ++dIdx) {
            sec_assert(pos[0].valid());
            sec_assert(pos[1].valid());
            
            int oneAbs  = curr->drawn[0] ? curr->abs[0] : curr->abs[1];
            sec_assert(oneAbs >= 0); // otherwise current position should have been eliminated by align_helix_strands
            curr->isPair = (helix->pairtype(oneAbs) != HELIX_NONE);

            for (int strand = 0; strand<2; ++strand) {
                if (curr->isPair) {
                    curr->realpos[strand] = pos[strand];
                    curr->drawn[strand]   = true;
                }
                else {
                    curr->realpos[strand] = pos[strand]+toNonBind[strand];
                }

                sec_assert(curr->realpos[strand].valid());
            }

            if (dIdx >= base_count) break;

            ++idx[0];
            --idx[1];

            StrandPositionData *prev = curr;
            curr                     = &data[dIdx];

            for (int strand = 0; strand<2; ++strand) {
                pos[strand]           += vnext;
                curr->abs[strand]       = Region[strand]->getBasePos(idx[strand]);
                curr->previous[strand]  = prev->drawn[strand] ? dIdx-1 : prev->previous[strand];
                curr->drawn[strand]     = (curr->abs[strand] >= 0);
            }
        }
    }

    const int pair2helixGC[2] = { SEC_GC_NHELIX, SEC_GC_HELIX };
    const int pair2skelGC[2] = { SEC_SKELE_NHELIX, SEC_SKELE_HELIX };

    const SEC_db_interface   *db   = root->get_db();
    const SEC_displayParams&  disp = root->display_params();

    // draw background and skeleton
    for (int pos = 1; pos<base_count; ++pos) {
        StrandPositionData *curr = &data[pos];
        for (int strand = 0; strand<2; ++strand) {
            if (curr->drawn[strand]) {
                StrandPositionData *prev = &data[curr->previous[strand]];

                int backAbs = disp.edit_direction
                    ? max(prev->abs[strand], curr->abs[strand])
                    : min(prev->abs[strand], curr->abs[strand]);

                root->paintBackgroundColor(device,
                                           pos == base_count-1 ? BG_PAINT_NONE : BG_PAINT_SECOND,
                                           prev->realpos[strand], root->getBackgroundColor(prev->abs[strand]), pair2helixGC[prev->isPair],
                                           curr->realpos[strand], root->getBackgroundColor(curr->abs[strand]), pair2helixGC[curr->isPair],
                                           pair2skelGC[curr->isPair && prev->isPair],
                                           self(), backAbs);
            }
        }
    }

    // draw base characters and bonds
    char baseBuf[20] = "x";
    for (int pos = 0; pos<base_count; ++pos) {
        StrandPositionData *curr = &data[pos];
        char base[2] = { 0, 0 };
        
        int    gc          = pair2helixGC[curr->isPair];
        Vector center_char = root->get_center_char_vector(gc);

        for (int strand = 0; strand<2; ++strand) {
            if (curr->drawn[strand]) {
                int             abs     = curr->abs[strand];
                const Position& realPos = curr->realpos[strand];

                sec_assert(abs >= 0);

                // if (abs >= 0) {
                    base[strand] = db->baseAt(abs);
                    root->announce_base_position(abs, realPos);
                // }
                // else {
                    // base[strand] = '-';
                // }

                if (!disp.hide_bases) {
                    baseBuf[0]        = base[strand];
                    Position base_pos = realPos + center_char; // center base at realpos
#if defined(DEBUG)
                    if (disp.show_debug) device->line(gc, realPos, base_pos, -1, self(), abs);
                    // sprintf(baseBuf+1, "%i", abs);
#endif // DEBUG
                    
                    device->text(gc, baseBuf, base_pos, 0, -1, self(), abs, 0);
                }
            }
        }

        if (disp.show_bonds == SHOW_NHELIX_BONDS || (disp.show_bonds == SHOW_HELIX_BONDS && curr->isPair)) {
            db->bonds()->paint(device, base[0], base[1], curr->realpos[0], curr->realpos[1], vnext,
                               root->get_char_radius(pair2helixGC[curr->isPair]),
                               self(), curr->abs[0]);
        }
    }
}

void SEC_helix_strand::paint(AW_device *device) {
    sec_assert(isRootsideFixpoint());
    
    Vector strand_vec(rightAttach, other_strand->leftAttach);
    double strand_len     = strand_vec.length(); // length of strand

    if (strand_len>0) {
        strand_vec.normalize();     // normalize
    }
    else { // strand with zero length (contains only one base-pair)
        strand_vec = Vector(rightAttach, leftAttach).rotate90deg();
// #if defined(DEBUG)
        // device->set_line_attributes(SEC_GC_HELIX, 1, AW_DOTTED);
        // device->line(SEC_GC_HELIX, LineVector(fixpoint, strand_vec), -1, 0, 0);
// #endif // // DEBUG
    }

    other_strand->origin_loop->paint(device); // first paint next loop
    paint_strands(device, strand_vec, strand_len); // then paint strand

    SEC_root                 *root = get_root();
    const SEC_displayParams&  disp = root->display_params();

    if (disp.show_strSkeleton && !disp.show_bonds && disp.hide_bases) {
        // display strand direction
        LineVector strandArrow;
        if (strand_len>0) {
            strandArrow = LineVector(get_fixpoint(), strand_vec);
        }
        else {
            Vector fix2arrowStart(get_fixpoint(), leftAttachPoint());
            fix2arrowStart.rotate90deg();
            strandArrow = LineVector(get_fixpoint()-fix2arrowStart, 2*fix2arrowStart);
        }

        AW_CL cd1 = (AW_CL)get_helix()->self();
        AW_CL cd2 = (AW_CL)startAttachAbspos();

        device->line(SEC_GC_HELIX, strandArrow, -1, cd1, cd2);

        Vector right = strandArrow.line_vector(); // left arrowhead vector
        right        = (right * (disp.distance_between_strands*0.35/right.length())).rotate135deg();

        Vector left = Vector(right).rotate90deg();

        Position head = strandArrow.head();
        device->line(SEC_GC_HELIX, LineVector(head, left), -1, cd1, cd2);
        device->line(SEC_GC_HELIX, LineVector(head, right), -1, cd1, cd2);
    }

#if defined(DEBUG)
    if (disp.show_debug) paintStrandDebugInfo(device, SEC_GC_HELIX, other_strand);
#endif // DEBUG

    if (root->get_show_constraints() & SEC_HELIX) paint_constraints(device);
}


// --------------------
//      Paint loops
// --------------------


void SEC_segment::paint(AW_device *device, SEC_helix_strand *previous_strand_pointer) {
    int base_count = get_region()->get_base_count(); // bases in segment
    
    const Position& startP = previous_strand_pointer->rightAttachPoint();
    const Position& endP   = next_helix_strand->leftAttachPoint();

    Angle  current;             // start/current angle
    Angle  end;                 // end angle
    double radius1;             // start and..
    double radius2;             // end radius of segment

    {
        Vector seg_start_radius(center1, startP);
        radius1  = seg_start_radius.length();
        current = seg_start_radius;

        Vector seg_end_radius(center2, endP);
        radius2  = seg_end_radius.length();
        end = seg_end_radius;
    }

    int steps = base_count+1;

    double step = ((end-current)/steps).radian();

    // correct if we have to paint more than a full loop
    if ((alpha - (step*steps)) > M_PI) {
        step += (2*M_PI)/steps; 
    }

    double radStep = (radius2-radius1)/steps;
    
    Vector cstep(center1, center2);
    cstep /= steps;

    SEC_root                 *root = get_root();
    const SEC_db_interface   *db   = root->get_db();
    const SEC_displayParams&  disp = root->display_params();
#if defined(DEBUG)
    if (disp.show_debug) {
        paintStrandDebugInfo(device, SEC_GC_LOOP, previous_strand_pointer);

        int startAbsPos = previous_strand_pointer->rightAttachAbspos();
        int endAbsPos   = next_helix_strand->leftAttachAbspos();

        paintDebugInfo(device, SEC_GC_LOOP, center1, GBS_global_string("SC1 (step=%5.3f)", step), self(), startAbsPos);
        paintDebugInfo(device, SEC_GC_LOOP, center2, "SC2", self(), endAbsPos);
        device->line(SEC_GC_LOOP, center1, startP, -1, self(), startAbsPos);
        device->line(SEC_GC_LOOP, center2, endP, -1, self(), endAbsPos);
        device->line(SEC_GC_LOOP, center1, center2, -1, self(), startAbsPos);
    }
#endif // DEBUG

    char baseBuf[5]  = "?";      // contains base char during print
    Position pos    = startP;
    int      abs    = previous_strand_pointer->rightAttachAbspos();
    int      back   = root->getBackgroundColor(abs);
    int      gc     = root->getBondtype(abs) == HELIX_NONE ? SEC_GC_NHELIX : SEC_GC_HELIX;
    int      nextGc = SEC_GC_LOOP;

    Position currCenter = center1;
    double   currRadius = radius1;

    for (int i = -1; i<base_count; i++) { // for each segment position (plus one pre-loop)
        current    += step;     // iterate over angles
        currCenter += cstep;
        currRadius += radStep;

        Position nextPos = currCenter + current.normal()*currRadius;
        int      nextAbs;

        if (i == (base_count-1)) { // last position (belongs to strand)
            nextAbs    = next_helix_strand->leftAttachAbspos();
            if (nextAbs<0) { // helix doesn't start with pair
                nextAbs = next_helix_strand->getNextAbspos();
            }
            nextGc = root->getBondtype(nextAbs) == HELIX_NONE ? SEC_GC_NHELIX : SEC_GC_HELIX;;
        }
        else {
            nextAbs = get_region()->getBasePos(i+1);
        }

        int nextBack = root->getBackgroundColor(nextAbs);

        // paint background (from pos to nextPos)
        root->paintBackgroundColor(device, i == -1 ? BG_PAINT_BOTH : BG_PAINT_SECOND,
                                   pos, back, gc, nextPos, nextBack, nextGc, SEC_SKELE_LOOP, self(),
                                   disp.edit_direction ? nextAbs : abs);
        
        // if (disp.show_strSkeleton) {
            // device->line(SEC_SKELE_LOOP, pos, nextPos, -1, self(), abs);
        // }

        if (i >= 0) {
            // paint base char at pos
            // baseBuf[0] = abs>0 ? root->sequence[abs] : '?';
            baseBuf[0] = abs>0 ? db->baseAt(abs) : '?';
            Vector   center_char = root->get_center_char_vector(gc);
            Position base_pos    = pos + center_char; // center base character at pos
                
// #if defined(DEBUG)
//             if (disp.show_debug) {
// #if defined(PAINT_REGION_INDEX)
//                 sprintf(baseBuf+1, "%i", i);
// #endif // // PAINT_REGION_INDEX
//             }
// #endif // // DEBUG
                
            if (!disp.hide_bases) {
#if defined(DEBUG)
                // show line from base paint pos to calculated center of char
                // (which is currently calculated wrong!)
                if (disp.show_debug) device->line(SEC_GC_LOOP, pos, base_pos, -1, self(), abs);
#endif // DEBUG
                device->text(SEC_GC_LOOP, baseBuf, base_pos, 0, -1, self(), abs, 0 );
            }
            root->announce_base_position(abs, pos);
        }

        // prepare next loop
        pos  = nextPos;
        abs  = nextAbs;
        back = nextBack;
        gc   = nextGc;
    }
}

void SEC_loop::paint(AW_device *device) {
    for (SEC_segment_iterator seg(this); seg; ++seg) { // first paint all segments
        seg->paint(device, seg->get_previous_strand()); 
    }
    for (SEC_strand_iterator strand(this); strand; ++strand) { // then paint all outgoing strands
        if (strand->isRootsideFixpoint()) strand->paint(device);
    }

    SEC_root *root = get_root();
#if defined(DEBUG)
    if (root->display_params().show_debug) {
        SEC_helix_strand *fixpoint_strand = get_fixpoint_strand();
        int               abspos          = fixpoint_strand->startAttachAbspos();

        device->set_line_attributes(SEC_GC_CURSOR, 1, AW_SOLID);
        device->line(SEC_GC_CURSOR, get_center(), fixpoint_strand->get_fixpoint(), self(), abspos);

        paintStrandDebugInfo(device, SEC_GC_CURSOR, fixpoint_strand);
        paintDebugInfo(device, SEC_GC_CURSOR, get_center(), "LC", self(), abspos);
    }
#endif // DEBUG
    if (root->get_show_constraints() & SEC_LOOP) paint_constraints(device);
}

// ---------------------------------------------------------
//      Paint the whole structure (starting with SEC_root)
// ---------------------------------------------------------

GB_ERROR SEC_root::paint(AW_device *device) {
    SEC_loop *root_loop = get_root_loop();
    sec_assert(root_loop);
    clear_announced_positions(); // reset positions next to cursor

    const BI_helix *helix = get_helix();
    sec_assert(helix);

    GB_ERROR error = helix->get_error();

    if (!error) {
        sec_assert(SEC_GC_FIRST_FONT == 0);
        font_group.unregisterAll();
        for (int gc = SEC_GC_FIRST_FONT; gc <= SEC_GC_LAST_FONT; ++gc) {
            font_group.registerFont(device, gc, "ACGTU-.");
            center_char[gc] = device->rtransform(Vector(-0.5*font_group.get_width(gc), 0.5*font_group.get_ascent(gc)));
        }

        // calculate size for background painting
        sec_assert(SEC_GC_FIRST_DATA == 0);
        for (int gc = SEC_GC_FIRST_DATA; gc <= SEC_GC_LAST_DATA; ++gc) {
            int maxSize = max(font_group.get_width(gc), font_group.get_ascent(gc));

            maxSize +=2; // add 2 extra pixels

            bg_linewidth[gc] = maxSize;
            char_radius[gc]    = device->rtransform_size(maxSize) * 0.5; // was 0.75
        }

        cacheBackgroundColor();

        device->set_line_attributes(SEC_SKELE_HELIX,  displayParams.skeleton_thickness, AW_SOLID);
        device->set_line_attributes(SEC_SKELE_NHELIX, displayParams.skeleton_thickness, AW_SOLID);
        device->set_line_attributes(SEC_SKELE_LOOP, displayParams.skeleton_thickness, AW_SOLID);
        device->set_line_attributes(SEC_GC_BONDS, displayParams.bond_thickness, AW_SOLID);

        // mark the root_loop with a box and print stucture number
        {
            const Position&  loop_center = root_loop->get_center();
            const char      *structId    = db->structure()->name();

            // Vector textAdjust = center_char[SEC_GC_DEFAULT];
            // textAdjust.setx(0); // // only adjust y

            AW_CL cd1 = root_loop->self();
            AW_CL cd2 = -1;

            Vector center2corner(-1, -1);
            center2corner.set_length(root_loop->drawnSize()*0.33);
            
            Position upperleft_corner = loop_center+center2corner;
            Vector   diagonal         = -2*center2corner;

            Position textPos(loop_center.xpos(), upperleft_corner.ypos());

            device->box(SEC_GC_DEFAULT, AW_FALSE, upperleft_corner, diagonal, -1, cd1, cd2);
            device->text(SEC_GC_DEFAULT, structId, textPos, 0.5, -1, cd1, cd2, 0);
        }

#if defined(CHECK_INTEGRITY)
        check_integrity(CHECK_ALL);
#endif // CHECK_INTEGRITY

        root_loop->paint(device);

        // paint ecoli positions:
        if (displayParams.show_ecoli_pos) paintEcoliPositions(device);

        if (displayParams.show_helixNrs) {
            paintHelixNumbers(device);
        }

#if defined(PAINT_ABSOLUTE_POSITION)
        if (displayParams.show_debug) showSomeAbsolutePositions(device);
#endif // PAINT_ABSOLUTE_POSITION
        
        // paint cursor:
        if (!drawnPositions->empty() &&
            (device->filter&(AW_PRINTER|AW_PRINTER_EXT)) == 0) // dont print/xfig-export cursor
        {
            size_t   abs1, abs2;
            Position pos1, pos2;
            size_t curAbs;

            if (displayParams.edit_direction == 1) {
                pos1   = drawnPositions->drawn_before(cursorAbsPos, &abs1);
                pos2   = drawnPositions->drawn_after(cursorAbsPos-1, &abs2);
                curAbs = abs2;
            }
            else {
                pos1   = drawnPositions->drawn_before(cursorAbsPos+1, &abs1);
                pos2   = drawnPositions->drawn_after(cursorAbsPos, &abs2);
                curAbs = abs1;
            }

#if defined(DEBUG) && 1
            // draw a testline to see the baseline on that the cursor is positioned
            device->set_line_attributes(SEC_GC_CURSOR, 1, AW_DOTTED);
            device->line(SEC_GC_CURSOR, pos1, pos2, -1, 0, curAbs);
#endif

            Position mid = centroid(pos1, pos2);
            Vector   v(pos1, pos2);
            {
                Vector v_drawn      = device->transform(v);
                double drawn_length = v_drawn.length();

                sec_assert(drawn_length>0.0);

                double cursor_size = 1.3 * max(font_group.get_max_width(), font_group.get_max_ascent()); // 30% bigger than max font size
                double stretch     = cursor_size*0.5/drawn_length; // stretch cursor (half fontsize in each direction)
                    
                v.rotate90deg() *= stretch;
            }

            LineVector cursor(mid+v, mid-v);
            device->set_line_attributes(SEC_GC_CURSOR, 3, AW_SOLID);
            device->line(SEC_GC_CURSOR, cursor, -1, 0, curAbs);
            set_last_drawed_cursor_position(cursor);

            LineVector cursor_dir(cursor.head(), displayParams.edit_direction ? v.rotate270deg() : v.rotate90deg());
            device->line(SEC_GC_CURSOR, cursor_dir, -1, 0, curAbs);


            int cursor_gc;
            int cursor_pos;
            switch (displayParams.show_curpos) {
                case SHOW_ABS_CURPOS:
                    cursor_gc  = SEC_GC_CURSOR;
                    cursor_pos = curAbs+1; // show absolute position starting with 1
                    break;
                case SHOW_BASE_CURPOS:
                    cursor_gc  = SEC_GC_DEFAULT;
                    cursor_pos = ED4_get_base_position(db->get_seqTerminal(), curAbs)+1; // show base position starting with 1
                    break;
                case SHOW_ECOLI_CURPOS: {
                    cursor_gc  = SEC_GC_ECOLI;
                    cursor_pos = db->ecoli()->abs_2_rel(curAbs)+1; // show ecoli base position starting with 1
                    break;
                }
                case SHOW_NO_CURPOS:
                    cursor_gc        = -1;
                    break;
            }

            if (cursor_gc >= 0) {
                paintPosAnnotation(device, cursor_gc, curAbs, GBS_global_string("%u", cursor_pos), true, true);
            }
        }
    }
    return error;
}

void SEC_region::align_helix_strands(SEC_root *root, SEC_region *other_region) {
    if (abspos_array) {
        const BI_helix *helix = root->get_helix();
        if (helix && !helix->get_error()) {
            SEC_region *reg[2] = { this, other_region }; 
            int incr[2] = { 1, -1 }; // this is iterated forward, other_region backward
            int *absarr[2];
            int *new_absarr[2] = { 0, 0 };


            for (int r = 0; r<2; ++r) {
                absarr[r] = reg[r]->abspos_array;
            }

            for (int write = 0; write < 2; ++write) {
                int curr[2] = { 0, reg[1]->baseCount-1 };
                int last[2] = { reg[0]->baseCount-1, 0 };
                int newp[2] = { 0, 0 };

                while (curr[0] <= last[0] && curr[1] >= last[1]) {
                    int  abs[2];
                    bool ispair[2];

                    for (int r = 0; r<2; ++r) {
                        abs[r]    = absarr[r][curr[r]];
                        ispair[r] = abs[r] >= 0 && (helix->pairtype(abs[r]) != HELIX_NONE);
                    }

                    if (ispair[0] && ispair[1]) {
                        if (helix->opposite_position(abs[0]) != size_t(abs[1]) ||
                            helix->opposite_position(abs[1]) != size_t(abs[0]))
                        {
                            GB_ERROR error = GBS_global_string("Helix '%s/%s' folded at wrong position. Please refold.",
                                                               helix->helixNr(abs[0]), helix->helixNr(abs[1]));
                            aw_message(error);
                        }

                        for (int r = 0; r<2; ++r) { // fill up to align binding positions
                            while (newp[r]<newp[1-r]) {
                                if (write) {
                                    new_absarr[r][newp[r]] = -1;
                                }
                                newp[r]++;
                            }
                        }

                        sec_assert(newp[0] == newp[1]);

                        for (int r = 0; r<2; ++r) { // copy binding positions
                            if (write) new_absarr[r][newp[r]] = abs[r];
                            newp[r]++; curr[r] += incr[r]; 
                        }
                    }
                    else {
                        bool collected = false;
                        for (int r = 0; r<2; ++r) {
                            if (abs[r] >= 0 && !ispair[r]) { // collect non-pairing bases
                                if (write) {
                                    new_absarr[r][newp[r]] = abs[r];
                                }
                                newp[r]++; curr[r] += incr[r];
                                collected           = true;
                            }
                        }
                        if (!collected) {
                            for (int r = 0; r<2; ++r) {
                                if (abs[r]<0) curr[r] += incr[r];
                            }
                        }
                    }
                }

                sec_assert(newp[0] == newp[1]); // alignment failed

                for (int r = 0; r<2; ++r) {
                    if (write) {
                        if (r == 1) { // reverse positions
                            int  p2  = newp[1]-1;
                            int *arr = new_absarr[1];
                            for (int p = 0; p<p2; ++p, --p2) {
                                swap(arr[p], arr[p2]);
                            }
                        }

                        delete [] reg[r]->abspos_array;
                        reg[r]->abspos_array      = new_absarr[r];
#if defined(DEBUG)
                        reg[r]->abspos_array_size = newp[r];
#endif                          // DEBUG
                        reg[r]->set_base_count(newp[r]);
                    }
                    else {
                        // allocate buffers for second pass
                        new_absarr[r] = new int[newp[r]];
                    }
                }
            }
        }
    }
}

