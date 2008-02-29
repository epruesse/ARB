
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>
#include <arbdb.h>
#include "arbdb++.hxx"
#include "adtools.hxx"
/***************************************************************************
class ADT_COMPLEMENT

***************************************************************************/
ADT_COMPLEMENT::ADT_COMPLEMENT() {
//	ad_species 	= NULL;
	species_name	= NULL;
	alignment_type	= NULL;
	alignment_name	= NULL;
	sequence		= NULL;
	char_array	= NULL;
	sequence_buffer= NULL;
	index_buffer	= NULL;

	adt_acid			= ADT_ALITYPE_UNDEFINED;
	complement_seq 	= NO;
	invert_seq		= NO;
	seq_is_complemented	= NO;
	seq_is_inverted	= NO;
	take_cursor		= NO;
	take_borders		= NO;
	remove_gaps_points	= NO;
	which_button		= NO_BUTTON;

	alignment_length	= max_value;		//max_value
	sequence_length	= 0;
	left_border		= 0;
	right_border		= 0;

}

//*******************
ADT_COMPLEMENT::~ADT_COMPLEMENT() {
	// delete	char_array;
}



//*******************
char * ADT_COMPLEMENT::make_char_array()
{
	char	*local_char_array = new char[256];

	for( int i=0 ; i <= 255 ; i++ ) {
		local_char_array[i] = (char) i;
	}
	return local_char_array;
}

//*******************
AD_ERR * ADT_COMPLEMENT::complement_compile(void) {  //Veraendert Zeichensatz

	delete char_array;
	char_array = make_char_array();

	char_array[(int)'C'] = 'G';
	char_array[(int)'c'] = 'g';
	char_array[(int)'G'] = 'C';
	char_array[(int)'g'] = 'c';
	char_array[(int)'U'] = 'A';
	char_array[(int)'u'] = 'a';
	char_array[(int)'T'] = 'A';
	char_array[(int)'t'] = 'a';
	char_array[(int)'M'] = 'K';
	char_array[(int)'m'] = 'k';
	char_array[(int)'R'] = 'Y';
	char_array[(int)'r'] = 'y';
//	char_array[(int)'W'] = 'W';		//der Vollstaendigkeit halber
//	char_array[(int)'w'] = 'w';		//der Vollstaendigkeit halber
//	char_array[(int)'S'] = 'S';		//der Vollstaendigkeit halber
//	char_array[(int)'s'] = 's';		//der Vollstaendigkeit halber
	char_array[(int)'Y'] = 'R';
	char_array[(int)'y'] = 'r';
	char_array[(int)'K'] = 'M';
	char_array[(int)'k'] = 'm';
	char_array[(int)'V'] = 'B';
	char_array[(int)'v'] = 'b';
	char_array[(int)'H'] = 'D';
	char_array[(int)'h'] = 'd';
	char_array[(int)'D'] = 'H';
	char_array[(int)'d'] = 'h';
	char_array[(int)'B'] = 'V';
	char_array[(int)'b'] = 'v';
//	char_array[(int)'N'] = 'N';		//der Vollstaendigkeit halber
//	char_array[(int)'n'] = 'n';		//der Vollstaendigkeit halber
//	char_array[(int)'X'] = 'X';		//der Vollstaendigkeit halber
//	char_array[(int)'x'] = 'x';		//der Vollstaendigkeit halber
//	char_array[(int)'.'] = '.';		//der Vollstaendigkeit halber

	switch(adt_acid)  {

		case ADT_ALITYPE_RNA:
			char_array[(int)'A'] = 'U';
			char_array[(int)'a'] = 'u';
			break;

		case ADT_ALITYPE_DNA:
			char_array[(int)'A'] = 'T';
			char_array[(int)'a'] = 't';
			break;
	default:
	    break;
	}

	return 0;
}

//*******************
AD_ERR * ADT_COMPLEMENT::complement_buffers(void) {  //Puffer fuer Seq + Index

	char		*seq_buff;
	long		*ind_buff;

	long		buffer_len	= 0;

	//buffer_len = (long)strlen(sequence);
	buffer_len = sequence_length;

	seq_buff = (char *)calloc( ((int)buffer_len + 1), sizeof(char) );
	memset(seq_buff, (int)'.', ((int)buffer_len));
	sequence_buffer = seq_buff;


	ind_buff = (long *) calloc( ((int)buffer_len + 1), sizeof(long) );
	ind_buff[buffer_len] = -1;
	index_buffer = ind_buff;

	return 0;
}



/***************************************************************************
class ADT_EDIT

***************************************************************************/
ADT_EDIT::ADT_EDIT() {

	selection			=  0;
	found_matchp 		=  0;
	actual_cursorpos_editor = -1;
	replace_overflow	=  0;
	mistakes_found		=  0;
	seq_equal_match		=  1;

	db_status			=  ADT_DB_CLOSED;
	border_overflow	=  NO;
}

