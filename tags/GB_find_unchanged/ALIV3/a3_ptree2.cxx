// -----------------------------------------------------------------------------
//  Include-Dateien
// -----------------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <cstdlib>

#include "a3_basen.h"
#include "a3_ptree.hxx"

using std::cout;

// -----------------------------------------------------------------------------
static int int_compare ( const void *a,
                         const void *b )
// -----------------------------------------------------------------------------
{
    int result = 0,
        diff   = *(int*)a - *(int*)b;
    
    if      (diff < 0) result = -1;
    else if (diff > 0) result =  1;

    return result;
}

// -----------------------------------------------------------------------------
    static void SortIntArray ( int *array,
                               int *anz )
// -----------------------------------------------------------------------------
{
    qsort(array,*anz,sizeof(int),int_compare);

    int source;
    int dest = 1;
    int lastelem = array[0];

    for (source = 1; source < *anz; source++) {
        if (array[source] != lastelem){
            array[dest++] = lastelem = array[source];
        }
    }
    *anz = dest;
}

// -----------------------------------------------------------------------------
//  Liefert die Anzahl aller Positionen unterhalb eines bestimmten Knotens
// -----------------------------------------------------------------------------
    static int CountLeaves ( PtNode &node )
// -----------------------------------------------------------------------------
{
    int panz = node.NumOfPos(),
        nanz = node.NumOfNext();

    while (nanz--) panz += CountLeaves(*node.next[nanz]);

    return panz;
}

// -----------------------------------------------------------------------------
//  Sucht alle Positionen unterhalb eines bestimmten Knotens
// -----------------------------------------------------------------------------
    static int GetLeaves ( PtNode &node,
                           int    *field )
// -----------------------------------------------------------------------------
{
    int panz  = node.NumOfPos(),
        nanz  = node.NumOfNext(),
        count = 0;

    while (count < panz) {
        field[count] = node.position[count];
        count++;
    }

    while (nanz--) panz += GetLeaves(*node.next[nanz],&field[panz]);

    return panz;
}

// -----------------------------------------------------------------------------
//  Sucht alle Positionen unterhalb eines bestimmten Knotens
// -----------------------------------------------------------------------------
    static int *FindLeaves ( PtNode &node,
                             int    *anz )
// -----------------------------------------------------------------------------
{
    int *field = NULL;

    if (((*anz = CountLeaves(node)) > 0) &&
        ((field = new int [*anz])   != NULL)) GetLeaves(node,field);
    
    return field;
}

// -----------------------------------------------------------------------------
//  Konstruktor zum Erzeugen einer leeren Instanz der Klasse Postree
// -----------------------------------------------------------------------------
    Postree::Postree ( void ) : sequence ()
// -----------------------------------------------------------------------------
{
    topnode = NULL;
}

// -----------------------------------------------------------------------------
//  Konstruktor zum Erzeugen einer Instanz der
//  Klasse Postree aus einer vorgegebenen Sequenz
// -----------------------------------------------------------------------------
    Postree::Postree ( str seq2 ) : sequence ( seq2 )
// -----------------------------------------------------------------------------
{
    str seq = sequence.Compressed();
    
    if (seq) topnode = new PtNode(seq,NULL,0,0), delete seq;
}

// -----------------------------------------------------------------------------
//  Konstruktor zum Erzeugen einer Instanz der Klasse Postree aus einer 
//  zufaellig besetzten Instanz der Klasse Sequenz mit vorgebener Sequenzlaenge
// -----------------------------------------------------------------------------
    Postree::Postree ( uint len ) : sequence ( len )
// -----------------------------------------------------------------------------
{
    str seq = sequence.Compressed();
    
    if (seq) topnode = new PtNode(seq,NULL,0,0), delete seq;
}

// -----------------------------------------------------------------------------
//  Konstruktor zum Erzeugen einer leeren Instanz der Klasse Postree
//  aus einer Instanz der Klasse Sequenz aus ein vorgebener Datei mit
//  vorgegebener Zeilennummer
// -----------------------------------------------------------------------------
    Postree::Postree ( str  file,
                       uint line ) : sequence ( file, line )
