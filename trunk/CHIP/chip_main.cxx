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

struct datarow {
  unsigned int m_row;
  unsigned int m_col;
  unsigned int row;
  unsigned int col;
  char name[64];
  float sig;
  float bg;
  float delta;
};


// ############
// DEFINITIONEN
// ############

vector<datarow> vData;


// ##########
// FUNKTIONEN
// ##########

int readdata(char *fn) {
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
	rawdata= 1;
      }
      if(strstr(line,"End Raw Data")) {
	rawdata= 0;
      }

      // lese Daten ein
      if(rawdata) {
	// Zeile durchgehen und einlesen
	int i= 0;
	char buf[32];
	int buf_count= 0;
	datarow dr;
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
	      strcpy(dr.name, buf); // Check >64Bytes fehlt noch!
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
	vData.push_back(dr);
      }
    }
    iS.close();

    // ###DEBUG
    for(unsigned int x= 0; x < vData.size(); x++) {
      cout << "reading...  m_row=" << vData[x].m_row << "  m_col=" << vData[x].m_col;
      cout << "  row=" << vData[x].row << "  col=" << vData[x].col << "  name=" << vData[x].name;
      cout << "  sig=" << vData[x].sig << "  bg=" << vData[x].bg << "  --> delta=" << vData[x].delta << endl;
    }
    // ###DEBUG

    return 0;
  }
  else {
    return -1;
  }
}


// #############
// HAUPTFUNKTION
// #############

int main(int argc, char **argv) {

  if(argc == 2) {
    readdata(argv[1]);
  }
  else {
    cout << "\nwrong number of arguments...\n";
  }
  return(0);
}

