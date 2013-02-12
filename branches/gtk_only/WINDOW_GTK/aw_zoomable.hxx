/* 
 * File:   AW_zoomable.hxx
 * Author: aboeckma
 *
 * Created on November 13, 2012, 12:44 PM
 */

#pragma once

#include "aw_position.hxx"

// --------------------------------------------------
// general note on world- vs. pixel-positions:(WORLD_vs_PIXEL)
// 
// A position is interpreted as the center of the corresponding pixel
// (pixel refers to screen; printer pixel are 15 times smaller!)
// 
// Hence, when scaling factor is 1.0, then
// - any position inside [-0.5, 0.5[ will fall into the pixel 0, any inside [0.5, 1.5[ into pixel 1.
// - a line from 0.0 to 2.0 will paint THREE pixels (0, 1 and 2). A line from -0.5 to 2.499 will to the same
// - clipping to area [0, 100] should in fact clip to [-0.5, 100.5[ (@@@ check this)

class AW_zoomable {
    AW::Vector offset;
    AW_pos     scale;
    AW_pos     unscale;         // = 1.0/scale

public:
    AW_zoomable() { this->reset(); };
    virtual ~AW_zoomable() {}

    void zoom(AW_pos scale);

    AW_pos get_scale() const { return scale; };
    AW_pos get_unscale() const { return unscale; };
    AW::Vector get_offset() const { return offset; }

    void rotate(AW_pos angle);

    void set_offset(const AW::Vector& off) { offset = off*scale; }
    void shift(const AW::Vector& doff) { offset += doff*scale; }

    /**
     * resets scale to 1 and offset to 0
     */
    void reset();

    double transform_size(const double& size) const { return size*scale; }
    double rtransform_size(const double& size) const { return size*unscale; }

    double rtransform_pixelsize(int pixelsize) const {
        // return world-size needed to draw line/box with length/size == 'pixelsize'
        return (pixelsize-1)*unscale;
    }

    // transforming a Vector only scales the vector (a Vector has no position!)
    AW::Vector transform (const AW::Vector& vec) const { return vec*scale; }
    AW::Vector rtransform(const AW::Vector& vec) const { return vec*unscale; }

    // transform a Position
    AW::Position transform (const AW::Position& pos) const {
        AW::Vector vec = transform(AW::Vector(pos+offset));
        return vec.endpoint();
    }
    AW::Position rtransform(const AW::Position& pos) const {
        return rtransform(AW::Vector(pos)).endpoint()-offset;
    }
#if defined(WARN_TODO) && 0
#warning fix transformations
    // @@@ I think this calculation is wrong, cause offset is already scaled
    //     (same applies to old-style transform/rtransform below)
#endif

    AW::LineVector transform (const AW::LineVector& lvec) const {
        return AW::LineVector(transform(lvec.start()), transform(lvec.line_vector()));
    }
    AW::LineVector rtransform(const AW::LineVector& lvec) const { return AW::LineVector(rtransform(lvec.start()), rtransform(lvec.line_vector())); }

    AW::Rectangle transform (const AW::Rectangle& rect) const { return AW::Rectangle(transform(static_cast<const AW::LineVector&>(rect))); }
    AW::Rectangle rtransform(const AW::Rectangle& rect) const { return AW::Rectangle(rtransform(static_cast<const AW::LineVector&>(rect))); }
    
    // old style functions, not preferred:
    void transform(AW_pos x, AW_pos y, AW_pos& xout, AW_pos& yout) const {
        xout = (x+offset.x())*scale;
        yout = (y+offset.y())*scale;
    }
    void rtransform(AW_pos x, AW_pos y, AW_pos& xout, AW_pos& yout) const {
        xout = x*unscale - offset.x();
        yout = y*unscale - offset.y();
    }
};

