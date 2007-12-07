/*	edit_tools.cxx
*******************************************/

#include <stdio.h>
#include <stdlib.h>	// system
#include <string.h>

#include <arbdb.h>
#include <arbdb++.hxx>
#include <adtools.hxx>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include "edit.hxx"



//**************************************************************************
void create_tool_variables(AW_root *root,AW_default db1)
{
    root->awar_string( "tmp/tools/search/string", "" ,	    db1);
    root->awar_string( "tmp/tools/search/replace_string", "" ,	    db1);
    root->awar_int(    "tmp/tools/search/mismatches", 0,       db1);
    root->awar_int(    "tmp/tools/search/mistakes_found", 0,       db1);
    root->awar_int(    "tmp/tools/search/t_equal_u", 0,       db1);
    root->awar_int(    "tmp/tools/search/upper_eq_lower", 0,       db1);
    root->awar_int(    "tmp/tools/search/gaps", -1,       db1);
    //-----------------------------------------------------------complement
    root->awar_int(    "tmp/tools/complement/action", 0,       db1);
    root->awar_int(    "tmp/tools/complement/gaps_points", 0,       db1);
    root->awar_int(    "tmp/tools/complement/left", 0,       db1);
    root->awar_int(    "tmp/tools/complement/right", 0,       db1);
}
//**************************************************************************




//**************************************************************************
//	Search/Replace  Liefert alle Sequenzen eines Bereiches.
//**************************************************************************
void search_get_area( AED_window *aedw, AED_area_entry  *get_area_current,
		      ADT_SEARCH *ptr_adt_search, ADT_EDIT *ptr_adt_edit ) {

    AD_ERR	*ad_err;

	//================================Sequenzsuche solange AREA nicht leer.
    while( get_area_current != NULL ) 	{
	ADT_SEQUENCE 	*get_area_adt_sequence;

	if( ptr_adt_search->replace_option != ADT_NO_REPLACE ) {
	    if (aw_status()) break;
	}
	//----------------------------------------selektierte sequence
	if(get_area_current->is_selected != 0) {
	    ptr_adt_edit->selection = 1;
	    // im Alignement selektierten String gefunden
	}
	//---------------------------------------------------end selec

	//-------------------------------------------func aufruf/fehler
	get_area_adt_sequence = get_area_current->adt_sequence;

	if( (ptr_adt_edit->selection == 1) &&
	    (get_area_adt_sequence != NULL) ){
	    ad_err = get_area_adt_sequence->
		show_edit_seq_search(ptr_adt_search, ptr_adt_edit);
	    //###############################################

	    if(ad_err) {
		//		 			printf("**** ERROR ADT_SEARCH");
		//					printf("	show_edit_replace()\n");
		aw_message(ad_err->show());
		return;
	    } // end if(ad_err)

	} // end if (selection)
	//---------------------------------------------end func aufruf

	//------------------------------------------REPLACE REST EDITOR
	if(ptr_adt_search->replace_option == ADT_REPLACE_REST_EDITOR) {
	    ptr_adt_edit->found_matchp = 0;
	    ptr_adt_search->replace_loop_sequence =
		ADT_DONT_STOPP_REPLACE;
	}
	//-----------------------------------------------------END REST

	//-------------------------------------------get next Sequence
	if( ptr_adt_edit->found_matchp == 0 ) {
	    if(ptr_adt_search->search_direction ==
	       ADT_SEARCH_FORWARD) {
		get_area_current = get_area_current->next;
	    }
	    else {
		get_area_current = get_area_current->previous;
	    }

	}
	//----------matchpattern found
	else {
	    break;
	}
	//------------------------------------------------------------

    } //===========================================end while: Sequenzsuche


    //---------------------------------------------------Eintrag in Editor
    if(ptr_adt_edit->found_matchp == 1) {
	set_cursor_to( aedw,
		       (int)ptr_adt_edit->actual_cursorpos_editor,
		       get_area_current );
    }
    //--------------------------------------------------ENDE Editoreintrag

} // end func: search_get_area()
//**************************************************************************




