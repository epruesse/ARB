#include "awt_iupac.hxx"

class SQ_ambiguities {

public:
    SQ_ambiguities();
    void SQ_count_ambiguities(const char *iupac, int length);
    int SQ_get_nr_ambiguities();
    int SQ_get_percent_ambiguities();
    int SQ_get_iupac_value();

private:
    const char *iupac;
    int number;
    int percent;
    int iupac_value;

};


SQ_ambiguities::SQ_ambiguities(){
    iupac       = 0;
    number      = 0;
    percent     = 0;
    iupac_value = 0;
}


void SQ_ambiguities::SQ_count_ambiguities(const char *iupac, int length){
    this->iupac = iupac;
    char c;

    for (int i = 0; i <length; i++) {
	c = iupac[i];
	switch (c){
	    case 'R':
		number++;
		iupac_value = iupac_value+2;
		break;
	    case 'Y':
		number++;
		iupac_value = iupac_value+3;
		break;
	    case 'M':
		number++;
		iupac_value = iupac_value+2;
		break;
	    case 'K':
		number++;
		iupac_value = iupac_value+3;
		break;
	    case 'W':
		number++;
		iupac_value = iupac_value+3;
		break;
	    case 'S':
		number++;
		iupac_value = iupac_value+2;
		break;
	    case 'B':
		number++;
		iupac_value = iupac_value+4;
		break;
	    case 'D':
		number++;
		iupac_value = iupac_value+4;
		break;
	    case 'H':
		number++;
		iupac_value = iupac_value+4;
		break;
	    case 'V':
		number++;
		iupac_value = iupac_value+3;
		break;
	    case 'N':
		number++;
		iupac_value = iupac_value+5;
		break;
	    }
    }
    percent = (100 * number) / length;
}


int SQ_ambiguities::SQ_get_nr_ambiguities() {
    return number;
}


int SQ_ambiguities::SQ_get_percent_ambiguities() {
    return percent;
}

int SQ_ambiguities::SQ_get_iupac_value() {
    return iupac_value;
}
