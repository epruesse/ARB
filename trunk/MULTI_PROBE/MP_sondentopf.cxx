#include <MultiProbe.hxx>
#include <stdlib.h>
#include <string.h>
#include <arbdbt.h>
#include <awt_tree.hxx>
#include <math.h>

//############################################################################################
/*
 */
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Methoden ST_Container ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ST_Container::ST_Container(int anz_sonden)      // ist ein define das auf 1000 steht. (wir wissen, das das nicht ganz sauber ist so!)
{
    long    laenge_markierte;

    Sondennamen = new List<char>;

    Auswahlliste = new MO_Liste();
    Bakterienliste = new MO_Liste();

    Bakterienliste->get_all_species();

    if (pt_server_different) return;
    laenge_markierte = Auswahlliste->fill_marked_bakts();

    anz_elem_unmarked = Bakterienliste->debug_get_current()-1 - laenge_markierte;
    // STATISTIK

    anzahl_basissonden =  anz_sonden;

    cachehash = GBS_create_hash(anzahl_basissonden + 1,1);
    // hashlaenge darf nicht 0 sein, groesser schadet nicht

    sondentopf = new Sondentopf(Bakterienliste,Auswahlliste);
    // Momentan wird auf diesem sondentopf gearbeitet
}


ST_Container::~ST_Container()
{
    char* Sname;
    Sonde* csonde;

    delete Bakterienliste;
    delete Auswahlliste;
    delete sondentopf;

    Sname = Sondennamen->get_first();
    while (Sname)
    {
        csonde = get_cached_sonde(Sname);
        delete csonde;
        free(Sname);
        Sondennamen->remove_first();
        Sname = Sondennamen->get_first();
    }

    delete Sondennamen;
    GBS_free_hash(cachehash);
}



Sonde* ST_Container::cache_Sonde(char *name, int allowed_mis, double outside_mis)
{
    long    hashreturnval;
    char*   name_for_plist = strdup(name);
    //char* hashname = name;
    Sonde* s = new Sonde(name, allowed_mis, outside_mis);

    Sondennamen->insert_as_first(name_for_plist);
    s->gen_Hitliste(Bakterienliste);

    hashreturnval = GBS_write_hash(cachehash, name, (long) s);
    // Reine Sonde plus Hitliste geschrieben, der Zeiger auf die Sonde liegt als long gecastet im Hash
    return s;
}

Sonde* ST_Container::get_cached_sonde(char* name)
{
    if (name)
        return (Sonde*)GBS_read_hash(cachehash,name);
    else
        return NULL;
}

