/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef GEN_GENE_HXX
#define GEN_GENE_HXX

#ifndef __SET__
#include <set>
#endif
#ifndef __STRING__
#include <string>
#endif

class GEN_root;

//  -----------------------
//      class GEN_gene
//  -----------------------
class GEN_gene {
private:
    GBDATA   *gb_gene;
    GEN_root *root;
    string    name;
    long      pos1;
    long      pos2;
    int       level; // on which "level" the gene is printed

public:
    GEN_gene(GBDATA *gb_gene_, GEN_root *root_);
    virtual ~GEN_gene();

    void paint(AW_device *device, int &x, int &y);

    inline bool operator<(const GEN_gene& other) const {
        int cmp = pos1-other.pos1;
        if (cmp) cmp = pos2-other.pos2;
        return cmp<0;
    }
};

typedef set<GEN_gene> GEN_gene_set;

//  -----------------------
//      class GEN_root
//  -----------------------
class GEN_root {
private:
    GBDATA       *gb_main;
    string        species_name; // name of current species
    string        gene_name;    // name of current gene
    GEN_gene_set  gene_set;
    string        error_reason; // reason why we can't display gene_map
    long          length;       // length of organism sequence
    long          bp_per_line;  // base positions per line

public:
    GEN_root(const char *species_name_, const char *gene_name_, GBDATA *gb_main_, const char *genom_alignment);
    virtual ~GEN_root();

    const string& GeneName() const { return gene_name; }
    const string& SpeciesName() const { return species_name; }

    void set_GeneName(const string& gene_name_) { gene_name = gene_name_; }

    void paint(AW_device *device);
};



#else
#error GEN_gene.hxx included twice
#endif // GEN_GENE_HXX
