// -----------------------------------------------------------------------------
//  Include-Dateien
// -----------------------------------------------------------------------------

#ifndef  _POSTREE_HXX
#include "postree.hxx"
#endif

#ifndef  _BASEN_H
#include "basen.h"
#endif

#ifndef  _FSTREAM_H
#include <fstream.h>
#endif

#ifndef  _STDLIB_H
#include <stdlib.h>
#endif

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
    int count = 0;

    qsort(array,*anz,sizeof(int),int_compare);

    while (count < (*anz - 1))
    {
        int same = 0;
        
        while (((count + 1+ same) < *anz) &&
               (array[count] == array[count + 1 + same])) same++;
        
        if (same)
        {
            memmove(&array[count],&array[count + same],
                    (*anz - (count + same)) * sizeof(int));

            (*anz) -= same;
        }

        count++;
    }
}

// -----------------------------------------------------------------------------
    static void CountAppearances ( str  seq,
                                   int *pos,
                                   int  num,
                                   int  off,
                                   int *anz )
// -----------------------------------------------------------------------------
{
    if (!num)
    {
        int base;
        
        while ((base = *seq++) != 0)
        {
            int index = BIndex[base];

            if ((index >= 0) &&
                (index < BASENPUR)) anz[index]++;
        }
    }
    else
    {
        while (num--)
        {
            int base = seq[pos[num] + off];
            
            if (base == 0) anz[BASEN] = 1;
            else           anz[BIndex[base]]++;
        }
    }
}

// -----------------------------------------------------------------------------
    static void FindAppearances ( int *list,
                                  int  base,
                                  int  anz,
                                  str  seq,
                                  int *pos,
                                  int  num,
                                  int  off )
