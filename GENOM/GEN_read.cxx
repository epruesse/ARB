#include <cstdio>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <awtc_rename.hxx>
#include <inline.h>

#include <string>
#include <list>
#include <iostream>
#include <map>

#include "GEN_local.hxx"

#define STD_DATA_START 12
#define FEATURE_DATA_START 21
#define SEQ_DATA_START 10

using namespace std;

//  -------------------------
//      class FileBuffer
//  -------------------------

#define BUFFERSIZE (64*1024)

class FileBuffer {
private:
    char buf[BUFFERSIZE];
    size_t read; // chars in buf
    size_t offset; // offset to next line

    FILE *fp;

    string *next_line;

    long   lineNumber;          // current line number
    string filename;

    void fillBuffer() {
        if (read==BUFFERSIZE) {
            read = fread(buf, sizeof(buf[0]), BUFFERSIZE, fp);
            offset = 0;
        }
        else {
            offset = read;
        }
    }

    bool getLine_intern(string& line) {
        if (offset==read) return false;
        char *cr = (char*)memchr(buf+offset, '\n', read-offset);
        if (cr) {
            size_t cr_offset = cr-buf;
            line = string(buf+offset, cr_offset-offset);
            offset = cr_offset+1;
            if (offset==read) fillBuffer();
        }
        else {
            line = string(buf+offset, read-offset);
            fillBuffer();
            string rest;
            if (getLine(rest)) line = line+rest;
        }
        return true;
    }

public:
    FileBuffer(const string& filename_) {
        filename = filename_;
        fp       = fopen(filename.c_str(), "rb");
        if (fp) {
            read = BUFFERSIZE;
            fillBuffer();
        }
        next_line  = 0;
        lineNumber = 0;
    }
    virtual ~FileBuffer() {
        delete next_line;
        if (fp) fclose(fp);
    }

    bool good() { return fp!=0; }

    bool getLine(string& line) {
        lineNumber++;
        if (next_line) {
            line = *next_line;
            delete next_line;
            next_line = 0;
            return true;
        }
        return getLine_intern(line);
    }

    long getLineNumber() const { return lineNumber; }

    void backLine(const string& line) {
        gen_assert(next_line==0);
        next_line = new string(line);
        lineNumber--;
    }

    GB_ERROR lineError(GB_CSTR msg) {
        return GBS_global_string("%s (#%li): %s", filename.c_str(), lineNumber, msg);
    }
};

//  ----------------------------------------------------
//      inline int parseLine_genbank(const string& line, string& section_name)
//  ----------------------------------------------------
// returns:  0 -> no section
//           1 -> main section (starting in column 0)
//           2 -> sub section (starting in column 2 or 3)
//           3 -> sub feature (starting in column 5)

inline bool space(char c) { return c==' '; }

inline int parseLine_genbank(const string& line, size_t data_start, string& section_name, string& rest) {
    if (line.empty()) return 0;

    int    first_char = -1;
    int    last_char  = -1;
    size_t p;

    for (p=0; p<data_start; ++p) {
        if (!space(line[p])) {
            first_char = p;
            break;
        }
    }
    for (; p<data_start; ++p) {
        if (space(line[p])) {
            last_char = p-1;
            break;
        }
    }

    int sec_type = 0;

    if (first_char!=-1) {
        gen_assert(last_char!=-1);
        switch (first_char) {
            case 0: sec_type = 1; break;
            case 2:
            case 3: sec_type = 2; break;
            case 5: sec_type = 3; break;
            default: gen_assert(0);
        }
        section_name = string(line.data()+first_char, last_char-first_char+1);
    }
    else { // Folgezeile
        gen_assert(last_char==-1);
        section_name = "";
    }

    if (line.length()>=data_start) {
        rest = string(line.data()+data_start, line.length()-data_start);
    }
    else {
        rest = "";
    }

    return sec_type;
}

//  ---------------------------------------------------
//      inline int count_qm(const string& content)
//  ---------------------------------------------------
inline size_t count_qm(const string& content) {
    size_t count = 0;
    for (size_t p=0; p<content.length(); ++p) {
        if (content[p]=='\"') ++count;
    }
    return count;
}

//  ------------------------------------------------------------------------------------------------------------
//      void completeSection(FileBuffer& file, string& content, size_t data_start, bool smart_spaces=false)
//  ------------------------------------------------------------------------------------------------------------
void completeSection(FileBuffer& file, string& content, size_t data_start, bool smart_spaces=false) {
    string next_line;
    string dummy;
    string next_content;
    size_t qm = smart_spaces ? count_qm(content) : 0;

    while (file.getLine(next_line)) {
        int next_section = parseLine_genbank(next_line, data_start, dummy, next_content);

        if (next_section!=0) {
            file.backLine(next_line);
            break;
        }

        if (smart_spaces && (qm%2)==1) {
            content = content + next_content;
        }
        else {
            content = content + " " + next_content;
        }
        qm += count_qm(next_content);
    }

}

//  ---------------------------------------------------------------------------------------------------------------------------------------
//      static bool parseFeatureEntry(const char *& buffer, string& entry_name, string& entry_content, bool& is_alpha, GB_ERROR error)
//  ---------------------------------------------------------------------------------------------------------------------------------------
static bool parseFeatureEntry(const char *& buffer, string& entry_name, string& entry_content, bool& is_alpha, GB_ERROR error) {
    while (space(buffer[0])) ++buffer;

    if (buffer[0]=='/') {
        const char *eq = strchr(buffer+1, '=');
        const char *sp = strchr(buffer+1, ' ');

        if (sp && eq && sp<eq) { // entry has no '=' (avoid using next entry)
            entry_name = string(buffer+1, sp-buffer-1);
            entry_content = "true";
            is_alpha = true;
            buffer = sp+1;
            return true;
        }

        if (eq) {
            entry_name = string(buffer+1, eq-buffer-1);
            buffer = eq+1;
            while (space(buffer[0])) ++buffer;
            const char *end = 0;
            if (buffer[0]=='\"') {
                is_alpha = true;
                end = strchr(buffer+1, '\"');
                if (end) {
                    entry_content = string(buffer+1, end-buffer-1);
                }
            }
            else {
                is_alpha = false;
                end = strchr(buffer+1, ' ');
                if (!end) end = strchr(buffer+1, 0);
                if (end) {
                    entry_content = string(buffer, end-buffer);
                }
            }

            if (end) {
                buffer = end+1;
                return true;
            }
        }
        error = GB_export_error("incomplete entry in '%s'", buffer);
    }
    return false;
}


