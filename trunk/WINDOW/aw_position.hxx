// =============================================================== //
//                                                                 //
//   File      : aw_position.hxx                                   //
//   Purpose   : Positions, Vectors and Angles                     //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AW_POSITION_HXX
#define AW_POSITION_HXX

#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef _GLIBCXX_CMATH
#include <cmath>
#endif

#ifndef aw_assert
#define aw_assert(bed) arb_assert(bed)
#endif

// ------------------------
//      validity checks

#if defined(DEBUG)

#define ISVALID(a) aw_assert((a).valid())

inline const double& NONAN(const double& d) {
    aw_assert(d == d);
    return d;
}

#else
#define ISVALID(a)
#define NONAN(d)   (d)
#endif // DEBUG

namespace AW {

    const double EPSILON = 0.001; // how equal is nearly equal

    inline bool nearlyEqual(const double& val1, const double& val2) { return std::abs(val1-val2) < EPSILON; }
    inline bool nearlyZero(const double& val) { return nearlyEqual(val, 0.0); }
    inline double centroid(const double& val1, const double& val2) { return (val1+val2)*0.5; }

    // -------------------------------------------------------
    //      class Position represents 2-dimensional positions

    // Note: orientation of drawn canvases is like shown in this figure:
    //
    //        __________________\ +x
    //       |                  /                       .
    //       |
    //       |
    //       |
    //       |
    //       |
    //       |
    //      \|/
    //    +y
    //
    // i.e. rotating an angle by 90 degrees, means rotating it 3 hours in clockwise direction

    class Vector;

    class Position {
        double x, y;

        static bool is_between(const double& coord1, const double& between, const double& coord2) {
            return ((coord1-between)*(between-coord2)) >= 0.0;
        }

    public:

        bool valid() const { return !is_nan_or_inf(x) && !is_nan_or_inf(y); }

        Position(double X, double Y) : x(X), y(Y) { ISVALID(*this); }
        // Position(const Position& other) : x(other.x), y(other.y) { ISVALID(*this); }
        Position() : x(NAN), y(NAN) {} // default is no position
        ~Position() {}

        inline Position& operator += (const Vector& v);
        inline Position& operator -= (const Vector& v);

        const double& xpos() const { return x; }
        const double& ypos() const { return y; }

        void setx(const double& X) { x = NONAN(X); }
        void sety(const double& Y) { y = NONAN(Y); }

        void movex(const double& X) { x += NONAN(X); }
        void movey(const double& Y) { y += NONAN(Y); }

        void move(const Vector& movement) { *this += movement; }
        void moveTo(const Position& pos) { *this = pos; }

        inline bool is_between(const Position& p1, const Position& p2) const {
            return is_between(p1.x, x, p2.x) && is_between(p1.y, y, p2.y);
        }
    };

    extern const Position Origin;

    inline bool nearlyEqual(const Position& p1, const Position& p2) {
        return
            nearlyEqual(p1.xpos(), p2.xpos()) && 
            nearlyEqual(p1.ypos(), p2.ypos()); 
    }
    
    // -------------------------------
    //      a 2D vector

    class Vector {
        Position       end;                         // endpoint of vector (vector starts at Position::origin)
        mutable double len;                         // once calculated, length of vector is stored here (negative value means "not calculated")

    public:
        bool valid() const { return end.valid() && !is_nan(len); } // infinite len is allowed (but untested)

        Vector()                                         : len(NAN) {} // default is not a vector
        Vector(const double& X, const double& Y)         : end(X, Y), len(-1) { ISVALID(*this); } // vector (0,0)->(X,Y)
        Vector(const double& X, const double& Y, const double& Length) : end(X, Y), len(Length) { ISVALID(*this); } // same with known length
        explicit Vector(const Position& to)              : end(to), len(-1) { ISVALID(*this); } // vector origin->to
        Vector(const Position& from, const Position& to) : end(to.xpos()-from.xpos(), to.ypos()-from.ypos()), len(-1) { ISVALID(*this); } // vector from->to
        ~Vector() {}

