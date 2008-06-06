#include <stdio.h>
#include <arbdb.h>
#include "arbdb++.hxx"

CLISTENTRY::CLISTENTRY() {
    entry  = 0;
    entry2 = 0;
    next   = 0;
}

CONTLIST::CONTLIST() {
    first  = 0;
    anzahl = 0;
}

CONTLIST::~CONTLIST() {
}


int CONTLIST::insert(AD_SPECIES *ad_species,AD_ALI *ad_ali)
{
    if (element(ad_species,ad_ali)) return 0;
    
    CLISTENTRY *newentry;
    newentry         = new CLISTENTRY;
    newentry->entry  = ad_species;
    newentry->entry2 = ad_ali;
    newentry->next   = first;

    first = newentry;
    anzahl ++;
    return 1;
}

int CONTLIST::element(AD_SPECIES *ad_species,AD_ALI *ad_ali)
{
    // testet ob ad_ptr in CONTLIST vorhanden
    CLISTENTRY *a;
    if (anzahl == 0) { return 0; }
    a = first;
    do {
        if (a->entry ==ad_species && a->entry2 ==ad_ali) return 1;
        a = a->next;
    } while ( a != 0);
    return 0;       // nicht element der liste
}



void CONTLIST::remove(AD_SPECIES *ad_species,AD_ALI *ad_ali)
{
    CLISTENTRY* a,*b;
    if (element(ad_species,ad_ali)) {
        if (first->entry ==ad_species && first->entry2 ==ad_ali) {
            first = first->next;
            delete first;
        }
        else {
            a = first;
            b = a->next;
            while (!(b->entry ==ad_species && b->entry2 ==ad_ali)) {
                a = b;
                b = b->next;
            }
            a->next = b->next;
            delete b;
        }
        anzahl --;
    }
}