// -----------------------------------------------------------------------------
{
    str seq = sequence.Compressed();
    
    if (seq) topnode = new PtNode(seq,NULL,0,0), delete seq;
}

// -----------------------------------------------------------------------------
//  Kopierkonstruktor
// -----------------------------------------------------------------------------
    Postree::Postree ( Postree &postree ) : sequence ( postree.sequence )
// -----------------------------------------------------------------------------
{
    cout << "\nKopierkonstruktor(Postree)\n";

    if (!postree.topnode) topnode = NULL;
    else                  topnode = new PtNode(*postree.topnode);
}

// -----------------------------------------------------------------------------
//  Destruktor
// -----------------------------------------------------------------------------
    Postree::~Postree ( void )
// -----------------------------------------------------------------------------
{
    if (topnode) delete topnode;
}

// -----------------------------------------------------------------------------
//  Liefert Laenge der kompremierten Sequenz
// -----------------------------------------------------------------------------
    int Postree::SeqLen ( void )
// -----------------------------------------------------------------------------
{
    return sequence.CompressedLen();
}

// -----------------------------------------------------------------------------
//  Positionsbaum fuer neue Sequenz aufbauen
// -----------------------------------------------------------------------------
    void Postree::Set ( str seq )
// -----------------------------------------------------------------------------
{
    if (seq && *seq)
    {
        str tmp;
        
        sequence.Set(seq);
        
        if (topnode) delete topnode, topnode = NULL;
        
        tmp = sequence.Compressed();
        
        if (tmp) topnode = new PtNode(tmp,NULL,0,0), delete tmp;
    }
}

// -----------------------------------------------------------------------------
//  Exakte Suche nach den Vorkommen einer Teilsequenz
// -----------------------------------------------------------------------------
    int Postree::Find ( str   seq,
                        int **pos,
                        int  *anz,
                        int  *len,
                        int   sort )
// -----------------------------------------------------------------------------
{
    int result = INVALID;

    *pos = NULL;
    *anz = 0;
    *len = 0;
    
    if (seq && *seq && pos && anz && len)
    {
        PtNode *node = topnode;
        
        result = 0;
        
        while (!result && *seq)
        {
            int base = BIndex[*seq++];
            
            if ((base < ADENIN) ||
                (base > URACIL)) result = INVALID;
            else
            {
                Mask pmask = P_ADENIN,
                     nmask = N_ADENIN;
                
                while (base--)
                {
                    pmask = (Mask)(pmask * 2);
                    nmask = (Mask)(nmask * 2);
                }
                
                if (node->mask & pmask) // Position gefunden
                {
                    (*len)++;

                    *anz = 1;
                    *pos = new int [1];
                        
                    if (!*pos) result = INVALID;    // Speicher
                    else
                    {
                        *pos[0] = node->position[node->IndexOfPos(pmask)];
                    
                        if (*seq)   // Gesuchte Sequenz ist laenger als Baum tief
                        {
                            str org = sequence.Compressed();

                            if (!org) result = INVALID; // Speicher
                            else
                            {
                                str ptr = &org[*pos[0] + *len];
                            
                                while (*ptr && *seq && (*seq == *ptr))
                                {
                                    (*len)++;
                                    seq++;
                                    ptr++;
                                }

                                if (*seq && *ptr) result = 1;   // Unterschied
                            
                                delete org;
                            }
                        }
                    }
                }
                else if (node->mask & nmask)    // Verzweigung existiert
                {
                    (*len)++;

                    node = node->next[node->IndexOfNext(nmask)];
                }
                else    // Unterschied => alle Moeglichkeiten suchen
                {
                    result = 1;

                    *pos = FindLeaves(*node,anz);
                }
            }
        }

        if (!result && !*seq && !*pos)  // Sequenz ist kuerzer als Baum tief
        {
            *pos = FindLeaves(*node,anz);

            if (!*pos) result = INVALID;    // Speicher
        }
    }

    if (sort && *pos && (*anz > 1)) SortIntArray(*pos,anz);
    
    return result;
}

