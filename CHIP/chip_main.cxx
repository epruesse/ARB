//################################
// chip-reader
//################################


#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <stdlib.h>
//using namespace std;
#include <vector.h>


// ##########
// STRUKTUREN
// ##########


struct chiprow {
  unsigned int m_row;
  unsigned int m_col;
  unsigned int row;
  unsigned int col;
  char name[32];
  float sig;
  float bg;
  float delta;
  vector<int> probedeps;
};

struct proberow {
  char sProbe[32];
  char lProbe[128];
  char sMO[32];
  char lMO[128];
  vector<int> chipdeps;
};

struct results {
  char sMO[32];
  char lMO[128];
  float value;
  float pval;
  unsigned char grey; // reserved for further use - kai
  vector<int> chipdeps;
  //vector<int> probedeps;
};


// ############
// DEFINITIONEN
// ############


// Rueckgabewerte fuer Funktionen definieren
#define ret_ok    0
#define ret_err   1
#define ret_unref 2

// Hier werden die Ergebnisse der Gensonden abgelegt
vector<chiprow> chipData;
vector<proberow> probeData;
vector<results> resultData;
//vector<float> results;

bool ignore_unref = 0;


// ##########
// FUNKTIONEN
// ##########


int readchip(char *fn) {
  bool rawdata= 0;
  char line[255];
  ifstream iS;

  iS.open(fn);
  if(iS) {
    while(!iS.eof()) {
      iS.getline(line, 255);

      // finde Anfang und Ende der Daten
      if(strstr(line,"Begin Raw Data")) {
	iS.getline(line, 255); // Zeile++
	iS.getline(line, 255); // Zeile++
	rawdata= 1;
      }
      if(strstr(line,"End Raw Data")) {
	rawdata= 0;
      }

      // lese Daten ein
      if(rawdata) {
	// Zeile durchgehen und einlesen
	int i= 0;
	char buf[128];
	int buf_count= 0;
	chiprow dr;
	int dr_count= 0;

	while(line[i] && (i < 255)) {
	  while(line[i]== ' ' || line[i]== '\t' && line[i]) i++;
	  buf_count= 0;
	  while(line[i]!= ' ' && line[i]!= '\t' && line[i]) {
	    buf[buf_count]= line[i];
	    i++;
	    buf_count++;
	  }
	  buf[buf_count]= 0;

	  // Spalten aufdroeseln ...
	  switch(dr_count) {
	    case 1:
	      dr.m_row= atoi(buf);
	      break;
	    case 2:
	      dr.m_col= atoi(buf);
	      break;
	    case 3:
	      dr.row= atoi(buf);
	      break;
	    case 4:
	      dr.col= atoi(buf);
	      break;
	    case 5:
	      buf[31]= 0; // ziemlich dreckiger laengencheck ...
	      strcpy(dr.name, buf);
	      break;
	    case 7:
	      dr.sig= atof(buf);
	      break;
	    case 8:
	      dr.bg= atof(buf);
	      break;
	    default:
	      break;
	  }
	  dr_count++;
	}

	// Daten in vData abspeichern
	dr.delta= dr.sig - dr.bg;
	chipData.push_back(dr);
      }
    }
    iS.close();
    return ret_ok;
  }
  else {
    return ret_err;
  }
}

