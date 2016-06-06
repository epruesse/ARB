// ================================================================ //
//                                                                  //
//   File      : aw_device_click.hxx                                //
//   Purpose   : Detect which graphical element is "nearby"         //
//               a given mouse position                             //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef AW_DEVICE_CLICK_HXX
#define AW_DEVICE_CLICK_HXX

#ifndef AW_DEVICE_HXX
#include "aw_device.hxx"
#endif

class AW_clicked_element {
    AW_CL client_data1;
    AW_CL client_data2;

    bool   exists;          // true if a drawn element was clicked, else false
    int    distance;        // distance in pixel to nearest line/text
    AW_pos nearest_rel_pos; // 0 = at left(upper) small-side, 1 = at right(lower) small-side of textArea or line (does not make sense for box or polygon)

    void copy_cds(const AW_click_cd *click_cd) {
        if (click_cd) {
            client_data1 = click_cd->get_cd1();
            client_data2 = click_cd->get_cd2();
        }
        else {
            client_data1 = 0;
            client_data2 = 0;
        }
    }
    void set_rel_pos(double rel) { aw_assert(rel >= 0.0 && rel <= 1.0); nearest_rel_pos = rel; }

protected:

    AW_clicked_element()
        : client_data1(0),
          client_data2(0),
          exists(false),
          distance(-1),
          nearest_rel_pos(0)
    {}

    void assign(int distance_, const AW_pos& nearest_rel_pos_, const AW_click_cd *click_cd_) {
        distance = distance_;
        set_rel_pos(nearest_rel_pos_);
        copy_cds(click_cd_);
        exists   = true;
    }

public:
    virtual ~AW_clicked_element() {}

    AW_CL cd1() const { return client_data1; }
    AW_CL cd2() const { return client_data2; }

    virtual AW::Position get_attach_point() const  = 0;
    virtual AW::Rectangle get_bounding_box() const = 0;
    virtual AW_clicked_element *clone() const      = 0;

    bool does_exist() const { return exists; }

    inline bool is_text() const;
    inline bool is_line() const;
    inline bool is_box() const;
    inline bool is_polygon() const;

    double get_rel_pos() const { return nearest_rel_pos; }
    int get_distance() const { return distance; }
    AW::LineVector get_connecting_line(const AW_clicked_element& other) const {
        //! determine LineVector between two clicked elements (e.g. for drag&drop)
        return AW::LineVector(get_attach_point(), other.get_attach_point());
    }

    virtual bool operator==(const AW_clicked_element& other) const = 0;
    virtual int indicate_selected(AW_device *d, int gc) const  = 0;
};

class AW_clicked_line : public AW_clicked_element {
    AW::LineVector line; // world coordinates of clicked line
public:
    void assign(const AW::LineVector& line_, int distance_, const AW_pos& nearest_rel_pos_, const AW_click_cd *click_cd_) {
        AW_clicked_element::assign(distance_, nearest_rel_pos_, click_cd_);
        line = line_;
    }

    bool operator == (const AW_clicked_element& other) const OVERRIDE {
        const AW_clicked_line *otherLine = dynamic_cast<const AW_clicked_line*>(&other);
        return otherLine ? nearlyEqual(get_line(), otherLine->get_line()) : false;
    }

    AW::Position get_attach_point() const OVERRIDE {
        double nrp = get_rel_pos();
        return line.start() + nrp*line.line_vector();
    }
    AW::Rectangle get_bounding_box() const OVERRIDE { return AW::Rectangle(line); }
    const AW::LineVector& get_line() const { return line; }

    int indicate_selected(AW_device *d, int gc) const OVERRIDE;
    AW_clicked_element *clone() const OVERRIDE { return new AW_clicked_line(*this); }
};

class AW_clicked_text : public AW_clicked_element {
    AW::Rectangle textArea; // world coordinates of clicked text
public:
    void assign(AW::Rectangle textArea_, int distance_, const AW_pos& nearest_rel_pos_, const AW_click_cd *click_cd_) {
        AW_clicked_element::assign(distance_, nearest_rel_pos_, click_cd_);
        textArea = textArea_;
    }

    bool operator == (const AW_clicked_element& other) const OVERRIDE {
        const AW_clicked_text *otherText = dynamic_cast<const AW_clicked_text*>(&other);
        return otherText ? nearlyEqual(textArea, otherText->textArea) : false;
    }

    AW::Position get_attach_point() const OVERRIDE { return textArea.centroid(); }
    AW::Rectangle get_bounding_box() const OVERRIDE { return textArea; }

    int indicate_selected(AW_device *d, int gc) const OVERRIDE;
    AW_clicked_element *clone() const OVERRIDE { return new AW_clicked_text(*this); }
};

class AW_clicked_box : public AW_clicked_element {
    AW::Rectangle box; // world coordinates of clicked box
public:
    void assign(AW::Rectangle box_, int distance_, const AW_pos& nearest_rel_pos_, const AW_click_cd *click_cd_) {
        AW_clicked_element::assign(distance_, nearest_rel_pos_, click_cd_);
        box    = box_;
    }

