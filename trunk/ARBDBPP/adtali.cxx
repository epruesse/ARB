#include <stdio.h>
#include <stdlib.h>
// #include <malloc.h>
#include <arbdb.h>
#include "adtools.hxx"

ADT_ALI::ADT_ALI() {
    gapsequence = 0;
    gapshowoffset = 0;
    gaprealoffset = 0;
    inited = 0;
}

ADT_ALI::~ADT_ALI() {
}

void ADT_ALI::init(AD_MAIN *ad_maini)
{
    AD_ALI::init(ad_maini);
    int ali_len = (int)(AD_ALI::len());
    if (ali_len<0) ali_len = 0;
    gapsequence = (char *) malloc(ali_len+1);               // gap_init
    gapshowoffset = (int *) calloc(ali_len,sizeof(int));
    gaprealoffset = (int *) calloc(ali_len,sizeof(int));
    // testversion leerer gap
    for (int i = 0; i < ali_len; i++) {
        gapsequence[i] = NOGAP_SYMBOL;
        gapshowoffset[i] = 0;
        gaprealoffset[i] = 0;
    }
    gapshowoffset_len = ali_len;
    inited = 1;
}




// Funktionen auf gapsequence
AD_ERR *  ADT_ALI::gap_make(int position,int length)    {
    int offset_sum = 1;
    int i,counter = 0;
    if (!((position+length > AD_ALI::len()) || gap_inside(position,position + length))) {
        for (i = position;i<position +length;i++) {     // gapsequence
            gapsequence[i]  = GAP_SYMBOL;
        }
        if (position == 0) {            // luecke am anfang
            offset_sum = -length-1;
        }
        for (i = 0; i<AD_ALI::len(); i++) {             // gaprealoffset
            if (gapsequence[i] == GAP_SYMBOL) {
                offset_sum ++;  }
            gaprealoffset[i] = offset_sum;
        }

        offset_sum = 0;                                 // gapshowoffset
        for (i = 0;i<AD_ALI::len() ; i++) {
            if (gapsequence[i] != GAP_SYMBOL) {
                gapshowoffset[counter] = offset_sum;
                counter ++;
            } else {
                offset_sum ++;
            }
        }
        gapshowoffset_len = counter-1;
        printf("gap maked % d of len %d\n",position,length);
        return 0;
    } else {
        return new AD_ERR("ADT_ALI::gap_make(int position,int length) ungueltige parameter");
    }
}


AD_ERR  * ADT_ALI::gap_delete(int showposition) {
    // showposition zeigt auf die position vor dem gap
    int i,counter=0,offset_sum = 0;
    int realpos = gap_realpos(showposition);
    int startpos = realpos + 1;
    int endpos = gap_realpos(showposition+1);
    for (i=startpos;i<endpos;i++) {
        gapsequence[i] = NOGAP_SYMBOL;
    }
    for (i = 0; i<AD_ALI::len(); i++) {             // gaprealoffset
        if (gapsequence[i] == GAP_SYMBOL) {
            offset_sum ++;  }
        gaprealoffset[i] = offset_sum;
    }

    offset_sum = 0;                                 // gapshowoffset
    for (i = 0;i<AD_ALI::len() ; i++) {
        if (gapsequence[i] != GAP_SYMBOL) {
            gapshowoffset[counter] = offset_sum;
            counter ++;
        } else {
            offset_sum ++;
        }
    }

    gapshowoffset_len = gapshowoffset_len + endpos-startpos;
    return 0;
}

AD_ERR * ADT_ALI::gap_update(char *oldseq,char *newseq) {
    oldseq=oldseq;newseq=newseq;
    // realsequenz wurde in DAtenbank geaendert -> gap sequenz muss erneuert werden
    return 0;
}

char * ADT_ALI::gap_make_real_sequence(char *realseq,const char *showseq) {
    realseq=realseq;showseq=showseq;
    return 0;
}

char * ADT_ALI::gap_make_show_sequence(const char *realseq,char *showseq) {
    realseq=realseq;showseq=showseq;
    return 0;
}

int ADT_ALI::gap_inside(int realposition) {
    GBUSE(realposition);
    return 0;
    //      if (gapsequence[realposition] != GAP_SYMBOL)
    //              return 0;
    //      return 1;
}

int ADT_ALI::gap_inside(int showposition1,int showposition2) {
    //      if (gapshowoffset[showposition1] != gapshowoffset[showposition2])
    //              return 1;
    GBUSE(showposition1);GBUSE(showposition2);
    return 0;
}

int ADT_ALI::gap_behind(int showposition) {     /* returns 1 if
                                                   any excluded data is after
                                                   showposition */
    //      if (gapshowoffset[showposition] != gapshowoffset[gapshowoffset_len-1]) {
    //              return 1; }
    GBUSE(showposition);
    return 0;
}

int ADT_ALI::gap_realpos(int showposition) {
    return showposition;
    //      return (showposition + gapshowoffset[showposition]);
}

int ADT_ALI::gap_showpos(int realposition) {
    return realposition;
    //      return (realposition - gaprealoffset[realposition]);
}

#if 0
void ADT_ALI::operator = (ADT_ALI &adtali)
{
    // hier gibt es probleme, wenn speicherplatz noch nicht reserviert wurde
    gapsequence = adtali.gapsequence;
    gaprealoffset = adtali.gaprealoffset;
    gapshowoffset = adtali.gapshowoffset;
    (AD_ALI)*this = (AD_ALI &)adtali;

}
#endif