ADT_EDIT::~ADT_EDIT() {
}


/***************************************************************************
class ADT_SEARCH

***************************************************************************/
ADT_SEARCH::ADT_SEARCH() {

	seq_anfang		= NULL;
	save_sequence		= NULL;
	seq_index_start	= NULL;
	matchpattern_buffer	= NULL;
	search_array		= NULL;
	replace_string		= NULL;
	mistakes_allowed	= 0;
	gaps			= -1;		//gaps werden nicht ignoriert
	upper_eq_lower 		=  0;		//gross/KLEIN-Schreibung,
	t_equal_u 		=  0;		//t und u gleichbehandeln,
	search_start_cursor_pos =  0;
	replace_start_cursor_pos=  0;
	search_direction 	= ADT_SEARCH_FORWARD;	//Vorwaertssuche
	replace_option		= ADT_NO_REPLACE;	//Kein REPLACE
	replace_loop_sequence	= ADT_DONT_STOPP_REPLACE;

	found_cursor_pos 	= 0;
	string_replace		= 0;
}

ADT_SEARCH::~ADT_SEARCH() {
	delete search_array;
	delete matchpattern_buffer;
}

//*************************************
char * ADT_SEARCH::show_search_array()
{
	search_array = new char[256];

	for( int i=0 ; i <= 255 ; i++ ) {
		search_array[i] = (char) i;
	}
	return search_array;
}

//***********************************
			//Veraendert Zeichensatz, Liefert Matchpattern ohne Gaps
AD_ERR * ADT_SEARCH::compile(void) {
	delete search_array;
	search_array = show_search_array();

	if(t_equal_u) {
		search_array[(int)'U'] = 'T';
		search_array[(int)'u'] = 't';
	}

	if(upper_eq_lower) {
		int char_pos;
		for( char_pos = 'a'; char_pos <= 'z' ; char_pos++) {
			search_array[char_pos] -= ( 'a' - 'A' );
		}
	}

		///////////////////////////////////Matchpattern WITHOUT gaps.
	if((gaps == 1) || (gaps == 2)) {
		delete matchpattern_buffer;
		matchpattern_buffer=(char *)calloc(strlen(matchpattern)+1,sizeof(char));

		if(matchpattern_buffer != NULL) {
			char	*matchp_copy_loop;
			char	*matchp_buffer_start, *matchp_buffer_loop;

			matchp_buffer_start = matchp_buffer_loop =
							matchpattern_buffer;

			for( matchp_copy_loop = matchpattern ;
				*matchp_copy_loop != '\0' ;
							matchp_copy_loop++ ) {
				if(*matchp_copy_loop != '-') {
					*matchp_buffer_loop  = *matchp_copy_loop;
					matchp_buffer_loop++;
				}
			}
			*matchp_buffer_loop	= '\0';
			matchpattern = matchp_buffer_start;
		}
	}
	return 0;
}


