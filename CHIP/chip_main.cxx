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
};

struct proberow {
  char sProbe[32];
  char lProbe[128];
  char sMO[32];
  char lMO[128];
};


// ############
// DEFINITIONEN
// ############

// Hier werden die Ergebnisse der Gensonden abgelegt
vector<chiprow> chipData;
vector<proberow> probeData;

// Sonstige interne Einstellungen

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
    return 0;
  }
  else {
    cout << "ERROR WHILE PROCESSING CHIP-DATA ..." << endl;
    return -1;
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
    return 0;
  }
  else {
    cout << "ERROR WHILE PROCESSING PROBE-DATA ..." << endl;
    return -1;
  }
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
      default:
	args_err= 1;
	break;
      }
    }
  }

  if((argc > 2) && (argv[argc-2][0] != '-') && (argv[argc-1][0] != '-')) {
    arg_chipdata= (char *)argv[argc-2];
    arg_probedata= (char *)argv[argc-1];

    // ###DEBUG
    cout << "Chipdata:  " << arg_chipdata <<"\n"
	 << "Probedata: " << arg_probedata <<"\n\n";
    // ###DEBUG
  }

  if(args_help && !args_err) {
    // Gib die Hilfe aus...
    cout << "Usage: " << argv[0] << " [OPTION]... CHIPRESULT PROBEDATA\n"
         << "Analyses the chipresult-file and computes a list of (possibly) matching microorganisms.\n\n"
         << " -h, --help\t\tdisplay this help and exit\n"
       //<< " -y, --yyyy\t\tstill not available\n"
       //<< " -z, --zzzzzzz\t\tyou can't read, can you?\n"
         << "\nPlease don't send bug-reports ... "
         << "(baderk@in.tum.de)\n";

    // Programm fehlerhaft beenden
    return(-1);
  }

  if(args_err) {
    // Irgendwas ist bei den Parametern schief gelaufen...
    cout << argv[0] << ": error while processing arguments. Use \'-h\' or \'--help\' for further information.\n";

    // Programm fehlerhaft beenden
    return(-1);
  }

  // Hier landet die Funktion falls die Parameter okay sind
  readchip(arg_chipdata);
  readprobe(arg_probedata);

  // ###DEBUG
  for(unsigned int x= 0; x < chipData.size(); x++) {
    cout << "chip  m_row=" << chipData[x].m_row << "\tm_col=" << chipData[x].m_col
	 << "\trow=" << chipData[x].row << "\tcol=" << chipData[x].col << "\tname=" << chipData[x].name
	 << "\tsig=" << chipData[x].sig << "\tbg=" << chipData[x].bg << "\t--> delta=" << chipData[x].delta << endl;
  }
  for(unsigned int x= 0; x < probeData.size(); x++) {
    cout << "probe sProbe=" << probeData[x].sProbe << "\tlProbe=" << probeData[x].lProbe
	 << "\tsMO=" << probeData[x].sMO << "\tlMO=" << probeData[x].lMO << endl;
  }
  // ###DEBUG

  // Programm beenden
  return(0);
}
