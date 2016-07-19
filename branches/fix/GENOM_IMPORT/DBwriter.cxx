// ================================================================ //
//                                                                  //
//   File      : DBwriter.cxx                                       //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "DBwriter.h"

#define AW_RENAME_SKIP_GUI

#include <algorithm>
#include <AW_rename.hxx>
#include <Translate.hxx>
#include <aw_question.hxx>
#include <GEN.hxx>
#include <adGene.h>

#define ARB_GENE_REF "ARB_is_gene"

using namespace std;

typedef SmartCustomPtr(GEN_position, GEN_free_position) GEN_positionPtr;

// --------------------------------------------------------------------------------

void DBerror::init(const string& msg, GB_ERROR gberror) {
    gi_assert(gberror);
    if (gberror) err = msg+" (Reason: "+gberror+")";
    else err = msg; // ndebug!
}

DBerror::DBerror() { init("", GB_await_error()); }
DBerror::DBerror(const char *msg) { string errmsg(msg); init(errmsg, GB_await_error()); }
DBerror::DBerror(const string& msg) { init(msg, GB_await_error()); }
DBerror::DBerror(const char *msg, GB_ERROR gberror) { string errmsg(msg); init(errmsg, gberror); }
DBerror::DBerror(const string& msg, GB_ERROR gberror) { init(msg, gberror); }

// --------------------------------------------------------------------------------
// DB access functions, throwing DBerror on failure

static GBDATA *DB_create_container(GBDATA *parent, const char *name, bool mark) {
    // create container (optionally set mark flag)
    GBDATA *gb_container = GB_create_container(parent, name);
    if (!gb_container) throw DBerror(GBS_global_string("Failed to create container '%s'", name));

    if (mark) GB_write_flag(gb_container, 1);

    return gb_container;
}

static GBDATA *DB_create_string_field(GBDATA *parent, const char *field, const char *content) {
    // create field with content

    gi_assert(content[0]);
    // do NOT WRITE empty string-fields into ARB DB,
    // cause ARB DB does not differ between empty content and non-existing fields
    // (i.e. when writing an empty string, ARB removes the field)

    GBDATA *gb_field = GB_create(parent, field, GB_STRING);
    if (!gb_field) throw DBerror(GBS_global_string("Failed to create field '%s'", field));

    GB_ERROR err = GB_write_string(gb_field, content);
    if (err) throw DBerror(GBS_global_string("Failed to write to field '%s'", field), err);

    return gb_field;
}

static GBDATA *DB_create_byte_field(GBDATA *parent, const char *field, unsigned char content) {
    // create field with content
    GBDATA *gb_field = GB_create(parent, field, GB_BYTE);
    if (!gb_field) throw DBerror(GBS_global_string("Failed to create field '%s'", field));

    GB_ERROR err = GB_write_byte(gb_field, content);
    if (err) throw DBerror(GBS_global_string("Failed to write to field '%s'", field), err);

    return gb_field;
}

// --------------------------------------------------------------------------------

void DBwriter::createOrganism(const string& flatfile, const char *importerTag)
{
    gi_assert(!gb_organism && !gb_gene_data);

    // create the organism
    {
        UniqueNameDetector& UND_species = *session.und_species;

        char *organism_name = AWTC_makeUniqueShortName("genome", UND_species);
        if (!organism_name) throw DBerror();

        gb_organism = DB_create_container(session.gb_species_data, "species", true);
        DB_create_string_field(gb_organism, "name", organism_name);

        UND_species.add_name(organism_name);
        free(organism_name);
    }

    // store info about source
    DB_create_string_field(gb_organism, "ARB_imported_from", flatfile.c_str());
    DB_create_string_field(gb_organism, "ARB_imported_format", importerTag);
}

class NoCaseCmp {
    static bool less_nocase(char c1, char c2) { return toupper(c1) < toupper(c2); }
    static bool nonequal_nocase(char c1, char c2) { return toupper(c1) != toupper(c2); }
public:
    NoCaseCmp() {}