//**************************************************************************
//	Funktion liefert die Sequenz entsprechend OHNE Gaps !!!!
//	(Sequenz wird ohne Gaps in ein Puffer-Array geschrieben,
//	=> Pointer auf Sequenz-Puffer)
//
//**************************************************************************
AD_ERR * ADT_SEQUENCE::make_sequence_buffer(ADT_SEARCH
				      *ptr_adt_search, ADT_EDIT *ptr_adt_edit) {

	//-------------------------------------------------start sequencebuffer
	char	*seq_anfang, *seq_copy_loop;
	char 	*seq_buffer_start, *seq_buffer_loop;	//buffer
	long	*seq_index_start, *seq_index_loop;	//index buffer

	seq_anfang = ptr_adt_search->seq_anfang;

        char source;

	//------------------------------------------memory for sequence-buffer
	seq_buffer_start = (char *)calloc(strlen(seq_anfang)+1,sizeof(char));

	if (!seq_buffer_start) {
//		printf("\n**** ERROR sequence-buffer no memory allocated");
	}
	seq_buffer_loop = seq_buffer_start;
	//------------------------------------------------------end seq_buffer

	//---------------------------------------------memory for index-buffer
	seq_index_start = (long *) calloc(strlen(seq_anfang)+1,sizeof(long));

	if (!seq_index_start) {
//		printf("\n**** ERROR index-buffer no memory allocated");
	}
	seq_index_loop = seq_index_start;
	//----------------------------------------------------end index_buffer

	for( seq_copy_loop = seq_anfang ;
	     (source = *seq_copy_loop) ;
	     seq_copy_loop++ ) {
		if(*seq_copy_loop != '-') {
			*(seq_buffer_loop++) = source;	//kopiert Zeichen
			*(seq_index_loop++)  = seq_copy_loop - seq_anfang;
							//Index des Zeichens
			}
		}
	*seq_buffer_loop	= '\0';
	*seq_index_loop		= -1;
	ptr_adt_search->seq_anfang = seq_buffer_start; //SEQUENCE START
	//--------------------------------------------------end sequencebuffer

	// -------------------------------------neu search/replace cursorstart
	long	var_cursorpos_editor = ptr_adt_edit->actual_cursorpos_editor;
						  //========================!!
					// Fuer die Neuberechnung der Startpos.
	long	buffer_index = 0;		//zaehlvariable fuer
						//Cursorberechnung
	for(  buffer_index = 0 ;
			(seq_index_start[buffer_index] <
				   		       var_cursorpos_editor) &&
				  (seq_index_start[buffer_index] != -1 ) ;
							buffer_index++ ) {
	}
				//   "buffer_index"
				//somit steht hier der Index, unter dessen
				//Adresse die Cursorposition steht, die
				//entweder groesser (Cursor stand auf einem GAP)
				//oder gleich (Cursor stand auf einem Zeichen)
				//der Cursorpos im Editor ist, vielmehr war.

	if( ptr_adt_search->search_direction == ADT_SEARCH_FORWARD) {
		if( (seq_index_start[buffer_index] == var_cursorpos_editor) &&
						 (var_cursorpos_editor > 0) ) {
			ptr_adt_search->search_start_cursor_pos =
							   buffer_index + 1;
		}
		else {
			ptr_adt_search->search_start_cursor_pos =
							       buffer_index;
		}
	} // end if
	else {
		ptr_adt_search->search_start_cursor_pos = buffer_index - 1;
	} // end else
	delete ptr_adt_search->seq_index_start;
	ptr_adt_search->seq_index_start	 = seq_index_start; //indexarray
	ptr_adt_edit->actual_cursorpos_editor = buffer_index;
	//----------------------------------------------------end cursorstart

	return 0;
}
//**************************************************************************



//**************************************************************************
//
//**************************************************************************
AD_ERR * ADT_SEQUENCE::rewrite_from_sequence_buffer(ADT_SEARCH
				    *ptr_adt_search, ADT_EDIT  *ptr_adt_edit) {

	char	*seq_anfang;		//Sequenz
	char 	*seq_buffer_start;	//Sequenz ohne GAPS
	long	*seq_index_start;	//index buffer, siehe unten!
	long	found_cursor_pos_buffer = ptr_adt_search->found_cursor_pos;

	//--------------------------------found_cursor_pos nach Suche ohne Gaps
	if (( ptr_adt_edit->found_matchp == 1 ) &&
			( ptr_adt_search->replace_option == ADT_NO_REPLACE )) {
		ptr_adt_search->found_cursor_pos =
		      ptr_adt_search->seq_index_start[found_cursor_pos_buffer];
	}
	//-------------------------------------------------END found_cursor_pos

	//---------------------------------------rewrite Seq-Puffer auf Sequenz
	if( ptr_adt_search->replace_option != ADT_NO_REPLACE )  {
		seq_buffer_start = ptr_adt_search->seq_anfang;
		seq_index_start  = ptr_adt_search->seq_index_start;
		seq_anfang = get();

		long buffer_index = 0; //zaehlvariable
                char source;

		for( ; (source = seq_buffer_start[buffer_index]) ; buffer_index++ ) {
                    *(seq_anfang + seq_index_start[buffer_index]) = source;
		}
		ptr_adt_search->seq_anfang = seq_anfang;
		//-------------------------------------------------end rewrite

		char *replace_anfang	   = ptr_adt_search->replace_string;
		long  var_start_cursor_pos = ptr_adt_search->replace_start_cursor_pos;

		//--------------------------------------------cursorpos_editor
		long  buffer_end_replace = var_start_cursor_pos + strlen(replace_anfang);

		ptr_adt_edit->actual_cursorpos_editor =
			ptr_adt_search->seq_index_start[buffer_end_replace-1];
					//  nur bei Suchrichtung
					//  "Vorwaerts" !!!
		//----------------------------------------end cursorpos_editor

		free(seq_buffer_start);
		ptr_adt_search->seq_anfang = NULL;
		free((char *)seq_index_start);
		ptr_adt_search->seq_index_start = NULL;

	}
	//--------------------------------------------------------END rewrite

	return 0;
}
//-----------------------------------------END rewrite_from_sequence_buffer()
//**************************************************************************




