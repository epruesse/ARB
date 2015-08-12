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
    AW_pos nearest_rel_pos; // 0 = at left(upper) small-side, 1 = at right(lower) small-side of textArea or line

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

    virtual AW::Position get_attach_point() const = 0;
    virtual AW_clicked_element *clone() const     = 0;

    bool does_exist() const { return exists; }
    inline bool is_text() const;
    inline bool is_line() const;

    double get_rel_pos() const { return nearest_rel_pos; }
    int get_distance() const { return distance; }
    AW::LineVector get_connecting_line(const AW_clicked_element& other) const {
        //! determine LineVector between two clicked elements (e.g. for drag&drop)
        return AW::LineVector(get_attach_point(), other.get_attach_point());
    }

    virtual int indicate_selected(AW_device *d, int gc) const = 0;
};

class AW_clicked_line : public AW_clicked_element {
    AW::LineVector line; // world coordinates of clicked line
public:
    void assign(const AW::LineVector& line_, int distance_, const AW_pos& nearest_rel_pos_, const AW_click_cd *click_cd_) {
        AW_clicked_element::assign(distance_, nearest_rel_pos_, click_cd_);
        line = line_;
    }

    bool operator == (const AW_clicked_line& other) const { return nearlyEqual(get_line(), other.get_line()); }

    AW::Position get_attach_point() const OVERRIDE {
        double nrp = get_rel_pos();
        return line.start() + nrp*line.line_vector();
    }
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

    bool operator == (const AW_clicked_text& other) const { return nearlyEqual(textArea, other.textArea); }

    AW::Position get_attach_point() const OVERRIDE {
        return textArea.centroid(); // @@@ uses center atm - should attach to bounding box
    }

    int indicate_selected(AW_device *d, int gc) const OVERRIDE;
    AW_clicked_element *clone() const OVERRIDE { return new AW_clicked_text(*this); }
};

// ---------------------
//      type checks

inline bool AW_clicked_element::is_text() const { return dynamic_cast<const AW_clicked_text*>(this); }
inline bool AW_clicked_element::is_line() const { return dynamic_cast<const AW_clicked_line*>(this); }

#define AWT_CATCH    30         // max-pixel distance to graphical element (to accept a click or command)
#define AWT_NO_CATCH -1

class AW_device_click : public AW_simple_device {
    AW::Position mouse;

    int max_distance_line;
    int max_distance_text;

    AW_clicked_line opt_line;
    AW_clicked_text opt_text;

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri) OVERRIDE;
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) OVERRIDE;
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
