// -----------------------------------------------------------------------------
//  Include-Dateien
// -----------------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <cstring>

#include "a3_helix.hxx"
#include "a3_ali.hxx"
#include "a3_matrix.hxx"

using std::cout;

// -----------------------------------------------------------------------------
static str CleanHelix ( str hel )
// -----------------------------------------------------------------------------
{
    str heli = strdup(hel),
        ptr  = heli;

    if (ptr)
    {
        int ch;
        
        while ((ch = *ptr) != 0)
        {
            if (ch == '<') *ptr = '[';
            if (ch == '>') *ptr = ']';

            ptr++;
        }
    }

    return heli;
}

// -----------------------------------------------------------------------------
    static int FindHelices ( str       helix,
                             int       left,
                             int       right,
                             A3Matrix &matrix )
// -----------------------------------------------------------------------------
{
    int    error     =  0,
           increment =  0,
           count     =  0,
           last      = -1,
           pos       =  0;
    DArray mark(10,5,DARRAY_NOFREE);
    
    while (!error && (pos <= right))
    {
        if       (helix[pos] == '[') count++, increment = 1;
        else if ((helix[pos] == ']') && count)
        {
            if (increment > 0)
            {
                int back = pos - 1;
                
                while ((back >= left) && (helix[back] != '[')) back--;

                if      (back < left) error = 2;
                else if (matrix.Set(pos - left,back - left,(vp)1) ||
                         matrix.Set(back - left,pos - left,(vp)1)) error = 3;
                {
                    if      (last < 0)                     last = mark.Add((vp)pos);
                    else if ((back - (int)mark[last]) > 1) last = mark.Add((vp)pos);
                }
            }
            else if (increment < 0)
            {
                int back    = (int)mark[last],
                    backend = left,
                    tmp     = back;
                
                while (tmp--) if (matrix.Get(back - left,tmp - left)) break;
                
                back = tmp - 1;
                
                if (last > 0) backend = (int)mark[last - 1] + 1;

                while (back >= backend)
                {
                    if (helix[back] == '[')
                    {
                        tmp = back + 1;
                        
                        while (tmp <= right)
                        {
                            if (matrix.Get(tmp - left,back - left)) break;
                            else                                    tmp++;
                        }

                        if (tmp > right)
                        {
                            if (matrix.Set(pos - left,back - left,(vp)1) ||
                                matrix.Set(back - left,pos - left,(vp)1)) error = 3;
                            else
                            {
                                if (last >= 0) mark.Del(last--);

                                if      (last < 0)                     last = mark.Add((vp)pos);
                                else if ((back - (int)mark[last]) > 1) last = mark.Add((vp)pos);

                                break;
                            }
                        }
                    }

                    if (back == backend)
                    {
                        if (last >= 0) mark.Del(last--);
                        
                        if (last > 0) backend = (int)mark[last - 1] + 1;
                        else          backend = left;
                    }
                    
                    back--;
                }
            }
            else error = 1;

            increment = -1;
            count--;
        }
        
        pos++;
    }

    return error;
}

// -----------------------------------------------------------------------------
    int intcmp ( const void *a,
                 const void *b )
// -----------------------------------------------------------------------------
{
    int res  = 0,
        diff = *(int*)a - *(int*)b;

    if      (diff < 0) res = -1;
    else if (diff > 0) res =  1;

    return res;
}

// -----------------------------------------------------------------------------
    int hmatchcmp ( const void *a,
                    const void *b )
// -----------------------------------------------------------------------------
{
    int res  = 0,
        diff = (*(HMatch**)a)->first - (*(HMatch**)b)->first;

    if      (diff < 0) res = -1;
    else if (diff > 0) res =  1;

    return res;
}

// -----------------------------------------------------------------------------
    void hmatchdump ( vp val )
// -----------------------------------------------------------------------------
{
    HMatch *m = (HMatch*)val;
    
    if (m) cout << "   " << m->first << ", " << m->last;
}