//**************************************************************************
//
//**************************************************************************
AD_ERR * ADT_SEQUENCE::show_edit_seq_search(ADT_SEARCH *ptr_adt_search,
						     ADT_EDIT *ptr_adt_edit) {
	AD_ERR	*ad_err;

	long	replace_loop = ADT_DONT_STOPP_REPLACE;	//default
					     // ADT_DONT_STOPP_REPLACE == 1 !!

	ptr_adt_search->seq_anfang	=  get();


					// Sicherheitsabfrage fuer die
					// Rueckwaertssuche !!
	if(ptr_adt_search->search_start_cursor_pos > len()) {
						//Cursorpos > Sequenzlaenge?
		ptr_adt_search->search_start_cursor_pos = len() - 1;
	}

	//-----------------------------------------------------Sequenz puffern
 	if( (ptr_adt_search->gaps == 0) ||  (ptr_adt_search->gaps == 1) ) {
		make_sequence_buffer(ptr_adt_search, ptr_adt_edit);
		///////////////////////////////////////////////////
	} //end if,  >>Sequenz<< OHNE Gaps.
	//---------------------------------------------------- end seq puffern

	//---------------------------------Sicherungskopie der Seq fuer REPLACE
	if( ptr_adt_search->replace_option != ADT_NO_REPLACE ) {
		ptr_adt_search->save_sequence = strdup(ptr_adt_search->seq_anfang);
	}
	//--------------------------------------------------END Sicherungskopie

	//---------------------------------------korrigiert die Start Cursorpos
				// bis Ende.., da erst search(), dann replace()
	if( ( (ptr_adt_search->replace_option == ADT_REPLACE_ONLY)         ||
	      (ptr_adt_search->replace_option == ADT_REPLACE_AND_SEARCH_NEXT)||
	      (ptr_adt_search->replace_option == ADT_REPLACE_REST_SEQUENCE)||
	      (ptr_adt_search->replace_option == ADT_REPLACE_REST_EDITOR)  ) &&
	    (ptr_adt_search->search_start_cursor_pos > 0)		   ) {

		ptr_adt_search->search_start_cursor_pos--;
	}
	//----------------------------------------------------END start_cursor

	//====================================================================
	//=================================================while(replace_loop)
	long max_loop = this->seq_len;
	do {						 // END Sequenz/Editor

		//------------------------------------------------------SEARCH
		ptr_adt_edit->found_matchp = 0;

		show_edit_search(ptr_adt_search, ptr_adt_edit);
		//////////////////////////////////////////////
		//------------------------------------------------- END SEARCH

		if( ((ptr_adt_search->replace_option == ADT_REPLACE_ONLY) ||
		     (ptr_adt_search->replace_option == ADT_REPLACE_AND_SEARCH_NEXT)) &&
		     (ptr_adt_edit->found_matchp == 0) )   {
			ptr_adt_edit->seq_equal_match = 0;
			ptr_adt_edit->found_matchp = 1;
			return 0;
		} // end if ( ... == ADT_REPLACE_ONLY)

		else {
			ptr_adt_search->replace_start_cursor_pos =
					      ptr_adt_search->found_cursor_pos;
		}

		//---------------------------------------------------- REPLACE
		if( (ptr_adt_edit->found_matchp == 1) &&
			(ptr_adt_search->replace_option != ADT_NO_REPLACE) ) {

			ad_err = show_edit_replace(ptr_adt_search,ptr_adt_edit);
				 //////////////////////////////////////////////
			if(ad_err) 	return ad_err;
		}
		//-------------------------------------------------END REPLACE

		//------------------------------------------repl and find next
		if( (ptr_adt_search->replace_option == ADT_REPLACE_AND_SEARCH_NEXT) &&
		    (ptr_adt_search->string_replace == 1) )  		    {
			ptr_adt_search->string_replace = 0;
			ptr_adt_search->search_start_cursor_pos =
				ptr_adt_edit->actual_cursorpos_editor + 1;
			ptr_adt_search->replace_option = ADT_NO_REPLACE;
		}
						//---------end repl/find next
		else  {
			if( (ptr_adt_search->replace_option == 	ADT_REPLACE_REST_SEQUENCE) ||
			    (ptr_adt_search->replace_option == 	ADT_REPLACE_REST_EDITOR) )  {

				replace_loop = ptr_adt_search->replace_loop_sequence;
				ptr_adt_search->search_start_cursor_pos =
						( ptr_adt_edit->actual_cursorpos_editor + 1 );

					// ABBRECHEN, sonst naechste Sequenz
				if( replace_loop == ADT_STOPP_REPLACE ) {
					ptr_adt_edit->found_matchp = 1;
				}
				if (max_loop-- <0) replace_loop = ADT_STOPP_REPLACE;
			} else {
				replace_loop = ADT_STOPP_REPLACE;
			}
		}

	} while(replace_loop);
	//============================================================end loop
	//====================================================================

	delete ptr_adt_search->save_sequence; 	//Sicherungskopie loeschen

	//-----------------------------------------Zurueckschreiben aus Puffer
	if( (ptr_adt_search->gaps == 0) ||  (ptr_adt_search->gaps == 1) ) {
		rewrite_from_sequence_buffer(ptr_adt_search, ptr_adt_edit);
		/////////////////////////////////////////////
	} //end if,  >>Sequenz<< OHNE Gaps.
	//---------------------------------------------------------end rewrite

	//----------------------------------- zurueckschreiben in die Datenbank
	if( (ptr_adt_search->string_replace == 1)  ||
	    			  (ptr_adt_edit->db_status == ADT_DB_OPEN) ) {
		ad_err = put();
		if(ad_err) {
//			printf("**** ERROR  ADT_SEARCH	show_edit_replace(), put() \n");
			return  ad_err;
		}

		show_update();
	}
	//---------------------------------------------------end write into DB

	if( (ptr_adt_edit->found_matchp == 1) &&
				      (ptr_adt_search->string_replace == 0) ) {
		ptr_adt_edit->actual_cursorpos_editor =
					       ptr_adt_search->found_cursor_pos;
	}

					//   nichts gefunden,
	//---------------------------------- cursorpos in next sequence
	if( (ptr_adt_edit->found_matchp == 0)  ||
	    (ptr_adt_search->replace_option == ADT_REPLACE_REST_EDITOR) ) {
		if( ptr_adt_search->search_direction == ADT_SEARCH_FORWARD ) {
			ptr_adt_edit->actual_cursorpos_editor   = 0;
			ptr_adt_search->search_start_cursor_pos = 0;
					//startet naechste Seq bei Cursorpos = 0
		}
		if( ptr_adt_search->search_direction == ADT_SEARCH_BACKWARD ) {
			ptr_adt_edit->actual_cursorpos_editor   = max_value;
			ptr_adt_search->search_start_cursor_pos = max_value;
		}
	}
	//-------------------------------end cursorpos in next sequence

	return 0;
} // end func show_edit_seq_search()