        const double& x() const { return end.xpos(); }
        const double& y() const { return end.ypos(); }
        const Position& endpoint() const { return end; }

        Vector& set(const double& X, const double& Y, double Length = -1) { end = Position(X, Y); len = Length; return *this; }
        Vector& setx(const double& X) { end.setx(X); len = -1; return *this; }
        Vector& sety(const double& Y) { end.sety(Y); len = -1; return *this; }

        const double& length() const {
            if (len<0.0) len = sqrt(x()*x() + y()*y());
            return len;
        }

        // length-modifying members:

        Vector& operator *= (const double& factor)  { return set(x()*factor, y()*factor, length()*std::abs(factor)); }
        Vector& operator /= (const double& divisor) { return operator *= (1.0/divisor); }

        Vector& operator += (const Vector& other)  { return set(x()+other.x(), y()+other.y()); }
        Vector& operator -= (const Vector& other)  { return set(x()-other.x(), y()-other.y()); }

        Vector& normalize() {
            aw_assert(length()>0); // cannot normalize zero-Vector!
            return *this /= length();
        }
        bool is_normalized() const { return nearlyEqual(length(), 1); }
        bool has_length() const { return !nearlyEqual(length(), 0); }

        Vector& set_length(double new_length) {
            double factor  = new_length/length();
            return (*this  *= factor);
        }

        // length-constant members:

        Vector& neg()  { end = Position(-x(), -y()); return *this; }
        Vector& negx() { end.setx(-x()); return *this; }
        Vector& negy() { end.sety(-y()); return *this; }

        Vector& flipxy() { end = Position(y(), x()); return *this; }

        Vector& rotate90deg()  { return negy().flipxy(); }
        Vector& rotate180deg() { return neg(); }
        Vector& rotate270deg() { return negx().flipxy(); }

        Vector& rotate45deg();
        Vector& rotate135deg() { return rotate45deg().rotate90deg(); }
        Vector& rotate225deg() { return rotate45deg().rotate180deg(); }
        Vector& rotate315deg() { return rotate45deg().rotate270deg(); }

        Vector operator-() const { return Vector(-x(), -y(), len); } // unary minus
    };

    extern const Vector ZeroVector;

    inline bool nearlyEqual(const Vector& v1, const Vector& v2) { return nearlyEqual(v1.endpoint(), v2.endpoint()); }
    
    // -----------------------------------------
    //      inline Position members

    inline Position& Position::operator += (const Vector& v) { x += v.x(); y += v.y(); ISVALID(*this); return *this; }
    inline Position& Position::operator -= (const Vector& v) { x -= v.x(); y -= v.y(); ISVALID(*this); return *this; }

    // ------------------------------------------
    //      basic Position / Vector functions

    // Difference between Positions
    inline Vector operator-(const Position& to, const Position& from) { return Vector(from, to); }

    // Position +- Vector -> new Position
    inline Position operator+(const Position& p, const Vector& v) { return Position(p) += v; }
    inline Position operator+(const Vector& v, const Position& p) { return Position(p) += v; }
    inline Position operator-(const Position& p, const Vector& v) { return Position(p) -= v; }

    // Vector addition
    inline Vector operator+(const Vector& v1, const Vector& v2) { return Vector(v1) += v2; }
    inline Vector operator-(const Vector& v1, const Vector& v2) { return Vector(v1) -= v2; }

    // stretch/shrink Vector
    inline Vector operator*(const Vector& v, const double& f) { return Vector(v) *= f; }
    inline Vector operator*(const double& f, const Vector& v) { return Vector(v) *= f; }
    inline Vector operator/(const Vector& v, const double& d) { return Vector(v) /= d; }

    inline Position centroid(const Position& p1, const Position& p2) { return Position(centroid(p1.xpos(), p2.xpos()), centroid(p1.ypos(), p2.ypos())); }

