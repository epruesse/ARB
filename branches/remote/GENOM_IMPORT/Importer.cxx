// ================================================================ //
//                                                                  //
//   File      : Importer.cxx                                       //
//   Purpose   : Genome importer core                               //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "tools.h"
#include "DBwriter.h"
#include <arbdb.h>
#include <arb_stdstr.h>

using namespace std;

// --------------------------------------------------------------------------------

static bool is_escaped(const string& str, size_t pos) {
    // returns true, if position 'pos' in string 'str' is escaped by '\\'

    bool escaped = false;
    if (pos != 0) { // pos 0 can't be escaped
        if (str[pos-1] == '\\') {                   // is an escape before pos ?
            escaped = !is_escaped(str, pos-1);   // pos is escaped, if the escape isn't!
        }
    }
    return escaped;
}

FeatureLine::FeatureLine(const string& line) {
    // start parsing at position 5
    string::size_type first_char = line.find_first_not_of(' ', 5);

    orgLine = line;

    if (first_char == 5) { // feature start
        string::size_type behind_name = line.find_first_of(' ', first_char);
        string::size_type rest_start = line.find_first_not_of(' ', behind_name);

        if (rest_start == string::npos) {
            if (behind_name == string::npos) throw "Expected space behind feature name";
            throw "Expected some content behind feature name";
        }

        name = line.substr(first_char, behind_name-first_char);
        rest = line.substr(rest_start);
        type = FL_START;
    }
    else if (first_char >= 21) { // not feature start
        if (first_char == 21 && line[first_char] == '/') { // qualifier start
            string::size_type equal_pos = line.find_first_of('=', first_char);
            if (equal_pos == string::npos) {
                // qualifier w/o data (i.e. "/pseudo")
                name = line.substr(first_char+1);
                rest = "true";
                type = FL_QUALIFIER_NODATA;
            }
            else {
                name = line.substr(first_char+1, equal_pos-first_char-1);
                rest = line.substr(equal_pos+1);

                if (rest[0] == '"') {
                    size_t rlen = rest.length();

                    if (rlen == 1) {                // special case: only one open quote behind qualifier
                        type = FL_QUALIFIER_QUOTE_OPENED;
                    }
                    else if (rest[rlen-1] == '"' && !is_escaped(rest, rlen-1)) { // closing non-escaped quote at eol
                        type = FL_QUALIFIER_QUOTED;
                    }
                    else {
                        type = FL_QUALIFIER_QUOTE_OPENED;
                    }
                }
                else {
                    type = FL_QUALIFIER;
                }
            }
        }
        else {                             // continued line
            interpret_as_continued_line();
        }
    }
    else {
        if (first_char == string::npos) {
            throw "Expected feature line, found empty line";
        }
        throw GBS_global_string("Expected feature line (first char at pos=%zu unexpected)", first_char);
    }
}

void FeatureLine::interpret_as_continued_line() {
    rest = orgLine.substr(21);
    if (rest[rest.length()-1] == '"') {
        type = FL_CONTINUED_QUOTE_CLOSED;
    }
    else {
        type = FL_CONTINUED;
    }
}

bool FeatureLine::reinterpret_as_continued_line() {
    bool ok = false;

    if (type == FL_QUALIFIER || type == FL_QUALIFIER_NODATA) {
        string::size_type first_char = orgLine.find_first_not_of(' ', 5);
        if (first_char >= 21) {
            interpret_as_continued_line();
            ok = true;
        }
    }

    return ok;
}

// --------------------------------------------------------------------------------

Importer::Importer(LineReader& Flatfile, DBwriter& DB_writer, const MetaTag *meta_description)
    : db_writer(DB_writer), 
      flatfile(Flatfile), 
      tagTranslator(meta_description), 
      expectedSeqLength(-1)
{}

void Importer::warning(const char *msg) {
    warnings.push_back(msg);
}

FeatureLinePtr Importer::getFeatureTableLine() {
    FeatureLinePtr fline;

    if (pushedFeatureLines.empty()) { // nothing on stack -> read new
        string line;
        if (readFeatureTableLine(line)) fline = new FeatureLine(line);
    }
    else {
        fline = pushedFeatureLines.back();
        pushedFeatureLines.pop_back();
    }
    return fline;
}

