// ================================================================ //
//                                                                  //
//   File      : aw_device_click.hxx                                //
//   Purpose   :                                                    //
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

// @@@ FIXME: elements of the following classes should go private!

class AW_clicked_element {
public:
    AW_CL client_data1;
    AW_CL client_data2;
    bool  exists;                                   // true if a drawn element was clicked, else false
};

class AW_clicked_line : public AW_clicked_element {
public:
    AW_pos x0, y0, x1, y1;  // @@@ make this a Rectangle
    AW_pos distance;        // min. distance to line
    AW_pos nearest_rel_pos; // 0 = at x0/y0, 1 = at x1/y1
};

class AW_clicked_text : public AW_clicked_element {
public:
    AW::Rectangle textArea;     // world coordinates of text
    AW_pos        alignment;
    AW_pos        rotation;
    AW_pos        distance;     // y-Distance to text, <0 -> above, >0 -> below
    AW_pos        dist2center;  // Distance to center of text
    int           cursor;       // which letter was selected, from 0 to strlen-1
    bool          exactHit;     // true -> real click on text (not only near text)
};

class AW_device_click : public AW_simple_device {
    AW_pos          mouse_x, mouse_y;
    AW_pos          max_distance_line;
    AW_pos          max_distance_text;

    bool line_impl(int gc, const AW::LineVector& Line, AW_bitset filteri) OVERRIDE;
    bool text_impl(int gc, const char *str, const AW::Position& pos, AW_pos alignment, AW_bitset filteri, long opt_strlen) OVERRIDE;
    bool invisible_impl(const AW::Position& pos, AW_bitset filteri) OVERRIDE { return generic_invisible(pos, filteri); }

    void specific_reset() OVERRIDE {}
    
public:
    AW_clicked_line opt_line;
    AW_clicked_text opt_text;

    AW_device_click(AW_common *common_)
        : AW_simple_device(common_)
    {
        init(0, 0, -1, -1, 0, AW_ALL_DEVICES);
    }

    AW_DEVICE_TYPE type() OVERRIDE;

    void init(AW_pos mousex, AW_pos mousey, AW_pos max_distance_liniei, AW_pos max_distance_texti, AW_pos radi, AW_bitset filteri);

    void get_clicked_line(class AW_clicked_line *ptr) const;
    void get_clicked_text(class AW_clicked_text *ptr) const;
};

bool AW_getBestClick(AW_clicked_line *cl, AW_clicked_text *ct, AW_CL *cd1, AW_CL *cd2);

#else
#error aw_device_click.hxx included twice
#endif // AW_DEVICE_CLICK_HXX
