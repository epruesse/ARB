#include <cstdio>
#include <cstring>
#include <memory.h>

#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>

#include "arbdb++.hxx"

/***********************************
 ***********************************
        DB MAIN
        ************************************
        ************************************/

AD_MAIN::AD_MAIN() {
    gbd = 0;
    species_data = extended_data = presets = 0;
    AD_fast = MAXCACH;
}

AD_MAIN::~AD_MAIN()
{
    if (gbd)
        new AD_ERR("AD_MAIN: no close or exit !!");
}

/*********************************
open : oeffnet die Datenbank mit dem namen *path
        default: cach abgeschaltet, AD_fast =   MAXCACH;
                 cach an                        MINCACH
***********************************/

AD_ERR *AD_MAIN::open(const char *path) {
    if (AD_fast == MAXCACH) {
        gbd = GB_open(path,"rw");
    } else {
        gbd = GB_open(path,"rwt"); // tiny speichersparend
    }
    if (gbd) {  //  DB geoeffnet
        GB_begin_transaction(gbd); // Zeiger initialisieren
        species_data =
            GB_find(gbd,"species_data",NULL,down_level);
        extended_data =
            GB_find(gbd,"extended_data",NULL,down_level);
        presets =
            GB_find(gbd,"presets",NULL,down_level);
        GB_commit_transaction(gbd);
        gbdataptr = gbd;
        return 0;
    } else
    {
        return new AD_ERR("database doesnt exist");
    }
}

AD_ERR *AD_MAIN::open(const char *path,int cach = MAXCACH)
{
    AD_fast = cach;
    if (AD_fast) {
        gbd = GB_open(path,"rw");
    } else {
        gbd = GB_open(path,"rwt"); // tiny speichersparend
    }
    if (gbd) {  //  DB geoeffnet
        GB_begin_transaction(gbd); // Zeiger initialisieren
        species_data =
            GB_find(gbd,"species_data",NULL,down_level);
        extended_data =
            GB_find(gbd,"extended_data",NULL,down_level);
        presets =
            GB_find(gbd,"presets",NULL,down_level);
        GB_commit_transaction(gbd);
        gbdataptr = gbd;
        return 0;
    } else {
        return new AD_ERR("database doesnt exist");
    }
}

AD_ERR * AD_MAIN::save(const char *modus)
{
    // binary format als Voreinstellung
    char *error;
    if (strncmp("ascii",modus,strlen(modus))) {
        error = (char *)GB_save(gbd,0,"b"); }
    else {
        error = (char *)GB_save(gbd,0,"a");
    }
    if (error)  printf("%s\n",error); //return new AD_ERR(error);
    return 0;
}

AD_ERR * AD_MAIN::save_as(const char *modus)
{
    // binary format als Voreinstellung
    char *error;
    if (strncmp("ascii",modus,strlen(modus))) {
        error = (char *)GB_save_as(gbd,0,"b"); }
    else {
        error = (char *)GB_save_as(gbd,0,"a");
    }
    if (error)  printf("%s\n",error); //return new AD_ERR(error);
    return 0;
}

AD_ERR * AD_MAIN::save_home(const char *modus)
{
    // binary format als Voreinstellung
    char *error;
    if (strncmp("ascii",modus,strlen(modus))) {
        error = (char *)GB_save_in_home(gbd,0,"b"); }
    else {
        error = (char *)GB_save_in_home(gbd,0,"a");
    }
    if (error)  printf("%s\n",error); //return new AD_ERR(error);
    return 0;
}




AD_ERR *AD_MAIN::close()
{
    if (gbd) { GB_exit(gbd); }
    gbd = 0;
    return 0;
}
AD_ERR *AD_MAIN::push_transaction()
{
    char *error = 0;
    error = (char *)GB_push_transaction(gbd);
    if (!error)
        return 0;
    return new AD_ERR(error);

}


AD_ERR *AD_MAIN::pop_transaction()
{
    char *error = 0;
    error = (char *)GB_pop_transaction(gbd);
    if (!error)
        return 0;
    return new AD_ERR(error);
}


AD_ERR *AD_MAIN::begin_transaction()
{
    char *error = 0;
    error = (char *)GB_begin_transaction(gbd);
    if (!error)
        return 0;
    return new AD_ERR(error);

}


AD_ERR *AD_MAIN::commit_transaction()
{
    char *error = 0;
    error = (char *)GB_commit_transaction(gbd);
    if (!error)
        return 0;
    return new AD_ERR(error);
}

AD_ERR * AD_MAIN::abort_transaction()
{
    char* error = 0;
    error = (char *)GB_abort_transaction(gbd);
    if (!error) return 0;
    return new AD_ERR(error);

}

int AD_MAIN::get_cach_flag()
// liefert das Speicherflag zurueck
{
    return AD_fast;
}

int AD_MAIN::time_stamp(void)
{
    return GB_read_clock(species_data);
}

AD_ERR * AD_MAIN::change_security_level(int level) {
    GB_ERROR error;
    char passwd='\n'; // not implemented
    error = GB_change_my_security(gbd,level,&passwd);
    if (error == 0)
        return 0;
    return new AD_ERR(error);
}       

/**************************************

AD_READWRITE

*************************/