FeatureLinePtr Importer::getUnwrappedFeatureTableLine() {
    FeatureLinePtr fline = getFeatureTableLine();
    if (!fline.isNull()) {
        if (fline->type & FL_META_CONTINUED) throw "Expected start of feature or qualifier";

        if (0 == (fline->type & (FL_QUALIFIER_NODATA|FL_QUALIFIER_QUOTED))) {
            // qualifier/featurestart may be wrapped
            FeatureLinePtr next_fline = getFeatureTableLine();

            while (!next_fline.isNull() &&
                   fline->type != FL_QUALIFIER_QUOTED) // already seen closing quote
            {
                if ((next_fline->type&FL_META_CONTINUED) == 0) {
                    // special case: a wrapped line of a quoted qualifier may start with /xxx
                    // (in that case it is misinterpreted as qualifier start)
                    if (fline->type == FL_QUALIFIER_QUOTE_OPENED) {
                        if (!next_fline->reinterpret_as_continued_line()) {
                            throw "did not see end of quoted qualifier (instead found next qualifiert)";
                        }
                        gi_assert(next_fline->type & FL_META_CONTINUED);
                    }
                    else {
                        break;
                    }
                }

                if (next_fline->type == FL_CONTINUED_QUOTE_CLOSED) {
                    if (fline->type != FL_QUALIFIER_QUOTE_OPENED) throw "Unexpected closing quote";
                    fline->type = FL_QUALIFIER_QUOTED;
                }
                else {
                    gi_assert(next_fline->type == FL_CONTINUED);
                    gi_assert(fline->type == FL_START || fline->type == FL_QUALIFIER || fline->type == FL_QUALIFIER_QUOTE_OPENED);
                }

                fline->rest.append(next_fline->rest);
                next_fline = getFeatureTableLine();
            }

            if (!next_fline.isNull()) backFeatureTableLine(next_fline);
        }
    }
    return fline;
}

FeaturePtr Importer::parseFeature() {
    FeaturePtr     feature;
    FeatureLinePtr fline = getUnwrappedFeatureTableLine();

    if (!fline.isNull()) {         // found a feature table line
        if (fline->type != FL_START) throw "Expected feature start";

        feature = new Feature(fline->name, fline->rest);

        fline = getUnwrappedFeatureTableLine();
        while (!fline.isNull() && (fline->type & FL_META_QUALIFIER)) {
            feature->addQualifiedEntry(fline->name, fline->rest);
            fline = getUnwrappedFeatureTableLine();
        }
        if (!fline.isNull()) backFeatureTableLine(fline);
    }

    return feature;
}

void Importer::parseFeatureTable() {
    FeaturePtr feature = parseFeature();

    while (!feature.isNull()) {
        feature->expectLocationInSequence(expectedSeqLength);
        feature->fixEmptyQualifiers();
        db_writer.writeFeature(*feature, expectedSeqLength);
        feature = parseFeature();
    }
}

void Importer::show_warnings(const string& import_of_what) {
    if (!warnings.empty()) {
        const char         *what = import_of_what.c_str();
        stringVectorCRIter  e    = warnings.rend();
        for (stringVectorCRIter i = warnings.rbegin(); i != e; ++i) {
            GB_warningf("Warning: %s: %s", what, i->c_str());
        }
        warnings.clear();
    }
}


void Importer::import() {
    try {
        string line;
        while (flatfile.getLine(line)) {
            if (!line.empty()) { // silently skip empty lines before or after section
                flatfile.backLine(line);

                // cleanup from import of previous section
                gi_assert(pushedFeatureLines.empty()); // oops - somehow forgot a feature
                pushedFeatureLines.clear();
                warnings.clear();

                expectedSeqLength = 0; // reset expected seq. length
                import_section();

                gi_assert(warnings.empty());
                gi_assert(pushedFeatureLines.empty()); // oops - somehow forgot a feature
            }
        }
    }
    catch (const DBerror& err) { throw err.getMessage(); }
    catch (const string& err) { throw flatfile.lineError(err); }
    catch (const char *err) { throw flatfile.lineError(err); }
}

// --------------------------------------------------------------------------------
// Meta information definitions
//
//
// [ please keep the list of common entries in
//      ../HELP_SOURCE/oldhelp/sp_info.hlp
//   up to date! ]