//  --------------------------------------------------------------------------
//      static string removeFunction(string& content, const char *&error)
//  --------------------------------------------------------------------------
//  removes a function from content and returns the name of the function
//  example:    removeFunction("complement(777...888)")
//              returns "complement"
//              content changed to "777...888"

static string removeFunction(string& content, const char *&error) {
    size_t popen = content.find('(');
    string function;

    if (popen == string::npos)  {
        function = "";
    }
    else {
        function = content.substr(0, popen);

        size_t pclose = content.rfind(')');
        if (pclose == string::npos) {
            error   = GBS_global_string("')' awaited after %s", function.c_str());
        }
        else {
            content = content.substr(popen+1, pclose-popen-1);
        }
    }
    return function;
}

//  -------------------------
//      class GeneBorder
//  -------------------------
class GeneBorder {
private:
    char sureness;              // 0 -> sure; '<' -> maybe lower; '>' -> maybe higher
    long pos;

public:
    // format of pos_string:
    //  ['<'|'>']<num>
    GeneBorder() : sureness(0), pos(0) {}
    virtual ~GeneBorder() {}

    GB_ERROR parsePosString(const string& pos_string) {
        const char *p = pos_string.c_str();
        if (p[0] == '<' || p[0] == '>') {
            sureness  = p[0];
            ++p;
        }
        char *end = 0;
        pos             = strtoul(p, &end, 10);
        if (end[0] != 0) {
            return GBS_global_string("Garbage ('%s') found after position", end);
        }
        return 0;
    }

    long getPos() const { return pos; }
    char getSureness() const { return sureness; }
};


//  ---------------------------
//      class GenePosition
//  ---------------------------
// class GenePosition {
// private:
//     GeneBorder lower, upper;

// public:
//     // format of pos_string:
//     //  ['<'|'>']<num>'...'['<'|'>']<num>
//     GenePosition(const string& pos_string, const char *&error) {
//         size_t points         = pos_string.find("..");
//         if (points != string::npos) {
//             error             = lower.parsePosString(pos_string.substr(0, points));
//             if (!error) error = upper.parsePosString(pos_string.substr(points+2));
//         }
//         else {                  // lower equal upper border
//             error             = lower.parsePosString(pos_string);
//             if (!error) upper = lower;
//         }
//     }
//     virtual ~GenePosition() {}

//     const GeneBorder& getLower() const { return lower; }
//     const GeneBorder& getUpper() const { return upper; }
// };


//  ---------------------------
//      class GenePosition
//  ---------------------------
class GenePosition {
private:
    GeneBorder lower, upper;

public:
    // format of pos_string:
    //  ['<'|'>']<num>'...'['<'|'>']<num>
    GenePosition(const string& pos_string, const char *&error) {
        size_t points         = pos_string.find("..");
	//        size_t points         = pos_string.find("..");
        if (points != string::npos) {
            error             = lower.parsePosString(pos_string.substr(0, points));
	    //            if (!error) error = upper.parsePosString(pos_string.substr(points+2));
            if (!error) error = upper.parsePosString(pos_string.substr(points+2));
        }
        else {                  // lower equal upper border
            error             = lower.parsePosString(pos_string);
            if (!error) upper = lower;
        }
    }
    virtual ~GenePosition() {}

    const GeneBorder& getLower() const { return lower; }
    const GeneBorder& getUpper() const { return upper; }
};


//  --------------------------------------------------------------------------
//      GB_ERROR GEN_insert_warning(GBDATA *gb_item, const char *warning)
//  --------------------------------------------------------------------------
GB_ERROR GEN_insert_warning(GBDATA *gb_item, const char *warning) {
    // @@@ FIXME: Should append and not overwrite ARB_warning
    GBDATA *gb_warning = GB_search(gb_item, "ARB_warning", GB_STRING);
    GB_write_string(gb_warning, warning);

    return 0;
}


typedef map<string, string> GeneNameSubstitute;

