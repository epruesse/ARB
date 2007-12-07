#include <cstdio>
#include <cstring>
#include <memory.h>

#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>

#include "arbdb++.hxx"

/***********************
***************************************************************
class AD_SPECIES

*******************************************
*******************************************************/

AD_SPECIES::AD_SPECIES()
{
    ad_main =0;
    gb_spdata = 0;
    gb_species = 0;
    gb_name = 0;
    last = 0;
    count = 0;
    container = new CONTLIST;
    spname  = 0;
}

AD_SPECIES::~AD_SPECIES()
// callbacks nur innerhalb von transaktionen releasen ->exit
// gibt speicherplatz frei
{
    if (ad_main)
        new AD_ERR("AD_SPECIES: No exit() !!",CORE);
}

AD_ERR * AD_SPECIES::exit()
{
    if (ad_main) {
        release();
        delete container;
    }
    ad_main = 0;
    return 0;
}

AD_ERR * AD_SPECIES::error()
// interne funktion, die testet ob ein Fehler in der
// anwendung aufgetreten ist
// Funktionen wie next, ... die auf andere spezies initialisieren
// duerfen nicht veraendert werden,solange container daranhaengen
// oder aufgerufen wenn schon das letzte erreicht.
{

    if (count > 0) {
        return new AD_ERR("AD_SPECIES: existing Subobjects ! No change of species allowed\n",CORE);
    }
    if (last == 1)
        return new AD_ERR("AD_SPECIES: already at EOF !",CORE);
    return 0;
}

AD_ERR * AD_SPECIES::init(AD_MAIN * gb_ptr)
// stellt verknuepfung mit ubergeordneter Klasse her
//
// default einstellung moeglich ni ->initpntr()
{
    if (ad_main != 0)   {

        return new AD_ERR("AD_SPECIES: no reinit\n");
    }
    if (gb_ptr->gbd) {
        ad_main = gb_ptr;
        gb_spdata = gb_ptr->species_data;
        last = count = 0;
        AD_READWRITE::gbdataptr = 0;
        return 0;
    }
    else        {
        return new AD_ERR("SPECIES init (NULL)\n");
    }
}

AD_ERR  *AD_SPECIES::first()
{
    //erstes spezies initialisieren

    if (ad_main) {
        gb_species = GB_find(gb_spdata, "species", 0, down_level);
        //erstes species
        AD_SPECIES::initpntr();
        return 0;
    }
    return new AD_ERR("AD_species first: NO AD_MAIN\n");
}


void  AD_SPECIES::initpntr()
// gb_spezies muss initialisiert sein
// je nach AD_fast wird das objekt initialisiert
{
    long a;
    if (!gb_species) {
        last = 1;
        spname  = 0;
        gb_name = 0;
        //kein spezies gefunden
    } else {
        last = 0;
        gb_name = GB_find(gb_species, "name", 0, down_level);
        //name und flag cachen
        spname = GB_read_string(gb_name);
        if (ad_main->AD_fast == MINCACH) {
            a = GB_read_usr_private(gb_species);
            // Wieviele Variablen existieren

            a ++;                               // auf gb_species
            GB_write_usr_private(gb_species,a);
        }
        AD_READWRITE::gbdataptr = gb_species;
        GB_add_callback(gb_species, GB_CB_DELETE, (GB_CB) AD_SPECIES_destroy, (int *) this);
        GB_add_callback(gb_name, GB_CB_CHANGED, (GB_CB) AD_SPECIES_name_change, (int *) this);

        //callback in DB mit routine die beim loeschen der species
        // aufgerufen wird(speicherfreigaben)
    }
}

AD_ERR * AD_SPECIES::find(const char *path)
//   sucht nach species mit namen = path
{
    error();
    AD_SPECIES::release();      // speicherplatz freigeben`
    gb_species = GBT_find_species_rel_species_data(ad_main->species_data,path);
    AD_SPECIES::initpntr();
    return 0;
}