//**************************************************************************




//**************************************************************************
//
//**************************************************************************
AD_ERR * ADT_SEQUENCE::show_edit_search(ADT_SEARCH *ptr_adt_search,
						      ADT_EDIT *ptr_adt_edit) {

	char	*seq_anfang, *seq_loop_start, *seq_loop_ptr;	// Suche Sequenz
	char	*matchp_anfang, *matchp_loop_ptr;		// Suche
								// Matchpattern

	long	matchp_len;
	long	vorgabefehler, realfehler=0, equal_chars;
	long	seq_empty_var;
	long 	search_var;

	search_var	=  ptr_adt_search->search_direction;

	matchp_anfang	=  ptr_adt_search->matchpattern;
	matchp_len	=  strlen(matchp_anfang);

	seq_empty_var=0;
	vorgabefehler = ptr_adt_search->mistakes_allowed;

	seq_anfang	=  ptr_adt_search->seq_anfang;
	long	var_start_cursor_pos = ptr_adt_search->search_start_cursor_pos;

	seq_loop_start	=  seq_anfang + var_start_cursor_pos;

	//---------------------------------------------------------start search
	for(  ; (*seq_loop_start != '\0') &&
				(seq_loop_start >= seq_anfang) ;
				 seq_loop_start+=search_var )     {
		realfehler  = 0;
		equal_chars = 0;
		seq_loop_ptr=seq_loop_start;

		for( matchp_loop_ptr=matchp_anfang ; *matchp_loop_ptr != '\0' ;
 							matchp_loop_ptr++ ) {

		       unsigned char index_seq = *(unsigned char *)seq_loop_ptr;
		       unsigned char index_mat=*(unsigned char*)matchp_loop_ptr;
			char i_s = ptr_adt_search->search_array[(int)index_seq];
 			char i_m = ptr_adt_search->search_array[(int)index_mat];
			if( i_m == '?' ) {	// Wilde Karten
				equal_chars++;
			}

			else if( i_s != i_m ) {
					realfehler++;
				}
				else {
					equal_chars++;
				}
						//   mehr Fehler als erlaubt?
			if(realfehler > vorgabefehler) {
				ptr_adt_edit->found_matchp = 0;
				break;
			}
					//gefunden? (noetig, da Restsequenz
					//kuerzer
					//als Suchstringlaenge sein kann.)
					//Fehler + Uebereinstimmungen = Laenge
					//des Matchpatterns
			if(realfehler+equal_chars == matchp_len) {
				ptr_adt_edit->found_matchp = 1;
				ptr_adt_edit->mistakes_found = realfehler;
				break;
			}

			seq_loop_ptr++;		//  Ende der Sequenz erreicht?
			if( *seq_loop_ptr == '\0') {		//
				seq_empty_var = 1;
				break;
			}

		} // end for()

		if( *(seq_loop_ptr+1) == '\0') {
			ptr_adt_search->replace_loop_sequence =
							    ADT_STOPP_REPLACE;
		}
		//----------------------------------------search end found y/n

		//---------------------------------found cursorpos in sequence
		if( ptr_adt_edit->found_matchp == 1)  {
			ptr_adt_search->found_cursor_pos =
						  seq_loop_start - seq_anfang;
			break;
		}
		//-----------------------------------end cursorpos in sequence

		if( (ptr_adt_search->replace_option == ADT_REPLACE_ONLY) ||
		    (ptr_adt_search->replace_option ==
					       ADT_REPLACE_AND_SEARCH_NEXT)) {
			break;		 //for schleife nur einmal ausfuehren.
		}
	} // end for()
	//----------------------------------------------------------end search

	return 0;
}
//**************************************************************************




