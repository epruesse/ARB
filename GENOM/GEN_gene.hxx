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
    GBDATA         *gb_gene;
    GEN_root       *root;
    string          name;
    mutable string  nodeInfo;
    long            pos1;
    long            pos2;
    bool            complement;

    //     int       level; // on which "level" the gene is printed

    // Note: if a gene is joined from several parts it is represented in several GEN_gene's!

    void init(GBDATA *gb_gene_, GEN_root *root_);
    void load_positions(int part);

public:
    GEN_gene(GBDATA *gb_gene_, GEN_root *root_);
    GEN_gene(GBDATA *gb_gene_, GEN_root *root_, int partNumber, int maxParts);
    virtual ~GEN_gene();

    inline bool operator<(const GEN_gene& other) const {
        long cmp     = pos1-other.pos1;
        if (cmp) cmp = pos2-other.pos2;
        return cmp<0;
    }

    long StartPos() const { return pos1; }
    long EndPos() const { return pos2; }
    long Length() const { return pos2-pos1+1; }
    bool Complement() const { return complement; }
    //     int Level() const { return level; }
    const string& NodeInfo() const { return nodeInfo; }
    const string& Name() const { return name; } // returns the short name of the gene
    const GBDATA *GbGene() const { return gb_gene; }
    GEN_root *Root() { return root; }

    void reinit_NDS() const;
};

typedef multiset<GEN_gene> GEN_gene_set;
typedef GEN_gene_set::iterator GEN_iterator;

//  -----------------------
//      class GEN_root
//  -----------------------
class GEN_root {
private:
    GBDATA *gb_main;
    string  organism_name;      // name1 of current species
    // (in case of a pseudo gene-species this is the name of the species it originated from)

    string       gene_name;     // name of current gene
    GEN_gene_set gene_set;
    string       error_reason;  // reason why we can't display gene_map
    long         length;        // length of organism sequence

    GBDATA *gb_gene_data;       // i am build upon this
    int     change_flag;        // == 1 -> update needed

public:
    GEN_root(const char *organism_name_, const char *gene_name_, GBDATA *gb_main_, AW_root *aw_root);
    virtual ~GEN_root();

    const string& GeneName() const { return gene_name; }
    const string& OrganismName() const { return organism_name; }

    GBDATA *GbMain() { return gb_main; }

    void set_GeneName(const string& gene_name_) {
        if (gene_name != gene_name_) change_flag = -1;
        gene_name = gene_name_;
    }

    void paint(AW_device *device);

    int check_update(GBDATA *gbdummy);

    void reinit_NDS();
};



#else
#error GEN_gene.hxx included twice
#endif // GEN_GENE_HXX
