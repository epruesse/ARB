/**
 * Declaration of all objects belonging to this version of
 * the valid names text file
 *
 * 29. November 2002
 * 
 * copyright by Lothar Richter
 */

#ifndef NT_VALID_NAMEPARSER
#define NT_VALID_NAMEPARSER

namespace validNames {
  
    typedef list<string> LineSet;
    typedef LineSet* LineSetPtr;

    typedef vector<string> TokL;
    typedef TokL *TokLPtr;

    typedef enum {
        VALGEN, HETGEN, HOMGEN, RENGEN, CORGEN, 
        VALSPEC, HETSPEC, HOMSPEC, RENSPEC, CORSPEC,
        NOTYPE, VAL, HET, HOM, REN, COR
    } DESCT;

    class Desco {
    private:
        DESCT type;
        bool isCorrected;
        string firstgen;
        string firstspec;
        string firstsub;
        string secondgen;
        string secondspec;
        string secondsub;
    
    public:
        inline  Desco(DESCT type_ , bool isCorrected_, string firstgen_,string firstspec_,string firstsub_, 
                      string secondgen_ ,string secondspec_, string secondsub_){
            type = type_;
            isCorrected = isCorrected_;
            firstgen = firstgen_;
            firstspec = firstspec_;
            firstsub = firstsub_;
            secondgen = secondgen_;
            secondspec = secondspec_;
            secondsub = secondsub_;
        };

        string getFirstName();
        string getSecondName();
        inline DESCT getType(){return type;}
    };
  



    LineSet* readFromFile(const char* infile, LineSet* listOfLines);

    TokLPtr tokenize(const string& description, TokLPtr tokenLP);

    Desco determineType(const string& descriptionString);

    bool isUpperCase(const string& input);
}; /* end namespace */

#endif