static MetaTag genebank_meta_description[] = {
    { "LOCUS",     "org_locus",   MT_HEADER },

    { "REFERENCE", "",            MT_REF_START },
    { "  AUTHORS", "author",      MT_REF },
    { "  TITLE",   "title",       MT_REF },
    { "  CONSRTM", "refgrp",      MT_REF },
    { "  JOURNAL", "journal",     MT_REF },
    { "   PUBMED", "pubmed_id",   MT_REF },
    { "  MEDLINE", "medline_id",  MT_REF },
    { "  REMARK",  "refremark",   MT_REF },

    { "DEFINITION", "definition", MT_BASIC },
    { "ACCESSION",  "acc",        MT_BASIC },
    { "VERSION",    "version",    MT_BASIC },
    { "KEYWORDS",   "keywd",      MT_BASIC },
    { "SOURCE",     "full_name",  MT_BASIC },
    { "  ORGANISM", "tax",        MT_BASIC },
    { "COMMENT",    "comment",    MT_BASIC },
    { "PROJECT",     "projref",   MT_BASIC },

    { "FEATURES", "", MT_FEATURE_START },
    { "CONTIG",   "", MT_CONTIG },
    { "BASE",     "", MT_SEQUENCE_START }, // BASE COUNT (sometimes missing)
    { "ORIGIN",   "", MT_SEQUENCE_START }, // only used if BASE COUNT is missing
    { "//",       "", MT_END },

    { "", "", MT_IGNORE },      // End of array
};

static MetaTag embl_meta_description[] = {
    { "ID", "org_id",          MT_HEADER },

    { "RN", "",                MT_REF_START },
    { "RA", "author",          MT_REF },
    { "RC", "auth_comm",       MT_REF },
    { "RG", "refgrp",          MT_REF },
    { "RL", "journal",         MT_REF },
    { "RP", "nuc_rp",          MT_REF },
    { "RT", "title",           MT_REF },
    { "RX", "",                MT_REF_DBID }, // @@@ extract field 'pubmed_id' ?

    { "AC", "acc",             MT_BASIC },
    { "AH", "assembly_header", MT_BASIC },
    { "AS", "assembly_info",   MT_BASIC },
    { "CC", "comment",         MT_BASIC },
    { "CO", "contig",          MT_BASIC },
    { "DE", "description",     MT_BASIC },
    { "DR", "db_xref",         MT_BASIC },
    { "DT", "date",            MT_BASIC },
    { "SV", "version",         MT_BASIC },
    { "KW", "keywd",           MT_BASIC },
    { "OS", "full_name",       MT_BASIC },
    { "OC", "tax",             MT_BASIC },
    { "OG", "organelle",       MT_BASIC },
    { "PR", "projref",         MT_BASIC },

    { "FH", "", MT_FEATURE_START },
    { "FT", "", MT_FEATURE },
    { "SQ", "", MT_SEQUENCE_START },
    { "//", "", MT_END },

    { "XX", "", MT_IGNORE }, // spacer

    { "", "", MT_IGNORE }, // End of array
};

// --------------------------------------------------------------------------------


GenebankImporter::GenebankImporter(LineReader& Flatfile, DBwriter& DB_writer)
    : Importer(Flatfile, DB_writer, genebank_meta_description)
{}

bool GenebankImporter::readFeatureTableLine(string& line) {
    if (flatfile.getLine(line)) {
        if (beginsWith(line, "     ")) {
            return true;
        }
        flatfile.backLine(line);
    }
    return false;
}

static bool splitGenebankTag(const string& line, string& tag, string& content) {
    // split a line into tag (incl. preceding spaces) and content
    // returns true, if line suffices the format requirements
    // Note: returns tag="" at wrapped lines

    string::size_type first_non_space = line.find_first_not_of(' ');

    if (first_non_space == 12 || // no tag, only content
        (first_non_space == string::npos && line.length() == 12)) { // same with empty content
        tag     = "";
        content = line.substr(12);
        return true;
    }

    if (first_non_space>12) return false;

    string::size_type behind_tag = line.find_first_of(' ', first_non_space);
    if (behind_tag == string::npos) { // only tag w/o spaces behind
        tag     = line;
        content = "";
        return true;
    }

    string::size_type content_start = line.find_first_not_of(' ', behind_tag);
    if (content_start == string::npos) { // line w/o content
        content = "";
    }
    else {
        content = line.substr(content_start);
    }

    tag = line.substr(0, behind_tag);
    return true;
}