    inline double Distance(const Position& from, const Position& to) { return Vector(from, to).length(); }
    inline double scalarProduct(const Vector& v1, const Vector& v2) { return v1.x()*v2.x() + v1.y()*v2.y(); }

    inline bool are_distinct(const Position& p1, const Position& p2) { return Vector(p1, p2).has_length(); }

    inline bool is_vertical(const Vector& v) { return nearlyZero(v.x()) && !nearlyZero(v.y()); }
    inline bool is_horizontal(const Vector& v) { return !nearlyZero(v.x()) && nearlyZero(v.y()); }

    inline Vector orthogonal_projection(const Vector& projectee, const Vector& target) {
        //! returns the orthogonal projection of 'projectee' onto 'target'
        double tlen = target.length();
        return target * (scalarProduct(projectee, target) / (tlen*tlen));
    }
    inline Vector normalized(const Vector& v) {
        //! returns the normalized Vector of 'v', i.e. a Vector with same orientation, but length 1
        return Vector(v).normalize();
    }
    inline bool are_parallel(const Vector& v1, const Vector& v2) {
        //! returns true, if two vectors have the same orientation
        Vector diff = normalized(v1)-normalized(v2);
        return !diff.has_length();
    }
    //! returns true, if two vectors have opposite orientations
    inline bool are_antiparallel(const Vector& v1, const Vector& v2) { return are_parallel(v1, -v2); }

    // -------------------------------------------------
    //      a positioned vector, representing a line

    enum AW_screen_area_conversion_mode { INCLUSIVE_OUTLINE, UPPER_LEFT_OUTLINE };

    class LineVector {
        Position Start;         // start point
        Vector   ToEnd;         // vector to end point

    protected:
        void standardize();

    public:
        bool valid() const { return Start.valid() && ToEnd.valid(); }

        LineVector(const Position& startpos, const Position& end) : Start(startpos), ToEnd(startpos, end) { ISVALID(*this); }
        LineVector(const Position& startpos, const Vector& to_end) : Start(startpos), ToEnd(to_end) { ISVALID(*this); }
        LineVector(double X1, double Y1, double X2, double Y2) : Start(X1, Y1), ToEnd(X2-X1, Y2-Y1) { ISVALID(*this); }
        explicit LineVector(const AW_screen_area& r, AW_screen_area_conversion_mode mode) {
            switch (mode) {
                case INCLUSIVE_OUTLINE:
                    Start = Position(r.l, r.t);
                    ToEnd = Vector(r.r-r.l+1, r.b-r.t+1);
                    break;
                case UPPER_LEFT_OUTLINE: 
                    Start = Position(r.l, r.t);
                    ToEnd = Vector(r.r-r.l, r.b-r.t);
                    break;
            }
            ISVALID(*this);
        }
        explicit LineVector(const AW_world& r) : Start(r.l, r.t), ToEnd(r.r-r.l, r.b-r.t) { ISVALID(*this); }
        LineVector() {}

        const Vector& line_vector() const { return ToEnd; }
        const Position& start() const { return Start; }
        Position head() const { return Start+ToEnd; }

        Position centroid() const { return Start+ToEnd*0.5; }
        double length() const { return line_vector().length(); }
        bool has_length() const { return line_vector().has_length(); }

        const double& xpos() const { return Start.xpos(); }
        const double& ypos() const { return Start.ypos(); }

        void move(const Vector& movement) { Start += movement; }
        void moveTo(const Position& pos) { Start = pos; }

        LineVector reverse() const { return LineVector(head(), Vector(ToEnd).neg()); }
    };

    Position crosspoint(const LineVector& l1, const LineVector& l2, double& factor_l1, double& factor_l2);
    Position nearest_linepoint(const Position& pos, const LineVector& line, double& factor);
    inline Position nearest_linepoint(const Position& pos, const LineVector& line) { double dummy; return nearest_linepoint(pos, line, dummy); }
    inline double Distance(const Position& pos, const LineVector& line) { return Distance(pos, nearest_linepoint(pos, line)); }