    bool operator()(const string& s1, const string& s2) const {
        return lexicographical_compare(s1.begin(), s1.end(),
                                       s2.begin(), s2.end(),
                                       less_nocase);
    }

    static bool has_prefix(const string& s1, const string& s2) {
        // return true if s1 starts with prefix s2
        return
            s1.length() >= s2.length() &&
            !lexicographical_compare(s1.begin(), s1.begin()+s2.length(),
                                     s2.begin(), s2.end(),
                                     nonequal_nocase);
    }
};

typedef map<string, string, NoCaseCmp> TranslateMap;
struct Translator { TranslateMap trans; };

Translator *DBwriter::unreserve = 0;

const string& DBwriter::getUnreservedQualifier(const string& qualifier) {
    // return a non-reserved qualifier
    // (some are reserved - e.g. name, pos_start, ... and all starting with 'ARB_')
    // if a qualifier is reserved, 'ORG_' is prepended.
    //
    // (Note: When we'll export data, 'ORG_' shall be removed from qualifiers!)

    static string prefix = "ORG_";

    if (!unreserve) {
        unreserve = new Translator;
        const char *reserved[] = {
            "name", "type",
            "pos_start", "pos_stop", "pos_complement", "pos_certain", "pos_joined",
            0
        };

        for (int i = 0; reserved[i]; ++i) {
            unreserve->trans[reserved[i]] = prefix+reserved[i];
        }
    }

    TranslateMap::const_iterator found = unreserve->trans.find(qualifier);
    if (found != unreserve->trans.end()) { // qualifier is reserved
        return found->second;
    }
    if (NoCaseCmp::has_prefix(qualifier, prefix) || // qualifier starts with 'ORG_'
        NoCaseCmp::has_prefix(qualifier, "ARB_")) // or 'ARB_'
    {
        unreserve->trans[qualifier] = prefix+qualifier; // add as 'ORG_ORG_' or 'ORG_ARB_' to TranslateMap
        return unreserve->trans[qualifier];
    }
    return qualifier;
}

void DBwriter::writeFeature(const Feature& feature, long seqLength)
{
    gi_assert(gb_organism);
    if (!gb_gene_data) {
        gb_gene_data = DB_create_container(gb_organism, "gene_data", false);
        generatedGenes.clear();
    }

    // create new gene
    GBDATA *gb_gene;
    {
        string                gene_name = feature.createGeneName();
        NameCounter::iterator existing  = generatedGenes.find(gene_name);
        if (existing == generatedGenes.end()) { // first occurrence of that gene name
            generatedGenes[gene_name] = 1;
        }
        else {
            existing->second++; // increment occurrences
        }

        gb_gene = DB_create_container(gb_gene_data, "gene", false);
        DB_create_string_field(gb_gene, "name", gene_name.c_str());

        string type = feature.getType();
        DB_create_string_field(gb_gene, "type", type.c_str());

        if (type == "source") {
            DB_create_byte_field(gb_gene, ARB_HIDDEN, 1);
        }
    }

    // store location
    {
        GEN_positionPtr pos = feature.getLocation().create_GEN_position();
        GB_ERROR        err = GEN_write_position(gb_gene, &*pos, seqLength);
        if (err) throw DBerror("Failed to write location", err);
    }

    // store qualifiers
    {
        const stringMap& qualifiers = feature.getQualifiers();
        stringMapCIter   e          = qualifiers.end();

        for (stringMapCIter i = qualifiers.begin(); i != e; ++i) {
            const string& unreserved = getUnreservedQualifier(i->first);
            DB_create_string_field(gb_gene, unreserved.c_str(), i->second.c_str());
        }
    }
}

void DBwriter::writeSequence(const SequenceBuffer& seqData)
{
    gi_assert(gb_organism);
    GBDATA *gb_data = GBT_add_data(gb_organism, ali_name, "data", GB_STRING);
    if (!gb_data) throw DBerror("Failed to create alignment");

    GB_ERROR err = GB_write_string(gb_data, seqData.getSequence());
    if (err) throw DBerror("Failed to write alignment", err);
}

