// ============================================================= //
//                                                               //
//   File      : MP_sondentopf.cxx                               //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "MultiProbe.hxx"
#include "MP_probe.hxx"

#include <AP_Tree.hxx>                              // needed for GCs
#include <aw_msg.hxx>

#include <cmath>

ST_Container::ST_Container(int anz_sonden)
    : targetGroup(mp_global->get_marked_species())
{
    Sondennamen = new List<char>;

    if (pt_server_different) return;

    anz_elem_unmarked  = targetGroup.outgroup_species_count();
    anzahl_basissonden = anz_sonden;
    cachehash          = GBS_create_hash(anzahl_basissonden + 1, GB_IGNORE_CASE);
    sondentopf         = new Sondentopf(get_TargetGroup());
}


ST_Container::~ST_Container() {
    char* Sname;
    Sonde* csonde;

    delete sondentopf;

    Sname = Sondennamen->get_first();
    while (Sname) {
        csonde = get_cached_sonde(Sname);
        delete csonde;
        free(Sname);
        Sondennamen->remove_first();
        Sname = Sondennamen->get_first();
    }

    delete Sondennamen;
    GBS_free_hash(cachehash);
}



Sonde* ST_Container::cache_Sonde(char *name, int allowed_mis, double outside_mis, GB_ERROR& error) {
    mp_assert(!error);

    char*  name_for_plist = strdup(name);
    Sonde* s              = new Sonde(name, allowed_mis, outside_mis);

    Sondennamen->insert_as_first(name_for_plist);
    error = s->gen_Hitliste(targetGroup);

    GBS_write_hash(cachehash, name, (long) s);
    // Reine Sonde plus Hitliste geschrieben, der Zeiger auf die Sonde liegt als long gecastet im Hash
    return s;
}

Sonde* ST_Container::get_cached_sonde(char* name) {
    if (name)
        return (Sonde*)GBS_read_hash(cachehash, name);
    else
        return NULL;
}

// ############################################################################################
/*
  Zu jeder Kombination von Mehrfachsonden gehoert ein Sondentopf. Dieser enthaelt eine Liste mit
  Sonden und eine Liste mit Kombinationen aus diesen Sonden. Die Kombinationen entstehen aus den
  Sonden und/oder aus Kombinationen durch Verknuepfung mit der Methode Probe_AND.
*/
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Methoden SONDENTOPF ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Sondentopf::Sondentopf(const TargetGroup& targetGroup_)
    : targetGroup(targetGroup_)
{
    Listenliste = new List<void*>;
    color_hash  = GBS_create_hash(targetGroup.known_species_count()*1.25+1, GB_IGNORE_CASE);
    Listenliste->insert_as_last((void**) new List<Sonde>);
}

Sondentopf::~Sondentopf() {
    // darf nur delete auf die listenliste machen, nicht auf die MO_Lists, da die zu dem ST_Container gehoeren
    Sonde *stmp = NULL;

    List<Sonde> *ltmp = LIST(Listenliste->get_first());
    if (ltmp) {
        stmp = ltmp->get_first();
    }
    while (ltmp) {
        while (stmp) {
            ltmp->remove_first();
            stmp = ltmp->get_first();
        }
        Listenliste->remove_first();
        delete ltmp;
        ltmp = LIST(Listenliste->get_first());
    }

    delete Listenliste;
    GBS_free_hash(color_hash);
}



void Sondentopf::put_Sonde(char *name, int allowed_mis, double outside_mis, GB_ERROR& error) {
    mp_assert(!error);

    positiontype  pos;
    ST_Container *stc         = mp_global->get_stc();
    List<Sonde>  *Sondenliste = LIST(Listenliste->get_first());
    Sonde *s;
    int    i = 0;

    if (!name) {
        aw_message("No name specified for species. Abort."); // @@@
        exit(111);
    }
    if (!Sondenliste) {
        Sondenliste = new List<Sonde>;
        Listenliste->insert_as_last((void**) Sondenliste);
    }

    s = stc->get_cached_sonde(name);
    if (!s) {
        s = stc->cache_Sonde(name, allowed_mis, outside_mis, error);
    }
    pos = Sondenliste->insert_as_last(s);
    if (! s->get_bitkennung())
        s->set_bitkennung(new Bitvector(((int) pos)));
    s->set_far(0);
    s->set_mor(pos);
    s->get_bitkennung()->setbit(pos-1);
    // im cache steht die Mismatch info noch an Stelle 0. Hier muss sie an Stelle pos  verschoben werden
    if (pos!=0)
        for (i=0; i<s->get_length_hitliste(); i++)
            if (s->get_hitdata_by_number(i))
                s->get_hitdata_by_number(i)->set_mismatch_at_pos(pos, s->get_hitdata_by_number(i)->get_mismatch_at_pos(0));
}