//**************************************************************************
//    Aufruf der Dialogbox > search/replace < + deren Funktionalitaeten
//**************************************************************************
void edit_tool_search_cb(AW_window *aw, AED_window *aedwindow,
			 ADT_BUTTON pressed_button ){

    AW_root 		*awroot = aw->get_root();
    ADT_SEARCH	adt_search;
    ADT_EDIT		adt_edit;
    AD_ERR			*adt_error;

    long	mismatches 	= awroot->awar("tmp/tools/search/mismatches")->read_int(); //erlaubte Fehler
    char	*search		= awroot->awar("tmp/tools/search/string")->read_string(); //Suchstring
    char	*repl_str	= awroot->awar("tmp/tools/search/replace_string")->read_string(); //replace String
    long	t_equal_u	= awroot->awar("tmp/tools/search/t_equal_u")->read_int(); //t und u gleich?
    long	upper_eq_lower	= awroot->awar("tmp/tools/search/upper_eq_lower")->read_int(); //GROSS/klein beachten?
    long	gaps		= awroot->awar("tmp/tools/search/gaps")->read_int(); //Gaps beachten?

    adt_search.matchpattern		= search;	//  Suchstring
    adt_search.replace_string	= repl_str;
    adt_search.gaps 		= gaps;		//  Mit oder ohne Gaps

    ADT_SEARCH_DIRECTION	direction = ADT_SEARCH_FORWARD;
    ADT_REPLACE_OPTION	option = ADT_NO_REPLACE;

    switch(pressed_button) {

				// SEARCH-Buttons
	case BUTTON_SEARCH_ALL:
	    direction = ADT_SEARCH_ALL;
	    break;
	case BUTTON_SEARCH_FORWARD:
	    direction = ADT_SEARCH_FORWARD;
	    break;
	case BUTTON_SEARCH_BACKWARD:
	    direction = ADT_SEARCH_BACKWARD;
	    break;

				// REPLACE-Buttons
	case BUTTON_REPLACE_ONLY:
	    option	  = ADT_REPLACE_ONLY;
	    break;
	case BUTTON_REPLACE_AND_SEARCH_NEXT:
	    direction = ADT_SEARCH_FORWARD;
	    option	  = ADT_REPLACE_AND_SEARCH_NEXT;
	    break;
	case BUTTON_SEARCH_AND_REPLACE_NEXT:
	    direction = ADT_SEARCH_FORWARD;
	    option	  = ADT_SEARCH_AND_REPLACE_NEXT;
	    break;
	case BUTTON_REPLACE_REST_SEQUENCE:
	    direction = ADT_SEARCH_FORWARD;
	    option	  = ADT_REPLACE_REST_SEQUENCE;
	    break;
	case BUTTON_REPLACE_REST_EDITOR:
	    direction = ADT_SEARCH_FORWARD;
	    option	  = ADT_REPLACE_REST_EDITOR;
	    break;
	default:
	    GB_warning("Unknown case in switch in search");
	    // Default ueber Konstruktor von ADT_SEARCH
    }

    if(option >= 0) {
	adt_search.replace_option = option;
    }
    //Mehr oder gleichviele Fehler erlaubt
    //als Suchstring lang?
    if( strlen(search) == 0 && option < 0 ) {
	aw_message("*********   Please insert Matchpattern!   *********");
	free(search);
	return;
    }

    /*	if( option >= 0 ) {
		if( strlen(repl_str) == 0 ) {
			aw_message("*********   Please insert Replace-String!   *********");
			free(search);
			free(repl_str);
			return;
		}
	}
*/

if( option >= 0 && adt_search.gaps >= 0 ) {
    if( strlen(adt_search.matchpattern) !=
	strlen(adt_search.replace_string) ) {
	aw_message("*********   WITHOUT GAPS   *********\n    Search_String length must be\n\n        >>>>   EQUAL   <<<<\n\n     as Replace_String length!");
	free(search);
	free(repl_str);
	return;
    }
}

 if( (mismatches > 0) && (strlen(search) <= (unsigned long)mismatches)) {
     aw_message("*********   Mismatches must be lower as Search_String length!   *********");
     free(search);
     return;
 }

 //--------------------------------------------Anzahl erlaubter Fehler;
 adt_search.mistakes_allowed = mismatches;
 //-------------------------------------------------------------------

	//---------------------------------------Suchen ja, aber im Editor ist
						//noch nichts markiert!
 if(aedwindow->one_area_entry_is_selected == AW_FALSE) {

     //-------------------------------------no selection no replace
     if( adt_search.replace_option != ADT_NO_REPLACE )   {
	 aw_message("There`s nothing  SELECTED !!");
	 free(search);
	 free(repl_str);
	 return;
     }
     //---------------------------------------------end select-repl

     adt_search.search_direction=ADT_SEARCH_FORWARD;
     adt_search.search_start_cursor_pos = 0;
     adt_edit.selection=1;
 }
 //--------------------------------------------------------------------

	//-------------------------------------Hole Cursorposition der Sequenz;
 if(aedwindow->cursor_is_managed) {
     adt_edit.actual_cursorpos_editor = (long) aedwindow->cursor;
 }
 //--------------------------------------------------------------------

	//---------------------------------------Suchrichtung, Start der Suche;
 if(direction == ADT_SEARCH_ALL) {
     adt_search.search_direction=ADT_SEARCH_FORWARD;
     adt_search.search_start_cursor_pos = 0;
     adt_edit.actual_cursorpos_editor = -1;
     adt_edit.selection=1;
 }
 else{
     adt_search.search_direction = direction;
     adt_search.search_start_cursor_pos =
	 adt_edit.actual_cursorpos_editor + adt_search.search_direction;
 }				// Cursorpos. +/-1
 //--------------------------------------------------------------------

	//-------------------------------------------- t und u gleichbehandeln;
 if(t_equal_u) {
     adt_search.t_equal_u = 1;	// JA
 }
 //--------------------------------------------------------------------

	//----------------------------------------------- GROSS/klein beachten;
 if(upper_eq_lower) {
     adt_search.upper_eq_lower = 1;	// JA
 }
 //--------------------------------------------------------------------

 adt_search.compile();		   //Stellt Zeichensatz zur Verfuegung

 //============================================== REPLACE => DB oeffnen
 AD_MAIN *database_functions = aedwindow->root->ad_main;

 if( adt_search.replace_option != ADT_NO_REPLACE ) {
     adt_error = database_functions->begin_transaction();
     if(adt_error) {
	 aw_message("ADT_SEARCH::begin_transaction failed");
     }
     adt_edit.db_status = ADT_DB_OPEN;
 }
 // ========================================================== DB offen

	//--------------------------------------------Funktionsaufrufe liefern
					//die Sequenzen der drei Bereiche:
					//Top, Middle, Bottom, ab !! der
					//markierten Sequenz
 if( adt_search.replace_option != ADT_NO_REPLACE ) {
     aw_openstatus("SEARCH & REPLACE");
 }

 if(adt_search.search_direction == ADT_SEARCH_FORWARD) {
     search_get_area( aedwindow, aedwindow->area_top->first,
		      &adt_search, &adt_edit );

     if( adt_edit.found_matchp == 0 ) {
	 search_get_area( aedwindow,
			  aedwindow->area_middle->first,
			  &adt_search, &adt_edit );
     }

     if( adt_edit.found_matchp == 0 ) {
	 search_get_area( aedwindow,
			  aedwindow->area_bottom->first,
			  &adt_search, &adt_edit );
     }
 }else {	// adt_search.search_direction == ADT_SEARCH_BACKWARD
     search_get_area( aedwindow, aedwindow->area_bottom->last,
		      &adt_search, &adt_edit );

     if( adt_edit.found_matchp == 0 ) {
	 search_get_area( aedwindow, aedwindow->area_middle->last,
			  &adt_search, &adt_edit );
     }

     if( adt_edit.found_matchp == 0 ) {
	 search_get_area( aedwindow, aedwindow->area_top->last,
			  &adt_search, &adt_edit );
     }
 }

 aw_closestatus();

	//-------------------------------------------------------------------

	// ======================================================DB schliessen
 if( adt_edit.db_status == ADT_DB_OPEN ) {
     adt_error = database_functions->commit_transaction();

     if(adt_error) {
	 aw_message("ADT_SEARCH::commit_transaction failed");
     }
     adt_edit.db_status = ADT_DB_CLOSED;
 }
 // =====================================================DB geschlossen

 	// ----------------------------------------------------Fehlermeldungen
 if( (adt_edit.found_matchp == 0) && (adt_search.string_replace == 0)) {
     aw_message("********   Nothing found!   *********");
 }

 if( adt_edit.seq_equal_match == 0 ) {
     aw_message("*********   For Replace_Only   *********\nCursor moved  ???\nFrom cursorposition on,\nSequence and Matchpattern are not equal! ");
 }

 if( adt_edit.replace_overflow == 1 ) {
     aw_message("*********   Didn`t you SEARCH ???   *********\nOr Replace-String is longer as Rest-Sequence !!!");
 }
 //-------------------------------------------------end Fehlermeldungen

	//-------------------------------------------------Schreiben in Editor
					//Schreibt Anzahl der gefundenen
					//Fehler in den Editor
 awroot->awar("tmp/tools/search/mistakes_found")->write_int( adt_edit.mistakes_found);
 //------------------------------------------------------end schreiben

	//-------------------------------------------------Speicher freigeben
 free(search);
 free(repl_str);
 //-------------------------------------------------------------------


} // end edit_tool_search_cb()
//**************************************************************************