static long scanSeqlenFromLOCUS(const string& locusContent) {
    StringParser parser(locusContent);
    parser.extractWord();       // id
    parser.eatSpaces();

    long bp = parser.extractNumber();
    parser.eatSpaces();
    parser.expectContent("bp");

    return bp;
}

void GenebankImporter::import_section() {
    MetaInfo   meta;
    References refs;

    const MetaTag *prevTag = 0; // previously handled tag
    string         prevContent; // previously found content

    bool seenHeaderLine = false;
    bool EOS            = false; // end of section ?

    // read header of file
    while (!EOS) {
        string line, tag, content;
        expectLine(line);
        if (!splitGenebankTag(line, tag, content)) {
            gi_assert(0);
        }

        if (tag.empty()) {      // no tag - happens at wrapped lines
            prevContent.append(1, ' ');
            prevContent.append(content);
        }
        else { // start of new tag
            const MetaTag *knownTag = findTag(tag);
            if (!knownTag) throw GBS_global_string("Invalid tag '%s'", tag.c_str());

            if (prevTag) { // save previous tag
                switch (prevTag->type) {
                    case MT_REF:        refs.add(prevTag->field, prevContent); break;
                    case MT_BASIC:      meta.add(prevTag, prevContent, true); break;
                    case MT_HEADER:
                        meta.add(prevTag, prevContent, true); // save header line
                        expectedSeqLength = scanSeqlenFromLOCUS(prevContent);
                        break;
                    case MT_REF_DBID: // embl only
                    default: gi_assert(0); break;
                }
                prevTag = 0;
            }

            switch (knownTag->type) {
                case MT_HEADER:
                    if (seenHeaderLine) throw GBS_global_string("Multiple occurrences of tag '%s'", tag.c_str());
                    seenHeaderLine = true;
                    // fall-through
                case MT_BASIC:
                case MT_REF:
                    prevTag     = knownTag;
                    prevContent = content;
                    break;

                case MT_REF_START:
                    refs.start(); // start a new reference
                    break;

                case MT_FEATURE_START:
                    db_writer.createOrganism(flatfile.getFilename(), "NCBI");
                    parseFeatureTable();
                    break;

                case MT_SEQUENCE_START:
                    parseSequence(knownTag->tag, content);
                    EOS = true; // end of section
                    break;

                case MT_IGNORE:
                    break;

                case MT_END:
                    EOS = true;
                    break;

                case MT_CONTIG:
                    throw GBS_global_string("Cannot import files containing CONTIG");

                case MT_REF_DBID: // embl only
                default:
                    gi_assert(0);
                    throw GBS_global_string("Tag '%s' not expected here", knownTag->tag.c_str());
            }
        }
    }

    db_writer.finalizeOrganism(meta, refs, *this);
    show_warnings(meta.getAccessionNumber());
}

// --------------------------------------------------------------------------------


EmblImporter::EmblImporter(LineReader& Flatfile, DBwriter& DB_writer)
    : Importer(Flatfile, DB_writer, embl_meta_description)
{}

static bool splitEmblTag(const string& line, string& tag, string& content) {
    // split a line into 2-character tag and content
    // return true on success (i.e. if line suffices the required format)

    if (line.length() == 2) {
        tag   = line;
        content = "";
    }
    else {
        string::size_type spacer = line.find("   "); // separator between tag and content
        if (spacer != 2) return false; // expect spacer at pos 2-4

        tag     = line.substr(0, 2);
        content = line.substr(5);
    }

    return true;
}

bool EmblImporter::readFeatureTableLine(string& line) {
    if (flatfile.getLine(line)) {
        if (beginsWith(line, "FT   ")) {
            return true;
        }
        flatfile.backLine(line);
    }
    return false;
}