// -----------------------------------------------------------------------------
{
    int next  = 0,
        count = 0,
        tmp;

    if (!num)
    {
        num = strlen(seq);

        while ((count < num) && (next < anz))
        {
            if (BIndex[seq[count]] == base) list[next++] = count;
                
            count++;
        }
    }
    else
        while ((count < num) && (next < anz))
        {
            if (base == BASEN)
            {
                if (!seq[pos[count]+ off]) list[next++] = pos[count];
            }
            else
            {
                if (BIndex[seq[pos[count]+ off]] == base) list[next++] = pos[count];
            }

            count++;
        }
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

    while (count < panz) field[count] = node.position[count++];
    
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
//  Konstruktor zum Erstellen eines Positionsbaumes aus einer Sequenz
// -----------------------------------------------------------------------------
    PtNode::PtNode ( str  seq,
                     int *pos,
                     int  num,
                     int  rec )
// -----------------------------------------------------------------------------
{
    if (!seq) mask = (Mask)0, position = NULL, next = NULL;
    else
    {
        int anz[BASEN + 1],
            base;

        mask     = (Mask)0;
        position = NULL;
        next     = NULL;
        
        memset(anz,0,(BASEN + 1) * sizeof(int));
        
        CountAppearances(seq,pos,num,rec,anz);

        if (anz[ADENIN] > 0)
            if (anz[ADENIN] > 1)    mask = (Mask)(mask | N_ADENIN);
            else                    mask = (Mask)(mask | P_ADENIN);

        if (anz[CYTOSIN] > 0)
            if (anz[CYTOSIN] > 1)   mask = (Mask)(mask | N_CYTOSIN);
            else                    mask = (Mask)(mask | P_CYTOSIN);

        if (anz[GUANIN] > 0)
            if (anz[GUANIN] > 1)    mask = (Mask)(mask | N_GUANIN);
            else                    mask = (Mask)(mask | P_GUANIN);

        if (anz[URACIL] > 0)
            if (anz[URACIL] > 1)    mask = (Mask)(mask | N_URACIL);
            else                    mask = (Mask)(mask | P_URACIL);

        if (anz[ONE] > 0)
            if (anz[ONE] > 1)       mask = (Mask)(mask | N_ONE);
            else                    mask = (Mask)(mask | P_ONE);

        if (anz[ANY] > 0)
            if (anz[ANY] > 1)       mask = (Mask)(mask | N_ANY);
            else                    mask = (Mask)(mask | P_ANY);

        if (anz[BASEN])             mask = (Mask)(mask | P_FINISH);
        
        {
            int panz = NumOfPos(),
                nanz = NumOfNext();

            if (panz) position = new int     [panz];
            if (nanz) next     = new PtNode* [nanz];
        }

        for (base = 0;base < (BASEN + 1);base++)
        {
            switch (anz[base])
            {
                case 0:     break;
                case 1:
                {
                    int  list,
                         which = base;
                    Mask tmp = P_ADENIN;
                    
                    while (which--) tmp = (Mask)(tmp * 2);
                    
                    FindAppearances(&list,base,1,seq,pos,num,rec);

                    position[IndexOfPos(tmp)] = list;

                    break;
                }
                default:
                {
                    int    *list  = new int [anz[base]],
                            which = base,
                            index;
                    Mask    tmp   = N_ADENIN;
                    PtNode *ptn;
                        
                    while (which--) tmp = (Mask)( tmp * 2);
                        
                    FindAppearances(list,base,anz[base],seq,pos,num,rec);

                    index = IndexOfNext(tmp);
                    ptn   = new PtNode(seq,list,anz[base],rec + 1);
                    next[index] = ptn;

                    delete list;
                    
                    break;
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------
//  Kopierkonstruktor
// -----------------------------------------------------------------------------
    PtNode::PtNode ( PtNode &node )
// -----------------------------------------------------------------------------
{
    cout << "\nKopierkonstruktor(PtNode)\n";
    
    mask = node.mask;
    
    if (position)
    {
        int anz = node.NumOfPos();
        
        position = new int [anz];

        memcpy(position,node.position,anz * sizeof(int));
    }
    
    if (next)
    {
        int anz = node.NumOfNext();
        
        next = new PtNode* [anz];
        
        while (anz--)
            if (node.next[anz]) next[anz] = new PtNode(*node.next[anz]);
            else                next[anz] = NULL;
    }
}

// -----------------------------------------------------------------------------
//  Destruktor
// -----------------------------------------------------------------------------
    PtNode::~PtNode ( void )
// -----------------------------------------------------------------------------
{
    if (position) delete position;
    
    if (next)
    {
        int anz = NumOfNext();
        
        while (anz--) if (next[anz]) delete next[anz];
        
        delete next;
    }
}

// -----------------------------------------------------------------------------
//  Anzahl der vorhandenen Positionen
// -----------------------------------------------------------------------------
    int PtNode::NumOfPos ( void )
// -----------------------------------------------------------------------------
{
    int anz  = 0,
        test = P_ADENIN;
        
    while (test <= P_FINISH)
    {
        if (mask & test) anz++;
            
        test *= 2;
    }

    return anz;
}

// -----------------------------------------------------------------------------
//  Positionsindex einer Base
// -----------------------------------------------------------------------------
    int PtNode::IndexOfPos ( Mask base )
// -----------------------------------------------------------------------------
{
    int index = INVALID;
    
    if ((base <= P_FINISH) && (mask & base))
    {
        int test = P_ADENIN;
        
        index = 0;
        
        while (test <= base)
        {
            if (mask & test) index++;
            
            test *= 2;
        }

        index--;
    }

    return index;
}

// -----------------------------------------------------------------------------
//  Anzahl der vorhandenen Verzweigungen
// -----------------------------------------------------------------------------
    int PtNode::NumOfNext ( void )
// -----------------------------------------------------------------------------
{
    int anz  = 0,
        test = N_ADENIN;
        
    while (test < (N_ANY * 2))
    {
        if (mask & test) anz++;
            
        test *= 2;
    }

    return anz;
}

// -----------------------------------------------------------------------------
//  Verzweigungsindex einer Base
// -----------------------------------------------------------------------------
    int PtNode::IndexOfNext ( Mask base )
// -----------------------------------------------------------------------------
{
    int index = INVALID;
    
    if ((base >= N_ADENIN) && (base <= N_ANY) && (mask & base))
    {
        int test = N_ADENIN;
        
        index = 0;
        
        while (test <= base)
        {
            if (mask & test) index++;
            
            test *= 2;
        }

        index--;
    }

    return index;
}

// -----------------------------------------------------------------------------
//  Ausgabefunktion fuer ein Knoten eines Positionsbaumes
// -----------------------------------------------------------------------------
    void PtNode::Dump ( void )
// -----------------------------------------------------------------------------
{
    cout << "\nnode: " << this;
    cout << "\nmask = 0x" << hex << mask << "\n";
    
    if (position)
    {
        int anz = NumOfPos(),
            pos = 0;
        
        cout << "position[" << dec << anz << "] =";
        
        while (pos < anz) cout << " " << position[pos++];

        cout << "\n";
    }

    if (next)
    {
        int anz = NumOfNext(),
            pos = 0;
        
        cout << "next[" << dec << anz << "]:";
        
        while (pos < anz) cout << " " << next[pos++];

        cout << "\n";

        pos = 0;
        
        while (pos < anz) next[pos++]->Dump();
    }
}

// -----------------------------------------------------------------------------
//  Ausgabefunktion fuer einen Positionsbaum
// -----------------------------------------------------------------------------
    void PtNode::Show ( int  rec,
                        int *where )
// -----------------------------------------------------------------------------
{
    int  panz   = NumOfPos(),
         nanz   = NumOfNext();
    str  prefix = new char [rec * 3];

    prefix[0] = 0;
    
    if (rec)
    {
        int count = 0;
        
        prefix[0] = 0;
        
        while (count < rec)
            if (where[count++]) strcat(prefix,"|  ");
            else                strcat(prefix,"   ");
    }
    
    if (panz)
    {
        Base base  = ADENIN;
        Mask which = P_ADENIN;
        
        while (which <= P_FINISH)
        {
            int index = IndexOfPos(which);

            if ((index != INVALID) && (index < panz))
            {
                if (base == BASEN)
                {
                    if (!nanz && (index == (panz - 1)))
                        cout << prefix << "\\- # " << position[index] << "\n";
                    else
                        cout << prefix << "|- # " << position[index] << "\n";
                }
                else
                {
                    if (!nanz && (index == (panz - 1)))
                        cout << prefix << "\\- " << (char)BCharacter[base],
                        cout << " " << position[index] << "\n";
                    else
                        cout << prefix << "|- " << (char)BCharacter[base],
                        cout << " " << position[index] << "\n";
                }
            }
            
            which = (Mask)(which * 2);
            base  = (Base)(base  + 1);
        }
    }

    if (nanz)
    {
        Base  base     = ADENIN;
        Mask  which    = N_ADENIN;
        int  *wherenew = new int [rec + 1];

        memcpy(wherenew,where,rec * sizeof(int));

        while (which <= N_ANY)
        {
            int index = IndexOfNext(which);

            if ((index != INVALID) && (index < nanz))
            {
                if (base == BASEN)
                {
                    if (index == (nanz - 1))
                        cout << prefix << "\\- #\n",
                        wherenew[rec] = 0;
                    else
                        cout << prefix << "|- #\n",
                        wherenew[rec] = 1;
                }
                else
                {
                    if (index == (nanz - 1))
                        cout << prefix << "\\- " << (char)BCharacter[base] << "\n",
                        wherenew[rec] = 0;
                    else
                        cout << prefix << "|- " << (char)BCharacter[base] << "\n",
                        wherenew[rec] = 1;
                }
                
                next[index]->Show(rec + 1,wherenew);
            }
            
            which = (Mask)(which * 2);
            base  = (Base)(base  + 1);
        }

        delete wherenew;
    }

    delete prefix;
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
    Postree::Postree ( str seq ) : sequence ( seq )
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
        int base;

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
             llen[3] = { *len, 0, *len },
             res [3] = { INVALID, INVALID, INVALID },
             count;
        
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