    inline bool is_vertical(const LineVector& line) { return is_vertical(line.line_vector()); }
    inline bool is_horizontal(const LineVector& line) { return is_horizontal(line.line_vector()); }
    inline bool nearlyEqual(const LineVector& L1, const LineVector& L2) {
        return
            nearlyEqual(L1.line_vector(), L2.line_vector()) &&
            nearlyEqual(L1.start(), L2.start());
    }
    
    // ---------------------
    //      a rectangle

    class Rectangle : public LineVector { // the LineVector describes one corner and the diagonal
    public:
        explicit Rectangle(const LineVector& Diagonal) : LineVector(Diagonal) { standardize(); }
        Rectangle(const Position& corner, const Position& opposite_corner) : LineVector(corner, opposite_corner) { standardize(); }
        Rectangle(const Position& corner, const Vector& to_opposite_corner) : LineVector(corner, to_opposite_corner) { standardize(); }
        Rectangle(double X1, double Y1, double X2, double Y2) : LineVector(X1, Y1, X2, Y2) { standardize(); }
        explicit Rectangle(const AW_screen_area& r, AW_screen_area_conversion_mode mode) : LineVector(r, mode) { standardize(); }
        explicit Rectangle(const AW_world& r) : LineVector(r) { standardize(); }
        Rectangle() {};

        const Vector& diagonal() const { return line_vector(); }

        const Position& upper_left_corner() const { return start(); }
        Position lower_left_corner()        const { return Position(start().xpos(),                   start().ypos()+line_vector().y()); }
        Position upper_right_corner()       const { return Position(start().xpos()+line_vector().x(), start().ypos()); }
        Position lower_right_corner()       const { return head(); }

        double left()   const { return upper_left_corner().xpos(); }
        double top()    const { return upper_left_corner().ypos(); }
        double right()  const { return lower_right_corner().xpos(); }
        double bottom() const { return lower_right_corner().ypos(); }

        double width() const { return diagonal().x(); }
        double height() const { return diagonal().y(); }

        LineVector upper_edge() const { return LineVector(start(), Vector(line_vector().x(),  0)); }
        LineVector left_edge()  const { return LineVector(start(), Vector(0,                  line_vector().y())); }
        LineVector lower_edge() const { return LineVector(head(),  Vector(-line_vector().x(), 0)); }
        LineVector right_edge() const { return LineVector(head(),  Vector(0,                  -line_vector().y())); }

        LineVector horizontal_extent() const { return LineVector(left_edge().centroid(), Vector(width(), 0.0)); }
        LineVector vertical_extent() const { return LineVector(upper_edge().centroid(), Vector(0.0, height())); }

        LineVector bigger_extent() const { return width()>height() ? horizontal_extent() : vertical_extent(); }
        LineVector smaller_extent() const { return width()<height() ? horizontal_extent() : vertical_extent(); }

        void standardize() { LineVector::standardize(); }

        bool contains(const Position& pos) const { return pos.is_between(upper_left_corner(), lower_right_corner()); }
        bool contains(const LineVector& lvec) const { return contains(lvec.start()) && contains(lvec.head()); }

        bool distinct_from(const Rectangle& rect) const {
            // returns false for adjacent rectangles (even if they only share one corner)
            return
                top()       > rect.bottom() ||
                rect.top()  > bottom()      ||
                left()      > rect.right()  ||
                rect.left() > right();
        }
        bool overlaps_with(const Rectangle& rect) const { return !distinct_from(rect); }

        Rectangle intersect_with(const Rectangle& rect) const {
            aw_assert(overlaps_with(rect));
            return Rectangle(Rectangle(upper_left_corner(), rect.upper_left_corner()).lower_right_corner(),
                             Rectangle(lower_right_corner(), rect.lower_right_corner()).upper_left_corner());
        }