int readprobe(char *fn) {
  bool rawdata= 0;
  char line[255];
  ifstream iS;

  iS.open(fn);
  if(iS) {
    while(!iS.eof()) {
      iS.getline(line, 255);

      // finde Anfang und Ende der Daten
      if(strstr(line,"Begin Raw Data")) {
	iS.getline(line, 255); // Zeile++
	iS.getline(line, 255); // Zeile++
	rawdata= 1;
      }
      if(strstr(line,"End Raw Data")) {
	rawdata= 0;
      }

      // lese Daten ein
      if(rawdata) {
	// Zeile durchgehen und einlesen
	int i= 0;
	char buf[128];
	int buf_count= 0;
	proberow dr;
	int dr_count= 0;

	while(line[i] && (i < 255)) {
	  while(line[i]== ' ' || line[i]== '\t' && line[i]) i++;
	  buf_count= 0;
	  //while(line[i]!= ' ' && line[i]!= '\t' && line[i]) {
	  while(line[i]!= ':' && line[i]) {
	    buf[buf_count]= line[i];
	    i++;
	    buf_count++;
	  }

	  // rechte spaces entfernen
	  do {
	    buf_count--;
	  } while((buf[buf_count]==' ' || buf[buf_count]== '\t') && buf_count);
	  buf[buf_count+1]= 0;

	  i++; // muss hier stehen um ':' zu ueberspringen

	  // Spalten aufdroeseln ...
	  switch(dr_count) {
	    case 0:
	      buf[31]= 0; // ziemlich dreckiger laengencheck ...
	      strcpy(dr.sProbe, buf);
	      break;
	    case 1:
	      buf[127]= 0; // ziemlich dreckiger laengencheck ...
	      strcpy(dr.lProbe, buf);
	      break;
	    case 2:
	      buf[31]= 0; // ziemlich dreckiger laengencheck ...
	      strcpy(dr.sMO, buf);
	      break;
	    case 3:
	      buf[127]= 0; // ziemlich dreckiger laengencheck ...
	      strcpy(dr.lMO, buf);
	      break;
	    default:
	      break;
	  }
	  dr_count++;
	}
	probeData.push_back(dr);
      }
    }
    iS.close();
    return ret_ok;
  }
  else {
    return ret_err;
  }
}


int gen_deps(void)
{
  // size of vector_chiprow & vector_proberow ...
  int c_size= chipData.size();
  int p_size= probeData.size();

  int retval= ret_ok;

  // quick and dirty comparison ...  ;)
  for(int i= 0; i < p_size; i++) {
    for(int j= 0; j < c_size; j++) {
      if(!strcmp(chipData[j].name,probeData[i].sProbe)) {

	// link both structures together
	probeData[i].chipdeps.push_back(j);
	chipData[j].probedeps.push_back(i);

	// ### DEBUG
	// cout << i << ": found pair with \"" << probeData[j].sProbe << "\"\tpushed element #" << j <<endl;
	// ### DEBUG
      }
    }

    // throw a warning if there are unreferenced probes
    if(probeData[i].chipdeps.size() < 1) {
      cout << "warning: probe " << probeData[i].sProbe << "(" << probeData[i].sMO << ") is unreferenced.\n\n";
      retval= ret_unref;
    }
  }

  //generate unique (!) result table mask
  for(int k= 0; k < p_size; k++) {

    bool found_it= 0;

    for(unsigned int l= 0; l < resultData.size(); l++) {
      if(!strcmp(probeData[k].sMO, resultData[l].sMO)) found_it= 1;
    }
    if(!found_it && (probeData[k].chipdeps.size() > 0)) {
      results res;
      res.sMO=      probeData[k].sMO;
      res.lMO=      probeData[k].lMO;
      res.value=    0;
      res.pval=     0;
      res.chipdeps= probeData[k].chipdeps;
      resultData.push_back(res);

      // ###DEBUG
      // cout << "added: " << res.sMO << "\t\t" << res.lMO << "\n";
      // ###DEBUG
    }
  }
  return retval;
}

int formula(void) {
  // size of vector_chiprow & vector_proberow ...
  //int c_size= chipData.size();
  int r_size= resultData.size();

  float result= 0;
  float max_val= 0;

  for(int i= 0; i < r_size; i++) {
    result= 0;

    for(unsigned int j= 0; j < resultData[i].chipdeps.size(); j++) {

      // MATHS ARE DONE HERE !!!
      // #######################

      result= result + (chipData[resultData[i].chipdeps[j]].delta)/chipData[resultData[i].chipdeps[j]].probedeps.size();

      // #######################
      // MATHS ARE DONE HERE !!!

    }

    resultData[i].value= result;
    if(result > max_val) max_val= result;

    // ###DEBUG
    // cout << resultData[i].sMO << "\t\t" << result << endl;
    // ###DEBUG
  }

  //generate max_val
  for(int i= 0; i < r_size; i++) {
    resultData[i].pval= resultData[i].value / max_val;
    resultData[i].grey= (unsigned char)((resultData[i].value * 255) / max_val);
  }

  return ret_ok;
}


// #############
// HAUPTFUNKTION
// #############