//**************************************************************************
//
//	Ueberprueft zuerst, ob die Sequenz ab der Cursorposition mit dem
//	Matchpattern uebereinstimmt. Wenn ja, dann erfolgt die Ersetzung.
//	Mit GAPS: durch remove(), und insert() der AD_SEQ-Klasse;
//	Ohne Gaps: direktes ueberschreiben im Sequenzpuffer, danach
//		ueberschreiben der Sequenzkopie im Cache.
//	Beide Varianten schreiben die geaenderte Sequenz mittels put() in
//	die Datenbank zurueck. (Security-Level nicht vergessen!!)
//
//**************************************************************************
AD_ERR * ADT_SEQUENCE::show_edit_replace(ADT_SEARCH *ptr_adt_search,
							ADT_EDIT *ptr_adt_edit)

{
	char	*seq_anfang, *seq_loop_start, *seq_loop_ptr;	// Sequenz
	char	*matchp_anfang;
	char	*replace_anfang, *replace_loop_ptr;	// Replace-String
	AD_ERR	*ad_err;

	matchp_anfang			=  ptr_adt_search->matchpattern;
	replace_anfang			=  ptr_adt_search->replace_string;
	seq_anfang			=  ptr_adt_search->seq_anfang;

	ptr_adt_search->string_replace = 0;
	long	var_start_cursor_pos	=
				       ptr_adt_search->replace_start_cursor_pos;

	seq_loop_start = seq_anfang + var_start_cursor_pos;
	seq_loop_ptr   = seq_loop_start;

	//============================================================REPLACE

	long	match_len	= strlen(ptr_adt_search->matchpattern);
	long	replace_len	= strlen(ptr_adt_search->replace_string);

	if( (match_len == replace_len)  ||
	    (ptr_adt_search->gaps == 0) || (ptr_adt_search->gaps == 1) ) {

		//----------------------------------------------------replace
		seq_loop_ptr = seq_loop_start;
		for( replace_loop_ptr = replace_anfang ;
			    *replace_loop_ptr != '\0' ; replace_loop_ptr++ ) {
			*seq_loop_ptr = *replace_loop_ptr;
			seq_loop_ptr++;
		} // end for()
	} // end if(match_len == replace_len)

	else {
		//-----------------------------------------------------remove
		ad_err = remove( strlen(matchp_anfang),
					(int)var_start_cursor_pos,1  );
		if (ad_err) {	// Fehler bei remove
			replace(ptr_adt_search->save_sequence,0,1);
//			printf("**** ERROR  REPLACE	remove() with GAPS\n");
			free(ptr_adt_search->save_sequence);
			return ad_err;
		}

		//-----------------------------------------------------insert
		else if( strlen(replace_anfang) != 0 ){
			ad_err = insert( replace_anfang,
					(int)var_start_cursor_pos ,1 );

			if (ad_err) {	// Fehler bei replace
				replace(ptr_adt_search->save_sequence,0,1);
//				printf("**** ERROR  REPLACE  	insert() with GAPS\n");
				free(ptr_adt_search->save_sequence);
				return ad_err;
			}
			ptr_adt_search->seq_anfang = get();
		}
	}
	//========================================================END REPLACE
	//-------------------------------------------------- Cursorpos Editor
	ptr_adt_edit->actual_cursorpos_editor =
				   ptr_adt_search->replace_start_cursor_pos +
						   strlen(replace_anfang) - 1;
	//-----------------------------------------------------END Cursorpos

	ptr_adt_search->string_replace = 1; 		//wir haben replaced!
	return 0;

}  // end fkt show_edit_replace()
//*************************************************************************




