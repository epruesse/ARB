//################################
// chip-reader
// v1.0
//################################


#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#include <string.h>
//#include "stdlib.h"
//using namespace std;
//#include <vector>


// ##########
// STRUKTUREN
// ##########


// ###################
// KLASSENDEFINITIONEN
// ###################


// Klasse textdata
// liest eine Textdatei ein und parst diese in ein ARRAY
//
class textdata {
private:
  int arr[255][32];
  char v[255][32];
  //list<const char *> v;
public:
  textdata() {
  }
  ~textdata() {
  }
  // liest eine Textdatei ein
  int read(char *fn) {
    char line[255];
    int rowcount = 0;
    int colcount = 0;
    int v_count = 0;
    ifstream iF;
    iF.open(fn);
    if(iF) {

      cout << "File was opened ...\n";    // DEBUG !!!

      while(!iF.eof()) {
	iF.getline(line, 255);

	cout << "Reading new row \"" << rowcount << "\"\n";   // DEBUG !!!

	int i = 0;
	bool eol = 0;
	char wbuf[255];

	while(!eol) {
	  cout << "Reading new col \"" << colcount << "\"\n";    // DEBUG !!!

	  while(line[i] == '\t' || line[i] == ' ') i++;
	  int wbuf_count = 0;
	  while(line[i] != '\t' && line[i] != ' ' && line[i] != 0) {
	    wbuf[wbuf_count++] = line[i++];
	  }
	  if(line[i] == 0) {
	    eol= 1;
	    wbuf_count--;  // ACHTUNG: Koennte bei Unix-texten zu Problemen fuehren (besser oben anpassen)
	  }
	  wbuf[wbuf_count] = 0;
	  strcpy(v[v_count], wbuf);

	  printf("    String \"%s\" was added\n", wbuf);

	  arr[rowcount][colcount] = v_count;
	  colcount++;
	  v_count++;  
	}
	rowcount++;
	colcount = 0;
      }
      iF.close();
      return 0;
    }
    else {
      return -1;
    }
  }
  // liefert den Text an der Position row,col
  int data(char &ptr, int row, int col){
    return 0;
  }
};


// #############
// Hauptfunktion
// #############


int main(int argc, char **argv) {
  if(argc == 2) {
    textdata td;
    if(!(td.read(argv[1]))) {
      cout << "...hat scheinbar funktioniert...\n";
    }
  }
  else {
    cout << "\nUSAGE: wrong number of arguments...\n";
  }

  return 0;
}