GB_ERROR parseFeature(string section_name, string content, FileBuffer& file,
                      GeneNameSubstitute& gene_name_subst, GBDATA* gb_species, unsigned long data_length)
{
    GB_ERROR       error            = 0;

#if defined(DEBUG) && 0
    cout << "Feature='" << section_name << "'\n";
    cout << "content='" << content << "'\n";
#endif // DEBUG

    bool complement       = false;
    bool several_parts    = false; // gene is joined from several parts

    list<GenePosition> gene_positions;

    {
        string pos_string;
        {
            size_t space = content.find(' ');
            if (space) {
                pos_string = content.substr(0, space);
                content    = content.substr(space+1);
            }
            else {
                pos_string = content;
                content    = "";
            }
        }

        while (1) {
            string function = removeFunction(pos_string, error);

            if (error || function == "") break;

            if (function == "complement") complement = true;
            else if ((function == "join") || (function == "order")) several_parts = true;
            else  {
                error = file.lineError((string("Unknown function '")+function+"'").c_str());
                break;
            }
        }


        if (!error) {
            if (several_parts) {
                while (!error) {
                    size_t komma  = pos_string.find(',');
                    if (komma == string::npos) break;
                    string prefix = pos_string.substr(0, komma);
                    pos_string    = pos_string.substr(komma+1);

                    gene_positions.push_back(GenePosition(prefix, error));
                }
            }
            if (!error) {
                gene_positions.push_back(GenePosition(pos_string, error));
            }
        }
    }

    const char *cont = content.c_str();

    if (!error) {
        typedef map<string,string> EntryMap;
        EntryMap                   featureEntries;

        while (1) {
            string entry_name;
            string entry_content;
            bool is_alpha;
            bool has_entries = parseFeatureEntry(cont, entry_name, entry_content, is_alpha, error);

            if (!has_entries) break;

            featureEntries[entry_name] = entry_content;
        }

        bool used = false;

        // --------------------------------------------------------------------------------
        if (section_name=="source") { // insert feature-entries of source entry into organism
            for (EntryMap::const_iterator e=featureEntries.begin();e!=featureEntries.end();++e) {
                const string& entry_name = e->first;

                GBDATA *gb_field = GB_find(gb_species, entry_name.c_str(), NULL, down_level);
                const char *entry_content = e->second.c_str();
                if (gb_field) {
                    char *old_content = strdup(GB_read_char_pntr(gb_field));


                    if (ARB_stricmp(old_content, entry_content) != 0) {
                        if (old_content[0]==0) GB_write_string(gb_field, entry_content);
                        else GB_write_string(gb_field, GBS_global_string("%s; %s", old_content, entry_content));
                    }

                    free(old_content);

                }
                else {
                    GBDATA *gb_field = GB_search(gb_species, entry_name.c_str(), GB_STRING);
                    GB_write_string(gb_field, entry_content);
                }
            }

            used = true;
        }


        // --------------------------------------------------------------------------------
        if (!used) {
            string                       gene_name; // not necessarily a gene (i.e rRNA.xx, misc_feature.xx, etc.)
            EntryMap::iterator           gene_entry     = featureEntries.find("gene");
            GeneNameSubstitute::iterator subst          = gene_name_subst.end();
            bool                         handle_as_gene = false;

            if (gene_entry!=featureEntries.end()) // we have a "gene"-entry!
            {
                gene_name = gene_entry->second;
                subst     = gene_name_subst.find(gene_name);

                if (subst != gene_name_subst.end()) // gene_name is already substituted
                {
                    gene_name = subst->second;
                }
                handle_as_gene = (section_name == "gene");
            }
            else // handle features w/o 'gene' entry
            {
                gene_name      = section_name;
                subst          = gene_name_subst.find(gene_name);
                handle_as_gene = true;
            }

            GBDATA *gb_gene          = GEN_create_gene(gb_species, gene_name.c_str()); // create or search gene
            GBDATA *gb_sub_container = 0;

            if (handle_as_gene) {
                // test if gene with same name exists:
                GBDATA *pos_begin = GB_find(gb_gene, "pos_begin", 0, down_level);

                if (pos_begin) // oops - gene with same name exists already
                {
                    int this_counter;

                    if (subst != gene_name_subst.end()) // gene_name is already substituted
                    {
                        string last_subst = subst->second;
                        gene_name         = subst->first; // restore original gene-name

                        int  last_counter = atoi(last_subst.c_str()+gene_name.length()+1);
                        char buf[10];
                        sprintf(buf, ".%02i", last_counter+1);

                        gene_name_subst[gene_name] = gene_name+buf;
                        this_counter               = last_counter+1;
                    }
                    else {
                        gene_name_subst[gene_name] = gene_name+".02";
                        this_counter               = 2;
                    }

#if defined(DEBUG) && 0
                    cout << "Name-Substitution : '" << gene_name << "' -> '";
#endif // DEBUG

                    gene_name = gene_name_subst[gene_name];
#if defined(DEBUG) && 0
                    cout << gene_name << "'\n";
#endif // DEBUG

                    gb_gene = GEN_create_gene(gb_species, gene_name.c_str()); // create or search gene
                }

                gb_sub_container  = gb_gene;
            }
            else {
                string sub_container_name = section_name;
                char   buf[50];
                int    counter            = 1;

                strcpy(buf, sub_container_name.c_str());

                while (1) {
                    char *buf2key = GBS_string_2_key(buf);
                    strcpy(buf, buf2key);
                    free(buf2key);

                    if (GB_find(gb_gene, buf, 0, down_level) == 0) break;
                    ++counter;
                    sprintf(buf, "%s_%02i", sub_container_name.c_str(), counter);
                }

                gb_sub_container = GB_create_container(gb_gene, buf);
            }

            for (EntryMap::const_iterator e=featureEntries.begin();e != featureEntries.end();++e)
            {
                const string& entry_name = e->first;

                if (entry_name!="gene" && entry_name!="translation") {
                    GBDATA     *gb_field      = GB_find(gb_sub_container, entry_name.c_str(), NULL, down_level);
                    const char *entry_content = e->second.c_str();
                    if (gb_field) {
                        char *old_content = strdup(GB_read_char_pntr(gb_field));

                        if (ARB_stricmp(old_content, entry_content)!=0) {
                            if (old_content[0]==0) {
                                GB_write_string(gb_field, entry_content);
                            }
                            else {
                                GB_write_string(gb_field, GBS_global_string("%s; %s", old_content, entry_content));
                            }
                        }
                        free(old_content);
                    }
                    else {
                        GBDATA *gb_field = GB_search(gb_sub_container, entry_name.c_str(), GB_STRING);
                        GB_write_string(gb_field, entry_content);
                    }
                }
            }

            // obligatory gene-entries:
            int     pos_counter = 0;
            GBDATA *gb_field    = 0;

            for (list<GenePosition>::const_iterator gp = gene_positions.begin();
                 gp != gene_positions.end();
                 ++gp, ++pos_counter)
            {
                bool uncertain = false;

                for (int end = 0; end <= 1; ++end) {
                    const char *field_name           = end ? "pos_end" : "pos_begin";
                    const char *real_field_name      = field_name;
                    if (pos_counter) real_field_name = GBS_global_string("%s%i", field_name, pos_counter+1);

                    gb_field                 = GB_search(gb_sub_container, real_field_name, GB_INT);
                    const GeneBorder& border = end ? gp->getUpper() : gp->getLower();
                    GB_write_int(gb_field, border.getPos());

                    if (end && data_length>0) {
                        aw_status(double(border.getPos())/double(data_length));
                    }

                    if (border.getSureness()) uncertain = true;
                }

                if (uncertain) {
                    char sureness[] = "==";
                    char s;

                    s = gp->getLower().getSureness(); if (s) sureness[0] = s;
                    s = gp->getUpper().getSureness(); if (s) sureness[1] = s;

                    const char *field_name           = "pos_uncertain";
                    const char *real_field_name      = field_name;
                    if (pos_counter) real_field_name = GBS_global_string("%s%i", field_name, pos_counter+1);

                    gb_field = GB_search(gb_sub_container, real_field_name, GB_STRING);
                    GB_write_string(gb_field, sureness);
                }
            }

            if (pos_counter>1) {
                gb_field = GB_search(gb_sub_container, "pos_joined", GB_INT);
                GB_write_int(gb_field, pos_counter);
            }

            gb_field = GB_search(gb_sub_container, "complement", GB_BYTE);
            GB_write_byte(gb_field, int(complement));

            used = true;
        }
    }
    return error;
}