//***************************************************************************
//	Sucht die richtige Sequenz fuer Complement/Invertieren
//***************************************************************************
void compl_get_area(  AED_window *aedw, AED_area_entry  *get_area_current, ADT_COMPLEMENT *ptr_adt_complement, ADT_EDIT *ptr_adt_edit ) {

    AD_ERR	*ad_err;
    AWUSE(aedw);

    while(get_area_current != NULL)
    {
	ADT_SEQUENCE 	*get_area_adt_sequence;
	AD_SPECIES	*ptr_ad_species;

	//----------------------------------------selektierte sequence
	if(get_area_current->is_selected != 0) {
	    ptr_adt_edit->selection = 1;
	    // im Alignement selektierten String gefunden
	}
	//---------------------------------------------------end selec

	//-------------------------------------------func aufruf/fehler
	get_area_adt_sequence = get_area_current->adt_sequence;

	if( (ptr_adt_edit->selection == 1) && (get_area_adt_sequence != NULL) ) {
	    //---------------------------------------------- Species name
	    ptr_ad_species = get_area_current->ad_species;
	    ptr_adt_complement->species_name = ptr_ad_species->name();
	    //------------------------------------------ end Species name

	    ad_err = get_area_adt_sequence->show_edit_seq_compl( ptr_adt_complement, ptr_adt_edit);
	    //###############################################

	    if(ad_err) {
		//		 		printf("**** ERROR ADT_COMPLEMENT");
		//				printf("    show_edit_seq_compl()\n");
		aw_message(ad_err->show());
		return;
	    } // end if(ad_err)

	    //-------------------------------------------get next Sequence
	    if((ptr_adt_complement->which_button == BUTTON_COMPL_BLOCK) ||
	       (ptr_adt_complement->which_button == BUTTON_COMPL_REST_SEQUENCE) ||
	       (ptr_adt_complement->which_button == BUTTON_COMPL_SEQUENCE))
	    {
		break;
	    }
	    //------------------------------------------------------------
	} // end if (selection)
	//---------------------------------------------end func aufruf

	get_area_current = get_area_current->next;

    } //===========================================end while: Sequenzsuche

} // end compl_get_area()
//***************************************************************************