static long scanSeqlenFromID(const string& idContent) {
    StringParser parser(idContent);
    string       lastWord = parser.extractWord(); // eat id
    bool         bpseen   = false;
    long         bp       = -1;

    while (!bpseen) {
        parser.eatSpaces();
        string word = parser.extractWord();
        if (word == "BP.") {
            //  basecount is in word before "BP."
            bp     = atol(lastWord.c_str());
            bpseen = true;
        }
        else {
            lastWord = word;
        }
    }

    if (bp == -1) throw "Could not parse bp from header";

    return bp;
}

void EmblImporter::import_section() {
    MetaInfo   meta;
    References refs;

    const MetaTag *prevTag      = 0;                // previously handled tag
    string         prevContent;                     // previously found content
    bool           prevAppendNL = false;            // append '\n' into  multiline tags

    bool seenHeaderLine = false;
    bool EOS            = false; // end of section ?

    // read header of file
    while (!EOS) {
        string line, tag, content;
        expectLine(line);
        if (!splitEmblTag(line, tag, content)) {
            throw "Expected two-character tag at start of line";
        }

        const MetaTag *knownTag = findTag(tag);
        if (!knownTag) throw GBS_global_string("Invalid tag '%s'", tag.c_str());

        if (knownTag == prevTag) {                  // multiline tag
            if (prevAppendNL) prevContent.append("\n"); // append a newline to make parsing in add_dbid() more easy
            prevContent.append(content);            // append w/o space - EMBL flatfiles have spaces at EOL when needed
        }
        else {                                      // start of new tag
            if (prevTag) {                          // save previous tag
                switch (prevTag->type) {
                    case MT_REF:        refs.add(prevTag->field, prevContent); break;
                    case MT_REF_DBID:   refs.add_dbid(prevContent); prevAppendNL = false; break;
                    case MT_BASIC:      meta.add(prevTag, prevContent, true); break;
                    case MT_HEADER:
                        meta.add(prevTag, prevContent, true);
                        expectedSeqLength = scanSeqlenFromID(prevContent);
                        break;
                    default: gi_assert(0); break;
                }
                prevTag = 0;
            }

            switch (knownTag->type) {
                case MT_HEADER:
                    if (seenHeaderLine) throw GBS_global_string("Multiple occurrences of tag '%s'", tag.c_str());
                    seenHeaderLine = true;
                    // fall-through
                case MT_BASIC:
                case MT_REF:
                    prevTag        = knownTag;
                    prevContent    = content;
                    break;

                case MT_REF_DBID:
                    prevTag      = knownTag;
                    prevContent  = content;
                    prevAppendNL = true;
                    break;

                case MT_REF_START:
                    refs.start(); // start a new reference
                    break;

                case MT_FEATURE:
                    flatfile.backLine(line);
                    db_writer.createOrganism(flatfile.getFilename(), "EMBL");
                    parseFeatureTable();
                    break;

                case MT_SEQUENCE_START:
                    parseSequence(content);
                    EOS = true; // end of section
                    break;

                case MT_FEATURE_START:
                case MT_IGNORE:
                    break;

                default:
                    gi_assert(0);
                    throw GBS_global_string("Tag '%s' not expected here", knownTag->tag.c_str());
            }
        }
    }
    db_writer.finalizeOrganism(meta, refs, *this);
    show_warnings(meta.getAccessionNumber());
}

// --------------------------------------------------------------------------------
// sequence readers:

inline bool parseCounter(bool expect, BaseCounter& headerCount, StringParser& parser, Base base, const char *word) {
    // parses part of string (e.g. " 6021225 BP;" or " 878196 A;")
    // if 'expect' == true -> throw exception if missing
    // if 'expect' == false -> return false if missing

    bool        found = false;
    stringCIter start = parser.getPosition();

    parser.expectSpaces(0);

    bool seen_number;
    long count = parser.eatNumber(seen_number);

    if (seen_number) {
        headerCount.addCount(base, count);
        size_t spaces = parser.eatSpaces();
        if (spaces>0) {
            size_t len = parser.lookingAt(word);
            if (len>0) {                        // seen
                parser.advance(len);
                found = true;
            }
        }
    }

    if (!found) {
        parser.setPosition(start); // reset position
        if (expect) throw GBS_global_string("Expected counter '### %s', found '%s'", word, parser.rest().c_str());
    }
    return found;
}