//  ---------------------------------------------------------------------------------------
//      GB_ERROR GEN_read_genbank(GBDATA *gb_main, const char *filename, const char *ali_name)
//  ---------------------------------------------------------------------------------------
GB_ERROR GEN_read_genbank(GBDATA *gb_main, const char *filename, const char *ali_name){
    GB_ERROR       error            = 0;
    FileBuffer     file(filename);
    string         line;
    string         section_name;
    string         content;
    GBDATA        *gb_species_data  = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    char          *new_species_name = AWTC_makeUniqueShortName("genom", gb_species_data);
    GBDATA        *gb_species       = GBT_create_species(gb_main, new_species_name);
    int            reference        = 0; // count "REFERENCE"
    bool           data_read        = false;
    unsigned long  data_length      = 0; // 0: not read, still unknown length

    //     typedef map<string, int> NonGene;
    //     NonGene                  nonGene;

    GeneNameSubstitute          gene_name_subst;

    if (!file.good()) error = GBS_global_string("Can't open '%s'", filename);

    while (!error && file.getLine(line)) {
        int section_type = parseLine_genbank(line, STD_DATA_START, section_name, content);

        if (section_type==1) {
            if (section_name=="ORIGIN") {
                if (data_length==0) error = "could not detect sequence length -- check input format";
                gen_assert(data_length!=0);
                char *data = new char[data_length+1];
                char *dp = data;

                while (!error) {
                    if (file.getLine(line)) {
                        if (line=="//") {
                            data_read = true;
                            break;
                        }

                        size_t p;
                        for (p=0; space(line[p]); ++p) ;
                        for (; isdigit(line[p]); ++p) ;
                        for (; space(line[p]); ++p) ;

                        gen_assert(p==SEQ_DATA_START);
                        gen_assert(p<line.length());

                        for (; p<line.length(); ++p)  {
                            if (!space(line[p])) {
                                *dp++ = toupper(line[p]);
                            }
                        }
                    }
                    else {
                        error = "Unexpected end of file";
                    }
                }

                if (data_read) {
                    unsigned long seq_length = (unsigned long)(dp-data); // real length of the sequence

                    gb_assert(seq_length <= data_length); // if > then buffer overflow in data!!!
                    if (seq_length != data_length)
                    {
                        error = file.lineError(GBS_global_string("Data length mismatch (length of sequence = %lu ; specified length = %lu)",
                                                                 seq_length, data_length));
                    }
                    else {
                        dp[0]           = 0;
                        GBDATA *gb_data = GBT_add_data(gb_species, ali_name, "data", GB_STRING);
                        GB_write_string(gb_data, data);
                    }
                }

                delete data;

                break; // stop reading
            }
            else if (section_name=="FEATURES") {
                completeSection(file, content, STD_DATA_START);
                while (file.getLine(line)) {
                    int section_type = parseLine_genbank(line, FEATURE_DATA_START, section_name, content);
                    if (section_type!=3) {
                        file.backLine(line);
                        break;
                    }
                    completeSection(file, content, FEATURE_DATA_START, true);

                    error = parseFeature(section_name, content, file, gene_name_subst, gb_species, data_length);
                }
            }
            else {
                completeSection(file, content, STD_DATA_START);
                if (section_name=="ACCESSION") {
                    GBDATA *gb_acc = GB_search(gb_species, "acc", GB_STRING);
                    GB_write_string(gb_acc, content.c_str());
                }
                else if (section_name=="VERSION") {
                    GBDATA *gb_version = GB_search(gb_species, "version", GB_STRING);
                    GB_write_string(gb_version, content.c_str());
                }
                else if (section_name=="LOCUS") {
                    int p;
                    for (p=0; !space(content[p]); ++p) ;
                    for (; space(content[p]); ++p) ;
                    int count_start = p;
                    for (; !space(content[p]); ++p) ;
                    for (; space(content[p]); ++p) ;

                    const char *cont = content.c_str();
                    if (strncmp(cont+p, "bp", 2)==0) {
                        data_length = strtoul(cont+count_start, 0, 10);
                        //  if (data_length>0) data_length_read = true;
                        GB_write_int( GB_search(gb_species, "sequence length", GB_INT), data_length);
                    }

                    if (data_length==0) {
                        error = "Format-Error in LOCUS: could not find bp (awaited 'LOCUS acc count bp ...')";
                    }
                }
                else if (section_name=="SOURCE") {
                    string full_name = content;
                    if (file.getLine(line)) {
                        section_type = parseLine_genbank(line, STD_DATA_START, section_name, content);
                        if (section_type==2 && section_name=="ORGANISM") {
                            full_name = content;
                            content = "";
                            completeSection(file, content, STD_DATA_START);

                            GBDATA *gb_tax = GB_search(gb_species, "tax", GB_STRING);
                            GB_write_string(gb_tax, content.c_str());
                        }
                        else {
                            file.backLine(line);
                        }
                        //GBDATA *gb_full_name = GB_search(gb_species, "full_name", GB_STRING);
                        //GB_write_string(gb_full_name, content.c_str());
                    }
                }
                else if (section_name=="DEFINITION") {
                    GBDATA *gb_full_name = GB_search(gb_species, "full_name", GB_STRING);
                    GB_write_string(gb_full_name, content.c_str());
                }
                else if (section_name=="REFERENCE") {
                    reference++;
                    completeSection(file, content, STD_DATA_START);
                    while (file.getLine(line)) {
                        section_type = parseLine_genbank(line, STD_DATA_START, section_name, content);
                        if (section_type!=2) {
                            file.backLine(line);
                            break;
                        }
                        completeSection(file, content, STD_DATA_START);

                        const char *entry_name = 0;

                        if (section_name=="AUTHORS") entry_name = "author";
                        else if (section_name=="TITLE") entry_name = "title";
                        else if (section_name=="JOURNAL") entry_name = "journal";

                        if (entry_name) {
                            GBDATA *gbd = GB_find(gb_species, entry_name, 0, down_level);
                            if (!gbd) {
                                gbd = GB_create(gb_species, entry_name, GB_STRING);
                                GB_write_string(gbd, content.c_str());
                            }
                            else {
                                string old_content = GB_read_char_pntr(gbd);
                                if (reference==2) old_content = string("[1] ")+old_content;
                                char buf[10];
                                sprintf(buf, "\n[%i] ", reference);
                                string new_content = old_content + buf + content;
                                GB_write_string(gbd, new_content.c_str());

                            }
                        }
                    }
                }
                else {
                    cout << "Ignoring section '" << section_name << "'\n";
                }
            }
        }
    }
    return error;
}

//  ----------------------------------------------------
//      inline int parseLine_embl(const string& sub_section, string& content)
//  ----------------------------------------------------
// returns:  0 -> no section
//           1 -> main section (starting in column 0)
//           2 -> sub section (starting in column 2 or 3)
//           3 -> sub feature (starting in column 5)