// -----------------------------------------------------------------------------
    HMatch &A3Helix::GetPart ( int part )
// -----------------------------------------------------------------------------
{
    HMatch *area  = NULL;
    int     parts = Parts();
    
    if ((part >= 0) && (part < parts))
    {
        int anz = match.Elements(),
            beg = !!(((HMatch*)match[0])->first),
            end = (((HMatch*)match[anz - 1])->last == (length - 1)),
            pos = 0;
        
        area  = new HMatch;
        parts = 0;

        while (pos <= anz)
        {
            if (!pos)
            {
                area->first = 0;

                if (beg) area->last = ((HMatch*)match[pos])->last;
                else     area->last = ((HMatch*)match[pos + 1])->last;
            }
            else if (pos == anz)
            {
                if (!beg) break;

                area->first = ((HMatch*)match[pos - 1])->first;
                area->last  = length - 1;
            }
            else if (pos == (anz - 1))
            {
                if (beg)
                {
                    area->first = ((HMatch*)match[pos - 1])->first;
                    area->last  = ((HMatch*)match[pos])->last;
                }

                if (end) break;

                if (!beg)
                {
                    area->first = ((HMatch*)match[pos])->first;
                    area->last  = length - 1;
                }
            }
            else
            {
                if (beg)
                {
                    area->first = ((HMatch*)match[pos - 1])->first;
                    area->last  = ((HMatch*)match[pos])->last;
                }
                else
                {
                    area->first = ((HMatch*)match[pos])->first;
                    area->last  = ((HMatch*)match[pos + 1])->last;
                }
            }
            
            if (parts == part) break;

            parts++;
            pos++;
        }
    }
    
    return *area;
}

// -----------------------------------------------------------------------------
    A3Helix::A3Helix ( void ) : match()
// -----------------------------------------------------------------------------
{
    helix     =
    consensus = NULL;
    length    = 0;

    match.Free(DARRAY_NOFREE);
    match.Null(DARRAY_NONULL);
    match.Sort(intcmp);
}

// -----------------------------------------------------------------------------
    A3Helix::A3Helix ( str     hel,
                       str     kon,
                       DArray &mat ) : match(mat)
// -----------------------------------------------------------------------------
{
    helix     = CleanHelix(hel);
    consensus = strdup(kon);
    length    = strlen(helix);
}

// -----------------------------------------------------------------------------
    A3Helix::~A3Helix ( void )
// -----------------------------------------------------------------------------
{
    if (helix)  delete helix;
    if (consensus)  delete consensus;
}

// -----------------------------------------------------------------------------
    void A3Helix::Set ( DArray &mat )
// -----------------------------------------------------------------------------
{
    match = mat;
}

// -----------------------------------------------------------------------------
    int A3Helix::Parts ( void )
// -----------------------------------------------------------------------------
{
    int anz   = match.Elements(),
        parts = anz;

    if (parts)
    {
        parts++;

        if (!((HMatch*)match[0])->first) parts--;
        if (((HMatch*)match[anz - 1])->last == (length - 1)) parts--;
    }
    
    return parts;
}

// -----------------------------------------------------------------------------
    DArray &A3Helix::Helices ( int part,
                               int minlen )
// -----------------------------------------------------------------------------
{
    DArray   *helices = new DArray;
    HMatch   &area    = GetPart(part);
    int       len     = area.last - area.first + 1;
    A3Matrix  mat(len,DARRAY_NOFREE);

    if (!FindHelices(helix,area.first,area.last,mat)) mat.Dump(NULL);
    
    delete &area;
    
    return *helices;
}

// -----------------------------------------------------------------------------
    void A3Helix::Dump ( int all )
// -----------------------------------------------------------------------------
{
    if (all)
    {
        cout << "\nhelix     = "   << helix;
        cout << "\n\nkonsensus = " << konsensus;
        cout << "\n\nlength    = " << length;

        cout << "\n";
    }

    match.Dump(hmatchdump);
}
