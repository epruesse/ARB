// =============================================================== //
//                                                                 //
//   File      : AW_position.cxx                                   //
//   Purpose   : Positions, Vectors and Angles                     //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_position.hxx"

using namespace std;
using namespace AW;

const Position AW::Origin(0, 0);
const Vector   AW::ZeroVector(0, 0, 0);

const double AW::Angle::rad2deg = 180/M_PI;
const double AW::Angle::deg2rad = M_PI/180;

const Angle AW::Eastwards (  0*Angle::deg2rad);
const Angle AW::Southwards( 90*Angle::deg2rad);
const Angle AW::Westwards (180*Angle::deg2rad);
const Angle AW::Northwards(270*Angle::deg2rad);

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

Position Rectangle::nearest_corner(const Position& topos) const {
    Position nearest = get_corner(0);
    double   mindist = Distance(nearest, topos);
    for (int i = 1; i<4; ++i) {
        Position c    = get_corner(i);
        double   dist = Distance(c, topos);
        if (dist<mindist) {
            mindist = dist;
            nearest = c;
        }
    }
    return nearest;
}

// --------------------------------------------------------------------------------

namespace AW {
    Position crosspoint(const LineVector& l1, const LineVector& l2, double& factor_l1, double& factor_l2) {
        // calculates the crossing point of the two straight lines defined by l1 and l2.
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

        aw_assert(s.has_length() && t.has_length());

        factor_l1 = (t.x()*(p1.ypos()-p2.ypos()) + t.y()*(p2.xpos()-p1.xpos()))
            / (s.x()*t.y() - s.y()*t.x());

        factor_l2 = (p1.ypos()-p2.ypos()+s.y()*factor_l1) / t.y();

        return p1 + factor_l1*s; 
    }

    Position nearest_linepoint(const Position& pos, const LineVector& line, double& factor) {
        // returns the Position on 'line' with minimum distance to 'pos'
        // factor is set to [0.0 .. 1.0],
        //    where 0.0 means "at line.start()"
        //    and   1.0 means "at line.head()"

        if (!line.has_length()) {
            factor = 0.5;
            return line.start();
        }

        Vector upright(line.line_vector());
        upright.rotate90deg();

        LineVector pos2line(pos, upright);

        double   unused;
        Position nearest = crosspoint(line, pos2line, factor, unused);

        if (factor<0) {
            nearest = line.start();
            factor  = 0;
        }
        else if (factor>1) {
            nearest = line.head();
            factor  = 1;
        }
        return nearest;
    }
};

