#include <cstdio>
#include <cstring>
#include <memory.h>

#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>

#include "arbdb++.hxx"

// --------------------------------------------------------------------------------
// class AD_SAI

AD_SAI::AD_SAI()
{

    gb_main = 0;
}

AD_SAI::~AD_SAI()
// callbacks nur innerhalb von transaktionen releasen ->exit
// gibt speicherplatz frei
{
    if (ad_main)
        new AD_ERR("AD_SAI: No exit() !!",CORE);
}

AD_ERR * AD_SAI::exit()
{
    if (ad_main) release();
    delete container;
    ad_main = 0;
    return 0;
}


AD_ERR * AD_SAI::init(AD_MAIN * gb_ptr)
// stellt verknuepfung mit ubergeordneter Klasse her
{
    if (ad_main != 0)       {
        return new AD_ERR("AD_SAI: no reinit\n");
    }
    if (gb_ptr->gbd) {
        ad_main = gb_ptr;
        gb_main = gb_ptr->gbd;
        last = count = 0;
        AD_READWRITE::gbdataptr = 0;
        return 0;
    }
    else    {
        return new AD_ERR("SAI init (NULL)\n");
    }
}

AD_ERR  *AD_SAI::first()
{
    if (ad_main) {
        AD_SAI::release();      // speicherplatz freigeben
        gb_species = GBT_first_SAI(gb_main);
        //erstes extended
        AD_SAI::initpntr();
        return 0;
    }
    return new AD_ERR("AD_extended first: NO AD_MAIN\n");
}



AD_ERR * AD_SAI::find(char *path)
//       sucht nach extended mit namen = path
{
    error();
    AD_SPECIES::release();  // speicherplatz freigeben`
    gb_species = GBT_find_SAI_rel_exdata(ad_main->extended_data,path);
    AD_SPECIES::initpntr();
    return 0;
}




AD_ERR * AD_SAI::create(char *name) {
    GBDATA *extended;
    if (strlen(name) < 2)
        return new AD_ERR("AD_SAI::create ungueltige Parameter");
    extended = GBT_create_SAI(gb_main,name);
    gb_species = extended;
    initpntr();
    return 0;
}



char * AD_SAI::fullname()
{
    return 0;
}

AD_ERR * AD_SAI::next()
// initialisiert objekt auf naechstes extended oder erstes
{
    error();

    if (!gb_species && last == 0)
        first();
    AD_SAI::release();      // speicherplatz freigeben
    gb_species = GBT_next_SAI(gb_species);
    AD_SAI::initpntr();
    return 0;
}

/******** not yet in GBT
          AD_ERR *AD_SAI::firstmarked()
          {
          if (ad_main )
          {
          gb_extended = GBT_first_marked_extended(gb_exdata);
          initpntr();
          return 0;
          }
          return new AD_ERR("AD_SAI::firstmarked() but no init()!",CORE);
          }

          AD_ERR * AD_SAI::nextmarked()
          // naechstes markiertes extended oder erstes markiertes
          {
          if ((!gb_extended && (last ==0)))
          {
          gb_extended = GBT_first_marked_extended(gb_exdata);
          initpntr();
          return 0;
          }
          else {
          release();
          gb_extended = GBT_next_marked_extended(gb_extended);
          initpntr();
          return 0;
          }
          }
************/


void AD_SAI::operator =(const AD_SAI& right)
{
    gb_main = right.gb_main;
    (AD_SPECIES &) *this = (AD_SPECIES&) right;
}