void GenebankImporter::parseSequence(const string& tag, const string& headerline) {
    SmartPtr<BaseCounter> headerCount;

    if (tag == "BASE") { // base count not always present
        // parse headerline :
        headerCount = new BaseCounter("sequence header");
        {
            StringParser parser(headerline);

            parser.expectContent("COUNT");

            parseCounter(true, *headerCount, parser, BC_A, "a");
            parseCounter(true, *headerCount, parser, BC_C, "c");
            parseCounter(true, *headerCount, parser, BC_G, "g");
            parseCounter(true, *headerCount, parser, BC_T, "t");
            parseCounter(false, *headerCount, parser, BC_OTHER, "others"); // not always present

            headerCount->calcOverallCounter();
        }
    }

    // parse sequence data
    size_t         est_seq_size = headerCount.isNull() ? 500000 : headerCount->getCount(BC_ALL);
    SequenceBuffer seqData(est_seq_size);
    {
        string line;

        if (!headerCount.isNull()) {
            // if BASE COUNT was present, check ORIGIN line
            // otherwise ORIGIN line has already been read
            expectLine(line);
            if (!beginsWith(line, "ORIGIN")) throw "Expected 'ORIGIN'";
        }

        bool eos_seen = false;
        while (!eos_seen) {
            expectLine(line);
            if (beginsWith(line, "//")) {
                eos_seen = true;
            }
            else {
                string       data;
                data.reserve(60);
                StringParser parser(line);

                parser.eatSpaces(); // not sure whether there really have to be spaces if number has 9 digits or more
                size_t cur_pos  = (size_t)parser.extractNumber();
                size_t datasize = seqData.getBaseCounter().getCount(BC_ALL);

                if (cur_pos != (datasize+1)) {
                    throw GBS_global_string("Got wrong base position (found=%zu, expected=%zu)", cur_pos, size_t(datasize+1));
                }

                int blocks = 0;
                while (!parser.atEnd() && parser.at() == ' ') {
                    parser.expectSpaces(1);

                    stringCIter start = parser.pos;
                    stringCIter end   = parser.find(' ');

                    data.append(start, end);
                    blocks++;
                }

                if (blocks>6) throw "Found more than 6 parts of sequence data";
                seqData.addLine(data);
            }
        }
    }

    if (headerCount.isNull()) {
        warning("No 'BASE COUNT' found. Base counts have not been validated.");
    }
    else {
        headerCount->expectEqual(seqData.getBaseCounter());
    }
    db_writer.writeSequence(seqData);
}

void EmblImporter::parseSequence(const string& headerline) {
    // parse headerline:
    BaseCounter  headerCount("sequence header");
    {
        StringParser parser(headerline);

        parser.expectContent("Sequence");

        parseCounter(true, headerCount, parser, BC_ALL,   "BP;");
        parseCounter(true, headerCount, parser, BC_A,     "A;");
        parseCounter(true, headerCount, parser, BC_C,     "C;");
        parseCounter(true, headerCount, parser, BC_G,     "G;");
        parseCounter(true, headerCount, parser, BC_T,     "T;");
        parseCounter(true, headerCount, parser, BC_OTHER, "other;");

        headerCount.checkOverallCounter();
    }

    // parse sequence data
    SequenceBuffer seqData(headerCount.getCount(BC_ALL));
    {
        bool   eos_seen = false;
        string line;

        while (!eos_seen) {
            expectLine(line);
            if (beginsWith(line, "//")) {
                eos_seen = true;
            }
            else {
                string data;
                data.reserve(60);
                StringParser parser(line);

                parser.expectSpaces(5, false);
                int blocks = 0;
                while (!parser.atEnd() && isalpha(parser.at())) {
                    stringCIter start = parser.pos;
                    stringCIter end   = parser.find(' ');

                    data.append(start, end);
                    blocks++;
                    parser.expectSpaces(1);
                }

                if (blocks>6) throw "Found more than 6 parts of sequence data";

                size_t basecount = (size_t)parser.extractNumber();

                seqData.addLine(data);
                size_t datasize = seqData.getBaseCounter().getCount(BC_ALL);

                if (basecount != datasize) {
                    throw GBS_global_string("Got wrong base counter(found=%zu, expected=%zu)", basecount, datasize);
                }
            }
        }
    }

    headerCount.expectEqual(seqData.getBaseCounter());
    db_writer.writeSequence(seqData);
}