void DBwriter::renumberDuplicateGenes() {
    NameCounter renameCounter; // for those genes which get renumbered, count upwards here
    {
        NameCounter::iterator gg_end = generatedGenes.end();
        for (NameCounter::iterator gg = generatedGenes.begin(); gg != gg_end; ++gg) {
            if (gg->second > 1) renameCounter[gg->first] = 0;
        }
    }
    NameCounter::iterator rc_end = renameCounter.end();

    for (GBDATA *gb_gene = GB_entry(gb_gene_data, "gene");
         gb_gene;
         gb_gene = GB_nextEntry(gb_gene))
    {
        GBDATA *gb_name   = GB_entry(gb_gene, "name");
        string  gene_name = GB_read_char_pntr(gb_name);

        NameCounter::iterator rc = renameCounter.find(gene_name);
        if (rc != rc_end) { // rename current_gene
            int maxOccurrences = generatedGenes[gene_name];
            rc->second++;   // increment occurrence counter
            gi_assert(rc->second <= maxOccurrences);

            int digits = strlen(GBS_global_string("%i", maxOccurrences));
            gene_name    += GBS_global_string("_%0*i", digits, rc->second);
            GB_ERROR err  = GB_write_string(gb_name, gene_name.c_str());
            if (err) throw DBerror("Failed to write to field 'name' (during gene-renumbering)", err);
        }
    }
}

static void importerWarning(AW_CL cl_importer, const char *message) {
    Importer *importer = reinterpret_cast<Importer*>(cl_importer);
    importer->warning(message);
}

void DBwriter::testAndRemoveTranslations(Importer& importer) {
    GEN_testAndRemoveTranslations(gb_gene_data, importerWarning, reinterpret_cast<AW_CL>(&importer), session.ok_to_ignore_wrong_start_codon);
}

// ----------------------------------------------
//      hide duplicated genes (from genebank)

inline bool operator<(const GEN_positionPtr& A, const GEN_positionPtr& B) {
    const GEN_position& a = *A;
    const GEN_position& b = *B;

    int cmp = int(a.start_pos[0]) - int(b.start_pos[0]);
    if (!cmp) {
        cmp = int(a.stop_pos[a.parts-1]) - int(b.stop_pos[b.parts-1]);
        if (!cmp) {
            cmp = int(a.parts) - int(b.parts); // less parts is <
            if (!cmp) {
                for (int p = 0; p<a.parts; ++p) { // compare all parts
                    cmp = int(b.complement[p]) - int(a.complement[p]); if (cmp) break; // non-complement is <
                    cmp = int(a.start_pos[p])  - int(b.start_pos[p]);  if (cmp) break;
                    cmp = int(a.stop_pos[p])   - int(b.stop_pos[p]);   if (cmp) break;
                }
            }
        }
    }

    return cmp<0;
}

class PosGene {
    GBDATA                  *gb_Gene;
    mutable GEN_positionPtr  pos;

public:
    PosGene(GBDATA *gb_gene) : gb_Gene(gb_gene) {
        GEN_position *pp = GEN_read_position(gb_gene);
        if (!pp) {
            throw GBS_global_string("Can't read gene position (Reason: %s)", GB_await_error());
        }
        pos = pp;
    }
    PosGene(const PosGene& other)
        : gb_Gene(other.gb_Gene),
          pos(other.pos)
    {}
    DECLARE_ASSIGNMENT_OPERATOR(PosGene);

    const GEN_positionPtr& getPosition() const { return pos; }

    const char *getName() const {
        GBDATA *gb_name = GB_entry(gb_Gene, "name");
        gi_assert(gb_name);
        return GB_read_char_pntr(gb_name);
    }
    const char *getType() const {
        GBDATA *gb_type = GB_entry(gb_Gene, "type");
        gi_assert(gb_type);
        return GB_read_char_pntr(gb_type);
    }
    bool hasType(const char *type) const { return strcmp(getType(), type) == 0; }