//***************************************************************************
//  Bildet das Complement und invertiert die Sequence
//***************************************************************************
void edit_tool_complement_cb(AW_window *aw, AED_window *aedwindow,
			     ADT_BUTTON pressed_button ){

    AW_root 			*awroot = aw->get_root();
    AD_ERR			*adt_error;
    ADT_COMPLEMENT		adt_complement;
    ADT_EDIT			adt_edit;


    switch(pressed_button) {

	case NO_BUTTON:
	    aw_message("Oh Sweety - You are really in trouble now.");
	    return;
	    //break;
	case BUTTON_COMPL_BLOCK:
	    adt_complement.which_button = BUTTON_COMPL_BLOCK;
	    adt_complement.take_borders = YES;
	    break;
	case BUTTON_COMPL_REST_SEQUENCE:
	    adt_complement.which_button = BUTTON_COMPL_REST_SEQUENCE;
	    adt_complement.take_cursor  = YES;
	    break;
	case BUTTON_COMPL_SEQUENCE:
	    adt_complement.which_button = BUTTON_COMPL_SEQUENCE;
	    break;
	case BUTTON_COMPL_REST_TEXT:
	    adt_complement.which_button = BUTTON_COMPL_REST_TEXT;
	    adt_complement.take_cursor  = YES;
	    break;
	case BUTTON_COMPL_ALL:
	    adt_complement.which_button = BUTTON_COMPL_ALL;
	    adt_edit.selection	= 1;
	    break;
	default:
	    GB_warning("Unknown case in switch in search");
    }

    if( (!(adt_complement.which_button ==  BUTTON_COMPL_ALL)) &&
	(aedwindow->one_area_entry_is_selected == AW_FALSE)
	)  {
	aw_message("**** ERROR:    no sequence selected !");
	return;
    }

    //	adt_complement.ad_species = aedwindow->selected_area_entry->ad_species;
    adt_complement.alignment_type = aedwindow->alignment->type();
    adt_complement.alignment_name = aedwindow->alignment->name();
    adt_complement.alignment_length = aedwindow->alignment->len();


    //-----------------------------------------------------complement/invert
    long	action	= awroot->awar("tmp/tools/complement/action")->read_int();
    if( (action == 0) || (action == 1) )   {		// complement
	adt_complement.complement_seq = YES;
    }
    if( (action == 0) || (action == 2) )   {		// invert
	adt_complement.invert_seq = YES;
	//adt_complement.complement_buffers();		// wird in der fct aufger
    }
    //-------------------------------------------------end complement/invert

	//----------------------------------------------------remove GAPS/points
    long	gaps_points  = awroot->awar("tmp/tools/complement/gaps_points")->read_int();
    if(gaps_points == 1)  {			// DEFAULT:  0 = NO !!
	adt_complement.remove_gaps_points = YES;
    }
    //------------------------------------------------end remove GAPS/points

	//-------------------------------------Hole Cursorposition der Sequenz;
    if(aedwindow->cursor_is_managed) {
	adt_edit.actual_cursorpos_editor = (long) aedwindow->cursor;
    }
    else {
	adt_edit.actual_cursorpos_editor = 0;
    }
    //--------------------------------------------------------end cursorpos

	//---------------------------------------------------------take borders
    if(adt_complement.take_borders == YES)  {
	long	 left_border  = awroot->awar("tmp/tools/complement/left")->read_int();
	long	 right_border = awroot->awar("tmp/tools/complement/right")->read_int();

	if(left_border < 0)  {
	    aw_message("**** ERROR:     left border >= 0 !");
	    return;
	}
	if(left_border > (adt_complement.alignment_length - 1))  {
	    aw_message("**** ERROR:     left border <= alignment length !");
	    return;

	}
	if(right_border < left_border)  {
	    aw_message("**** ERROR:     right border >= left border !");
	    return;
	}
	if(right_border > (adt_complement.alignment_length - 1))  {
	    right_border = (adt_complement.alignment_length - 1);
	}

	adt_complement.left_border	= left_border;
	adt_complement.right_border	= right_border;
    }
    //----------------------------------------------------------end borders

	//---------------------------------------------Complement dna/rna/... ?
    if( adt_complement.complement_seq == YES )  {
	if( strcmp(adt_complement.alignment_type,"rna")==0 )  {
	    adt_complement.adt_acid = ADT_ALITYPE_RNA;
	}
	else if( strcmp(adt_complement.alignment_type,"dna")==0 )  {
	    adt_complement.adt_acid = ADT_ALITYPE_DNA;
	}
	else if( strcmp(adt_complement.alignment_type,"ami")==0 )  {
	    //adt_complement.adt_acid = ADT_ALITYPE_AMI;
	    aw_message("NO COMPLEMENT:	Alignment Type is AMINO-ACID!");
	    return;
	}
	else {
	    //adt_complement.adt_acid = ADT_ALITYPE_UNDEFINED;
	    aw_message("NO COMPLEMENT:	Alignment Type is UNDEFINED!");
	    return;
	}
	adt_complement.complement_compile();
    }
    //---------------------------------------------------end compl dna/rna/

	//============================================== REPLACE => DB oeffnen
    AD_MAIN *database_functions = aedwindow->root->ad_main;

    adt_error = database_functions->begin_transaction();

    if(adt_error) {
	aw_message("ADT_SEARCH::begin_transaction failed");
    }
    adt_edit.db_status = ADT_DB_OPEN;
    // ========================================================== DB offen

    //--------------------------------------------Funktionsaufrufe liefern
    //die Sequenzen der drei Bereiche:
    //Top, Middle, Bottom, ab !! der
    //markierten Sequenz

    //soll entfallen	compl_get_area( aedwindow, aedwindow->area_top->first,
    //						  &adt_complement, &adt_edit );

    compl_get_area( aedwindow, aedwindow->area_middle->first,
		    &adt_complement, &adt_edit );

    //soll entfallen	compl_get_area( aedwindow, aedwindow->area_bottom->first,
    //						  &adt_complement, &adt_edit );
    //-------------------------------------------------------------------

	// ======================================================DB schliessen
    adt_error = database_functions->commit_transaction();

    if(adt_error) {
	aw_message("ADT_SEARCH::commit_transaction failed");
    }
    adt_edit.db_status = ADT_DB_CLOSED;
    // =====================================================DB geschlossen

} // end edit_tool_complement_cb()
//***************************************************************************