void AD_SPECIES::release()
//  gibt den speicherplatz des objektes wieder frei
//  sowie den in der DatenBank belegten speicherplatz
{
    long a;
    if (count > 0) new AD_ERR("AD_SPECIES: no change of object with subobjects !",CORE);
    if (spname) delete spname;
    if (gb_species)
    {
        GB_remove_callback(gb_species,GB_CB_DELETE,(GB_CB)AD_SPECIES_destroy,(int *)this);
        GB_remove_callback(gb_name,GB_CB_CHANGED,(GB_CB)AD_SPECIES_name_change,(int *)this);
        // altes species wird freigegeben -> callback entfernen

        if (ad_main->get_cach_flag() == MINCACH ) {
            // wenn nur ein speichersparendenr Zugriff besteht
            // Speicerplatz freigeben -> Muliteditoraufrufe
            // eines users
            a = GB_read_usr_private(gb_species);
            if (a == 1) {       // einziger verweis
                // lokalen cache freigeben
                GB_release(gb_species);
            }
            GB_write_usr_private(gb_species,--a);
        }
        AD_READWRITE::gbdataptr = 0;
    }
}



int AD_SPECIES_destroy(GBDATA *gb_species,AD_SPECIES *ad_species)
{
    // Diese Funktion wird nach einem commit transaction aufgerufen, wenn
    // das entsprechende species in der DB geloescht wurde
    //

    if (ad_species->gb_species != gb_species) {
        // muessen uebereinstimmen
        new AD_ERR(" strange CALLBACK occured - int AD_SPECIES",CORE);
    }

    if (ad_species->spname) delete ad_species->spname;
    ad_species->spname = 0;

    ad_species->gb_species = 0;
    ad_species->gbdataptr = 0;

    // hier stehehn weitere comandos die beim loeschen
    // eines species ausgefuehrt werden sollen
    // (z.B.) untergeordnete Klassen

    return 0;
}

int AD_SPECIES_name_change(GBDATA *gb_name,AD_SPECIES *ad_species)
{
    // Diese Funktion wird nach einem commit transaction aufgerufen, wenn
    // das entsprechende species in der DB geloescht wurde
    //
    if (ad_species->spname) delete ad_species->spname;
    ad_species->spname = GB_read_string(        gb_name );      // reread name
    return 0;
}

AD_ERR * AD_SPECIES::create(const char *name) {
    GBDATA *species;
    if (strlen(name) < 2)
        return new AD_ERR("AD_SPECIES::too short name");
    species = GBT_create_species_rel_species_data(gb_spdata,name);
    gb_species = species;
    initpntr();
    return 0;
}


const char * AD_SPECIES::name()
{
    if (AD_SPECIES::gb_species != 0)
        return spname;
    return "deleted";

}

int AD_SPECIES::flag()
{
    if (gb_species != 0)
        return GB_read_flag(gb_species);
    return 0;
}

const char * AD_SPECIES::fullname()
{
    GBDATA *gb_ptr;
    gb_ptr = GB_find(gb_spdata,"full_name",NULL,down_level);
    if (gb_ptr) // fullname existiert
        return GB_read_char_pntr(gb_ptr);
    return 0;
}


AD_ERR * AD_SPECIES::next()
// initialisiert objekt auf naechstes species oder erstes
{
    error();

    if (!gb_species && last == 0)
        first();
    AD_SPECIES::release();      // speicherplatz freigeben
    gb_species = GB_find(gb_species,"species",0,this_level | search_next);
    AD_SPECIES::initpntr();
    return 0;
}


AD_ERR *AD_SPECIES::firstmarked()
{
    if (ad_main )
    {
        gb_species = GBT_first_marked_species_rel_species_data(gb_spdata);
        initpntr();
        return 0;
    }
    return new AD_ERR("AD_SPECIES::firstmarked() but no init()!",CORE);
}

AD_ERR * AD_SPECIES::nextmarked()
// naechstes markiertes species oder erstes markiertes
{
    if ((!gb_species && (last ==0)))
    {
        gb_species = GBT_first_marked_species_rel_species_data(gb_spdata);
        initpntr();
        return 0;
    }
    else {
        release();
        gb_species = GBT_next_marked_species(gb_species);
        initpntr();
        return 0;
    }
}



int AD_SPECIES::eof()
{
    return last;
}


void AD_SPECIES::operator =(const AD_SPECIES& right)
{
    release();          // free left side
    ad_main = right.ad_main;
    gb_spdata = right.gb_spdata;
    gb_species = right.gb_species;
    this->initpntr();
}

int AD_SPECIES::time_stamp(void)
{
    if (gb_species != 0)
        return GB_read_clock(gb_species);
    new AD_ERR("AD_SPECIES::time_stamp - no species selected");
    return 0;
}




        