char *AD_READWRITE::readstring(char *feld) {
    GBDATA *gbptr = 0;
    GB_TYPES type;
    if (gbdataptr != 0) {
        gbptr = GB_find(gbdataptr,feld,NULL,down_level);
    }
    if (gbptr != 0) {
        type = GB_read_type(gbptr);
        if (type == GB_STRING) {
            return (char *)GB_read_string(gbptr);
        }
                
    }
    return 0;   // falscher type oder eintrag nicht gefunden;

}

int AD_READWRITE::readint(char *feld) {
    GBDATA *gbptr = 0;
    GB_TYPES type;
    if (gbdataptr != 0) {
        gbptr = GB_find(gbdataptr,feld,NULL,down_level);
    }
    if (gbptr != 0) {
        type = GB_read_type(gbptr); 
        if (type == GB_INT ) {
            return  (int)GB_read_int(gbptr);
        }
        new AD_ERR("readint: no int type!");
        return 0;
    }
    return 0;   // falscher type oder eintrag nicht gefunden;

}

float AD_READWRITE::readfloat(char *feld) {
    GBDATA *gbptr = 0;
    GB_TYPES type;
    if (gbdataptr != 0) {
        gbptr = GB_find(gbdataptr,feld,NULL,down_level);
    }
    if (gbptr != 0) {
        type = GB_read_type(gbptr); 
        if (type == GB_FLOAT ) {
            return  (float)GB_read_float(gbptr);
        }
        new AD_ERR("readfloat: no float type!");
        return 0;
    }
    return 0;   // falscher type oder efloatrag nicht gefunden;

}
AD_ERR *AD_READWRITE::writestring(char *feld,char *eintrag) {
    GBDATA *gbptr = 0;
    GB_TYPES type;
    GB_ERROR error;
    if (gbdataptr != 0) {
        gbptr = GB_find(gbdataptr,feld,NULL,down_level);
    }
    if (gbptr != 0) {
        type = GB_read_type(gbptr); 
        if (type == GB_STRING ) {
            error = GB_write_string(gbptr,eintrag);
            if (error == 0) {
                return 0; }
            return new AD_ERR("writestring not possible");
        }
        return new AD_ERR("writestring on non string entry!");
    }
    return new AD_ERR("writestring: feld not existing",CORE);   
}

        
AD_ERR *AD_READWRITE::writeint(char *feld,int eintrag) {
    GBDATA *gbptr = 0;
    GB_TYPES type;
    GB_ERROR error;
    if (gbdataptr != 0) {
        gbptr = GB_find(gbdataptr,feld,NULL,down_level);
    }
    if (gbptr != 0) {
        type = GB_read_type(gbptr); 
        if (type == GB_INT) {
            error = GB_write_int(gbptr,eintrag);
            if (error == 0) {
                return 0; }
            return new AD_ERR("writeint not possible");
        }
        return new AD_ERR("writeint on non string entry!");
        //return new AD_ERR("writeint on non string entry!",AD_ERR_WARNING);
    }
    return new AD_ERR("writeint: feld not existing",CORE);      
}

AD_ERR *AD_READWRITE::writefloat(char *feld,float eintrag) {
    GBDATA *gbptr = 0;
    GB_TYPES type;
    GB_ERROR error;
    if (gbdataptr != 0) {
        gbptr = GB_find(gbdataptr,feld,NULL,down_level);
    }
    if (gbptr != 0) {
        type = GB_read_type(gbptr); 
        if (type == GB_FLOAT) {
            error = GB_write_float(gbptr,eintrag);
            if (error == 0) {
                return 0; }
            return new AD_ERR("writefloat not possible");
        }
        return new AD_ERR("writefloat on non string entry!");
        //return new AD_ERR("writefloat on non string entry!",AD_ERR_WARNING);
    }
    return new AD_ERR("writefloat: feld not existing",CORE);    
}

AD_ERR *AD_READWRITE::create_entry(char *key, AD_TYPES type) {
    GBDATA *newentry = 0;
    if (gbdataptr == 0) {
        return new AD_ERR("AD_READWRITE::create_entry : not inited right");
    }
    // place to check the rights for creation of a new entry
    // not yet implemented
    newentry = GB_create(gbdataptr,key,(GB_TYPES)type);
    // NULL standing for previous GBDATA and is so not used
    if (newentry == 0) {
        return new AD_ERR("AD_READWRITE::create_entry didn't work",CORE);
    }
    return 0;   
}

AD_TYPES AD_READWRITE::read_type(char *key) {
    GBDATA *gbptr =0;
    if (gbdataptr != 0) {
        gbptr = GB_find(gbdataptr,key,NULL,down_level);
    }
    if (gbptr != 0) {
        return (AD_TYPES)GB_read_type(gbptr); 
    }
    return ad_none;
}



/*********************************************
AD_ERR 
*************************/

AD_ERR::~AD_ERR()
{
}


AD_ERR::AD_ERR (const char *pntr)
// setzt den Fehlertext und zeigt ihn an
{
    text = (char *)pntr;

}

AD_ERR::AD_ERR (void )
// setzt den Fehlertext und zeigt ihn an
{
    text = 0;
    printf("%c\n",7);
}

AD_ERR::AD_ERR (const char *pntr, const int core)
// setzt den Fehlertext 
// bricht ab 
// -> besseres Debugging
// wird bei flascher Anwendung der AD_~Klassen verwendet

{
    text = (char *)pntr;
    //cout << "ERROR in ARBDB++: \n" << text << "\n";
    //cout.flush();
    if (core == CORE)
        ADPP_CORE;      // -segmantation Fault
}

char *AD_ERR::show()
{
    return text;
}