//***************************************************************************
//	gemeinsam in SEARCH und REPLACE
//***************************************************************************
AW_window *common_window_features( AW_window_simple *aws,
				   AED_window *aedwindow )
{
    aws->label_length( 7 );
    aws->button_length( 14 );

    aws->at( "close" );
    aws->callback     ( (AW_CB0)AW_POPDOWN  );
    aws->create_button( "CLOSE", "CLOSE", "C" );

    aws->label_length( 7 );
    aws->button_length( 18 );

    aws->at("start");
    aws->callback     ( (AW_CB)edit_tool_search_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_SEARCH_FORWARD );// +1
    aws->create_button( "FIND_NEXT", "FIND NEXT     ", "1" );

    aws->at("start_rev");
    aws->callback     ( (AW_CB)edit_tool_search_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_SEARCH_BACKWARD );// -1
    aws->create_button( "FIND_BACK", "FIND BACKWARD ", "2" );

    aws->at("start_all");
    aws->callback     ( (AW_CB)edit_tool_search_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_SEARCH_ALL );	//  0
    aws->create_button( "FIND_ALL", "FIND ALL      ", "3" );

    aws->at("search");
    aws->create_input_field("tmp/tools/search/string",49);

    aws->at("mismatches");
    aws->create_input_field("tmp/tools/search/mismatches",3);

    aws->at("mistakes");
    aws->create_button(0,"tmp/tools/search/mistakes_found");

    aws->at("t_equal_u");
    aws->create_option_menu("tmp/tools/search/t_equal_u",0,"");
    aws->insert_default_option("T <> U","N",0);
    aws->insert_option("T = U ","Y",1);
    aws->update_option_menu();

    aws->at("upper_eq_lower");
    aws->create_option_menu("tmp/tools/search/upper_eq_lower",0,"");
    aws->insert_default_option("a <> A","N",0);
    aws->insert_option("a = A ","Y",1);
    aws->update_option_menu();

    aws->at("gaps");
    aws->create_option_menu("tmp/tools/search/gaps",0,"");
    aws->insert_default_option	(" 'A-A' <> 'AA'        ","D",-1);
    aws->insert_option		(" IGNORE GAPS IN SEQ   ","S",0);
    aws->insert_option		(" IGNORE GAPS IN SEARCH","M",2);
    aws->insert_option		(" IGNORE ALL GAPS       ","B",1);
    aws->update_option_menu();

    return aws;
}
//***************************************************************************