    bool operator == (const AW_clicked_element& other) const OVERRIDE {
        const AW_clicked_box *otherBox = dynamic_cast<const AW_clicked_box*>(&other);
        return otherBox ? nearlyEqual(box, otherBox->box) : false;
    }

    AW::Position get_attach_point() const OVERRIDE { return box.centroid(); }
    AW::Rectangle get_bounding_box() const OVERRIDE { return box; }
    int indicate_selected(AW_device *d, int gc) const OVERRIDE;
    AW_clicked_element *clone() const OVERRIDE { return new AW_clicked_box(*this); }
};

class AW_clicked_polygon : public AW_clicked_element {
    int           npos; // number of corners
    AW::Position *pos;  // world coordinates of clicked polygon

public:
    AW_clicked_polygon()
        : npos(0),
          pos(NULL)
    {}
    AW_clicked_polygon(const AW_clicked_polygon& other)
        : AW_clicked_element(other),
          npos(other.npos)
    {
        if (other.pos) {
            pos = new AW::Position[npos];
            for (int i = 0; i<npos; ++i) pos[i] = other.pos[i];
        }
        else {
            pos = NULL;
        }
    }
    DECLARE_ASSIGNMENT_OPERATOR(AW_clicked_polygon);
    ~AW_clicked_polygon() {
        delete [] pos;
    }

    void assign(int npos_, const AW::Position *pos_, int distance_, const AW_pos& nearest_rel_pos_, const AW_click_cd *click_cd_) {
        if (pos) delete [] pos;

        AW_clicked_element::assign(distance_, nearest_rel_pos_, click_cd_);

        npos = npos_;
        pos  = new AW::Position[npos];
        for (int i = 0; i<npos; ++i) pos[i] = pos_[i];
    }

    bool operator == (const AW_clicked_element& other) const OVERRIDE {
        const AW_clicked_polygon *otherPoly = dynamic_cast<const AW_clicked_polygon*>(&other);
        if (otherPoly) {
            if (npos == otherPoly->npos) {
                for (int i = 0; i<npos; ++i) {
                    if (!nearlyEqual(pos[i], otherPoly->pos[i])) {
                        return false;
                    }
                }
                return true;
            }
        }
        return false;
    }

    AW::Position get_attach_point() const OVERRIDE {
        AW::Position c = pos[0];
        for (int i = 1; i<npos; ++i) {
            c += AW::Vector(pos[i]);
        }
        return AW::Position(c.xpos()/npos, c.ypos()/npos);
    }
    AW::Rectangle get_bounding_box() const OVERRIDE;
    int indicate_selected(AW_device *d, int gc) const OVERRIDE;
    AW_clicked_element *clone() const OVERRIDE {
        return new AW_clicked_polygon(*this);
    }

    const AW::Position *get_polygon(int& posCount) const {
        aw_assert(does_exist());
        posCount = npos;
        return pos;
    }
};


// ---------------------
//      type checks

inline bool AW_clicked_element::is_text()    const { return dynamic_cast<const AW_clicked_text*>(this); }
inline bool AW_clicked_element::is_line()    const { return dynamic_cast<const AW_clicked_line*>(this); }
inline bool AW_clicked_element::is_box()     const { return dynamic_cast<const AW_clicked_box*>(this); }
inline bool AW_clicked_element::is_polygon() const { return dynamic_cast<const AW_clicked_polygon*>(this); }

#define AWT_CATCH    30         // max-pixel distance to graphical element (to accept a click or command)
#define AWT_NO_CATCH -1

class AW_device_click : public AW_simple_device {
    AW::Position mouse;

    int max_distance_line;
    int max_distance_text;

    AW_clicked_line    opt_line;
    AW_clicked_text    opt_text;
    AW_clicked_box     opt_box;
    AW_clicked_polygon opt_polygon;

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri) OVERRIDE;
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) OVERRIDE;
    bool box_impl(int gc, AW::FillStyle filled, const AW::Rectangle& rect, AW_bitset filteri) OVERRIDE;
    bool polygon_impl(int gc, AW::FillStyle filled, int npos, const AW::Position *pos, AW_bitset filteri) OVERRIDE;

    // completely ignore clicks to circles and arcs
    bool circle_impl(int, AW::FillStyle, const AW::Position&, const AW::Vector&, AW_bitset) OVERRIDE { return false; }
    bool arc_impl(int, AW::FillStyle, const AW::Position&, const AW::Vector&, int, int, AW_bitset) OVERRIDE { return false; }

    bool invisible_impl(const AW::Position& pos, AW_bitset filteri) OVERRIDE { return generic_invisible(pos, filteri); }
    void specific_reset() OVERRIDE {}
    
public:
    AW_device_click(AW_common *common_);

    AW_DEVICE_TYPE type() OVERRIDE;

    void init_click(const AW::Position& click, int max_distance, AW_bitset filteri);

    enum ClickPreference { PREFER_NEARER, PREFER_LINE, PREFER_TEXT };
    const AW_clicked_element *best_click(ClickPreference prefer = PREFER_NEARER);
};

#else
#error aw_device_click.hxx included twice
#endif // AW_DEVICE_CLICK_HXX