//############################################################################################
/*
  Zu jeder Kombination von Mehrfachsonden gehoert ein Sondentopf. Dieser enthaelt eine Liste mit
  Sonden und eine Liste mit Kombinationen aus diesen Sonden. Die Kombinationen entstehen aus den
  Sonden und/oder aus Kombinationen durch Verknuepfung mit der Methode Probe_AND.
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Methoden SONDENTOPF ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Sondentopf::Sondentopf(MO_Liste *BL, MO_Liste *AL)
{
    Listenliste = new List<void*>;
    color_hash  = GBS_create_hash((long)(BL->get_laenge()/0.8)+1,1);
    if (!BL)
    {
        aw_message("List of species is empty. Terminating program");
        exit(333);
    }
    if (!AL)
    {
        aw_message("List of marked species is empty. Terminating program");
        exit(334);
    }
    BaktList = BL;
    Auswahllist = AL;
    Listenliste->insert_as_last((void**) new List<Sonde>);

}



Sondentopf::~Sondentopf()
{
    //darf nur delete auf die listenliste machen, nicht auf die MO_Lists, da die zu dem ST_Container gehoeren
    Sonde *stmp = NULL;

    List<Sonde> *ltmp = LIST(Listenliste->get_first());
    if (ltmp)
    {
        stmp = ltmp->get_first();
    }
    while (ltmp)
    {
        while (stmp)
        {
            ltmp->remove_first();
            //delete stmp;
            stmp = ltmp->get_first();
        }
        Listenliste->remove_first();
        delete ltmp;
        ltmp = LIST(Listenliste->get_first());
    }

    delete Listenliste;
    //delete BaktList;
    //delete Auswahllist;
    GBS_free_hash(color_hash);
}



void Sondentopf::put_Sonde(char *name, int allowed_mis, double outside_mis)
{
    positiontype    pos;
    ST_Container    *stc = mp_main->get_stc();
    List<Sonde>     *Sondenliste = LIST(Listenliste->get_first());
    Sonde       *s;
    int i=0;

    if (!name)
    {
        aw_message("No name specified for species. Abort.");
        exit(111);
    }
    if (!Sondenliste)
    {
        Sondenliste = new List<Sonde>;
        Listenliste->insert_as_last((void**) Sondenliste );
    }

    s = stc->get_cached_sonde(name);
    if (!s)
    {
        s = stc->cache_Sonde(name, allowed_mis, outside_mis);
    }
    pos = Sondenliste->insert_as_last( s );
    if (! s->get_bitkennung())
        s->set_bitkennung(new Bitvector( ((int) pos) ) );
    s->set_far(0);
    s->set_mor(pos);
    s->get_bitkennung()->setbit(pos-1);
    // im cache steht die Mismatch info noch an Stelle 0. Hier muss sie an Stelle pos  verschoben werden
    if (pos!=0)
        for (i=0; i<s->get_length_hitliste(); i++)
            if (s->get_hitdata_by_number(i))
                s->get_hitdata_by_number(i)->set_mismatch_at_pos( pos,s->get_hitdata_by_number(i)->get_mismatch_at_pos(0) );
}


double** Sondentopf::gen_Mergefeld()
{

    // Zaehler
    int         i,j;

    Sonde*      sonde;

    List<Sonde>*    Sondenliste = LIST(Listenliste->get_first());
    long        alle_bakterien = BaktList->debug_get_current()-1;
    long        H_laenge, sondennummer;
    double**        Mergefeld = new double*[alle_bakterien+1];

    for (i=0; i<alle_bakterien+1; i++)
    {
        Mergefeld[i] = new double[mp_gl_awars.no_of_probes];
        for(j=0; j<mp_gl_awars.no_of_probes; j++)
            Mergefeld[i][j] = 100;
    }

    sondennummer=0;
    sonde = Sondenliste->get_first();
    while (sonde)
    {
        H_laenge = sonde->get_length_hitliste();
        for (i=0; i<H_laenge; i++)
        {
            Mergefeld[sonde->get_hitdata_by_number(i)->get_baktid()][sondennummer] =
                sonde->get_hitdata_by_number(i)->get_mismatch_at_pos(0) ;
        }

        sondennummer++;
        sonde = Sondenliste->get_next();
    }

    return Mergefeld;
}

probe_tabs* Sondentopf::fill_Stat_Arrays()
{
    // erstmal generische Felder
    List<Sonde>*    Sondenliste = LIST(Listenliste->get_first());
    long        feldlen = (long) pow(3.0,(int)(mp_gl_awars.no_of_probes));
    int*        markierte = new int[feldlen];                   //MEL
    int*        unmarkierte = new int[feldlen];                 //MEL
    int     i=0,j=0;
    long        alle_bakterien = BaktList->debug_get_current()-1;
    long        wertigkeit = 0;
    double**    Mergefeld;
    int*        AllowedMismatchFeld = new int[mp_gl_awars.no_of_probes];
    Sonde*      sonde;

    sonde = Sondenliste->get_first();
    for (i=0; i<mp_gl_awars.no_of_probes; i++)
    {
        AllowedMismatchFeld[i] = (int) sonde->get_Allowed_Mismatch_no(0);
        sonde = Sondenliste->get_next();
    }


    for (i=0; i<feldlen; i++)
    {
        markierte[i] = 0;
        unmarkierte[i] = 0;
    }

    int faktor=0;
    Mergefeld = gen_Mergefeld();


    for (i=0; i < alle_bakterien+1; i++)
    {
        wertigkeit=0;
        for (j=0; j<mp_gl_awars.no_of_probes; j++)
        {
            if ( Mergefeld[i][j] <= ((double) AllowedMismatchFeld[j] + (double) mp_gl_awars.greyzone ) )
                faktor = 0;
            else if ( Mergefeld[i][j] <= ( (double) AllowedMismatchFeld[j] +
                                           (double) mp_gl_awars.greyzone +
                                           mp_gl_awars.outside_mismatches_difference ) )
                faktor = 1;
            else
                faktor = 2;

            wertigkeit += faktor * (long) pow(3,j);
        }


        if (BaktList->get_entry_by_index(i))
        {
            if (  Auswahllist->get_index_by_entry( BaktList->get_entry_by_index(i) )  )
            {
                markierte[wertigkeit]++;
            }
            else
            {
                unmarkierte[wertigkeit]++;
            }
        }
    }

    for (i=0;i<alle_bakterien+1;i++) {
        delete [] Mergefeld[i];
    }
    delete [] Mergefeld;
    delete [] AllowedMismatchFeld;
    probe_tabs *pt = new probe_tabs(markierte, unmarkierte,feldlen);            //MEL (sollte bei Andrej passieren
    return pt;
}


void Sondentopf::gen_color_hash(positiontype anz_sonden)
{

    List<Sonde>*    Sondenliste = LIST(Listenliste->get_first());
    double**        Mergefeld;
    long        alle_bakterien = BaktList->debug_get_current() -1;
    int*        AllowedMismatchFeld = new int[mp_gl_awars.no_of_probes];            //MEL
    int*        rgb = new int[3];                               //MEL
    Sonde*      sonde;
    int         i=0, j=0;
    int         marker=0;
    int         r=0,g=0,b=0,y=0,w=0,l=0,n=0;
    int         check;

    sonde = Sondenliste->get_first();
    for (i=0; i<mp_gl_awars.no_of_probes; i++)
    {
        AllowedMismatchFeld[i] = (int) sonde->get_Allowed_Mismatch_no(0);
        sonde = Sondenliste->get_next();
    }


    if (!anz_sonden)
        return;


    Mergefeld = gen_Mergefeld();


    for (i=1; i < alle_bakterien+1; i++)
    {
        rgb[0]=0; rgb[1]=0; rgb[2]=0;

        for (j=0; j<mp_gl_awars.no_of_probes; j++)
        {
            check=0;
            if (Mergefeld[i][j] <= ((double) AllowedMismatchFeld[j] + (double) mp_gl_awars.greyzone + mp_gl_awars.outside_mismatches_difference)  )
            {
                rgb[ j%3 ]++;
                check++;
            }
            if (check)
                marker++;
        }

        int codenum = 0;
        int color = 0;

        if (!rgb[0] && !rgb[1] && !rgb[2])
        {
            codenum = 0;
        }
        else if (rgb[0] && !rgb[1] && !rgb[2])
        {
            codenum =  1;
            if (Auswahllist->get_entry_by_index(i))
                r++;
        }
        else if (!rgb[0] && rgb[1] && !rgb[2])
        {
            codenum =  2;
            if (Auswahllist->get_entry_by_index(i))
                g++;
        }
        else if (!rgb[0] && !rgb[1] && rgb[2])
        {
            codenum =  3;
            if (Auswahllist->get_entry_by_index(i))
                b++;
        }
        else if (rgb[0] && rgb[1] && !rgb[2])
        {
            codenum =  4;
            if (Auswahllist->get_entry_by_index(i))
                y++;
        }
        else if (rgb[0] && !rgb[1] && rgb[2])
        {
            codenum =  5;
            if (Auswahllist->get_entry_by_index(i))
                w++;
        }
        else if (!rgb[0] && rgb[1] && rgb[2])
        {
            codenum =  6;
            if (Auswahllist->get_entry_by_index(i))
                l++;
        }
        else if (rgb[0] && rgb[1] && rgb[2])
        {
            codenum =  7;
            if (Auswahllist->get_entry_by_index(i))
                n++;
        }
        else
            aw_message("Error in color assignment");

        switch ( codenum ) {
            case 0: color = AWT_GC_BLACK;   break;
            case 1: color = AWT_GC_RED;     break;
            case 2: color = AWT_GC_GREEN;   break;
            case 3: color = AWT_GC_BLUE;    break;
            case 4: color = AWT_GC_YELLOW;  break;
            case 5: color = AWT_GC_MAGENTA; break; // was NAVY
            case 6: color = AWT_GC_CYAN;    break; // was LIGHTBLUE
            case 7: color = AWT_GC_WHITE ;  break;
            default:   aw_message("Error in color assignment");
        }

        GBS_write_hash(color_hash, BaktList->get_entry_by_index(i) , (long) color);

    }

    for (i=0;i<alle_bakterien+1;i++)
        delete [] Mergefeld[i];
    delete [] Mergefeld;


    delete [] AllowedMismatchFeld;
    delete [] rgb;
}