//*************************************************************************
//	Invert/Complement - Haelt und arbeitet auf der Sequenz
//*************************************************************************
AD_ERR * ADT_SEQUENCE::show_edit_seq_compl(
		 ADT_COMPLEMENT *ptr_adt_complement, ADT_EDIT *ptr_adt_edit) {

	AD_ERR	*ad_err;

	ptr_adt_complement->sequence = get();

	ptr_adt_complement->sequence_length =
						   (long)strlen(ptr_adt_complement->sequence);

	//-------------------------------------------------------- border check
	if( ptr_adt_complement->take_borders == YES ) {
		if( (ptr_adt_complement->sequence_length - 1) <
							ptr_adt_complement->right_border )  {
			ptr_adt_complement->right_border =
							ptr_adt_complement->sequence_length - 1;
		}
		if( (ptr_adt_complement->sequence_length - 1) <
							ptr_adt_complement->left_border )  {
			return 0;
		}
	}
	//---------------------------------------------------- end border check

	//-----------------------------------------------------fct complement()
	if( ptr_adt_complement->complement_seq == YES )  {
		show_edit_complement(ptr_adt_complement, ptr_adt_edit);
		//*****************************************************
	}
	//-------------------------------------------------end fct complement()

	//---------------------------------------------------------fct invert()
	if( ptr_adt_complement->invert_seq == YES )  {
		show_edit_invert(ptr_adt_complement, ptr_adt_edit);
		//*****************************************************
	}
	//-----------------------------------------------------end fct invert()

	if(	(ptr_adt_complement->seq_is_complemented == YES)  ||
		(ptr_adt_complement->seq_is_inverted == YES)			) {
		ptr_adt_edit->actual_cursorpos_editor = 0;
	}

	//----------------------------------- zurueckschreiben in die Datenbank
	if( ptr_adt_edit->db_status == ADT_DB_OPEN ) {
		ad_err = put();
		if(ad_err) {
//			printf("**** ERROR  ADT_COMPLEMENT, no rewrite into DB\n");
			return  ad_err;
		}

		show_update();
	}
	//---------------------------------------------------end write into DB

//----------------------------------------------------------------------------
//testen
#if 0
char* seq_loop   = ptr_adt_complement->sequence;
printf("\n================================================================\n");
printf("Species-name:		%s\n", ptr_adt_complement->species_name );
printf("Alignment-type:		%s == %d\n", ptr_adt_complement->alignment_type,
								ptr_adt_complement->adt_acid);
printf("Alignment-name:		%s\n", ptr_adt_complement->alignment_name );
printf("SEQUENCE:\n");
//printf("SEQUENCE:   (ab Cursorpos 200)\n");
	//seq_loop+=200;
	for(int k=0; (k<100) && (*seq_loop); k++) {
		printf("%c", *seq_loop);
		seq_loop++;
	}
	printf("\n");
printf("alignment_length:	%d\n", ptr_adt_complement->alignment_length );
printf("Borders [left..right]:	[%d  ..  %d]\n",
	ptr_adt_complement->left_border, ptr_adt_complement->right_border );
printf("seq_is_complemented:	%d\n", ptr_adt_complement->seq_is_complemented );
printf("seq_is_inverted:	%d\n", ptr_adt_complement->seq_is_inverted );
printf("which_button:	%d\n", ptr_adt_complement->which_button );
#endif
//end test
//---------------------------------------------------------------------------
	return 0;

} // end show_edit_seq_compl()
//*************************************************************************




//*************************************************************************
//	Fkt bildet das Complement
//*************************************************************************

AD_ERR * ADT_SEQUENCE::show_edit_complement(ADT_COMPLEMENT *ptr_adt_complement,
										 ADT_EDIT *ptr_adt_edit) {
	char		*seq_start, *right_border;
	char		*compl_start, *compl_loop;

	seq_start    =  ptr_adt_complement->sequence;
	compl_start  =  ptr_adt_complement->sequence;		//vorbelegung

	right_border	 = seq_start + max_value;
	//--------------------------------------------------------ab Cursorpos
	if(ptr_adt_complement->take_cursor == YES)  {
		compl_start  = seq_start + ptr_adt_edit->actual_cursorpos_editor;
	}
	//-------------------------------------------------------end Cursorpos

	//-----------------------------------------------------------Intervall
	if(ptr_adt_complement->take_borders == YES)  {
		compl_start  = seq_start + ptr_adt_complement->left_border;
		right_border = seq_start + ptr_adt_complement->right_border;
	}
	//-------------------------------------------------------end Intervall

	//-------------------------------------------------erledigt complement
	for( compl_loop = compl_start;
	     (*compl_loop) && (compl_loop <= right_border) ; compl_loop++ )  {
		*compl_loop = ptr_adt_complement->char_array[(int)*compl_loop];
	}
	ptr_adt_complement->seq_is_complemented = YES;
	//------------------------------------------------------end complement

	return 0;

} // end fct show_edit_complement()
//*************************************************************************



