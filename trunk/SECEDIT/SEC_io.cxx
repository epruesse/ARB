// =============================================================== //
//                                                                 //
//   File      : SEC_io.cxx                                        //
//   Purpose   : io-related things                                 //
//   Time-stamp: <Fri Sep/07/2007 09:02 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


#include <sstream>

#include "SEC_root.hxx"
#include "SEC_iter.hxx"

using namespace std;

void SEC_region::generate_x_string(XString& x_string) {
    x_string.addXpair(get_sequence_start(), get_sequence_end());
}

void SEC_root::generate_x_string(void) {
    sec_assert(!xString);
    xString = new XString(db->length());

    for (SEC_base_part_iterator part(this); part; ++part) {
        part->get_region()->generate_x_string(*xString);
    }
    xString->initialize();
}

SEC_helix_strand * SEC_segment::get_previous_strand(void) {
    SEC_helix_strand *strand_pointer = next_helix_strand;
    SEC_segment *segment_pointer = strand_pointer->get_next_segment();

    while (segment_pointer != this) {
        strand_pointer = segment_pointer->get_next_strand();
        segment_pointer = strand_pointer->get_next_segment();
    }
    return strand_pointer;
}

inline void do_indent(ostream& out, int indent) {
    for (int i=0; i<indent; i++) out << "\t";
}

class Block {
    int&     indent;
    ostream& out;
public:
    Block(const char *name, ostream& Out, int& Indent)
        : indent(Indent)
        , out(Out)
    {
        do_indent(out, indent);
        out << name << "={\n";
        ++indent;
    }

    ~Block() {
        --indent;
        do_indent(out, indent);
        out << "}\n";
    }
};

void SEC_segment::save(ostream & out, int indent, const XString& x_string) {
    Block b("SEGMENT", out, indent);
    get_region()->save(out, indent, x_string);
}

void SEC_region::save(ostream & out, int indent, const XString& x_string) {
    int x_count_start = x_string.getXleftOf(sequence_start);
    int x_count_end   = x_string.getXleftOf(sequence_end);
    
    do_indent(out, indent); out << "SEQ=" << x_count_start << ":" << x_count_end << "\n";
}

void SEC_helix::save(ostream & out, int indent, const XString& x_string) {
    Block b("STRAND", out, indent);

    strandToOutside()->get_region()->save(out, indent, x_string);
    
    do_indent(out, indent); out << "REL=" << get_rel_angle().radian() << "\n";
    do_indent(out, indent); out << "LENGTH=" << minSize() << ":" << maxSize() << "\n";;

    outsideLoop()->save(out, indent, x_string);
    strandToRoot()->get_region()->save(out, indent, x_string);
}

void SEC_loop::save(ostream & out, int indent, const XString& x_string) {
    Block b("LOOP", out, indent);

    do_indent(out, indent); out << "RADIUS=" << minSize() << ":" << maxSize() << "\n";
    do_indent(out, indent); out << "REL=" << get_rel_angle().radian() << "\n";

    SEC_helix *primary   = get_fixpoint_helix();
    bool       root_loop = is_root_loop();
    
    for (SEC_strand_iterator strand(this); strand; ++strand) {
        SEC_helix *helix = strand->get_helix();
        if (helix != primary || root_loop) helix->save(out, indent, x_string);
        strand->get_next_segment()->save(out, indent, x_string);
    }
}

char *SEC_root::buildStructureString(void) {
    delete xString;
    xString = 0;
    if (db->canDisplay()) generate_x_string();

    ostringstream out;
    
    out << "VERSION=" << DATA_VERSION << "\n";

    get_root_loop()->save(out, 0, *xString);

    out << '\0';

    const string&  outstr = out.str();
    char          *result = new char[outstr.length()+1];
    strcpy(result, outstr.c_str());

    return result;
}