        Rectangle bounding_box(const Rectangle& rect) const {
            return Rectangle(Rectangle(upper_left_corner(), rect.upper_left_corner()).upper_left_corner(),
                             Rectangle(lower_right_corner(), rect.lower_right_corner()).lower_right_corner());
        }
        Rectangle bounding_box(const Position& pos) const {
            return Rectangle(Rectangle(upper_left_corner(), pos).upper_left_corner(),
                             Rectangle(lower_right_corner(), pos).lower_right_corner());
        }

        double surface() const { return width()*height(); }
    };

    inline Rectangle bounding_box(const Rectangle& r1, const Rectangle& r2) { return r1.bounding_box(r2); }

    inline Rectangle bounding_box(const Rectangle& rect, const LineVector& line) { return rect.bounding_box(Rectangle(line)); }
    inline Rectangle bounding_box(const LineVector& line, const Rectangle& rect) { return rect.bounding_box(Rectangle(line)); }

    inline Rectangle bounding_box(const LineVector& l1, const LineVector& l2) { return Rectangle(l1).bounding_box(Rectangle(l2)); }

    inline Rectangle bounding_box(const Rectangle& rect, const Position& pos) { return rect.bounding_box(pos); }
    inline Rectangle bounding_box(const Position& pos, const Rectangle& rect) { return rect.bounding_box(pos); }

    inline Rectangle bounding_box(const LineVector& line, const Position& pos) { return Rectangle(line).bounding_box(pos); }
    inline Rectangle bounding_box(const Position& pos, const LineVector& line) { return Rectangle(line).bounding_box(pos); }
    
    inline Rectangle bounding_box(const Position& p1, const Position& p2) { return Rectangle(p1, p2); }


    // ------------------------------------------------------------------
    //      class angle represents an angle using a normalized vector

    class Angle {
        mutable Vector Normal;  // the normal vector representing the angle (x = cos(angle), y = sin(angle))
        mutable double Radian;  // the radian of the angle

        void recalcRadian() const;
        void recalcNormal() const;

    public:
        bool valid() const { return Normal.valid() && !is_nan_or_inf(Radian); }

        static const double rad2deg;
        static const double deg2rad;

        explicit Angle(double Radian_) : Radian(Radian_) { recalcNormal(); ISVALID(*this); }
        Angle(double x, double y) : Normal(x, y) { Normal.normalize(); recalcRadian(); ISVALID(*this); }
        explicit Angle(const Vector& v) : Normal(v) { Normal.normalize(); recalcRadian(); ISVALID(*this); }
        Angle(const Vector& n, double r) : Normal(n), Radian(r) { aw_assert(n.is_normalized()); ISVALID(*this); }
        Angle(const Position& p1, const Position& p2) : Normal(p1, p2) { Normal.normalize(); recalcRadian(); ISVALID(*this); }
        Angle() : Radian(NAN) {}  // default is not an angle

        Angle& operator = (const Angle& other) { Normal = other.Normal; Radian = other.Radian; return *this; }
        Angle& operator = (const Vector& vec) { Normal = vec; Normal.normalize(); recalcRadian(); return *this; }

        void fixRadian() const { // force radian into range [0, 2*M_PI[
            while (Radian<0.0) Radian       += 2*M_PI;
            while (Radian >= 2*M_PI) Radian -= 2*M_PI;
        }

        const double& radian() const { fixRadian(); return Radian; }
        double degrees() const { fixRadian(); return rad2deg*Radian; }
        const Vector& normal() const { return Normal; }

        const double& sin() const { return Normal.y(); }
        const double& cos() const { return Normal.x(); }

        Angle& operator += (const Angle& o) {
            Radian += o.Radian;

            double norm = normal().length()*o.normal().length();
            if (nearlyEqual(norm, 1)) {  // fast method
                Vector newNormal(cos()*o.cos() - sin()*o.sin(),
                                 sin()*o.cos() + cos()*o.sin());
                aw_assert(newNormal.is_normalized());
                Normal = newNormal;
            }
            else {
                recalcNormal();
            }
            return *this;
        }
        Angle& operator -= (const Angle& o) {
            Radian -= o.Radian;

            double norm = normal().length()*o.normal().length();
            if (nearlyEqual(norm, 1)) { // fast method
                Vector newNormal(cos()*o.cos() + sin()*o.sin(),
                                 sin()*o.cos() - cos()*o.sin());
                aw_assert(newNormal.is_normalized());

                Normal = newNormal;
            }
            else {
                recalcNormal();
            }
            return *this;
        }

