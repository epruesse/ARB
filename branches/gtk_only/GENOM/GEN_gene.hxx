// =============================================================== //
//                                                                 //
//   File      : GEN_gene.hxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 2001           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GEN_GENE_HXX
#define GEN_GENE_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef AW_POSITION_HXX
#include <aw_position.hxx>
#endif

#ifndef _GLIBCXX_SET
#include <set>
#endif
#ifndef _GLIBCXX_STRING
#include <string>
#endif


// ------------------------------------------
//      display classes for ARB_GENE_MAP:

class  GEN_root;
class  GEN_graphic;
struct GEN_position;

class GEN_gene {
    RefPtr<GBDATA>      gb_gene;
    RefPtr<GEN_root>    root;
    std::string         name;
    mutable std::string nodeInfo;
    long                pos1;
    long                pos2;
    bool                complement;

    // Note: if a gene is joined from several parts it is represented in several GEN_gene's!

    void init();
    void load_location(int part, const GEN_position *location);

public:
    GEN_gene(GBDATA *gb_gene_, GEN_root *root_, const GEN_position *location);
    GEN_gene(GBDATA *gb_gene_, GEN_root *root_, const GEN_position *location, int partNumber);

    inline bool operator<(const GEN_gene& other) const {
        long cmp     = pos1-other.pos1;
        if (cmp) cmp = pos2-other.pos2;
        return cmp<0;
    }

    long StartPos() const { return pos1; } // first position of gene (1..n)
    long EndPos() const { return pos2; } // last position of gene (1..n)
    long Length() const { return pos2-pos1+1; }
    bool Complement() const { return complement; }
    const std::string& NodeInfo() const { return nodeInfo; }
    const std::string& Name() const { return name; } // returns the short name of the gene
    const GBDATA *GbGene() const { return gb_gene; }
    GEN_root *Root() { return root; }

    void reinit_NDS() const;
};

typedef std::multiset<GEN_gene> GEN_gene_set;
typedef GEN_gene_set::iterator GEN_iterator;

class AW_device;

class GEN_root : virtual Noncopyable {
    GBDATA      *gb_main;
    GEN_graphic *gen_graphic;
    std::string  organism_name; // name1 of current species
    // (in case of a pseudo gene-species this is the name of the species it originated from)

    std::string  gene_name;     // name of current gene
    GEN_gene_set gene_set;
    std::string  error_reason;  // reason why we can't display gene_map
    long         length;        // length of organism sequence

    GBDATA *gb_gene_data;       // i am build upon this

    AW::Rectangle gene_range;

    
    void clear_selected_range() { gene_range = AW::Rectangle(); }
    void increase_selected_range(AW::Position pos) {
        gene_range = gene_range.valid() ? bounding_box(gene_range, pos) : AW::Rectangle(pos, pos);
    }
    void increase_selected_range(AW::Rectangle rect) {
        gene_range = gene_range.valid() ? bounding_box(gene_range, rect) : rect;
    }

    int smart_text(AW_device *device, int gc, const char *str, AW_pos x, AW_pos y);
    int smart_line(AW_device *device, int gc, AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1);

public:
    GEN_root(const char *organism_name_, const char *gene_name_, GBDATA *gb_main_, AW_root *aw_root, GEN_graphic *gen_graphic_);
    ~GEN_root() {}

    const std::string& GeneName() const { return gene_name; }
    const std::string& OrganismName() const { return organism_name; }

    GBDATA *GbMain() const { return gb_main; }

    void set_GeneName(const std::string& gene_name_) { gene_name = gene_name_; }

    void paint(AW_device *device);

    void reinit_NDS() const;

    const AW::Rectangle& get_selected_range() const { return gene_range; }
};

#else
#error GEN_gene.hxx included twice
#endif // GEN_GENE_HXX