const char* SECTION_INDICATOR[] = {   "  ", "FT", "ID",   "XX",   "AC",  "SV",  "DT",  "DE",  "KW",  "OS",  "OC",  "RN",  "RA",  "RT",  "RL",  "RP",  "CC",  "FH",  "SQ" , "DR"};
enum sc_section                 { SC_SQ2 = 0, SC_FT, SC_ID, SC_XX, SC_AC, SC_SV, SC_DT, SC_DE, SC_KW, SC_OS, SC_OC, SC_RN, SC_RA, SC_RT, SC_RL, SC_RP, SC_CC, SC_FH, SC_SQ, SC_DR, SC_MAX };

const char* SUBSECTION_INDICATOR[] = { 	"Key", "source", "CDS", "misc_feature", "promoter", "protein_bind","stem_loop", "repeat_region", "tRNA",   "rRNA", "mRNA", "repeat_unit", "misc_difference", "misc_RNA", "5'UTR", "3'UTR", "RBS", "terminator", "intron", "LTR", "variation", "rep_origin", "D-loop", "mat_peptide", "exon", "gene", "snoRNA", " polyA_signal", "-35_signal", "-10_signal", "snRNA", "scRNA", "misc_structure", "sig_peptide", "primer_bind", "misc_signal", "conflict" };

enum ss_sections              { SS_KEY = 0, SS_SOURCE, SS_CDS, SS_MISC_FEATURE, SS_PROMOTER, SS_PROTEIN_BIND, SS_STEM, SS_REPEAT_REGION, SS_TRNA, SS_RRNA, SS_MRNA, SS_REPEAT_UNIT, SS_MISC_DIFFERENCE, SS_MISC_RNA, SS_5UTR, SS_3UTR, SS_RBS, SS_TERMINATOR, SS_INTRON, SS_LTR, SS_VARIATION, SS_REP_ORI, SS_D_LOOP, SS_MAT_PEPTIDE, SS_EXON, SS_GENE, SS_SNORNA, SS_POLYA, SS_35_SIG, SS_10_SIG, SS_SNRNA, SS_SCRNA, SS_MISC_STRUCTURE, SS_SIG_PEPTIDE, SS_PRIMER_BIND, SS_MISC_SIGNAL, SS_CONFLICT, SS_MAX };

inline int parseLine_embl(const string& line, int& sub_section, string& content) {
    if (line.empty()) return 0;

    int i, first, last;
    int section_type = -1;	// Standardmaessig "no section"
    const char* l = line.c_str();

    sub_section = -1;

    // Auswertung der Sektion - 2 Zeichen-Kuerzel am Anfang von (fast) jeder Zeile
    for(i=0;i<SC_MAX;i++)
        if ((SECTION_INDICATOR[i][0] == l[0]) &&
            (SECTION_INDICATOR[i][1] == l[1])) {
            section_type = i;
            break;
        }

    for(first = 2; l[first] == ' '; first++);
    for(last = first; l[last] != ' ' && l[last] != 0; last++);
    if (first==last) return section_type;

    // Subsections only for FT and FH
    if ((section_type == SC_FT) || (section_type == SC_FH)) {
        if (first > 10)
            sub_section = SS_MAX;
        else {
            for(i=0;i<SS_MAX;i++)
                if (strncmp(l+first, SUBSECTION_INDICATOR[i], last-first) == 0) {
                    sub_section = i;
                    break;
                }

            for(first = last; l[first] == ' '; first++);
        }
    }
    content = string(line.data()+first, line.length()-first);

#if defined(DEBUG)
    //cout << " section type is " << section_type << "\n";
#endif // DEBUG
    return section_type;
}


// Write the sequence data to string 'dp' (until end of string, ignore spaces)
char* get_sequenceline(char* dp, const char* s)
{
    while(*s)
    {
        if(*s >= 'a' && *s <= 'z')
	{
	    *dp= *s + ('A'-'a');
	    dp++;
	}
	else if(*s >= 'A' && *s <= 'Z')
	{
	    *dp= *s;
	    dp++;
	}
        s++;
    }
    return dp;
}


