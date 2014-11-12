#include <stdio.h> // needed for arbdb.h
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <AP_filter.hxx>
#include <seqio.hxx>

#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <iostream>
#include <sstream>
#include <set>
#include <iterator>
#include <algorithm>
#include <insdel.h>

using std::cerr;
using std::endl;
using std::clog;
using std::cout;
using std::string;
using std::map;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::stringstream;
using std::getline;
using std::set;

template<typename T>
struct equals {
    const T& t;
    equals(const T& _t) : t(_t) {}
    bool operator()(const T& _t) { return _t == t; }
};

/* fake aw_message/aw_status functions */

void aw_status(double d) {
    cerr << "aw_status_fraction: " << d << endl;
    cerr.flush();
}
void aw_message(char const* msg) {
    cerr << "aw_message: " << msg << endl;
}
void aw_status(char const* /*msg*/) {
//    cerr << "aw_status_msg: " << msg << endl;
}

/*  generic writer class */
class Writer {
public:
    virtual void addSequence(GBDATA*)=0;
    virtual void finish() {};
    virtual ~Writer() {}
};


class MultiFastaWriter : public Writer {
    ofstream file;
    char *default_ali;
    double count, count_max;
public:
    MultiFastaWriter(string s,  char* a, int c)
        : file(s.c_str()),
          default_ali(a),
          count(0), count_max(c)
    {
    }

    ~MultiFastaWriter() {
    }

    string readString(GBDATA* spec, const char* key) {
        GBDATA *gbd = GB_find(spec, key, SEARCH_CHILD);
        if (!gbd) return string("<empty>");
        char *str= GB_read_as_string(gbd);
        if (!str) return string("<empty>");
        string rval = str;
        free(str);
        return rval;
    }

    string readData(GBDATA* spec) {
        GBDATA *gbd    = GB_find(spec, default_ali, SEARCH_CHILD);
        if (!gbd) return string("<empty>");
        string  result = readString(gbd, "data");
        GB_release(gbd);
        return result;
    }

    void addSequence(GBDATA* spec) {
        file << ">"
             << readString(spec, "acc") << "."
             << readString(spec, "start") << "."
             << readString(spec, "stop") << " | "
             << readString(spec, "full_name") << "\n";

        string tmp = readData(spec);
        std::replace_if(tmp.begin(),tmp.end(),equals<char>('.'),'-');

        unsigned int i;
        for (i=0; i+70<tmp.size(); i+=70)
            file << tmp.substr(i,70) << "\n";
        file << tmp.substr(i) << "\n";
        aw_status(++count/count_max);
    }
};

class ArbWriter : public Writer {
    string template_arb;
    string filename;
    double count, count_max;
    GBDATA *gbdst, *gbdst_spec;

public:
    ArbWriter(string ta, string f, int c)
        : template_arb(ta), filename(f), count(0), count_max(c)
    {
        gbdst = GB_open(template_arb.c_str(), "rw");
        if (!gbdst) {
            cerr << "ERROR: Unable to open template file" << endl;
            exit(1);
        }

        if (GB_request_undo_type(gbdst, GB_UNDO_NONE)) {
            cerr << "WARN: Unable to disable undo buffer";
        }

        GB_begin_transaction(gbdst);
        gbdst_spec = GB_search(gbdst, "species_data", GB_CREATE_CONTAINER);
        GB_commit_transaction(gbdst);

        if (GB_save_as(gbdst, filename.c_str(), "b")) {
            cerr << "ERROR: Unable to open output file" << endl;
            exit(1);
        }
    }

    void addSequence(GBDATA* gbspec) {
        GB_begin_transaction(gbdst);
        GBDATA *gbnew = GB_create_container(gbdst_spec, "species");
        GB_copy(gbnew, gbspec);
        GB_release(gbnew); 
        GB_commit_transaction(gbdst);
        aw_status(++count/count_max);
    }

    void finish() {
        cerr << "Compressing..." << endl;

        {
            GB_transaction trans(gbdst);
            GB_ERROR       error = ARB_format_alignment(gbdst, GBT_get_default_alignment(gbdst));

            if (error) {
                cerr << "ERROR: Failed to format alignments (" << error << ')' << endl;
                exit(5);
            }
        }

        if (GB_save_as(gbdst, filename.c_str(), "b")) {
            cerr << "ERROR: Unable to save to output file" << endl;
            exit(5);
        }

        GB_close(gbdst);
    }

    ~ArbWriter() {
    }
};

class AwtiExportWriter : public Writer {
    GBDATA *gbmain;
    const char *dbname;
    const char *formname;
    const char *outname;
    char *real_outname;
    const int compress;
public:
    AwtiExportWriter(GBDATA *_gbmain, const char *_dbname, const char* eft,
                     const char* out, int c)
        : gbmain(_gbmain), dbname(_dbname), formname(eft), outname(out), compress(c)
    {
        GBT_mark_all(gbmain, 0);
    }
    void addSequence(GBDATA *gbspec) {
        GB_write_flag(gbspec, 1);
    }