//*************************************************************************
//	Fkt fuehrt Invertierung durch  ( = REVERT )
//*************************************************************************

AD_ERR * ADT_SEQUENCE::show_edit_invert(ADT_COMPLEMENT *ptr_adt_complement,
										 ADT_EDIT *ptr_adt_edit) {
    char  source;

	char		*right_border;
	char		*sequence_loop_start, *sequence_loop, *sequence;
	char		*seq_buffer, *seq_buffer_loop_start, *seq_buffer_loop;
	long		*index_buffer, *index_buffer_loop_start, *index_buffer_loop;

	long		char_counter	= 0;

	sequence		= ptr_adt_complement->sequence;
	right_border	= sequence + max_value;

	//------------------------------------------------------ make Seq/Puffer
	ptr_adt_complement->complement_buffers();
	//-------------------------------------------------- end make Seq/Puffer

	seq_buffer		= ptr_adt_complement->sequence_buffer;
	index_buffer		= ptr_adt_complement->index_buffer;

	sequence_loop_start		= sequence;
	seq_buffer_loop_start	= seq_buffer;
	index_buffer_loop_start	= index_buffer;

	//--------------------------------------------------------ab Cursorpos
	if( ptr_adt_complement->take_cursor == YES ) {
		sequence_loop_start		+= ptr_adt_edit->actual_cursorpos_editor;
		seq_buffer_loop_start	+= ptr_adt_edit->actual_cursorpos_editor;
		index_buffer_loop_start	+= ptr_adt_edit->actual_cursorpos_editor;
	}
	//-------------------------------------------------------end Cursorpos

	//------------------------------------------------------- take Borders
	if( ptr_adt_complement->take_borders == YES ) {
		sequence_loop_start		+= ptr_adt_complement->left_border;
		seq_buffer_loop_start	+= ptr_adt_complement->left_border;
		index_buffer_loop_start	+= ptr_adt_complement->left_border;
		right_border 		= sequence + ptr_adt_complement->right_border;
	}
	//-------------------------------------------------------- end Borders

		seq_buffer_loop	= seq_buffer_loop_start;
		index_buffer_loop	= index_buffer_loop_start;

	//========================================== kopieren der Seq in Puffer
	for( sequence_loop = sequence_loop_start;
		( source = *sequence_loop) && (sequence_loop <= right_border) ;
											 sequence_loop++ ) {
		if( !((source == '-') || (source == '.')) )
		{	*( seq_buffer_loop++ )	= source;
			*( index_buffer_loop++ )	= sequence_loop - sequence;
			char_counter++;
		}
	}
	//=============================================== end kopieren in Puffer

	//========================================================== invertieren
	char		*first, *last;
	first 	= seq_buffer_loop_start;
	last		= seq_buffer + char_counter - 1;

	for( ; first < last ; first++, last-- )
	{	source	= *first;
		*first	= *last;
		*last	= source;
	}
	//====================================================== end invertieren

	//================================================ueberschreiben der Seq

	long		buffer_index = 0;

	//---------------------------------------------------------- take cursor
	if( ptr_adt_complement->take_cursor == YES ) {
		buffer_index	= ptr_adt_edit->actual_cursorpos_editor;
	}
	//----------------------------------------------------------- end cursor

	//--------------------------------------------------------- take borders
	if(ptr_adt_complement->take_borders == YES)  {
		buffer_index	= ptr_adt_complement->left_border;
		right_border 	= seq_buffer + ptr_adt_complement->right_border;
	}

	//---------------------------------------------------------- end borders

	sequence_loop = sequence_loop_start;

	if(ptr_adt_complement->remove_gaps_points == YES) {
		for( seq_buffer_loop = seq_buffer_loop_start ;
		    (source = *seq_buffer_loop) && (seq_buffer_loop <= right_border);
											 seq_buffer_loop++ ) {
			*(sequence_loop++) = source;
		}
	}
	else {		//////////// ptr_adt_complement->remove_gaps_points == NO
		for(	; (source = seq_buffer[buffer_index]) &&
			  	(seq_buffer[buffer_index] != '.');
			buffer_index++) {
			*(sequence + index_buffer[buffer_index]) = source;

		}
	}
	//=========================================== end ueberschreiben der Seq
	ptr_adt_complement->seq_is_inverted = YES;

	free(ptr_adt_complement->sequence_buffer);
	free((char *)ptr_adt_complement->index_buffer);
	ptr_adt_complement->sequence_buffer	= NULL;
	ptr_adt_complement->index_buffer		= NULL;

	return 0;

} // end fct show_edit_invert()
//*************************************************************************
