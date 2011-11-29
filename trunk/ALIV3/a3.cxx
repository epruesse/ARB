// ================================================================ //
//                                                                  //
//   File      : a3.cxx                                             //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "a3_arbdb.hxx"
#include "a3_ali.hxx"

#include <BI_helix.hxx>

#include <iostream>
#include <fstream>

using std::cout;
using std::ifstream;
using std::flush;

// -----------------------------------------------------------------------------
static void Usage ( void )
// -----------------------------------------------------------------------------
{
    cout << "\nAUFRUF: aliv3 <Datenbank> <Sequenz>\n";
    cout << "\n        <Datenbank> ::= Eine ARB-Datenbank (z.B.: 23sx.arb).";
    cout << "\n        <Sequenz>   ::= Name einer RNS-Sequenz (z.B.: ECOLI).\n";
    cout << "\nDarueberhinaus muss noch eine Datei matches.txt existieren, in der die Teilloesungen";
    cout << "\ndes Prealigners zeilenweise abgelegt sind. Innerhalb einer Zeile muessen die Positions-";
    cout << "\npaare der Loesung durch Kommata getrennt sein (z.B.: 5 10, 20 30, ...).\n\n";
}

// -----------------------------------------------------------------------------
    static DArray *ReadMatches ( str file )
// -----------------------------------------------------------------------------
{
    short     error = 0;
    DArray   *pre   = new DArray;
    ifstream  input(file);
        
    if (!pre || !input) error = 1;
    else
    {
        str line = NULL;
        
        pre->Free(DARRAY_NOFREE);
        pre->Null(DARRAY_NONULL);
        
        while (!error)
        {
            PSolution *psol = new PSolution;

            if (!psol) error = 3;
            else
            {
                str tmp = strtok(line,",");

                while (!error && tmp)
                {
                    HMatch *m = new HMatch;

                    if (!m) error = 4;
                    else
                    {
                        sscanf(tmp,"%d%d",&m->first,&m->last);

                        psol->match.Add(m);
                        psol->score += (m->last - m->first + 1);

                        tmp = strtok(NULL,",");
                    }
                }

                if (!error)
                {
                    psol->match.Sort(hmatchcmp);
                    pre->Add(psol);
                }
            }

            delete line;

            if (input.eof()) break;
        }

        if (!error) pre->Sort(psolcmp);
    }

    if (error) delete pre, pre = NULL;
    
    return pre;
}

// -----------------------------------------------------------------------------
int ARB_main ( int   argc,
               const char *argv[] )
// -----------------------------------------------------------------------------
{
    int error = 0;
    
    if (argc != 3) Usage();
    else
    {
        A3Arbdb db;
        
        if (db.open(argv[1])) error = 1;
        else
        {
            db.begin_transaction();
            
            str seq = db.get_sequence_string(argv[2]),
            fam = db.get_sequence_string("EscCol19");

            db.commit_transaction();
            
            if (!(seq && fam)) error = 2;
            else
            {
                cout << "\n" << seq;
                
                BI_helix    helix;
                const char *err = helix.init(db.gb_main);
                size_t      pos = 0;

                if (err) cout << err << ", " << helix.size();

                while (pos < helix.size())
                {
                    if (helix.pairtype(pos) != HELIX_PAIR) cout << ".";
                    else
                    {
                        if (helix.opposite_position(pos) < pos) cout << "]";
                        else cout << "[";
                    }

                    pos++;
                }

                cout << flush;
            }

            if (seq) delete seq;
            if (fam) delete fam;

            db.close();
        }
    }

    if (error) cout << "\nALIV3: Fehler Nummer " << error << " ist aufgetreten!\n\n";
    
    return error;
}