GB_ERROR parseFeature_embl(string section_name, string content, FileBuffer& file,
                           GeneNameSubstitute& gene_name_subst, GBDATA* gb_species, unsigned long data_length)
{
    GB_ERROR       error            = 0;

#if defined(DEBUG) && 0
    cout << "Feature='" << section_name << "'\n";
    cout << "content='" << content << "'\n";
#endif // DEBUG

    // This is a quick'n'dirty fix if a join occurs. It helps to avoid the need
    // for bracket-counting by parsing joins from _embl_ to _genbank_ format
    if((content.substr(0,4)=="join") || (content.substr(0,5)=="order"))
    {
	int contentpos= content.find("complement");
	int ccpos= content.find(" /");
	string tmp_content1= content.substr(0, ccpos);
	string tmp_content2= "complement(join(";

	if((ccpos > 0) && (contentpos > 0))
	{
	    int pos= tmp_content1.find("complement");
	    int counter= 0; // counter prevents infinite loops (just for debugging)
	    while((pos >= 0) && (counter < 99))
	    {
	        int pos2= tmp_content1.find('(', pos);
		int pos3= tmp_content1.find(')', pos2);

		tmp_content2+= tmp_content1.substr(pos2+1, (pos3-pos2-1)) + ",";
		pos= tmp_content1.find("complement", pos3);
		counter++;
	    }
	    tmp_content2= tmp_content2.substr(0, tmp_content2.length() - 1) + "))";
	    content= tmp_content2 + content.substr(ccpos, content.length());
	}
    }

    bool complement       = false;
    bool several_parts    = false; // true, if gene is joined from several parts

    list<GenePosition> gene_positions;

    {
        string pos_string;
        {
            size_t space = content.find('/'); // anstatt ' ' für mehrere Zeilen!
            if (space) {
		if (content[space-1] == ' ') space--;

                pos_string = content.substr(0, space);
                content    = content.substr(space+1);
            }
            else {
                pos_string = content;
                content    = "";
            }
        }

        while (1) {
            string function = removeFunction(pos_string, error);

            if (error || function == "") break;

            if (function == "complement") complement = true;
            else if (function == "join" || (function == "order")) several_parts = true;
            else  {
                error = file.lineError((string("Unknown function '")+function+"'").c_str());
                break;
            }
        }


        if (!error) {
            if (several_parts) {
                while (!error) {
                    size_t komma  = pos_string.find(',');
                    if (komma == string::npos) break;
                    string prefix = pos_string.substr(0, komma);
		    if (pos_string[komma+1] == ' ') komma++; // skip space after ','
                    pos_string    = pos_string.substr(komma+1);

                    gene_positions.push_back(GenePosition(prefix, error));
                }
            }
            if (!error) {
                gene_positions.push_back(GenePosition(pos_string, error));
            }
        }
    }

    const char *cont = content.c_str();

    if (!error) {
        typedef map<string,string> EntryMap;
        EntryMap                   featureEntries;

        while (1) {
            string entry_name;
            string entry_content;
            bool is_alpha;
            bool has_entries = parseFeatureEntry(cont, entry_name, entry_content, is_alpha, error);

            if (!has_entries) break;

            featureEntries[entry_name] = entry_content;
        }

        bool used = false;

        // --------------------------------------------------------------------------------
        if (section_name=="source") { // insert feature-entries of source entry into organism
            for (EntryMap::const_iterator e=featureEntries.begin();e!=featureEntries.end();++e) {
                const string& entry_name = e->first;

                GBDATA *gb_field = GB_find(gb_species, entry_name.c_str(), NULL, down_level);
                const char *entry_content = e->second.c_str();
                if (gb_field) {
                    char *old_content = strdup(GB_read_char_pntr(gb_field));

                    if (ARB_stricmp(old_content, entry_content)!=0) {
                        if (old_content[0]==0) GB_write_string(gb_field, entry_content);
                        else GB_write_string(gb_field, GBS_global_string("%s; %s", old_content, entry_content));
                    }

                    free(old_content);

                }
                else {
                    GBDATA *gb_field = GB_search(gb_species, entry_name.c_str(), GB_STRING);
                    GB_write_string(gb_field, entry_content);
                }
            }

            used = true;
        }


        // --------------------------------------------------------------------------------
        if (!used) {
            string                       gene_name; // not necessarily a gene (i.e rRNA.xx, misc_feature.xx, etc.)
            EntryMap::iterator           gene_entry     = featureEntries.find("gene");
            GeneNameSubstitute::iterator subst          = gene_name_subst.end();
            bool                         handle_as_gene = false;

            if (gene_entry!=featureEntries.end()) // we have a "gene"-entry!
            {
                gene_name = gene_entry->second;
                subst     = gene_name_subst.find(gene_name);

                if (subst != gene_name_subst.end()) // gene_name is already substituted
                {
                    gene_name = subst->second;
                }
                handle_as_gene = (section_name == "gene" || section_name == "CDS");
            }


	    else // handle features w/o 'gene' entry
            {
                if (section_name == "misc_feature") {
		  GeneNameSubstitute::iterator last = gene_name_subst.find("last_gene");
		  //GeneNameSubstitute::iterator gene = featureEntries.find("gene");
		  //gene_name = gene->second;
		  //handle_as_gene = true;
		     if (last == gene_name_subst.end()) {
			 //if (gene == featureEntries.end()) {
		        //handle_as_gene == true;
		        EntryMap::iterator note = featureEntries.find("note");
		        gene_name = note->second;
			//error =  "misc_feature without a Gene";
		        //return error;
		}
		    else {
                    gene_name = last->second;
		    }
                }

                else if (section_name == "tRNA") {
		    //GeneNameSubstitute::iterator gene = featureEntries.find("gene");
		     GeneNameSubstitute::iterator product = featureEntries.find("product");
                    //GeneNameSubstitute::iterator product = featureEntries.find("product");
		    if (product!=featureEntries.end()) {
			cout << "no gene-Entry found, try to write product-Entry as gene_name" << "'\n";

			//if (product == featureEntries.end()) {
			//error =  "No entry '/gene' found in tRNA feature";
			//return error;
			 gene_name = product->second;
			 handle_as_gene = true;

		    }
		    else {
			//gene_name = gene->second;
			//gene_name = product->second;
			handle_as_gene = true;
		    }
                }

                else if (section_name == "rRNA") {
                    EntryMap::iterator note  = featureEntries.find("note");
                    EntryMap::iterator product  = featureEntries.find("product");
                    if (note == featureEntries.end()) {
			gene_name = product->second;
			handle_as_gene = true;
			    //error = "No note found in rRNA feature";
			    //return error;
                    }

                    else {
		    gene_name = note->second;
                    //gene_name = product->second;
                    handle_as_gene = true;
		    }
                }
                else {
                    gene_name      = section_name;
                    subst          = gene_name_subst.find(gene_name);
                    handle_as_gene = true;
                }
            }



//                 else if (section_name == "rRNA") {
//                     EntryMap::iterator note  = featureEntries.find("note");
//                     EntryMap::iterator product  = featureEntries.find("product");
// 		    EntryMap::iterator standard_name = featureEntries.find("standard_name");
//                     if (note == featureEntries.end()) {
// 			gene_name = product->second;
// 			handle_as_gene = true;
// 			    //error = "No note found in rRNA feature";
// 			    //return error;
//                     }

//                     else if {
// 		    gene_name = note->second;
//                     //gene_name = product->second;
//                     handle_as_gene = true;
// 		    }
		    
// 		    else {
// 		      gene_name = standard_name->second;
// 		      handle_as_gene = true;
// 		    }


//                 }

//                 else {
//                     gene_name      = section_name;
//                     subst          = gene_name_subst.find(gene_name);
//                     handle_as_gene = true;
//                 }
//             }

            GBDATA *gb_gene          = GEN_create_gene(gb_species, gene_name.c_str()); // create or search gene
            GBDATA *gb_sub_container = 0;

            if (handle_as_gene) {
                // test if gene with same name exists:
                GBDATA *pos_begin = GB_find(gb_gene, "pos_begin", 0, down_level);

                if (pos_begin) // oops - gene with same name exists already
                {
                    int this_counter;

                    if (subst != gene_name_subst.end()) // gene_name is already substituted
                    {
                        string last_subst = subst->second;
                        gene_name         = subst->first; // restore original gene-name

                        int  last_counter = atoi(last_subst.c_str()+gene_name.length()+1);
                        char buf[10];
                        sprintf(buf, ".%02i", last_counter+1);

                        gene_name_subst[gene_name] = gene_name+buf;
                        this_counter               = last_counter+1;
                    }
                    else {
                        gene_name_subst[gene_name] = gene_name+".02";
                        this_counter               = 2;
                    }

#if defined(DEBUG) && 0
                    cout << "Name-Substitution : '" << gene_name << "' -> '";
#endif // DEBUG

                    gene_name = gene_name_subst[gene_name];
#if defined(DEBUG) && 0
                    cout << gene_name << "'\n";
#endif // DEBUG

                    gb_gene = GEN_create_gene(gb_species, gene_name.c_str()); // create or search gene
                }

                gb_sub_container  = gb_gene;
            }
            else {
                string sub_container_name = section_name;
                char   buf[50];
                int    counter            = 1;

                strcpy(buf, sub_container_name.c_str());

                while (1) {
                    char *buf2key = GBS_string_2_key(buf);
                    strcpy(buf, buf2key);
                    free(buf2key);

                    if (GB_find(gb_gene, buf, 0, down_level) == 0) break;
                    ++counter;
                    sprintf(buf, "%s_%02i", sub_container_name.c_str(), counter);
                }

                gb_sub_container = GB_create_container(gb_gene, buf);
            }

            gene_name_subst["last_gene"] = gene_name;

            for (EntryMap::const_iterator e=featureEntries.begin();e != featureEntries.end();++e)
            {
                const string& entry_name = e->first;

                if (entry_name!="gene" && entry_name!="translation") {
                    GBDATA     *gb_field      = GB_find(gb_sub_container, entry_name.c_str(), NULL, down_level);
                    const char *entry_content = e->second.c_str();
                    if (gb_field) {
                        char *old_content = strdup(GB_read_char_pntr(gb_field));

                        if (ARB_stricmp(old_content, entry_content)!=0) {
                            if (old_content[0]==0) {
                                GB_write_string(gb_field, entry_content);
                            }
                            else {
                                GB_write_string(gb_field, GBS_global_string("%s; %s", old_content, entry_content));
                            }
                        }
                        free(old_content);
                    }
                    else {
                        GBDATA *gb_field = GB_search(gb_sub_container, entry_name.c_str(), GB_STRING);
                        GB_write_string(gb_field, entry_content);
                    }
                }
            }

            // obligatory gene-entries:

            int     pos_counter = 0;
            GBDATA *gb_field    = 0;

            for (list<GenePosition>::const_iterator gp = gene_positions.begin();
                 gp != gene_positions.end();
                 ++gp, ++pos_counter)
            {
                bool uncertain = false;

                for (int end = 0; end <= 1; ++end) {
                    const char *field_name           = end ? "pos_end" : "pos_begin";
                    const char *real_field_name      = field_name;
                    if (pos_counter) real_field_name = GBS_global_string("%s%i", field_name, pos_counter+1);

                    gb_field                 = GB_search(gb_sub_container, real_field_name, GB_INT);
                    const GeneBorder& border = end ? gp->getUpper() : gp->getLower();
                    GB_write_int(gb_field, border.getPos());

                    if (end && data_length>0) {
                        aw_status(double(border.getPos())/double(data_length));
                    }

                    if (border.getSureness()) uncertain = true;
                }

                if (uncertain) {
                    char sureness[] = "==";
                    char s;

                    s = gp->getLower().getSureness(); if (s) sureness[0] = s;
                    s = gp->getUpper().getSureness(); if (s) sureness[1] = s;

                    const char *field_name           = "pos_uncertain";
                    const char *real_field_name      = field_name;
                    if (pos_counter) real_field_name = GBS_global_string("%s%i", field_name, pos_counter+1);

                    gb_field = GB_search(gb_sub_container, real_field_name, GB_STRING);
                    GB_write_string(gb_field, sureness);
                }
            }

            if (pos_counter>1) {
                gb_field = GB_search(gb_sub_container, "pos_joined", GB_INT);
                GB_write_int(gb_field, pos_counter);
            }

            gb_field = GB_search(gb_sub_container, "complement", GB_BYTE);
            GB_write_byte(gb_field, int(complement));

            used = true;
        }
    }
    return error;
}