double** Sondentopf::gen_Mergefeld() {

    // Zaehler
    int         i, j;

    Sonde*      sonde;

    List<Sonde>* Sondenliste    = LIST(Listenliste->get_first());
    long         alle_bakterien = targetGroup.known_species_count();
    long         H_laenge, sondennummer;
    double**     Mergefeld      = new double*[alle_bakterien+1];

    for (i=0; i<alle_bakterien+1; i++) {
        Mergefeld[i] = new double[mp_gl_awars.no_of_probes];
        for (j=0; j<mp_gl_awars.no_of_probes; j++) Mergefeld[i][j] = 100;
    }

    sondennummer=0;
    sonde = Sondenliste->get_first();
    while (sonde) {
        H_laenge = sonde->get_length_hitliste();
        for (i=0; i<H_laenge; i++) {
            Mergefeld[sonde->get_hitdata_by_number(i)->get_baktid()][sondennummer] = sonde->get_hitdata_by_number(i)->get_mismatch_at_pos(0);
        }

        sondennummer++;
        sonde = Sondenliste->get_next();
    }

    return Mergefeld;
}

probe_tabs* Sondentopf::fill_Stat_Arrays() {
    // erstmal generische Felder
    List<Sonde>* Sondenliste = LIST(Listenliste->get_first());

    mp_assert(Sondenliste);

    long feldlen        = (long) pow(3.0, (int)(mp_gl_awars.no_of_probes));
    int* markierte      = new int[feldlen];                     // MEL
    int* unmarkierte    = new int[feldlen];                     // MEL
    int  i              = 0, j=0;
    long alle_bakterien = targetGroup.known_species_count();
    long wertigkeit     = 0;

    double** Mergefeld;
    int*     AllowedMismatchFeld = new int[mp_gl_awars.no_of_probes];
    Sonde*   sonde;

    sonde = Sondenliste->get_first();

    for (i=0; i<mp_gl_awars.no_of_probes; i++) {
        mp_assert(sonde);
        AllowedMismatchFeld[i] = (int) sonde->get_Allowed_Mismatch_no(0);
        sonde = Sondenliste->get_next();
    }


    for (i=0; i<feldlen; i++) {
        markierte[i] = 0;
        unmarkierte[i] = 0;
    }

    int faktor=0;
    Mergefeld = gen_Mergefeld();


    for (i=0; i < alle_bakterien+1; i++) {
        wertigkeit=0;
        for (j=0; j<mp_gl_awars.no_of_probes; j++) {
            if (Mergefeld[i][j] <= ((double) AllowedMismatchFeld[j] + (double) mp_gl_awars.greyzone)) {
                faktor = 0;
            }
            else if (Mergefeld[i][j] <= ((double) AllowedMismatchFeld[j] +
                                         (double) mp_gl_awars.greyzone +
                                         mp_gl_awars.outside_mismatches_difference)) {
                faktor = 1;
            }
            else {
                faktor = 2;
            }

            wertigkeit += faktor * (long) pow(3, j);
        }

        switch (targetGroup.getMembership(i)) {
            case GM_IN_GROUP:  markierte[wertigkeit]++;   break;
            case GM_OUT_GROUP: unmarkierte[wertigkeit]++; break;
            case GM_UNKNOWN: break;
        }
    }

    for (i=0; i<alle_bakterien+1; i++) {
        delete [] Mergefeld[i];
    }
    delete [] Mergefeld;
    delete [] AllowedMismatchFeld;
    probe_tabs *pt = new probe_tabs(markierte, unmarkierte, feldlen);           // MEL (sollte bei Andrej passieren
    return pt;
}


void Sondentopf::gen_color_hash(positiontype anz_sonden) {
    if (anz_sonden) {
        List<Sonde>* Sondenliste = LIST(Listenliste->get_first());
        long alle_bakterien      = targetGroup.known_species_count();
        int* AllowedMismatchFeld = new int[mp_gl_awars.no_of_probes];

        {
            Sonde* sonde = Sondenliste->get_first();
            for (int i=0; i<mp_gl_awars.no_of_probes; i++) {
                AllowedMismatchFeld[i] = (int) sonde->get_Allowed_Mismatch_no(0);
                sonde = Sondenliste->get_next();
            }
        }

        double** Mergefeld = gen_Mergefeld();

        for (int i=1; i < alle_bakterien+1; i++) {
            int rgb[3] = {0, 0, 0};

            for (int j=0; j<mp_gl_awars.no_of_probes; j++) {
                if (Mergefeld[i][j] <= ((double) AllowedMismatchFeld[j] + (double) mp_gl_awars.greyzone + mp_gl_awars.outside_mismatches_difference)) {
                    rgb[j%3]++;
                }
            }

            int coloridx = bool(rgb[0])+2*bool(rgb[1])+4*bool(rgb[2]);
            mp_assert(coloridx >= 0 && coloridx <= 7);

            static int idx2color[] = {
                AWT_GC_BLACK,   // 0
                AWT_GC_RED,     // 1
                AWT_GC_GREEN,   // 2
                AWT_GC_YELLOW,  // 3
                AWT_GC_BLUE,    // 4
                AWT_GC_MAGENTA, // 5
                AWT_GC_CYAN,    // 6
                AWT_GC_WHITE,   // 7
            };

            GBS_write_hash(color_hash, targetGroup.idx2name(i), idx2color[coloridx]);
        }

        for (int i=0; i<alle_bakterien+1; i++) delete [] Mergefeld[i];
        delete [] Mergefeld;


        delete [] AllowedMismatchFeld;
    }
}