        Angle& operator *= (const double& fact) {
            fixRadian();
            Radian *= fact;
            recalcNormal();
            return *this;
        }

        Angle& rotate90deg()   { Normal.rotate90deg();  Radian += 0.5*M_PI; return *this; }
        Angle& rotate180deg()  { Normal.rotate180deg(); Radian +=     M_PI; return *this; }
        Angle& rotate270deg()  { Normal.rotate270deg(); Radian += 1.5*M_PI; return *this; }

        Angle operator-() const { return Angle(Vector(Normal).negy(), 2*M_PI-Radian); } // unary minus
    };

    inline Angle operator+(const Angle& a1, const Angle& a2) { return Angle(a1) += a2; }
    inline Angle operator-(const Angle& a1, const Angle& a2) { return Angle(a1) -= a2; }

    inline Angle operator*(const Angle& a, const double& fact) { return Angle(a) *= fact; }
    inline Angle operator/(const Angle& a, const double& divi) { return Angle(a) *= (1.0/divi); }

    // ---------------------
    //      some helpers

    // pythagoras:

    inline double hypotenuse(double cath1, double cath2) { return sqrt(cath1*cath1 + cath2*cath2); }
    inline double cathetus(double hypotenuse, double cathetus) {
        aw_assert(hypotenuse>cathetus);
        return sqrt(hypotenuse*hypotenuse - cathetus*cathetus);
    }

#if defined(DEBUG)
    // don't use these in release code - they are only approximations!

    // test whether two doubles are "equal" (slow - use for assertions only!)
    inline bool are_equal(const double& d1, const double& d2) {
        double diff = std::abs(d1-d2);
        return diff < 0.000001;
    }

    inline bool are_orthographic(const Vector& v1, const Vector& v2) { // orthogonal (dt.)
        return are_equal(scalarProduct(v1, v2), 0);
    }

#endif // DEBUG

    inline bool isOrigin(const Position& p) {
        return p.xpos() == 0 && p.ypos() == 0;
    }

#if defined(DEBUG)
    inline void aw_dump(const double& p, const char *varname) {
        fprintf(stderr, "%s=%f", varname, p);
    }
    inline void aw_dump(const Position& p, const char *varname) {
        fprintf(stderr, "Position %s={ ", varname);
        aw_dump(p.xpos(), "x"); fputs(", ", stderr);
        aw_dump(p.ypos(), "y"); fputs(" }", stderr);
    }
    inline void aw_dump(const Vector& v, const char *varname) {
        fprintf(stderr, "Vector %s={ ", varname);
        aw_dump(v.x(), "x"); fputs(", ", stderr);
        aw_dump(v.y(), "y"); fputs(" }", stderr);
    }
    inline void aw_dump(const LineVector& v, const char *varname) {
        fprintf(stderr, "LineVector %s={ ", varname);
        aw_dump(v.start(), "start"); fputs(", ", stderr);
        aw_dump(v.line_vector(), "line_vector"); fputs(" }", stderr);
        
    }
    inline void aw_dump(const Rectangle& r, const char *varname) {
        fprintf(stderr, "Rectangle %s={ ", varname);
        aw_dump(r.upper_left_corner(), "upper_left_corner"); fputs(", ", stderr);
        aw_dump(r.lower_right_corner(), "lower_right_corner"); fputs(" }", stderr);
    }

#define AW_DUMP(x) do { aw_dump(x, #x); fputc('\n', stderr); } while(0)
    
#endif

};

#else
#error aw_position.hxx included twice
#endif // AW_POSITION_HXX