    void hide() { DB_create_byte_field(gb_Gene, ARB_HIDDEN, 1); }
    void addRefToGene(const char *name_of_gene) { DB_create_string_field(gb_Gene, ARB_GENE_REF, name_of_gene); }

#if defined(DEBUG)
    const char *description() const {
        return GBS_global_string("%zi-%zi (%i parts)", pos->start_pos[0], pos->stop_pos[pos->parts-1], pos->parts);
    }
    void dump() {
        GB_dump(gb_Gene);
    }
#endif // DEBUG

};

inline bool operator<(const PosGene& a, const PosGene& b) {
    return a.getPosition() < b.getPosition();
}
inline bool operator == (const PosGene& a, const PosGene& b) {
    return !(a<b || b<a);
}

class hasType {
    const char *type;
public:
    hasType(const char *t) : type(t) {}
    bool operator()(const PosGene& pg) { return pg.hasType(type); }
};

void DBwriter::hideUnwantedGenes() {
    typedef vector<PosGene> Genes;
    typedef Genes::iterator GeneIter;

    Genes gps;

    // read all gene positions
    for (GBDATA *gb_gene = GB_entry(gb_gene_data, "gene"); gb_gene; gb_gene = GB_nextEntry(gb_gene)) {
        gps.push_back(PosGene(gb_gene));
    }

    sort(gps.begin(), gps.end()); // sort positions

    // find duplicate geness (with identical location)
    GeneIter end        = gps.end();
    GeneIter p          = gps.begin();
    GeneIter firstEqual = p;
    GeneIter lastEqual  = p;

    while (firstEqual != end) {
        ++p;
        if (p != end && *p == *firstEqual) {
            lastEqual = p;
        }
        else {                  // found different element (or last)
            int count = lastEqual-firstEqual+1;

            if (count>1) { // we have 2 or more duplicate genes
                GeneIter equalEnd = lastEqual+1;
                GeneIter gene     = find_if(firstEqual, equalEnd, hasType("gene")); // locate 'type' == 'gene'

                if (gene != equalEnd) { // found type gene
                    bool        hideGene  = false;
                    GeneIter    e         = firstEqual;
                    const char *gene_name = gene->getName();

                    while (e != equalEnd) {
                        if (e != gene) { // for all genes that have 'type' != 'gene'
                            e->addRefToGene(gene_name); // add a reference to the gene
                            hideGene = true; // and hide the gene
                        }
                        ++e;
                    }

                    if (hideGene) gene->hide();
                }
            }
            firstEqual = lastEqual = p;
        }
    }
}


void DBwriter::finalizeOrganism(const MetaInfo& meta, const References& refs, Importer& importer)
{
    // write metadata
    {
        const stringMap& entries = meta.getEntries();
        stringMapCIter   e       = entries.end();

        for (stringMapCIter i = entries.begin(); i != e; ++i) {
            const string& content = i->second;
            if (!content.empty()) { // skip empty entries
                DB_create_string_field(gb_organism, i->first.c_str(), content.c_str());
            }
        }
    }

    // write references
    {
        stringSet refKeys;
        refs.getKeys(refKeys);

        stringSetIter e = refKeys.end();
        for (stringSetIter i = refKeys.begin(); i != e; ++i) {
            DB_create_string_field(gb_organism, i->c_str(), refs.tagged_content(*i).c_str());
        }
    }

    // finalize genes data
    if (gb_gene_data) {
        renumberDuplicateGenes();            // renumber genes with equal names
        testAndRemoveTranslations(importer); // test translations and remove reproducible translations
        hideUnwantedGenes();
    }
    else GB_warning("No genes have been written (missing feature table?)");

    // cleanup
    generatedGenes.clear();
    gb_gene_data = 0;
    gb_organism  = 0;
}

void DBwriter::deleteStaticData() {
    if (unreserve) {
        delete unreserve;
        unreserve = NULL;
    }
}