    void finish() {
        const int marked_only = 1;
        const int cut_stop_codon = 0;
        const int multiple = 0;

        char *aliname = GBT_get_default_alignment(gbmain);
        AP_filter filter(GBT_get_alignment_len(gbmain, aliname));
        GB_ERROR err = 0;
        err = SEQIO_export_by_format(gbmain, marked_only, &filter,
                                 cut_stop_codon, compress, dbname,
                                 formname, outname, multiple, &real_outname);
        if (err) {
            cerr << err << endl;
        }
    }

    ~AwtiExportWriter() {
    }
};



void help() {
    cerr <<
        "Exports sequences from an ARB database."
        "\n"
        "Parameters:\n"
        " --source <FILE>        source file\n"
        " --dest <FILE>          destination file\n"
        " --format <ARG>         'ARB', 'FASTA' or path to ARB EFT file\n"
        " --accs <FILE>          export only sequences matching these accessions\n"
        "\n"
        "only for EFT output format:\n"
        " --eft-compress-gaps <ARG>  'none', 'vertical' or 'all' (default none)\n"
        "\n"
        "required for ARB output format:\n"
        " --arb-template <FILE>      template (empty) arb file\n"

         << endl;;
}

set<string> read_accession_file(const char* file) {
    set<string> accessions;
    ifstream ifs(file);
    string linestring;

    do {
        getline(ifs,linestring);
        if (!linestring.empty()) {
            stringstream line(linestring);
            string item;
            do {
                getline(line,item,',');
                item.erase(item.find_last_not_of(" \n,\t")+1);
                item.erase(0,item.find_first_not_of(" \n,\t"));
                if (!item.empty())
                    accessions.insert(item);
            } while(line.good());
        }
    } while(ifs.good());

    return accessions;
}

int main(int argc, char** argv) {
    enum {
        FMT_ARB,
        FMT_FASTA,
        FMT_EFT,
        FMT_NONE
    } format = FMT_NONE;
    char* src = 0;
    char* dest = 0;
    char* tmpl = 0;
    char* accs = 0;
    char* eft = 0;
    int compress = 0;
    GB_shell shell;

    if (argc <2) {
        help();
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (argc > i+1) {
            if (!strcmp(argv[i], "--source")) {
                src = argv[++i];
            } else if (!strcmp(argv[i], "--dest")) {
                dest = argv[++i];
            } else if (!strcmp(argv[i], "--arb-template")) {
                tmpl = argv[++i];
            } else if (!strcmp(argv[i], "--accs")) {
                accs = argv[++i];
            } else if (!strcmp(argv[i], "--format")) {
                if (!strcmp(argv[++i], "ARB")) {
                    format = FMT_ARB;
                } else if (!strcmp(argv[i], "FASTA")) {
                    format = FMT_FASTA;
                } else {
                    format = FMT_EFT;
                    eft = argv[i];
                }
            } else if (!strcmp(argv[i], "--eft-compress-gaps")) {
                if (!strcmp(argv[++i], "none")) {
                    compress = 0;
                } else if (!strcmp(argv[i], "vertical")) {
                    compress = 1;
                } else if (!strcmp(argv[i], "all")) {
                    compress = 2;
                } else {
                    cerr << "missing argument to --compress-gaps?";
                    i--;
                }
            } else {
                cerr << "ignoring malformed argument: '" << argv[i] << "'" << endl;
            }
        } else {
            cerr << "ignoring malformed argument: '" << argv[i] << "'" << endl;
        }
    }

    if (!src) {
        cerr << "need '--source' parameter" << endl;
        return 1;
    }
    if (!dest) {
        cerr << "need '--dest' parameter" << endl;
        return 1;
    }
    if (format == FMT_NONE) {
        cerr << "need '--format' parameter" << endl;
        return 1;
    }
    if (format == FMT_ARB && !tmpl) {
        cerr << "need '--tmpl' parameter for output type ARB'" << endl;
        return 1;
    }

    set<string> accessions = read_accession_file(accs);

    GBDATA *gbsrc = GB_open(src, "r");
    if (!gbsrc) {
        cerr << "unable to open source file" << endl;
        return 1;
    }

    GB_transaction trans(gbsrc);

    int count_max = 0;
    if (accs) {
        count_max = accessions.size();
    } else {
        for (GBDATA *gbspec = GBT_first_species(gbsrc);
             gbspec; gbspec = GBT_next_species(gbspec)) {
            count_max++;
        }
    }

    char* default_ali = GBT_get_default_alignment(gbsrc);

    // create writer for target type
    Writer *writer=0;
    switch(format) {
    case FMT_ARB:
        writer = new ArbWriter(tmpl, dest, count_max);
        break;
    case FMT_FASTA:
        writer = new MultiFastaWriter(dest, default_ali, count_max);
        break;
    case FMT_EFT:
        writer = new AwtiExportWriter(gbsrc, src, eft, dest, compress);
        break;
    case FMT_NONE:
        cerr << "internal error" << endl;
        return 2;
        break;
    }

    // generate file
    for (GBDATA *gbspec = GBT_first_species(gbsrc);
         gbspec; gbspec = GBT_next_species(gbspec)) {

        GBDATA *gbname = GB_find(gbspec, "acc", SEARCH_CHILD);
        string name(GB_read_string(gbname));

        if (!accs || accessions.count(name))  {
            writer->addSequence(gbspec);
        }
        GB_release(gbspec);
    }

    writer->finish();
    delete writer;
    trans.close(NULL);
    GB_close(gbsrc);
}


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
