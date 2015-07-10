#include <stdio.h> // needed for arbdb.h
#include <string.h>
#include <unistd.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <arb_file.h>
#include <AP_filter.hxx>
#include <seqio.hxx>
#include <insdel.h>

#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <iostream>
#include <sstream>
#include <set>
#include <iterator>
#include <algorithm>

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

static void aw_status(double d) {
    cerr << "aw_status_fraction: " << d << endl;
    cerr.flush();
}

/*  generic writer class */
class Writer : virtual Noncopyable {
    string err_state;
protected:
    virtual void set_error(const char *error) {
        arb_assert(error);
        if (!err_state.empty()) {
            cerr << "ERROR: " << err_state << endl;
        }
        err_state = error;
    }

public:
    virtual ~Writer() {}

    virtual void addSequence(GBDATA*) = 0;
    virtual void finish() {}

    const char *get_error() const {
        return err_state.empty() ? NULL : err_state.c_str();
    }
    bool ok() const { return !get_error(); }
};

class MultiFastaWriter : public Writer { // derived from Noncopyable
    ofstream file;
    string default_ali;
    double count, count_max;
public:
    MultiFastaWriter(string s, const char* a, int c)
        : file(s.c_str()),
          default_ali(a),
          count(0), count_max(c)
    {
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
        GBDATA *gbd    = GB_find(spec, default_ali.c_str(), SEARCH_CHILD);
        if (!gbd) return string("<empty>");
        string  result = readString(gbd, "data");
        GB_release(gbd);
        return result;
    }

    void addSequence(GBDATA* spec) OVERRIDE {
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

class ArbWriter : public Writer { // derived from Noncopyable
    string  template_arb;
    string  filename;
    double  count, count_max;
    GBDATA *gbdst, *gbdst_spec;

    void close() {
        if (gbdst) {
            GB_close(gbdst);
            gbdst = NULL;
        }
    }

    void set_error(const char *error) OVERRIDE {
        Writer::set_error(error);
        close();
    }

public:
    ArbWriter(string ta, string f, int c)
        : template_arb(ta),
          filename(f),
          count(0),
          count_max(c)
    {
        gbdst = GB_open(template_arb.c_str(), "rw");
        if (!gbdst) set_error("Unable to open template file");
        else {
            if (GB_request_undo_type(gbdst, GB_UNDO_NONE)) {
                cerr << "WARN: Unable to disable undo buffer";
            }

            GB_begin_transaction(gbdst);
            gbdst_spec = GB_search(gbdst, "species_data", GB_CREATE_CONTAINER);
            GB_commit_transaction(gbdst);

            GB_ERROR error = GB_save_as(gbdst, filename.c_str(), "b");
            if (error) set_error(GBS_global_string("Unable to open output file (Reason: %s)", error));
        }
    }
    ~ArbWriter() {
        close();
        if (!ok()) {
            unlink(filename.c_str());
        }
    }

    void addSequence(GBDATA* gbspec) OVERRIDE {
        if (gbdst) {
            GB_transaction  ta(gbdst);
            GBDATA         *gbnew = GB_create_container(gbdst_spec, "species");
            GB_ERROR        error = NULL;
            if (!gbnew) {
                error = GB_await_error();
            }
            else {
                error             = GB_copy(gbnew, gbspec);
                if (!error) error = GB_release(gbnew);
            }
            if (error) set_error(error);
        }
        aw_status(++count/count_max);
    }

    void finish() OVERRIDE {
        GB_ERROR error = NULL;
        if (gbdst) {
            cerr << "Compressing..." << endl;
            GB_transaction  trans(gbdst);
            char           *ali = GBT_get_default_alignment(gbdst);
            if (!ali) {
                error = "Your template DB has no default alignment";
            }
            else {
                error            = ARB_format_alignment(gbdst, ali);
                if (error) error = GBS_global_string("Failed to format alignments (Reason: %s)", error);
                free(ali);
            }
        }

        if (!error && GB_save_as(gbdst, filename.c_str(), "b")) {
            error = "Unable to save to output file";
        }

        if (error) set_error(error);
    }
};

class AwtiExportWriter : public Writer { // derived from Noncopyable
    GBDATA *gbmain;
    const char *dbname;
    const char *formname;
    const char *outname;
    const int compress;

public:
    AwtiExportWriter(GBDATA *_gbmain, const char *_dbname, const char* eft,
                     const char* out, int c)
        : gbmain(_gbmain), dbname(_dbname), formname(eft), outname(out), compress(c)
    {
        GBT_mark_all(gbmain, 0);
    }
    void addSequence(GBDATA *gbspec) OVERRIDE {
        GB_write_flag(gbspec, 1);
    }

    void finish() OVERRIDE {
        const int marked_only = 1;
        const int cut_stop_codon = 0;
        const int multiple = 0;

        char      *aliname      = GBT_get_default_alignment(gbmain);
        AP_filter  filter(GBT_get_alignment_len(gbmain, aliname));
        char      *real_outname = 0;

        GB_ERROR err = SEQIO_export_by_format(gbmain, marked_only, &filter,
                                              cut_stop_codon, compress, dbname,
                                              formname, outname, multiple, &real_outname);
        if (err) set_error(err);

        free(real_outname);
        free(aliname);
    }
};



static void help() {
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

static set<string> read_accession_file(const char* file) {
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
                    cerr << "missing argument to --eft-compress-gaps?";
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
        cerr << "need '--arb-template' parameter for output type ARB'" << endl;
        return 1;
    }

    set<string> accessions;
    if (accs) {
        if (!GB_is_regularfile(accs)) {
            cerr << "unable to open accession number file '" << accs << '\'' << endl;
            return 1;
        }
        accessions = read_accession_file(accs);
    }

    GB_ERROR error    = NULL;
    int      exitcode = 0;

    GBDATA *gbsrc = GB_open(src, "r");
    if (!gbsrc) {
        error    = "unable to open source file";
        exitcode = 1;
    }

    if (!error) {
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

        // create writer for target type
        Writer *writer = 0;
        switch(format) {
        case FMT_ARB:
            writer = new ArbWriter(tmpl, dest, count_max);
            break;
        case FMT_FASTA: {
            char *default_ali = GBT_get_default_alignment(gbsrc);
            writer            = new MultiFastaWriter(dest, default_ali, count_max);
            free(default_ali);
            break;
        }
        case FMT_EFT:
            writer = new AwtiExportWriter(gbsrc, src, eft, dest, compress);
            break;

        case FMT_NONE:
            error    = "internal error";
            exitcode = 2;
            break;
        }

        if (writer) {
            // generate file
            for (GBDATA *gbspec = GBT_first_species(gbsrc);
                 gbspec && writer->ok();
                 gbspec = GBT_next_species(gbspec))
            {
                GBDATA *gbname = GB_find(gbspec, "acc", SEARCH_CHILD);
                string name(GB_read_char_pntr(gbname));

                if (!accs || accessions.count(name))  {
                    writer->addSequence(gbspec);
                }
                GB_release(gbspec);
            }

            if (writer->ok()) writer->finish();
            
            if (!writer->ok()) {
                error    = GBS_static_string(writer->get_error());
                exitcode = 5;
            }
            delete writer;
        }
    }
    if (gbsrc) GB_close(gbsrc);

    if (error) cerr << "ERROR: " << error << endl;
    return exitcode;
}