#if 0
// Get next <section,content> pair out of a Features string (NOT USED!)
const char* parse_features_embl(const char *str, string& subsubsection, string& subsubcontent)
{
    int s1,s2,c1,c2;

    while(*str==' ') str++;
    if (*str==0) return 0;

    if (*str=='/') {
        for(s1=1;str[s1]==' ';s1++);
        for(s2=s1;str[s2]!='=' && str[s2]!=0;s2++);
    }
    else {
        s1 = s2 = 0;
    }
    c1=s2;
    if (str[c1]=='=') c1++;
    for(c1=1;str[c1]==' ';c1++);
    if (str[c1]=='"') {
        c1++;
        for(c2=c1;str[c2]!='"' && str[c2]!=0;c2++);
    }
    else {
        //	for(c2=c1;str[c2]!=' ' && str[c2]!="/" && str[c2]!=0;c2++);
    }

    subsubsection = string(str+s1,str+s2);
    subsubcontent = string(str+c1,str+c2);
    return str+c2;
}
#endif

// Parse a range string (e.g. "23..49"), returning true if it was a range (NOT USED!)
bool get_range_embl(const char *str, int& from, int& to)
{
    from=0;
    to=0;

    while(*str>='0' && *str<='9') from = 10*from + (*str++)-'0';
    while(*str=='.' || *str==' ') str++;
    while(*str>='0' && *str<='9') to = 10*to + (*str++)-'0';

    return (from>0 && to>0);
}


