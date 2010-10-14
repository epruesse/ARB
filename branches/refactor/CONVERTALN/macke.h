// ================================================================= //
//                                                                   //
//   File      : macke.h                                             //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef MACKE_H
#define MACKE_H

class MackeReader : public DataReader {
    bool        firstRead;
    char       *inName;
    const char *seqabbr; // owner Macke.seqabbr (set by mackeIn())

    Reader *r1, *r2, *r3;
    void start_reading();
    void stop_reading();

public:

    MackeReader(const char *inName_)
        : firstRead(true),
          inName(strdup(inName_)),
          seqabbr(0),
          r1(NULL), 
          r2(NULL), 
          r3(NULL) 
    {}
    ~MackeReader() {
        stop_reading();
        free(inName);
    }

    bool mackeIn(Macke& macke);

    SeqPtr read_sequence_data() {
        SeqPtr seq = new Seq;

        ca_assert(seqabbr);
        macke_origin(*seq, seqabbr, *r2);
        return seq;
    }
};

inline bool isMackeHeader(const char *line)    { return line[0] == '#'; }
inline bool isMackeSeqHeader(const char *line) { return line[0] == '#' && line[1] == '='; }
inline bool isMackeSeqInfo(const char *line)   { return line[0] == '#' && line[1] == ':'; }

inline bool isMackeNonSeq(const char *line)    { return line[0] == '#' || line[0] == '\n' || line[0] == ' '; }

class Not {
    typedef bool (*LinePredicate)(const char *line);
    LinePredicate p;
public:
    Not(LinePredicate p_) : p(p_) {}
    bool operator()(const char *line) { return !p(line); }
};


#else
#error macke.h included twice
#endif // MACKE_H
