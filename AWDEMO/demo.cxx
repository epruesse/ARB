#include <stdio.h>
#include <stdlib.h> // wegen exit();
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <demo.hxx>
#include <memory.h>
// #include <malloc.h>
#include <string.h>

AW_HEADER_MAIN;



/******************************************************************************************/




void terminate_program( AW_window *aw, AW_CL cd1, AW_CL cd2 ) {
AWUSE(aw);AWUSE(cd1);AWUSE(cd2);

	exit( 0 );
}



AW_window *create_demo_window_1( AW_root *root, AW_CL cd2 ) {
AWUSE(cd2);

	AW_window_simple *aws = new AW_window_simple;
	aws->init( root, "Buttons Probe", 0, 0, 10, 10 );
	aws->load_xfig("demo/dm.fig");
	aws->button_length( 10 );


	aws->at ( "button1" );
	aws->callback     ( terminate_program, (long)0, (long)0 );
	aws->create_button ( 0, "button 1", "1" );

	aws->at ( "button2" );
	aws->callback     ( terminate_program, (long)0, (long)0 );
	aws->create_button ( 0, "button 2", "1" );

	return (AW_window *)aws;
}




void create_main_window( AW_root *aw_root ) {

AW_window_menu_modes	*awmm;

	awmm = new AW_window_menu_modes;
	awmm->init( aw_root, "DEMO Main Window", 600, 500, 1, 1 );

	awmm->create_menu(       "1",   "Menu 1",     "1", "No Help",  AW_ALL );
	awmm->insert_menu_topic( "1.1", "CLOSE",     "C", "No Help", AW_ALL,    terminate_program,   (long)0, (long)0 );

	awmm->set_info_area_height(130);
	awmm->set_bottom_area_height(130);


	awmm->show();

}


/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/

int main(int argc,char **argv) {
AWUSE(argc);AWUSE(argv);

AW_root		*aw_root;
AW_default	aw_default;

	aw_root = new AW_root;

	aw_default = aw_root->open_default( ".arb_prop/demo.arb" );

	aw_root->init_variables( aw_default );

	aw_root->init( "DEMO Program" );


	create_main_window( aw_root );


	aw_root->main_loop();

}
