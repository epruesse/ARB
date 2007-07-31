#include <memory.h>
#include <string.h>
#include <stdio.h>

#include <arbdb.h>
#include "arbdb++.hxx"
#include "adtools.hxx"

ADT_SEQUENCE::ADT_SEQUENCE() {
	memset(this,0,sizeof(ADT_SEQUENCE));
	adt_ali = 0;
}

ADT_SEQUENCE::~ADT_SEQUENCE() {
//	if (showsequence) delete showsequence;
}


void ADT_SEQUENCE::init(ADT_ALI *adtali,AD_CONT * adcont) {
 	adt_ali =  adtali;
	AD_SEQ::init(adcont);
	show_timestamp = timestamp;

}

char *ADT_SEQUENCE::show_get() {
	if (adt_ali != 0) {
		if ((seq == 0) || (show_timestamp < timestamp)  ) {
			ADT_SEQUENCE::show_update();
		}
		return seq;
	}
	new AD_ERR("ADT_SEQUENCE::show_get() class not inited ?",CORE);
	return 0;
}

void ADT_SEQUENCE::show_update() {
	;
}

int ADT_SEQUENCE::show_len() {
	return seq_len;
}

AD_ERR * ADT_SEQUENCE::show_put() {
	AD_ERR * error = 0;
	error = put();
	if (error != 0) {
		show_update();
		return error;
		}
	show_timestamp = timestamp;
	return  error ;
}


/*** EDITIERFUNKTIONEN *******/
// show_insert
// show_remov5// show_replace
//
// beeinflussen nicht seq_len und showseq_len ( -> wird von gaps beeinflusst)
// seq_len entspricht der alignment laenge
// das Ende wird mit punkten aufgefuellt
//

AD_ERR * ADT_SEQUENCE::show_insert(char *text,int showposition) {
// insert text in showsequenc und seq (AD)
// funktionen: gap_realpos,
// nur erlaubt, wenn kein gap dahinter
    AD_ERR *ad_err;
    int realposition = adt_ali->gap_realpos(showposition);	// bereich in der richtigen sequence
    if (show_timestamp != timestamp) {
	return new AD_ERR("ADT_SEQUENCE::show_insert  -- not allowed -- because timeupdate not done !");
    }
    if (adt_ali->gap_behind(realposition) == 1) {	// kein insert wenn luecke dahinter
	return new AD_ERR("ADT_SEQUENCE::show_insert  -- not allowed because Gap behind position");
    }
    ad_err = this->insert(text,realposition,1); 			//@@@ insert in blocks !!!
    if (!ad_err){
	this->show_update();
    }
    return ad_err;
}



AD_ERR  * ADT_SEQUENCE::show_remove(int len,int showposition) {
	AD_ERR *ad_err;
int realposition = adt_ali->gap_realpos(showposition);
	if ((realposition < 0) || (len<0) || (len > seq_len) || (showposition >= seq_len)) {
		return new AD_ERR("ADT_SEQ.remove() WARNING ! Wrong Parameters !");
		}
	if (adt_ali->gap_behind(realposition) == 1) {	// kein remove wenn luecke dahinter
		return new AD_ERR("ADT_SEQUENCE::show_remove  -- not allowed because Gap behind position");
		}
	if (show_timestamp != timestamp) {
		return new AD_ERR("ADT_SEQUENCE::show_remove  -- not allowed -- because timeupdate not done !");
		 }
	ad_err = this->remove(len,realposition,1);
	if (!ad_err) this->show_update();
	return ad_err;
}


AD_ERR * ADT_SEQUENCE::show_replace(char *text,int showposition) {
	if (show_timestamp != timestamp) {
		return new AD_ERR("ADT_SEQUENCE::show_replace  -- not allowed -- because timeupdate not done !");
		}
	AD_ERR *ad_err;
	ad_err = this->replace(text,showposition,1);
	if (!ad_err) this->show_update();
	return ad_err;
}

AD_ERR *ADT_SEQUENCE::show_command( AW_key_mod keymod, AW_key_code keycode, char key, int direction, long &cursorpos, int& changeflag)
	{
	AD_ERR *err= this->command(keymod,keycode,key,direction, cursorpos,changeflag);
	if (changeflag) this->show_update();
	return err;
}


