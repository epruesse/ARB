#include <cstdio>
#include <cstring>
#include <memory.h>

#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>

#include "arbdb++.hxx"

// --------------------------------------------------------------------------------
// class: AD_ALI

AD_ALI::AD_ALI()
{
    gb_ali = gb_aligned = gb_name = gb_len = gb_type = 0;
    ad_main = 0;
    last = count = 0;
    ad_name = ad_type = 0;
    ad_aligned = ad_len = 0;
}

AD_ALI::~AD_ALI()
{
    if (ad_main) new AD_ERR("AD_ALI: no exit !",CORE);
}



AD_ERR *AD_ALI::init(AD_MAIN * gbptr)
{
    if (!gbptr) {
        return new AD_ERR("AD_ALI.init(NULL)",CORE);
    } else {
        ad_main = gbptr;
        ddefault();
        return 0;
    }
}

AD_ERR *AD_ALI::exit()
{
    if (ad_main) {
        ad_main = 0;
        return 0;
    }
    return new AD_ERR("AD_ALI: exit() without init()");
}

AD_ERR *AD_ALI::ddefault()
{
    AD_ERR *res;
    char *def = GBT_get_default_alignment(ad_main->gbd);
    res =find(def);
    delete def;
    return res;
}

AD_ERR *AD_ALI::initpntr()
{
    // initialisiert das objekt
    // gb_ali sollte veraendert werden, dann aufrufen
    if (!gb_ali) {
        last = 1;
        gb_ali = gb_aligned = gb_name = gb_len = gb_type = 0;
        return 0;
    }
    last = 0;
    gb_name = GB_find(gb_ali,"alignment_name",NULL,down_level);
    gb_aligned = GB_find(gb_ali,"aligned",NULL,down_level);
    gb_len =  GB_find(gb_ali,"alignment_len",NULL,down_level);
    gb_type = GB_find(gb_ali,"alignment_type",NULL,down_level);
    ad_name = GB_read_string(gb_name);
    ad_type = GB_read_string(gb_type);
    ad_len = GB_read_int(gb_len);
    ad_aligned = GB_read_int(gb_aligned);
    AD_READWRITE::gbdataptr = gb_ali;
    return 0;
}

AD_ERR *AD_ALI::release() {
    if (ad_name) delete ad_name;
    if (ad_type) delete ad_type;
    AD_READWRITE::gbdataptr = 0;
    ad_name = 0; ad_type = 0;
    return 0;
}


AD_ERR *AD_ALI::first()
{
    // initialisiert das Objekt mit dem ersten gefundenen
    // Alignment, eof falls keins existiert
    release();
    gb_ali = GB_find(ad_main->gbd,"alignment",NULL,down_level);
    initpntr();
    return 0;
}


AD_ERR *AD_ALI::find(char* name)
{
    // sucht nach name, eof wenn nicht gefunden !
    //
    release();
    gb_ali = GBT_get_alignment(ad_main->gbd,  name);
    initpntr();
    return 0;
}

AD_ERR *AD_ALI::next()
// initialisiert auf das naechste, bzw. erste gefundene Objekt
// falss kein weiteres liefert es einen Fehler zurueck
// und setzt last-flag
{
    GBDATA *gbptr;
    if (!gb_ali) {
        AD_ALI::first();
        return 0;
    } else {
        release();
        gbptr = GB_find(gb_ali,"alignment",0,this_level | search_next);
        gb_ali = gbptr;
        initpntr();
    }
    return 0;
}

int AD_ALI::time_stamp(void)
{
    if (gb_ali != 0)
        return GB_read_clock(gb_ali);
    new AD_ERR("AD_ALI::time_stamp - no alignment selected");
    return 0;
}



int AD_ALI::eof()
{
    return last;
}

int AD_ALI::aligned()
{
    return ad_aligned;
}

int AD_ALI::len()
{
    return ad_len;
}

char *AD_ALI::type()
{
    // Achtung: kurzlebiger speicher !! ansonsten read_string !!
    return ad_type;
}

char *AD_ALI::name()
{
    return ad_name;
}


void AD_ALI::operator=(AD_ALI& ali)
{
    ad_main = ali.ad_main;
    gb_ali = ali.gb_ali;
    gb_aligned = ali.gb_aligned;
    gb_name = ali.gb_name;
    gb_len = ali.gb_len;
    gb_type = ali.gb_type;
    count = 0;              // keine contianer haengen am objekt
    last = ali.last;
    if (ali.ad_name) ad_name = strdup(ali.ad_name);
    else    ad_name = 0;
    if (ali.ad_type) ad_type = strdup(ali.ad_type);
    else    ad_type = 0;
}

