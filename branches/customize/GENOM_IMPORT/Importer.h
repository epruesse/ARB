// ================================================================ //
//                                                                  //
//   File      : Importer.h                                         //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#ifndef IMPORTER_H
#define IMPORTER_H

#ifndef BUFFEREDFILEREADER_H
#include <BufferedFileReader.h>
#endif
#ifndef SMARTPTR_H
#include <smartptr.h>
#endif
#ifndef METATAG_H
#include "MetaTag.h"
#endif

class DBwriter;
class Feature;

enum FeatureLineType {
    FL_START                  = 1,  // start of feature (e.g. 'CDS             352120..353193'). starts at offset 5
    // all types below start at offset 21 (or higher):
    FL_QUALIFIER              = 2,  // start of qualifier (e.g. '/codon_start=1')
    FL_QUALIFIER_NODATA       = 4,  // start of qualifier w/o data (e.g. '/pseudo')
    FL_QUALIFIER_QUOTED       = 8,  // start of qualifier with quoted data (e.g. '/product="phosphate"')
    FL_QUALIFIER_QUOTE_OPENED = 16, // start of qualifier with quoted data (e.g. '/product="phosphate')
    FL_CONTINUED_QUOTE_CLOSED = 32, // something terminated by a quote ('"')
    FL_CONTINUED              = 64, // other

    // meta types:
    FL_META_QUALIFIER = (FL_QUALIFIER|FL_QUALIFIER_NODATA|FL_QUALIFIER_QUOTED|FL_QUALIFIER_QUOTE_OPENED),
    FL_META_CONTINUED = (FL_CONTINUED_QUOTE_CLOSED|FL_CONTINUED),
};

class FeatureLine {
    void interpret_as_continued_line();
public:
    string          name;       // feature or qualifier name (only valid for FL_START, FL_QUALIFIER...)
    string          rest;       // rest of line (behind '=' for FL_QUALIFIER..., not for FL_QUALIFIER_NODATA)
    string          orgLine;
    FeatureLineType type;

    FeatureLine(const string& line);
    bool reinterpret_as_continued_line();
};

typedef SmartPtr<Feature>           FeaturePtr;
typedef SmartPtr<FeatureLine>       FeatureLinePtr;
typedef std::vector<FeatureLinePtr> FeatureLines;

class Importer : virtual Noncopyable {
protected:
    DBwriter&         db_writer;
    LineReader&       flatfile;
    MetaTagTranslator tagTranslator;
    FeatureLines      pushedFeatureLines; // pushed back feature lines
    stringVector      warnings;
    long              expectedSeqLength; // length read from LOCUS or ID line ( = 0 -> no length info found)

    void expectLine(string& line) { if (!flatfile.getLine(line)) throw flatfile.lineError("Unexpected EOF"); }
    const MetaTag *findTag(const string& tag) { return tagTranslator.get(tag); }

    virtual bool readFeatureTableLine(string& line) = 0;

    FeatureLinePtr getFeatureTableLine();
    void           backFeatureTableLine(FeatureLinePtr& fline) { pushedFeatureLines.push_back(fline); }

    FeatureLinePtr getUnwrappedFeatureTableLine();

    FeaturePtr parseFeature();
    void       parseFeatureTable();

    virtual void import_section() = 0;

    void show_warnings(const string& import_of_what);

public:
    Importer(LineReader& Flatfile, DBwriter& DB_writer, const MetaTag *meta_description);
    virtual ~Importer() {}

    void import();
    void warning(const char *msg); // add a warning
};


class GenebankImporter : public Importer {
    void         import_section() OVERRIDE;
    virtual bool readFeatureTableLine(string& line) OVERRIDE;
    void         parseSequence(const string& tag, const string& headerline);

public:
    GenebankImporter(LineReader& Flatfile, DBwriter& DB_writer);
    virtual ~GenebankImporter() OVERRIDE {}

};


class EmblImporter : public Importer {
    void         import_section() OVERRIDE;
    virtual bool readFeatureTableLine(string& line) OVERRIDE;
    void         parseSequence(const string& headerline);

public:
    EmblImporter(LineReader& Flatfile, DBwriter& DB_writer);
    virtual ~EmblImporter() OVERRIDE {}
};


#else
#error Importer.h included twice
#endif // IMPORTER_H