int main(int argc, char **argv) {
  // Parameter 'okay-flag'
  int args_err= 0;
  int args_help= 0;
  char *arg_chipdata;
  char *arg_probedata;

  // Ueberpruefung der Parameter
  if(argc < 3) args_help= 1;
  for(int c= 1; c < argc; c++) {
    if((argv[c][0]== '-') && (argv[c][1]!= '-')) {
      switch(argv[c][1]) {
      case 'h':
	args_help= 1;
	break;
      case 'i':
	ignore_unref= 1;
	break;
      default:
	args_err= 1;
	break;
      }
    }
    if((argv[c][0]== '-') && (argv[c][1]== '-')) {
      switch(argv[c][2]) {
      case 'h':
	args_help= 1;
	break;
      case 'i':
	ignore_unref= 1;
	break;
      default:
	args_err= 1;
	break;
      }
    }
  }

  if((argc > 2) && (argv[argc-2][0] != '-') && (argv[argc-1][0] != '-')) {
    arg_chipdata= (char *)argv[argc-2];
    arg_probedata= (char *)argv[argc-1];
  }

  if(args_help && !args_err) {
    // Gib die Hilfe aus...
    cout << "Usage: " << argv[0] << " [OPTION]... CHIPRESULT PROBEDATA\n"
         << "Analyses the chipresult-file and computes a list of (possibly) matching microorganisms.\n\n"
         << " -h, --help\t\tdisplay this help and exit\n"
	 << " -i, --ignore\t\tignore possibly unreferenced probes\n"
       //<< " -d, --dest\t\tset a destination file for the results\n"
         << "\nPlease don't send bug-reports ... "
         << "(baderk@in.tum.de)\n";

    // Programm fehlerhaft beenden
    return ret_err;
  }

  if(args_err) {
    // Irgendwas ist bei den Parametern schief gelaufen...
    cout << argv[0] << ": error while processing arguments.\nUse \'-h\' or \'--help\' for further information.\n";
    return ret_err;
  }

  // ###
  // Hier folgen wichtige Funktionsaufrufe zur Datenverarbeitung ...
  // ###

  // Hier landet die Funktion falls die Parameter okay sind
  if(readchip(arg_chipdata) == ret_err) {
    // Konnte Chip-datei nicht lesen ...
    cout << argv[0] << ": error while accessing chipdata: " << arg_chipdata;
    cout << "\nUse \'-h\' or \'--help\' for further information.\n";
    return ret_err;
  }
  if(readprobe(arg_probedata) == ret_err) {
    // Konnte Probe-datei nicht lesen ...
    cout << argv[0] << ": error while accessing probedata: " << arg_probedata;
    cout << "\nUse \'-h\' or \'--help\' for further information.\n";
    return ret_err;
  }

  int gen_ret= gen_deps();
  if(gen_ret == ret_err) {
    // Konnte Verknuepfungen nicht erstellen ...
    cout << argv[0] << ": error while processing chip-/probedata.";
    cout << "\nUse \'-h\' or \'--help\' for further information.\n";
    return ret_err;
  }
  if((gen_ret == ret_unref) && !ignore_unref) {
    // Konnte Verknuepfungen nicht erstellen ...
    cout << argv[0] << ": unreferenced probes. Please check your probedata.";
    cout << "\nUse \'-h\' or \'--help\' for further information.\n";
    return ret_err;
  }

  formula();

  for(unsigned int u= 0; u < resultData.size(); u++) {
    cout << resultData[u].sMO << endl;
    for(int q= 0; q < (resultData[u].grey / 4); q++) cout << "#";
    cout << " " << resultData[u].pval << endl << endl;
  }

  // ###DEBUG
  //for(unsigned int x= 0; x < chipData.size(); x++) {
  //  cout << "chip  m_row=" << chipData[x].m_row << "\tm_col=" << chipData[x].m_col
  // << "\trow=" << chipData[x].row << "\tcol=" << chipData[x].col << "\tname=" << chipData[x].name
  // << "\tsig=" << chipData[x].sig << "\tbg=" << chipData[x].bg << "\t--> delta=" << chipData[x].delta << endl;
  //}
  //for(unsigned int x= 0; x < probeData.size(); x++) {
  //  cout << "probe sProbe=" << probeData[x].sProbe << "\tlProbe=" << probeData[x].lProbe
  // << "\tsMO=" << probeData[x].sMO << "\tlMO=" << probeData[x].lMO << endl;
  //}
  // ###DEBUG

  // Programm beenden
  return ret_ok;
}