// -----------------------------------------------------------------------------
//  Suche nach den Vorkommen einer Teilsequenz mit bis zu einer Substitution
// -----------------------------------------------------------------------------
    int Postree::OneSubstitution ( str   seq,
                                   int **pos,
                                   int  *anz,
                                   int  *len )
// -----------------------------------------------------------------------------
{
    int  result  = INVALID,
         seqlen  = strlen(seq),
        *lpos    = NULL,
         lanz    = 0,
         llen    = 0,
         res,
         count;
    str  search  = new char [seqlen + 1];
        
    for (count = 0;count < seqlen;count++)
    {
        int base;
                
        strcpy(search,seq);
            
        for (base = ADENIN;base <= URACIL;base++)
        {
            if (search[count] == BCharacter[base]) continue;

            search[count] = BCharacter[base];

            res = Find(search,&lpos,&lanz,&llen,0);

            if (res >= 0)
            {
                if (llen > *len)
                {
                    if (*pos) delete *pos;

                    *pos   = lpos;
                    *len   = llen;
                    *anz   = lanz;
                    result = res;
                }
                else if (llen == *len)
                {
                    int *tmp = new int [*anz + lanz];
                            
                    if (*anz) memcpy(tmp,*pos,*anz * sizeof(int));

                    memcpy(&tmp[*anz],lpos,lanz * sizeof(int));

                    if (*pos) delete *pos;
                            
                    *pos    = tmp;
                    *anz   += lanz;
                    result  = res;
                }
            }

            search[count] = seq[count];
        }
    }

    delete search;

    return result;
}

// -----------------------------------------------------------------------------
//  Suche nach den Vorkommen einer Teilsequenz mit bis zu einer Deletion
// -----------------------------------------------------------------------------
    int Postree::OneDeletion ( str   seq,
                               int **pos,
                               int  *anz,
                               int  *len )
// -----------------------------------------------------------------------------
{
    int  result  = INVALID,
         seqlen  = strlen(seq),
        *lpos    = NULL,
         lanz    = 0,
         llen    = 0,
         res,
         count;
    str  search  = new char [seqlen + 1];
        
    for (count = 0;count < seqlen;count++)
    {
        if (!count) *search = 0;
        else         strncpy(search,seq,count), search[count] = 0;
        
        strcat(search,&seq[count + 1]);

        res = Find(search,&lpos,&lanz,&llen,0);

        if (res >= 0)
        {
            if (llen > *len)
            {
                if (*pos) delete *pos;

                *pos   = lpos;
                *len   = llen;
                *anz   = lanz;
                result = res;
            }
            else if (llen == *len)
            {
                int *tmp = new int [*anz + lanz];
                            
                if (*anz) memcpy(tmp,*pos,*anz * sizeof(int));
                
                memcpy(&tmp[*anz],lpos,lanz * sizeof(int));

                if (*pos) delete *pos;
                            
                *pos    = tmp;
                *anz   += lanz;
                result  = res;
            }
        }
    }

    delete search;

    return result;
}

// -----------------------------------------------------------------------------
//  Suche nach den Vorkommen einer Teilsequenz mit bis zu einer Insertion
// -----------------------------------------------------------------------------
    int Postree::OneInsertion ( str   seq,
                                int **pos,
                                int  *anz,
                                int  *len )