//***************************************************************************
//
//***************************************************************************
AW_window *create_tool_search( AW_root *root, AED_window *aedwindow )
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "SEARCH_STRING","EDIT SEARCH");
    aws->load_xfig("ed_searc.fig");

    aws->label_length( 7 );
    aws->button_length( 14 );


    common_window_features( aws, aedwindow );

    return aws;
}
//***************************************************************************



//***************************************************************************
//
//***************************************************************************
AW_window *create_tool_replace( AW_root *root, AED_window *aedwindow )
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "REPLACE_STRING", "EDIT REPLACE");
    aws->load_xfig("ed_repla.fig");

    aws->label_length( 7 );
    aws->button_length( 14 );

    //	aws->at( "reopen" );
    //	aws->callback     ( AW_POPUP, (AW_CL)create_tool_replace,
    //							(AW_CL)aedwindow  );
    //	aws->create_button( "REOPEN", "R" );

    aws->at("replace");
    aws->create_input_field("tmp/tools/search/replace_string",49);

    aws->button_length( 28 );
    aws->at("start_replace");
    aws->callback     ( (AW_CB)edit_tool_search_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_REPLACE_ONLY );
    aws->create_button( "REPLACE", "REPLACE", "4" );

    aws->at("replace_and_find_next");
    aws->callback     ( (AW_CB)edit_tool_search_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_REPLACE_AND_SEARCH_NEXT );
    aws->create_button( "REPLACE_FIND", "REPLACE THEN FIND", "5" );

    aws->at("find_and_replace_next");
    aws->callback     ( (AW_CB)edit_tool_search_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_SEARCH_AND_REPLACE_NEXT );
    aws->create_button( "FIND_REPLACE", "FIND THEN REPLACE", "6" );

    aws->at("replace_rest_sequence");
    aws->callback     ( (AW_CB)edit_tool_search_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_REPLACE_REST_SEQUENCE );
    aws->create_button( "REPLACE_TO_END_OF_SEQUENCE", "REPLACE TO END OF SEQUENCE", "7" );

    aws->at("replace_rest_editor");
    aws->callback     ( (AW_CB)edit_tool_search_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_REPLACE_REST_EDITOR );
    aws->create_button( "REPLACE_TO_END_OF_EDITOR", "REPLACE TO END OF EDITOR", "8" );

    common_window_features( aws, aedwindow );
    return aws;
}
//***************************************************************************



