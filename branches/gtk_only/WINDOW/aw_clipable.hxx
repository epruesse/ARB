/* 
 * File:   AW_clippable.hxx
 * Author: aboeckma
 *
 * Created on November 13, 2012, 12:39 PM
 */

#pragma once
#include "aw_base.hxx"
#include "aw_position.hxx"


struct AW_font_overlap { bool top, bottom, left, right; };


class AW_clipable {
    const AW_screen_area& common_screen;
    const AW_screen_area& get_screen() const { return common_screen; }

    AW_screen_area  clip_rect;    // holds the clipping rectangle coordinates
    AW_font_overlap font_overlap;

    void set_cliprect_oversize(const AW_screen_area& rect, bool allow_oversize);
    bool need_extra_clip_position(const AW::Position& p1, const AW::Position& p2, AW::Position& extra);
protected:
    int compoutcode(AW_pos xx, AW_pos yy) const {
        // calculate outcode for clipping the current line
        // order - top,bottom,right,left
        int code = 0;
        if (clip_rect.b - yy < 0)       code = 4;
        else if (yy - clip_rect.t < 0)  code = 8;
        if (clip_rect.r - xx < 0)       code |= 2;
        else if (xx - clip_rect.l < 0)  code |= 1;
        return (code);
    };

    void set_cliprect(const AW_screen_area& rect) { clip_rect = rect; }

public:
    
    AW_clipable(const AW_screen_area& screen)
        : common_screen(screen)
    {
        clip_rect.clear();
        set_font_overlap(false);
    }
    virtual ~AW_clipable() {}

    bool is_below_clip(double ypos) const { return ypos > clip_rect.b; }
    bool is_above_clip(double ypos) const { return ypos < clip_rect.t; }
    bool is_leftof_clip(double xpos) const { return xpos < clip_rect.l; }
    bool is_rightof_clip(double xpos) const { return xpos > clip_rect.r; }

    bool is_outside_clip(AW::Position pos) const {
        return
            is_below_clip(pos.ypos()) || is_above_clip(pos.ypos()) ||
            is_leftof_clip(pos.xpos()) || is_rightof_clip(pos.xpos());
    }

    bool is_outside_clip(AW::Rectangle rect) const {
        return !rect.overlaps_with(AW::Rectangle(get_cliprect(), AW::INCLUSIVE_OUTLINE));
    }

    bool clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out);
    bool clip(const AW::LineVector& line, AW::LineVector& clippedLine);

    bool box_clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out);
    bool box_clip(const AW::Rectangle& rect, AW::Rectangle& clippedRect);
    bool box_clip(int npos, const AW::Position *pos, int& nclippedPos, AW::Position*& clippedPos);
    bool force_into_clipbox(const AW::Position& pos, AW::Position& forcedPos);

    void set_top_clip_border(int top, bool allow_oversize = false);
    void set_bottom_clip_border(int bottom, bool allow_oversize = false); // absolute
    void set_bottom_clip_margin(int bottom, bool allow_oversize = false); // relative
    void set_left_clip_border(int left, bool allow_oversize = false);
    void set_right_clip_border(int right, bool allow_oversize = false);
    const AW_screen_area& get_cliprect() const { return clip_rect; }

    void set_clipall() {
        // clip all -> nothing drawn afterwards
        AW_screen_area rect = { 0, -1, 0, -1};
        set_cliprect_oversize(rect, false);
    }

    bool completely_clipped() const { return clip_rect.l>clip_rect.r || clip_rect.t>clip_rect.b; }

    bool allow_top_font_overlap() const { return font_overlap.top; }
    bool allow_bottom_font_overlap() const { return font_overlap.bottom; }
    bool allow_left_font_overlap() const { return font_overlap.left; }
    bool allow_right_font_overlap() const { return font_overlap.right; }
    const AW_font_overlap& get_font_overlap() const { return font_overlap; }
    
    void set_top_font_overlap(bool allow) { font_overlap.top = allow; }
    void set_bottom_font_overlap(bool allow) { font_overlap.bottom = allow; }
    void set_left_font_overlap(bool allow) { font_overlap.left = allow; }
    void set_right_font_overlap(bool allow) { font_overlap.right = allow; }

    void set_vertical_font_overlap(bool allow) { font_overlap.top = font_overlap.bottom = allow; }
    void set_horizontal_font_overlap(bool allow) { font_overlap.left = font_overlap.right = allow; }
    void set_font_overlap(bool allow) { set_vertical_font_overlap(allow); set_horizontal_font_overlap(allow); }
    void set_font_overlap(const AW_font_overlap& fo) { font_overlap = fo; }

    // like set_xxx_clip_border but make window only smaller:

    void reduce_top_clip_border(int top);
    void reduce_bottom_clip_border(int bottom);
    void reduce_left_clip_border(int left);
    void reduce_right_clip_border(int right);

    int reduceClipBorders(int top, int bottom, int left, int right);
};