// -----------------------------------------------------------------------------
{
    int  result  = INVALID,
         seqlen  = strlen(seq),
        *lpos    = NULL,
         lanz    = 0,
         llen    = 0,
         res,
         count;
    str  search  = new char [seqlen + 2];
        
    for (count = 0;count <= seqlen;count++)
    {
        int base;

        if (!count) search[0] = ' ',
                    search[1] = 0;
        else        strncpy(search,seq,count),
                    search[count] = ' ',
                    search[count + 1] = 0;
        
        strcat(search,&seq[count]);
            
        for (base = ADENIN;base <= URACIL;base++)
        {
            search[count] = BCharacter[base];

            res = Find(search,&lpos,&lanz,&llen,0);

            if (res >= 0)
            {
                if (llen > *len)
                {
                    if (*pos) delete *pos;

                    *pos   = lpos;
                    *len   = llen;
                    *anz   = lanz;
                    result = res;
                }
                else if (llen == *len)
                {
                    int *tmp = new int [*anz + lanz];
                            
                    if (*anz) memcpy(tmp,*pos,*anz * sizeof(int));

                    memcpy(&tmp[*anz],lpos,lanz * sizeof(int));

                    if (*pos) delete *pos;
                            
                    *pos    = tmp;
                    *anz   += lanz;
                    result  = res;
                }
            }

            search[count] = seq[count];
        }
    }

    delete search;

    return result;
}

// -----------------------------------------------------------------------------
//  Suche nach den Vorkommen einer Teilsequenz mit bis zu einem Fehler
// -----------------------------------------------------------------------------
    int Postree::OneMismatch ( str   seq,
                               int **pos,
                               int  *anz,
                               int  *len )
// -----------------------------------------------------------------------------
{
    int result = Find(seq,pos,anz,len,0);

    if (result >= 0)
    {
        int *lpos[3] = { NULL, NULL, NULL },
             lanz[3] = { 0, 0, 0 },
             llen[3],
             res [3] = { INVALID, INVALID, INVALID },
             count;
        
        llen[0] = llen[2] = *len;
        llen[1] = 0;
        res[0] = OneSubstitution(seq,&lpos[0],&lanz[0],&llen[0]);
        res[1] = OneDeletion    (seq,&lpos[1],&lanz[1],&llen[1]);
        res[2] = OneInsertion   (seq,&lpos[2],&lanz[2],&llen[2]);

        for (count = 0; count < 3;count++)
        {
            if (res[count] >= 0)
            {
                result = 1;
                
                if (llen[count] > *len)
                {
                    if (*pos) delete *pos;

                    *pos   = lpos[count];
                    *len   = llen[count];
                    *anz   = lanz[count];
                    result = res [count];

                    lpos[count] = NULL;
                }
                else if (llen[count] == *len)
                {
                    int *tmp = new int [*anz + lanz[count]];
                            
                    if (*anz) memcpy(tmp,*pos,*anz * sizeof(int));

                    memcpy(&tmp[*anz],lpos[count],lanz[count] * sizeof(int));

                    if (*pos) delete *pos;
                            
                    *pos    = tmp;
                    *anz   += lanz[count];
                }
            }

            if (lpos[count]) delete lpos[count];
        }

        if (*pos && (*anz > 1)) SortIntArray(*pos,anz);
    }

    return result;
}

// -----------------------------------------------------------------------------
//  Ausgabefunktion fur eine Instanz der Klasse Postree zu debug-Zwecken
// -----------------------------------------------------------------------------
    void Postree::Dump ( void )
// -----------------------------------------------------------------------------
{
    cout << "\nsequence :\n";
    
    sequence.Dump();
    
    cout << "topnode :\n";
    
    if (topnode) topnode->Dump(), cout << "\n";
}

// -----------------------------------------------------------------------------
//  Ausgabefunktion fur eine Instanz der Klasse Postree
// -----------------------------------------------------------------------------
    void Postree::Show ( int mode )
// -----------------------------------------------------------------------------
{
    switch (mode)
    {
        case 0:     cout << "\npostree:\n\n";
                    if (topnode) topnode->Show(0,NULL), cout << "\n";
                    break;
        case 1:
        {
            str seq = sequence.Compressed();
            
            cout << "\nsequence:\n\n";

            if (seq)
            {
                cout << "Laenge: " << strlen(seq) << "\n";
                cout << seq << "\n";
                delete seq;
            }
            
            break;
        }
        case 2:     cout << "\nsequence :\n";
                    sequence.Dump();
                    cout << "\npostree :\n\n";
                    if (topnode) topnode->Show(0,NULL), cout << "\n";
                    break;
    }
}
