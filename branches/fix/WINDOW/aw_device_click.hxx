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

// @@@ TODO: elements of the following classes should go private!

class AW_clicked_element {
    AW_CL client_data1;
    AW_CL client_data2;

public: // @@@ make private

    bool exists;            // true if a drawn element was clicked, else false
    int  distance;          // distance in pixel to nearest line/text

    AW_pos nearest_rel_pos;        // 0 = at left(upper) small-side, 1 = at right(lower) small-side of textArea

protected:

    void init() {
        client_data1    = 0;
        client_data2    = 0;
        exists          = false;
        distance        = -1;
        nearest_rel_pos = 0;
    }

    AW_clicked_element() { init(); }
    virtual ~AW_clicked_element() {}

    virtual void clear() = 0;

public:

    AW_CL cd1() const { return client_data1; }
    AW_CL cd2() const { return client_data2; }

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

    virtual AW::Position get_attach_point() const = 0;
    virtual bool is_text() const                  = 0;

    bool is_line() const { return !is_text(); }

    double get_rel_pos() const { return nearest_rel_pos; }
    void set_rel_pos(double rel) { aw_assert(rel >= 0.0 && rel <= 1.0); nearest_rel_pos = rel; }

    AW::LineVector get_connecting_line(const AW_clicked_element& other) const {
        //! determine LineVector between two clicked elements (e.g. for drag&drop)
        return AW::LineVector(get_attach_point(), other.get_attach_point());
    }

    virtual int indicate_selected(AW_device *d, int gc) const = 0;
};

class AW_clicked_line : public AW_clicked_element {
public:
    AW_pos x0, y0, x1, y1;  // @@@ make this a LineVector and private!
private:
    void init() {
        x0 = 0; y0 = 0;
        x1 = 0; y1 = 0;
    }
public:

    AW_clicked_line() { init(); }
    void clear() OVERRIDE { AW_clicked_element::init(); init(); }

    bool is_text() const OVERRIDE { return false; }
    bool operator == (const AW_clicked_line& other) const { return nearlyEqual(get_line(), other.get_line()); }

    AW::Position get_attach_point() const OVERRIDE {
        double nrp = get_rel_pos();
        return AW::Position(x0*(1-nrp)+x1*nrp,
                            y0*(1-nrp)+y1*nrp);
    }

    AW::LineVector get_line() const { return AW::LineVector(x0, y0, x1, y1); }
    int indicate_selected(AW_device *d, int gc) const OVERRIDE;
};

class AW_clicked_text : public AW_clicked_element {
public: // @@@ make members private
    AW::Rectangle textArea;     // world coordinates of text
    int           cursor;       // which letter was selected, from 0 to strlen-1 [or -1 if not 'exactHit']
    bool          exactHit;     // true -> real click inside text bounding-box (not only near text) (@@@ redundant: exactHit == (distance == 0))
private:
    void init() {
        textArea = AW::Rectangle();
        cursor   = -1;
        exactHit = false;
    }
public:

    AW_clicked_text() { init(); }
    void clear() OVERRIDE { AW_clicked_element::init(); init(); }

    bool is_text() const OVERRIDE { return true; }
    bool operator == (const AW_clicked_text& other) const { return nearlyEqual(textArea, other.textArea); }

    AW::Position get_attach_point() const OVERRIDE {
        return textArea.centroid(); // @@@ uses center atm - should attach to bounding box
    }
    int indicate_selected(AW_device *d, int gc) const OVERRIDE;
};

#define AWT_CATCH    30         // max-pixel distance to graphical element (to accept a click or command)
#define AWT_NO_CATCH -1

class AW_device_click : public AW_simple_device {
    AW_pos mouse_x, mouse_y; // @@@ use 'int' instead
    int    max_distance_line;
    int    max_distance_text;

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri) OVERRIDE;
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) OVERRIDE;
    bool invisible_impl(const AW::Position& pos, AW_bitset filteri) OVERRIDE { return generic_invisible(pos, filteri); }

    void specific_reset() OVERRIDE {}
    
public:
    AW_clicked_line opt_line; // @@@ make private
    AW_clicked_text opt_text;

    AW_device_click(AW_common *common_);

    AW_DEVICE_TYPE type() OVERRIDE;

    void init_click(AW_pos mousex, AW_pos mousey, int max_distance, AW_bitset filteri);

    void get_clicked_line(class AW_clicked_line *ptr) const; // @@@ make real accessors returning const&
    void get_clicked_text(class AW_clicked_text *ptr) const;
};

#else
#error aw_device_click.hxx included twice
#endif // AW_DEVICE_CLICK_HXX
