// -----------------------------------------------------------------------------
//  Include-Dateien
// -----------------------------------------------------------------------------

#include <iostream.h>
#include <stdlib.h>

#include "a3_ptree.hxx"

// -----------------------------------------------------------------------------
    static void Menu ( Postree &pt )
// -----------------------------------------------------------------------------
{
    int done = 0;
    
    while (!done)
    {
        int choice;
        
        cout << "\nMenue:\n";
        cout << "\na   - Sequenz anzeigen";
        cout << "\nb   - Positionsbaum anzeigen";
        cout << "\nf   - Teilsequenz suchen (exakt)";
        cout << "\ns   - Teilsequenz suchen (<= 1 Fehlposition)";
        cout << "\ne/q - Programm beenden\n";
        cout << "\nDeine Wahl? ";
        
        choice = cin.get();
        
        switch (choice)
        {
            case 'a':   pt.Show(1);
                        break;
            case 'b':   pt.Show(0);
                        break;
            case 'f':
            {
                int  seqlen = pt.SeqLen(),
                    *pos    = NULL,
                     anz    = 0,
                     len    = 0,
                     res;
                str  seq    = new char [seqlen + 1];
                
                cout << "\nSuchsequenz eingeben : ";
                cin  >> seq;

                if ((res = pt.Find(seq,&pos,&anz,&len,1)) < 0) cout << "\nGrober Fehler!\n";
                else
                {
                    int count = 0;

                    if (!res)
                    {
                        cout << "\nSuchsequenz " << seq << " der Laenge " << len;
                        cout << " wurde an " << anz << " Position(en) gefunden:";

                        while (count < anz) cout << " " << pos[count++];
                        
                        cout << "\n";
                    }
                    else
                    {
                        cout << "\nSuchsequenz " << seq << " der Laenge ";
                        cout << len << " wurde nicht gefunden!";
                        seq[len] = 0;
                        cout << "\nDas laengste Prefix " << seq << " mit Laenge " << len;
                        cout << " wurde an " << anz << " Position(en) gefunden:";

                        while (count < anz) cout << " " << pos[count++];
                        
                        cout << "\n";
                    }
                }

                break;
            }
            case 's':
            {
                int  seqlen = pt.SeqLen(),
                    *pos    = NULL,
                     anz    = 0,
                     len    = 0,
                     res;
                str  seq    = new char [seqlen + 1];
                
                cout << "\nSuchsequenz eingeben : ";
                cin  >> seq;

                if      ((res = pt.OneMismatch(seq,&pos,&anz,&len)) < 0) cout << "\nGrober Fehler!\n";
                else
                {
                    int count = 0;
                    
                    seq[len] = 0;
                    
                    cout << "\nSuchsequenz " << seq << " der Laenge " << len;
                    cout << " wurde an " << anz << " Position(en) gefunden:";

                    while (count < anz) cout << " " << pos[count++];
                        
                    cout << "\n";
                }

                break;
            }
            case 'q':
            case 'e':   done = 1;
                        break;
            default:    cout << "\nUnzulaessige Eingabe '" << choice << "'\n";
                        break;
        }

        choice = cin.get();
    }
}

// -----------------------------------------------------------------------------
    int main ( int   argc,
               char *argv[] )
// -----------------------------------------------------------------------------
{
    int error = 0;
    
    if ((argc < 2) || (argc > 3))
        cout << "\nUSAGE: aliv3 [<length> | [<file> <line>]]\n\n";
    else
    {
        if (argc == 2)
        {
            Postree pt(atoi(argv[1]));

            Menu(pt);
        }
        else if (argc == 3)
        {
            Postree pt(argv[1],atoi(argv[2]));

            Menu(pt);
        }
    }
    
    return error;
}