//***************************************************************************
//
//***************************************************************************
AW_window *create_tool_complement( AW_root *root, AED_window *aedwindow )
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "COMPLEMENT_SEQUENCE", "COMPLEMENT  and  REVERS");
    aws->load_xfig("ed_compl.fig");

    aws->label_length( 7 );
    aws->button_length( 14 );

    aws->at( "close" );
    aws->callback     ( (AW_CB0)AW_POPDOWN  );
    aws->create_button( "CLOSE", "CLOSE", "C" );


    aws->at("action");
    aws->create_option_menu("tmp/tools/complement/action",0,"");
    aws->insert_default_option(" COMPLEMENT AND REVERS ","1",0);
    aws->insert_option		 (" COMPLEMENT             ","2",1);
    aws->insert_option		 (" REVERS                ","3",2);
    aws->update_option_menu();

    aws->at("gaps_points");
    aws->id("remove_option");
    aws->create_toggle("tmp/tools/complement/gaps_points");

    aws->at("left");
    aws->create_input_field("tmp/tools/complement/left",8);

    aws->at("right");
    aws->create_input_field("tmp/tools/complement/right",8);

    aws->button_length( 17 );
    aws->at("block");
    aws->callback     ( (AW_CB)edit_tool_complement_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_COMPL_BLOCK );
    aws->create_button( "COMPLEMENT_PART", "SEQUENCE PART", "P" );

    aws->at("rest_seq");
    aws->callback     ( (AW_CB)edit_tool_complement_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_COMPL_REST_SEQUENCE );
    aws->create_button( "COMPLEMENT_REST", "REST SEQUENCE", "1" );

    aws->at("sequence");
    aws->callback     ( (AW_CB)edit_tool_complement_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_COMPL_SEQUENCE );
    aws->create_button( "COMPLEMENT_SEQUENCE", "SEQUENCE", "2" );

    aws->at("rest_text");
    aws->callback     ( (AW_CB)edit_tool_complement_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_COMPL_REST_TEXT );
    aws->create_button( "COMPLEMENT_REST_EDITOR", "REST EDITOR", "3" );

    aws->at("all");
    aws->callback     ( (AW_CB)edit_tool_complement_cb, (AW_CL)aedwindow,
			(AW_CL)BUTTON_COMPL_ALL );
    aws->create_button( "COMPLEMENT_ALL", "ALL", "4" );

    return aws;
}
//***************************************************************************