//  ---------------------------------------------------------------------------------------
//      GB_ERROR GEN_read_embl(GBDATA *gb_main, const char *filename, const char *ali_name)
//  ---------------------------------------------------------------------------------------
GB_ERROR GEN_read_embl(GBDATA *gb_main, const char *filename, const char *ali_name){
    GB_ERROR       error            = 0;
    FileBuffer     file(filename);
    string         line;
    string         section_name;
    string         content, new_content;
    GBDATA        *gb_species_data  = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    char          *new_species_name = AWTC_makeUniqueShortName("genom", gb_species_data);
    GBDATA        *gb_species       = GBT_create_species(gb_main, new_species_name);
    int            reference        = 0;
    string         reference_string;

    bool           data_read        = false;
    unsigned long  data_length      = 0;

    int         section_type, subsection_type = -1, new_section_type, new_subsection_type;
    int         line_number                   = 0;
    const char* ccontent;

    typedef map<string, int> NonGene;
    NonGene                  nonGene;

    GeneNameSubstitute          gene_name_subst;

    if (!file.good()) error = GBS_global_string("Can't open '%s'", filename);

    cout << "Reading '" << filename << "'\n"; // please keep this line (makes it more easy for biologists to find out which file crashes)

    // Read Embl format until sequence data
    content      = "";
    section_type = -2;
    while(file.getLine(line)) {
        line_number++;
        new_section_type = parseLine_embl(line, new_subsection_type, new_content);

        // Special handling of FT and FH: Continuating lines have subsection type SS_MAX
        if ((new_section_type == SC_FT) || (new_section_type == SC_FH)) {
            if (new_subsection_type == SS_MAX) {
                content = content + " " + new_content;
                continue;
            }
        }
        else {
            // For other sections, all sections with same type are concatenated
            if (new_section_type == section_type) {
                if (section_type == SC_SQ2)
                    break;
                else
                    content = content + " " + new_content;
                continue;
            }
        }
        if (section_type>=0)
#if defined(DEBUG)
            cout << "Parsing line " << line_number  <<", Section " << SECTION_INDICATOR[section_type] << ": ";
#endif // DEBUG
        ccontent = content.c_str();
        switch(section_type) {
            case SC_AC:
                GB_write_string( GB_search(gb_species, "acc", GB_STRING), ccontent);
#if defined(DEBUG)
                cout << "OK\n";
#endif // DEBUG
                break;
            case SC_SV:
                GB_write_string( GB_search(gb_species, "version", GB_STRING), ccontent);
#if defined(DEBUG)
                cout << "OK\n";
#endif // DEBUG
                break;
            case SC_DT:
                GB_write_string( GB_search(gb_species, "date", GB_STRING), ccontent);
#if defined(DEBUG)
                cout << "OK\n";
#endif // DEBUG
                break;
            case SC_DE:
                GB_write_string( GB_search(gb_species, "full_name", GB_STRING), ccontent);
#if defined(DEBUG)
                cout << "OK\n";
#endif // DEBUG
                break;
                //case SC_OS:
                //GB_write_string( GB_search(gb_species, "organism", GB_STRING), ccontent);
                //cout << "OK\n";
                //break;
            case SC_OC:
                GB_write_string( GB_search(gb_species, "tax", GB_STRING), ccontent);
#if defined(DEBUG)
                cout << "OK\n";
#endif // DEBUG
                break;
            case SC_RN:
                reference++;
                reference_string = content;
#if defined(DEBUG)
                cout << "Ref " << reference_string << ".\n";
#endif // DEBUG
                break;
            case SC_RA:
                if (reference==0) reference_string = "[1]";
                content = reference_string + " " + content;
                {
                    GBDATA *gbd = GB_find(gb_species, "author", 0, down_level);
                    if (!gbd) {
                        GB_write_string(GB_create(gb_species, "author", GB_STRING), content.c_str() );
                    }
                    else {
                        string old_content = GB_read_char_pntr(gbd);
                        string new_content = old_content + " " + content;
                        GB_write_string(gbd, new_content.c_str());
                    }
                }
#if defined(DEBUG)
                cout << "Author added\n";
#endif // DEBUG
                break;
            case SC_RT:
                if (reference==0) reference_string = "[1]";
                content = reference_string + " " + content;
                {
                    GBDATA *gbd = GB_find(gb_species, "title", 0, down_level);
                    if (!gbd) {
                        GB_write_string(GB_create(gb_species, "title", GB_STRING), ccontent );
                    }
                    else {
                        string old_content = GB_read_char_pntr(gbd);
                        string new_content = old_content + " " + content;
                        GB_write_string(gbd, new_content.c_str());
                    }
                }
#if defined(DEBUG)
                cout << "Title added\n";
#endif // DEBUG
                break;
            case SC_RL:
                if (reference==0) reference_string = "[1]";
                content = reference_string + " " + content;
                {
                    GBDATA *gbd = GB_find(gb_species, "journal", 0, down_level);
                    if (!gbd) {
                        GB_write_string(GB_create(gb_species, "journal", GB_STRING), content.c_str() );
                    }
                    else {
                        string old_content = GB_read_char_pntr(gbd);
                        string new_content = old_content + " " + content;
                        GB_write_string(gbd, new_content.c_str());
                    }
                }
#if defined(DEBUG)
                cout << "Journal added\n";
#endif // DEBUG
                break;

            case SC_SQ:
                {
                    // make s a pointer to sequence length number
                    const char* s = ccontent;
                    for(;*s!=0 && !isdigit(*s);s++);
                    data_length = strtoul(s, 0, 10);
                    GB_write_int( GB_search(gb_species, "sequence length", GB_INT), data_length);
                }
#if defined(DEBUG)
                cout << "Length " << data_length << "\n";
#endif // DEBUG
                break;

            case SC_FT:
                gen_assert(subsection_type != -1);
                if (subsection_type>=0) {
                    error = parseFeature_embl( string(SUBSECTION_INDICATOR[subsection_type]), content, file,
                                               gene_name_subst, gb_species, data_length);
#if defined(DEBUG)
                    cout << "Feature " << SUBSECTION_INDICATOR[subsection_type] << "\n";
#endif // DEBUG
                }
                else {
#if defined(DEBUG)
                    cout << "Subsection " << subsection_type << " ??\n";
#endif // DEBUG
                }
                break;

            case -1:
                cout << "ERROR - Unknown Section skipped\n";
                break;
            default:
                cout << "Skipped\n";
        }

        if (error) return error;

        section_type = new_section_type;
        subsection_type = new_subsection_type;
        content = new_content;
    }

    // rest of file: sequence data

    if (data_length==0) {
        error = "could not detect sequence length -- check input format";
        return error;
    }

    gen_assert(data_length!=0);
    char *data = new char[data_length+1];
    char *dp = data;

    dp = get_sequenceline(dp, content.c_str());
    dp = get_sequenceline(dp, new_content.c_str());

    while ( file.getLine(line) ) {
        line_number++;
        if (line=="//") {
            data_read = true;
            break;
        }
        dp = get_sequenceline(dp, line.c_str());

	// the following gb_assert causes segfaults -> commented out! Sequence
	// length errors will be captured (two if's) later!
        //gb_assert( ((unsigned)(dp-data)) <= data_length );
    }

    if (!data_read) {
        error  = "Sequence data incomplete. ('\\' missing?)\nPlease check genom file!\n";
        return error;
    }

    unsigned long seq_length = (unsigned long)(dp-data); // real length of the sequence

    if (seq_length != data_length)
    {
        error = file.lineError(GBS_global_string("Data length mismatch (real length of sequence = %lu ; specified length = %lu)!\n",
                                                 seq_length, data_length));
        return error;
    }

    cout << " Parsed Sequence (till line " << line_number <<"), length " << seq_length << "\n";

    dp[0]           = 0;
    GBDATA *gb_data = GBT_add_data(gb_species, ali_name, "data", GB_STRING);
    GB_write_string(gb_data, data);

    delete data;

    return 0;

}
