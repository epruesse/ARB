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

static void delete_cached_probe(long hashval) {
    Probe *s = (Probe*)hashval;
    delete s;
}

ProbeCache::ProbeCache(size_t probeCount)
    : targeted(mp_global->get_marked_species())
{
    if (pt_server_different) return;

    cachehash = GBS_create_dynaval_hash(probeCount + 1, GB_IGNORE_CASE, delete_cached_probe);
}

Probe* ProbeCache::add(char *name, int allowed_mis, double outside_mis, GB_ERROR& error) {
    mp_assert(!error);

    Probe* s = new Probe(name, allowed_mis, outside_mis);
    error    = s->gen_Hitliste(targeted);
    
    GBS_write_hash(cachehash, name, (long) s);

    return s;
}

MultiProbeCombinations::MultiProbeCombinations(const TargetGroup& targetGroup_)
    : targetGroup(targetGroup_),
      color_hash(NULL)
{
    probeLists = new List<ProbeList>;
    probeLists->insert_as_last(new List<Probe>);
}

MultiProbeCombinations::~MultiProbeCombinations() {
    // darf nur delete auf die listenliste machen, nicht auf die MO_Lists, da die zu dem ProbeCache gehoeren
    Probe *stmp = NULL;

    List<Probe> *ltmp = probeLists->get_first();
    if (ltmp) {
        stmp = ltmp->get_first();
    }
    while (ltmp) {
        while (stmp) {
            ltmp->remove_first();
            stmp = ltmp->get_first();
        }
        probeLists->remove_first();
        delete ltmp;
        ltmp = probeLists->get_first();
    }

    delete probeLists;
    if (color_hash) GBS_free_hash(color_hash);
}



void MultiProbeCombinations::add_probe(char *name, int allowed_mis, double outside_mis, GB_ERROR& error) {
    mp_assert(!error);
    mp_assert(name);

    List<Probe> *probe_list = probeLists->get_first();
    if (!probe_list) {
        probe_list = new List<Probe>;
        probeLists->insert_as_last(probe_list);
    }

    ProbeCache *cachedProbes = mp_global->get_probe_cache();
    Probe      *probe        = cachedProbes->get(name);
    if (!probe) probe        = cachedProbes->add(name, allowed_mis, outside_mis, error);

    positiontype pos = probe_list->insert_as_last(probe);
    if (!probe->get_bitkennung()) {
        probe->set_bitkennung(new Bitvector(((int) pos)));
    }
    probe->set_far(0);
    probe->set_mor(pos);
    probe->get_bitkennung()->setbit(pos-1);
    // im cache steht die Mismatch info noch an Stelle 0. Hier muss sie an Stelle pos  verschoben werden
    if (pos!=0) {
        for (int i=0; i<probe->get_length_hitliste(); i++) {
            Hit *hit = probe->hit(i);
            if (hit) {
                hit->set_mismatch_at_pos(pos, hit->get_mismatch_at_pos(0));
            }
        }
    }
}


double** MultiProbeCombinations::gen_Mergefeld() {
    List<Probe>* probes       = probeLists->get_first();   // @@@ rename
    long         speciesCount = targetGroup.known_species_count();
    double**     Mergefeld    = new double*[speciesCount+1];

    for (int i=0; i <= speciesCount; i++) {
        Mergefeld[i] = new double[mp_gl_awars.no_of_probes];
        for (int j=0; j<mp_gl_awars.no_of_probes; j++) Mergefeld[i][j] = 100;
    }

    long   probeIdx = 0;
    Probe *probe    = probes->get_first();
    while (probe) {
        long H_laenge = probe->get_length_hitliste();
        for (int i=0; i<H_laenge; i++) {
            Hit       *hit     = probe->hit(i);
            SpeciesID  species = hit->target();

            Mergefeld[species][probeIdx] = hit->get_mismatch_at_pos(0);
        }

        probeIdx++;
        probe = probes->get_next();
    }

    return Mergefeld;
}

probe_tabs* MultiProbeCombinations::fill_Stat_Arrays() {
    // erstmal generische Felder
    List<Probe> *probes = probeLists->get_first();
    mp_assert(probes);

    long feldlen     = (long) pow(3.0, (int)(mp_gl_awars.no_of_probes));
    int* markierte   = new int[feldlen];                        // MEL
    int* unmarkierte = new int[feldlen];                        // MEL

    long speciesCount = targetGroup.known_species_count();
    long wertigkeit   = 0;

    int* AllowedMismatchFeld = new int[mp_gl_awars.no_of_probes];

    Probe *sonde = probes->get_first();

    for (int i=0; i<mp_gl_awars.no_of_probes; i++) {
        mp_assert(sonde);
        AllowedMismatchFeld[i] = (int) sonde->get_Allowed_Mismatch_no(0);
        sonde = probes->get_next();
    }

    for (int i=0; i<feldlen; i++) {
        markierte[i] = 0;
        unmarkierte[i] = 0;
    }

    int      faktor    = 0;
    double** Mergefeld = gen_Mergefeld();

    for (SpeciesID id(0); id < speciesCount+1; ++id) {
        wertigkeit=0;
        for (int j=0; j<mp_gl_awars.no_of_probes; j++) {
            if (Mergefeld[id][j] <= ((double) AllowedMismatchFeld[j] + (double) mp_gl_awars.greyzone)) {
                faktor = 0;
            }
            else if (Mergefeld[id][j] <= ((double) AllowedMismatchFeld[j] +
                                          (double) mp_gl_awars.greyzone +
                                          mp_gl_awars.outside_mismatches_difference)) {
                faktor = 1;
            }
            else {
                faktor = 2;
            }

            wertigkeit += faktor * (long) pow(3, j);
        }

        switch (targetGroup.getMembership(id)) {
            case GM_IN_GROUP:  markierte[wertigkeit]++;   break;
            case GM_OUT_GROUP: unmarkierte[wertigkeit]++; break;
            case GM_UNKNOWN: break;
        }
    }

    for (int i=0; i<speciesCount+1; i++) {
        delete [] Mergefeld[i];
    }
    delete [] Mergefeld;
    delete [] AllowedMismatchFeld;
    probe_tabs *pt = new probe_tabs(markierte, unmarkierte, feldlen);           // MEL (sollte bei Andrej passieren
    return pt;
}


void MultiProbeCombinations::gen_color_hash() {
    if (mp_gl_awars.no_of_probes) {
        color_hash = GBS_create_hash(targetGroup.known_species_count()*1.25+1, GB_IGNORE_CASE);

        List<Probe>* Sondenliste  = probeLists->get_first();
        long         speciesCount = targetGroup.known_species_count();

        int* AllowedMismatchFeld = new int[mp_gl_awars.no_of_probes];

        {
            Probe* sonde = Sondenliste->get_first();
            for (int i=0; i<mp_gl_awars.no_of_probes; i++) {
                AllowedMismatchFeld[i] = (int) sonde->get_Allowed_Mismatch_no(0);
                sonde = Sondenliste->get_next();
            }
        }

        double** Mergefeld = gen_Mergefeld();

        for (SpeciesID i(1); i < speciesCount+1; ++i) {
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

            GBS_write_hash(color_hash, targetGroup.id2name(i), idx2color[coloridx]);
        }

        for (int i=0; i<speciesCount+1; i++) delete [] Mergefeld[i];
        delete [] Mergefeld;


        delete [] AllowedMismatchFeld;
    }
}

