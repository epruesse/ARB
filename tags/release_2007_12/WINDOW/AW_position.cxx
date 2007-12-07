// =============================================================== //
//                                                                 //
//   File      : AW_position.cxx                                   //
//   Purpose   : Positions, Vectors and Angles                     //
//   Time-stamp: <Fri Sep/07/2007 11:45 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arb_assert.h>
#define aw_assert(cond) arb_assert(cond)

#include "aw_position.hxx"

using namespace std;
using namespace AW;

const Position AW::Origin(0, 0);
const Vector   AW::ZeroVector(0, 0, 0);

const double AW::Angle::rad2deg = 180/M_PI;
const double AW::Angle::deg2rad = M_PI/180;

void LineVector::standardize() {
    // make diagonal positive (i.e. make it a Vector which contains width and height of a Rectangle)
    // this changes the start position to the upper-left corner

    double dx = ToEnd.x();
    double dy = ToEnd.y();

    if (dx<0) {
        if (dy<0) {
            Start += ToEnd; // lower-right to upper-left
            ToEnd.rotate180deg();
        }
        else {
            Start.movex(dx); // upper-right to upper-left
            ToEnd.negx();
        }
    }
    else if (dy<0) {
        Start.movey(dy); // lower-left to upper-left
        ToEnd.negy();
    }
}

Vector& Vector::rotate45deg() {
    static double inv_sqrt2 = 1/sqrt(2.0);

    *this = (*this+Vector(*this).rotate90deg()) * inv_sqrt2;
    return *this;
}

void Angle::recalcRadian() const {
    Radian = atan2(Normal.y(), Normal.x());
}

void Angle::recalcNormal() const {
    Normal = Vector(std::cos(Radian), std::sin(Radian));
    aw_assert(Normal.is_normalized());
}

// --------------------------------------------------------------------------------

namespace AW {
    Position crosspoint(const LineVector& l1, const LineVector& l2, double& factor_l1, double& factor_l2) {
        // calculates the crossing point of the two staight lines defined by l1 and l2.
        // sets two factors, so that
        // crosspoint == l1.start()+factor_l1*l1.line_vector();
        // crosspoint == l2.start()+factor_l2*l2.line_vector();

        // Herleitung: 
        // x1+g*sx = x2+h*tx
        // y1+g*sy = y2+h*ty
        //
        // h = -(x2-sx*g-x1)/tx                                       
        // h = (y1-y2+sy*g)/ty                                        (h is factor_l2)
        // 
        // -(x2-sx*g-x1)/tx = (y1-y2+sy*g)/ty
        // 
        // g = (tx*y1+ty*x2-tx*y2-ty*x1)/(sx*ty-sy*tx)                
        //
        // g = (tx*(y1-y2)+ty*(x2-x1))/(sx*ty-sy*tx)                  (g is factor_l1)

        const Position& p1 = l1.start();
        const Position& p2 = l2.start();

        const Vector& s = l1.line_vector();
        const Vector& t = l2.line_vector();

        factor_l1 = ( t.x()*(p1.ypos()-p2.ypos()) + t.y()*(p2.xpos()-p1.xpos()) )
            / (s.x()*t.y() - s.y()*t.x());

        factor_l2 = (p1.ypos()-p2.ypos()+s.y()*factor_l1) / t.y();

        return p1 + factor_l1*s;
    }

    double Distance(const AW::Position pos, const AW::LineVector line) {
        Vector upright(line.line_vector());
        upright.rotate90deg();

        LineVector pos2line(pos, upright);

        double f1, f2;
        Position cross = crosspoint(line, pos2line, f1, f2);

        double dist;
        if (f1 >= 0 && f1 <= 1) { // 'cross' is at 'line'
            dist = Distance(pos, cross);
        }
        else if (f1<0) {
            dist = Distance(pos, line.start());
        }
        else {
            aw_assert(f1>1);
            dist = Distance(pos, line.head());
        }
    
        return dist;
    }

};
